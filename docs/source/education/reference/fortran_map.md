(edu-ref-fortran-map)=
# Fortran との対応表

Fortran で書いていた処理を、C++ と MISO でどう書くかを引くための表です。
左の列で Fortran の書き方を探し、右の列の書き方と、説明している章へ進んでください。
表の対応は「同じ目的に使う書き方」の対応で、意味が完全に同じとは限りません。違いがある行には注意を書いています。
ソースのパスは、MISO を clone したフォルダからの相対パスです。

## 目次

- {ref}`1. プログラムの構成 <edu-ref-fortran-map-program>`
- {ref}`2. 型・宣言・定数 <edu-ref-fortran-map-types>`
- {ref}`3. 演算子と組み込み関数 <edu-ref-fortran-map-operators>`
- {ref}`4. 制御文 <edu-ref-fortran-map-control>`
- {ref}`5. 配列 <edu-ref-fortran-map-arrays>`
- {ref}`6. 手続き（subroutine・function） <edu-ref-fortran-map-procedures>`
- {ref}`7. module と use <edu-ref-fortran-map-modules>`
- {ref}`8. 派生型 <edu-ref-fortran-map-derived-types>`
- {ref}`9. pointer と allocatable <edu-ref-fortran-map-pointers>`
- {ref}`10. 入出力とエラー処理 <edu-ref-fortran-map-io>`
- {ref}`11. MPI <edu-ref-fortran-map-mpi>`
- {ref}`12. 並列化の指示文とビルド <edu-ref-fortran-map-parallel>`

(edu-ref-fortran-map-program)=
## 1. プログラムの構成

| Fortran | C++・MISO | 注意 | 説明 |
|---|---|---|---|
| `program main` 〜 `end program` | `int main(int argc, char **argv) { ... }` | `argc`, `argv` はコマンド行の引数です | {ref}`第 2 章 <edu-ch02>` |
| `implicit none` | 不要 | C++ では、すべての変数を宣言してから使います。暗黙の型はありません | {ref}`第 2 章 <edu-ch02>` |
| `! コメント` | `// コメント`、`/* ... */` | | {ref}`第 2 章 <edu-ch02>` |
| 行の継続 `&` | 不要 | 文は `;` で終わります。改行は文の区切りになりません | {ref}`第 2 章 <edu-ch02>` |
| 大文字と小文字を区別しない | 区別する | `Real` と `real` は別の名前です | {ref}`第 2 章 <edu-ch02>` |
| `end do`, `end if` | `}` | `{ }` で囲んだ範囲がブロックです | {ref}`第 2 章 <edu-ch02>` |
| `include 'file.inc'` | `#include "file.hpp"` | `#include` はファイルの中身をその場所に展開します | {ref}`第 2 章 <edu-ch02>` |
| ソースファイル `.F90`（プリプロセッサあり） | ヘッダ `.hpp` と実装 `.cpp`。MISO はヘッダだけ（ヘッダオンリー） | ヘッダの中のテンプレートでない関数には `inline` を付けます | {ref}`第 2 章 <edu-ch02>` |
| 初期化の `call xxx_init` を順に並べる | オブジェクトを作るとコンストラクタが動く（`Model model(config);`） | 後片付けはデストラクタが自動で行います | {ref}`第 4 章 <edu-ch04>`、{ref}`第 11 章 <edu-ch11>` |

(edu-ref-fortran-map-types)=
## 2. 型・宣言・定数

| Fortran | C++・MISO | 注意 | 説明 |
|---|---|---|---|
| `integer :: n` | `int n;` | 関数の中の `int n;` は値が決まりません。`int n = 0;` のように初期化します | {ref}`第 2 章 <edu-ch02>` |
| `real :: x`（単精度） | `float x;` | | {ref}`第 2 章 <edu-ch02>` |
| `real(8) :: x`、`double precision :: x` | `double x;` | | {ref}`第 2 章 <edu-ch02>` |
| `integer, parameter :: dp = kind(1.0d0)` と `real(dp) :: x` | `using Real = double;` と `Real x;` | MISO のデモは `using Real = float;` です。MISO の部品は `Real` をテンプレート引数で受け取ります | {ref}`第 2 章 <edu-ch02>`、{ref}`第 6 章 <edu-ch06>` |
| `1.0`（既定は単精度）、`1.0d0`、`1.0_dp` | `1.0f`（`float`）、`1.0`（`double`）、`Real(1.0)` | **C++ の `1.0` は倍精度です**。`Real` が `float` の計算に `0.5` を混ぜると倍精度で計算されるので、MISO では `Real(0.5)` と書きます | {ref}`第 2 章 <edu-ch02>` |
| `logical :: f = .true.` | `bool f = true;` | | {ref}`第 2 章 <edu-ch02>` |
| `character(len=*) :: s`、`//`（連結） | `std::string s;`、`+` | 長さは自動で決まり、`trim` は不要です | {ref}`第 2 章 <edu-ch02>` |
| `real, parameter :: pi = 3.14159` | `constexpr double pi = 3.14159;`、`const` | MISO の $\pi$ は `pi<Real>`（変数テンプレート、`miso/include/miso/constants.hpp`） | {ref}`第 2 章 <edu-ch02>`、{ref}`第 6 章 <edu-ch06>` |
| `real(x)`、`dble(n)`（型変換） | `Real(x)`、`static_cast<double>(n)` | | {ref}`第 2 章 <edu-ch02>` |
| `int(x)` | `static_cast<int>(x)` | どちらも 0 の方向へ切り捨てます | {ref}`第 2 章 <edu-ch02>` |
| 型を書かずに右辺から決める書き方はない | `auto x = ...;` | 右辺の型がそのまま変数の型になります | {ref}`第 2 章 <edu-ch02>` |

(edu-ref-fortran-map-operators)=
## 3. 演算子と組み込み関数

| Fortran | C++・MISO | 注意 | 説明 |
|---|---|---|---|
| `x**2`、`x**3` | `util::pow2(x)`、`util::pow3(x)`（`miso/include/miso/utility.hpp`）、`std::pow(x, y)` | C++ にべき乗の演算子はありません。`^` は排他的論理和です | {ref}`第 2 章 <edu-ch02>` |
| `7 / 2`（整数の割り算） | `7 / 2` | どちらも結果は 3 です | {ref}`第 2 章 <edu-ch02>` |
| `mod(a, b)` | `a % b`（整数）、`std::fmod(a, b)`（浮動小数点） | 負の数でも符号の規則は同じです（割られる数の符号） | {ref}`第 2 章 <edu-ch02>` |
| `==`、`/=`、`.eq.`、`.ne.` | `==`、`!=` | `/=` は C++ では「割って代入」です | {ref}`第 2 章 <edu-ch02>` |
| `.and.`、`.or.`、`.not.` | `&&`、`\|\|`、`!` | | {ref}`第 2 章 <edu-ch02>` |
| `x = x + 1` | `x = x + 1;`、`x += 1;`、`++x;` | | {ref}`第 2 章 <edu-ch02>` |
| `sqrt`、`exp`、`sin`、`abs` | `util::sqrt`、`util::exp`、`util::sin`、`util::fabs` | `util::` の関数は `float` と `double` の両方に対応し、GPU でも呼べます | {ref}`第 2 章 <edu-ch02>`、{ref}`第 9 章 <edu-ch09>` |
| `max(a, b)`、`min(a, b, c)` | `util::max2(a, b)`、`util::min3(a, b, c)` | | {ref}`第 2 章 <edu-ch02>` |
| `sign(a, b)` | `util::copysign(a, b)` | | {ref}`第 2 章 <edu-ch02>` |
| `write(str, '(i8.8)') n`（0 で埋める） | `util::zfill(n, 8)` | 出力ファイル名に使います | {ref}`第 2 章 <edu-ch02>` |

(edu-ref-fortran-map-control)=
## 4. 制御文

| Fortran | C++・MISO | 注意 | 説明 |
|---|---|---|---|
| `if (c) then` 〜 `else if` 〜 `else` 〜 `end if` | `if (c) { } else if (c2) { } else { }` | 条件は `( )` で囲みます | {ref}`第 2 章 <edu-ch02>` |
| `do i = 1, n` | `for (int i = 0; i < n; ++i)` | **0 から始まり、上限 `n` を含みません**。回数は同じ `n` 回です | {ref}`第 2 章 <edu-ch02>` |
| `do i = n, 1, -1` | `for (int i = n - 1; i >= 0; --i)` | | {ref}`第 2 章 <edu-ch02>` |
| `do while (t < tend)` | `while (t < tend) { }` | MISO の時間ループは `ModelBase::run` の `while (time.time < time.tend)` | {ref}`第 2 章 <edu-ch02>`、{ref}`第 11 章 <edu-ch11>` |
| `exit`、`cycle` | `break`、`continue` | **C++ の `continue` は「次の回へ進む」で、Fortran の `continue` 文（何もしない）とは別物です** | {ref}`第 2 章 <edu-ch02>` |
| `select case (d)` 〜 `case (1)` | `switch (d) { case Direction::X: ...; break; }` | 各 `case` の最後に `break` がないと、次の `case` も実行されます | {ref}`第 2 章 <edu-ch02>` |
| `#ifdef` で処理を選ぶ（コンパイル時） | `#ifdef`、または `if constexpr` | `if constexpr` は型や定数で選び、選ばれなかった側はコンパイルされません | {ref}`第 2 章 <edu-ch02>`、{ref}`第 6 章 <edu-ch06>` |

(edu-ref-fortran-map-arrays)=
## 5. 配列

### 5.1 宣言と確保

| Fortran | C++・MISO | 注意 | 説明 |
|---|---|---|---|
| `real, allocatable :: a(:,:,:)` と `allocate(a(nx, ny, nz))` | `Array3D<Real> a(nx, ny, nz);` | 宣言と確保が同時です。`Array3D<Real, backend::CUDA>` なら GPU 上に確保します | {ref}`第 7 章 <edu-ch07>` |
| `deallocate(a)` | 不要 | `a` の寿命が終わるとデストラクタが解放します | {ref}`第 4 章 <edu-ch04>`、{ref}`第 7 章 <edu-ch07>` |
| `real :: a(n)`（1 次元） | `Array1D<Real> a(n);`、`std::vector<Real> a(n);` | | {ref}`第 2 章 <edu-ch02>`、{ref}`第 7 章 <edu-ch07>` |
| `real :: q(nx, ny, nz, 9)`（変数を 4 番目の添字に並べる） | `Fields` の 9 つのメンバ `ro`, `vx`, ... `ph`（それぞれ `Array3D`） | MISO は変数ごとに別の配列を持ちます（`miso/include/miso/mhd_fields.hpp`） | {ref}`第 4 章 <edu-ch04>`、{ref}`第 11 章 <edu-ch11>` |
| `size(a)`、`size(a, 1)` | `a.size()`、`a.extent(0)` | 次元の番号も 0 から数えます | {ref}`第 7 章 <edu-ch07>` |

### 5.2 添字の始まりとゴーストセル

| Fortran | C++・MISO | 注意 | 説明 |
|---|---|---|---|
| `a(1)` が最初の要素 | `a(0)` が最初の要素 | | {ref}`第 2 章 <edu-ch02>`、{ref}`第 7 章 <edu-ch07>` |
| `real :: a(1-m:nx+m)`（下限を決めてゴーストセルを持つ） | 添字は常に `0` から `i_total - 1`。内部の格子点は `i_margin` から `i_total - i_margin - 1` | MISO の配列は下限を変えられません。`i_total = i_size + 2 * i_margin` です（`miso/include/miso/grid.hpp`） | {ref}`第 8 章 <edu-ch08>` |
| `do i = 1, nx`（内部の格子点） | `for (int i = grid.i_margin; i < grid.i_total - grid.i_margin; ++i)` | | {ref}`第 8 章 <edu-ch08>`、{ref}`第 11 章 <edu-ch11>` |

### 5.3 メモリ上の並び

**Fortran は最初の添字が連続（列優先、column-major）、MISO の `Array3D` は最後の添字が連続（行優先、row-major）です。**
同じ `a(i, j, k)` と書いても、メモリ上で隣にある要素が違います。

| | Fortran の `a(i, j, k)` | MISO の `a(i, j, k)` |
|---|---|---|
| メモリ上で隣の要素 | `a(i+1, j, k)` | `a(i, j, k+1)` |
| 速く回すループ（最も内側） | `i` | `k` |
| 要素の位置（`a(nx, ny, nz)`） | `i + nx*(j + ny*k)`（0 始まりに直した場合） | `(i*ny + j)*nz + k` |

次の短いプログラムは、MISO の `Array3D` で要素がメモリの先頭から何番目にあるかを表示します（説明用の例）。

```cpp
// order.cpp: Array3D のメモリ上の並びを確かめる
#include <iostream>

#include <miso/array3d.hpp>

int main() {
  miso::Array3D<double> a(2, 3, 4);  // a(i, j, k): i < 2, j < 3, k < 4
  const double *p = a.data();       // 先頭の要素のアドレス
  // 要素のアドレスと先頭のアドレスの差 = 先頭から何番目か
  std::cout << "a(0,0,1): " << &a(0, 0, 1) - p << "\n";
  std::cout << "a(0,1,0): " << &a(0, 1, 0) - p << "\n";
  std::cout << "a(1,0,0): " << &a(1, 0, 0) - p << "\n";
  std::cout << "a(1,2,3): " << &a(1, 2, 3) - p << "\n";
}
```

```text
a(0,0,1): 1
a(0,1,0): 4
a(1,0,0): 12
a(1,2,3): 23
```

`k` を 1 増やすと隣（1 番目）、`j` を 1 増やすと 4 個先、`i` を 1 増やすと $3 \times 4 = 12$ 個先です。
Fortran で同じ大きさの配列 `a(2, 3, 4)` を作ると、`a(2, 1, 1)` が先頭から 2 番目（1 始まり）になり、`i` の方向が連続します。
MISO の出力ファイル（`mhd/mhd.*.bin`）もこの並びで書かれるので、Fortran や NumPy で読むときは添字の順に注意します。

### 5.4 配列の式と部分配列

| Fortran | C++・MISO | 注意 | 説明 |
|---|---|---|---|
| `a = 0.0`（配列全体への代入） | ループ、または `for_each` | C++ の配列型に、配列全体の演算の書き方はありません | {ref}`第 7 章 <edu-ch07>` |
| `a(:,:,:) = b(:,:,:)` | `a.copy_from(b);` | CPU と GPU の間のコピーにも使います | {ref}`第 7 章 <edu-ch07>`、{ref}`第 9 章 <edu-ch09>` |
| `maxval(a)`、`minval(a)`、`sum(a)` | `reduce(btag, range, init, map, op)` | 最小値なら `op` に `util::min2` を使います | {ref}`第 7 章 <edu-ch07>` |
| `where (a < 0) a = 0` | ループの中の `if` | | {ref}`第 7 章 <edu-ch07>` |
| 部分配列を引数に渡す `call s(a(:, :, k))` | View（`Array3DView` など）を渡すか、添字の範囲を引数で渡す | MISO の View は配列全体を指す軽い窓です。部分配列を切り出す機能はありません | {ref}`第 4 章 <edu-ch04>`、{ref}`第 7 章 <edu-ch07>` |

(edu-ref-fortran-map-procedures)=
## 6. 手続き（subroutine・function）

| Fortran | C++・MISO | 注意 | 説明 |
|---|---|---|---|
| `subroutine s(a, b)` と `call s(a, b)` | `void s(T a, U b) { }` と `s(a, b);` | 戻り値のない関数です | {ref}`第 2 章 <edu-ch02>` |
| `function f(x) result(y)` | `Real f(Real x) { return y; }` | 戻り値の型を名前の前に書き、`return` で返します | {ref}`第 2 章 <edu-ch02>` |
| 引数は参照で渡す（既定） | **引数は値で渡す（既定）**。参照で渡すには `T &a` | `void s(Real a)` の中で `a` を変えても、呼び出し側の変数は変わりません | {ref}`第 3 章 <edu-ch03>` |
| `intent(in)` | 小さな値は値渡し（`Real x`）、大きなものは `const T &x` | `const` の付いた参照は、コピーせず、書き換えも禁止します | {ref}`第 3 章 <edu-ch03>` |
| `intent(out)`、`intent(inout)` | `T &x` | | {ref}`第 3 章 <edu-ch03>` |
| `optional` と `present(x)` | 既定の引数 `void s(int n = 1)`、`std::optional<T>` と `has_value()`、`value_or()` | `parse_config_filepath` は `std::optional<std::string>` を返します（`miso/include/miso/config.hpp`） | {ref}`第 2 章 <edu-ch02>`、{ref}`第 4 章 <edu-ch04>` |
| `interface` で同じ名前に複数の手続きを束ねる（総称名） | 同じ名前で引数の型が違う関数を定義する（オーバーロード）、テンプレート | `space_centered_4th` は配列 1〜3 個の 3 通りがあります（`miso/include/miso/mhd_integrator.hpp`） | {ref}`第 2 章 <edu-ch02>`、{ref}`第 6 章 <edu-ch06>` |
| `interface` で外部手続きの引数の形を書く（明示的引用仕様） | 関数の宣言（ヘッダに書く） | | {ref}`第 2 章 <edu-ch02>` |
| `real` 版と `real(8)` 版を別々に書く | 関数テンプレート `template <typename Real> Real f(Real x)` | 1 つの定義から型ごとの関数が作られます | {ref}`第 6 章 <edu-ch06>` |
| `pure`、`elemental` | `__host__ __device__ inline` の関数を `for_each` の中で呼ぶ | GPU から呼ぶ関数には `__host__ __device__` を付けます | {ref}`第 7 章 <edu-ch07>`、{ref}`第 9 章 <edu-ch09>` |
| `recursive` | 不要 | C++ の関数は、そのまま自分を呼べます（`merge_yaml`） | {ref}`第 11 章 <edu-ch11>` |
| `contains` の内部手続き | ラムダ式 `[=](int i) { ... }`、MISO では `MISO_LAMBDA(int i) { ... }` | 周りの変数をコピーして持ち込みます（キャプチャ） | {ref}`第 7 章 <edu-ch07>` |
| 手続きを引数に渡す（`external`、手続き仮引数） | 関数オブジェクトやラムダを引数に渡す（`for_each(btag, range, f)`） | | {ref}`第 7 章 <edu-ch07>` |

(edu-ref-fortran-map-modules)=
## 7. module と use

| Fortran | C++・MISO | 注意 | 説明 |
|---|---|---|---|
| `module m` 〜 `contains` 〜 `end module` | ヘッダファイルと `namespace miso { ... }` | 名前の衝突を避ける役割は名前空間、中身を共有する役割は `#include` が担います | {ref}`第 2 章 <edu-ch02>` |
| `use m` | `#include <miso/core.hpp>` と `using namespace miso;` | `using namespace` を書かなければ `miso::Config` のように名前空間付きで書きます | {ref}`第 2 章 <edu-ch02>` |
| `use m, only: f` | `using miso::limiter::dqq_eval;` | 1 つの名前だけを使えるようにします（`miso/include/miso/mhd_artificial_viscosity_host.hpp`） | {ref}`第 2 章 <edu-ch02>` |
| サブモジュールや入れ子の分類 | 入れ子の名前空間 `miso::mhd`、`miso::util` | | {ref}`第 2 章 <edu-ch02>` |
| `private`（module の中だけで使う） | 慣習として `impl_host` などの名前空間に置く | 名前空間には `private` がありません。クラスのメンバには `private` があります | {ref}`第 2 章 <edu-ch02>`、{ref}`第 4 章 <edu-ch04>` |
| module 変数（プログラム全体で 1 つ） | 名前空間の `inline` 変数 | MPI のランクは `mpi::world::rank`（`miso/include/miso/env.hpp`） | {ref}`第 2 章 <edu-ch02>`、{ref}`第 8 章 <edu-ch08>` |
| `save` 属性の局所変数 | `static` の局所変数 | | {ref}`第 2 章 <edu-ch02>` |

(edu-ref-fortran-map-derived-types)=
## 8. 派生型

| Fortran | C++・MISO | 注意 | 説明 |
|---|---|---|---|
| `type :: t` 〜 `real :: x` 〜 `end type` | `struct T { Real x; };` | | {ref}`第 4 章 <edu-ch04>` |
| `type(t) :: a` と `a%x` | `T a;` と `a.x` | 成分の参照は `%` ではなく `.` です。ポインタ経由なら `p->x` | {ref}`第 4 章 <edu-ch04>` |
| 構造体構成子 `a = t(1.0)` | コンストラクタ `T a(1.0);` | 初期化子リスト `: x(x_)` でメンバを初期化します | {ref}`第 4 章 <edu-ch04>` |
| 型束縛手続き `procedure :: apply` と `call a%apply()` | メンバ関数 `void apply() const;` と `a.apply();` | `const` メンバ関数はメンバを書き換えません | {ref}`第 4 章 <edu-ch04>` |
| `final` 手続き | デストラクタ `~T()` | 寿命の終わりに自動で呼ばれます | {ref}`第 4 章 <edu-ch04>` |
| `extends(t)`（型の拡張） | 継承 `struct U : public T { };` | | {ref}`第 5 章 <edu-ch05>` |
| `class(t)` の多態と `deferred` 手続き | 仮想関数 `virtual`、または CRTP | MISO は仮想関数を使わず、`ModelBase<Model, Real, Backend>` の CRTP で派生クラスの部品を呼びます | {ref}`第 5 章 <edu-ch05>` |
| パラメータ化派生型 `type t(k)` | クラステンプレート `template <typename Real> struct T` | | {ref}`第 6 章 <edu-ch06>` |

(edu-ref-fortran-map-pointers)=
## 9. pointer と allocatable

| Fortran | C++・MISO | 注意 | 説明 |
|---|---|---|---|
| 仮引数の別名（引数で受けた配列を書き換える） | 参照 `T &r = a;` | 参照は作ったときに指す先が決まり、付け替えられません | {ref}`第 3 章 <edu-ch03>` |
| `real, pointer :: p` と `p => x`（`target` 属性） | `Real *p = &x;` と `*p` | `&x` はアドレス、`*p` は指す先の値。宣言と式で `*`, `&` の意味が変わります | {ref}`第 3 章 <edu-ch03>` |
| `nullify(p)`、`associated(p)` | `p = nullptr;`、`p != nullptr` | | {ref}`第 3 章 <edu-ch03>` |
| 配列ポインタで配列を指して渡す | View（`a.view()`、`a.const_view()`） | View は確保・解放をしない軽い窓で、ラムダにコピーして GPU にも渡せます | {ref}`第 4 章 <edu-ch04>`、{ref}`第 7 章 <edu-ch07>` |
| `allocatable` の成分を持つ派生型 | `Array3D` などをメンバに持つ構造体（`Fields`） | コピーは禁止または明示（`copy_from`）です | {ref}`第 4 章 <edu-ch04>` |
| `allocate` した pointer の `deallocate` 忘れ | `new`/`delete` を直接書かず、`Array3D` や `std::unique_ptr` に持たせる | | {ref}`第 3 章 <edu-ch03>` |

(edu-ref-fortran-map-io)=
## 10. 入出力とエラー処理

| Fortran | C++・MISO | 注意 | 説明 |
|---|---|---|---|
| `print *, x`、`write(*,*) x` | `std::cout << x << "\n";` | | {ref}`第 2 章 <edu-ch02>` |
| `namelist` で設定を読む | YAML の設定ファイルと `Config`（`config["time"]["tend"].as<Real>()`） | 書かなかった値は `config_defaults.hpp` の既定値で補います | {ref}`第 11 章 <edu-ch11>` |
| `open(..., form='unformatted', access='stream')` と `write(u) a` | `std::ofstream ofs(name, std::ios::binary);` と `ofs.write(...)` | MISO の出力は `miso/include/miso/mhd_checkpoint.hpp` | {ref}`第 11 章 <edu-ch11>` |
| `iostat` で失敗を調べる | `if (!ofs.is_open())`、`if (!ofs)` | | {ref}`第 11 章 <edu-ch11>` |
| `stop`、`error stop` | `throw std::runtime_error("...")`、MPI 全体を止めるなら `MPI_Abort` | MISO では、実行時のエラーを例外で知らせます | {ref}`第 2 章 <edu-ch02>` |
| ファイル名を文字列で連結する | `std::filesystem::path` と `/` | `fs::path(dir) / "config.yaml"` のように書きます | {ref}`第 2 章 <edu-ch02>` |

(edu-ref-fortran-map-mpi)=
## 11. MPI

| Fortran | C++・MISO | 注意 | 説明 |
|---|---|---|---|
| `use mpi`、`include 'mpif.h'` | `#include <mpi.h>` | | {ref}`第 8 章 <edu-ch08>` |
| `call MPI_Init(ierr)` 〜 `call MPI_Finalize(ierr)` | `Env env(argc, argv);`（`MPI_Init(&argc, &argv)` を呼び、寿命の終わりに `MPI_Finalize`） | | {ref}`第 8 章 <edu-ch08>` |
| 最後の引数 `ierr` | 関数の戻り値 | C++ では `ierr` を引数に書きません | {ref}`第 8 章 <edu-ch08>` |
| `MPI_COMM_WORLD` | `MPI_COMM_WORLD`、MISO では `mpi::comm()` | | {ref}`第 8 章 <edu-ch08>` |
| `MPI_REAL`、`MPI_DOUBLE_PRECISION` | `MPI_FLOAT`、`MPI_DOUBLE`。MISO では `mpi::data_type<Real>()` | `Real` に合わせて選ばれます（`miso/include/miso/mpi_util.hpp`） | {ref}`第 6 章 <edu-ch06>`、{ref}`第 8 章 <edu-ch08>` |
| `call MPI_Allreduce(dt, dtg, 1, MPI_REAL, MPI_MIN, MPI_COMM_WORLD, ierr)` | `MPI_Allreduce(&dt, &dtg, 1, mpi::data_type<Real>(), MPI_MIN, mpi::comm());` | スカラーは `&dt` でアドレスを渡し、配列は `a.data()` で先頭のアドレスを渡します | {ref}`第 3 章 <edu-ch03>`、{ref}`第 8 章 <edu-ch08>` |
| `integer :: req(12)`、`MPI_STATUSES_IGNORE` | `MPI_Request reqs[12];`、`MPI_STATUSES_IGNORE` | | {ref}`第 8 章 <edu-ch08>` |
| `use mpi_f08` の `type(MPI_Comm)` | `MPI_Comm` | | {ref}`第 8 章 <edu-ch08>` |
| ランク番号、`MPI_Cart_coords` の座標 | 同じ | **ランクと座標は Fortran でも 0 から始まります** | {ref}`第 8 章 <edu-ch08>` |
| `MPI_Cart_create`、`MPI_Cart_shift`、`MPI_PROC_NULL` | 同じ関数（`mpi::Shape`） | 周期境界の方向では、端の隣が反対側の端になります | {ref}`第 8 章 <edu-ch08>`、{ref}`第 11 章 <edu-ch11>` |

(edu-ref-fortran-map-parallel)=
## 12. 並列化の指示文とビルド

| Fortran | C++・MISO | 注意 | 説明 |
|---|---|---|---|
| `!$omp parallel do` | `for_each(backend::Host{}, range, f)` | MISO は OpenMP を使いません。CPU 版の `for_each` はふつうの `for` 文です | {ref}`第 7 章 <edu-ch07>` |
| `!$omp parallel do reduction(min:dt)` | `reduce(btag, range, init, map, op)` | | {ref}`第 7 章 <edu-ch07>` |
| `!$acc parallel loop`、CUDA Fortran のカーネル | `for_each(backend::CUDA{}, range, f)` と `MISO_LAMBDA` | 同じラムダが CPU と GPU の両方で動きます | {ref}`第 9 章 <edu-ch09>` |
| `!$acc data copyin(a)`、`update host(a)` | `Array3D<Real, backend::CUDA>` と `copy_from` | | {ref}`第 9 章 <edu-ch09>` |
| `-DUSE_CUDA` などのマクロ | 同じ（`#ifdef USE_CUDA`）。CMake では `target_compile_definitions` | | {ref}`第 1 章 <edu-ch01>`、{ref}`第 2 章 <edu-ch02>` |
| Makefile | `CMakeLists.txt` と `cmake -B build -S .`、`cmake --build build` | | {ref}`第 1 章 <edu-ch01>` |
| `mpif90` | `mpicxx`（CMake では `find_package(MPI)`） | | {ref}`第 1 章 <edu-ch01>` |

ここまでの書き方を組み合わせた計算は、{ref}`第 10 章 <edu-ch10>` の熱伝導で自分で書き、{ref}`第 11 章 <edu-ch11>` で MISO 本体の時間発展として読みます。
