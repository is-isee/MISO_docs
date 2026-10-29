(edu-ch09)=
# 9. GPU（CUDA）での計算とデータ転送

第 7 章で書いた `for_each` と `reduce` を、NVIDIA の GPU で動かすための考え方を学びます。
計算する場所と配列を置く場所を区別し、CUDA のカーネル、メモリの確保と転送、
MISO の `for_each`・`reduce` の CUDA 版、MPI との組み合わせの順に進みます。

GPU がなくても、この章は読めます。動かして確かめる例には、CPU での出力と GPU での出力を並べて載せています。
CPU だけで進める場合は、この章を読んだあと {ref}`第 10 章 <edu-ch10>` の逐次・MPI 部分へ進み、
GPU を使える環境を用意できた段階で GPU 版を追加してください。

## 目次

- {ref}`1. ホストとデバイス <edu-ch09-host-device>`
- {ref}`2. CUDA のカーネル <edu-ch09-kernel>`
- {ref}`3. デバイスメモリの確保と転送 <edu-ch09-memory>`
- {ref}`4. for_each の CUDA 版 <edu-ch09-for-each>`
- {ref}`5. this キャプチャの落とし穴 <edu-ch09-this-capture>`
- {ref}`6. reduce の CUDA 版 <edu-ch09-reduce>`
- {ref}`7. 同期とエラーの見つけ方 <edu-ch09-sync>`
- {ref}`8. GPU の選択と MPI <edu-ch09-mpi>`
- {ref}`9. MISO を GPU 向けにビルドする <edu-ch09-build>`
- {ref}`10. CPU で作ったプログラムを GPU で動かす <edu-ch09-practice>`

(edu-ch09-host-device)=
## 1. ホストとデバイス

GPU を使うプログラムでも、プログラム全体を進めるのは CPU です。
CPU は設定ファイルを読み、配列を用意し、時間ステップのループを回します。
その途中で、格子点ごとの計算を GPU へ依頼します。

CUDA では、CPU 側を**ホスト**（host）、GPU 側を**デバイス**（device）と呼びます。
2 つは別々のメモリを持っています。

| | ホスト（CPU） | デバイス（GPU） |
|---|---|---|
| メモリ | 主記憶 | GPU に載ったメモリ |
| 同時に動く処理 | コア数ぶん | 数千〜数万のスレッド |
| MISO のバックエンド | `backend::Host` | `backend::CUDA` |

ここで大切なのは、**ホストのメモリにある配列は、そのままではデバイスから読めない**ことと、
**デバイスのメモリにある配列は、そのままではホストから読めない**ことです。
Fortran の配列は、宣言すればどのループからでも読み書きできました。
GPU では、配列をどちらのメモリに置いたかを、プログラムを書く側が管理します。

```text
   ホスト（CPU）                         デバイス（GPU）
  +--------------------+               +--------------------+
  | 主記憶             |  cudaMemcpy   | GPU のメモリ        |
  |  a_host[0..n-1]  --+-------------->+-- a_dev[0..n-1]    |
  |                  <-+---------------+--                  |
  +--------------------+   (PCIe など)  +--------------------+
     ループの進行、入出力                  格子点ごとの計算
```

ホストとデバイスの間の転送は、デバイス内のメモリの読み書きに比べてずっと遅い経路です。
そのため MISO では、物理量の配列を最初にデバイスへ置き、時間発展の間はデバイスの中で計算を続けます。
ホストへ写すのは、出力するときなど必要なときだけです。

(edu-ch09-kernel)=
## 2. CUDA のカーネル

### 2.1 関数の指定子

GPU で実行される関数を**カーネル**（kernel）と呼びます。
CUDA は C++ を拡張した言語で、関数の前に付ける指定子で、どこで実行される関数かを表します。

| 指定子 | 実行する場所 | 呼び出す場所 | 例 |
|---|---|---|---|
| `__global__` | デバイス | ホスト（`<<< >>>` で起動） | カーネル。戻り値は `void` |
| `__device__` | デバイス | デバイス（カーネルや他の `__device__` 関数） | カーネルから呼ぶ小さな関数 |
| `__host__` | ホスト | ホスト | 普通の C++ の関数（何も付けないのと同じ） |
| `__host__ __device__` | 両方 | 両方 | ホストでもデバイスでも使う関数 |

`__host__ __device__` を付けた関数は、ホスト用とデバイス用の 2 通りにコンパイルされます。
MISO の小さな関数の多くはこの形です。たとえば `miso/include/miso/utility.hpp` の
`util::sqrt` や `util::min2` は `__host__ __device__` が付いているので、CPU のループからも GPU のカーネルからも呼べます。

CUDA のコードは、NVIDIA の CUDA Toolkit に含まれるコンパイラ `nvcc` でコンパイルします。
`nvcc` は、ホスト用の部分を普段の C++ コンパイラ（g++ など）に渡し、デバイス用の部分を自分で GPU の命令に変換します。

### 2.2 スレッドとブロック

カーネルは、多数の**スレッド**（thread）で同時に実行されます。
どのスレッドも同じカーネルの命令を実行しますが、自分の番号を使って、担当する要素を変えます。

スレッドは**ブロック**（block）という組にまとめられ、ブロックを並べたものを**グリッド**（grid）と呼びます。
カーネルを起動するときに、グリッドのブロック数とブロックあたりのスレッド数を `<<<グリッド, ブロック>>>` で指定します。
カーネルの中では、次の組み込み変数で自分の位置が分かります。

| 変数 | 意味 |
|---|---|
| `blockIdx.x` | 自分のブロックの番号（0 始まり） |
| `blockDim.x` | ブロックあたりのスレッド数 |
| `threadIdx.x` | ブロックの中での自分の番号（0 始まり） |

1 次元の配列なら、通し番号は `blockIdx.x * blockDim.x + threadIdx.x` です。
要素数 1000 を、ブロックあたり 256 スレッドで覆うと次のようになります。

```text
ブロック数 = (1000 + 256 - 1) / 256 = 4   （整数の割り算。切り上げになる）

blockIdx.x :   0            1            2            3
threadIdx.x:   0 ... 255    0 ... 255    0 ... 255    0 ... 255
通し番号 i :   0 ... 255  256 ... 511  512 ... 767  768 ... 1023
                                                    ~~~~~~~~~~~~
                                                    i = 1000〜1023 は配列の外。
                                                    何もせずに終わらせる
```

スレッドはブロック単位で起動するので、全スレッド数は要素数ちょうどにはなりません。
はみ出したスレッドが配列の外を書かないよう、カーネルの中で `if (i < n)` と調べます。

`.x` は、ブロックやグリッドを 3 次元（`.x`, `.y`, `.z`）にも並べられるためです。
3 次元の並べ方は 2.4 で MISO の実物を見ます。

### 2.3 ベクトル加算のカーネル

`c[i] = a[i] + b[i]` を GPU で計算する、MISO を使わない CUDA だけの例です。
デバイスメモリを扱う関数（`cudaMalloc`, `cudaMemcpy`, `cudaFree`）は次節で説明します。
ここではカーネルの定義と起動の形を見てください。

```cuda
#include <cstdio>
#include <vector>

// GPU で動く関数（カーネル）。1 つのスレッドが 1 要素を担当する
__global__ void add_kernel(int n, const float *a, const float *b, float *c) {
  int i = blockIdx.x * blockDim.x + threadIdx.x;  // 通し番号
  if (i < n) {                                    // はみ出したスレッドは何もしない
    c[i] = a[i] + b[i];
  }
}

int main() {
  const int n = 1000;
  std::vector<float> a(n), b(n), c(n);  // ホスト（CPU）側の配列
  for (int i = 0; i < n; ++i) {
    a[i] = 1.0f * i;
    b[i] = 2.0f * i;
  }

  // デバイス（GPU）側の配列を確保する
  float *a_d = nullptr, *b_d = nullptr, *c_d = nullptr;
  cudaMalloc(&a_d, sizeof(float) * n);
  cudaMalloc(&b_d, sizeof(float) * n);
  cudaMalloc(&c_d, sizeof(float) * n);

  // ホスト → デバイス
  cudaMemcpy(a_d, a.data(), sizeof(float) * n, cudaMemcpyHostToDevice);
  cudaMemcpy(b_d, b.data(), sizeof(float) * n, cudaMemcpyHostToDevice);

  // 256 スレッドのブロックを、n 要素を覆うだけ並べて起動する
  const int block = 256;
  const int grid = (n + block - 1) / block;  // 切り上げ: 4
  add_kernel<<<grid, block>>>(n, a_d, b_d, c_d);
  cudaError_t err = cudaGetLastError();  // 起動そのものの失敗を調べる
  if (err != cudaSuccess) {
    std::printf("launch error: %s\n", cudaGetErrorString(err));
    return 1;
  }

  // デバイス → ホスト（カーネルの終了を待ってから写す）
  cudaMemcpy(c.data(), c_d, sizeof(float) * n, cudaMemcpyDeviceToHost);

  std::printf("grid = %d, block = %d\n", grid, block);
  std::printf("c[1] = %g, c[999] = %g\n", c[1], c[999]);

  cudaFree(a_d);
  cudaFree(b_d);
  cudaFree(c_d);
  return 0;
}
```

ファイル名を `vecadd.cu` として、次のようにコンパイル・実行します。
`.cu` は CUDA のソースを表す拡張子です。
`-arch=sm_89` は、どの世代の GPU 向けの命令を作るかの指定で、使う GPU に合わせます（9 節）。

```bash
nvcc -arch=sm_89 vecadd.cu -o vecadd
./vecadd
```

```text
grid = 4, block = 256
c[1] = 3, c[999] = 2997
```

カーネルの中身は、Fortran の次のループの「1 回分」にあたります。

```fortran
do i = 1, n
  c(i) = a(i) + b(i)
end do
```

ループの `do` に相当するものはカーネルの中にありません。
「どの `i` を誰が計算するか」は、`<<<grid, block>>>` で起動したスレッドの番号が決めます。
そのため、各 `i` の計算は互いに独立である必要があります。
`c(i)` を計算するのに `c(i-1)` の新しい値を使う漸化式は、このままでは GPU に渡せません。
これは第 7 章の `for_each` に渡すラムダと同じ条件です。

### 2.4 MISO の手書きカーネル

MISO の MHD の時間発展は、ほとんどを `__global__` のカーネルとして直接書いています。
`miso/include/miso/mhd_integrator_cuda.cuh` の `bb_ht_vb_kernel` の冒頭です。

```cuda
template <typename Real>
__global__ void bb_ht_vb_kernel(FieldsView<const Real> qq_argm,
                                Array3DView<Real> pr, Array3DView<Real> bb,
                                Array3DView<Real> ht, Array3DView<Real> vb,
                                GridView<const Real> grid) {
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  int j = blockIdx.y * blockDim.y + threadIdx.y;
  int k = blockIdx.z * blockDim.z + threadIdx.z;

  if (i < 0 || i >= grid.i_total || j < 0 || j >= grid.j_total || k < 0 ||
      k >= grid.k_total)
    return;
```

ブロックとグリッドを 3 次元に並べ、`i`, `j`, `k` をそれぞれの方向の番号から作っています。
`return` で、格子の外にはみ出したスレッドを終わらせます。
引数は、配列を所有する `Array3D` ではなく、軽い `Array3DView` や `FieldsView` です（第 4 章）。
カーネルの引数は値でデバイスへ渡される（3.3 で説明します）ので、ポインタと大きさだけを持つ View を渡します。

ブロックの形は `miso/include/miso/cuda_util.cuh` の `KernelShape3D` で、8×8×8 = 512 スレッドに決めています。
起動側は `Integrator<Real, backend::CUDA>::update_sc4` にあります。

```cuda
    bb_ht_vb_kernel<Real><<<cu_shape.grid_dim, cu_shape.block_dim>>>(
        qq_argm.view(), pr.view(), bb.view(), ht.view(), vb.view(), cgrid);
    MISO_CUDA_CHECK(cudaGetLastError());
```

`MISO_CUDA_CHECK` は、CUDA の関数が返すエラーを調べるマクロです（7 節）。

### 練習 1：ブロック数とはみ出しの判定

要素数 `n = 300` を、ブロックあたり 128 スレッドで処理します。
ブロック数と起動される全スレッド数を求め、`if (i < n)` がない場合に何が起きるかを説明してください。

<details>
<summary>解答・解説</summary>

ブロック数は `(300 + 128 - 1) / 128 = 427 / 128 = 3`（整数の割り算）で、全スレッド数は 3 × 128 = 384 です。
通し番号 `i` は 0〜383 になり、300〜383 の 84 個のスレッドは配列の外を指します。

`if (i < n)` がないと、この 84 個のスレッドが `c[300]`〜`c[383]` を書きます。
`cudaMalloc` で確保した 300 要素の外なので、他の配列を壊すか、不正なメモリアクセスとしてプログラムが止まります。
Fortran で配列の上限を超えて書いた場合と同じ種類の誤りですが、GPU では多数のスレッドが同時に書くので、
結果がどこに現れるかを追いにくくなります。

</details>

(edu-ch09-memory)=
## 3. デバイスメモリの確保と転送

### 3.1 cudaMalloc と cudaMemcpy

デバイスのメモリは、CUDA の関数で確保・転送・解放します。

| 関数 | 役割 | Fortran・C++ で近いもの |
|---|---|---|
| `cudaMalloc(&p, バイト数)` | デバイスにメモリを確保し、その番地を `p` に入れる | `allocate`、`new` |
| `cudaMemcpy(先, 元, バイト数, 向き)` | メモリの内容を写す | 配列の代入 |
| `cudaFree(p)` | デバイスのメモリを解放する | `deallocate`、`delete[]` |

`cudaMemcpy` の向きは、4 番目の引数で指定します。

| 向き | 写す方向 |
|---|---|
| `cudaMemcpyHostToDevice` | ホスト → デバイス |
| `cudaMemcpyDeviceToHost` | デバイス → ホスト |
| `cudaMemcpyDeviceToDevice` | デバイス内 |

`cudaMalloc` が返すのはデバイスの番地です。型は普通のポインタ `float *` ですが、
ホストのコードで `a_d[0]` のように読むことはできません（ホストのメモリの番地ではないためです）。
逆に、`std::vector` の `a.data()` のようなホストの番地をカーネルへ渡しても、デバイスからは読めません。
**ポインタの型だけでは、それがどちらのメモリを指すかは区別されません。** 2.3 の例で、
デバイス側の変数に `_d` を付けたのはそのためです。

### 3.2 MISO の Array3D\<T, backend::CUDA\>

MISO では、`cudaMalloc` などを直接書く代わりに、配列クラスの 2 つ目のテンプレート引数でメモリの場所を選びます（第 6 章）。

```cpp
Array3D<Real, backend::Host> a_host(nx, ny, nz);  // ホストのメモリ
Array3D<Real, backend::CUDA> a_dev(nx, ny, nz);   // デバイスのメモリ
```

`miso/include/miso/array3d.hpp` の CUDA 版のコンストラクタとデストラクタです。

```cpp
template <typename T> class Array3D<T, backend::CUDA> {
private:
  T *data_ = nullptr;
  std::array<int, 3> shape_ = {-1, -1, -1};

public:
  Array3D(int nx0, int nx1, int nx2) : shape_{nx0, nx1, nx2} {
    // （大きさの検査を省略）
    MISO_CUDA_CHECK(cudaMalloc(&data_, sizeof(T) * array_size));
  }

  ~Array3D() {
    if (data_)
      cudaFree(data_);
    data_ = nullptr;
  }
```

ホスト版が `new T[...]` と `delete[]` を使うところを、`cudaMalloc` と `cudaFree` に替えています。
第 4 章の RAII と同じく、変数の寿命が終わると自動で解放されます。

転送は `copy_from` というメンバ関数が受け持ちます。
引数の型（どちらのバックエンドの配列か）で、`cudaMemcpy` の向きが決まります。

| 呼び出し | 写す方向 | 中で使う向き |
|---|---|---|
| `a_dev.copy_from(a_host)` | ホスト → デバイス | `cudaMemcpyHostToDevice` |
| `a_host.copy_from(a_dev)` | デバイス → ホスト | `cudaMemcpyDeviceToHost` |
| `a_dev.copy_from(b_dev)` | デバイス内 | `cudaMemcpyDeviceToDevice` |
| `a_host.copy_from(b_host)` | ホスト内 | `std::copy` |

これは第 6 章で学んだオーバーロードの選択です。
ホスト版の `Array3D<T, backend::Host>` にも、`#ifdef __CUDACC__` の中に「CUDA 版から写す」`copy_from` が用意されています。
`__CUDACC__` は `nvcc` がコンパイルしているときだけ定義されるマクロです。

`Array1D`〜`Array4D` のすべてに同じ形の CUDA 版があります。

### 3.3 CUDA 版の配列で添字が使えない理由

ホスト版の `Array3D` には `a(i, j, k)` と書くための `operator()` がありますが、CUDA 版にはありません。
`a_dev(i, j, k)` をホストのコードに書いても、ホストからはデバイスのメモリを読めないためです。

デバイスの要素は、カーネルの中で View を通して読み書きします。
`Array3DView` のメンバ関数には `__host__ __device__` が付いています。

```cpp
  /// @brief Return reference to the element at the given indices.
  __host__ __device__ T &operator()(int i0, int i1, int i2) const noexcept {
    // （添字の範囲の assert を省略）
    return data_[(i0 * shape_[1] + i1) * shape_[2] + i2];
  }
```

View は、ポインタと各方向の大きさだけを持つ小さな値です。
カーネルの引数や次節のラムダに値として渡すと、この数個の値だけがデバイスへ写ります。
要素はすでにデバイスのメモリにあり、View はその番地を指しているだけです。
つまり、CUDA 版の `Array3D` はホストのコードで確保・解放・`copy_from`・`view()` に使い、
要素の読み書きはカーネルの中で View を通して行う、という役割分担になります。

### 練習 2：配列の場所と転送の向き

`demo/mhd2d_vortex/src/main.cpp` の `InitialCondition::apply` は、普通の `for` 文で `qq.ro(i, j, k) = 1.0;` のように初期値を書いています。
CUDA ビルドでは物理量の配列がデバイスにあるので、このループからデバイスの配列を直接書くことはできません。
`ModelBase::run()`（`miso/include/miso/mhd_model_base.hpp`）から呼ばれる `MHD::apply_initial_condition`（`miso/include/miso/mhd_solver.hpp`）を読み、
初期条件がどの配列に書かれ、どのようにデバイスへ届くかを説明してください。

<details>
<summary>解答・解説</summary>

`apply_initial_condition` は、ホストの格子 `grid_h`（`Grid<Real, backend::Host>`）と
ホストの配列 `qq_h`（`Fields<Real, backend::Host>`）をローカル変数として作ります。
`ic.apply(qq_h.view(), grid_h.const_view())` で、`InitialCondition::apply` の `for` 文がホストの配列に初期値を書きます。
その後 `qq.copy_from(qq_h)` で、計算用の配列 `qq`（`Fields<Real, Backend>`）へ写します。

`Backend` が `backend::CUDA` なら、この `copy_from` は 3.2 の表の「ホスト → デバイス」で、中では `cudaMemcpyHostToDevice` が使われます。
`Backend` が `backend::Host` なら「ホスト内」の写しです。
どちらのビルドでも、初期条件はホストで書き、計算用の配列へ写すという同じ手順になります。
`qq_h` と `grid_h` はローカル変数なので、関数を抜けると解放されます。

出力は逆向きです。`miso/include/miso/mhd_checkpoint.hpp` の `Checkpoint::save` は、
ホストの配列 `qq` に `qq.copy_from(qq_)` でデバイスの値を写してから、ファイルに書き出します。

</details>

(edu-ch09-for-each)=
## 4. for_each の CUDA 版

### 4.1 同じラムダを GPU で動かす

第 7 章では、`for_each` にラムダを渡して格子点ごとの計算を書きました。
Host 版の `for_each` は、ラムダ `f` を `for` 文の中で `f(i)` と呼ぶだけでした。
CUDA 版では、同じラムダをカーネルの引数として GPU へ渡し、各スレッドが `f(i)` を呼びます。
`miso/include/miso/execution.hpp` の 1 次元の CUDA 版です。

```cuda
template <class F> __global__ void for_each_kernel(Range1D range, F f) {
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  if (idx < range.size()) {
    int i = range.unravel(idx);
    f(i);
  }
}

/// @brief 1D parallel (concurrent) for loop in the CUDA backend.
template <class F> inline void for_each(backend::CUDA, Range1D range, F f) {
  int n = range.size();
  if (n == 0)
    return;  // Avoid launching kernel with zero threads
  int block = 256;
  int grid = (n + block - 1) / block;
  for_each_kernel<<<grid, block>>>(range, f);
  MISO_CUDA_CHECK(cudaGetLastError());
}
```

2.3 のベクトル加算と同じ形です。違いは、計算の中身 `c[i] = a[i] + b[i]` が、カーネルの中に直接書かれず、
引数 `f` として外から渡されることです。
`range.unravel(idx)` は通し番号 `idx`（0 始まり）を、範囲の始まり `range.begin` からの番号 `i` に直します。

第 7 章で、ラムダは「取り込んだ値をメンバに持つ小さな構造体」と考えられることを学びました。
カーネルの引数は値渡しなので、この構造体がまるごとデバイスへ写ります。

```text
ホスト側のコード                               デバイス（GPU）
--------------------------------------         -----------------------------------------
auto fv = f.view();   // ポインタ + 大きさ
const Real dx = ...;

MISO_LAMBDA(int i) { fv[i] = ... dx ...; }
   │  コンパイラが作る構造体:
   │    struct ラムダ型 {
   │      Array1DView<Real> fv;  // 写し
   │      Real dx;               // 写し
   │      __device__ void operator()(int i) const;
   │    };
   ▼
for_each(backend::CUDA{}, Range1D{0, n}, ラムダ)
   │
   └─ for_each_kernel<<<grid, 256>>>(range, f)
          │  引数 range と f（fv, dx の値）を
          │  カーネル起動時にデバイスへ写す   ──────▶  全スレッドが同じ f の写しを読む
                                                      スレッド idx=0   : f(range.begin + 0)
                                                      スレッド idx=1   : f(range.begin + 1)
                                                        ...
                                                      スレッド idx=n-1 : f(range.begin + n-1)
                                                      idx >= n のスレッドは何もしない
```

デバイスへ写るのは、ラムダが取り込んだ値（View のポインタと大きさ、`dx` などの数値）だけです。
View が指す配列の要素は、もともとデバイスのメモリにあります。
そのため、**ラムダが取り込む View は、デバイスの配列（`Array1D<Real, backend::CUDA>` など）から作ったもの**でなければなりません。
ホストの配列の View を CUDA 版の `for_each` へ渡すと、デバイスからホストの番地を読むことになり、実行時のエラーになります。

3 次元の `for_each(backend::CUDA, Range3D, f)` も、スレッドを 1 次元に並べて起動します。
`Range3D::unravel` が通し番号を `(i0, i1, i2)` に直し、最後の添字が最も速く変わる順に割り当てます。
メモリ上で隣り合う要素を、隣り合うスレッドが読むようにするためです。

### 4.2 MISO_LAMBDA と \_\_host\_\_ \_\_device\_\_

デバイスで呼ぶラムダには、関数と同じく `__device__` の指定が必要です。
`MISO_LAMBDA` は、それをビルドに合わせて付けるマクロです（`miso/include/miso/execution.hpp`）。

```cpp
#ifdef __CUDACC__
#define MISO_LAMBDA [=] __host__ __device__
#else
#define MISO_LAMBDA [=]
#endif  // __CUDACC__
```

`nvcc` でコンパイルするときは `[=] __host__ __device__`、g++ などでコンパイルするときは `[=]` になります。
`[=]` は第 7 章で学んだ「使う外側の変数を値で取り込む」指定です。
参照で取り込む `[&]` は使いません。参照はホストの変数を指すので、デバイスからは読めないためです。

`__host__ __device__` を付けたラムダは、CUDA では**拡張ラムダ**（extended lambda）と呼ばれる機能で、
`nvcc` に `--expt-extended-lambda` オプションを付けたときだけ使えます。
MISO は `miso/CMakeLists.txt` でこのオプションを付けています。

```cmake
    target_compile_options(
        MISO INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:--expt-extended-lambda>)
```

`$<$<COMPILE_LANGUAGE:CUDA>:...>` は「CUDA としてコンパイルするファイルにだけ付ける」という CMake の書き方です。
`MISO::MISO` をリンクすると、自分のプログラムにもこのオプションが付きます。

CPU だけでビルドするとき、`__host__` と `__device__` は g++ が知らない語です。
`miso/include/miso/cuda_compat.hpp` が、そのときは空の語として定義します。

```cpp
#else  // __CUDACC__（nvcc 以外でコンパイルするとき）

#ifndef __device__
#define __device__
#endif

#ifndef __host__
#define __host__
#endif

// Do not define __global__:
// CUDA kernel semantics and launch syntax are not portable.
```

そのため、`__host__ __device__` を付けた関数や View は、CPU ビルドでは普通の C++ として使えます。
一方、`__global__` は定義していません。カーネルと `<<< >>>` の起動は CPU ビルドに意味がないので、
CUDA 版の `for_each`・`reduce` や手書きのカーネルは `#ifdef __CUDACC__` の中に置いてあります。

### 4.3 CPU と GPU で同じ main.cpp を動かす

`for_each` と `reduce` を使い、$\int_0^1 x^2\,dx = 1/3$ を中点則で計算する例です。
`Backend` を切り替えるだけで、同じコードが CPU でも GPU でも動きます。
`reduce` の CUDA 版の仕組みは 6 節で説明します。ここでは呼び方が Host 版と同じであることを見てください。

```cpp
#include <cstdio>

#include <miso/core.hpp>

using namespace miso;
using Real = float;

#ifdef USE_CUDA
using Backend = backend::CUDA;
#else
using Backend = backend::Host;
#endif

int main(int argc, char **argv) {
  Env env(argc, argv);  // MPI の初期化と、CUDA ビルドでは GPU の選択

  const int n = 1000;
  const Real dx = Real(1) / n;

  Array1D<Real, Backend> f(n);  // Backend が CUDA なら GPU のメモリに確保
  auto fv = f.view();           // ラムダへ渡す軽い View

  // 各要素 f[i] = x^2（x はセルの中心）を計算する
  for_each(Backend{}, Range1D{0, n}, MISO_LAMBDA(int i) {
    const Real x = (i + Real(0.5)) * dx;
    fv[i] = x * x;
  });

  // f[i] * dx の総和（0 から 1 までの x^2 の積分、厳密値は 1/3）
  const Real integral = reduce(
      Backend{}, Range1D{0, n}, Real(0),
      MISO_LAMBDA(int i) { return fv[i] * dx; },
      MISO_LAMBDA(Real a, Real b) { return a + b; });

  // 確認のため、CPU 側の配列へ写して 1 要素を表示する
  Array1D<Real, backend::Host> f_host(n);
  f_host.copy_from(f);

  if (mpi::is_root()) {
    std::printf("f[999]   = %.7f\n", f_host[999]);
    std::printf("integral = %.7f\n", integral);
  }
}
```

読むときの要点です。

- `fv` と `dx` はラムダの外で用意したローカル変数です。ラムダはこの 2 つを値で取り込みます。
- `f` そのもの（`Array1D`）はラムダの中で使っていません。所有する配列はホストに置いたまま、View だけを渡します。
- 縮約の演算 `op`（2 つ目のラムダ）もデバイスで呼ばれるので、`MISO_LAMBDA` で書きます。
- 結果をホストで表示するには、`f_host.copy_from(f)` でホストへ写します。
- `USE_CUDA` は、自分の CMake 設定で CUDA ビルドのときに定義するマクロです（MISO の demo と同じ名前にしています）。

CPU ビルドと、GPU ビルドの出力です（GPU は RTX 4090 で確かめました）。

```text
# CPU（backend::Host）
f[999]   = 0.9990003
integral = 0.3333332

# GPU（backend::CUDA）
f[999]   = 0.9990003
integral = 0.3333333
```

`f[999]` は一致していますが、`integral` は最後の桁が違います。
`float` の足し算は、足す順序で丸め誤差が変わります。Host 版は `i = 0, 1, 2, ...` の順に足しますが、
CUDA 版は 6 節で見るように、木の形に組んで足します。
**CPU と GPU の結果は、ビット単位では一致しないことがある**と考えて、比較のしかたを決めます。

### 4.4 CMake の設定

このプログラムを CUDA でビルドするには、第 1 章で書いた CMake の設定に、次のことを足します。
MISO の demo の `CMakeLists.txt`（`demo/mhd2d_vortex/CMakeLists.txt`）が同じことをしているので、抜粋で示します。

```cmake
option(USE_CUDA "Enable CUDA support" OFF)
if(USE_CUDA)
    # Select GPU architecture before enable_language(CUDA); otherwise CMake
    # defines the default (52) and this setting is ignored.
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES
            native
            CACHE STRING "CUDA architectures")
    endif()
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
endif(USE_CUDA)
```

```cmake
if(USE_CUDA)
    find_package(CUDAToolkit REQUIRED)
    target_link_libraries(${app_name} PRIVATE CUDA::cudart)

    # Force main.cpp to be treated as CUDA source
    set_source_files_properties(${app_main} PROPERTIES LANGUAGE CUDA)
    target_compile_definitions(${app_name} PRIVATE USE_CUDA)
endif(USE_CUDA)
```

`enable_language(CUDA)` で CMake に `nvcc` を探させます。`CMAKE_CUDA_ARCHITECTURES`（9 節）はその前に決めます。
`set_source_files_properties(... LANGUAGE CUDA)` は、拡張子が `.cpp` のファイルを g++ ではなく `nvcc` でコンパイルさせる指定です。
CPU ビルドと GPU ビルドで同じファイルを使うためです。
`target_compile_definitions(... USE_CUDA)` で、ソースの `#ifdef USE_CUDA` が有効になります。
さらに demo は、MISO を読み込む前に MISO 側のオプション `MISO_USE_CUDA` を ON にし、MISO に CUDA の実行時ライブラリと `--expt-extended-lambda` を付けさせます。
`add_subdirectory` で MISO を読み込む場合も、その前に `set(MISO_USE_CUDA ON CACHE BOOL "...")` とします。
ビルドフォルダは CPU 用と GPU 用で分けます。

```bash
# CPU
cmake -B build_cpu -S .
cmake --build build_cpu -j

# GPU（nvcc に PATH が通っていること。89 は RTX 40 シリーズの例）
cmake -B build_gpu -S . -DUSE_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES=89
cmake --build build_gpu -j
```

### 練習 3：ラムダが取り込むもの

4.3 の例で、`for_each` に渡したラムダが取り込む変数を挙げ、それぞれがデバイスへ写るときに何バイト程度かを見積もってください。
`Array1DView<Real>` は、ポインタ 1 つと `int` 1 つを持ちます（`miso/include/miso/array1d.hpp`）。
また、ラムダの中で `fv[i]` の代わりに `f.view()[i]` と書くと、どこが問題になるかを説明してください。

<details>
<summary>解答・解説</summary>

取り込むのは `fv`（`Array1DView<Real>`）と `dx`（`Real` = `float`）の 2 つです。
`fv` はポインタ 8 バイトと `int` 4 バイトで、構造体の詰め物を含めて 16 バイト程度、`dx` は 4 バイトです。
合わせて 20〜24 バイト程度で、配列の要素 1000 個（4000 バイト）は写りません。
`n` は `Range1D{0, n}` を作るのに使っただけで、ラムダの中では使っていないので取り込まれません。

ラムダの中で `f.view()` と書くと、`f`（所有する `Array1D<Real, backend::CUDA>`）を取り込もうとします。
`Array1D` はコピーが禁止されている（第 4 章の `= delete`）ので、`[=]` で値として取り込めず、コンパイルエラーになります
（`nvcc` では `... cannot be referenced -- it is a deleted function` と表示されます）。
仮に取り込めたとしても、`view()` には `__device__` が付いていないので、デバイスから呼ぶことはできません。
所有する配列はホストのコードで扱い、ラムダへは View だけを渡す、という役割分担を守ります。

</details>

(edu-ch09-this-capture)=
## 5. this キャプチャの落とし穴

### 5.1 メンバ変数を使うと this が取り込まれる

クラスのメンバ関数の中でラムダを書き、その中でメンバ変数を使う場合に、CPU では動くのに GPU では止まる誤りが起きます。

第 4 章で学んだとおり、メンバ関数の中でメンバ変数 `factor` と書くのは、`this->factor` の省略です。
`this` は、メンバ関数を呼び出したオブジェクトを指すポインタです。
C++ の規則では、ラムダの中でメンバ変数を使うと、`[=]` でも `factor` の値ではなく **`this` というポインタ**が取り込まれます（第 7 章）。

GPU では、これが問題になります。`this` はホストのメモリにあるオブジェクトの番地です。
ラムダと一緒に `this` の値（番地）はデバイスへ写りますが、その番地が指すオブジェクトは写りません。
デバイスのスレッドが `this->factor` を読もうとすると、ホストの番地を読むことになります。

```text
ホストのメモリ                     デバイス（GPU）
+------------------+               ラムダの写し
| Scaler s         |                 this = 0x7fff....  ──┐
|   factor = 3     | ◀── 0x7fff....                          │ デバイスからは
+------------------+                                      │ 読めない番地
                                   this->factor を読む ◀──┘ → illegal memory access
```

次の例は、同じことをする 2 つのメンバ関数を比べています（説明用のコードです）。

```cpp
struct Scaler {
  Real factor = 3;

  // 誤り: ラムダの中でメンバ変数 factor を直接使う
  void apply_wrong(Array1DView<Real> a) const {
    for_each(Backend{}, Range1D{0, a.size()},
             MISO_LAMBDA(int i) { a[i] = factor * i; });
  }

  // 正しい: ラムダの前でローカル変数へコピーする
  void apply_right(Array1DView<Real> a) const {
    const Real factor_ = factor;
    for_each(Backend{}, Range1D{0, a.size()},
             MISO_LAMBDA(int i) { a[i] = factor_ * i; });
  }
};
```

`main` で `apply_right` と `apply_wrong` を順に呼び、それぞれの後に `a_host.copy_from(a)` でホストへ写して `a[3]` を表示しました。

```text
# CPU（backend::Host）
right: a[3] = 9
wrong: a[3] = 9

# GPU（backend::CUDA）
right: a[3] = 9
CUDA Error: an illegal memory access was encountered .../miso/include/miso/array1d.hpp 109
```

CPU ではホストとデバイスの区別がないので、`this` を取り込んでも正しく動きます。
そのため、**CPU のテストが通っても、この誤りは見つかりません**。
GPU では `apply_wrong` のカーネルが不正なメモリアクセスで止まります。
エラーが報告された場所が `array1d.hpp`（`copy_from` の中の `cudaMemcpy`）になっている理由は 7 節で説明します。

`apply_right` では、ラムダの直前で `factor` をローカル変数 `factor_` へコピーしています。
ラムダはローカル変数 `factor_` の値（`float` 1 つ）を取り込むので、`this` は取り込まれません。
`a` は関数の引数（View の値）なので、そのまま使えます。

### 5.2 MISO での書き方

MISO の `dev/notes/usage_ai.md` には、この規則が例とともに書かれています。

```cpp
const Real gm_ = gm;  // メンバ変数をローカルにコピー
for_each(btag, range, MISO_LAMBDA(int i) { pr[i] = (gm_ - Real(1)) * qq.ro[i] * qq.ei[i]; });
```

実物は `miso/include/miso/eos.hpp` の `IdealEOS::gas_pressure` です。

```cpp
  void gas_pressure(Backend btag, mhd::FieldsView<const Real> qq,
                    Array3DView<Real> pr) const {
    const Real gm_ = gm;  // device lambdas must not capture `this`
    Range1D range{0, qq.size()};
    for_each(
        btag, range,
        MISO_LAMBDA(int i) { pr[i] = (gm_ - Real(1)) * qq.ro[i] * qq.ei[i]; });
  }
```

`gm`（比熱比）は `IdealEOS` のメンバ変数なので、ラムダの前で `gm_` へコピーします。
`qq` と `pr` は関数の引数（View）なので、そのまま取り込めます。
`Integrator<Real, backend::CUDA>::cfl`（`miso/include/miso/mhd_integrator_cuda.cuh`）でも、
メンバ変数 `cfl_number` を `cfl_number_` へコピーし、配列も `qq_v`, `grid_v`, `cs_v` というローカルの View にしてからラムダを書いています。

この規則は、MISO の issue #170（<https://github.com/is-isee/MISO/issues/170>）で見つかった不具合から来ています。
`eos.hpp`、`cfl`、`table_interpolator.hpp`、`demo/mhd3d_magnetosphere` のラムダがメンバ変数を使っていて `this` を取り込み、
RTX 4090 の環境で `mhd2d_vortex` と `mhd3d_magnetosphere` が `illegal memory access` で止まりました。
一方、別の環境では偶然動いていました。HMM（Heterogeneous Memory Management）という、
デバイスからホストのメモリを読めるようにする仕組みが有効な環境では、ホストの番地を読んでも止まらないためです。
ラムダの直前でメンバをローカル変数にコピーする形に直し、磁気圏デモが 2 GPU で動き、CPU と `float` の精度の範囲で一致することを確かめています。

「ある GPU 環境で動いた」ことも、`this` を取り込んでいないことの確認にはなりません。
ラムダを書いたら、中で使っている名前が「ローカル変数か引数か」を、宣言まで戻って確かめます。

### 練習 4：ラムダが使う名前の確認

`miso/include/miso/eos.hpp` の `IdealEOS::sound_speed` を読み、ラムダの中で使っている名前を
「ローカル変数」「関数の引数」「メンバ変数」に分けてください。
また、`const Real gm_ = gm;` の行を消してラムダの中を `gm` に戻した場合、CPU ビルドと GPU ビルドでそれぞれどうなるかを説明してください。

<details>
<summary>解答・解説</summary>

`sound_speed` のラムダは `cs[i] = util::sqrt(gm_ * (gm_ - Real(1)) * qq.ei[i]);` を計算します。

| 名前 | 種類 |
|---|---|
| `gm_` | ローカル変数（メンバ `gm` の写し） |
| `qq` | 関数の引数（`mhd::FieldsView<const Real>`） |
| `cs` | 関数の引数（`Array3DView<Real>`） |
| `i` | ラムダの引数 |

`util::sqrt` は関数名で、変数ではないので取り込まれません。`__host__ __device__` が付いているのでデバイスから呼べます。
ラムダの中にメンバ変数はありません。

`gm_` をやめて `gm` と書くと、`gm` は `this->gm` の意味になり、ラムダは `this` を取り込みます。
CPU ビルドでは `this` が指すオブジェクトをそのまま読めるので、正しく動きます。
GPU ビルドでは、コンパイルは通りますが、実行するとデバイスがホストの番地を読み、
`illegal memory access` で止まります（HMM が有効な環境では偶然動くこともあります）。

</details>

(edu-ch09-reduce)=
## 6. reduce の CUDA 版

### 6.1 多数のスレッドで 1 つの値を作る

`reduce` は、配列全体から最小値や合計などの 1 つの値を作る処理でした（第 7 章）。
Host 版は、変数 `acc = init` に `acc = op(acc, map(i))` で 1 つずつ合わせていくだけです。
GPU で同じことをするには工夫が要ります。
数千のスレッドが 1 つの変数 `acc` を同時に書き換えると、互いの書き込みを上書きして正しい値になりません。
そこで MISO の CUDA 版は、**ブロックの中で値を半分ずつにまとめていく**方法を使います。
スポーツのトーナメントのように、2 つずつ組にして勝者（合計なら和、最小値なら小さい方）を残し、1 つになるまで繰り返します。

### 6.2 共有メモリとブロック内の縮約

ブロックの中のスレッドは、**共有メモリ**（shared memory）という小さく速いメモリを共有できます。
共有メモリはブロックごとに別々にあり、他のブロックからは見えません。

1 ブロックのスレッド数を 8 として、8 個の値の合計を求める様子です。

```text
共有メモリ s[]:   s0  s1  s2  s3  s4  s5  s6  s7     各スレッドが自分の値を書く
                   │   │   │   │   │   │   │   │
                   │   │   │   │   │   │   │   │      __syncthreads()（全員が書き終わるまで待つ）
offset = 4:       s0+s4 s1+s5 s2+s6 s3+s7              スレッド 0〜3 が、4 つ先の値と合わせる
                   │     │     │     │                __syncthreads()
offset = 2:     (s0+s4)+(s2+s6)  (s1+s5)+(s3+s7)       スレッド 0〜1
                   │               │                  __syncthreads()
offset = 1:     s0+s1+...+s7                          スレッド 0
                   │
                 ブロックの結果 → out[blockIdx.x]
```

8 個の値は 3 段（$\log_2 8$）で 1 つになります。
各段で働くスレッドは前の段の半分です。

`__syncthreads()` は、ブロックの中の全スレッドがその地点に着くまで待つ命令です。
これがないと、あるスレッドが `s[tid + offset]` を読むときに、その値を書くはずのスレッドがまだ書いていない、ということが起こります。

MISO の実物は `miso/include/miso/execution.hpp` の `block_reduce_shared` です。

```cuda
template <class T, class Op>
__device__ inline T block_reduce_shared(T *s, int tid, int n, Op op) {
  __syncthreads();
  for (int offset = (n + 1) >> 1; offset > 0; offset = (offset + 1) >> 1) {
    if (tid < offset && tid + offset < n) {
      s[tid] = op(s[tid], s[tid + offset]);
    }
    __syncthreads();
    if (offset == 1)
      break;
  }
  return s[0];
}
```

`>> 1` は 2 で割る（右へ 1 ビットずらす）演算で、`(n + 1) >> 1` は $n/2$ の切り上げです。
切り上げにしているのは、スレッド数 `n` が 2 の累乗でなくても使えるようにするためです。
`tid + offset < n` の条件で、相手がいないスレッドは何もしません。

### 6.3 ブロックの結果をさらにまとめる

1 ブロックの縮約で得られるのは、そのブロックの担当分の結果です。
ブロックが 64 個あれば、64 個の値が残ります。MISO の `reduce` は、これを 1 つになるまでカーネルを繰り返して縮約します。

```text
段階 1: reduce_stage_kernel        map(i) を計算し、ブロックごとに縮約
        n 個の要素 ──▶ buf0[grid]   （grid = n / 256 の切り上げ）

段階 2 以降: reduce_stage_array_kernel
        buf0[grid] ──▶ buf1[g]  ──▶ buf0[...] ── ... ──▶ 1 個
        （buf0 と buf1 を入れ替えながら、1 個になるまで繰り返す）

最後: cudaMemcpyAsync で 1 個の値をホストへ写し、cudaStreamSynchronize で待つ
```

段階 1 の `reduce_stage_kernel`（`miso/include/miso/execution.hpp`）では、範囲の中のスレッドが `map` で自分の値を作り、
範囲の外のスレッドは `init` を、それぞれ共有メモリ `s[tid]` に書きます。
そのあと `block_reduce_shared` を呼び、スレッド 0 がブロックの結果を `out[blockIdx.x]` に書きます。
1 次元と 3 次元の範囲で `map` の呼び方を変えるのに、第 6 章の `if constexpr` を使っています。

途中の結果を置く `buf0`, `buf1` はデバイスのメモリで、`ReduceHelper<T>` が `cudaMalloc` で確保して持っています。
`reduce` を呼ぶたびに確保・解放すると時間がかかるので、何度も呼ぶ場所では `ReduceHelper` を用意して使い回します。
`Integrator<Real, backend::CUDA>` は、時間刻みを求める `cfl` のために `ReduceHelper<Real> cfl_helper;` をメンバに持っています。

```cpp
    const auto op = MISO_LAMBDA(Real a, Real b) { return util::min2(a, b); };
    const Real dt_max = 1.e10;
    const auto dt = reduce(backend::CUDA{}, range, dt_max, f, op, cfl_helper);
```

最後の引数 `cfl_helper` を省いた形もあり、そのときは `reduce` の中で一時的な `ReduceHelper` を作ります。

### 6.4 初期値は op の単位元にする

CUDA 版では、範囲の外のスレッドが `init` を共有メモリに書き、それも縮約に加わります。
そのため `init` は、`op` で合わせても相手の値を変えない値（**単位元**）にします。

| 縮約 | `op` | `init` |
|---|---|---|
| 合計 | `a + b` | `0` |
| 最小値 | `util::min2(a, b)` | 十分大きな値（`cfl` では `1.e10`） |
| 最大値 | `util::max2(a, b)` | 十分小さな値 |

Host 版の `init` は「最初の値」として 1 回だけ加わるので、単位元でなくても動きます。
同じ呼び出しでも、Host 版と CUDA 版で結果が変わることを確かめました（10 個の要素がすべて 1 で、合計を求める例）。

```text
init = 0: CPU 10, GPU 10
init = 5: CPU 15, GPU 1240   ← 範囲外の 246 スレッドぶんの 5 も足される
```

### 6.5 縮約の順序と丸め誤差

Host 版は前から順に、CUDA 版はトーナメントの形で合わせます。
最小値・最大値なら順序によらず同じ値になりますが、浮動小数点の合計は順序で丸め誤差が変わります。
4.3 の例で `integral` の最後の桁が CPU と GPU で違ったのはこのためです。
CPU と GPU の結果を比べるときは、{ref}`テストと計算結果の比較 <edu-ref-testing>` を参照して、許す誤差を決めます。

<details>
<summary>コラム：extern __shared__ は型によらず 1 つの実体になる</summary>

ブロック内の縮約で使う共有メモリの大きさ（スレッド数 × `sizeof(T)`）は、起動のたびに変わります。
このような共有メモリは、カーネルの中で大きさを書かずに `extern __shared__` で宣言し、
起動時の `<<<グリッド, ブロック, バイト数>>>` の 3 つ目で大きさを渡します。
MISO の `reduce` が `<<<grid, block, helper.shmem, stream>>>` と 4 つの値で起動しているのはそのためです（4 つ目は 7 節のストリームです）。

以前の MISO は、テンプレートのカーネルの中で次のように宣言していました。

```cuda
extern __shared__ T s[];
```

`T` ごとに別の配列ができるように見えますが、`extern __shared__` の配列は、テンプレートの型によらず、同じ名前の 1 つの実体として扱われます。
そのため、同じファイルで `int` の `reduce` と `double` の `reduce` を使うと、「同じ `s` が違う型で宣言された」というコンパイルエラーになりました（MISO issue #190、<https://github.com/is-isee/MISO/issues/190>）。
（`error: declaration is incompatible with previous "s"` というエラーです。）
現在の MISO は、共有メモリを型を持たないバイト列として 1 回だけ宣言し、使う側で `T *` に読み替えています。

```cuda
template <class T> __device__ inline T *reduce_shared_memory() {
  extern __shared__ __align__(16) unsigned char reduce_smem[];
  static_assert(alignof(T) <= 16, "Unsupported alignment for reduce");
  return reinterpret_cast<T *>(reduce_smem);
}
```

`__align__(16)` は、先頭の番地を 16 の倍数にそろえる指定です。
`double` などを置くには、番地が型の大きさに合った倍数になっている必要があるためです。
`static_assert` は、それより厳しいそろえ方を要求する型が使われたら、コンパイル時に止めます（第 6 章）。
`miso/tests/serial/test_execution.cu` には、`int` と `double` の `reduce` を同じファイルで使うテストがあります。

`extern __shared__` の扱いは、NVIDIA の CUDA C++ Programming Guide（<https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html>）の共有メモリの節に説明があります。

</details>

### 練習 5：縮約の段数を数える

`mhd2d_vortex` の既定の設定（`demo/mhd2d_vortex/config.yaml`）で、`i_size: 128`, `j_size: 128`, `k_size: 1`、MPI は x 方向に 2 分割です。
各ランクで `cfl` の `reduce` が扱う要素数（ゴーストセルを除いた内部の格子点数）を求め、
ブロックあたり 256 スレッドのとき、段階 1 のブロック数と、段階 2 以降のカーネルの起動回数を求めてください。

<details>
<summary>解答・解説</summary>

x 方向に 2 分割なので、各ランクの内部は 64 × 128 × 1 = 8192 点です。
`cfl` の範囲 `Range3D` は `i_margin` から `i_total - i_margin` までなど、内部の格子点だけを指します。

段階 1 のブロック数は `(8192 + 255) / 256 = 32` です。段階 1 で 32 個の値が `buf0` に残ります。
段階 2 では `cur_n = 32` に対して `g = (32 + 255) / 256 = 1` ブロックで起動し、1 個になります。
したがって段階 2 以降の起動は 1 回で、`reduce` 全体ではカーネルを 2 回起動します。
最後に 1 個の値をホストへ写し、各ランクの値を `MPI_Allreduce` の `MPI_MIN` で全体の最小値にします（第 8 章）。

</details>

(edu-ch09-sync)=
## 7. 同期とエラーの見つけ方

### 7.1 カーネルの起動は待たない

`<<< >>>` でカーネルを起動すると、ホストは**カーネルの終了を待たずに**次の行へ進みます。
GPU に仕事を預けて、CPU は先へ進むという動きです。この性質を**非同期**（asynchronous）と呼びます。

GPU への仕事は、**ストリーム**（stream）という待ち行列に入り、ストリームの中では入れた順に実行されます。
`<<< >>>` の 4 つ目の値や、`cudaMemcpyAsync` の最後の引数がストリームの指定で、省略すると既定のストリームが使われます。
MISO の `for_each` と `reduce` は、既定ではどちらも既定のストリームを使うので、`for_each` の結果を次の `reduce` が読むときに順序は守られます。

ホストが GPU の結果を使う前には、仕事の完了を待つ（**同期**する）必要があります。

| 関数 | 待つもの |
|---|---|
| `cudaMemcpy` | それまでに既定のストリームに入れた仕事が終わってから写し、写し終えるまで待つ |
| `cudaDeviceSynchronize()` | その GPU のすべての仕事 |
| `cudaStreamSynchronize(stream)` | 指定したストリームの仕事 |
| `cudaMemcpyAsync` | 待たない。写しをストリームに入れるだけ |

4.3 の例で、`for_each` の後にすぐ `f_host.copy_from(f)` を呼べたのは、中の `cudaMemcpy` がカーネルの終了を待つからです。
`reduce` は最後に `cudaStreamSynchronize` で待ってから値を返すので、戻り値はすぐに使えます。

`copy_from` には、ストリームを引数に取る非同期版（中で `cudaMemcpyAsync` を使う）もあり、その場合は呼び出す側が同期の責任を持ちます。

MISO で `cudaDeviceSynchronize` を明示的に呼んでいるのは、GPU の結果を GPU 以外のものに渡す前です。
8 節のハロー交換では、送るデータを詰めるカーネルの後、`MPI_Isend` の前に呼んでいます。
MPI は CUDA のストリームを知らないので、詰め終わる前のバッファを送らないよう、ここで待つ必要があります。

### 7.2 エラーは後で報告される

非同期であることは、エラーの見つけ方にも影響します。
カーネルの中で起きたエラー（範囲外の読み書きなど）は、起動した時点ではまだ起きていません。
カーネルが実行され、その後にホストが**同期する CUDA の関数を呼んだときに**、その戻り値として報告されます。

| 確かめる方法 | 分かること |
|---|---|
| 起動直後の `cudaGetLastError()` | 起動そのものの失敗（ブロックが大きすぎる、GPU 向けの命令がない、など） |
| 後の `cudaMemcpy`, `cudaDeviceSynchronize` などの戻り値 | カーネルの実行中に起きたエラー |

5.1 の例で、エラーの場所が `array1d.hpp`（`copy_from` の `cudaMemcpy`）と表示されたのはこのためです。
誤っていたのは `apply_wrong` のラムダですが、それが分かるのはホストが次に同期したときでした。

MISO は CUDA の関数の戻り値を `MISO_CUDA_CHECK` で調べます（`miso/include/miso/cuda_util.cuh`）。

```cpp
#define MISO_CUDA_CHECK(ans) \
  do { miso::cuda::check_error((ans), __FILE__, __LINE__); } while (0)
```

`check_error` は、エラーなら `CUDA Error: <内容> <ファイル> <行>` を表示し、MPI が初期化されていれば `MPI_Abort` で全ランクを止めます。
`__FILE__` と `__LINE__` は、その行のファイル名と行番号に置き換わるマクロです。
表示されるのは「エラーに気づいた場所」で、「エラーが起きた場所」とは限らないことに注意します。

起きた場所を突き止めるには、CUDA Toolkit に含まれる `compute-sanitizer` で実行します。
どのカーネルのどのスレッドが不正な読み書きをしたかが表示されます。
使い方と 5.1 の例での表示は {ref}`GPU で困ったとき <edu-ref-gpu-troubleshooting>` にまとめています。

(edu-ch09-mpi)=
## 8. GPU の選択と MPI

### 8.1 ランクごとに GPU を選ぶ

1 台の計算機に GPU が複数ある場合、MPI のランクごとに使う GPU を決めます。
MISO では、`main` の最初に作る `Env` がこれを行います（`miso/include/miso/env.hpp`）。

```cpp
struct Env {
  mpi::Env mpi_env;
#ifdef __CUDACC__
  cuda::Env cuda_env;
#endif
```

CUDA ビルドでは、`mpi::Env` で MPI を初期化した後に `cuda::Env` が作られます。
`cuda::Env` のコンストラクタの中心部分です。

```cpp
    cudaGetDeviceCount(&internal::device_count);
    if (internal::device_count == 0) {
      throw std::runtime_error("No CUDA-capable devices were detected.");
    }
    internal::device_id = mpi::local_rank() % internal::device_count;
    cudaSetDevice(internal::device_id);
```

`mpi::local_rank()` は、同じ計算機（ノード）の中でのランク番号です。
`mpi::Env` が `MPI_Comm_split_type` で、同じノードのプロセスだけの通信グループを作って求めています。
ノード内のランク番号を GPU の数で割った余りを GPU の番号にします。
GPU が 2 つの計算機で 3 ランクを動かし、各ランクの `cuda::device_id()` を表示すると次のようになりました。

```text
rank 0: local rank 0, device 0 of 2
rank 1: local rank 1, device 1 of 2
rank 2: local rank 2, device 0 of 2
```

ランク 2 はランク 0 と同じ GPU を分け合うので、通常はノードあたりのランク数を GPU の数に合わせます。

### 8.2 GPU のメモリを MPI で送る

第 8 章のハロー交換では、隣のランクへ送る値をバッファに詰め、`MPI_Isend`・`MPI_Irecv` で送受信しました。
CUDA 版では、物理量の配列がデバイスにあるので、詰める処理もカーネルで行います。
`miso/include/miso/mhd_halo_exchange.hpp` の `HaloExchanger<Real, backend::CUDA>::apply` の x 方向の一部です。

```cuda
    if (mpi_shape.x_procs_pos != MPI_PROC_NULL && grid.i_total > 1) {
      pack_x_send<<<cu_shape.grid_dim_x_margin, cu_shape.block_dim>>>(
          buff.view(), qq_trgt.view(), grid.view(), Face::Pos);
      MISO_CUDA_CHECK(cudaGetLastError());
      MISO_CUDA_CHECK(cudaDeviceSynchronize());

      MPI_Isend(buff.send_x_pos,
                grid.i_margin * grid.j_total * grid.k_total * n_fields,
                mpi::data_type<Real>(), mpi_shape.x_procs_pos, 100,
                mpi_shape.cart_comm, &reqs[req_count++]);
```

流れは次のとおりです。

1. `pack_x_send` カーネルで、9 つの物理量の端の値を送信バッファ `buff.send_x_pos` に詰める。
2. `cudaDeviceSynchronize()` で、詰め終わるのを待つ（7.1）。
3. `MPI_Isend`・`MPI_Irecv` に、**デバイスのメモリにあるバッファの番地をそのまま渡す**。
4. `MPI_Waitall` で送受信の完了を待ってから、`unpack_x_recv` カーネルで受信バッファの値をゴーストセルへ書く。

送受信バッファ（`miso/include/miso/mhd_buffers.hpp` の `Buffers<Real, backend::CUDA>`）は `cudaMalloc` で確保したデバイスのメモリです。
デバイスの番地を直接受け取って送受信できる MPI を **CUDA-aware MPI** と呼びます。
MISO の CUDA 版は CUDA-aware MPI を前提にしていて、ホストのバッファを経由して送る経路は持っていません。
GPU で複数ランクを動かすには、CUDA に対応してビルドされた MPI（NVIDIA HPC SDK に含まれる Open MPI など）を使います。

GPU の計算の完了待ち（`cudaDeviceSynchronize`）と、MPI の通信の完了待ち（`MPI_Waitall`）は別のものです。
送る前には GPU の完了を、受け取った値を使う前には MPI の完了を待ちます。

### 8.3 実行

GPU ビルドの実行も、CPU ビルドと同じく `mpiexec` で行います。
MISO の demo の `app_run.sh` は、`demo/shared/openmpi_config.env` の環境変数を読み込んでから 2 ランクで実行します。

```bash
set -a
source /path/to/MISO/demo/shared/openmpi_config.env
set +a
mpiexec -np 2 ./build/mhd2d_vortex --config=config.yaml
```

`mhd2d_vortex` を GPU 2 枚・2 ランクで実行すると、CPU ビルドと同じ形式で時刻と出力番号が表示され、`tend: 0.8` まで進みます。

(edu-ch09-build)=
## 9. MISO を GPU 向けにビルドする

### 9.1 CUDA Toolkit と GPU の世代

GPU 向けにビルドするには、CPU 用の道具（第 1 章）に加えて、NVIDIA の CUDA Toolkit（`nvcc` と CUDA の実行時ライブラリ）が必要です。
Mac には NVIDIA の GPU がないので、GPU ビルドは NVIDIA の GPU を積んだ Linux の計算機で行います。

`nvcc` は、GPU の世代（**compute capability**）ごとに異なる命令を作ります。
CMake では `CMAKE_CUDA_ARCHITECTURES` に、compute capability から小数点を除いた数を指定します。

| GPU の例 | compute capability | `CMAKE_CUDA_ARCHITECTURES` |
|---|---|---|
| V100 | 7.0 | `70` |
| A100 | 8.0 | `80` |
| RTX 4090、L40 | 8.9 | `89` |
| H100 | 9.0 | `90` |

手元の GPU の値は、次のコマンドで分かります（新しめのドライバの場合）。

```bash
nvidia-smi --query-gpu=name,compute_cap --format=csv
```

MISO の `miso/CMakeLists.txt` は、`CMAKE_CUDA_ARCHITECTURES` を指定しなければ `native`（ビルドする計算機の GPU に合わせる）にします。
ビルドする計算機に GPU がない場合（ログインノードでビルドして計算ノードで実行する場合など）は、使う GPU の値を明示します。
コメントにあるとおり、この指定は `enable_language(CUDA)` より前に決める必要があります。
後から決めると、CMake が既定の古い値を先に使ってしまうためです。

使う GPU に合わない値でビルドすると、ビルドは通っても実行時にカーネルの起動が失敗します（{ref}`GPU で困ったとき <edu-ref-gpu-troubleshooting>`）。

### 9.2 ユニットテストと demo

MISO のユニットテストを GPU 向けにビルドするには、`-DMISO_USE_CUDA=ON` を付けます。
`miso/tests/` の `*.cu` のテスト（`test_execution.cu` など）は、このときだけビルドされます。

```bash
cd /path/to/MISO
cmake -B build_gpu -S miso -DMISO_USE_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES=89
cmake --build build_gpu -j
(cd build_gpu && ctest --output-on-failure)
```

demo は `-DUSE_CUDA=ON` です（4.4 の CMake 設定）。

```bash
cmake -B demo/mhd2d_vortex/build -S demo/mhd2d_vortex -DUSE_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES=89
cmake --build demo/mhd2d_vortex/build -j
./demo/mhd2d_vortex/app_run.sh
```

demo の `app_run.sh` は `build/` の中の実行ファイルを使います。
CPU 版から GPU 版へ切り替えるときは、CMake のキャッシュに前の設定が残らないよう、`build/` を消してから作り直します（`app_clean.sh` は `build/` と一緒に出力の `data/` と `figs/` も消します）。

(edu-ch09-practice)=
## 10. CPU で作ったプログラムを GPU で動かす

第 7 章で作った {ref}`配列計算の実習 <edu-practice-execution>`（ベクトル加算、縮約、1 次元移流）を、GPU 用のビルドフォルダでビルドして動かします。
4.4 の CMake 設定を自分の `CMakeLists.txt` に足します。`Backend` は、{ref}`実習の入口 <edu-practice-execution>`の `#ifdef __CUDACC__` で切り替えます（4.3 の `USE_CUDA` で切り替えても結果は同じです）。
書き直すときは、次の点を確かめてください。

- `for_each`・`reduce` に渡すラムダは `MISO_LAMBDA` で書き、`[&]` を使っていない。
- ラムダが取り込む配列は、`Backend` の配列から作った View である。所有する配列そのものを取り込んでいない。
- ラムダの中で使う名前は、ローカル変数か引数である。クラスの中に書いた場合、メンバ変数を直接使っていない（5 節）。
- `reduce` の `init` は `op` の単位元である（6.4）。
- 結果を表示する前に、`backend::Host` の配列へ `copy_from` で写している。

CPU と GPU で同じ入力を与え、ベクトルの各要素、総和、移流後の分布を比べます。
各要素の値は一致することが多いですが、総和は加算の順序の違いで最後の桁が変わることがあります（6.5）。

GPU 環境がまだない場合は、比較を後で追加することにして、{ref}`第 10 章 <edu-ch10>` へ進めます。
CPU で熱伝導の逐次版と MPI 版を作り、GPU 版はこの章の内容を使って後から加えます。
