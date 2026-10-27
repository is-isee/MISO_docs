(edu-ch03)=
# 3. 参照・ポインタと寿命

{ref}`第 2 章 <edu-ch02>` では、値をコピーして関数へ渡す方法（値渡し）を学びました。
この章では、コピーを作らずに**元の変数を使う**ための参照とポインタを学び、どの変数を読み書きしているか、その変数がいつまで使えるかを追えるようにします。
MISO では、設定や格子を参照で受け取って持ち続けたり、配列の場所をポインタで持つ軽い「View」を計算に渡したりしています。

記号の意味だけを確かめたいときは {ref}`記号の早見表 <edu-ref-cpp>` も使えます。

## 目次

- {ref}`1. コピーと参照 <edu-ch03-references>`
  - {ref}`1.1 値のコピーと参照 <edu-ch03-reference-basics>` ／ {ref}`1.2 参照渡し <edu-ch03-pass-by-reference>` ／ {ref}`1.3 出力引数 <edu-ch03-output-arguments>`
  - {ref}`1.4 読むための const 参照 <edu-ch03-const-reference>` ／ {ref}`1.5 引数の受け取り方の使い分け <edu-ch03-argument-choice>`
- {ref}`2. ポインタ <edu-ch03-pointers>`
  - {ref}`2.1 アドレスとポインタ <edu-ch03-address>` ／ {ref}`2.2 宣言と式で変わる * と & <edu-ch03-symbols-in-declaration>` ／ {ref}`2.3 指す先の付け替え <edu-ch03-rebinding>` ／ {ref}`2.4 nullptr <edu-ch03-nullptr>` ／ {ref}`2.5 ポインタを受け取る関数 <edu-ch03-pointer-arguments>`
- {ref}`3. const の位置 <edu-ch03-const-position>`
- {ref}`4. 配列とポインタ <edu-ch03-arrays>`
- {ref}`5. 変数の寿命とスコープ <edu-ch03-lifetime>`
- {ref}`6. new・delete とスマートポインタ <edu-ch03-dynamic-memory>`
- {ref}`7. 練習 <edu-ch03-exercises>`
- {ref}`8. 記号のまとめ <edu-ch03-summary>`

(edu-ch03-references)=
## 1. コピーと参照

(edu-ch03-reference-basics)=
### 1.1 値のコピーと参照

同じ値から始めることと、同じ変数を使うことを区別します。

```cpp
double a = 1.0;
double b = a;   // a の値をコピーした、別の変数 b
double &r = a;  // a の別名 r（参照）

b = 2.0;        // b だけが 2.0 になる
r = 3.0;        // r は a そのものなので、a が 3.0 になる
// a = 3.0, b = 2.0, r = 3.0
```

`double &r = a;` の `r` は、新しい `double` の変数ではなく、**`a` に付けた別の名前**です。
このような別名を**参照**（reference）と呼び、宣言では型の後に `&` を付けます。
`double& r` と `double &r` は同じ意味で、MISO のソースは `double &r` の書き方（`&` を名前に寄せる）に揃えています。

参照は、宣言するときに必ず対象を決めます。`double &r;` のように対象なしでは作れません。
また、**後から別の変数へ付け替えることはできません**。上の状態で `r = b;` と書くと、`a` に `b` の値が代入されるだけです。

(edu-ch03-pass-by-reference)=
### 1.2 参照渡し

関数の引数を参照にすると、呼び出し元の変数そのものを関数の中で使えます。

```cpp
void set_copy(double value) {      // 値渡し：value はコピー
  value = 5.0;
}

void set_original(double &value) { // 参照渡し：value は呼び出し元の変数の別名
  value = 5.0;
}

double c = 1.0;
set_copy(c);      // c は 1.0 のまま
set_original(c);  // c は 5.0 になる
```

呼び出す側はどちらも `set_copy(c)`・`set_original(c)` と書き、見た目は同じです。
違いは**関数を定義する側の `double value` と `double &value`** にあります。
このように引数を参照で受け取る方法を**参照渡し**（pass by reference）と呼びます。

Fortran では、引数は通常、呼び出し元の変数そのものとして扱われ、サブルーチンの中で代入すると呼び出し元が変わります。
**Fortran の引数に近いのは、C++ の値渡しではなく参照渡しの方です。**
Fortran で `intent(in)`、`intent(out)`、`intent(inout)` を書き分けるように、C++ では引数の型で受け取り方を書き分けます（{ref}`1.5 節 <edu-ch03-argument-choice>`）。

(edu-ch03-output-arguments)=
### 1.3 出力引数

関数の戻り値は 1 つですが、参照引数を使えば複数の結果を呼び出し元へ渡せます。
Fortran のサブルーチンが `intent(out)` の引数に結果を書くのと同じ使い方です。

```cpp
void sum_and_difference(double a, double b, double &total, double &difference) {
  total = a + b;       // 呼び出し元の total に書き込む
  difference = a - b;  // 呼び出し元の difference に書き込む
}

double total = 0.0;
double difference = 0.0;
sum_and_difference(3.0, 1.0, total, difference);
// total は 4.0、difference は 2.0
```

`a` と `b` は値渡しで計算に使う値を受け取り、`total` と `difference` は参照で結果を書き込む先を受け取っています。
結果を受け取るための引数を**出力引数**と呼びます。

**MISO の実例** — {ref}`第 2 章 <edu-ch02>` の `switch` で見た `miso/include/miso/boundary_condition.hpp` の `symmetric_index` の全体です。

```cpp
__host__ __device__ inline void symmetric_index(int i, int i_total, int i_margin,
                                                int &i_ghst, int &i_trgt,
                                                Side side) {
  switch (side) {
  case Side::INNER:
    i_ghst = i;
    i_trgt = 2 * i_margin - i - 1;
    break;
  case Side::OUTER:
    i_ghst = i_total - i_margin + i;
    i_trgt = i_total - i_margin - i - 1;
    break;
  }
}
```

先頭の `__host__ __device__` は CPU と GPU の両方から呼べる印で（{ref}`第 9 章 <edu-ch09>`）、CPU ビルドでは空に置き換わります（{ref}`第 2 章 <edu-ch02>` のプリプロセッサ）。
`i`, `i_total`, `i_margin`, `side` は値渡しの入力、`int &i_ghst` と `int &i_trgt` は出力引数です。
呼び出す側で用意した 2 つの `int` に、ゴーストセルの添字と、値を写す元の添字が書き込まれます。

```cpp
#include <iostream>

#include <miso/core.hpp>

int main() {
  const int i_total = 8;   // ゴーストセルを含む格子点数
  const int i_margin = 2;  // ゴーストセルの幅
  for (int i = 0; i < i_margin; ++i) {
    int i_ghst = -1;
    int i_trgt = -1;
    miso::boundary_condition::symmetric_index(i, i_total, i_margin, i_ghst,
                                              i_trgt, miso::Side::OUTER);
    std::cout << i << ": i_ghst = " << i_ghst << ", i_trgt = " << i_trgt << "\n";
  }
}
```

```text
0: i_ghst = 6, i_trgt = 5
1: i_ghst = 7, i_trgt = 4
```

{ref}`第 1 章 <edu-ch01>` の、`MISO::MISO` をリンクする CMake 設定でビルドできます（`miso/core.hpp` が MPI と yaml-cpp のヘッダを読むためです）。
外側の境界では、ゴーストセル 6, 7 に、境界をはさんで鏡に映した位置 5, 4 の値を写すことが分かります。

(edu-ch03-const-reference)=
### 1.4 読むための const 参照

元の変数をコピーせずに使いながら、関数の中では書き換えないようにするには、`const` を付けて `const T &` と書きます（`T` は型を表す仮の名前です）。

```cpp
double twice(const double &value) {
  return 2.0 * value;  // 読むことはできる
  // value = 5.0;      // エラー：const 参照を通して書き換えることはできない
}
```

`const double &value` は「`double` を読むためだけの参照」です。Fortran の `intent(in)` に当たります。
禁止されるのは**この参照を通した書き換え**で、呼び出し元の変数が書き換え禁止になるわけではありません。
`const` 参照には `twice(1.0)` のように数値を直接渡すこともできます（{ref}`5 節 <edu-ch03-lifetime>`）。書き換え用の `double &` には数値を渡せません。

{ref}`第 2 章 <edu-ch02-exceptions>` の `catch (const std::runtime_error &e)` も、受け止めた例外を const 参照で受け取る書き方です。

**MISO の実例** — `miso/include/miso/config.hpp` の `merge_yaml` は、設定ファイルの内容 `target` に、既定値 `defaults` のうち足りない項目を書き足す関数です（宣言部分）。

```cpp
inline void merge_yaml(YAML::Node &target, const YAML::Node &defaults) {
```

`YAML::Node` は yaml-cpp の型で、YAML の木構造を表します。
書き足す先の `target` は `YAML::Node &`（書き換える）、既定値の `defaults` は `const YAML::Node &`（読むだけ）です。
宣言を見るだけで、どちらの引数が変わるかが分かります。

`miso/include/miso/time.hpp` の `Time` は、設定を `const Config &config` で受け取って、終了時刻などを読み出します（クラスのコンストラクタという仕組みで、{ref}`第 4 章 <edu-ch04>` で説明します）。

```cpp
Time(const Config &config)
```

(edu-ch03-argument-choice)=
### 1.5 引数の受け取り方の使い分け

| 関数でしたいこと | 宣言 | Fortran で近いもの |
|---|---|---|
| 数値 1 個など、コピーの軽い値を使う | `T value` | `intent(in)`（値を使うだけ） |
| コピーに手間がかかるデータを読む | `const T &value` | `intent(in)` |
| 呼び出し元の変数を書き換える | `T &value` | `intent(out)`, `intent(inout)` |

**`int` や `double` 1 個を読むだけなら、値渡しが基本です。** 数値 1 個のコピーは安く、参照にしても速くなるとは限りません。
`std::string`、`std::vector`、`Config` のように中身が大きくなりうる型を読むだけなら `const T &` にします。
書き換えるなら、数値 1 個でも `T &` にします。
この使い分けは [C++ Core Guidelines の F.16](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-in) と [F.17](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-inout) でも勧められています。

値渡しの引数に `const` を付けた `const T value` も書けます。これは「関数の中でこのコピーを書き換えない」という印で、呼び出し元には影響しません。

**MISO の実例** — `miso/include/miso/time.hpp` の `Time::update` は、時刻刻み `dt` を `const Real dt`（書き換えないコピー）で受け取ります。

```cpp
void update(const Real dt) {
  time += dt;
  n_step++;
};
```

`demo/mhd2d_vortex/src/main.cpp` の `Model` は、設定を `Config &config` で受け取ります。

```cpp
Model(Config &config) : ModelBase(config), eos(config), ic(eos), bc(), src() {}
```

`const` が付いていないのは、受け取った `config` を基底クラス `ModelBase`（{ref}`第 5 章 <edu-ch05>`）に渡し、そこで書き換え可能な参照として持ち続けるためです（{ref}`5 節 <edu-ch03-lifetime>`）。
`:` の後ろの並びは、メンバの初期化（{ref}`第 4 章 <edu-ch04>`）です。ここでは「`config` をコピーせずに、そのまま渡している」と読めば十分です。

(edu-ch03-pointers)=
## 2. ポインタ

(edu-ch03-address)=
### 2.1 アドレスとポインタ

変数はメモリ上のどこかに置かれています。その場所を表す値を**アドレス**と呼び、**アドレスを保持する変数をポインタ**（pointer）と呼びます。

```cpp
double a = 1.0;   // 数値を入れる変数
double *p = &a;   // a のアドレスを保持するポインタ p。「p は a を指す」と言う
```

| 部分 | 意味 |
|---|---|
| `double *` | 「`double` を指すポインタ」という型 |
| `p` | ポインタ変数の名前 |
| `&a` | `a` のアドレスを取り出す式 |

ポインタが指す先の値を使うには、式の中で `*p` と書きます（**間接参照**）。

```cpp
double copied = *p;  // p が指す a の値を読む。copied は 1.0
*p = 2.0;            // p が指す a を書き換える。a は 2.0
```

`a` は数値 1.0 を、`p` は「その 1.0 が置かれている場所」を持っています。
アドレスの数値そのものを覚える必要はなく、**どの変数を指しているか**を追います。

Fortran にも `pointer` 属性がありますが、Fortran のポインタは名前を書くだけで指す先の値になります。
C++ のポインタは、`p` と書けばアドレス、`*p` と書けば指す先の値で、両者を書き分けます。

(edu-ch03-symbols-in-declaration)=
### 2.2 宣言と式で変わる * と &

参照とポインタの両方に `&` と `*` が出てきます。**同じ記号でも、宣言に書く場合と式に書く場合で意味が変わります。**

```cpp
double a = 1.0;
double &r = a;   // 宣言の &：r を a の参照（別名）として作る
double *p = &a;  // 宣言の *：p をポインタとして作る。式の &a：a のアドレス
*p = 2.0;        // 式の *：p が指す先（a）に代入する。a も r も 2.0
r = 3.0;         // a に代入する。a も *p も 3.0
```

| 書く場所 | 例 | 意味 |
|---|---|---|
| 宣言の `&` | `double &r = a;` | `r` は `a` の参照 |
| 式の `&` | `&a` | `a` のアドレス |
| 宣言の `*` | `double *p = &a;` | `p` は `double` を指すポインタ |
| 式の `*` | `*p` | `p` が指す先の値 |
| 2 つの値の間の `*`, `&` | `a * b`, `i & 1` | 掛け算、ビットごとの AND |

`r` は `a` の別名なので `&r` も `a` のアドレスで、`&a`, `&r`, `p` はすべて同じ場所を表します。
宣言の `double *p` は「`*p` と書くと `double` になるような `p`」と読むと、式の `*p` とのつながりが分かります。

宣言でもう 1 つ注意する点があります。**`*` と `&` は型ではなく、それぞれの名前に付きます。**

```cpp
double *p, q;   // p はポインタ、q はただの double
double *x = nullptr, *y = nullptr;  // x も y もポインタ
```

**MISO の実例** — `miso/include/miso/grid.hpp` の `GridView` は、座標の配列をポインタで持っています（抜粋）。

```cpp
/// @brief coordinate in x/y/z direction
Real *x = nullptr, *y = nullptr, *z = nullptr;
```

3 つとも `Real *` で、名前ごとに `*` を書いています。`nullptr` は {ref}`2.4 節 <edu-ch03-nullptr>` で説明します。
`GridView` が何を指すかは {ref}`4 節 <edu-ch03-arrays>` で読みます。

<details>
<summary>コラム：宣言を使い方に似せる C の設計と、参照の & の由来</summary>

`double *p` の `*` と `*p` の `*` が同じ記号なのは、C の設計者 Ritchie が「宣言は、その変数を式で使うときの形に似せる」と決めたためです。
`double *p` は「`*p` が `double` である」という宣言として読めます。
出典：[D. M. Ritchie, *The Development of the C Language*](https://www.nokia.com/bell-labs/about/dennis-m-ritchie/chist.html)（"Embryonic C" の節）。

参照は C++ で加わった機能です。Stroustrup は、参照を導入した主な理由として演算子のオーバーロード（行列 `A + B` を、大きなデータをコピーせずに書くこと）を挙げています。
同じ節には、Fortran の関数に値を渡すために const 参照を使う例も出てきます。
出典：[B. Stroustrup, *A History of C++: 1979–1991*, 3.3.4 節](https://www.stroustrup.com/hopl2.pdf#page=25)。
ただし、参照の記号に `&` を選んだ理由そのものを述べた設計者の説明は、これらの資料には見当たりません。アドレスと関わりの深い機能なので既存の `&` を使った、というのは推測です。

宣言の `*` を型に寄せて `double* p` と書くか、名前に寄せて `double *p` と書くかは書式の好みで、意味は同じです。
Stroustrup は型を強調する `int* p` を好むと述べ、1 行に 1 つずつ宣言することを勧めています。
出典：[Stroustrup, *C++ Style and Technique FAQ*](https://www.stroustrup.com/bs_faq2.html#whitespace)。
MISO は clang-format の設定（`.clang-format`）で `double *p` の形に揃えています。

</details>

(edu-ch03-rebinding)=
### 2.3 指す先の付け替え

参照と違い、ポインタは後から別の変数を指すように変えられます。

```cpp
double a = 1.0;
double b = 4.0;
double *p = &a;  // p は a を指す
p = &b;          // p の指す先を b に変える。a と b の値は変わらない
*p = 5.0;        // 今は b を指しているので b が 5.0 になる。a は 1.0 のまま
```

`p = &b;` はポインタが持つアドレスを変える代入、`*p = 5.0;` は指す先の値を変える代入です。どちらを変えているかを、`*` の有無で見分けます。

(edu-ch03-nullptr)=
### 2.4 nullptr

`nullptr`（ヌルポインタ）は、**どこも指していない**ことを表すポインタの値です。Fortran の `nullify(p)` した状態、`associated(p)` が偽の状態に当たります。

```cpp
double *q = nullptr;  // まだ何も指していない
if (q != nullptr) {   // 指す先があるときだけ使う
  *q = 1.0;
}
```

`nullptr` のまま `*q` を読み書きしてはいけません。結果は**未定義動作**（言語が動作を保証しないこと）で、すぐ止まるとは限らず、誤った値で計算が進むこともあります。
関数の中で `double *q;` とだけ書いたポインタは、自動では `nullptr` になりません。初期値を決められないときは `nullptr` で初期化します。

**MISO の実例** — `miso/include/miso/array3d.hpp` の `Array3DView` は、データを指すポインタを `nullptr` で初期化しています（抜粋）。

```cpp
T *data_ = nullptr;
```

`T` は要素の型（`Real` など）を表すテンプレート引数です（{ref}`第 6 章 <edu-ch06>`）。ここでは `Real *data_ = nullptr;` と読めます。

(edu-ch03-pointer-arguments)=
### 2.5 ポインタを受け取る関数

関数がポインタを受け取ると、呼び出し元の変数のアドレスを通して値を書き込めます。参照引数と同じ働きを、C の書き方で表したものです。

```cpp
void get_rank(int *rank) {  // 書き込む先のアドレスを受け取る
  *rank = 3;                // 指す先に書き込む
}

int rank = -1;
get_rank(&rank);            // rank のアドレスを渡す。rank は 3 になる
```

参照引数との違いは、呼び出す側で `&rank` とアドレスを明示すること、そして `nullptr` も渡せることです。
C++ で新しく書く関数では、書き込む先が必ずあるなら参照引数を使うのが基本です。

**MISO の実例** — MPI は C の関数として作られているので、結果をポインタ引数で返します。`miso/include/miso/env.hpp` の `mpi::Env::setup` より抜粋します。

```cpp
int is_mpi_initialized = false;
MPI_Initialized(&is_mpi_initialized);
// ...
MPI_Comm_rank(world::comm, &world::rank);
MPI_Comm_size(world::comm, &world::size);
```

`MPI_Comm_rank` は、第 2 引数に渡したアドレスへ自分のランク番号を書き込みます。
`&world::rank` は、{ref}`第 2 章 <edu-ch02-namespaces>` で見た名前空間 `miso::mpi::world` の変数 `rank` のアドレスです。
Fortran の `call MPI_Comm_rank(comm, rank, ierr)` では引数がそのまま書き換えられますが、C の MPI ではアドレスを渡します（MPI は {ref}`第 8 章 <edu-ch08>` で扱います）。

同じファイルの `mpi::Env` には、次の 2 行があります（クラスのコンストラクタで、{ref}`第 4 章 <edu-ch04>` で説明します）。

```cpp
Env() { setup(nullptr, nullptr); }
Env(int &argc, char **&argv) { setup(&argc, &argv); }
```

`setup` は `void setup(int *argc, char ***argv)` と宣言され、`MPI_Init(argc, argv)` にそのまま渡します。
引数なしで作る場合は「コマンドライン引数はない」ことを `nullptr` で表し、`main` の `argc`, `argv` を受け取る場合はそのアドレスを渡しています。
`char **&argv` は「`char **` 型の変数 `argv` への参照」です。`char **` は {ref}`4 節 <edu-ch03-arrays>` で読みます。

(edu-ch03-const-position)=
## 3. const の位置

ポインタには、**ポインタ自身（どこを指すか）**と、**指す先の値**の 2 つがあり、`const` でどちらを固定するかを選べます。
`*` を境目にして、`const` が左右どちらにあるかを見ます。

- **`*` より左の `const` は指す先に掛かります。** `const double *p` は「読むだけの `double` を指すポインタ」で、`*p = 2.0;` ができません。
- **`*` より右の `const` はポインタ自身に掛かります。** `double *const p` は「指す先を変えられないポインタ」で、`p = &b;` ができません。

```cpp
double a = 1.0;
double b = 4.0;

const double *p = &a;  // 指す先を読むだけ
// *p = 2.0;           // エラー：assignment of read-only location '* p'
p = &b;                // できる：p 自身は const ではない

double *const q = &a;  // 指す先を固定
*q = 2.0;              // できる：a が 2.0 になる
// q = &b;             // エラー：assignment of read-only variable 'q'
```

| 宣言 | `*p = 2.0;`（指す先の値を変える） | `p = &b;`（指す先を変える） |
|---|---|---|
| `double *p` | できる | できる |
| `const double *p` | できない | できる |
| `double *const p` | できる | できない |
| `const double *const p` | できない | できない |

`const double *p` と `double const *p` は同じ意味です（どちらも `const` が `*` の左）。
`const double &r` は、`const double *p` と同じく「この参照を通しては読むだけ」という意味です。参照はもともと付け替えられないので、右側の `const` に当たるものはありません。
どの場合も、元の変数 `a` を `a = 5.0;` と直接書き換えることは禁止されません。

**MISO の実例** — `miso/include/miso/array1d.hpp` の `Array1DView` は、データの先頭を返す関数を 2 つ持っています（抜粋）。

```cpp
/// @brief Return pointer to the data.
__host__ __device__ T *data() noexcept { return data_; }

/// @brief Return const pointer to the data.
__host__ __device__ const T *data() const noexcept { return data_; }
```

1 つ目は書き換えてよいポインタ `T *` を、2 つ目は読むだけのポインタ `const T *` を返します。
同じ名前で 2 つあり、読むだけの場面では 2 つ目が選ばれます。どちらが選ばれるかを決める末尾の `const` は {ref}`第 4 章 <edu-ch04>`、`noexcept`（例外を投げない印）も {ref}`第 4 章 <edu-ch04>` で扱います。
ここでは、戻り値の型の `T *` と `const T *` の違いが、「書き換えられるか、読むだけか」の違いだと読めれば十分です。

(edu-ch03-arrays)=
## 4. 配列とポインタ

C++ の組込みの配列は、同じ型の値をメモリ上に連続して並べたものです。

```cpp
double raw[4] = {0.0, 1.0, 2.0, 3.0};  // double を 4 個。添字は 0〜3
double *p = raw;                        // 配列の名前は、先頭要素 raw[0] のアドレスになる
std::cout << *(p + 2) << " " << p[2] << "\n";  // どちらも raw[2] で、2 2
```

- 配列の名前 `raw` を式で使うと、ほとんどの場面で先頭要素のアドレス（`&raw[0]`）になります。
- `p + 2` は「`p` から要素 2 個分先のアドレス」です（**ポインタ演算**）。バイト数ではなく要素の個数で数えます。
- `p[i]` は `*(p + i)` と同じ意味です。添字が 0 から始まるのは、先頭が「0 個先」だからです。

そのため、関数に配列を渡すと、実際に渡るのは先頭のアドレスだけで、**要素数は伝わりません**。要素数は別の引数で渡します。

```cpp
double sum(const double *values, int n) {  // 先頭のアドレスと要素数
  double total = 0.0;
  for (int i = 0; i < n; ++i) {
    total += values[i];
  }
  return total;
}

sum(raw, 4);      // 0 + 1 + 2 + 3 = 6
sum(raw + 1, 2);  // raw[1] から 2 個：1 + 2 = 3
```

`raw + 1` を渡すと、配列の途中から始まる部分を渡せます。Fortran で `call sub(a(2), 2)` と書いて途中から渡す使い方に似ています。
`const double *` なので、`sum` の中で配列を書き換えることはできません。

範囲を「先頭のアドレス」と「最後の要素の 1 つ後ろのアドレス」の組で表すこともよくあります。`p` から `p + n` までは `0 <= i < n` と同じ範囲です。

`std::vector` も要素を連続して並べており、`v.data()` で先頭のアドレスを得られます。

```cpp
#include <algorithm>  // std::fill
std::vector<double> v(5, 1.0);                  // 1.0 を 5 個
std::fill(v.data(), v.data() + v.size(), 0.0);  // [先頭, 先頭 + 5) を 0.0 で埋める
```

**MISO の実例 1** — `miso/include/miso/utility.hpp` の `util::clear_array` は、配列を 0 で埋めます（`template` と `using T = ...` の行は {ref}`第 6 章 <edu-ch06>` の内容なので、`T` を `Real` と読みます）。

```cpp
// @brief Clear array (zero-fill)
template <typename VectorLike> inline void clear_array(VectorLike &arr) {
  using T = std::decay_t<decltype(*arr.data())>;
  std::fill(arr.data(), arr.data() + arr.size(), T(0));
}
```

`arr.data()` から `arr.data() + arr.size()` までを `T(0)`（0 を要素の型にした値）で埋めています。上の `std::vector` の例と同じ形です。

**MISO の実例 2** — `miso/include/miso/array3d.hpp` の `Array3DView` は、3 次元配列を、1 次元に並んだデータの先頭を指すポインタ `data_` と、各方向の大きさ `shape_[3]` で表しています。
`a(i0, i1, i2)` と書いたときに要素を返す部分は次のとおりです（`operator()` の仕組みは {ref}`第 4 章 <edu-ch04>`、`assert` の行は省略しました）。

```cpp
return data_[(i0 * shape_[1] + i1) * shape_[2] + i2];
```

3 つの添字から「先頭から何個目か」を計算し、`data_[...]` で 1 次元のデータを読んでいます。
最後の添字 `i2` が 1 増えると 1 個隣、`i1` が 1 増えると `shape_[2]` 個先です。最後の添字の方向に連続して並ぶので、Fortran（最初の添字の方向に連続）とは逆です。この並びが計算速度に関わることは {ref}`第 7 章 <edu-ch07>` で扱います。

**MISO の実例 3** — `miso/include/miso/grid.hpp` の `Grid::view` は、`std::vector` の座標 `x`（{ref}`第 2 章 <edu-ch02-stdlib>`）の先頭アドレス `x.data()` を `GridView` に渡します（抜粋）。

```cpp
return GridView<Real>(i_total, j_total, k_total, is, js, ks, i_margin,
                      j_margin, k_margin, min_dxyz, x.data(), y.data(),
                      z.data(), dx.data(), dy.data(), dz.data(), dxi.data(),
                      dyi.data(), dzi.data());
```

`GridView` の `x` は {ref}`2.2 節 <edu-ch03-symbols-in-declaration>` で見た `Real *x` です。
{ref}`第 2 章 <edu-ch02-nested>` の初期条件で書いた `grid.x[i]` は、このポインタに添字を付けて、`Grid` が持つ `std::vector` の要素を読んでいたことになります。
`GridView` は座標のコピーを持たず、場所だけを持つ軽い型です。このように、データを所有せずに場所だけを持つ型を MISO では **View** と呼びます（{ref}`第 4 章 <edu-ch04>`）。

**`main` の `char **argv`** — `char *` は 1 文字目を指すポインタで、C の文字列（末尾に終端の文字 `'\0'` が付いた文字の並び）を表します。
`char **argv` は「`char *` を指すポインタ」、つまり文字列の配列の先頭です。`argv[i]` が `i` 番目のコマンドライン引数の文字列です。
{ref}`第 2 章 <edu-ch02-jumps>` の `parse_config_filepath` では、`std::string_view arg(argv[i]);` で `i` 番目の引数を読んでいました。

(edu-ch03-lifetime)=
## 5. 変数の寿命とスコープ

関数の中で宣言した変数は、宣言した場所から、その変数を囲む `{ ... }` の終わりまで使えます。この範囲を**スコープ**と呼びます。
そして、通常の変数の実体は、そのブロックを抜けるときに消えます。変数が存在する期間を**寿命**と呼びます。

```cpp
double *p = nullptr;   // p は外側のブロックにある
{
  double local = 1.0;  // local はこのブロックの中だけに存在する
  p = &local;          // p に local のアドレスを保存する
}                      // local の寿命が終わる。p は残る
// ここで *p を読み書きしてはいけない
```

ポインタや参照が残っていても、**指す先の寿命は延びません**。寿命が終わった変数を指すポインタや参照を、**ぶら下がりポインタ**・**ぶら下がり参照**（dangling pointer / reference）と呼びます。
これを読み書きすると未定義動作です。正しそうな値がたまたま読めることもあるので、気づきにくい誤りになります。

Fortran でも、サブルーチンの局所変数（`save` 属性のないもの）は、サブルーチンを抜けると値が保証されません。C++ では、`for` の中や `if` の中のブロックでも同じことが起こります。

関数の中の変数への参照やポインタを返すのも、同じ誤りです。

```cpp
double &bad() {
  double local = 1.0;
  return local;  // 関数を抜けると local は消える
}
```

```text
warning: reference to local variable 'local' returned [-Wreturn-local-addr]
```

g++ は `-Wall` で警告を出しますが、どの場合でも検出できるわけではありません。値を持ち帰るときは、`double good()` のように**値で返します**（[C++ Core Guidelines F.43](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rf-dangle)）。

`const` 参照の引数に数値や計算結果を渡すと、その値を入れた一時的な実体が作られます。`twice(1.0)` の `1.0` は、その文（`;` まで）が終わるまで存在します。関数の中で、その参照をどこかに保存して後で使ってはいけません。

同じブロックの中で宣言した変数は、**宣言と逆の順に**消えます。後から宣言した変数が先に消えるので、先に宣言した変数を後の変数が参照で持つのは安全です。

**MISO の実例 1** — `miso/include/miso/mhd_model_base.hpp` の `ModelBase` は、設定 `Config` を参照のメンバとして持ち続けます（抜粋）。

```cpp
Config &config;
```

参照をメンバに持つ型の変数は、参照先（ここでは `Config` の変数）より長生きしてはいけません。
`demo/mhd2d_vortex/src/main.cpp` の `main` では、`config` を先に、`model` を後に宣言しています（{ref}`第 2 章 <edu-ch02-program>`）。

```cpp
Config config(config_path.value_or("./config.yaml"));

// Run simulation
Model model(config);
model.run();
```

`main` の終わりでは、後に宣言した `model` が先に消え、その後に `config` が消えます。`model` が使っている間、`config` は必ず存在します。

**MISO の実例 2** — `miso/include/miso/mhd_integrator_host.hpp` の `Integrator` も、格子を参照のメンバとして持ちます（抜粋）。

```cpp
/// @brief Spatial grid
Grid<Real, backend::Host> &grid;
```

`Integrator` は時間積分のたびに格子を使いますが、格子のコピーは持ちません。格子の本体は `ModelBase` の `grid` で、`Integrator` はそれより長生きしない位置に置かれています。
`<Real, backend::Host>` はテンプレート引数（{ref}`第 6 章 <edu-ch06>`）です。

**MISO の実例 3** — {ref}`4 節 <edu-ch03-arrays>` の `GridView` は、`Grid` の `std::vector` の要素を指すポインタを持つだけです。
したがって `GridView` を使えるのは、元の `Grid` が存在する間だけです。
また、`std::vector` は要素数を増やす（`push_back` など）と別の場所に確保し直すことがあり、そのとき古い `data()` のアドレスは使えなくなります。
MISO では、格子を作った後に座標の要素数を変えないので、この問題は起こりません。
所有する配列と、それを指す View の関係は {ref}`第 4 章 <edu-ch04>` で詳しく扱います。

(edu-ch03-dynamic-memory)=
## 6. new・delete とスマートポインタ

ブロックの寿命に縛られず、必要な大きさのメモリを実行時に確保したいことがあります。
C++ の基本の道具は `new` と `delete` です。Fortran の `allocate` と `deallocate` に当たります。

```cpp
int n = 4;
double *raw = new double[n];  // double を n 個確保し、先頭のアドレスを raw に入れる
for (int i = 0; i < n; ++i) {
  raw[i] = i;
}
delete[] raw;                 // 自分で解放する（配列には delete[]）
raw = nullptr;
```

`new` で確保したメモリは、`delete` するまで残ります。これを直接書くと、次の誤りが起こりやすくなります。

- 解放し忘れる（**メモリリーク**）。途中で `return` したり例外が投げられたりすると、`delete` の行に届きません。
- 解放した後のメモリを使う、同じメモリを 2 回解放する。
- `new[]` に対して `delete`（`[]` なし）を使う。

Fortran の `allocatable` 配列は、スコープを抜けると自動で解放されますが、C++ のポインタにはその仕組みがありません。
そのため、**現在の C++ では `new` と `delete` を直接書かず、解放を自動で行う型を使うのが基本です**（[C++ Core Guidelines R.11](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rr-newdelete)）。

- **`std::vector`**：要素の並びが欲しいだけなら、まずこれを使います（{ref}`第 2 章 <edu-ch02-stdlib>`）。
- **`std::unique_ptr`**（`<memory>`）：確保したメモリのただ 1 人の持ち主になるポインタです。`unique_ptr` の変数の寿命が終わると、自動で `delete` します。コピーはできません。
- **`std::shared_ptr`**（`<memory>`）：複数の持ち主で共有するポインタです。持ち主の数を数えていて、最後の持ち主が消えたときに `delete` します。

```cpp
#include <memory>

int n = 4;
std::unique_ptr<double[]> u = std::make_unique<double[]>(n);  // n 個、0 で初期化
u[1] = 1.5;

std::shared_ptr<double> s1 = std::make_shared<double>(2.0);
std::shared_ptr<double> s2 = s1;   // 同じ double を共有する
*s2 = 3.0;                         // *s1 も 3.0
// s1.use_count() は 2（持ち主が 2 つ）
// ブロックの終わりで u, s1, s2 は自動で解放される。delete は書かない
```

`<double[]>` などの `<>` はテンプレート引数（{ref}`第 6 章 <edu-ch06>`）です。`make_unique`・`make_shared` は、確保とポインタの作成を一度に行う関数です。

**MISO の実例** — MISO の配列 `Array1D`〜`Array4D` は、中で `new[]` と `delete[]` を使っています。`miso/include/miso/array1d.hpp` の `Array1D<T, backend::Host>` より抜粋します。

```cpp
Array1D(int nx0) : size_(nx0) {
  assert(size_ > 0);
  data_ = new T[size_];
}

~Array1D() { delete[] data_; }
```

1 つ目は配列を作るときに実行される部分（コンストラクタ）で、`nx0` 個の要素を確保します。2 つ目は配列の変数の寿命が終わるときに自動で実行される部分（デストラクタ）で、確保したメモリを解放します。
`new` と `delete` の組を配列の型の中に閉じ込めることで、使う側は `delete` を書かずに済み、`std::vector` と同じように寿命が終われば解放されます。この仕組み（RAII）は {ref}`第 4 章 <edu-ch04>` で学びます。
GPU 用の `Array1D<T, backend::CUDA>` では、同じ場所で GPU のメモリを確保・解放しています（{ref}`第 9 章 <edu-ch09>`）。

MISO 本体では `std::unique_ptr` は使っていません。`std::shared_ptr` も、GPU 用の縮約の作業領域を保持する `miso/include/miso/mhd_resistivity.hpp` の 1 か所だけです。メモリの確保は配列の型にまとめ、それ以外の場所は参照と View で受け渡しています。

(edu-ch03-exercises)=
## 7. 練習

**練習 1**　次のコードを順に実行した後の `a`, `b`, `r`, `*p` の値を書いてください。また、`a` を変えたのはどの行かを説明してください。

```cpp
double a = 1.0;
double b = a;
double &r = a;
double *p = &a;
b = 2.0;
r = 3.0;
*p = 4.0;
p = &b;
*p = 5.0;
```

**練習 2**　`miso/include/miso/boundary_condition.hpp` の `symmetric_index` で、`int &i_ghst` を `int i_ghst` に変えると、{ref}`1.3 節 <edu-ch03-output-arguments>` の確認プログラムの出力がどうなるかを説明してください（実際に変更する必要はありません）。

**練習 3**　次の 3 つの関数の宣言を、引数を通して何ができるか（読む・書き換える・指す先を変える）の観点で説明してください。

```cpp
void f1(const double *values, int n);
void f2(double *const values, int n);
void f3(const YAML::Node &defaults);
```

**練習 4**　次の関数の問題点を説明し、正しく直してください。

```cpp
int *make_counts(int n) {
  int counts[100];
  for (int i = 0; i < n; ++i) {
    counts[i] = 0;
  }
  return counts;
}
```

<details>
<summary>解答・解説</summary>

**練習 1**　最後は `a = 4.0`, `b = 5.0`, `r = 4.0`, `*p = 5.0` です。
`b = a;` で作った `b` は独立したコピーです。`r = 3.0;` で `a` が 3.0 に、`*p = 4.0;` で（`p` が `a` を指しているので）4.0 になります。
`p = &b;` で `p` は `b` を指すようになり、`*p = 5.0;` は `b` を変えます。`r` は `a` の別名のままなので 4.0 です。

**練習 2**　`i_ghst` が値渡しになるので、関数の中の代入はコピーにしか届きません。呼び出し元の `i_ghst` は初期値の `-1` のままで、出力の `i_ghst` はどちらの行も `-1` になります。`i_trgt` は参照のままなので、5 と 4 が表示されます。

**練習 3**　`f1` は `values` の指す先を読むだけで書き換えられません（`const` が `*` の左）。関数の中で `values` 自身を別のアドレスに変えることはできます（呼び出し元には影響しません）。
`f2` は指す先を書き換えられますが、`values` 自身を別のアドレスに変えられません（`const` が `*` の右）。
`f3` は `defaults` をコピーせずに読むだけです。`merge_yaml` の 2 つ目の引数と同じです。

**練習 4**　`counts` は関数の中の配列なので、関数を抜けると寿命が終わります。返したアドレスはぶら下がりポインタになります（{ref}`5 節 <edu-ch03-lifetime>`）。また、`n` が 100 を超えると配列の外に書き込みます。
`std::vector` を値で返すように直します。

```cpp
#include <vector>

std::vector<int> make_counts(int n) {
  std::vector<int> counts(n, 0);  // n 個、すべて 0
  return counts;
}
```

`std::vector` を値で返しても、要素が 1 つずつコピーされることは通常ありません（中身を移すだけで済むように最適化されます。ムーブは {ref}`第 4 章 <edu-ch04>` で扱います）。

</details>

(edu-ch03-summary)=
## 8. 記号のまとめ

| 書き方 | 種類 | 意味 |
|---|---|---|
| `T &r = a;` | 宣言 | `r` は `a` の参照（別名）。付け替えられない |
| `const T &r = a;` | 宣言 | `r` を通しては読むだけの参照 |
| `T *p = &a;` | 宣言 | `p` は `T` を指すポインタ。`&a` は `a` のアドレス |
| `*p` | 式 | `p` が指す先の値 |
| `p[i]` | 式 | `*(p + i)`。`p` から `i` 個先の要素 |
| `const T *p` | 宣言 | 指す先を読むだけ。`p` 自身は付け替えられる |
| `T *const p` | 宣言 | `p` 自身を付け替えられない。指す先は書き換えられる |
| `nullptr` | 値 | どこも指していないポインタ |
| `void f(T v)` | 引数 | 値渡し（コピー） |
| `void f(T &v)` | 引数 | 参照渡し。呼び出し元を書き換えられる |
| `void f(const T &v)` | 引数 | コピーせずに読むだけ |

ほかの記号（`::`, `<>`, `->` など）も含めた一覧は {ref}`記号の早見表 <edu-ref-cpp>` にあります。
参照・ポインタの規則の詳細は、C++17 規格草案 N4659 の [参照](https://timsong-cpp.github.io/cppwp/n4659/dcl.ref)、[ポインタ](https://timsong-cpp.github.io/cppwp/n4659/dcl.ptr)、[参照の初期化](https://timsong-cpp.github.io/cppwp/n4659/dcl.init.ref) で確認できます。
