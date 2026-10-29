(edu-ch07)=
# 7. 配列・ラムダと CPU での並列ループ

Fortran で配列を確保し、各格子点の値をループで計算する処理を考えます。
MISO では、この処理を次の 3 つに分けて書きます。

1. 配列のメモリを用意します。これを担当するのが `Array1D`〜`Array4D` です。
2. 計算に渡すための、軽い「配列の窓口」を取り出します。これが View（`Array3DView` など）です。
3. 各格子点で行う処理を**ラムダ式**で書き、添字の範囲と一緒に `for_each` へ渡します。

3 つに分けるのは、同じ計算を CPU でも GPU でも実行できるようにするためです。
計算する場所は、{ref}`第 6 章 <edu-ch06>`で学んだタグ型 `backend::Host`（CPU）と `backend::CUDA`（GPU）で選びます。
この章では `backend::Host` だけを使い、CPU で配列を計算して結果を確かめるところまで進みます。
GPU で同じ計算を動かす方法は {ref}`第 9 章 <edu-ch09>`で説明します。

この章の例は、MISO のヘッダだけを使います。
`miso/include` を検索先に加えれば、次のように `g++` で直接ビルドできます（`/path/to/MISO` は自分が MISO を置いた場所に置き換えます）。

```bash
g++ -std=c++17 -I/path/to/MISO/miso/include layout.cpp -o layout
./layout
```

この章で読み込むヘッダ（`array1d.hpp`, `array3d.hpp`, `execution.hpp`, `utility.hpp`）は、MPI や yaml-cpp を必要としません。
CMake から MISO を使う設定は、章末の実習で {ref}`第 1 章 <edu-ch01>`の設定を使います。

## 目次

- {ref}`1. 配列 Array1D〜Array4D <edu-ch07-arrays>`
  - {ref}`1.1 確保と添字 <edu-ch07-allocate>`
  - {ref}`1.2 メモリ上の並び <edu-ch07-layout>`
  - {ref}`1.3 View を取り出す <edu-ch07-view>`
- {ref}`2. ラムダ式 <edu-ch07-lambda>`
  - {ref}`2.1 名前のない関数 <edu-ch07-lambda-basics>`
  - {ref}`2.2 キャプチャ：[=] と [&] <edu-ch07-capture>`
  - {ref}`2.3 値を抱えた小さな構造体 <edu-ch07-lambda-struct>`
  - {ref}`2.4 メンバ変数と this <edu-ch07-this>`
  - {ref}`2.5 ラムダを関数に渡す <edu-ch07-pass-lambda>`
- {ref}`3. MISO_LAMBDA <edu-ch07-miso-lambda>`
- {ref}`4. for_each による並列ループ <edu-ch07-for-each>`
- {ref}`5. reduce による縮約 <edu-ch07-reduce>`
- {ref}`6. 小さなプログラムを書く <edu-ch07-practice>`

(edu-ch07-arrays)=
## 1. 配列 Array1D〜Array4D

(edu-ch07-allocate)=
### 1.1 確保と添字

{ref}`第 4 章 <edu-ch04>`で、`Array3D` が配列を所有するクラスであることを学びました。
MISO には次元ごとに 4 つの配列クラスがあります。

| クラス | ヘッダ | 要素の指定 |
|---|---|---|
| `Array1D<T, Backend>` | `miso/include/miso/array1d.hpp` | `a[i]` |
| `Array2D<T, Backend>` | `miso/include/miso/array2d.hpp` | `a(i, j)`、通し番号 `a[n]` |
| `Array3D<T, Backend>` | `miso/include/miso/array3d.hpp` | `a(i, j, k)`、通し番号 `a[n]` |
| `Array4D<T, Backend>` | `miso/include/miso/array4d.hpp` | `a(i, j, k, l)`、通し番号 `a[n]` |

`T` は要素の型、`Backend` は配列を置く場所です。
`Backend` を省くと `backend::Host` になります（`array3d.hpp` の先頭の宣言 `template <typename T, typename Backend = backend::Host> class Array3D;`）。
1 次元の `Array1D` だけは丸括弧を持たず、角括弧 `a[i]` で要素を指定します。

```cpp
miso::Array1D<double> x(100);           // 100 doubles: x[0] to x[99]
miso::Array3D<double> ro(16, 32, 8);    // ro(0,0,0) to ro(15,31,7)
```

添字はどの方向も 0 から始まり、最後は「大きさ − 1」です。
Fortran の `real(8) :: ro(0:15, 0:31, 0:7)` に当たります。

**確保した要素は初期化されません。**
Host 版のコンストラクタは `data_ = new T[array_size];` で確保するだけで、0 を入れません（{ref}`第 3 章 <edu-ch03>`の `new`）。
Fortran の `allocate` と同じく、値を読む前に必ず代入します。

(edu-ch07-layout)=
### 1.2 メモリ上の並び

3 次元の配列も、メモリ上では 1 列に並んでいます。
`a(i, j, k)` がその列の何番目にあるかは、`array3d.hpp` の `operator()` に書かれています。

```cpp
// excerpt from Array3D<T, backend::Host> in miso/include/miso/array3d.hpp
T &operator()(int i0, int i1, int i2) const noexcept {
  ...
  return data_[(i0 * shape_[1] + i1) * shape_[2] + i2];
}
```

`shape_` は各方向の大きさ `{nx0, nx1, nx2}` です。
大きさ `(nx, ny, nz)` の配列なら、`a(i, j, k)` は先頭から `(i * ny + j) * nz + k` 番目です。
最後の添字 `k` が 1 増えると 1 つ隣、`j` が 1 増えると `nz` 先、`i` が 1 増えると `ny * nz` 先へ進みます。
つまり **最後の添字 `k` がメモリ上で連続しています。**

Fortran は逆で、最初の添字 `i` が連続します（列優先）。
C++ の多次元配列や MISO の配列のように最後の添字が連続する並びを、行優先と呼びます。

次のプログラムで確かめます。
`a[n]` は先頭から `n` 番目の要素を直接読む書き方です。
`&a(0, 0, 1) - &a(0, 0, 0)` は、2 つの要素のアドレスの差を要素の個数で表します（{ref}`第 3 章 <edu-ch03>`のポインタ演算）。

```cpp
#include <iostream>

#include <miso/array3d.hpp>

int main() {
  miso::Array3D<double> a(2, 3, 4);  // omitting the 2nd type gives backend::Host
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 3; ++j) {
      for (int k = 0; k < 4; ++k) {
        a(i, j, k) = 100 * i + 10 * j + k;
      }
    }
  }
  // read the first 6 elements in memory by linear index a[n]
  for (int n = 0; n < 6; ++n) {
    std::cout << a[n] << " ";
  }
  std::cout << "\n";
  // how many elements in memory one index increment advances
  std::cout << "k: " << &a(0, 0, 1) - &a(0, 0, 0) << "\n";
  std::cout << "j: " << &a(0, 1, 0) - &a(0, 0, 0) << "\n";
  std::cout << "i: " << &a(1, 0, 0) - &a(0, 0, 0) << "\n";
}
```

```text
0 1 2 3 10 11
k: 1
j: 4
i: 12
```

先頭から `a(0,0,0)`, `a(0,0,1)`, `a(0,0,2)`, `a(0,0,3)`, `a(0,1,0)`, `a(0,1,1)` の順に並んでいます。

CPU は、メモリ上で近くにある値をまとめて読み込みます。
そのため、連続した要素を順に処理するループが速くなります。
Fortran では最も内側のループを `i` にしましたが、MISO の配列では **最も内側のループを `k`** にします。
MISO の `Integrator::cfl`（`miso/include/miso/mhd_integrator_host.hpp`）も、外側から `i`, `j`, `k` の順にループを重ねています。

`Array2D` の `a(i, j)` は `i * ny + j` 番目、`Array4D` の `a(i, j, k, l)` は最後の `l` が連続します。

練習 7-1：`miso::Array3D<double> a(4, 5, 6);` で、`a(1, 2, 3)` がメモリの先頭から何番目にあるかを求めてください。
また、`a(1, 2, 3)` のすぐ隣（1 つ前と 1 つ後）にある要素を答えてください。

<details>
<summary>解答・解説</summary>

大きさは `(nx, ny, nz) = (4, 5, 6)` なので、`(i * ny + j) * nz + k = (1 * 5 + 2) * 6 + 3 = 45` 番目です（0 番目から数えます）。
連続しているのは最後の添字 `k` なので、すぐ隣は `a(1, 2, 2)`（44 番目）と `a(1, 2, 4)`（46 番目）です。
`a(0, 2, 3)` や `a(2, 2, 3)` は `ny * nz = 30` 要素離れています。

</details>

(edu-ch07-view)=
### 1.3 View を取り出す

{ref}`第 4 章 <edu-ch04>`で見たとおり、`Array3D` はコピーを禁止しています（`Array3D(const Array3D &) = delete;`）。
コピーすると、同じメモリを 2 つのオブジェクトが解放しようとするためです。
一方、次節以降で学ぶ並列ループには、配列を**値で渡す**必要があります。
そこで、配列から所有しない軽い窓口である View を取り出して渡します。

```cpp
miso::Array3D<double> ro(16, 32, 8);
auto rov = ro.view();               // Array3DView<double>: read/write
auto roc = ro.const_view();         // Array3DView<const double>: read-only
```

`Array3DView` が持つのは、先頭のポインタ `data_` と大きさ `shape_[3]` だけです。
コピーしても要素は複製されず、コピーした View も同じ要素を指します。
添字の計算は `Array3D` と同じ `(i0 * shape_[1] + i1) * shape_[2] + i2` です。
View はメモリを解放しないので、**View を使っている間は元の `Array3D` を残しておく**必要があります。

`Array1D` から取り出すのは `Array1DView` です。`Array2DView`, `Array4DView` もあります。
MISO の物理量（密度、速度、磁場など 9 つ）をまとめた `mhd::FieldsView` は、`Array3DView` を 9 つ並べた構造体です（`miso/include/miso/mhd_fields.hpp`）。

View の `operator()` と `operator[]` には `const` が付いていますが、戻り値は `T &` です。
{ref}`第 3 章 <edu-ch03>`の `double* const` と同じで、View 自身（ポインタと大きさ）は変えられなくても、指す先の要素は書き換えられます。
この性質は、次節のラムダ式で使います。

(edu-ch07-lambda)=
## 2. ラムダ式

`for_each` には「各格子点で行う処理」を渡します。
処理を値のように渡すために、C++ では**ラムダ式**を使います。
MISO の並列計算はすべてラムダ式で書かれているので、この節で順を追って説明します。

(edu-ch07-lambda-basics)=
### 2.1 名前のない関数

Fortran では、サブルーチンの中だけで使う短い関数を `contains` の後に内部手続きとして書けます。

```fortran
subroutine scale_all(n, a, factor)
  integer, intent(in) :: n
  real(8), intent(inout) :: a(n)
  real(8), intent(in) :: factor
  integer :: i
  do i = 1, n
     a(i) = scaled(a(i))
  enddo
contains
  real(8) function scaled(x)
    real(8), intent(in) :: x
    scaled = factor * x   ! uses the parent procedure's factor directly
  end function scaled
end subroutine scale_all
```

C++ では、関数の中の**式の位置**に、その場で関数を書けます。これがラムダ式です。

```cpp
auto square = [](double x) { return x * x; };  // takes x and returns x*x
std::cout << square(3.0) << "\n";              // 9
```

| 部分 | 意味 |
|---|---|
| `[]` | キャプチャの指定（次節）。ここでは外の変数を使わないので空です |
| `(double x)` | 引数の並び。普通の関数と同じです |
| `{ return x * x; }` | 処理の本体。普通の関数と同じです |

戻り値の型は書いていません。`return` の式から、コンパイラが `double` と決めます。

内部手続きの `scaled` には名前がありました。ラムダ式の関数そのものには名前がありません。
上の例では、ラムダ式が作った「呼び出せるオブジェクト」を変数 `square` に入れ、`square(3.0)` で呼んでいます。
このオブジェクトの型はコンパイラが付ける名前のない型なので、変数の宣言には `auto` を使います。
変数に入れずに、関数の引数としてその場で書くこともできます。`for_each` ではこの書き方をします（{ref}`4 節 <edu-ch07-for-each>`）。

(edu-ch07-capture)=
### 2.2 キャプチャ：[=] と [&]

Fortran の内部手続き `scaled` は、親の手続きの変数 `factor` をそのまま使えました（ホスト結合）。
ラムダ式で外側の変数を使うには、`[]` の中で**どう取り込むか**を指定します。
外側の変数を取り込むことを**キャプチャ**（capture）と呼びます。

| 書き方 | 取り込み方 |
|---|---|
| `[=]` | 本体で使う外側の変数を、ラムダ式を作った時点の**値のコピー**として取り込みます |
| `[&]` | 本体で使う外側の変数を、**参照**（{ref}`第 3 章 <edu-ch03>`）として取り込みます |
| `[]` | 何も取り込みません。外側のローカル変数を使うとコンパイルエラーです |

違いは、ラムダ式を作った後で外側の変数を変えると分かります。

```cpp
#include <iostream>

int main() {
  double factor = 2.0;
  auto by_value = [=](double x) { return factor * x; };  // copy the value of factor (2.0)
  auto by_ref = [&](double x) { return factor * x; };    // reference factor itself
  factor = 10.0;                                         // change the outer variable after creation
  std::cout << by_value(3.0) << " " << by_ref(3.0) << "\n";
}
```

```text
6 30
```

`by_value` は作った時点の `2.0` を持ち続けるので `6`、`by_ref` は変更後の `factor` を読むので `30` です。
Fortran の内部手続きのホスト結合は、`[&]` に近い動きです。

`[=]` で取り込んだコピーは、本体の中では書き換えられません。

```cpp
int count = 0;
auto inc = [=]() { count += 1; };  // compile error: read-only variable 'count'
```

外側の変数そのものを更新したいときは `[&]` を使います。
ただし MISO の並列ループでは `[=]` を使います（{ref}`3 節 <edu-ch07-miso-lambda>`）。
`[=]` で配列の要素を書き換えられるのは、取り込むのが View だからです。

```cpp
miso::Array1D<double> a(3);
auto av = a.view();
auto set_one = [=](int i) { av[i] = 1.0; };  // holds a copy of av (pointer and size)
set_one(2);
std::cout << a[2] << "\n";  // 1
```

コピーした `av` は元の `av` と同じ要素を指すので、`av[i] = 1.0` は `a` の要素を書き換えます。
変えられないのは `av` 自身（ポインタと大きさ）です。{ref}`1.3 節 <edu-ch07-view>`の `const` と `T &` の関係です。

練習 7-2：次のプログラムが表示する 2 つの数を答えてください。

```cpp
#include <iostream>

int main() {
  int n = 1;
  auto f = [=]() { return n * 10; };
  auto g = [&]() { return n * 10; };
  n = 2;
  std::cout << f() << " " << g() << "\n";
}
```

<details>
<summary>解答・解説</summary>

`10 20` と表示します。
`f` は作った時点の `n`（1）のコピーを持つので `10`、`g` は `n` への参照を持ち、呼んだ時点の `n`（2）を読むので `20` です。
引数がないラムダ式も、`()` を付けて呼びます。

</details>

(edu-ch07-lambda-struct)=
### 2.3 値を抱えた小さな構造体

ラムダ式は、コンパイラが自動で作る**小さな構造体**と考えると、動きを正確に追えます。
`[=](double x) { return factor * x; }` は、次の構造体とほぼ同じ働きをします。

```cpp
#include <iostream>

// struct that behaves almost like [=](double x) { return factor * x; }
struct ScaleByValue {
  double factor;  // copy of the value at creation
  double operator()(double x) const { return factor * x; }
};

// struct that behaves almost like [&](double x) { return factor * x; }
struct ScaleByRef {
  double &factor;  // reference to the outer variable
  double operator()(double x) const { return factor * x; }
};

int main() {
  double factor = 2.0;
  ScaleByValue by_value{factor};  // initialize member factor with 2.0
  ScaleByRef by_ref{factor};      // bind member factor to the outer factor
  factor = 10.0;
  std::cout << by_value(3.0) << " " << by_ref(3.0) << "\n";  // 6 30
}
```

{ref}`第 4 章 <edu-ch04>`で学んだとおり、`operator()` を持つ構造体は `by_value(3.0)` と関数のように呼べます。
ラムダ式との対応は次のとおりです。

| ラムダ式 | 構造体 |
|---|---|
| キャプチャした変数 | メンバ変数（`[=]` なら値、`[&]` なら参照） |
| 引数と本体 | `operator()` の引数と本体 |
| ラムダ式を書いた場所 | その時点の値でメンバを初期化して、オブジェクトを作る |
| `[=]` のコピーを書き換えられない | `operator()` に `const` が付いている |

このモデルから、次のことが分かります。

- ラムダ式をコピーすると、抱えているメンバもコピーされます。`[=]` で View を抱えたラムダ式なら、ポインタと大きさがコピーされます。
- `[&]` で作ったラムダ式は、外の変数のアドレスを抱えています。外の変数が消えた後に呼ぶと、{ref}`第 3 章 <edu-ch03>`のぶら下がり参照になります。
- `[=]` は本体で**実際に使った変数だけ**を取り込みます。関数内の変数すべてをコピーするわけではありません。

GPU で計算するときは、このラムダ式のオブジェクトが GPU へコピーされます。
GPU から CPU のメモリは読めないので、`[&]` のような CPU 上のアドレスを抱えたラムダ式は GPU では使えません。
詳しくは {ref}`第 9 章 <edu-ch09>`で説明します。

(edu-ch07-this)=
### 2.4 メンバ変数と this

クラスのメンバ関数の中でラムダ式を書き、その本体でメンバ変数を使う場合を考えます。
`[=]` は「値のコピー」と説明しましたが、**メンバ変数はコピーされません。**
メンバ関数の中では、メンバ変数 `gm` は「自分自身のオブジェクトの `gm`」、つまり `this->gm` の省略形です。
`this` は、そのメンバ関数を呼んだオブジェクトを指すポインタです。
`[=]` が取り込むのは、このポインタ `this` のコピーです。

次の例で確かめます。
`Gas` は比熱比 `gm` をメンバに持ち、圧力を計算するラムダ式を作って返します。
戻り値の型を `auto` にすると、`return` の式からコンパイラが型を決めます。

```cpp
#include <iostream>

struct Gas {
  double gm;  // ratio of specific heats (member variable)

  // create and return a lambda that computes pressure
  auto pressure_function() const {
    return [=](double ro, double ei) { return (gm - 1.0) * ro * ei; };
  }

  // copy the member variable to a local variable before using it
  auto pressure_function_copy() const {
    const double gm_ = gm;
    return [=](double ro, double ei) { return (gm_ - 1.0) * ro * ei; };
  }
};

int main() {
  Gas gas{1.4};
  auto f = gas.pressure_function();
  auto g = gas.pressure_function_copy();
  std::cout << f(1.0, 2.5) << " " << g(1.0, 2.5) << "\n";
  gas.gm = 2.0;  // change the member variable after creating the lambda
  std::cout << f(1.0, 2.5) << " " << g(1.0, 2.5) << "\n";
}
```

```text
1 1
2.5 1
```

`f` は `gm` の値ではなく `this`（`gas` のアドレス）を抱えているので、`gas.gm` を変えると結果が変わります。
`g` はローカル変数 `gm_` の値 `1.4` をコピーして抱えているので、結果は変わりません。
構造体のモデルで書くと、`f` は `const Gas *self;` を、`g` は `double gm_;` をメンバに持つ構造体です。

CPU だけで計算する場合、この違いは「後で値を変えたときにどちらが見えるか」の違いです。
GPU では、`this` が CPU のメモリを指すことが問題になります。
そのため MISO では、ラムダ式の直前でメンバ変数をローカル変数にコピーしてから使います。
理由と、守らなかったときに起こることは {ref}`第 9 章 <edu-ch09>`で説明します。

MISO の実物では、理想気体の状態方程式 `eos::IdealEOS` の `gas_pressure` がこの形です。

```cpp
// excerpt from IdealEOS in miso/include/miso/eos.hpp
template <typename Real> struct IdealEOS {
  /// @brief Ratio of specific heats (gamma)
  Real gm;
  ...
  template <typename Backend>
  void gas_pressure(Backend btag, mhd::FieldsView<const Real> qq,
                    Array3DView<Real> pr) const {
    const Real gm_ = gm;  // device lambdas must not capture `this`
    Range1D range{0, qq.size()};
    for_each(
        btag, range,
        MISO_LAMBDA(int i) { pr[i] = (gm_ - Real(1)) * qq.ro[i] * qq.ei[i]; });
  }
```

コメントの「device lambdas must not capture `this`」は、「GPU で動かすラムダ式は `this` を取り込んではいけない」という意味です。
`for_each`, `Range1D`, `MISO_LAMBDA` は、この後の節で順に説明します。

練習 7-3：`gas_pressure` の `const Real gm_ = gm;` を消して、ラムダ式の中で `gm` を直接使ったとします。
このとき、ラムダ式が抱える値は何になるかを説明してください。

<details>
<summary>解答・解説</summary>

ラムダ式の中の `gm` は `this->gm` の省略形なので、`[=]` は `gm` の値ではなく、`IdealEOS` オブジェクトを指すポインタ `this` をコピーして抱えます。
ラムダ式の本体は、呼ばれるたびに `this` が指すオブジェクトから `gm` を読みます。
`pr`, `qq` は関数の引数（ローカル変数）なので、これまでどおり値（View）がコピーされます。
CPU では計算結果は変わりませんが、GPU では `this` が CPU のメモリを指すため問題になります（{ref}`第 9 章 <edu-ch09>`）。

</details>

(edu-ch07-pass-lambda)=
### 2.5 ラムダを関数に渡す

ラムダ式のオブジェクトの型には名前がないので、受け取る関数は{ref}`第 6 章 <edu-ch06>`の関数テンプレートで書きます。

```cpp
template <class F> void apply_to_all(int n, F f) {
  for (int i = 0; i < n; ++i) {
    f(i);  // call the given lambda with i
  }
}

// caller side
apply_to_all(5, [=](int i) { av[i] = 0.5 * i; });
```

`F` は、渡したラムダ式の型にコンパイラが決めます。
引数 `F f` は値渡しなので、ラムダ式のオブジェクト（抱えている View など）がコピーされて渡ります。
コンパイラは渡されたラムダ式の型ごとに `apply_to_all` を作ります。最適化を有効にしたビルドでは `f(i)` の中身がループの中に展開されるので、関数を呼ぶ手間はほとんど残りません。
次の 2 節で見る MISO の `for_each` は、ちょうどこの形をしています。

(edu-ch07-miso-lambda)=
## 3. MISO_LAMBDA

MISO では、`for_each` などへ渡すラムダ式を `[=]` と書かずに `MISO_LAMBDA` と書きます。
`MISO_LAMBDA` は `miso/include/miso/execution.hpp` で定義されたマクロ（{ref}`第 2 章 <edu-ch02>`の `#define`）です。

```cpp
// excerpt from miso/include/miso/execution.hpp
#ifdef __CUDACC__
#define MISO_LAMBDA [=] __host__ __device__
#else
#define MISO_LAMBDA [=]
#endif  // __CUDACC__
```

`__CUDACC__` は、CUDA のコンパイラ `nvcc` がコンパイルするときだけ定義される名前です。
`g++` でコンパイルする CPU ビルドでは、`MISO_LAMBDA(int i) { ... }` は `[=](int i) { ... }` に置き換わります。
つまり CPU ビルドでは、`MISO_LAMBDA` は値のキャプチャ `[=]` そのものです。

CUDA ビルドでは、`__host__ __device__` が加わります。
これは「この関数は CPU（host）でも GPU（device）でも実行できる」という CUDA の指定です（{ref}`第 9 章 <edu-ch09>`）。
CPU ビルドでは、`miso/include/miso/cuda_compat.hpp` が `__host__` と `__device__` を空の文字列として定義します。

```cpp
// excerpt from miso/include/miso/cuda_compat.hpp
#ifndef __device__
#define __device__
#endif

#ifndef __host__
#define __host__
#endif
```

このため、MISO のヘッダにある `__host__ __device__ int size() const` のような関数も、CPU ビルドでは普通の関数としてコンパイルされます。
自分でラムダ式の中から呼ぶ関数を書くときも、後で GPU で使う予定があれば `__host__ __device__` を付けておきます。
数学関数は、`std::sqrt` ではなく `miso::util::sqrt`、`util::exp`、`util::sin` などを使います（`miso/include/miso/utility.hpp`）。
`util` の関数は CPU と GPU の両方で使え、`float` と `double` の両方を受け取ります。

`MISO_LAMBDA` を使うのは、CPU と GPU で同じソースをコンパイルするためです。
`[=]` を使い、`[&]` を使わないのは、{ref}`2.3 節 <edu-ch07-lambda-struct>`で見たとおり GPU では CPU 上のアドレスを抱えたラムダ式が使えないためです。

MISO のユニットテスト `miso/tests/serial/test_execution.cpp` には、CPU 専用のテストで `[&]` を使っている箇所があります。
CPU（`backend::Host`）ではこれも動きますが、GPU と共用するコードでは `MISO_LAMBDA` を使います。

(edu-ch07-for-each)=
## 4. for_each による並列ループ

### 範囲を表す Range1D と Range3D

`for_each` に渡す添字の範囲は、`Range1D` と `Range3D` で表します（`miso/include/miso/execution.hpp`）。

```cpp
// excerpt from miso/include/miso/execution.hpp (member functions omitted)
struct Range1D {
  int begin, end;
};

struct Range3D {
  Range1D range0, range1, range2;
};
```

`Range1D` は開始 `begin` と終わり `end` を持つ構造体です。
**`begin` を含み、`end` を含みません**（{ref}`第 2 章 <edu-ch02>`の `for (int i = 0; i < n; ++i)` と同じ）。
開始を含み終わりを含まない範囲を半開区間と呼び、`[begin, end)` と書くこともあります。
`Range3D` は、3 方向の `Range1D` を並べたものです。

```cpp
miso::Range1D r1{0, n};                         // i = 0, 1, ..., n-1
miso::Range3D r3{{0, nx}, {0, ny}, {0, nz}};    // ranges in the 3 directions
```

`Range1D{0, n}` は、構造体のメンバを宣言の順に `begin = 0`, `end = n` で初期化する書き方です。
Fortran の `do i = 0, n - 1` に当たります。上限の書き方が 1 違うことに注意します。

### for_each の Host 版

`for_each` は、範囲と各点の処理を受け取って、範囲のすべての点で処理を呼びます。
Host 版の定義は、ただの `for` 文です。

```cpp
// excerpt from miso/include/miso/execution.hpp
template <class F> inline void for_each(backend::Host, Range1D range, F f) {
  for (int i = range.begin; i < range.end; ++i) {
    f(i);
  }
}

template <class F> inline void for_each(backend::Host, Range3D range, F f) {
  for (int i0 = range.range0.begin; i0 < range.range0.end; ++i0) {
    for (int i1 = range.range1.begin; i1 < range.range1.end; ++i1) {
      for (int i2 = range.range2.begin; i2 < range.range2.end; ++i2) {
        f(i0, i1, i2);
      }
    }
  }
}
```

{ref}`2.5 節 <edu-ch07-pass-lambda>`の `apply_to_all` と同じ形です。
最初の引数 `backend::Host` は{ref}`第 6 章 <edu-ch06>`のタグ型で、引数名がありません。
中身は使わず、型だけで Host 版と CUDA 版の `for_each` を選び分けます。
Host 版は CPU の 1 つのスレッドで順に実行します。
3 次元の版は、{ref}`1.2 節 <edu-ch07-layout>`のメモリの並びに合わせて、最後の添字 `i2` を最も内側で回します。

呼ぶ側は、次のように書きます。

```cpp
#include <iostream>

#include <miso/array3d.hpp>
#include <miso/execution.hpp>

using namespace miso;

int main() {
  const int nx = 2, ny = 3, nz = 4;
  Array3D<double, backend::Host> ro(nx, ny, nz);
  auto rov = ro.view();  // pass the View to the lambda

  Range3D range{{0, nx}, {0, ny}, {0, nz}};
  for_each(backend::Host{}, range, MISO_LAMBDA(int i, int j, int k) {
    rov(i, j, k) = 100 * i + 10 * j + k;
  });

  std::cout << rov(1, 2, 3) << "\n";  // 123
}
```

| 部分 | 役割 |
|---|---|
| `backend::Host{}` | 計算する場所。タグ型のオブジェクトを `{}` で作って渡します |
| `range` | 3 方向の範囲。各方向とも上限を含みません |
| `MISO_LAMBDA(int i, int j, int k) { ... }` | 各点で実行する処理。`Range3D` なら引数は 3 つ、`Range1D` なら 1 つです |

ラムダ式は `rov`（View）を値で抱えます。`ro`（`Array3D`）はコピーできないので、ラムダ式の中で直接使いません。
Fortran で書くと、次の計算に当たります。

```fortran
do i = 0, nx - 1
do j = 0, ny - 1
do k = 0, nz - 1
   ro(i,j,k) = 100*i + 10*j + k
enddo
enddo
enddo
```

### 実行順に頼らない

Host 版は添字の順に実行しますが、CUDA 版は多数のスレッドで**同時に**実行し、順番は決まっていません（{ref}`第 9 章 <edu-ch09>`）。
`for_each` に渡す処理は、各点を同時に実行しても結果が変わらないように書きます。
具体的には、次の 2 つを守ります。

- ある点の処理で書き込んだ要素を、別の点の処理で読まない。読む配列と書く配列を分けます。
- 異なる点の処理から、同じ要素へ書き込まない。

たとえば `av[i] = av[i - 1] + 1.0` は、`i - 1` の結果を `i` で読むので、Host 版では動いても GPU では結果が変わります。

### MISO の実例：gas_pressure

{ref}`2.4 節 <edu-ch07-this>`の `eos::IdealEOS::gas_pressure` を、もう一度読みます。

```cpp
// excerpt from miso/include/miso/eos.hpp
const Real gm_ = gm;  // device lambdas must not capture `this`
Range1D range{0, qq.size()};
for_each(
    btag, range,
    MISO_LAMBDA(int i) { pr[i] = (gm_ - Real(1)) * qq.ro[i] * qq.ei[i]; });
```

- `btag` は関数の引数で、`backend::Host{}` か `backend::CUDA{}` です。`gas_pressure` は `Backend` を型引数に持つ関数テンプレートなので、同じコードが CPU と GPU の両方で使われます。
- `qq` は 9 つの物理量の View をまとめた `mhd::FieldsView`、`pr` は圧力を書く `Array3DView` です。
- 3 次元の配列ですが、`Range1D{0, qq.size()}` で通し番号 `i` の 1 次元ループにしています。各点の圧力はその点の密度 `ro` と内部エネルギー `ei` だけで決まるので、3 つの添字に分ける必要がありません。範囲はゴーストセル（{ref}`第 8 章 <edu-ch08>`）を含む全要素です。
- `Real(1)` は、型 `Real` の 1 です。`Real` が `float` のときに `double` の定数を混ぜないための書き方です。

各点の処理は `pr[i]` に書き、`qq.ro[i]`, `qq.ei[i]` を読むだけなので、どの順で実行しても結果は同じです。

練習 7-4：次の Fortran のループを、`Array1D<double>` の `a` と `b`（大きさ `n`）、`for_each`、`MISO_LAMBDA` で書いてください。
配列は `a(0:n-1)`, `b(0:n-1)` と宣言されているとします。

```fortran
do i = 1, n - 2
   b(i) = 0.5d0 * (a(i-1) + a(i+1))
enddo
```

また、この処理を `b` ではなく `a(i)` へ書き込むように変えると、`for_each` では正しく計算できない理由を説明してください。

<details>
<summary>解答・解説</summary>

```cpp
auto av = a.view();
auto bv = b.view();
for_each(backend::Host{}, Range1D{1, n - 1}, MISO_LAMBDA(int i) {
  bv[i] = 0.5 * (av[i - 1] + av[i + 1]);
});
```

Fortran の `do i = 1, n - 2` は `n - 2` を含むので、`Range1D` の上限は 1 大きい `n - 1` です。
ラムダ式に渡すのは View の `av`, `bv` です。
`a(i) = 0.5 * (a(i-1) + a(i+1))` のように同じ配列へ書くと、点 `i` の処理が `a(i-1)` を読むときに、点 `i-1` の処理がすでに書き換えているかどうかが実行順で変わります。
Host 版では順に実行するので Fortran と同じ結果になりますが、GPU のように同時に実行すると結果が変わります。
更新前の値を読む配列と、結果を書く配列を分けます。

</details>

(edu-ch07-reduce)=
## 5. reduce による縮約

### 縮約とは

配列の全要素から、総和・最大値・最小値のような 1 つの値を求める操作を**縮約**（reduction）と呼びます。
Fortran の `sum`, `maxval`, `minval` に当たります。
MISO では `reduce` を使います（`miso/include/miso/execution.hpp`）。

```cpp
// excerpt from miso/include/miso/execution.hpp
template <class T, class F, class Op>
inline T reduce(backend::Host, Range1D range, T init, F map, Op op) {
  T acc = init;
  for (int i = range.begin; i < range.end; ++i)
    acc = op(acc, map(i));
  return acc;
}
```

| 引数 | 役割 |
|---|---|
| `backend::Host` | 計算する場所 |
| `range` | 範囲。`Range1D` と `Range3D` の版があります |
| `init` | 初期値。結果の型 `T` もこの引数から決まります |
| `map` | 各点の値を返すラムダ式。`Range1D` なら `map(i)`、`Range3D` なら `map(i, j, k)` |
| `op` | 2 つの値を 1 つにまとめるラムダ式。和なら `a + b`、最小値なら小さい方を返します |

`for_each` と違い、`map` は配列へ書き込まず、その点の値を `return` で返します。
Host 版は、`acc = op(acc, map(i))` を順に繰り返すだけです。
結果は `reduce` の戻り値として受け取ります。

```cpp
#include <iostream>

#include <miso/array1d.hpp>
#include <miso/execution.hpp>

using namespace miso;

int main() {
  const int n = 5;
  Array1D<double, backend::Host> a(n);
  auto av = a.view();
  for_each(backend::Host{}, Range1D{0, n}, MISO_LAMBDA(int i) {
    av[i] = 0.5 * i;  // 0, 0.5, 1, 1.5, 2
  });

  const auto value = MISO_LAMBDA(int i) { return av[i]; };              // value at each point
  const auto plus = MISO_LAMBDA(double s, double t) { return s + t; };  // combine two values
  const double total = reduce(backend::Host{}, Range1D{0, n}, 0.0, value, plus);
  std::cout << total << "\n";  // 5

  const double wrong = reduce(backend::Host{}, Range1D{0, n}, 100.0, value, plus);
  std::cout << wrong << "\n";  // 105 (the initial value is also added)
}
```

ラムダ式を `value`, `plus` という変数に入れてから渡しています。
`reduce` の呼び出しの中に直接書いてもかまいません。

### 初期値は単位元にする

上の例の 2 番目の呼び出しでは、初期値 `100.0` が結果に足されて `105` になりました。
Host 版は初期値を 1 回だけ使いますが、CUDA 版は範囲の外のスレッドの値として初期値を何回も使います（`execution.hpp` の `reduce_stage_kernel`）。
そのため、初期値は**まとめても値を変えないもの**（単位元）にします。

| 求めるもの | `op` | 初期値 |
|---|---|---|
| 総和 | `a + b` | `0.0` |
| 最小値 | 小さい方 | 十分大きな値（`std::numeric_limits<double>::max()` など） |
| 最大値 | 大きい方 | 十分小さな値（`std::numeric_limits<double>::lowest()` など） |

`std::numeric_limits` は `<limits>` で使える、型の最大値・最小値などを返す標準ライブラリの機能です。
`lowest()` は最も小さい（負で絶対値が最大の）値、`max()` は最も大きい値です。

結果の型 `T` は初期値の型で決まります。
総和の初期値を `0.0` でなく `0` と書くと `T` は `int` になり、途中の和が毎回整数に切り捨てられます。
上の配列で試すと、`5` ではなく `4` が返ります。浮動小数点数の縮約では、初期値を `0.0` のように書きます。

最大値・最小値をまとめる関数として、`utility.hpp` には `util::max2`, `util::min2` があります。
2026 年 10 月の MISO の main では、これらは `double` を受け取っても `float` に丸めて比べます（MISO の issue #189）。
`double` の配列では、`util::fmax`, `util::fmin` を使うか、`a < b ? b : a` のように自分で書きます。

### 和は足す順序で変わる

浮動小数点数の和は、足す順序によって末尾の桁が変わります。
Host 版は添字の順に足しますが、CUDA 版はスレッドのまとまりごとに部分和を作ってから足し合わせます。
そのため、CPU と GPU で総和のビット列が一致するとは限りません。
結果を比較するときは、完全一致を求めるのか、許容誤差を使うのかを区別します。
最大値・最小値は比べて選ぶだけなので、配列が同じなら一致します。

### MISO の実例：Integrator::cfl

時間刻み `dt` は、CFL 条件から全格子点の最小値として求めます。
CUDA 版の `Integrator::cfl` は `reduce` を使っています。
GPU がなくても、`reduce` の使い方として読めます。

```cpp
// excerpt from Integrator::cfl in miso/include/miso/mhd_integrator_cuda.cuh
auto qq_v = qq.const_view();
auto grid_v = grid.const_view();
auto cs_v = cs.view();
// device lambdas must not capture `this`
const Real cfl_number_ = cfl_number;

eos.sound_speed(backend::CUDA{}, qq_v, cs_v);
Range3D range{{grid.i_margin, grid.i_total - grid.i_margin},
              {grid.j_margin, grid.j_total - grid.j_margin},
              {grid.k_margin, grid.k_total - grid.k_margin}};
const auto f = MISO_LAMBDA(int i, int j, int k) {
  Real vv = util::sqrt(qq_v.vx(i, j, k) * qq_v.vx(i, j, k) +
                       qq_v.vy(i, j, k) * qq_v.vy(i, j, k) +
                       qq_v.vz(i, j, k) * qq_v.vz(i, j, k));
  Real ca = util::sqrt((qq_v.bx(i, j, k) * qq_v.bx(i, j, k) +
                        qq_v.by(i, j, k) * qq_v.by(i, j, k) +
                        qq_v.bz(i, j, k) * qq_v.bz(i, j, k)) /
                       qq_v.ro(i, j, k) * pii4<Real>);
  Real total_vel = cs_v(i, j, k) + vv + ca;
  Real dxyz = util::min3(grid_v.dx[i], grid_v.dy[j], grid_v.dz[k]);
  return cfl_number_ * dxyz / total_vel;
};
const auto op = MISO_LAMBDA(Real a, Real b) { return util::min2(a, b); };
const Real dt_max = 1.e10;
const auto dt = reduce(backend::CUDA{}, range, dt_max, f, op, cfl_helper);
```

- ラムダ式の前で、物理量・格子・音速の View（`qq_v`, `grid_v`, `cs_v`）と、メンバ変数 `cfl_number` のコピー `cfl_number_` を用意しています（{ref}`2.4 節 <edu-ch07-this>`）。
- 範囲は `Range3D` で、ゴーストセルを除く内側の格子点です（`i_margin` は {ref}`第 8 章 <edu-ch08>`で説明します）。
- `f` は各点の「その点で許される時間刻み」を返し、`op` は小さい方を返します。初期値 `1.e10` は、実際の時間刻みより十分大きな値として単位元の代わりに使っています。
- 最後の `cfl_helper` は、CUDA 版だけが使う作業用の領域です（{ref}`第 9 章 <edu-ch09>`）。
- `pii4<Real>` は{ref}`第 6 章 <edu-ch06>`の変数テンプレートです。

Host 版の `Integrator::cfl`（`miso/include/miso/mhd_integrator_host.hpp`）は、`reduce` を使わず、同じ計算を `i`, `j`, `k` の 3 重の `for` 文と `util::min2` で書いています。
どちらも、最後に `MPI_Allreduce` で全プロセスの最小値をとります（{ref}`第 8 章 <edu-ch08>`）。

練習 7-5：`Array1D<double>` の配列 `a`（大きさ `n`）の最小値を、`reduce` で求めるコードを書いてください。
初期値に何を選ぶかと、その理由も説明してください。

<details>
<summary>解答・解説</summary>

`std::numeric_limits` を使うので、`<limits>` を読み込んでおきます。

```cpp
auto av = a.view();
const double a_min = reduce(
    backend::Host{}, Range1D{0, n}, std::numeric_limits<double>::max(),
    MISO_LAMBDA(int i) { return av[i]; },
    MISO_LAMBDA(double s, double t) { return s < t ? s : t; });
```

初期値は、どの要素と比べても選ばれない値にします。`double` で最も大きな値 `std::numeric_limits<double>::max()` なら、配列の値と比べたときに必ず配列の値が残ります。
`0.0` を初期値にすると、配列の値がすべて正のときに結果が `0` になってしまいます。
`util::min2` は `double` を `float` に丸めるので（MISO #189）、ここでは条件演算子で小さい方を選んでいます。

</details>

(edu-ch07-practice)=
## 6. 小さなプログラムを書く

{ref}`配列と並列ループの実習 <edu-practice-execution>`で、ベクトル加算、縮約、1 次元移流の順に取り組みます。
CMake の設定は、{ref}`第 1 章 <edu-ch01>`の「自分のプログラムから MISO を使う最小の CMake 設定」を使います。
この段階では `backend::Host` で動かします。
GPU で同じ課題を動かすのは、{ref}`第 9 章 <edu-ch09>`を読んだ後です。

CPU で配列計算を確かめたら、{ref}`第 8 章 <edu-ch08>`で複数のプロセスによる領域分割へ進みます。
時間発展を先に書いてみたい場合は、{ref}`第 10 章 <edu-ch10>`の逐次計算にも取り組めます。

<details>
<summary>コラム：MISO は OpenMP を使っていない</summary>

Fortran のコードでは、`!$omp parallel do` で CPU の複数のスレッドにループを分ける OpenMP をよく使います。
MISO の `for_each` の Host 版は、{ref}`4 節 <edu-ch07-for-each>`で見たとおりただの `for` 文で、OpenMP を使っていません。
MISO のソース（`miso/`, `demo/`）には `#pragma omp` がなく、CMake も OpenMP を探しません。

MISO の並列化は、CPU では MPI による領域分割（{ref}`第 8 章 <edu-ch08>`）、GPU では CUDA（{ref}`第 9 章 <edu-ch09>`）が担います。
CPU で複数のコアを使うときは、コアの数だけ MPI のプロセスを起動します。

それでも Host 版を `for_each` で書いておく利点は、同じソースのまま CUDA 版へ切り替えられることです。
Host 版に OpenMP を加える場合も、変えるのは `execution.hpp` の `for_each` と `reduce` の中身だけで、呼び出す側は変わりません。
これは MISO の設計からの推測で、MISO の文書に書かれた方針ではありません。

</details>
