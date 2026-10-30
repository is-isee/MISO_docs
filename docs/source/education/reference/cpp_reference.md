(edu-ref-cpp)=
# 記号の早見表

C++ の第 2〜6 章と、第 7〜9 章で MISO のコードを読みながら、記号やキーワードの意味を確かめるための表です。
通読する必要はありません。各行の「詳しい説明」から本文の章へ戻れます。

## 目次

- {ref}`読み方の基本：宣言か式か <edu-ref-cpp-basics>`
- {ref}`* と & と && <edu-ref-cpp-star-amp>`
- {ref}`名前とメンバを指す記号（:: . ->） <edu-ref-cpp-scope>`
- {ref}`括弧（山括弧・角括弧・丸括弧・波括弧） <edu-ref-cpp-brackets>`
- {ref}`その他の演算子 <edu-ref-cpp-operators>`
- {ref}`プリプロセッサの記号（#） <edu-ref-cpp-preprocessor>`
- {ref}`キーワード <edu-ref-cpp-keywords>`
- {ref}`MISO と CUDA の記号 <edu-ref-cpp-miso>`
- {ref}`宣言を読むときの確認順序 <edu-ref-cpp-reading>`

(edu-ref-cpp-basics)=
## 読み方の基本：宣言か式か

C++ では、同じ記号が**宣言**（新しい名前を作る文）と**式**（すでにある名前を使う計算）で別の意味になります。
記号に迷ったら、まずその行が名前を作っているのか、使っているのかを確かめます。

```cpp
double a = 1.0;
double *p = &a;  // * in a declaration: p is a "pointer to double". & in an expression: address of a
*p = 2.0;        // * in an expression: write 2.0 to what p points to (a)
double &r = a;   // & in a declaration: r is an alias (reference) of a
```

以下の表では、`a`・`b` を `double` の変数、`p` を `double *` 型のポインタ、`T` をテンプレート引数の型として読みます。

(edu-ref-cpp-star-amp)=
## * と & と &&

| 見かけた形 | 宣言か式か | 意味 | 詳しい説明 |
|---|---|---|---|
| `double *p = &a;` | 宣言 | `p` は `double` を指すポインタです。`double* p` と書いても同じです | {ref}`第 3 章 <edu-ch03>` |
| `*p`、`*p = 5.0;` | 式 | `p` の指す先の値を読み書きします | {ref}`第 3 章 <edu-ch03>` |
| `a * b` | 式 | 掛け算です | {ref}`第 2 章 <edu-ch02>` |
| `double &r = a;` | 宣言 | `r` は `a` の別名（参照）です。`r` への代入は `a` を変えます | {ref}`第 3 章 <edu-ch03>` |
| `void f(double &x)` | 宣言（引数） | 参照渡しです。関数の中で `x` を変えると、呼び出し元の変数が変わります | {ref}`第 3 章 <edu-ch03>` |
| `const Config &config` | 宣言（引数） | 読むだけの参照渡しです。コピーせず、変更もしません | {ref}`第 3 章 <edu-ch03>` |
| `&a` | 式 | `a` のアドレスを取り出します | {ref}`第 3 章 <edu-ch03>` |
| `&T::gm` | 式 | クラス `T` のメンバ `gm` を指す式です。MISO では「`gm` があるか」の検査に使います | {ref}`第 6 章 <edu-ch06>` |
| `x && y` | 式 | 論理積（かつ）です。Fortran の `.and.` です | {ref}`第 2 章 <edu-ch02>` |
| `Array3D(Array3D &&other)` | 宣言（引数） | 右辺値参照です。ムーブ（中身の移し替え）を受け取るときに使います | {ref}`第 4 章 <edu-ch04>` |
| `const double *p` | 宣言 | `*p` を通した変更を禁止します。`p` の指す先は切り替えられます | {ref}`第 3 章 <edu-ch03>` |
| `double *const p` | 宣言 | `p` の指す先の切り替えを禁止します。`*p` を通した変更はできます | {ref}`第 3 章 <edu-ch03>` |
| `T *data_` | 宣言（メンバ） | `Array3DView` などが持つ、配列の先頭を指すポインタです | {ref}`第 4 章 <edu-ch04>` |
| `char **argv` | 宣言（引数） | ポインタを指すポインタです。`main` のコマンドライン引数で見かけます | {ref}`第 3 章 <edu-ch03>` |

MISO のソースは `double *p` のように `*` と `&` を名前の側に付けて書きます（`.clang-format` の設定）。
`double* p` と `double *p` は同じ意味です。

(edu-ref-cpp-scope)=
## 名前とメンバを指す記号（:: . ->）

| 見かけた形 | 意味 | 詳しい説明 |
|---|---|---|
| `std::cout`、`miso::mhd::MHD` | 名前空間の中の名前です。`miso::mhd::` は「`miso` の中の `mhd` の中」です | {ref}`第 2 章 <edu-ch02>` |
| `backend::Host` | 名前空間 `miso::backend` の中の型です。`using namespace miso;` の後では `miso::` を省けます | {ref}`第 2 章 <edu-ch02>`、{ref}`第 6 章 <edu-ch06>` |
| `::sinf(x)` | 先頭の `::` は、名前空間の外（一番外側）の名前を指します | {ref}`第 2 章 <edu-ch02>` |
| `Shape::ndims`、`std::true_type::value` | クラスに属する名前（`static` なメンバ）です。実体を作らずに使えます | {ref}`第 4 章 <edu-ch04>`、{ref}`第 6 章 <edu-ch06>` |
| `T::gm` | テンプレート引数 `T` のメンバです | {ref}`第 6 章 <edu-ch06>` |
| `grid.i_total`、`qq.ro(i, j, k)` | 実体（または参照）のメンバを使います | {ref}`第 4 章 <edu-ch04>` |
| `p->x` | ポインタ `p` の指す先のメンバを使います。`(*p).x` と同じです | {ref}`第 4 章 <edu-ch04>` |
| `this->config` | メンバ関数の中で、自分自身を指すポインタ `this` からメンバを使います。ふつうは `this->` を省きます | {ref}`第 4 章 <edu-ch04>` |

(edu-ref-cpp-brackets)=
## 括弧（<> [] () {}）

| 見かけた形 | 意味 | 詳しい説明 |
|---|---|---|
| `#include <vector>` | 標準ライブラリなど、決まった場所にあるヘッダを読み込みます | {ref}`第 1 章 <edu-ch01>`、{ref}`第 2 章 <edu-ch02>` |
| `#include "array3d.hpp"` | 自分のファイルからの相対位置でヘッダを探して読み込みます | {ref}`第 2 章 <edu-ch02>` |
| `a < b`、`n << 1`、`std::cout << x` | 比較、ビットのずらし、出力です | {ref}`第 2 章 <edu-ch02>` |
| `std::vector<double>`、`Array3D<Real, Backend>` | テンプレート引数です。山括弧まで含めて 1 つの型の名前になります | {ref}`第 6 章 <edu-ch06>` |
| `template <typename Real>` | 後から決める型の名前を宣言します | {ref}`第 6 章 <edu-ch06>` |
| `template <>` | 完全な特殊化です（すべてのテンプレート引数を決めた定義） | {ref}`第 6 章 <edu-ch06>` |
| `pi<Real>`、`mpi::data_type<Real>()` | 変数テンプレート・関数テンプレートに型を指定して使います | {ref}`第 6 章 <edu-ch06>` |
| `static_cast<Real>(x)` | `x` を `Real` 型に変換します | {ref}`第 2 章 <edu-ch02>` |
| `kernel<<<grid, block>>>(...)` | CUDA のカーネルの起動です。C++ の記号ではありません | {ref}`第 9 章 <edu-ch09>` |
| `x[i]`、`grid.x[i]` | 配列や `std::vector` の要素です。**0 始まり**です | {ref}`第 2 章 <edu-ch02>`、{ref}`第 7 章 <edu-ch07>` |
| `double x[3];`、`int shape_[3]` | 宣言の `[]` は、要素数の決まった配列を作ります | {ref}`第 3 章 <edu-ch03>` |
| `new T[n]`、`delete[] data_` | 配列の確保と解放です | {ref}`第 3 章 <edu-ch03>`、{ref}`第 4 章 <edu-ch04>` |
| `T &operator[](int idx)` | `a[idx]` と書けるようにする演算子の定義です | {ref}`第 4 章 <edu-ch04>` |
| `[=]`、`[&]`、`MISO_LAMBDA(int i) { ... }` | ラムダ式（名前のない関数）の始まりです。`[=]` は値のコピー、`[&]` は参照で外の変数を取り込みます | {ref}`第 7 章 <edu-ch07>` |
| `f(x)`、`square<double>(1.5)` | 関数の呼び出しです | {ref}`第 2 章 <edu-ch02>` |
| `double f(double x);` | 宣言の `()` は引数の並びです | {ref}`第 2 章 <edu-ch02>` |
| `qq.ro(i, j, k)` | `operator()` による、配列の要素の読み書きです。Fortran の `ro(i, j, k)` に似せた書き方です | {ref}`第 4 章 <edu-ch04>` |
| `T &operator()(int i0, int i1, int i2)` | `a(i, j, k)` と書けるようにする演算子の定義です | {ref}`第 4 章 <edu-ch04>` |
| `Array3D<Real> pr(nx, ny, nz);` | 宣言の後の `()` は、コンストラクタに渡す値です | {ref}`第 4 章 <edu-ch04>` |
| `(a + b) * c` | 計算の順序を決める括弧です | {ref}`第 2 章 <edu-ch02>` |
| `if (...) { ... }`、`for (...) { ... }` | 処理のまとまり（ブロック）です。中で宣言した変数は `}` で寿命が終わります | {ref}`第 2 章 <edu-ch02>`、{ref}`第 3 章 <edu-ch03>` |
| `struct Host {};` | クラスの本体です。`{}` は中身が空であることを表します | {ref}`第 4 章 <edu-ch04>`、{ref}`第 6 章 <edu-ch06>` |
| `int n{3};`、`Range1D range{0, n};` | 波括弧による初期化です | {ref}`第 2 章 <edu-ch02>`、{ref}`第 4 章 <edu-ch04>` |
| `backend::Host{}`、`Backend{}` | 型の値を初期値なしで作る式です。タグ型を関数に渡すときに使います | {ref}`第 6 章 <edu-ch06>` |
| `: grid(grid), qq(grid)` | コンストラクタの初期化子リストです（`)` と `{` の間に書きます） | {ref}`第 4 章 <edu-ch04>` |

`<...>` と `(...)` の区別が大切です。`Array3D<float> a(4, 4, 4);` の `float` は型の指定、`4, 4, 4` はコンストラクタへ渡す値です。

(edu-ref-cpp-operators)=
## その他の演算子

| 見かけた形 | 意味 | 詳しい説明 |
|---|---|---|
| `a = b` | 代入です。比較は `==` です | {ref}`第 2 章 <edu-ch02>` |
| `a == b`、`a != b` | 等しい・等しくない（Fortran の `==`・`/=`） | {ref}`第 2 章 <edu-ch02>` |
| `x \|\| y`、`!x` | 論理和（または）、否定（Fortran の `.or.`、`.not.`） | {ref}`第 2 章 <edu-ch02>` |
| `i % n` | 整数の割り算の余り（Fortran の `mod(i, n)`） | {ref}`第 2 章 <edu-ch02>` |
| `++i`、`i++` | `i` を 1 増やします | {ref}`第 2 章 <edu-ch02>` |
| `a += b` | `a = a + b` と同じです。`-=`、`*=`、`/=` もあります | {ref}`第 2 章 <edu-ch02>` |
| `c ? a : b` | `c` が真なら `a`、偽なら `b` です | {ref}`第 2 章 <edu-ch02>` |
| `~Array3D()` | デストラクタ（後片付けの関数）の名前です | {ref}`第 4 章 <edu-ch04>` |
| `= default`、`= delete` | 関数の本体の代わりに書く指定です。コンパイラに用意させる・使えなくする | {ref}`第 4 章 <edu-ch04>` |
| `: public ModelBase<...>` | クラスの宣言での `:` は継承です | {ref}`第 5 章 <edu-ch05>` |

(edu-ref-cpp-preprocessor)=
## プリプロセッサの記号（#）

`#` で始まる行は、コンパイルの前にプリプロセッサが処理します。C++ の文法とは別の、テキストの置き換えです。

| 見かけた形 | 意味 | 詳しい説明 |
|---|---|---|
| `#include` | ファイルの中身をその場所に読み込みます | {ref}`第 1 章 <edu-ch01>`、{ref}`第 2 章 <edu-ch02>` |
| `#pragma once` | 同じヘッダを 2 回読み込まないようにします | {ref}`第 2 章 <edu-ch02>` |
| `#define NAME 値` | `NAME` を後ろの文字列に置き換えるマクロを定義します | {ref}`第 2 章 <edu-ch02>` |
| `#ifdef USE_CUDA` 〜 `#else` 〜 `#endif` | マクロが定義されているかで、コンパイルする部分を選びます。Fortran の `#ifdef` と同じです | {ref}`第 2 章 <edu-ch02>` |
| `#ifdef __CUDACC__` | CUDA のコンパイラ（nvcc）でビルドしているときだけ有効な部分です | {ref}`第 6 章 <edu-ch06>`、{ref}`第 9 章 <edu-ch09>` |
| `#undef NAME` | マクロの定義を取り消します | {ref}`第 6 章 <edu-ch06>` |
| 行末の `\` | マクロの定義を次の行へ続けます | {ref}`第 6 章 <edu-ch06>` |
| マクロの中の `##` | 前後をつないで 1 つの名前にします（`has_##name` → `has_vx`） | {ref}`第 6 章 <edu-ch06>` |
| マクロの中の `#name` | 引数を文字列にします（`#name` → `"vx"`） | {ref}`第 6 章 <edu-ch06>` |

(edu-ref-cpp-keywords)=
## キーワード

宣言の中の位置によって意味が変わるものを中心にまとめます。

| キーワード | 見かけた形と意味 | 詳しい説明 |
|---|---|---|
| `const` | `const Real gm_ = gm;`：値を変えられない変数です | {ref}`第 2 章 <edu-ch02>` |
| | `const T &x`：読むだけの参照です | {ref}`第 3 章 <edu-ch03>` |
| | `const T *p` と `T *const p`：`*` の左なら指す先、右ならポインタ自身を固定します | {ref}`第 3 章 <edu-ch03>` |
| | `int size() const`：メンバ変数を変えないメンバ関数です | {ref}`第 4 章 <edu-ch04>` |
| | `Array3DView<const Real>`：要素を読むだけの View です | {ref}`第 4 章 <edu-ch04>` |
| `constexpr` | `constexpr int n_fields = 9;`：コンパイル時に値が決まる定数です | {ref}`第 2 章 <edu-ch02>` |
| `auto` | `auto dt = mhd.cfl(d.eos);`：初期値から型を決めます。`auto &d = derived();` は参照になります | {ref}`第 2 章 <edu-ch02>`、{ref}`第 3 章 <edu-ch03>` |
| `using` | `using Real = float;`：型の別名です | {ref}`第 2 章 <edu-ch02>` |
| | `using namespace miso;`：名前空間の名前を `miso::` なしで使えるようにします | {ref}`第 2 章 <edu-ch02>` |
| | `using Source = decltype(d.src);`：テンプレートの中で、求めた型に別名を付けます | {ref}`第 6 章 <edu-ch06>` |
| `template` | `template <typename T> ...`：テンプレートの宣言です | {ref}`第 6 章 <edu-ch06>` |
| | `template <> ...`：完全な特殊化です | {ref}`第 6 章 <edu-ch06>` |
| | `x.template as<bool>()`：この後の名前がテンプレートであることを示す印です | {ref}`第 6 章 <edu-ch06>` |
| `typename` | `template <typename T>`：`T` が型の名前であることを示します。`class` と書いても同じです | {ref}`第 6 章 <edu-ch06>` |
| | `typename T::value_type`：テンプレート引数に属する名前が型であることを示します | {ref}`第 6 章 <edu-ch06>` |
| `inline` | ヘッダに定義を置く通常の関数や変数に付けます。速くする指定ではありません | {ref}`第 2 章 <edu-ch02>`、{ref}`第 6 章 <edu-ch06>` |
| `static` | クラスの中では、実体によらずクラスに属するメンバです | {ref}`第 4 章 <edu-ch04>` |
| `explicit` | コンストラクタによる暗黙の型変換を禁止します | {ref}`第 4 章 <edu-ch04>` |
| `noexcept` | 関数が例外を外へ出さないことを表します | {ref}`第 4 章 <edu-ch04>` |
| `virtual` | 実行時に派生クラスの関数を選ぶ仮想関数です。MISO では使っていません | {ref}`第 5 章 <edu-ch05>` |
| `decltype(式)` | 式を実行せずに、その型を求めます | {ref}`第 6 章 <edu-ch06>` |
| `if constexpr` | 条件がコンパイル時に決まる分岐です。選ばれない側は組み込まれません | {ref}`第 6 章 <edu-ch06>` |
| `static_assert` | 条件が偽ならコンパイルエラーにします | {ref}`第 6 章 <edu-ch06>` |
| `nullptr` | どこも指していないポインタです | {ref}`第 3 章 <edu-ch03>` |
| `this` | メンバ関数の中で、自分自身を指すポインタです。ラムダの中で使うと GPU で問題になります | {ref}`第 4 章 <edu-ch04>`、{ref}`第 7 章 <edu-ch07>`、{ref}`第 9 章 <edu-ch09>` |
| `struct`、`class` | クラスの宣言です。`struct` は既定で `public`、`class` は既定で `private` です | {ref}`第 4 章 <edu-ch04>` |

(edu-ref-cpp-miso)=
## MISO と CUDA の記号

C++ のキーワードではなく、MISO や CUDA が用意する名前です。

| 見かけた形 | 意味 | 詳しい説明 |
|---|---|---|
| `Real` | demo で `using Real = float;` と決める浮動小数点の型です。ライブラリ側ではテンプレート引数です | {ref}`第 2 章 <edu-ch02>`、{ref}`第 6 章 <edu-ch06>` |
| `Backend`、`backend::Host`、`backend::CUDA` | 計算とメモリの場所を表すタグ型です（`miso/include/miso/backend.hpp`） | {ref}`第 6 章 <edu-ch06>` |
| `MISO_LAMBDA` | ラムダ式の始まりを表すマクロです。CPU のビルドでは `[=]`、CUDA のビルドでは `[=] __host__ __device__` になります（`miso/include/miso/execution.hpp`） | {ref}`第 7 章 <edu-ch07>`、{ref}`第 9 章 <edu-ch09>` |
| `__host__`、`__device__` | 関数を CPU・GPU のどちらから呼べるかの指定です。CPU だけのビルドでは空になります（`miso/include/miso/cuda_compat.hpp`） | {ref}`第 9 章 <edu-ch09>` |
| `__global__` | GPU で動くカーネル関数の指定です | {ref}`第 9 章 <edu-ch09>` |
| `MPI_Comm`、`MPI_Allreduce` など | MPI のライブラリの型と関数です | {ref}`第 8 章 <edu-ch08>` |

(edu-ref-cpp-reading)=
## 宣言を読むときの確認順序

長い宣言は、次の順に分けて読みます。

1. **名前を見つけます。** 変数・関数・型のどれを宣言しているかを確かめます。
2. **型を読みます。** `<...>` まで含めて 1 つの型です。`template <...>` が前にあれば、山括弧の中の名前は後から決まる型です。
3. **値・参照・ポインタを区別します。** 名前に `&` や `*` が付いているかを見ます。
4. **`const` の対象を確かめます。** 値、指す先、ポインタ自身、メンバ関数のどれかを区別します。
5. **作り方と寿命を確かめます。** 初期値、引数、戻り値がどの実体を使い、いつまで使えるかを追います。

例として、`miso/include/miso/mhd_integrator.hpp` の宣言を分けて読みます。

```cpp
template <typename Real, typename Source>
__host__ __device__ inline Real source_vx(const Source &src,
                                          const FieldsView<const Real> &qq,
                                          int i, int j, int k);
```

| 部分 | 読み方 |
|---|---|
| `template <typename Real, typename Source>` | `Real` と `Source` は後から決まる型です |
| `__host__ __device__` | CPU からも GPU からも呼べる関数です |
| `inline` | ヘッダに置く関数です |
| `Real source_vx(...)` | 名前は `source_vx`、戻り値の型は `Real` です |
| `const Source &src` | ソース項の実体を、読むだけの参照で受け取ります |
| `const FieldsView<const Real> &qq` | 要素を読むだけの View（`FieldsView<const Real>`）を、読むだけの参照で受け取ります |
| `int i, int j, int k` | 格子点の添字を値で受け取ります |

文法の詳細は、C++17 規格草案 N4659 の
[参照の宣言](https://timsong-cpp.github.io/cppwp/n4659/dcl.ref)、
[`const` の規則](https://timsong-cpp.github.io/cppwp/n4659/dcl.type.cv)、
[テンプレート](https://timsong-cpp.github.io/cppwp/n4659/temp)で確認できます。
