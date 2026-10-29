(edu-ref-gpu-troubleshooting)=
# GPU で困ったとき

GPU 向けのコードの基本は {ref}`第 9 章 <edu-ch09>` で説明しています。
ここでは、ビルドや実行で問題が起きたときに確かめることをまとめます。
ソースのパスは、MISO を clone したフォルダからのパスです。
エラーの文面は CUDA 12.9 の `nvcc` と RTX 4090 で確かめたもので、版によって少し変わることがあります。

## 1. CMake の設定で止まる

| 症状 | 確かめること |
|---|---|
| `No CMAKE_CUDA_COMPILER could be found` | `nvcc` に PATH が通っているか（`nvcc --version`）。通っていなければ `-DCMAKE_CUDA_COMPILER=/path/to/nvcc` で指定する |
| CPU 版から切り替えたら設定が反映されない | 同じビルドフォルダを使い回していないか。CPU 用と GPU 用でフォルダを分けるか、フォルダを消して作り直す |
| `CMAKE_CUDA_ARCHITECTURES` を指定したのに別の値になる | 自分の `CMakeLists.txt` で、`enable_language(CUDA)` より前にこの値を決めているか（第 9 章 4.4 節） |
| `native` で GPU が見つからないというエラー | ビルドする計算機に GPU がない（ログインノードなど）。使う GPU の値（`89` など）を明示する |

## 2. コンパイルできない

| エラーの文面（抜粋） | 原因と対処 |
|---|---|
| `__host__ or __device__ annotation on lambda requires --extended-lambda nvcc flag` | `MISO_LAMBDA` には `nvcc` の `--expt-extended-lambda`（`--extended-lambda` と同じ）が必要。`MISO::MISO` をリンクしているか、MISO の `MISO_USE_CUDA` を ON にしているかを確かめる |
| `The enclosing parent function ("work") for an extended __host__ __device__ lambda cannot have private or protected access within its class` | `MISO_LAMBDA` を `private`・`protected` のメンバ関数の中に書いている。`public` の関数か、クラスの外の関数へ移す |
| `... cannot be referenced -- it is a deleted function`（`Array1D` などのコピー） | ラムダの中で、所有する配列（`Array1D` など）を使っている。ラムダの前で `view()` を取り出し、View を使う |
| `calling a __host__ function("...") from a __host__ __device__ function("...") is not allowed`（警告 #20011） | ラムダやカーネルから、`__device__` の付いていない関数を呼んでいる。警告でもそのままにしない。自分の関数には `__host__ __device__` を付け、数学関数は `util::sqrt` などを使う（`miso/include/miso/utility.hpp`） |
| `declaration is incompatible with previous "s"` | テンプレートのカーネルで `extern __shared__ T s[];` と型付きで宣言している。MISO の `reduce_shared_memory` のように、バイト列で 1 回だけ宣言して読み替える（第 9 章のコラム、MISO issue #190） |
| g++ が `'__global__' does not name a type` を出す | カーネルを `#ifdef __CUDACC__` の外に書いているか、`.cpp` を `nvcc` に渡していない。CMake の `set_source_files_properties(... LANGUAGE CUDA)` を確かめる |

拡張ラムダ（`__host__ __device__` を付けたラムダ）には、ここに挙げた以外にも置き場所や取り込み方の制約があります。
利用している版の NVIDIA CUDA C++ Programming Guide（<https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html>）の extended lambda の節を確かめてください。

## 3. 実行時に止まる

### 3.1 よくあるエラー

| 表示 | 原因の候補 |
|---|---|
| `CUDA Error: an illegal memory access was encountered` | ラムダが `this` を取り込んでいる（メンバ変数を直接使っている）。ホストの配列の View を CUDA 版の `for_each` に渡している。添字が配列の範囲外 |
| `no kernel image is available for execution on the device` | `CMAKE_CUDA_ARCHITECTURES`（`nvcc -arch`）が使う GPU より新しい世代を指している。GPU の compute capability（`nvidia-smi --query-gpu=compute_cap --format=csv`）に合わせてビルドし直す |
| `CUDA driver version is insufficient for CUDA runtime version` | CUDA Toolkit がドライバより新しい。ドライバが対応する版の CUDA Toolkit を使う（`nvidia-smi` の右上に、ドライバが対応する CUDA の版が出る） |
| `No CUDA-capable devices were detected.` | MISO の `cuda::Env` が GPU を見つけられない。`nvidia-smi` で GPU が見えるか、環境変数 `CUDA_VISIBLE_DEVICES` で隠していないかを確かめる |

`illegal memory access` の中で最も見つけにくいのは `this` の取り込みです。
CPU ビルドでは正しく動き、HMM が有効な一部の GPU 環境でも偶然動くためです（MISO issue #170）。
ラムダの中で使っている名前が、ローカル変数か引数かを、宣言まで戻って確かめてください（第 9 章 5 節）。

### 3.2 エラーの場所を突き止める

カーネルの起動は非同期なので、`MISO_CUDA_CHECK` が表示するファイルと行は、エラーに**気づいた**場所です。
エラーが**起きた**カーネルは、それより前に起動したものです（第 9 章 7 節）。

**環境変数 `CUDA_LAUNCH_BLOCKING=1`** を付けて実行すると、カーネルの起動が終了まで待つようになり、
起動直後の `MISO_CUDA_CHECK(cudaGetLastError())` でエラーが報告されます。
第 9 章 5.1 の例では、表示が次のように変わりました。

```text
# run as is
CUDA Error: an illegal memory access was encountered .../miso/include/miso/array1d.hpp 109
# run with CUDA_LAUNCH_BLOCKING=1
CUDA Error: an illegal memory access was encountered .../miso/include/miso/execution.hpp 91
```

`array1d.hpp` は後の `copy_from` の中、`execution.hpp` は CUDA 版 `for_each` の起動直後の検査です。
さらに、`for_each` を呼んだ場所を調べるには、自分のコードの `for_each` の直後に `MISO_CUDA_CHECK(cudaDeviceSynchronize());` を一時的に足して範囲を狭めます。

**`compute-sanitizer`**（CUDA Toolkit に含まれる）で実行すると、不正な読み書きをしたカーネルとスレッドが表示されます。

```bash
compute-sanitizer ./this_capture
mpiexec -np 2 compute-sanitizer ./build/mhd2d_vortex --config=config.yaml   # with MPI
```

第 9 章 5.1 の例での表示（抜粋）です。

```text
========= Invalid __global__ read of size 4 bytes
=========     at void miso::for_each_kernel<Scaler::apply_wrong(miso::Array1DView<float>) const::[lambda(int) (instance 1)]>(miso::Range1D, T1)+0xd0
=========     by thread (0,0,0) in block (0,0,0)
=========     Access to 0x7fff1107e0a4 is out of bounds
```

`apply_wrong` の中のラムダを実行する `for_each_kernel` が、4 バイト（`float` のメンバ `factor`）を読もうとしたことが分かります。
`0x7fff...` で始まる番地は、Linux ではホストのスタック（ローカル変数を置く場所）によく使われる番地で、ホストのオブジェクトを読もうとした手がかりになります。
`compute-sanitizer` の下では実行がかなり遅くなるので、格子を小さくし、時間ステップ数を減らして試します。

## 4. MPI と組み合わせたとき

- **CUDA-aware MPI を使っているか。** MISO の CUDA 版のハロー交換は、デバイスのバッファの番地をそのまま `MPI_Isend`・`MPI_Irecv` に渡します（`miso/include/miso/mhd_halo_exchange.hpp`）。
  CUDA に対応していない MPI では、通信の中で止まるか、誤った値を受け取ります。NVIDIA HPC SDK に含まれる Open MPI など、CUDA に対応した MPI を使います。
- **ランクと GPU の対応。** MISO はノード内のランク番号を GPU の数で割った余りで GPU を選びます（`miso/include/miso/env.hpp` の `cuda::Env`）。
  ランク数が GPU の数より多いと、1 つの GPU を複数のランクで分け合います。
- **完了待ちの順序。** GPU のカーネルが書いたバッファを MPI へ渡す前に `cudaDeviceSynchronize` で待ちます。
  受信した値をカーネルで使う前には `MPI_Waitall` で待ちます。2 つの完了待ちは別のものです。
- **コンテナの中で実行するとき。** MISO の demo は `demo/shared/openmpi_config.env` の環境変数を読み込んでから `mpiexec` を実行します。

## 5. CPU と GPU で数値が違うとき

まず、入力（`config.yaml`）、格子、MPI の分割、`Real` の型（`float`/`double`）を揃えます。
そのうえで、差の大きさと場所を調べます。

| 差の様子 | 確かめること |
|---|---|
| 総和・平均などの縮約値の最後の数桁 | 加算の順序の違い（Host 版は前から順、CUDA 版はブロック内のトーナメント）。誤りではない |
| `reduce` の結果が大きく違う | `init` が `op` の単位元か（合計なら 0、最小値なら十分大きな値）。CUDA 版では範囲外のスレッドが `init` を縮約に加える（第 9 章 6.4 節） |
| 格子点ごとの値が丸め誤差程度に違う | `nvcc` は掛け算と足し算を 1 命令（FMA）にまとめることがあり、丸めが CPU と変わる。数学関数の実装も CPU と GPU で違う |
| 境界の近くだけ違う | 境界条件、ゴーストセルの更新、ハロー交換 |
| 特定の軸に沿ってずれる | 添字の順序、`Range3D` の範囲、配列の大きさ |
| 最初のステップから大きく違う | 初期条件がデバイスへ写っているか（`copy_from` の向き）、`this` の取り込み |

比較の方法と許す誤差の決め方は {ref}`テストと計算結果の比較 <edu-ref-testing>` を参照してください。
CPU と GPU の結果がビット単位で一致することは前提にしません。
