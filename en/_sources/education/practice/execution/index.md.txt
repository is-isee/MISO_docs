(edu-practice-execution)=
# 配列と並列ループの実習

{ref}`第 7 章 <edu-ch07>`で学んだ配列、View、`for_each`、`reduce` を使い、次の 3 つのプログラムを順に作ります。
まず CPU（`backend::Host`）で 3 つとも作ります。
{ref}`第 9 章 <edu-ch09>`を読んで GPU を使える段階になったら、同じソースを CUDA でも動かします。

1. {ref}`ベクトル加算 <edu-practice-execution-01>` — 配列を確保し、`for_each` で各要素を計算して、結果をホスト側の配列で確かめます。
2. {ref}`縮約 <edu-practice-execution-02>` — `reduce` で配列の二乗和と最大値を求めます。
3. {ref}`1 次元移流 <edu-practice-execution-03>` — ゴーストセルと周期境界、更新前後の 2 つの配列を使って時間発展を計算します。

`main.cpp` と `CMakeLists.txt` は自分で書きます。
MISO 本体のビルドやデモの実行は必要ありません。

## CMake の設定

{ref}`第 1 章 <edu-ch01>`の「自分のプログラムから MISO を使う最小の CMake 設定」を使います。
`add_subdirectory` で MISO の `miso/` を読み込み、自分の実行ファイルに `MISO::MISO` をリンクする設定です。
MISO の場所は、CMake を実行するときに `-DMISO_DIR=/path/to/MISO` で渡します（`/path/to/MISO` は自分が MISO を置いた場所に置き換えます）。

```bash
cmake -S . -B build -DMISO_DIR=/path/to/MISO
cmake --build build -j
./build/vector_add
```

3 つの課題は、1 つの `CMakeLists.txt` に実行ファイルを 3 つ並べても、課題ごとにフォルダを分けてもかまいません。
`MISO::MISO` をリンクすると MPI と yaml-cpp もリンクされますが、この実習で読み込むヘッダは MPI を使わないので、`mpiexec` なしで実行できます。

## 使うヘッダ

`<miso/core.hpp>` はすべての共通ヘッダをまとめて読み込みますが、この実習では使う機能のヘッダだけを読み込みます。

| 機能 | ヘッダ |
|---|---|
| 1 次元の配列 `Array1D` と `Array1DView` | `<miso/array1d.hpp>` |
| `Range1D`, `for_each`, `reduce`, `MISO_LAMBDA` | `<miso/execution.hpp>` |
| `util::sin`, `util::exp`, `util::fmod`, `util::fmax` などの数学関数 | `<miso/utility.hpp>` |

名前空間は、MISO のデモと同じく `using namespace miso;` と書いてよいです。

## GPU へ切り替えられる書き方

後で同じソースを CUDA でも動かすために、次の 2 点を守って書きます。

**計算する場所を 1 か所で決めます。**
`backend::Host` を何か所にも書かずに、型の別名 `Backend` を 1 つ用意して使います。

```cpp
#ifdef __CUDACC__
using Backend = backend::CUDA;
#else
using Backend = backend::Host;
#endif
```

`__CUDACC__` は CUDA のコンパイラがコンパイルするときだけ定義されます（{ref}`第 7 章 <edu-ch07>`の `MISO_LAMBDA`）。
CPU のビルドでは `Backend` は `backend::Host` です。
配列は `Array1D<double, Backend>`、ループは `for_each(Backend{}, ...)` と書きます。

**結果はホスト側の配列へ写してから読みます。**
GPU に置いた配列は、CPU のループから直接読めません。
結果を確かめるときは、`Array1D<double, backend::Host>` を別に用意し、`copy_from` で写してから読みます。

```cpp
Array1D<double, backend::Host> h(n);
h.copy_from(z);  // copy the elements of z to h
```

CPU のビルドでは `z` もホスト側にあるので、`copy_from` はただのコピーです。
CUDA のビルドでは、同じ 1 行が GPU から CPU への転送になります（{ref}`第 9 章 <edu-ch09>`）。

```{toctree}
:hidden:
:maxdepth: 1

01_vector_add
02_reduction
03_advection_1d
```
