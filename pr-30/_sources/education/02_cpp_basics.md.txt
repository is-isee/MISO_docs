(edu-ch02)=
# 2. 変数・関数・制御文

この章では、変数・演算子・条件分岐・ループ・関数を使って、短い逐次計算を C++ で書けるようにします。
続けて、処理をファイルと名前空間に分ける方法、標準ライブラリの入出力・文字列・可変長配列、例外、プリプロセッサの初歩を学びます。
各節では、小さな説明用の例のあとに、同じ文法を使っている MISO の実物を示します。

参照とポインタは {ref}`第 3 章 <edu-ch03>`、クラス・構造体は {ref}`第 4 章 <edu-ch04>`、テンプレートは {ref}`第 6 章 <edu-ch06>` で学びます。
MISO の抜粋にこれらが出てきたときは、読み飛ばしてよい部分を本文で示します。

## 目次

- {ref}`1. プログラムの形 <edu-ch02-program>`
- {ref}`2. 変数と型 <edu-ch02-variables>`
  - {ref}`2.1 型・名前・初期値 <edu-ch02-declaration>` ／ {ref}`2.2 初期化の書き方 <edu-ch02-initialization>` ／ {ref}`2.3 const と constexpr <edu-ch02-const>` ／ {ref}`2.4 auto <edu-ch02-auto>`
  - {ref}`2.5 整数と浮動小数点、型変換 <edu-ch02-conversion>` ／ {ref}`2.6 型の別名と Real <edu-ch02-real>` ／ {ref}`2.7 列挙型 <edu-ch02-enum>`
- {ref}`3. 演算子 <edu-ch02-operators>`
- {ref}`4. 条件分岐とループ <edu-ch02-control>`
  - {ref}`4.1 if と else <edu-ch02-if>` ／ {ref}`4.2 for と添字の範囲 <edu-ch02-for>` ／ {ref}`4.3 多重ループ <edu-ch02-nested>` ／ {ref}`4.4 while <edu-ch02-while>` ／ {ref}`4.5 break・continue・return <edu-ch02-jumps>` ／ {ref}`4.6 switch <edu-ch02-switch>`
- {ref}`5. 関数 <edu-ch02-functions>`
  - {ref}`5.1 宣言・定義・呼び出し <edu-ch02-function-basics>` ／ {ref}`5.2 値渡し <edu-ch02-pass-by-value>` ／ {ref}`5.3 オーバーロード <edu-ch02-overloads>`
- {ref}`6. ファイル分割とヘッダ <edu-ch02-headers>`
- {ref}`7. 名前空間 <edu-ch02-namespaces>`
- {ref}`8. 標準ライブラリの入出力・文字列・vector <edu-ch02-stdlib>`
- {ref}`9. 例外によるエラー処理 <edu-ch02-exceptions>`
- {ref}`10. プリプロセッサ <edu-ch02-preprocessor>`
- {ref}`11. 練習：demo の main.cpp の冒頭を読む <edu-ch02-demo-main>`

(edu-ch02-program)=
## 1. プログラムの形

次は説明用のプログラム全体です。{ref}`第 1 章 <edu-ch01>` と同じように `g++ -std=c++17 hello.cpp -o hello` でビルドし、`./hello` で実行できます。

```cpp
#include <iostream>

int main() {
  double dt = 0.1;  // 時間刻み
  int n_step = 3;   // ステップ数
  std::cout << "time = " << dt * n_step << std::endl;
}
```

```text
time = 0.3
```

プログラムは `main` という関数から始まり、その波括弧 `{ ... }` の中を上から順に実行します。
Fortran の `program` 〜 `end program` に当たる部分です。

- `#include <iostream>` は、画面への出力に使う `std::cout` を使えるようにする指定です（{ref}`6 節 <edu-ch02-headers>`）。
- `//` から行末まではコメントです。Fortran の `!` に当たります。`/* ... */` で複数行を囲むこともできます。
- 文の終わりには `;` を付けます。Fortran と違い、改行は文の区切りになりません。1 つの文を複数行に分けて書けます。
- `std::cout << 値` で値を画面へ出力します。`std::endl` は改行です（{ref}`8 節 <edu-ch02-stdlib>`）。
- 大文字と小文字は区別されます。`dt` と `DT` は別の名前です。

`main` の最後の `}` に達すると、プログラムは正常に終了します。
以下の短いコード例は、特に断らない限り `main` などの関数の中に書く部分です。`#include` の行はファイルの先頭に置きます。

**MISO の実例** — `demo/mhd2d_vortex/src/main.cpp` の `main` 関数です。

```cpp
int main(int argc, char **argv) {
  // Initialize MPI and CUDA environments
  Env env(argc, argv);

  // Read configuration file
  auto config_path = parse_config_filepath(argc, argv);
  Config config(config_path.value_or("./config.yaml"));

  // Run simulation
  Model model(config);
  model.run();
}
```

丸括弧の `int argc, char **argv` は、実行時にコマンドラインで与えた引数（`--config=...` など）を受け取るためのものです。`char **` の読み方は {ref}`第 3 章 <edu-ch03>` で説明します。
本体では、MPI などの環境を用意し（`env`）、設定ファイルを読み（`config`）、問題を表す `model` を作って `model.run()` で計算を進めています。
`Env`・`Config`・`Model` はクラス（{ref}`第 4 章 <edu-ch04>`）で、ここではそれぞれ「その型の変数を 1 つ作る」と読めば十分です。
MISO のデモの `main` はどれもこの程度の長さで、計算の中身はヘッダと `Model` の側にあります。

(edu-ch02-variables)=
## 2. 変数と型

(edu-ch02-declaration)=
### 2.1 型・名前・初期値

Fortran の `integer :: nx = 48` に当たる宣言は、C++ では次のように書きます。

```cpp
int nx = 48;
double dt = 0.1;
bool io_enabled = true;
```

`int nx = 48;` は、左から「型」「変数名」「初期値」と読みます。
変数の名前と型を知らせることを**宣言**、作るときに最初の値を与えることを**初期化**と呼びます。
その後の `nx = 64;` は、すでにある変数への**代入**です。
C++ では、Fortran のように宣言をプログラム単位の冒頭へ集める必要はなく、使い始める場所で宣言できます。
`implicit none` に当たる指定はなく、宣言していない名前を使うと常にコンパイルエラーになります。

| 型 | 扱う値 | Fortran でよく使う対応 | 宣言の例 |
|---|---|---|---|
| `int` | 整数（この教材の環境では 32 ビット） | `integer` | `int n_step = 0;` |
| `long long` | より広い範囲の整数 | `integer(8)` | `long long n_cells = 1000000;` |
| `float` | 単精度の実数（32 ビット） | `real` / `real(4)` | `float x = 0.5f;` |
| `double` | 倍精度の実数（64 ビット） | `real(8)`, `double precision` | `double time = 0.0;` |
| `bool` | 真偽値 `true` / `false` | `logical` | `bool periodic = false;` |
| `char` | 1 文字 | `character` | `char axis = 'x';` |
| `std::string` | 文字列（`#include <string>`） | `character(len=*)` | `std::string name = "mhd";` |

数値の書き方にも型があります。`48` は `int`、`0.1` は `double`、`0.1f` のように `f` を付けると `float` です。
Fortran の `0.1` が単精度、`0.1d0` が倍精度であるのと逆になっている点に注意します。
`'x'` は 1 文字（`char`）、`"x"` は文字列で、引用符の種類にも意味があります。

**関数の中で `double value;` とだけ書いた変数の値は決まっていません。** 0 になるとは限らないので、初期値を与えるか、読む前に必ず代入します。

(edu-ch02-initialization)=
### 2.2 初期化の書き方

初期値は `=`、丸括弧、波括弧のどれでも与えられます。

```cpp
int a = 48;
int b(48);   // 丸括弧でも初期値は 48
int c{48};   // 波括弧でも初期値は 48
int zero{};  // 空の波括弧は 0 で初期化する
```

波括弧は、値が変わってしまう変換をエラーにします。

```cpp
int n = 2.9;   // コンパイルできる。小数部が捨てられて n は 2
int m{2.9};    // エラー：narrowing conversion of '2.8999999999999999e+0' from 'double' to 'int'
```

MISO のソースには 3 つの書き方がすべて出てきます。どれも「作るときに値を入れる」と読めば十分です。
クラスでは丸括弧と波括弧の意味が違う場合があり、{ref}`第 4 章 <edu-ch04>` で扱います。

(edu-ch02-const)=
### 2.3 const と constexpr

**`const` は、初期化した後に書き換えないための指定**です。Fortran の `parameter` に近いですが、実行時に決まる値でも初期化できます。

```cpp
const int nx = 48;  // nx は以後書き換えられない
int ny = nx;        // nx の値をコピーした別の変数
ny = 72;            // ny だけが変わる。nx は 48 のまま
// nx = 64;         // エラー：const の変数には代入できない
```

`const int nx` と `int const nx` は同じ意味です。

**`constexpr` は、コンパイル時に値が決まる定数**の宣言です。初期値もコンパイル時に計算できる式でなければなりません。

```cpp
constexpr int n_dim = 3;
constexpr int n_faces = 2 * n_dim;  // コンパイル時に 6 と決まる
```

`const` は「書き換えない」、`constexpr` は「コンパイル時に決まる（したがって書き換えない）」です。
Fortran の `parameter` に意味が近いのは `constexpr` の方です。

**MISO の実例** — `miso/include/miso/eos.hpp` の `IdealEOS::gas_pressure` の中に、次の行があります。

```cpp
const Real gm_ = gm;  // device lambdas must not capture `this`
```

`Real` は実数の型の別名（{ref}`2.6 節 <edu-ch02-real>`）、`gm` は比熱比です。
実行時に読んだ比熱比 `gm` を、書き換えない局所変数 `gm_` にコピーしています。なぜコピーが必要かは {ref}`第 7 章 <edu-ch07>` と {ref}`第 9 章 <edu-ch09>` で説明します。

`constexpr` の実例は {ref}`4.5 節 <edu-ch02-jumps>` の `parse_config_filepath` にあります（`constexpr std::string_view key = "--config=";`）。

(edu-ch02-auto)=
### 2.4 auto

**`auto` は、初期値から型をコンパイラに決めてもらう指定**です。型が実行中に変わるわけではありません。

```cpp
auto n_step = 100;       // int
auto dt = 0.1;           // double
auto x = 0.1f;           // float
const auto limit = 100;  // const int
```

初期値がない `auto dt;` は書けません。
`auto` は、型の名前が長いときや、関数の戻り値をそのまま受け取るときによく使います。

**MISO の実例** — `demo/mhd2d_vortex/src/main.cpp` の `main` にある行です。

```cpp
auto config_path = parse_config_filepath(argc, argv);
```

`parse_config_filepath` の戻り値の型は `std::optional<std::string>` で、「文字列が 1 つあるか、何もない」を表す標準ライブラリの型です。
長い型名を書かずに、`auto` で受け取っています。
次の行の `config_path.value_or("./config.yaml")` は「値があればそれを、なければ `"./config.yaml"` を使う」という意味です。
つまり、`--config` を指定しなければ、カレントディレクトリの `config.yaml` を読みます。

(edu-ch02-conversion)=
### 2.5 整数と浮動小数点、型変換

整数どうしの割り算は、Fortran と同じく小数部を切り捨てた整数になります。余りは `%` で求めます（Fortran の `mod`）。

```cpp
int a = 7;
int b = 2;
std::cout << a / b << "\n";                       // 3
std::cout << a % b << "\n";                       // 1
std::cout << 1 / 2 << "\n";                       // 0
std::cout << 1.0 / 2 << "\n";                     // 0.5
std::cout << static_cast<double>(a) / b << "\n";  // 3.5
```

整数と実数を混ぜた式では、整数が実数に変換されてから計算されます（`1.0 / 2` は `1.0 / 2.0`）。
`float` と `double` を混ぜると、`float` が `double` に変換され、計算は `double` で行われます。
この自動の変換を**暗黙の型変換**と呼びます。

型をはっきり変えたいときは `static_cast<型>(値)` と書きます。Fortran の `dble(a)` や `int(x)` に当たります。
`<>` の中に変換先の型を書く形はテンプレート（{ref}`第 6 章 <edu-ch06>`）と同じ記法ですが、ここでは「型を変える」と読めば十分です。
`double(a)` や `(double)a` という古い書き方もあり、MISO では `Real(0.5)` の形をよく使います（{ref}`次節 <edu-ch02-real>`）。

`float` と `double` では、表せる桁数が違います。

```cpp
float x = 0.1f;
double y = 0.1;
std::cout.precision(17);           // 17 桁で表示する
std::cout << x << " " << y << "\n";
```

```text
0.10000000149011612 0.10000000000000001
```

`float` の有効桁はおよそ 7 桁、`double` はおよそ 16 桁です。

**MISO の実例** — `miso/include/miso/config.hpp` の `Config` のコンストラクタに次の行があります。

```cpp
int yaml_str_length = static_cast<int>(yaml_str.length());
```

文字列の長さ `yaml_str.length()` は負にならない整数型（`std::size_t`）で返ります。
MPI の関数に渡すため、`static_cast<int>` で `int` に変換しています。

(edu-ch02-real)=
### 2.6 型の別名と Real

`using 新しい名前 = 型;` で、型に別名を付けられます。変数の宣言ではありません。

```cpp
using Real = float;  // Real は float の別名
Real dt = 0.1f;      // float dt = 0.1f; と同じ
```

MISO のデモは、ファイルの冒頭で `Real` を決めています。`demo/mhd2d_vortex/src/main.cpp` より抜粋します。

```cpp
using Real = float;
```

物理量はすべて `Real` で宣言されているので、この 1 行を `using Real = double;` に変えれば倍精度で計算できます。
Fortran で `integer, parameter :: rk = kind(1.0d0)` を決めておき `real(rk)` と書くのと同じ考え方です。
（MISO 本体の中では `Real` はテンプレート引数として受け取っています。その仕組みは {ref}`第 6 章 <edu-ch06>` で説明します。）

`Real` を使うときは、**定数を `Real` に揃えて書く**のが MISO の約束です。

```cpp
using Real = float;
Real a = 1.0f / 3.0f;
Real h1 = a * 0.5;        // a を double に変換して double で計算し、結果を float に戻す
Real h2 = a * Real(0.5);  // float どうしで計算する
auto h3 = a * 0.5;        // auto なので h3 は double になる
```

`0.5` は `double` なので、`a * 0.5` は `double` で計算されます。
`h1` に入れるときに `float` へ戻るので値はほぼ同じですが、余分な変換が入り、GPU では倍精度の計算が遅くなる原因になります。
`h3` のように `auto` で受けると、`float` のつもりが `double` になります。

**MISO の実例** — `miso/include/miso/limiter.hpp` の `flux_core` より抜粋します。

```cpp
Real pp = (Real(0.5) + util::copysign(Real(0.5), ra)) *
          util::max2(Real(0.0), Real(1.0) + fh * (ra - Real(1.0)));
```

`Real(0.5)` は「0.5 を `Real` 型にした値」です。どの項も `Real` なので、計算は `Real` のまま行われます。
`util::copysign` と `util::max2` は MISO の小さな関数で、それぞれ符号の付け替えと 2 つの値の大きい方です。
（このファイルの関数の頭にある `template <typename Real>` は {ref}`第 6 章 <edu-ch06>`、`__host__ __device__` は {ref}`第 9 章 <edu-ch09>` で説明します。読むときは `Real` を `float` と思って構いません。）

(edu-ch02-enum)=
### 2.7 列挙型

決まった選択肢のうちどれかを表す型を、`enum class` で作れます。

```cpp
enum class Axis { X, Y, Z };  // 3 つの選択肢を持つ型 Axis
Axis direction = Axis::Y;     // その 1 つを変数に入れる
if (direction == Axis::Y) {
  // y 方向の処理
}
```

整数の 0・1・2 で方向を表すよりも、`Axis::Y` と書く方が意味が明確で、取り違えるとコンパイルエラーになります。

**MISO の実例** — `miso/include/miso/types.hpp` には、方向や境界の側を表す列挙型が並んでいます（抜粋）。

```cpp
enum class Direction { X, Y, Z };

enum class Side { INNER, OUTER };
```

`Direction::X` は x 方向、`Side::INNER` は領域の内側（座標の小さい側）の境界を表します。境界条件の関数などで使われます（{ref}`4.6 節 <edu-ch02-switch>`）。

(edu-ch02-operators)=
## 3. 演算子

算術の演算子は Fortran とほぼ同じです。違うのは、べき乗の演算子がないことです。
`x**2` とは書けないので、`x * x` と書くか、関数を使います（MISO では `util::pow2(x)`）。

| 意味 | C++ | Fortran |
|---|---|---|
| 四則演算 | `a + b`, `a - b`, `a * b`, `a / b` | 同じ |
| 整数の余り | `a % b` | `mod(a, b)` |
| べき乗 | なし（`x * x`, `std::pow(x, y)`） | `x**2` |
| 足し込み | `a += b`（`a = a + b`）。`-=`, `*=`, `/=` も同様 | `a = a + b` |
| 1 増やす・減らす | `++i`, `--i` | `i = i + 1` |
| 等しい・等しくない | `a == b`, `a != b` | `a == b`, `a /= b` |
| 大小 | `a < b`, `a <= b`, `a > b`, `a >= b` | 同じ |
| かつ・または・否定 | `a && b`, `a \|\| b`, `!a` | `.and.`, `.or.`, `.not.` |

注意する点が 3 つあります。

- **`=` は代入、`==` は比較です。** `if (i = 3)` は比較にならず、`i` に 3 を代入してしまいます。
- **`/=` は「割って代入」です。** Fortran の「等しくない」は C++ では `!=` です。
- **`&&` と `||` は左から調べ、左だけで結果が決まれば右を調べません**（短絡評価）。`if (n > 0 && total / n > 1.0)` では、`n` が 0 なら割り算をしません。Fortran の `.and.` には、この省略の保証がありません。

範囲の判定は `0 <= i && i < n` と書きます。数学のような `0 <= i < n` は意図どおりに動きません。

値を 2 つから選ぶ**条件演算子** `条件 ? 真のときの値 : 偽のときの値` もあります。

```cpp
double dt = 0.2;
double limited = (dt > 0.1) ? 0.1 : dt;  // 条件が真なので 0.1
```

**MISO の実例** — `miso/include/miso/env.hpp` の `is_root` は、比較の結果（`bool`）をそのまま返します。

```cpp
inline bool is_root() { return rank() == 0; }
```

`rank()` は MPI のランク番号を返す関数で、0 番のプロセスなら `true` になります（関数の書き方は {ref}`5 節 <edu-ch02-functions>`、`inline` は {ref}`6 節 <edu-ch02-headers>`）。

`miso/include/miso/utility.hpp` の `clamp` は、条件演算子を入れ子にして、値 `v` を `lo` 以上 `hi` 以下に収めます。

```cpp
return v < lo ? lo : (v > hi ? hi : v);
```

`v < lo` なら `lo`、そうでなければ「`v > hi` なら `hi`、そうでなければ `v`」です。

`miso/include/miso/config.hpp` の `parse_config_filepath` では、短絡評価を使っています。

```cpp
if (arg.size() >= key.size() && arg.compare(0, key.size(), key) == 0) {
```

左の条件で「引数が `--config=` 以上の長さがある」ことを確かめてから、右で先頭の文字が `--config=` と一致するかを調べます。左が偽なら右は実行されません。

(edu-ch02-control)=
## 4. 条件分岐とループ

(edu-ch02-if)=
### 4.1 if と else

```cpp
double dt = 0.2;
if (dt > 0.1) {   // 0.2 > 0.1 なので、こちらを実行する
  dt = 0.1;
} else {
  dt = 0.05;      // この例では実行されない
}
// dt は 0.1
```

Fortran の `if (...) then ... else ... end if` に当たります。
条件を丸括弧に入れ、実行する処理を波括弧 `{ ... }` で囲みます。波括弧で囲んだ範囲を**ブロック**と呼びます。
`then` と `end if` は書きません。条件を追加するときは `else if (...)` を使います。
処理が 1 文なら波括弧を省略できますが、この教材では範囲が分かるように付けます。

**MISO の実例** — `miso/include/miso/time.hpp` の `Time` のコンストラクタより抜粋します。

```cpp
if (!(tend > 0)) {
  throw std::runtime_error("time.tend must be positive.");
}
```

`config.yaml` の `time.tend`（終了時刻）が正でなければ、エラーを知らせて止めます（`throw` は {ref}`9 節 <edu-ch02-exceptions>`）。
`tend <= 0` ではなく `!(tend > 0)` と書いているのは、値が NaN（非数）のときも捕まえるためです。NaN との比較はすべて偽になるので、`tend <= 0` では NaN を見逃します。

(edu-ch02-for)=
### 4.2 for と添字の範囲

```cpp
int total = 0;
for (int i = 0; i < 3; ++i) {  // i = 0, 1, 2 のときに実行する
  total += i;                  // total は 0 → 1 → 3
}
// total は 3。for の中で宣言した i は、ここでは使えない
```

丸括弧の中は、`;` で区切った 3 つの部分からなります。

| 部分 | 例 | 実行する時点 |
|---|---|---|
| 初期化 | `int i = 0` | ループに入るとき 1 回だけ |
| 続ける条件 | `i < 3` | ブロックを実行する前に毎回 |
| 更新 | `++i` | ブロックを実行し終えるたびに |

Fortran で書くと次のとおりです。

```fortran
total = 0
do i = 0, 2
  total = total + i
end do
```

Fortran の `do i = 0, 2` は終点 2 を含みます。C++ の `i < 3` は 3 を含みません。
**C++ では、要素数 `n` の配列の添字は `0` から `n - 1` で、ループは `for (int i = 0; i < n; ++i)` と書くのが基本です。**
Fortran の `do i = 1, n` をそのまま `i <= n` にすると、1 つずれて配列の外を読みます。

開始値・条件・増分は自由に選べます。

| `for` の書き出し | ブロックで使う `i` |
|---|---|
| `for (int i = 1; i <= 3; ++i)` | 1, 2, 3 |
| `for (int i = 0; i < 6; i += 2)` | 0, 2, 4 |
| `for (int i = 3; i >= 1; --i)` | 3, 2, 1 |

続ける条件は毎回評価されます。ループの中で `n` を変えると、`i < n` の判定も変わります。
Fortran の `do` のように、繰り返す回数が最初に確定するわけではありません。
また、`for (...);` のように丸括弧の後に `;` を書くと、その `;` だけが繰り返され、後のブロックはループの外になります。

<details>
<summary>コラム：添字が 0 から始まり、上限を含まない理由</summary>

C++ の添字が 0 から始まるのは、C の配列の仕組みを受け継いでいるためです。配列 `a` の `a[i]` は「先頭から `i` 個先の要素」という意味で、先頭は 0 個先です（{ref}`第 3 章 <edu-ch03>` のポインタ演算）。

範囲を「下限を含み、上限を含まない」形 `0 <= i < n` で書く習慣には、Dijkstra の短い覚え書きがよく引用されます。
彼は、この形なら要素数が上限と下限の差 `n - 0` に等しく、空の範囲も `0 <= i < 0` と自然に書けること、さらに添字を 0 から始めれば「ある要素の添字は、その前にある要素の個数に等しい」ことを利点に挙げています。
出典：[E. W. Dijkstra, *Why numbering should start at zero* (EWD831, 1982)](https://www.cs.utexas.edu/~EWD/transcriptions/EWD08xx/EWD831.html)。

MISO でも、範囲は `[始点, 終点)` の形で持っています。たとえば並列ループの範囲 `Range1D{0, n}` は `0 <= i < n` を表します（{ref}`第 7 章 <edu-ch07>`）。

</details>

(edu-ch02-nested)=
### 4.3 多重ループ

```cpp
int visits = 0;
for (int k = 0; k < 2; ++k) {
  for (int j = 0; j < 3; ++j) {
    for (int i = 0; i < 4; ++i) {  // 最も内側の i が最も速く変わる
      ++visits;
    }
  }
}
// visits は 2 × 3 × 4 = 24
```

Fortran の入れ子の `do` と同じです。通常の `for` を書いただけでは並列には実行されません。並列に計算する書き方は {ref}`第 7 章 <edu-ch07>` で学びます。

**MISO の実例** — `demo/mhd2d_vortex/src/main.cpp` の `InitialCondition::apply` は、全格子点に初期値を入れる 3 重ループです。一部を抜粋し、省略した箇所に説明のコメントを足しました。

```cpp
for (int k = 0; k < grid.k_total; ++k) {
  for (int j = 0; j < grid.j_total; ++j) {
    for (int i = 0; i < grid.i_total; ++i) {
      qq.ro(i, j, k) = 1.0;
      qq.vx(i, j, k) = -v0 * util::sin(2.0 * pi<Real> * grid.y[j]);
      qq.vy(i, j, k) = +v0 * util::sin(2.0 * pi<Real> * grid.x[i]);
      // （以下、他の変数も同様）
    }
  }
}
```

`grid.i_total` は x 方向の格子点数（ゴーストセルを含む）で、ゴーストセルは領域の端に足す余白のセル（{ref}`第 8 章 <edu-ch08>`）です。`i` は 0 から `grid.i_total - 1` まで動きます。
`qq.ro(i, j, k)` は密度の配列の要素、`grid.x[i]` は x 座標です。
`qq.ro(i, j, k)` のように丸括弧で添字を書ける仕組みは {ref}`第 4 章 <edu-ch04>`、`pi<Real>`（円周率を `Real` 型で表した定数）は {ref}`第 6 章 <edu-ch06>` で説明します。
ここでは「3 方向の添字を 0 から順に回し、各点に値を入れる」と読めれば十分です。

(edu-ch02-while)=
### 4.4 while

回数ではなく条件で繰り返すときは `while` を使います。Fortran の `do while` に当たります。

```cpp
double residual = 1.0;
int iterations = 0;
while (residual > 0.1) {  // 実行する前に毎回条件を調べる
  residual *= 0.5;
  ++iterations;
}
// residual は 0.0625、iterations は 4
```

ブロックの中で条件に関わる変数を更新しないと、ループが終わりません。
先に 1 回実行してから条件を調べる `do { ... } while (条件);` もあります。

**MISO の実例** — `miso/include/miso/mhd_model_base.hpp` の `ModelBase::run` で、時間発展を進めるループです。

```cpp
while (time.time < time.tend) {
  d.update();
  save_if_needed();
}
```

現在の時刻 `time.time` が終了時刻 `time.tend` に達するまで、1 ステップの更新 `d.update()` と、必要なら出力を繰り返します。
`d` は問題ごとのモデルを指す名前で、その仕組みは {ref}`第 5 章 <edu-ch05>` で説明します。

(edu-ch02-jumps)=
### 4.5 break・continue・return

| C++ | 動作 | Fortran |
|---|---|---|
| `break;` | 今のループを終えて、その直後へ進む | `exit` |
| `continue;` | 今回の残りを飛ばして、次の繰り返しへ進む | `cycle` |
| `return;`、`return 値;` | 関数そのものを終えて、呼び出し元へ戻る | `return` |

```cpp
int total = 0;
for (int i = 0; i < 6; ++i) {
  if (i == 2) {
    continue;  // 足し込みを飛ばして ++i へ進む
  }
  if (i == 5) {
    break;     // ループを終える
  }
  total += i;  // 足すのは 0, 1, 3, 4
}
// total は 8
```

**C++ の `continue` は Fortran の `continue` 文（何もしない文）とは別物です。** Fortran の `cycle` に当たります。
多重ループの内側で `break` を書くと、抜けるのは内側のループだけです。

**MISO の実例** — `miso/include/miso/config.hpp` の `parse_config_filepath` の全体です（`inline` は {ref}`6 節 <edu-ch02-headers>`、`std::optional` は {ref}`2.4 節 <edu-ch02-auto>`、`throw` は {ref}`9 節 <edu-ch02-exceptions>`）。

```cpp
inline std::optional<std::string> parse_config_filepath(int argc, char **argv) {
  for (int i = 1; i < argc; ++i) {
    std::string_view arg(argv[i]);

    if (arg == "--config") {
      if (i + 1 >= argc)
        throw std::runtime_error("--config requires filepath");
      return std::string(argv[i + 1]);
    }

    constexpr std::string_view key = "--config=";
    if (arg.size() >= key.size() && arg.compare(0, key.size(), key) == 0) {
      return std::string(arg.substr(key.size()));
    }
  }
  return std::nullopt;
}
```

`argv[i]` は `i` 番目のコマンドライン引数の文字列です（`argv` の型は {ref}`第 3 章 <edu-ch03>` で読みます）。
`std::string_view`（`<string_view>`）は、文字列をコピーせずに読むための型で、`==` で比べたり、`size()`（長さ）、`compare`（部分の比較）、`substr`（部分の取り出し）を使えます。
`argv[0]` はプログラム自身の名前なので、ループは `i = 1` から始まり、`argc - 1` まで進みます。
`--config path` の形なら次の引数 `argv[i + 1]` を、`--config=path` の形なら `=` より後ろを返し、その時点で関数を終えます。
見つからずにループを終えたら、最後の `return std::nullopt;` で「値なし」を返します。
ループを抜ける `break` の代わりに `return` で関数ごと抜ける書き方は、探索によく使われます。

(edu-ch02-switch)=
### 4.6 switch

整数や列挙型の値で分岐するときは `switch` も使えます。Fortran の `select case` に近いものです。

```cpp
enum class Axis { X, Y, Z };
Axis axis = Axis::Y;
int n = 0;
switch (axis) {
case Axis::X:
  n = 64;
  break;
case Axis::Y:  // ここから実行する
  n = 32;
  break;       // switch を抜ける
case Axis::Z:
  n = 1;
  break;
}
// n は 32
```

**`case` の境目では処理は止まりません。** `break` を書き忘れると、次の `case` の処理も続けて実行します。
どの `case` にも当てはまらない場合の処理は `default:` に書きます。

**MISO の実例** — `miso/include/miso/boundary_condition.hpp` の `symmetric_index` は、境界の内側・外側で、ゴーストセルとその値を写す元のセルの添字を計算します。

```cpp
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
```

`side` は {ref}`2.7 節 <edu-ch02-enum>` の `Side` 型です。
ゴーストセルの幅 `i_margin` が 2 なら、内側の境界では `i = 0, 1` に対して `(i_ghst, i_trgt)` が `(0, 3)`, `(1, 2)` となり、境界をはさんで鏡に映した位置の組になります。
結果の `i_ghst` と `i_trgt` を呼び出し元へ返す仕組み（参照引数）は {ref}`第 3 章 <edu-ch03>` で扱います。

### 練習：ループの条件

次のループで、ブロックに入るときの `i` の値と、最後の `total` を答えてください。条件を `i < 5` に変えたときの `total` も求めてください。

```cpp
int total = 0;
for (int i = 1; i <= 5; i += 2) {
  if (i == 3) {
    continue;
  }
  total += i;
}
```

<details>
<summary>解答・解説</summary>

`i` は 1, 3, 5 です。3 のときは足し込みを飛ばすので、`total` は 1 + 5 = 6 です。
`continue` でも更新部分の `i += 2` は実行されます。
条件を `i < 5` に変えると `i` は 1, 3 だけになり、`total` は 1 です。

</details>

(edu-ch02-functions)=
## 5. 関数

(edu-ch02-function-basics)=
### 5.1 宣言・定義・呼び出し

C++ では、値を返す処理も、Fortran のサブルーチンに当たる処理も、すべて関数として書きます。

```cpp
double square(double x);   // 宣言

double square(double x) {  // 定義
  return x * x;
}
```

| 部分 | 意味 |
|---|---|
| 先頭の `double` | 返す値の型（**戻り値の型**） |
| `square` | 関数名 |
| `(double x)` | 引数の型と、関数の中で使う名前 |
| 宣言の末尾の `;` | 本体はここに書かないことを表す |
| 定義の `{ ... }` | 実行する処理 |
| `return` | 呼び出し元へ値を返して、関数を終える |

呼び出す側は `double area = square(2.0);` と書き、`area` は 4.0 になります。
関数は、呼び出す場所より前で宣言されている必要があります。定義が後ろにあるときは、前に宣言だけを書いておきます。

値を返さない関数は、戻り値の型を `void` にします。Fortran の `subroutine` に当たります。
呼び出しは `write_output();` のように書き、`call` は付けません。引数がなくても丸括弧は省略できません。

引数に既定値を与えることもできます（**デフォルト引数**）。

```cpp
double scale(double x, double factor = 1.0) {
  return x * factor;
}
// scale(2.0) は 2.0、scale(2.0, 3.0) は 6.0
```

**MISO の実例** — `miso/include/miso/env.hpp` の `rank` は、引数を取らず `int` を返す関数です。

```cpp
inline int rank() {
  if (!is_initialized()) {
    throw std::runtime_error(
        "mpi::Env must be instantiated at the beginning of the program.");
  }
  return world::rank;
}
```

MPI の準備ができていなければエラーを投げ、できていれば保存しておいたランク番号 `world::rank` を返します（`world::` は {ref}`7 節 <edu-ch02-namespaces>`）。

(edu-ch02-pass-by-value)=
### 5.2 値渡し

引数を `double x` と書くと、呼び出し元の値の**コピー**が関数に渡ります。関数の中で `x` を書き換えても、呼び出し元の変数は変わりません。

```cpp
void set_copy(double value) {
  value = 5.0;  // 関数の中のコピーだけが変わる
}

double a = 1.0;
set_copy(a);    // 呼び出した後も a は 1.0
```

**Fortran では、引数は通常、呼び出し元の変数そのものとして扱われ、サブルーチンの中で書き換えると呼び出し元も変わります。C++ の `double value` という引数はそうなりません。**
呼び出し元の変数を書き換えたいときに使う**参照渡し**は、{ref}`第 3 章 <edu-ch03>` で説明します。
このように値をコピーして渡す方法を**値渡し**と呼びます。数値 1 個のような小さな値は、値渡しで受け取るのが基本です。

**MISO の実例** — `miso/include/miso/utility.hpp` の `zfill` は、整数を 0 埋めした文字列にする関数です。

```cpp
/// @brief zero-fill integer to string
/// @param num target integer number
/// @param width width of the string
/// @return zero-filled string representation of the integer
inline std::string zfill(int num, int width) {
  std::ostringstream oss;
  oss << std::setw(width) << std::setfill('0') << num;
  return oss.str();
}
```

2 つの `int` を値で受け取り、`std::string` を返します。`zfill(3, 8)` は `"00000003"` です。
本体は、文字列への書き込み口 `oss` に、幅 `width`・0 埋めの指定をして `num` を書き込み、できた文字列を返しています（{ref}`8 節 <edu-ch02-stdlib>`）。
MHD の出力ファイル名 `mhd.00000003.00000000.bin`（出力番号とランク番号）や時刻ファイル名の番号は、この関数で作っています（`miso/include/miso/mhd_checkpoint.hpp`, `time.hpp`）。
関数の上の `///` で始まるコメントは、Doxygen という道具で API の説明を生成するための書き方です。

(edu-ch02-overloads)=
### 5.3 オーバーロード

C++ では、**引数の型や数が違えば、同じ名前の関数を複数定義できます**。これを**オーバーロード**と呼びます。

```cpp
int square(int x) { return x * x; }
double square(double x) { return x * x; }

int n = square(3);       // int 版を呼ぶ。n は 9
double y = square(1.5);  // double 版を呼ぶ。y は 2.25
```

どちらを呼ぶかは、渡した引数の型からコンパイル時に決まります。戻り値の型だけが違う関数は区別できません。
Fortran では、総称名を `interface` で宣言して個別の関数をまとめますが、C++ では同じ名前で定義を並べるだけです。

**MISO の実例** — `miso/include/miso/utility.hpp` の名前空間 `miso::util` には、`float` 用と `double` 用の数学関数が並んでいます。
ソースには `MISO_MATH_UTILS(sqrt, sqrtf)` という 1 行しかありませんが、これはマクロ（{ref}`10 節 <edu-ch02-preprocessor>`）で、CPU 向けにビルドすると次の 2 つの関数定義に置き換わります。

```cpp
// MISO_MATH_UTILS(sqrt, sqrtf) を展開した結果（説明のため改行を整えた）
inline float sqrt(float x) { return std::sqrt(x); }
inline double sqrt(double x) { return std::sqrt(x); }
```

同じ `sqrt` という名前で、`float` 版と `double` 版を定義しています。
`util::sqrt(x)` と書けば、`Real` が `float` でも `double` でも、その型の版が呼ばれます。
`sin`, `cos`, `exp`, `log`, `fabs` なども同じように定義されています。

次のプログラムで確かめられます（MISO のヘッダの場所を `-I` で指定します）。

```cpp
#include <iostream>

#include <miso/constants.hpp>
#include <miso/utility.hpp>

int main() {
  using Real = float;
  std::cout << miso::util::zfill(3, 8) << "\n";
  Real r = miso::util::sqrt(Real(2));    // float 版
  double d = miso::util::sqrt(2.0);      // double 版
  std::cout.precision(10);
  std::cout << r << " " << d << "\n";
  std::cout << miso::pi<Real> << " " << miso::pi<double> << "\n";
}
```

```bash
g++ -std=c++17 -I/path/to/MISO/miso/include miso_util.cpp -o miso_util
./miso_util
```

```text
00000003
1.414213538 1.414213562
3.141592741 3.141592654
```

`float` 版の結果は 7 桁程度で `double` 版とずれます。`miso::pi<Real>` は `miso/include/miso/constants.hpp` にある円周率で、`<>` に型を指定する書き方は {ref}`第 6 章 <edu-ch06>` で説明します。

### 練習：型変換とオーバーロード

`using Real = float;` のもとで、次の各行の右辺がどの型で計算されるか、`square` のどちらの版が呼ばれるかを答えてください。`square` は {ref}`5.3 節 <edu-ch02-overloads>` の 2 つです。

```cpp
Real a = 2.0f;
auto b = a * 0.5;
auto c = a * Real(0.5);
auto d = square(3) / 2;
auto e = square(3.0) / 2;
```

<details>
<summary>解答・解説</summary>

- `b`：`0.5` が `double` なので、`a` が `double` に変換されて `double` で計算されます。`b` は `double` の 1.0 です。
- `c`：`float` どうしの計算で、`c` は `float` の 1.0 です。
- `d`：`int` 版の `square` が呼ばれて 9、`9 / 2` は整数の割り算なので `d` は `int` の 4 です。
- `e`：`double` 版が呼ばれて 9.0、`9.0 / 2` は `double` の 4.5 です。

`b` のように `auto` で受けると、`float` のつもりが `double` になります。MISO で定数を `Real(0.5)` と書くのはこのためです。

</details>

(edu-ch02-headers)=
## 6. ファイル分割とヘッダ

プログラムが長くなると、関数を別のファイルに分けます。
C++ では、**ほかのファイルから使う宣言や定義をヘッダ（`.hpp`）に書き、使う側で `#include` します**。

```text
physics.hpp   関数を書いたヘッダ
main.cpp      #include "physics.hpp" して関数を使う
```

`#include "physics.hpp"` は、コンパイルの前にその位置へファイルの中身をそのまま貼り込む指定です。
Fortran の `use` は「コンパイル済みのモジュールの名前を使えるようにする」もので、仕組みは違います。
自分のプロジェクトのヘッダは `"..."` で、標準ライブラリやインストール済みのライブラリのヘッダは `<...>` で囲むのが慣例です。
コンパイラは `-I` で指定したディレクトリからもヘッダを探します（{ref}`第 1 章 <edu-ch01>`）。

**`#pragma once`**：1 つの `.cpp` の中で同じヘッダが何度も `#include` されると（ヘッダがほかのヘッダを読む場合に起こります）、同じ定義が重複してエラーになります。
ヘッダの先頭に `#pragma once` と書くと、2 回目以降の読み込みは無視されます。MISO のヘッダにはすべてこの行があります。

**`inline`**：ヘッダに関数の定義（本体）を書き、そのヘッダを 2 つの `.cpp` から `#include` すると、同じ関数の定義が 2 つできてリンクでエラーになります。
次の例では、`physics.hpp` を `main.cpp` と `a.cpp` の両方が `#include` しています。

```cpp
// physics.hpp
#pragma once

double kinetic_energy(double ro, double v) {
  return 0.5 * ro * v * v;
}
```

```bash
g++ -std=c++17 main.cpp a.cpp -o odr
```

```text
/usr/bin/ld: ...: in function `kinetic_energy(double, double)':
a.cpp:(.text+0x0): multiple definition of `kinetic_energy(double, double)'; ...main.cpp:(.text+0x0): first defined here
```

`#pragma once` は 1 つの `.cpp` の中での重複を防ぐだけで、別々の `.cpp` にできた定義の重複は防げません。
関数に `inline` を付けると、「同じ定義が複数の `.cpp` に現れてよい」という意味になり、リンクできるようになります。

```cpp
inline double kinetic_energy(double ro, double v) {
  return 0.5 * ro * v * v;
}
```

`inline` という名前は「呼び出し箇所に本体を展開する」という最適化に由来しますが、現在の C++ でのこの役割は「ヘッダに定義を置いてよい」ことです。展開するかどうかはコンパイラが決めます。

**MISO はヘッダオンリーのライブラリです。** すべての関数・クラスの定義がヘッダ（`miso/include/miso/*.hpp`）にあり、ライブラリとして別にコンパイルする `.cpp` はありません。
そのため、テンプレートでない自由関数（クラスの外の関数）には `inline` を付けるのが MISO の約束です。{ref}`5.2 節 <edu-ch02-pass-by-value>` の `zfill` や {ref}`5.1 節 <edu-ch02-function-basics>` の `rank` に `inline` が付いているのはこのためです。
（テンプレートとクラスの中で定義した関数は、`inline` がなくても複数の `.cpp` に現れてよい決まりです。{ref}`第 4 章 <edu-ch04>`・{ref}`第 6 章 <edu-ch06>` で扱います。）

**MISO の実例** — `miso/include/miso/core.hpp` は、ほかのヘッダをまとめて `#include` するだけのヘッダです。冒頭を抜粋します。

```cpp
#pragma once

// Umbrella header for the common layer shared by MHD and RT.
// Include this together with a physics header, e.g.
//   #include <miso/core.hpp>
//   #include <miso/mhd.hpp>  // or <miso/rt.hpp>

// fundamentals
#include "backend.hpp"
#include "constants.hpp"
#include "types.hpp"
#include "utility.hpp"
```

デモの `main.cpp` は `#include <miso/core.hpp>` と `#include <miso/mhd.hpp>` の 2 行で、MISO の必要なヘッダをすべて読み込みます。
MISO の中のヘッダどうしは、同じディレクトリにあるので `"..."` で、デモからは `-I` で指定した場所から探すので `<miso/...>` で読み込んでいます。

(edu-ch02-namespaces)=
## 7. 名前空間

別々のライブラリに同じ名前の関数があっても区別できるように、名前をグループに分けます。このグループを**名前空間**（namespace）と呼びます。

```cpp
namespace eos {
double gamma = 5.0 / 3.0;
}

namespace grid {
int gamma = 3;  // 名前は同じでも、eos::gamma とは別の変数
}

int main() {
  std::cout << eos::gamma << " " << grid::gamma << "\n";  // 1.66667 3
}
```

外から使うときは `名前空間::名前` と書きます。`::` は「この名前空間の中の」という意味です。
標準ライブラリの名前はすべて名前空間 `std` の中にあり、`std::cout`、`std::string` と書くのはこのためです。
名前空間は入れ子にでき、`a::b::name` のように外側から順にたどります。Fortran のモジュールの名前の区別に近い役割ですが、ファイルとは関係なく、1 つの名前空間を複数のファイルに分けて書けます。

名前を短く書く方法が 2 つあります。

- `using namespace eos;` と書くと、その後は `eos::` を省略して `gamma` と書けます。Fortran の `use eos` に近いですが、名前の衝突が起きやすくなるので、ヘッダの中では使わないのが慣例です。
- `namespace fs = std::filesystem;` と書くと、名前空間に短い別名を付けられます。

**MISO の実例** — MISO のすべての名前は名前空間 `miso` の中にあり、さらに分野ごとに分かれています。

| 名前空間 | 中身の例 | ファイル |
|---|---|---|
| `miso` | `Config`, `Env`, `Array3D`, `for_each`, `Direction` | `config.hpp`, `env.hpp`, `array3d.hpp` など |
| `miso::util` | `zfill`, `sqrt`, `pow2`, `clamp` | `utility.hpp` |
| `miso::mpi` | `rank()`, `size()`, `is_root()` | `env.hpp`, `mpi_util.hpp` |
| `miso::mhd` | `ModelBase`, `Fields`, `Integrator` | `mhd_*.hpp` |
| `miso::eos` | `IdealEOS` | `eos.hpp` |
| `miso::backend` | `Host`, `CUDA` | `backend.hpp` |

`miso/include/miso/env.hpp` の冒頭では、名前空間を 3 段に入れ子にして、MPI の情報を持つ変数を置いています。

```cpp
namespace miso {

/// @brief Global environment for MPI
namespace mpi {

// global (multi-node) MPI info
namespace world {
inline bool is_initialized = false;
inline MPI_Comm comm = MPI_COMM_NULL;
inline int rank = -1;
inline int size = -1;
}  // namespace world
```

外からは `miso::mpi::world::rank` という名前の変数で、{ref}`5.1 節 <edu-ch02-function-basics>` の関数 `rank()` が返していた値です。
変数に付いた `inline` も関数と同じ意味で、ヘッダに置いた変数が複数の `.cpp` で 1 つの変数として扱われます（C++17 から）。
`}` の後の `// namespace world` は、どの名前空間の終わりかを示すコメントです。
関数の `miso::mpi::rank()` と変数の `miso::mpi::world::rank` は、名前空間が違うので区別されます。

`miso/include/miso/utility.hpp` では、`namespace fs = std::filesystem;` で別名を付け、`fs::create_directories(...)` のように使っています。
デモの `main.cpp` は冒頭で `using namespace miso;` と書いているので、`miso::Config` を `Config`、`miso::mhd::ModelBase` を `mhd::ModelBase` と書けます。

(edu-ch02-stdlib)=
## 8. 標準ライブラリの入出力・文字列・vector

C++ に付属する部品群を**標準ライブラリ**と呼びます。数値計算コードでよく使う 3 つを紹介します。

```cpp
#include <iostream>
#include <string>
#include <vector>

int main() {
  std::string name = "mhd";
  std::string file = name + ".00000003.bin";  // + で連結する
  std::cout << file << " (" << file.size() << " 文字)\n";

  std::vector<double> x(4);  // double を 4 個。すべて 0.0
  for (int i = 0; i < static_cast<int>(x.size()); ++i) {
    x[i] = 0.5 * i;           // 添字は 0 から 3
  }
  x.push_back(2.0);           // 末尾に追加して 5 個
  std::cout << x.size() << " " << x[4] << "\n";
}
```

```text
mhd.00000003.bin (16 文字)
5 2
```

- **`std::cout`**（`<iostream>`）：`<<` で値を順に送り出して表示します。`"\n"` または `std::endl` で改行します。表示の桁数や幅は `<iomanip>` の `std::setw`, `std::setprecision` などで指定します。ファイルへの書き込みは同じ使い方の `std::ofstream`（`<fstream>`）、文字列への書き込みは `std::ostringstream`（`<sstream>`）です。
- **`std::string`**（`<string>`）：長さの変わる文字列です。`+` で連結し、`size()` で長さを得ます。Fortran の固定長の `character` と違い、後ろに空白は付きません。数値を文字列にするには `std::to_string(値)` を使います。
- **`std::vector<double>`**（`<vector>`）：要素数を実行時に決められる 1 次元配列です。Fortran の `allocatable` な 1 次元配列に近く、`x(4)` で確保し、変数の寿命が終わると自動で解放されます。添字は `x[i]` と角括弧で書き、0 から `x.size() - 1` までです。`<double>` の部分で要素の型を指定します（この記法は {ref}`第 6 章 <edu-ch06>`）。

`x.size()` のように、変数名の後に `.` を付けて呼ぶ関数をメンバ関数と呼びます（{ref}`第 4 章 <edu-ch04>`）。
`x.size()` は負にならない整数型を返すので、`int` の `i` と比べるときに `static_cast<int>` で型を揃えています。

MISO の配列計算には `std::vector` ではなく、MISO 自身の `Array1D`〜`Array4D` を使います。GPU のメモリにも置ける配列が必要だからです（{ref}`第 7 章 <edu-ch07>`）。
`std::vector` は、格子の座標のような CPU 側だけで使う 1 次元のデータに使われています。

**MISO の実例** — `miso/include/miso/grid.hpp` の `Grid` は、座標を `std::vector` で持っています（抜粋）。

```cpp
/// @brief coordinate in x direction
std::vector<Real> x;
/// @brief coordinate in y direction
std::vector<Real> y;
```

`miso/include/miso/time.hpp` の `Time::log` は、時刻とステップ数を画面に表示します（本体を抜粋）。

```cpp
if (mpi::is_root()) {
  std::cout << std::fixed << std::setprecision(2) << "time = " << std::setw(6)
            << time << ";  n_step = " << std::setw(8) << n_step
            << ";  n_output = " << std::setw(8) << n_output << std::endl;
}
```

MPI の 0 番のプロセスだけが表示します。`std::fixed << std::setprecision(2)` は「小数点以下 2 桁の固定小数点で表示」、`std::setw(6)` は「次の値を幅 6 で右詰め」の指定で、Fortran の書式 `(f6.2)` に当たります。
同じファイルの `Time::time_filepath` では、`"time." + util::zfill(...) + ".txt"` のように文字列の連結で出力ファイル名を作っています。

(edu-ch02-exceptions)=
## 9. 例外によるエラー処理

ファイルが開けない、設定値がおかしいなど、処理を続けられない状態になったら、**例外**を投げて知らせます。

```cpp
#include <stdexcept>
#include <string>

double checked_dt(double dt) {
  if (!(dt > 0.0)) {
    throw std::runtime_error("dt must be positive: " + std::to_string(dt));
  }
  return dt;
}
```

`throw` を実行すると、関数はそこで終わり、呼び出し元へ順にさかのぼります。
`std::runtime_error`（`<stdexcept>`）は、エラーの説明の文字列を持つ標準の例外です。
呼び出し側で `try` と `catch` を使うと、例外を受け止めて処理を続けられます。

```cpp
try {
  checked_dt(0.1);
  checked_dt(-1.0);  // ここで例外が投げられる
  std::cout << "ここは表示されません\n";
} catch (const std::runtime_error &e) {  // const と & は第 3 章で説明する
  std::cout << "error: " << e.what() << "\n";
}
```

```text
error: dt must be positive: -1.000000
```

どこでも `catch` されなかった例外は、プログラムを異常終了させます。そのとき、多くの環境で `what()` の文字列が表示されます。
Fortran の `stop "message"` や `error stop` に近い使い方ですが、呼び出し元で受け止めて回復する道も残せる点が違います。

似たものに `assert(条件)`（`<cassert>`）があります。条件が偽ならプログラムを止めますが、Release ビルドでは消えて何も調べません。
**MISO では、プログラムの誤り（添字の範囲外など）の検査には `assert` を、ファイルが開けないなど実行時に起こりうるエラーには例外を使います。**

**MISO の実例** — `miso/include/miso/config.hpp` の `Config` のコンストラクタは、設定ファイルがなければ例外を投げます（抜粋）。

```cpp
if (!fs::exists(load_filepath)) {
  throw std::runtime_error("Config file not found: " + load_filepath);
}
```

MISO のデモは例外を `catch` していないので、存在しない設定ファイルを指定すると次のように止まります（`mhd2d_vortex` を CPU でビルドして試した出力の冒頭）。

```bash
./mhd2d_vortex --config=no_such.yaml
```

```text
terminate called after throwing an instance of 'std::runtime_error'
  what():  Config file not found: no_such.yaml
```

`what():` の行に、`throw` で渡した文字列が表示されています。この行を読めば原因が分かります。
{ref}`4.1 節 <edu-ch02-if>` の `Time` のコンストラクタや、{ref}`5.1 節 <edu-ch02-function-basics>` の `rank()` も同じ形です。

(edu-ch02-preprocessor)=
## 10. プリプロセッサ

`#` で始まる行は、コンパイルの前に**プリプロセッサ**が処理する指示です。`#include` もその 1 つです。
プリプロセッサは C++ の文法を理解せず、テキストを置き換えたり、行を残したり消したりするだけです。

| 指示 | 意味 |
|---|---|
| `#include <file>` | ファイルの中身をその位置に貼り込む |
| `#define NAME 値` | 以後の `NAME` を `値` に置き換える（**マクロ**） |
| `#ifdef NAME` 〜 `#else` 〜 `#endif` | `NAME` が定義されていれば前半を、なければ後半を残す |
| `#ifndef NAME` | `NAME` が定義されていなければ残す |

Fortran でも、拡張子を `.F90` にして `#ifdef` を使うことがあります。それと同じものです。

```cpp
#include <iostream>

#define N_GHOST 2

#ifdef USE_DOUBLE
using Real = double;
#else
using Real = float;
#endif

int main() {
  std::cout << "N_GHOST = " << N_GHOST << ", sizeof(Real) = " << sizeof(Real) << "\n";
}
```

```bash
g++ -std=c++17 pp.cpp -o pp && ./pp                 # N_GHOST = 2, sizeof(Real) = 4
g++ -std=c++17 -DUSE_DOUBLE pp.cpp -o pp && ./pp    # N_GHOST = 2, sizeof(Real) = 8
```

`-DUSE_DOUBLE` は、コンパイラに「`USE_DOUBLE` を定義した状態で始める」と指定するオプションです。`sizeof(型)` はその型の大きさ（バイト数）です。
定数には、マクロより {ref}`2.3 節 <edu-ch02-const>` の `constexpr` を使う方が、型があって安全です。マクロは、ビルドごとの切り替えや、型では書けない繰り返しに使います。

**MISO の実例** — `demo/mhd2d_vortex/src/main.cpp` では、計算に使う機器を `USE_CUDA` で切り替えています。

```cpp
#ifdef USE_CUDA
using Backend = backend::CUDA;
#else
using Backend = backend::Host;
#endif
```

`-DUSE_CUDA=ON` を付けて CMake を実行すると、デモの `CMakeLists.txt` の `target_compile_definitions(${app_name} PRIVATE USE_CUDA)` によって `USE_CUDA` が定義され、`Backend` は GPU を表す `backend::CUDA` になります。付けなければ CPU を表す `backend::Host` です。
`Backend` の使われ方は {ref}`第 6 章 <edu-ch06>`・{ref}`第 7 章 <edu-ch07>` で説明します。

`miso/include/miso/cuda_compat.hpp` は、CUDA のコンパイラ（nvcc）が定義する `__CUDACC__` で切り替えます。

```cpp
#ifdef __CUDACC__

#include <cuda_runtime.h>

#else  // __CUDACC__

#ifndef __device__
#define __device__
#endif

#ifndef __host__
#define __host__
#endif
```

GPU 用にコンパイルするときは CUDA のヘッダを読み、CPU 用（g++ だけ）のときは `__host__` と `__device__` を「空」に置き換えます。
こうすると、GPU 用の印 `__host__ __device__` が付いた関数を、g++ でもそのままコンパイルできます。その印の意味は {ref}`第 9 章 <edu-ch09>` で説明します。

引数を取るマクロも書けます。{ref}`5.3 節 <edu-ch02-overloads>` の `MISO_MATH_UTILS` は、`miso/include/miso/utility.hpp` で次のように定義されています（CPU 用の部分を抜粋）。

```cpp
#define MISO_MATH_UTILS(name, namef)                                             \
  __host__ __device__ inline float name(float x) { return std::name(x); }        \
  __host__ __device__ inline double name(double x) { return std::name(x); }
```

行末の `\` は「次の行に続く」という意味で、マクロの定義は 1 行として扱われます。
`MISO_MATH_UTILS(sqrt, sqrtf)` と書くと、`name` が `sqrt` に置き換わり、`float` 版と `double` 版の 2 つの関数定義になります。
（`namef` の `sqrtf` は、GPU 用の定義の側で `float` 用の関数名として使われます。）
同じ形の関数を何組も書く手間を、マクロで省いています。

(edu-ch02-demo-main)=
## 11. 練習：demo の main.cpp の冒頭を読む

`demo/mhd2d_vortex/src/main.cpp` の冒頭は次のとおりです。各行（または各まとまり）が何をしているかを、この章の用語を使って説明してください。

```cpp
#include <miso/core.hpp>
#include <miso/mhd.hpp>

using namespace miso;

using Real = float;

#ifdef USE_CUDA
using Backend = backend::CUDA;
#else
using Backend = backend::Host;
#endif
```

<details>
<summary>解答・解説</summary>

- `#include <miso/core.hpp>`, `#include <miso/mhd.hpp>`：MISO の共通部分と MHD のヘッダを貼り込みます（{ref}`6 節 <edu-ch02-headers>`）。MISO はヘッダオンリーなので、これだけで MISO のすべての定義が使えるようになります。`<...>` なので、`-I` で指定した場所（CMake では `MISO::MISO` をリンクすると設定されます）から探します。
- `using namespace miso;`：以後、名前空間 `miso` の名前を `miso::` なしで書けるようにします（{ref}`7 節 <edu-ch02-namespaces>`）。`backend::CUDA` は本来 `miso::backend::CUDA` です。
- `using Real = float;`：実数の型 `Real` を単精度にします（{ref}`2.6 節 <edu-ch02-real>`）。この行を `double` に変えると倍精度で計算します。
- `#ifdef USE_CUDA` 〜 `#endif`：マクロ `USE_CUDA` が定義されていれば（CMake で `-DUSE_CUDA=ON`）、`Backend` を `backend::CUDA` の別名に、なければ `backend::Host` の別名にします（{ref}`10 節 <edu-ch02-preprocessor>`）。どちらか一方の行だけがコンパイラに渡ります。

`Real` と `Backend` の 2 つの型を冒頭で決め、その後のコード（`Model` など）はこの 2 つの名前だけを使って書かれています。精度と計算機器を、1 行ずつの変更で切り替えられる構成です。

</details>

文法の詳細は C++17 規格草案 N4659 の
[`if`・`switch`](https://timsong-cpp.github.io/cppwp/n4659/stmt.select)、
[`for`](https://timsong-cpp.github.io/cppwp/n4659/stmt.for)、
[`while`・`do`](https://timsong-cpp.github.io/cppwp/n4659/stmt.iter)、
[`break`・`continue`・`return`](https://timsong-cpp.github.io/cppwp/n4659/stmt.jump)、
[`&&`](https://timsong-cpp.github.io/cppwp/n4659/expr.log.and)・[`||`](https://timsong-cpp.github.io/cppwp/n4659/expr.log.or)、
[条件演算子](https://timsong-cpp.github.io/cppwp/n4659/expr.cond)で確認できます。
慣れない用語は {ref}`用語集 <edu-ref-glossary>`、Fortran の書き方との対応は {ref}`Fortran との対応表 <edu-ref-fortran-map>` で引けます。
