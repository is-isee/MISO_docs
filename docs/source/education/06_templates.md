(edu-ch06)=
# 6. テンプレート

## 目次

- {ref}`はじめに <edu-ch06-intro>`
- {ref}`6.1 関数テンプレート <edu-ch06-function-templates>`
- {ref}`6.2 クラステンプレート <edu-ch06-class-templates>`
- {ref}`6.3 変数テンプレート <edu-ch06-variable-templates>`
- {ref}`6.4 タグ型によるオーバーロードの選択 <edu-ch06-tag-dispatch>`
- {ref}`6.5 テンプレートの特殊化 <edu-ch06-specialization>`
- {ref}`6.6 if constexpr と static_assert <edu-ch06-if-constexpr>`
- {ref}`6.7 型の性質を調べる仕組み <edu-ch06-type-traits>`
  - {ref}`6.7.1 型の性質を値として持つ構造体 <edu-ch06-true-false-type>`
  - {ref}`6.7.2 std::void_t によるメンバの検査 <edu-ch06-void-t>`
  - {ref}`6.7.3 マクロとテンプレートの組み合わせ（ソース項） <edu-ch06-source-term-macro>`
  - {ref}`6.7.4 テンプレートの中の .template <edu-ch06-dot-template>`
- {ref}`練習：テンプレートの指定と選択 <edu-ch06-exercises>`

(edu-ch06-intro)=
## はじめに

MISO の計算は、単精度（`float`）でも倍精度（`double`）でも、CPU でも GPU でも動きます。
demo の `main.cpp` では、次の 2 行でそれを選んでいます（`demo/mhd2d_vortex/src/main.cpp` から抜粋）。

```cpp
using Real = float;

#ifdef USE_CUDA
using Backend = backend::CUDA;
#else
using Backend = backend::Host;
#endif
```

ライブラリ側（`miso/include/miso/`）には、`float` 用と `double` 用、CPU 用と GPU 用の計算を別々に書いてはいません。
型を「後から指定する名前」にしておき、使う側が `Real` や `Backend` を与えた時点で、コンパイラが具体的な関数やクラスを作ります。
この仕組みを**テンプレート**（template）と呼びます。

{ref}`第 5 章 <edu-ch05>`では `ModelBase<Model, Real, Backend>` の山括弧に型を渡すことだけを使いました。
この章では、関数・クラス・変数のテンプレートの書き方から始め、
MISO が `Backend` に応じて CPU 用と GPU 用の処理を選ぶ仕組み、モデルに必要なメンバがあるかをコンパイル時に調べる仕組みまでを読みます。
最後の 6.7 節は、自分で書けなくても、MISO のソースを読めれば十分です。

(edu-ch06-function-templates)=
## 6.1 関数テンプレート

二乗を計算する式は、整数でも実数でも `x * x` です。
{ref}`第 2 章 <edu-ch02>`のオーバーロードでは、`int` 用と `double` 用の関数をそれぞれ定義しました。
処理の形が型によらず同じなら、型の部分を `T` という仮の名前にして、1 つの定義にまとめられます。

```cpp
#include <iostream>

template <typename T>  // T は、使うときに決める型の名前です
T square(T x) {        // 引数と戻り値に、同じ T を使います
  return x * x;
}

int main() {
  double a = square<double>(1.5);  // T を double と指定します。2.25
  int b = square<int>(3);          // T を int と指定します。9
  auto c = square(2.5f);           // 引数 2.5f（float）から、T を float と決めます。6.25
  std::cout << a << " " << b << " " << c << "\n";
}
```

```text
2.25 9 6.25
```

このように定義した関数を**関数テンプレート**（function template）と呼びます。

- `template <typename T>` は「この後の定義では、`T` を型の名前として使う」という宣言です。`typename` の代わりに `class` と書いても同じ意味です。`class` と書いても、`T` がクラス型に限られるわけではありません。
- `square<double>` の山括弧に書く `double` を**テンプレート引数**（template argument）と呼びます。`(1.5)` は値の引数で、役割が違います。
- `square(2.5f)` のように山括弧を省くと、コンパイラは引数の型から `T` を決めます。これを**テンプレート引数の推論**（template argument deduction）と呼びます。

コンパイラは、`square<double>` が使われた時点で、`T` を `double` に置き換えた関数を作ります。
これを**実体化**（instantiation）と呼びます。実行中に型を取り替えるのではなく、コンパイル時に型ごとの関数が用意されます。
`square<int>` と `square<double>` は、名前が似た別々の関数です。

推論には、型が一つに決まる必要があります。次の `max_of` は 2 つの引数に同じ `T` を使っているので、
`int` と `double` を混ぜて渡すとコンパイルエラーになります。

```cpp
template <typename T> T max_of(T a, T b) { return a > b ? a : b; }

double x = max_of(1, 2.5);          // エラー：T が int か double か決まりません
double y = max_of<double>(1, 2.5);  // T を double と指定すれば、1 は 1.0 に変換されます
```

g++ では次のような説明が出ます。

```text
error: no matching function for call to 'max_of(int, double)'
note:   deduced conflicting types for parameter 'T' ('int' and 'double')
```

**テンプレートの定義はヘッダに置きます。**
コンパイラは実体化のときに関数の本体を見る必要があるので、テンプレートは宣言だけでなく本体まで、使う側から見える場所に書きます。
MISO がヘッダオンリー（`.hpp` だけのライブラリ）なのは、ほとんどがテンプレートだからです。
{ref}`第 2 章 <edu-ch02>`で、ヘッダに置く通常の関数には `inline` が必要だと説明しました。
関数テンプレートは、同じ定義が複数の `.cpp` に現れても許される決まりなので、`inline` がなくてもリンクの問題は起きません。
MISO では、テンプレートにも `inline` を付けている箇所があります。付けても意味は変わりません。

### MISO の例：4 次の中心差分

`miso/include/miso/mhd_integrator.hpp` の `space_centered_4th` は、`Real` を後から決める関数テンプレートです。
宣言の部分だけを示します（本体は第 11 章で読みます）。

```cpp
template <typename Real>
__host__ __device__ inline Real
space_centered_4th(Array3DView<const Real> qq, Real dxyzi, int i, int j, int k,
                   int is, int js, int ks);
```

- `__host__ __device__` は、この関数を CPU からも GPU からも呼べるようにする指定です（{ref}`第 9 章 <edu-ch09>`）。CPU だけのビルドでは空の文字列に置き換わります。
- 呼び出す側は `space_centered_4th(qq_view, dxi, i, j, k, 1, 0, 0)` のように `<Real>` を書きません。`qq_view` の型が `Array3DView<const float>` なら、`Real` は `float` と推論されます。
  `Array3DView<const Real>` のように、別のテンプレートの山括弧の中にある `Real` も推論の手がかりになります。

(edu-ch06-class-templates)=
## 6.2 クラステンプレート

クラス（構造体）にも、型を後から指定する仕組みがあります。
次は、同じ型の値 2 つと、それらを足すメンバ関数をまとめた**クラステンプレート**（class template）です。

```cpp
#include <iostream>

template <typename T>
struct ValuePair {
  T first;   // 2 つのメンバに、同じ型 T を使います
  T second;
  T sum() const { return first + second; }
};

int main() {
  ValuePair<double> reals{1.5, 2.5};  // double のメンバを 2 つ持つ実体
  ValuePair<int> integers{2, 3};      // int のメンバを 2 つ持つ実体
  std::cout << reals.sum() << " " << integers.sum() << "\n";  // 4 5
}
```

`ValuePair<double>` 全体で 1 つの型の名前です。`ValuePair<int>` は別の型で、互いに代入はできません。
どちらも同じ定義から作られますが、`ValuePair` だけでは型になりません（C++17 には、初期値から山括弧の中を推論する仕組みもありますが、この教材では使いません）。

{ref}`第 4 章 <edu-ch04>`の `std::vector<double>` や `Array3D<Real>` の山括弧も、このテンプレート引数です。

### テンプレート引数の既定値

関数の引数と同じく、テンプレート引数にも既定値を付けられます。
`miso/include/miso/array3d.hpp` の宣言は次のとおりです。

```cpp
/// @brief 3D Array in general execution/memory space.
template <typename T, typename Backend = backend::Host> class Array3D;
```

`T` は要素の型、`Backend` は配列を置く場所です。
`Array3D<float>` と書けば `Array3D<float, backend::Host>` の意味になり、CPU のメモリに置かれます。
GPU のメモリに置くときは `Array3D<float, backend::CUDA>` と、2 つ目を書きます。
`Fields`（`mhd_fields.hpp`）、`Grid`（`grid.hpp`）なども、同じように `Backend` の既定値を `backend::Host` にしています。

### MISO の例：Real を受け取るクラス

MISO のクラスの多くは、`Real` を受け取るクラステンプレートです。
`miso/include/miso/eos.hpp` の理想気体の状態方程式を、メンバ変数とコンストラクタだけにすると次の形です。

```cpp
template <typename Real> struct IdealEOS {
  /// @brief Ratio of specific heats (gamma)
  Real gm;

  /// @brief Construct the EOS from the configuration file
  explicit IdealEOS(const Config &config) : gm(config["eos"]["gm"].as<Real>()) {}
  // （以下省略）
};
```

demo では `eos::IdealEOS<Real> eos;` と宣言します。`Real` が `float` なら、`gm` は `float` のメンバになります。
`config["eos"]["gm"].as<Real>()` の `as<Real>` も、設定ファイルの値を `Real` 型で取り出す関数テンプレートの呼び出しです（yaml-cpp の機能）。

{ref}`第 5 章 <edu-ch05>`の `ModelBase` は、テンプレート引数を 3 つ持つクラステンプレートです。

```cpp
template <class Derived, class Real, class Backend> class ModelBase {
public:
  Config &config;
  mpi::Shape mpi_shape;
  Time<Real> time;
  Grid<Real, backend::Host> grid;

  ExecContext<Real, Backend> exec_ctx;
  MHD<Real, Backend> mhd;
  // （以下省略）
};
```

（`miso/include/miso/mhd_model_base.hpp` から抜粋）

受け取った `Real` と `Backend` を、そのまま `Time<Real>` や `MHD<Real, Backend>` のテンプレート引数に渡しています。
demo が `Real` と `Backend` を一度決めると、それがメンバのクラスへ順に伝わり、MISO 全体の型が決まります。
ここで `grid` だけは `Backend` によらず `backend::Host` です。格子の座標は、まず CPU 側で用意するためです。

(edu-ch06-variable-templates)=
## 6.3 変数テンプレート

$\pi$ や $1/12$ のような定数を、`Real` の精度で使いたいことがあります。
`float` の計算に `double` の定数を混ぜると、計算全体が `double` で行われてから `float` に戻され、遅くなったり、型の不一致で警告が出たりします。
型ごとに定数を用意するには、変数にもテンプレートを使います。これを**変数テンプレート**（variable template）と呼びます。

```cpp
#include <cstdio>

template <typename T>
inline constexpr T third = static_cast<T>(1.0 / 3.0);  // 1/3 を T に変換した定数

int main() {
  float a = third<float>;    // float に丸めた 1/3
  double b = third<double>;  // double の 1/3
  std::printf("%.17g\n%.17g\n", a, b);
}
```

```text
0.3333333432674408
0.33333333333333331
```

`third<float>` と `third<double>` は別々の定数で、それぞれの型の精度で値を持ちます。

- `constexpr` は、値がコンパイル時に決まる定数であることを表します（{ref}`第 2 章 <edu-ch02>`）。
- `static_cast<T>(...)` は、`double` で計算した `1.0 / 3.0` を `T` に変換します。
- `inline` は、この定義をヘッダに置き、複数の `.cpp` から読み込んでもよいことを表します（C++17 から変数にも付けられます）。

### MISO の例：constants.hpp

`miso/include/miso/constants.hpp` の全体です。

```cpp
#pragma once

#include <cmath>

namespace miso {

template <typename Real>
inline constexpr Real inv12 = static_cast<Real>(1.0 / 12.0);

template <typename Real> inline constexpr Real pi = static_cast<Real>(M_PI);

template <typename Real>
inline constexpr Real pii4 = static_cast<Real>(1.0 / (4.0 * M_PI));  // 1 / 4π

template <typename Real>
inline constexpr Real pii8 = static_cast<Real>(1.0 / (8.0 * M_PI));  // 1 / 8π

}  // namespace miso
```

使う側は `pi<Real>`、`pii4<Real>` と書きます。たとえば `demo/mhd2d_vortex/src/main.cpp` の初期条件には
`util::sin(2.0 * pi<Real> * grid.y[j])` があり、`mhd_integrator.hpp` の差分では `inv12<Real>` を掛けています。
`miso::pii4<float>` と `miso::pii4<double>` を表示すると、次のように精度が違います。

```text
0.07957746833562851
0.079577471545947673
```

`M_PI` は C++ 規格の定数ではなく、`<cmath>` が多くの環境で用意するマクロです。
C++20 には `std::numbers::pi_v<T>` という同じ役割の変数テンプレートがありますが、MISO は C++17 なので自前で定義しています。

(edu-ch06-tag-dispatch)=
## 6.4 タグ型によるオーバーロードの選択

同じ呼び出しの形のまま、CPU 用と GPU 用の処理をコンパイル時に選びたい場面を考えます。
{ref}`第 2 章 <edu-ch02>`のオーバーロードでは、引数の型によって呼ばれる関数が決まりました。
そこで、「どちらで計算するか」を表すだけの、中身のない型を用意して引数に渡します。

```cpp
#include <iostream>

struct Cpu {};  // メンバを持たない型（タグ型）
struct Gpu {};

void run(Cpu, int n) { std::cout << "Cpu 版: n = " << n << "\n"; }
void run(Gpu, int n) { std::cout << "Gpu 版: n = " << n << "\n"; }

template <typename Backend>
void compute(Backend b, int n) {
  run(b, n);  // b の型（Cpu か Gpu）で、呼ばれる run が決まります
}

int main() {
  compute(Cpu{}, 10);  // Backend は Cpu と推論されます
  compute(Gpu{}, 20);  // Backend は Gpu と推論されます
}
```

```text
Cpu 版: n = 10
Gpu 版: n = 20
```

- `Cpu{}` は、`Cpu` 型の値を初期値なしで 1 つ作る式です。中身がないので、値に意味はなく、**型だけ**が情報になります。
- `void run(Cpu, int n)` のように、使わない引数は名前を省略できます。型を見分けるためだけの引数です。
- `compute` の中の `run(b, n)` がどちらの `run` を呼ぶかは、コンパイル時に決まります。実行時に `if` で分岐するわけではありません。

このように、選択の目印にする中身のない型を**タグ型**（tag type）と呼び、
タグ型の引数でオーバーロードを選ぶ書き方を**タグディスパッチ**（tag dispatch）と呼びます。

### MISO の例：backend.hpp と for_each

MISO のタグ型は `miso/include/miso/backend.hpp` にある 2 つです。

```cpp
namespace miso {
namespace backend {

/// @brief Tag type representing CPU (host) execution backend.
/// @details Used for compile-time backend dispatch and specialization.
struct Host {};

/// @brief Tag type representing CUDA (GPU device) execution backend.
/// @details Used for compile-time backend dispatch and specialization.
struct CUDA {};

}  // namespace backend
}  // namespace miso
```

並列ループの `for_each`（`miso/include/miso/execution.hpp`）は、1 つ目の引数のタグ型で CPU 用と GPU 用を選びます。
本体を省略し、宣言の形だけを示します。

```cpp
/// @brief 1D parallel (concurrent) for loop in the Host backend.
template <class F> inline void for_each(backend::Host, Range1D range, F f) {
  // CPU 用：普通の for 文（第 7 章）
}

#ifdef __CUDACC__
/// @brief 1D parallel (concurrent) for loop in the CUDA backend.
template <class F> inline void for_each(backend::CUDA, Range1D range, F f) {
  // GPU 用：カーネルを起動します（第 9 章）
}
#endif
```

（説明用のコメントに置き換えています。3 次元の `Range3D` 用も同じ形で並んでいます。）

- 1 つ目の引数は名前のないタグ型です。上の `run(Cpu, int)` と同じ形です。
- `F` は、ループの中身として渡すラムダ式（名前のない関数）の型です。ラムダ式は {ref}`第 7 章 <edu-ch07>`で説明します。
- CUDA 版は `#ifdef __CUDACC__` の中にあり、CUDA のコンパイラ（nvcc）でビルドするときだけ定義されます。CPU だけのビルドで `backend::CUDA{}` を渡すと、該当する `for_each` がないのでコンパイルエラーになります。

タグを受け取って下へ渡す例が、`miso/include/miso/eos.hpp` の `IdealEOS` のメンバ関数です（本体は第 7 章で読みます）。

```cpp
  template <typename Backend>
  void gas_pressure(Backend btag, mhd::FieldsView<const Real> qq,
                    Array3DView<Real> pr) const {
    // （中略）
    for_each(btag, range, /* ラムダ式 */);
  }
```

`gas_pressure` は、クラステンプレートの中で、さらにテンプレートにしたメンバ関数です（**メンバ関数テンプレート**）。
CPU 用の `Integrator`（`miso/include/miso/mhd_integrator_host.hpp`）は
`eos.gas_pressure(backend::Host{}, qq_argm.const_view(), pr.view());` と呼び、GPU 用は `backend::CUDA{}` を渡します。
`backend::Host{}` を渡すと `Backend` は `backend::Host` と推論され、中の `for_each` も CPU 版が選ばれます。
状態方程式の側は CPU と GPU を区別せずに 1 回書くだけで済みます。

タグの値は `backend::Host{}` のほか、型の名前 `Backend` から `Backend{}` としても作れます。
`miso/include/miso/mhd_resistivity.hpp` の `ReduceWorkspace` には `miso::reduce(Backend{}, range, init, f, op)` という呼び出しがあります。

(edu-ch06-specialization)=
## 6.5 テンプレートの特殊化

関数テンプレートやクラステンプレートは、どの型にも同じ定義を使います。
特定の型のときだけ別の定義を使いたい場合は、その型用の定義を追加します。これを**特殊化**（specialization）と呼びます。

### 6.5.1 完全な特殊化

テンプレート引数をすべて決めた定義を、**完全な特殊化**（explicit specialization、明示的特殊化とも）と呼びます。
`template <>` と山括弧を空にして書きます。

```cpp
#include <iostream>

template <typename T> const char *type_name();  // 宣言だけ。本体はありません
template <> const char *type_name<float>() { return "float"; }    // T = float のとき
template <> const char *type_name<double>() { return "double"; }  // T = double のとき

int main() {
  std::cout << type_name<float>() << " " << type_name<double>() << "\n";  // float double
}
```

引数がないので推論はできず、`type_name<float>()` のように型を明示して呼びます。
`type_name<int>()` を呼ぶと、本体のある定義がないのでリンク時にエラーになります。

MISO では、`miso/include/miso/mpi_util.hpp` が `Real` に対応する MPI のデータ型を返すのに、この形を使っています。

```cpp
/// @brief MPI Datatype corresponding to Real type
template <typename Real> MPI_Datatype data_type();
template <> inline MPI_Datatype data_type<float>() { return MPI_FLOAT; }
template <> inline MPI_Datatype data_type<double>() { return MPI_DOUBLE; }
```

`mpi::data_type<Real>()` と書けば、`Real` が `float` なら `MPI_FLOAT` が返ります（{ref}`第 8 章 <edu-ch08>`）。
完全な特殊化はもうテンプレートではない普通の関数なので、ヘッダに置くには `inline` が必要です。

### 6.5.2 部分特殊化

クラステンプレートでは、テンプレート引数の**一部だけ**を決めた定義も書けます。これを**部分特殊化**（partial specialization）と呼びます。
要素の型 `T` は自由なまま、置き場所 `Backend` ごとに中身を変える例です。

```cpp
#include <iostream>

struct Cpu {};
struct Gpu {};

template <typename T, typename Backend> struct Storage;  // 宣言だけ（基本のテンプレート）

template <typename T> struct Storage<T, Cpu> {  // Backend が Cpu のときの定義
  void describe() const { std::cout << "CPU のメモリに置きます\n"; }
};

template <typename T> struct Storage<T, Gpu> {  // Backend が Gpu のときの定義
  void describe() const { std::cout << "GPU のメモリに置きます\n"; }
};

int main() {
  Storage<float, Cpu> a;   // T = float として、Cpu 用の定義を使います
  Storage<double, Gpu> b;  // T = double として、Gpu 用の定義を使います
  a.describe();
  b.describe();
}
```

```text
CPU のメモリに置きます
GPU のメモリに置きます
```

- 最初の `template <typename T, typename Backend> struct Storage;` を**基本のテンプレート**（primary template）と呼びます。ここでは宣言だけで、本体はありません。
- `template <typename T> struct Storage<T, Cpu>` は、「`T` は自由、2 つ目は `Cpu`」に当てはまるときの定義です。`template` の後の山括弧には、まだ決まっていない `T` だけを書きます。
- `Storage<float, Cpu>` と書くと、コンパイラは当てはまる部分特殊化を探して使います。どれにも当てはまらなければ、基本のテンプレートを使います。この例では基本のテンプレートに本体がないので、`Storage<float, int>` はコンパイルエラーです。

部分特殊化ごとに、メンバ変数もメンバ関数も自由に変えられます。使う側が同じ名前のメンバ関数を呼べるよう、名前と引数を揃えておくのが普通です。

関数テンプレートには部分特殊化がありません。
`for_each` の CPU 版と GPU 版を、特殊化ではなく 6.4 節のタグ型のオーバーロードで書き分けているのはそのためです。

### MISO の例：Array3D と Integrator

`miso/include/miso/array3d.hpp` は、6.2 節で見た宣言のあと、`Backend` ごとに部分特殊化を定義しています。
本体を省略して示します。

```cpp
/// @brief 3D Array in general execution/memory space.
template <typename T, typename Backend = backend::Host> class Array3D;

/// @brief 3D Array in host memory.
template <typename T> class Array3D<T, backend::Host> {
  // new T[...] で CPU のメモリを確保します（第 4 章）
};

#ifdef __CUDACC__
/// @brief 3D Array in CUDA device memory.
template <typename T> class Array3D<T, backend::CUDA> {
  // cudaMalloc で GPU のメモリを確保します（第 9 章）
};
#endif  // __CUDACC__
```

（説明用のコメントに置き換えています。）

上の `Storage` と同じ形です。`Array3D<float, backend::Host>` と `Array3D<float, backend::CUDA>` は、
どちらも `view()`、`extent()`、`copy_from()` などの同じ名前のメンバ関数を持ちますが、メモリの確保と解放の仕方が違います。
`Array1D`、`Array2D`、`Array4D`、`Fields`、`Grid` も同じ作りです。

時間積分を受け持つ `Integrator` も、基本のテンプレートは宣言だけです（`miso/include/miso/mhd_integrator.hpp`）。

```cpp
template <typename Real, typename Backend> struct Integrator;
```

CPU 用の定義は `miso/include/miso/mhd_integrator_host.hpp`、GPU 用は `miso/include/miso/mhd_integrator_cuda.cuh` にあります。

```cpp
template <typename Real> struct Integrator<Real, backend::Host> {
  // （省略）
};
```

```cpp
template <typename Real> struct Integrator<Real, backend::CUDA> {
  // （省略）
};
```

`mhd_integrator.hpp` の末尾で、CPU 用は常に、GPU 用は `__CUDACC__` が定義されているときだけ読み込みます。

```cpp
#include "mhd_integrator_host.hpp"

#ifdef __CUDACC__
#include "mhd_integrator_cuda.cuh"
#endif
```

`Integrator` を使う側の `MHD<Real, Backend>`（`miso/include/miso/mhd_solver.hpp`）は、メンバとして
`Integrator<Real, Backend> integrator;` を持ち、`integrator.update(...)`、`integrator.cfl(...)` を呼ぶだけです。
`Backend` が決まれば、どちらの定義が使われるかも決まります。
2 つの定義は、`update`・`cfl`・`apply_boundary_condition` を同じ名前と引数の並びで持つように揃えてあり、
GPU 用だけが CUDA のカーネルの形（`cuda::KernelShape3D`）や縮約用の作業領域をメンバに持ちます。

(edu-ch06-if-constexpr)=
## 6.6 if constexpr と static_assert

### if constexpr

テンプレート引数には、型だけでなく `bool` や `int` の値も使えます。
次は、符号を反転するかどうかをテンプレート引数で指定する関数です。

```cpp
template <bool Negate>
double change_sign(double value) {
  if constexpr (Negate) {  // Negate はコンパイル時に決まっています
    return -value;
  } else {
    return value;
  }
}
```

`change_sign<true>(3.0)` は −3、`change_sign<false>(3.0)` は 3 を返します。
`if constexpr` は、条件がコンパイル時に決まるときに使う分岐です。
実体化のときに、選ばれなかった側は関数に組み込まれません。`change_sign<true>` には `return -value;` だけが残ります。
{ref}`第 2 章 <edu-ch02>`の通常の `if` は、実行時に決まる値でも条件にできますが、両方の側がコンパイルされます。
`if constexpr` の条件には、コンパイル時に値の決まる式しか書けません。

型によって処理を変えるには、`<type_traits>` の `std::is_same_v<A, B>` を使います。
`A` と `B` が同じ型なら `true`、違えば `false` になる、コンパイル時の定数です。

```cpp
#include <type_traits>

template <typename T>
const char *precision_name() {
  if constexpr (std::is_same_v<T, float>) {
    return "単精度";
  } else {
    return "倍精度以上";
  }
}
```

`precision_name<float>()` は「単精度」、`precision_name<double>()` は「倍精度以上」を返します。

選ばれなかった側が組み込まれないことには、もう一つ意味があります。
テンプレート引数によっては**書けない**処理（その型にないメンバの呼び出しなど）を、選ばれない側に置いてもエラーになりません。
6.7 節の MISO の例は、この性質を使っています。

### static_assert

テンプレート引数に条件を付けたいときは、`static_assert`（コンパイル時の検査）を使います。

```cpp
#include <type_traits>

template <typename T>
T half(T x) {
  static_assert(std::is_floating_point_v<T>, "T must be float or double");
  return x / 2;
}
```

`static_assert(条件, "メッセージ");` は、条件が `false` ならコンパイルエラーにし、メッセージを表示します。
`half(3.0)` はそのままコンパイルできますが、`half(3)` は `T` が `int` になるので、次のエラーで止まります。

```text
error: static assertion failed: T must be float or double
```

`std::is_floating_point_v<T>` は、`T` が `float`・`double` などの浮動小数点型なら `true` になる定数です。
実行時の `assert` と違い、`static_assert` は Release ビルドでも消えません。間違った使い方を、実行する前に見つけられます。

文法の詳細は C++17 規格草案 N4659 の
[テンプレートのパラメータ](https://timsong-cpp.github.io/cppwp/n4659/temp.param)、
[`if constexpr`](https://timsong-cpp.github.io/cppwp/n4659/stmt.if)、
[型の性質（type traits）](https://timsong-cpp.github.io/cppwp/n4659/meta)で確認できます。

### MISO の例：縮約カーネルの分岐

`miso/include/miso/execution.hpp` の GPU 用の縮約（`reduce`）では、範囲が 1 次元か 3 次元かで添字の取り出し方を変えています。
`Range` はテンプレート引数で、1 次元の範囲を表す `Range1D` か 3 次元の範囲を表す `Range3D`（{ref}`第 7 章 <edu-ch07>`）が入ります。

```cpp
    if constexpr (std::is_same_v<Range, Range1D>) {
      const int i = range.unravel(idx);
      v = map(i);
    } else {
      Index3D p = range.unravel(idx);
      v = map(p.i0, p.i1, p.i2);
    }
```

`Range1D` の `unravel` は `int` を、`Range3D` の `unravel` は `Index3D`（3 つの添字の組）を返します。
通常の `if` で書くと、`Range1D` のときにも `Index3D p = range.unravel(idx);` がコンパイルされて型が合わずエラーになります。
`if constexpr` なら、その型に合う側だけが組み込まれます。

(edu-ch06-type-traits)=
## 6.7 型の性質を調べる仕組み

MISO の利用者は、`ModelBase` を継承したモデルに `eos`・`ic`・`bc`・`src` というメンバを用意します（{ref}`第 5 章 <edu-ch05>`）。
ソース項 `src` には、必要な項（たとえば重力なら `vy`）だけを書けばよく、書かなかった項は 0 として扱われます。
これを実現するには、「この型に `vy` というメンバ関数があるか」をコンパイル時に調べ、結果で `if constexpr` を分ける必要があります。
この節では、その仕組みを読める程度に説明します。

(edu-ch06-true-false-type)=
### 6.7.1 型の性質を値として持つ構造体

`<type_traits>` には、`true` または `false` を表す 2 つの構造体があります。

```cpp
std::true_type::value   // true
std::false_type::value  // false
```

`value` は、クラスに属する定数（`static constexpr bool`）です。実体を作らず、`型名::value` で読めます。
構造体がこのどちらかを継承すると、その構造体の `::value` も `true` か `false` になります。

6.6 節の `std::is_same_v<A, B>` は、`std::is_same<A, B>::value` の短い書き方です。
`std::is_same<A, B>` は、`A` と `B` が同じなら `std::true_type` を、違えば `std::false_type` を継承するように作られています。
このように、型について調べた結果を `::value` で返すクラステンプレートを**型特性**（type trait）と呼びます。

(edu-ch06-void-t)=
### 6.7.2 std::void_t によるメンバの検査

「型 `T` に `gm` というメンバがあるか」を調べる型特性 `has_gm` を、2 つの定義で作ります。

```cpp
#include <iostream>
#include <type_traits>

// (1) 基本のテンプレート：何も当てはまらなければ false
template <class T, class = void>
struct has_gm : std::false_type {};

// (2) 部分特殊化：T::gm が書けるときだけ当てはまり、true
template <class T>
struct has_gm<T, std::void_t<decltype(&T::gm)>> : std::true_type {};

struct Gas {
  double gm;  // gm というメンバがあります
};
struct Dust {
  double size;  // gm はありません
};

int main() {
  std::cout << has_gm<Gas>::value << " " << has_gm<Dust>::value << "\n";  // 1 0
}
```

部品を順に読みます。

- `class = void` は、名前のない 2 つ目のテンプレート引数で、既定値が `void` です。使う側は `has_gm<Gas>` と 1 つだけ書き、2 つ目は `void` になります。
- `&T::gm` は「`T` のメンバ `gm` を指す」式です。`T` に `gm` がなければ、この式は書けません。
- `decltype(式)` は、式を**実行せずに**その型を求めます。ここでは式が書けるかどうかだけが問題です。
- `std::void_t<...>` は、山括弧の中が正しく書ければ、中身によらず `void` になる型です。

`has_gm<Gas>` の場合、コンパイラは次のように考えます。

1. `has_gm<Gas>` は、既定値を補って `has_gm<Gas, void>` です。
2. 部分特殊化 (2) に `T = Gas` を当てはめると、`std::void_t<decltype(&Gas::gm)>` は `void` になり、(2) は `has_gm<Gas, void>` の定義になります。
3. 1 と 2 が一致するので、(2) が使われ、`value` は `true` です。

`has_gm<Dust>` の場合は、2 で `&Dust::gm` が書けません。
このとき、コンパイラはエラーにせず、(2) を候補から外すだけです。残った (1) が使われ、`value` は `false` です。
テンプレート引数を当てはめた結果が書けない形になっても、その候補を外すだけでエラーにしない、という C++ の規則を
**SFINAE**（スフィネー、Substitution Failure Is Not An Error：置き換えの失敗はエラーではない）と呼びます。
規則は C++17 規格草案の[テンプレート引数の推論](https://timsong-cpp.github.io/cppwp/n4659/temp.deduct)にあります。

メンバ関数を、決まった引数の並びで**呼べるか**を調べるには、`&T::名前` の代わりに呼び出しの式を書きます。

```cpp
#include <utility>  // std::declval

template <class T, class = void> struct has_dt_limit : std::false_type {};
template <class T>
struct has_dt_limit<T, std::void_t<decltype(std::declval<const T &>().dt_limit())>>
    : std::true_type {};
```

`std::declval<const T &>()` は、「`const T &` 型の値があるとしたら」という仮の式で、`decltype` の中だけで使えます。
`T` の実体を作らずに、`t.dt_limit()` が書けるかを調べられます。`const T &` から呼ぶので、`dt_limit` が `const` メンバ関数でなければ `false` になります。

```cpp
struct A { double dt_limit() const { return 0.1; } };  // has_dt_limit<A>::value は true
struct B { double dt_limit() { return 0.1; } };        // const がないので false
struct C {};                                           // dt_limit がないので false
```

#### MISO の例：has_eos と has_dt_limit

`miso/include/miso/mhd_model_base.hpp` の冒頭には、上と同じ形の型特性が並んでいます（抜粋）。

```cpp
// SFINAE to check if the derived class has required member variables.
template <class T, class = void> struct has_eos : std::false_type {};
template <class T>
struct has_eos<T, std::void_t<decltype(&T::eos)>> : std::true_type {};

// Optional time step limit of the source term: Real dt_limit() const
template <class T, class = void> struct has_dt_limit : std::false_type {};
template <class T>
struct has_dt_limit<T,
                    std::void_t<decltype(std::declval<const T &>().dt_limit())>>
    : std::true_type {};
template <class T, class = void> struct has_member_dt_limit : std::false_type {};
template <class T>
struct has_member_dt_limit<T, std::void_t<decltype(&T::dt_limit)>>
    : std::true_type {};
```

`has_ic`・`has_bc`・`has_src` も `has_eos` と同じ形です。
`ModelBase::run()` は、これらを `static_assert` で検査します（抜粋）。

```cpp
    static_assert(has_eos<Derived>::value,
                  "Derived must have member variable 'eos'.");
```

モデルに `eos` を書き忘れると、実行する前に、このメッセージでコンパイルが止まります。

`ModelBase::update()` は、ソース項が時間刻みの上限 `dt_limit()` を持つときだけ、それを使います（抜粋。MPI の部分は省略）。

```cpp
  void update() {
    auto &d = derived();
    using Source = decltype(d.src);
    static_assert(!has_member_dt_limit<Source>::value ||
                      has_dt_limit<Source>::value,
                  "Source::dt_limit must be callable as dt_limit() const");
    auto dt = mhd.cfl(d.eos);
    if constexpr (has_dt_limit<Source>::value) {
      Real dt_src = static_cast<Real>(d.src.dt_limit());
      // （中略：全ランクの最小値 dt_src_g を求めます）
      dt = std::min(dt, dt_src_g);
    }
    mhd.update(dt, d.eos, d.bc, d.src);
    time.update(dt);
  }
```

- `using Source = decltype(d.src);` は、メンバ `src` の型に `Source` という別名を付けます。
- `if constexpr` の中の `d.src.dt_limit()` は、`dt_limit` を持たないソース項では書けない式です。6.6 節で見たとおり、条件が `false` なら組み込まれないので、エラーになりません。
- `static_assert` の条件は「`dt_limit` という名前がない、**または**、`dt_limit() const` として呼べる」です（`!` は否定、`||` は「または」）。
  `dt_limit` を書いたのに `const` を付け忘れた場合、`has_dt_limit` だけで調べると `false` になり、黙って無視されてしまいます。
  名前の有無（`has_member_dt_limit`）と呼べるか（`has_dt_limit`）を分けて調べ、書き間違いをコンパイルエラーにしています。

(edu-ch06-source-term-macro)=
### 6.7.3 マクロとテンプレートの組み合わせ（ソース項）

ソース項は `ro`・`vx`・`vy`・`vz`・`bx`・`by`・`bz`・`ei` の 8 つあり、どれも省略できます。
8 つについて、上の 2 種類の型特性と、0 を補う関数を書くと、名前だけが違う定義が 8 回並びます。
`miso/include/miso/mhd_integrator.hpp` では、これをプリプロセッサのマクロ（{ref}`第 2 章 <edu-ch02>`）で 1 回だけ書いています。

```cpp
#define MISO_MHD_DEFINE_SOURCE_TERM(name)                                      \
  namespace impl_source {                                                      \
  template <class Source, class = void>                                        \
  struct has_member_##name : std::false_type {};                               \
  template <class Source>                                                      \
  struct has_member_##name<Source, std::void_t<decltype(&Source::name)>>       \
      : std::true_type {};                                                     \
  template <class Source, class Real, class = void>                            \
  struct has_##name : std::false_type {};                                      \
  template <class Source, class Real>                                          \
  struct has_##name<Source, Real,                                              \
                    std::void_t<decltype(std::declval<const Source &>().name(  \
                        std::declval<FieldsView<const Real>>(), 0, 0, 0))>>    \
      : std::true_type {};                                                     \
  }                                                                            \
  /** @brief Source term of `name` (0 if the Source does not define it) */     \
  template <typename Real, typename Source>                                    \
  __host__ __device__ inline Real source_##name(                               \
      const Source &src, const FieldsView<const Real> &qq, int i, int j,       \
      int k) {                                                                 \
    static_assert(!impl_source::has_member_##name<Source>::value ||            \
                      impl_source::has_##name<Source, Real>::value,            \
                  "Source::" #name " must be callable as " #name               \
                  "(FieldsView<const Real>, int, int, int) const");            \
    if constexpr (impl_source::has_##name<Source, Real>::value) {              \
      return src.name(qq, i, j, k);                                            \
    } else {                                                                   \
      return Real(0);                                                          \
    }                                                                          \
  }
MISO_MHD_DEFINE_SOURCE_TERM(ro)
MISO_MHD_DEFINE_SOURCE_TERM(vx)
// （vy, vz, bx, by, bz, ei も同様）
#undef MISO_MHD_DEFINE_SOURCE_TERM
```

マクロの記号を先に読みます。

| 記号 | 意味 |
|---|---|
| 行末の `\` | マクロの定義を次の行へ続けます。マクロは本来 1 行で書くものです |
| `name` | マクロの引数です。`MISO_MHD_DEFINE_SOURCE_TERM(vx)` なら、すべての `name` が `vx` に置き換わります |
| `has_##name` | `##` は、前後をつないで 1 つの名前にします。`name` が `vx` なら `has_vx` になります |
| `#name` | `#` は、引数を文字列 `"vx"` にします |
| `#undef` | 使い終わったマクロの定義を取り消します |

`"Source::" #name " must be ..."` は、`"Source::" "vx" " must be ..."` のように文字列が並んだ形になります。
C++ では、隣り合う文字列は 1 つにつながるので、メッセージは `"Source::vx must be callable as vx(...) const"` になります。

`MISO_MHD_DEFINE_SOURCE_TERM(vx)` を手で展開し、読みやすく整形すると次のとおりです（説明のために展開したもので、ソースにこの形では書かれていません）。

```cpp
namespace impl_source {
// vx という名前のメンバがあるか
template <class Source, class = void>
struct has_member_vx : std::false_type {};
template <class Source>
struct has_member_vx<Source, std::void_t<decltype(&Source::vx)>>
    : std::true_type {};
// src.vx(qq, i, j, k) と呼べるか（src は const、qq は FieldsView<const Real>）
template <class Source, class Real, class = void>
struct has_vx : std::false_type {};
template <class Source, class Real>
struct has_vx<Source, Real,
              std::void_t<decltype(std::declval<const Source &>().vx(
                  std::declval<FieldsView<const Real>>(), 0, 0, 0))>>
    : std::true_type {};
}  // namespace impl_source

/** @brief Source term of `vx` (0 if the Source does not define it) */
template <typename Real, typename Source>
__host__ __device__ inline Real source_vx(const Source &src,
                                          const FieldsView<const Real> &qq,
                                          int i, int j, int k) {
  static_assert(!impl_source::has_member_vx<Source>::value ||
                    impl_source::has_vx<Source, Real>::value,
                "Source::vx must be callable as vx(FieldsView<const Real>, int, int, int) const");
  if constexpr (impl_source::has_vx<Source, Real>::value) {
    return src.vx(qq, i, j, k);  // vx があれば、それを呼びます
  } else {
    return Real(0);              // なければ 0 です
  }
}
```

6.7.2 節の `has_member_dt_limit`・`has_dt_limit` と、`update()` の `static_assert`・`if constexpr` を、`vx` 用にまとめた形です。
`has_vx` が `Real` も受け取るのは、引数の型 `FieldsView<const Real>` を決めるためです。

時間積分（`miso/include/miso/mhd_integrator_host.hpp` の `update_sc4`）では、運動方程式の右辺に `source_vx(src, c_qq, i, j, k)` を足しています。
`<Real>` を書いていないのは、`c_qq` の型 `FieldsView<const Real>` から `Real` が推論されるからです（6.1 節）。
`src` が `vx` を持たなければ、`source_vx` は `Real(0)` を返すだけの関数になり、余計な計算は残りません。

`demo/hd2d_rayleigh_taylor/src/main.cpp` のソース項は、`vx`・`vy`・`vz`・`ei` の 4 つを定義しています（`vy` だけ抜粋）。

```cpp
struct SourceTerm {
  Real g_grav;
  // （中略）

  /// External force: y-direction
  __host__ __device__ inline Real vy(mhd::FieldsView<const Real> qq, int i, int j,
                                     int k) const noexcept {
    return -g_grav * qq.ro(i, j, k);
  }
  // （以下省略）
};
```

`vy` を `const` なしで書くと、`has_member_vy` は `true`、`has_vy` は `false` になり、コンパイルは次のメッセージで止まります。

```text
error: static assertion failed: Source::vy must be callable as vy(FieldsView<const Real>, int, int, int) const
```

<details>
<summary>コラム：C++20 の requires による書き方</summary>

C++20 では、「この式が書けるか」を `requires` 式で直接書けるようになりました。
`has_dt_limit` と同じ検査は、次のように書けます（`-std=c++20` が必要です）。

```cpp
template <class T>
concept HasDtLimit = requires(const T &t) { t.dt_limit(); };
```

`HasDtLimit<T>` は `true` か `false` になり、`if constexpr (HasDtLimit<Source>)` のように使えます。
`std::void_t` と部分特殊化を組み合わせる 6.7.2 節の方法は、C++17 までで同じことをするための工夫です。
MISO は C++17 で書かれているので、`std::void_t` の方法を使っています。
規則は C++20 規格草案 N4861 の [制約（constraints）](https://timsong-cpp.github.io/cppwp/n4861/temp.constr)と
[`requires` 式](https://timsong-cpp.github.io/cppwp/n4861/expr.prim.req)にあります。

</details>

(edu-ch06-dot-template)=
### 6.7.4 テンプレートの中の .template

`ModelBase::run()` には、次の行があります（`miso/include/miso/mhd_model_base.hpp`）。

```cpp
    if (config["io"]["continue"].template as<bool>() &&
```

6.2 節の `IdealEOS` では `config["eos"]["gm"].as<Real>()` と書いていたのに、ここでは `.` と `as` の間に `template` があります。
これは「この後の名前は関数テンプレート」とコンパイラに知らせる印です。必要になるのは、次のような場合です。

```cpp
#include <iostream>

struct Reader {
  template <class U> U get() const { return U(2.5); }  // メンバ関数テンプレート
};

template <class T>
double read_twice(const T &r) {
  // r の型 T はテンプレート引数なので、get がテンプレートかは実体化まで分かりません
  return r.template get<double>() * 2;
}

int main() {
  Reader r;
  std::cout << r.get<int>() << " " << read_twice(r) << "\n";  // 2 5
}
```

`main` の中の `r` は型が `Reader` と決まっているので、`r.get<int>()` とそのまま書けます。
`read_twice` の中の `r` は型が `T` で、`T` に何が入るかは実体化まで分かりません。
このときコンパイラは `get` をテンプレートでない名前として読み、`r.get < double > ()` を大小比較と解釈しようとしてエラーにします。
`.template` を付けると、`get<double>` をテンプレートの呼び出しとして読みます。
`typename T::value_type` のように、型の名前であることを知らせる `typename` も同じ理由で使います。

`ModelBase` の `config` は `Config &` 型で、テンプレート引数によらず型が決まっているので、この行の `.template` は省いてもコンパイルできます。
必要のない場所に付けても意味は変わりません（`miso/include/miso/mhd_resistivity.hpp` にも同じ書き方があります）。
読むときは「テンプレートの呼び出しの印」と分かれば十分で、書くべき場所はコンパイラがエラーで教えてくれます。
規則は C++17 規格草案の[名前の解決](https://timsong-cpp.github.io/cppwp/n4659/temp.res)にあります。

(edu-ch06-exercises)=
## 練習：テンプレートの指定と選択

1. 6.1 節の `max_of` について、`max_of(2, 7)` と `max_of<double>(1, 2.5)` の戻り値の型と値を答えてください。
2. $2\pi$ を表す変数テンプレート `two_pi<T>` を書き、`two_pi<float>` と `two_pi<double>` を `%.17g` で表示してください。
3. 6.4 節の `Cpu`・`Gpu`・`run`・`compute` に `struct Fpga {};` を足し、`compute(Fpga{}, 1)` を呼ぶと何が起きるかを確かめ、理由を説明してください。
4. 次の型について、`miso/include/miso/` のどのファイルのどの定義が使われるかを答えてください。
   - `Array3D<float>`
   - `Integrator<double, backend::Host>`
   - CPU だけのビルドでの `Array3D<float, backend::CUDA>`
5. 6.7.2 節の `has_gm` にならって、メンバ `size` があるかを調べる `has_size<T>` を書いてください。
   `miso/include/miso/execution.hpp` を読み込み、`has_size<miso::Range1D>::value` と `has_size<miso::Index3D>::value` を表示して、結果を `execution.hpp` の定義と照らし合わせてください。
6. `demo/hd2d_rayleigh_taylor/src/main.cpp` の `SourceTerm` を読み、`source_ro`〜`source_ei` の 8 つの関数のうち、`SourceTerm` のメンバを呼ぶものと `Real(0)` を返すものを分けてください。
   また、`ModelBase::update()` の `if constexpr (has_dt_limit<Source>::value)` の中が組み込まれるかを答えてください。

<details>
<summary>解答・解説</summary>

**1.** `max_of(2, 7)` は、2 つの引数がどちらも `int` なので `T` が `int` と推論され、`int` の 7 を返します。
`max_of<double>(1, 2.5)` は `T` を `double` と指定しているので、`1` は `1.0` に変換され、`double` の 2.5 を返します。
指定がなければ `T` が決まらずエラーになる呼び出しでも、明示すれば通常の型変換が使われます。

**2.** 例：

```cpp
#include <cstdio>

template <typename T>
inline constexpr T two_pi = static_cast<T>(2.0 * 3.14159265358979323846);

int main() { std::printf("%.17g\n%.17g\n", two_pi<float>, two_pi<double>); }
```

```text
6.2831854820251465
6.2831853071795862
```

`float` は有効数字がおよそ 7 桁なので、8 桁目から違います。
`constants.hpp` にならって `M_PI` を使い、`static_cast<T>(2.0 * M_PI)` と書いても構いません。

**3.** コンパイルエラーになります。g++ では `no matching function for call to 'run(Fpga&, int&)'` と表示されます。
`compute<Fpga>` を実体化すると `run(b, n)` の `b` が `Fpga` 型になりますが、`Fpga` を受け取る `run` が定義されていないためです。
タグ型によるオーバーロードの選択はコンパイル時に行われるので、対応する関数がなければ実行前に分かります。
MISO の CPU だけのビルドで `for_each(backend::CUDA{}, ...)` を呼んだ場合も同じです。

**4.**

- `Array3D<float>` は既定値で `Array3D<float, backend::Host>` になり、`array3d.hpp` の部分特殊化 `template <typename T> class Array3D<T, backend::Host>` が使われます。
- `Integrator<double, backend::Host>` は、`mhd_integrator_host.hpp` の `template <typename Real> struct Integrator<Real, backend::Host>` が使われます（`Real` は `double`）。
- CPU だけのビルドでは、`Array3D<T, backend::CUDA>` の部分特殊化が `#ifdef __CUDACC__` の中にあるので定義されません。
  基本のテンプレートは宣言だけなので、`Array3D<float, backend::CUDA> b(2, 2, 2);` と書くと、
  g++ では `has initializer but incomplete type`（定義のない型）というコンパイルエラーになります。

**5.** 例：

```cpp
#include <iostream>
#include <type_traits>
#include <miso/execution.hpp>

template <class T, class = void> struct has_size : std::false_type {};
template <class T>
struct has_size<T, std::void_t<decltype(&T::size)>> : std::true_type {};

int main() {
  std::cout << has_size<miso::Range1D>::value << " "
            << has_size<miso::Index3D>::value << "\n";
}
```

MISO のヘッダの場所を `-I/path/to/MISO/miso/include` で指定してコンパイルします。表示は `1 0` です。
`execution.hpp` の `Range1D` はメンバ関数 `size()` を持ち、`Index3D` は `int i0, i1, i2;` だけを持つので、結果と一致します。
`&T::size` は、メンバ変数にもメンバ関数にも使えます。

**6.** `SourceTerm` は `vx`・`vy`・`vz`・`ei` を、どれも `(mhd::FieldsView<const Real>, int, int, int) const` の形で定義しています。

| 関数 | 使われる処理 |
|---|---|
| `source_vx`, `source_vy`, `source_vz`, `source_ei` | `src.vx(qq, i, j, k)` などを呼びます |
| `source_ro`, `source_bx`, `source_by`, `source_bz` | `Real(0)` を返します |

`vx`・`vz`・`ei` は 0 を返すだけですが、定義されているので呼ばれます（定義を省いても結果は同じです）。
`SourceTerm` には `dt_limit` がないので、`has_dt_limit<SourceTerm>::value` は `false` です。
`if constexpr` の中は組み込まれず、時間刻みは CFL 条件（`mhd.cfl(d.eos)`）だけで決まります。

</details>
