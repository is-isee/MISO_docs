(edu-practice-heat1d-03)=
# 03. GPU 版：配列とループを GPU へ移す

{ref}`02. MPI 版 <edu-practice-heat1d-02>` のプログラムを、GPU でも CPU でもビルドできる `heat1d_gpu` に書き換えます。
GPU のメモリ、`MISO_LAMBDA`、`this` の取り込みは {ref}`第 9 章 <edu-ch09>`、この課題での注意点は {ref}`第 10 章 <edu-ch10>` で説明しています。
MPI 版のソースをコピーして `03_gpu.cpp` とし、そこから書き換えます。

## 作るもの

### 計算場所の切り替え

- ソースの先頭で、`__CUDACC__` が定義されていれば `using Backend = backend::CUDA;`、そうでなければ `using Backend = backend::Host;` とします。
- 計算に使う配列を `Array1D<Real, Backend>` に、`for_each`・`reduce` の第 1 引数を `Backend{}` に変えます。
- ラムダはすべて `MISO_LAMBDA` で書き、数学関数は `util::sin` などを使います（`std::sin` は GPU で使えません）。

### ゴーストセルの交換

交換をまとめた構造体 `Halo` を作ります。メンバは次のとおりです。

| メンバ | 役割 |
|---|---|
| `int n` | このランクの物理セル数 |
| `int left, right` | 左右の隣のランク（なければ `MPI_PROC_NULL`） |
| `MPI_Comm comm` | 通信に使うコミュニケータ |
| `Array1D<Real, Backend> send, recv` | 計算する側の送受信用の配列（2 要素。`[0]` が左側、`[1]` が右側） |
| `Array1D<Real> h_send, h_recv` | ホストの送受信用の配列（2 要素） |

配列はコンストラクタの初期化子リストで確保し（{ref}`第 4 章 <edu-ch04>`）、ステップごとに確保し直しません。

メンバ関数 `void apply(Array1D<Real, Backend> &u)` は、次の順に処理します。

1. `for_each`（`Range1D{0, 1}`）で、`u[1]` を `send[0]` に、`u[n]` を `send[1]` に詰めます。
2. `h_send.copy_from(send);` でホストへ写します。
3. MPI 版と同じ 2 回の `MPI_Sendrecv` を、`h_send` と `h_recv` で行います。
   1 回目は `h_send[0]` を左隣へ送り、右隣から `h_recv[1]` に受け取ります。2 回目は `h_send[1]` を右隣へ送り、左隣から `h_recv[0]` に受け取ります。
   アドレスは `h_send.data()`、`h_send.data() + 1` のように渡せます。
4. `recv.copy_from(h_recv);` で計算する側へ写します。
5. `for_each`（`Range1D{0, 1}`）で、左隣があれば `u[0] = recv[0]`、なければ `u[0] = -u[1]` とし、右側も同様にします。

**ラムダの中でメンバ変数（`n`、`left`、`right` など）を使わないでください。**
ラムダの前で `const int n_ = n;`、`const bool has_left = (left != MPI_PROC_NULL);` のようにローカル変数にコピーし、ラムダの中ではそれを使います。

### 誤差と出力

- 誤差は `reduce(Backend{}, ...)` で求めます。戻り値はホストの値なので、そのまま `MPI_Allreduce` に渡せます。
- 書き出す前に、`Array1D<Real> u_host(n + 2);` を作り、`u_host.copy_from(u);` でホストへ写してから `MPI_Gather` します。
- 表示の 1 行目の先頭に、GPU 用のビルドなら `[gpu]`、CPU 用なら `[cpu]` を付けます。
  ファイル名も `result_N64_P2_gpu.txt` / `result_N64_P2_cpu.txt` のように区別します。

## ビルドの設定

CPU 用には、MPI 版と同じく `heat1d_gpu` を `03_gpu.cpp` から作り、`MISO::MISO` をリンクするだけです。

GPU 用には、`cmake` に `-DMISO_USE_CUDA=ON` を与えたとき、次の 2 つが行われるように `CMakeLists.txt` に書き足します。

- `add_subdirectory` で MISO を取り込む**前に**、CUDA を使えるようにする（`enable_language(CUDA)`）。
  その前に、対象の GPU の世代 `CMAKE_CUDA_ARCHITECTURES` を決めておきます（指定がなければ `native`）。
- `03_gpu.cpp` を CUDA のソースとしてコンパイルさせる（`set_source_files_properties(03_gpu.cpp PROPERTIES LANGUAGE CUDA)`）。

書き足す部分は、たとえば次の形になります。

```cmake
option(MISO_USE_CUDA "Enable CUDA support" OFF)
if(MISO_USE_CUDA)
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES native CACHE STRING "CUDA architectures")
    endif()
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
endif()
```

`MISO_USE_CUDA` は MISO の `miso/CMakeLists.txt` の設定と同じ名前なので、MISO 側でも CUDA が有効になり、`--expt-extended-lambda` などが付きます。
GPU 用のビルドは、CPU 用とは別のフォルダに作ります。

```bash
cmake -B build-gpu -S . -DMISO_DIR=/path/to/MISO -DMISO_USE_CUDA=ON
cmake --build build-gpu -j
```

## 確かめること

### CPU 用のビルド（GPU がなくてもできます）

1. `mpiexec -np P ./build/heat1d_gpu 64`（$P = 1, 2, 4$）の結果が、MPI 版の `result_N64_P<P>.txt` とビット単位で一致することを `cmp` で確かめます。
   一致すれば、`Backend` への書き換えと、送受信用の配列を経由する交換が正しく書けています。

### GPU 用のビルド

2. **誤差**：$N = 32, 64, 128, 256, 512$ を 1 ランクで実行し、`max error (exact)` が {ref}`逐次版 <edu-practice-heat1d-01>` の表と 4〜5 桁目まで一致すること、
   `max error (discrete)` が $10^{-11}$ 以下であることを確かめます。
3. **ランク数**：$P = 1, 2, 4$ で実行し、GPU 用のビルドどうしでファイルがビット単位で一致することを確かめます。
   GPU が 1 枚でも、複数のランクで同じ GPU を共有して実行できます（`miso::Env` が、ノード内のランク番号から使う GPU を選びます）。
4. **CPU との比較**：同じ $N$・$P$ の CPU の結果と、全セルの最大の差を求めます。

   ```bash
   paste result_N64_P1_cpu.txt result_N64_P1_gpu.txt \
     | awk '{d = $2 - $4; if (d < 0) d = -d; if (d > m) m = d} END {print m}'
   ```

   最大の差が $10^{-12}$ 程度以下なら一致とみなします。
   FMA や数学関数の実装の違いで最後の桁が違うことがあるので、ビット単位の一致は求めません（一致することもあります）。
   `max error (discrete)` の表示も、CPU と最後の桁が違うことがあります（たとえば `1.249e-14` と `1.247e-14`）。

実行時に `an illegal memory access was encountered` と表示されたら、まずラムダの中にメンバ変数や、ホストの配列（`Array1D<Real>`）のビューがないかを調べます。
その他のエラーは {ref}`参考資料「GPU で困ったとき」 <edu-ref-gpu-troubleshooting>` を参照してください。

## 次へ進む目安

CPU 用のビルドで MPI 版と一致し、GPU を使える場合は GPU 用のビルドで上の 2〜4 を確かめたら、熱伝導の実習は完了です。
{ref}`第 11 章 <edu-ch11>` へ進み、MISO 本体の時間発展を読みます。
