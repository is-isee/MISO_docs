(edu-ch04)=
# 4. クラス・構造体

第 3 章までに学んだ変数、関数、参照、ポインタを組み合わせて、データとそれを扱う関数をまとめた型を自分で定義します。
この章を読み終えると、MISO の配列 `Array3D`、MHD の変数をまとめた `Fields`、時刻を管理する `Time`、設定ファイルを読む `Config`、demo の `InitialCondition` が、どのようなデータを持ち、どう作られ、どう後片付けされるかを読めるようになります。

- {ref}`edu-ch04-struct`
- {ref}`edu-ch04-access`
- {ref}`edu-ch04-constructor`
- {ref}`edu-ch04-explicit`
- {ref}`edu-ch04-const-member`
- {ref}`edu-ch04-destructor`
- {ref}`edu-ch04-copy-move`
- {ref}`edu-ch04-operator`
- {ref}`edu-ch04-view`
- {ref}`edu-ch04-initial-condition`
- {ref}`edu-ch04-exercises`

この章の MISO の抜粋には、`template <typename T>` や `Array3D<Real, Backend>` のように、山括弧 `<>` の付いた名前が出てきます。
これは「型を後から指定する印」です。たとえば `Array3D<float>` は「要素が `float` の `Array3D`」、`Time<Real>` は「時刻を `Real` 型で持つ `Time`」と読みます。
仕組みは {ref}`第 6 章 <edu-ch06>` で説明します。この章では、山括弧の中を「そのクラスで使う型の指定」と読んで先へ進みます。

(edu-ch04-struct)=
## 4.1 型の定義とオブジェクト

格子数 `nx`, `ny`, `nz` を 1 つにまとめる例から始めます。
次の `GridSize` は説明用の型です。

```cpp
struct GridSize {
  int nx = 1;
  int ny = 1;
  int nz = 1;

  int cell_count() const {
    return nx * ny * nz;  // multiplies the cell counts in the 3 directions and returns the total number of cells
  }
};
```

`struct GridSize { ... };` は、`GridSize` という名前の**型**を定義します。
この定義だけでは、まだ特定の格子の変数はできていません。`int` という型の名前を書いただけでは整数の変数ができないのと同じです。
型の中に書いた変数を**メンバ変数**、関数を**メンバ関数**と呼びます。
`int nx = 1;` の `= 1` は、そのメンバの初期値です。
閉じ波括弧の後の `;` は省略できません。

Fortran の派生型で書くと、データの部分は次に当たります。

```fortran
type :: grid_size
  integer :: nx = 1, ny = 1, nz = 1
end type grid_size
```

C++ では、データに加えて、そのデータを使う関数（ここでは `cell_count`）も同じ型の中に書けます。

型を使って変数を作り、メンバを使う例です。

```cpp
GridSize a{4, 3, 2};  // initializes a's nx, ny, nz to 4, 3, 2
GridSize b{8, 3, 2};  // b has its own nx, ny, nz, separate from a
a.nx = 5;             // only a.nx becomes 5. b.nx stays 8
std::cout << a.cell_count() << " " << b.cell_count() << "\n";  // 30 48

GridSize c;           // uses the initial value 1 written in the declaration. c.cell_count() is 1
```

`a` と `b` は、それぞれが自分の `nx`, `ny`, `nz` を保存する場所を持っています。
このように、型に応じた値を保存する領域を持つものを**オブジェクト**と呼び、クラスの型で作ったオブジェクトを**インスタンス**とも呼びます。

`.` は、オブジェクトのメンバを指定する記号で、Fortran の `a%nx` の `%` に当たります。
`a.cell_count()` を呼んでいる間、関数の中の `nx` は `a.nx` を表します。
`b.cell_count()` なら `b.nx` です。メンバ関数は、どのオブジェクトから呼んだかによって、使うメンバ変数が決まります。
`cell_count()` の末尾の `const` は「この関数ではメンバ変数を書き換えない」という指定です。{ref}`4.4 節 <edu-ch04-const-member>` で説明します。

ポインタからメンバを使うときは、`.` の代わりに `->` を書きます。

```cpp
GridSize *p = &a;  // p points to a
p->ny = 6;         // same as (*p).ny = 6. a.ny becomes 6
```

`p->ny` は「`p` の指す先のオブジェクトの `ny`」です。
オブジェクトやその参照からは `.`、ポインタからは `->` を使います。

C++ でデータと関数をまとめた型を**クラス**（class）と呼びます。
クラスは `struct` と `class` のどちらのキーワードでも定義できます。`struct` で定義したものは**構造体**（structure）とも呼びますが、C++ ではクラスの一種です。
2 つの違いは {ref}`次の節 <edu-ch04-access>` で説明します。

### MISO の例：時刻を管理する Time

`miso/include/miso/time.hpp` の `Time` は、時刻やステップ数をまとめた構造体です。
メンバ変数の部分を抜き出します（元のコメント `/// @brief ...` を、英語の短いコメントに置き換えています）。

```cpp
template <typename Real> struct Time {
  Real time;                  // current time
  Real tend;                  // end time
  Real dt_output;             // time interval between outputs
  int n_step;                 // time step number
  int n_output;               // output number
  int n_output_digits;        // number of digits in output file names
  std::string time_save_dir;  // directory where the time file is written
  bool io_enabled;            // whether to do input/output
  // ... (member functions are covered from Section 4.3)
};
```

先頭の `template <typename Real>` は、時刻を表す型 `Real` を後から指定する印です（{ref}`第 6 章 <edu-ch06>`）。
demo では `using Real = float;` としているので、`time` や `tend` は `float` になります。

`Time` のオブジェクトは、MISO の時間発展を管理するクラス（{ref}`第 5 章 <edu-ch05>` で読む `ModelBase`）のメンバ `time` として作られます。
`miso/include/miso/mhd_model_base.hpp` の `ModelBase::run()` には、次の行があります。

```cpp
while (time.time < time.tend) {
```

左の `time` はオブジェクトの名前、`.time` はそのメンバ変数です。同じ綴りでも役割が違います。
「`time` というオブジェクトの、現在時刻 `time` が終了時刻 `tend` より小さい間」と読みます。

(edu-ch04-access)=
## 4.2 公開範囲：public と private

前の節の `GridSize` では、使う側のコードがどのメンバでも自由に読み書きできました。
ここでは、1 次元の一様格子を表すクラスを作り、**外から読み書きできるメンバを決めます**。
格子のセル数と格子間隔は作るときに決め、その後は読むことだけを許します。

```cpp
class Axis {
public:
  // divides the interval [x_min, x_max] into n equal cells
  explicit Axis(int n, double x_min = 0.0, double x_max = 1.0)
      : n_(n), x_min_(x_min), dx_((x_max - x_min) / n) {}

  int size() const { return n_; }      // only reads the number of cells
  double dx() const { return dx_; }    // only reads the grid spacing
  double x(int i) const {              // coordinate of the center of cell i (i starts from 0)
    return x_min_ + (i + 0.5) * dx_;
  }

private:
  int n_;
  double x_min_;
  double dx_;
};
```

`public:` の後に書いたメンバは、`main()` など、`Axis` を使う側のコードから使えます。
`private:` の後に書いた `n_`, `x_min_`, `dx_` は、使う側から直接は読み書きできず、`Axis` のメンバ関数の中からだけ使えます。
先頭の `Axis(...)` で始まる部分はオブジェクトを作るときの処理で、{ref}`次の節 <edu-ch04-constructor>` で説明します。
`explicit` は {ref}`4.3 節の explicit の項 <edu-ch04-explicit>` で説明します。

使う側では、次のように書きます。

```cpp
Axis a(4);              // divides the interval [0, 1] into 4
Axis b(10, -1.0, 1.0);  // divides the interval [-1, 1] into 10
std::cout << a.size() << " " << a.dx() << " " << a.x(0) << "\n";  // 4 0.25 0.125
std::cout << b.size() << " " << b.dx() << " " << b.x(9) << "\n";  // 10 0.2 0.9
// a.n_ = 8;           // error: a private member cannot be assigned from outside
```

コメントにした最後の行を有効にすると、g++ は `'int Axis::n_' is private within this context` というエラーを出します。
セル数を変える操作を公開していないので、作った後にセル数だけが変わって格子間隔と合わなくなる、ということが起きません。
このように、データと、それを扱う関数をクラスにまとめ、外に公開する操作を決めることを**カプセル化**（encapsulation）と呼びます。

`struct` と `class` の違いは、公開範囲を書かなかったときの既定だけです。
`struct` は `public`、`class` は `private` から始まります。
メンバ名の末尾の `_` は、private なメンバ変数を区別するための命名の習慣で、C++ の文法ではありません。

### MISO の例：private を使う Array3D と、すべて公開の構造体

MISO では、2 つの書き方を役割で使い分けています。

- `miso/include/miso/array3d.hpp` の `Array3D` は `class` で、配列の先頭アドレス `data_` と大きさ `shape_` を `private` にしています。使う側は `extent()`, `size()`, `a(i, j, k)` などの公開された関数を通して使います。確保した配列の大きさを、使う側が勝手に書き換えられないようにするためです。
- `Time`、`Config`、`Fields` などは `struct` で、メンバ変数をすべて公開しています。MISO の内部や demo から `time.n_step` や `qq.ro` のように直接使う、データのまとまりとして設計されています。

`Array3D` の抜粋は {ref}`4.5 節 <edu-ch04-destructor>` 以降で読みます。

(edu-ch04-constructor)=
## 4.3 コンストラクタと初期化子リスト

`Axis(...)` のように、クラスと同じ名前を持つ特別なメンバ関数を**コンストラクタ**（constructor）と呼びます。
オブジェクトを作るときに自動で呼ばれ、メンバ変数を初期化します。戻り値の型は書きません（`void` も付けません）。

```cpp
explicit Axis(int n, double x_min = 0.0, double x_max = 1.0)
    : n_(n), x_min_(x_min), dx_((x_max - x_min) / n) {}
```

この 2 行は、次のように分けて読みます。

| 部分 | 意味 |
|---|---|
| `Axis(int n, double x_min = 0.0, double x_max = 1.0)` | 引数を 3 つ受け取るコンストラクタです |
| `= 0.0`、`= 1.0` | **デフォルト引数**です。呼ぶ側が省略すると、この値を使います |
| `:` から `{` の前まで | **初期化子リスト**（メンバ初期化子リスト）です |
| `n_(n)` | メンバ `n_` を引数 `n` の値で初期化します |
| `dx_((x_max - x_min) / n)` | メンバ `dx_` を、式の値で初期化します |
| `{}` | 初期化の後に実行する本体です。この例では空です |

`Axis a(4);` と書くと、`x_min` と `x_max` には既定値の 0.0 と 1.0 が使われます。
デフォルト引数は後ろの引数から順に省略でき、途中の引数だけを省略することはできません。
`Axis a{4};` のように波括弧で書いても同じコンストラクタが呼ばれます。
一方、`Axis a();` と空の丸括弧を書くと、「`Axis` を返す関数 `a` の宣言」と解釈されるので注意します。

Fortran には、型を作るときに必ず実行される手続きはありません。
C++ では、コンストラクタを書いたクラスのオブジェクトは、必ずコンストラクタを通って作られます。
`Axis` には引数を取るコンストラクタしかないので、`Axis a;` と引数なしで作ることもできません（エラーになります）。
コンストラクタを 1 つも書かなかった `GridSize` では、コンパイラが引数なしのコンストラクタを用意するので、`GridSize c;` と書けました。

### メンバを初期化する順番

初期化子リストに書いた順ではなく、**クラスの中でメンバ変数を宣言した順**に初期化されます。
`Axis` では `n_`, `x_min_`, `dx_` の順に宣言し、初期化子リストも同じ順に書いています。
次の `BadAxis` は、`dx_` を `n_` より先に宣言し、`dx_` の初期化に `n_` を使っています。

```cpp
class BadAxis {
public:
  BadAxis(int n, double x_min, double x_max)
      : n_(n), x_min_(x_min), dx_((x_max - x_min) / n_) {}

private:
  double dx_;  // declared before n_, so it is initialized first
  int n_;
  double x_min_;
};
```

`dx_` を初期化する時点では、`n_` にはまだ値が入っていません。
g++ に `-Wall` を付けると、順番の違い（`-Wreorder`）と、値の入っていない `n_` を使っていること（`-Wuninitialized`）を警告します。
初期化子リストは宣言と同じ順に書き、別のメンバを使う初期化では、その順番を確かめます。
上の `Axis` のように、メンバではなく引数 `n` を使えば、宣言の順番に左右されません。

### 初期化子リストでなければ値を決められないメンバ

`const` を付けたメンバと参照のメンバは、作った後に代入できません。
これらは初期化子リストで値や参照先を決めます。
コンストラクタの本体 `{ ... }` に入った時点で、メンバの初期化はすでに終わっているからです。

```cpp
struct Scaled {
  const double factor;   // not changed after construction
  double &target;        // refers to an outside variable (Chapter 3)

  Scaled(double f, double &t) : factor(f), target(t) {}
  // Scaled(double f, double &t) { factor = f; target = t; }  // error
};
```

### MISO の例：Time のコンストラクタ

`miso/include/miso/time.hpp` の `Time` のコンストラクタを読みます（ディレクトリを作る後半は省略しています）。

```cpp
  /// @brief Default constructor
  Time(const Config &config)
      : tend(config["time"]["tend"].as<Real>()),
        dt_output(config["time"]["dt_output"].as<Real>()),
        n_output_digits(config["io"]["n_output_digits"].as<int>()),
        io_enabled(config.yaml_obj["io"]["enabled"].as<bool>()) {
    if (!(tend > 0)) {
      throw std::runtime_error("time.tend must be positive.");
    }
    if (!(dt_output > 0)) {
      throw std::runtime_error("time.dt_output must be positive.");
    }

    initialize();
    // ... (decides time_save_dir and creates the directory)
  }
```

- 引数 `const Config &config` は、設定ファイルの内容を持つ `Config` をコピーせずに読むための参照です（{ref}`第 3 章 <edu-ch03>`）。
- `config["time"]["tend"]` は、`config.yaml` の `time:` の下の `tend:` の値を取り出す書き方です。`[]` の仕組みは {ref}`4.7 節 <edu-ch04-operator>` で説明します。`.as<Real>()` は、取り出した値を `Real` 型の数として読む yaml-cpp の関数です。
- 初期化子リストでは、宣言順（`tend`, `dt_output`, `n_output_digits`, `io_enabled`）どおりに 4 つのメンバを初期化しています。
- 本体では、値を検査してから、`initialize()` を呼んで `time`, `n_step`, `n_output` を 0 にします。値が正しくなければ例外を投げ、オブジェクトは作られません。

元のコメントには `Default constructor` とありますが、C++ の用語の「デフォルトコンストラクタ」は引数なしで呼べるコンストラクタのことで、このコンストラクタは `Config` を受け取ります。
コメントの語は C++ の用語と一致していないので、本文の説明で読みます。

(edu-ch04-explicit)=
### explicit と暗黙の型変換

`Axis` のコンストラクタには `explicit` を付けました。
**`explicit` は「このコンストラクタを、別の型からの自動的な変換に使わない」という指定**です。

デフォルト引数があるので、`Axis` は整数 1 個からでも作れます。
`explicit` がないと、C++ は「整数から `Axis` を作る変換」も自動で行います。
次の 2 行は、`explicit` があるとエラーになり、ないと通ります。

```cpp
double length(const Axis &axis) { return axis.size() * axis.dx(); }

// Axis c = 4;   // without explicit, Axis(4) is created automatically
// length(4);    // without explicit, a temporary Axis(4) is created and passed
length(Axis(8));  // explicitly creating it works even with explicit (returns 1)
```

型を変える処理を自分で書かず、コンパイラに任せる変換を**暗黙の型変換**と呼びます。
引数を取り違えて整数を渡したときに、意図しない格子が作られるのを防ぐのが、ここでの `explicit` の役割です。

`miso/include/miso/eos.hpp` の理想気体の状態方程式 `IdealEOS` には、2 つのコンストラクタがあり、どちらにも `explicit` が付いています。

```cpp
template <typename Real> struct IdealEOS {
  /// @brief Ratio of specific heats (gamma)
  Real gm;

  /// @brief Construct the EOS from the configuration file
  explicit IdealEOS(const Config &config) : gm(config["eos"]["gm"].as<Real>()) {}

  /// @brief Construct the EOS from the specific heat ratio (gamma)
  explicit IdealEOS(Real gm) : gm(gm) {}
  // ...
};
```

2 つ目の `explicit IdealEOS(Real gm)` は、比熱比 $\gamma$ から状態方程式を作ります。
`explicit` があるので、状態方程式を受け取る関数に、うっかり `1.4f` のような数をそのまま渡すとエラーになります。
`: gm(gm)` は、メンバ `gm` を、同じ名前の引数 `gm` で初期化する書き方です。
初期化子リストの括弧の外の名前はメンバ、括弧の中の名前は引数を指します。

このように、同じ名前で引数の型が違うコンストラクタを複数書けます（{ref}`第 2 章 <edu-ch02>` の関数のオーバーロードと同じです）。
`IdealEOS<Real> eos(config);` なら 1 つ目、`IdealEOS<Real> eos(Real(1.4));` なら 2 つ目が呼ばれます。

(edu-ch04-const-member)=
## 4.4 const メンバ関数と this

`int size() const` の末尾の `const` は、「この関数の中ではメンバ変数を書き換えない」という指定です。
`size()` の中に `n_ = 8;` と書くとコンパイルエラーになります。
読むだけの関数には末尾に `const` を付け、書き換える関数には付けません。

この区別は、`const` を付けたオブジェクトや `const` 参照から呼べる関数を決めます。

```cpp
const Axis a(4);   // forbids changes after construction
int n = a.size();  // functions with a trailing const can be called
```

`const Axis&` で受け取った引数（前の `length` 関数）からも、末尾が `const` の `size()` と `dx()` だけが呼べます。
末尾の `const` を付け忘れると、読むだけの関数でも `const` 参照から呼べなくなります。

`miso/include/miso/time.hpp` の `Time` で、2 種類の関数を比べます（抜粋）。

```cpp
  /// @brief update time parameters
  void update(const Real dt) {
    time += dt;  // modifies members, so no trailing const
    n_step++;
  };

  /// @brief Save time parameters to file
  void save() const {  // only reads members and writes to a file
    // ...
  }
```

`update` の閉じ括弧の後の `;` は不要ですが、書いてもエラーにはなりません。

### this：呼び出したオブジェクトを指すポインタ

メンバ関数の中では、メンバ変数の名前をそのまま書いて使えました。
これは、C++ がメンバ関数に「どのオブジェクトから呼ばれたか」を、**`this` という名前のポインタ**で渡しているからです。
`a.size()` と呼ぶと、`size()` の中の `this` は `a` を指し、`n_` は `this->n_` の省略です。

```cpp
int size() const { return this->n_; }  // same as return n_;
```

`this` はポインタなので、指す先のオブジェクトそのものは `*this` と書きます。
`Time::update` の `time += dt;` も、正確には `this->time += dt;` です。
この「メンバ変数は `this` を通して使われる」という性質は、{ref}`第 7 章 <edu-ch07>` でラムダ式を学ぶときに、また {ref}`第 9 章 <edu-ch09>` で GPU での落とし穴を学ぶときに重要になります。

(edu-ch04-destructor)=
## 4.5 デストラクタと後片付け（RAII）

配列を確保したら、使い終わったときに解放が必要です。
Fortran の `allocate` と `deallocate` の組を、C++ ではクラスに任せられます。
**オブジェクトの寿命が終わるときに自動で呼ばれる関数を、デストラクタ**（destructor）と呼びます。
名前は `~` とクラス名で、引数と戻り値の型はありません。

次は、コンストラクタで配列を確保し、デストラクタで解放する説明用のクラスです。
確保と解放のときに文字を表示して、呼ばれる時点を確かめます（`std::printf` は書式付きで表示する関数で、`#include <cstdio>` で使えます）。

```cpp
#include <cstdio>

class Buffer {
public:
  explicit Buffer(int n) : data_(new double[n]), n_(n) {
    std::printf("allocated (%d elements)\n", n_);
  }
  ~Buffer() {
    std::printf("releasing (%d elements)\n", n_);
    delete[] data_;  // pairs with new[] in the constructor
  }

private:
  double *data_;
  int n_;
};

int main() {
  std::puts("before block");
  {
    Buffer work(100);  // the constructor is called here
    std::puts("while in use");
  }  // the lifetime of work ends, and the destructor is called automatically
  std::puts("after block");
}
```

実行結果は次のとおりです。

```text
before block
allocated (100 elements)
while in use
releasing (100 elements)
after block
```

デストラクタを呼ぶ行を自分で書く必要はありません。
ブロックの終わりに達したとき、関数の途中で `return` したとき、例外で関数を抜けたときにも呼ばれます。
同じブロックに複数のオブジェクトがあるときは、作った順の逆に片付けられます。
メンバ変数がクラス型なら、外側のデストラクタの本体が終わった後に、メンバのデストラクタも自動で呼ばれます。

このように、資源（メモリ、ファイル、MPI の環境など）の確保をコンストラクタで、解放をデストラクタで行い、オブジェクトの寿命に結び付ける書き方を **RAII**（Resource Acquisition Is Initialization）と呼びます。
解放を書き忘れたり、途中の `return` で解放を飛ばしたりすることを防げます。

### MISO の例：Array3D の確保と解放

`miso/include/miso/array3d.hpp` の、CPU 用の `Array3D` のコンストラクタとデストラクタです（大きさの検査の一部を省略しています）。

```cpp
template <typename T> class Array3D<T, backend::Host> {
private:
  T *data_ = nullptr;
  std::array<int, 3> shape_ = {-1, -1, -1};

public:
  Array3D(int nx0, int nx1, int nx2) : shape_{nx0, nx1, nx2} {
    assert(shape_[0] > 0 && shape_[1] > 0 && shape_[2] > 0);
    const std::size_t array_size = static_cast<size_t>(shape_[0]) *
                                   static_cast<size_t>(shape_[1]) *
                                   static_cast<size_t>(shape_[2]);
    // ... (checks that array_size fits in the range of int)
    data_ = new T[array_size];
  }

  ~Array3D() { delete[] data_; }
  // ...
};
```

- `T` は要素の型、`backend::Host` は「CPU のメモリに置く」ことを示す指定です（{ref}`第 6 章 <edu-ch06>`）。
- `std::array<int, 3> shape_` は 3 方向の大きさを持つ固定長の配列で、まだ作っていない状態を `-1` で表しています。
- コンストラクタで `new T[array_size]` によって要素を確保し、デストラクタで `delete[]` によって解放します。上の `Buffer` と同じ形です。
- `static_cast<size_t>(...)` は、掛け算の前に `int` を大きな整数型 `size_t` へ変換する書き方です。3 方向の格子数を掛けると `int` の範囲を超えることがあるためです。

GPU 用の `Array3D<T, backend::CUDA>` では、`new`/`delete[]` の代わりに GPU のメモリを確保・解放する関数を使います（{ref}`第 9 章 <edu-ch09>`）。
使う側から見れば、どちらも「作れば確保され、寿命が終われば解放される」配列です。

### MISO の例：MPI の初期化と終了を受け持つ Env

demo の `main()` の最初の行は、次のとおりです（`demo/mhd2d_vortex/src/main.cpp`）。

```cpp
int main(int argc, char **argv) {
  // Initialize MPI and CUDA environments
  Env env(argc, argv);
  // ...
}
```

`miso/include/miso/env.hpp` の `miso::Env` は、メンバとして `mpi::Env` を持ちます。
`mpi::Env` のコンストラクタは `MPI_Init` を呼び、デストラクタは `MPI_Finalize` を呼びます（抜粋）。

```cpp
struct Env {
  bool owns_mpi = false;
  // ...
  ~Env() noexcept {
    // ...
    int is_mpi_finalized = false;
    MPI_Finalized(&is_mpi_finalized);
    if (owns_mpi && !is_mpi_finalized) {
      MPI_Finalize();
    }
  }
};
```

`env` は `main()` の最初に作られ、`main()` の終わりで寿命が終わるので、MPI の終了処理を書き忘れることがありません。
`noexcept` は「この関数は例外を外に投げない」という宣言です。
MPI の内容は {ref}`第 8 章 <edu-ch08>` で説明します。

(edu-ch04-copy-move)=
## 4.6 コピーの禁止とムーブ

### コピーで起きる問題と = delete

`int` の変数は `int b = a;` でコピーできます。クラスのオブジェクトも、何も指定しなければコピーでき、そのときはメンバ変数が 1 つずつコピーされます。
前の節の `Buffer` をコピーすると、メンバ `data_` に入っている**アドレスだけ**がコピーされます。
配列そのものは複製されず、2 つのオブジェクトが同じ配列を指します。
すると、2 つのデストラクタが同じ配列を `delete[]` することになり、プログラムが異常終了するなどの誤りになります。

配列を受け持つクラスでは、コピーを禁止します。
特別なメンバ関数の宣言の後に **`= delete`** を書くと、その関数は使えなくなります。

```cpp
class Buffer {
public:
  // ... (the constructor and destructor are the same as before)
  Buffer(const Buffer &) = delete;             // forbids creating by copy
  Buffer &operator=(const Buffer &) = delete;  // forbids copy assignment
};
```

1 行目の `Buffer(const Buffer &)` は、同じ型のオブジェクトから作るコンストラクタで、**コピーコンストラクタ**と呼びます。`Buffer b = a;` や `Buffer b(a);` で呼ばれます。
2 行目の `operator=` は、作った後の `b = a;` で呼ばれる関数です（`operator` の付く名前は {ref}`4.7 節 <edu-ch04-operator>` で説明します）。
どちらも禁止すると、`Buffer b = a;` は `use of deleted function 'Buffer::Buffer(const Buffer&)'` というコンパイルエラーになります。
誤りが実行時ではなくコンパイル時に分かります。

### ムーブ：中身の持ち主を移す

コピーを禁止しても、「配列の持ち主を別のオブジェクトへ移す」ことはしたい場合があります。
関数の中で作った配列を戻り値で返すときなどです。
これを**ムーブ**（move）と呼びます。`Buffer` にムーブのためのコンストラクタを加えます。

```cpp
  Buffer(Buffer &&other) noexcept      // creates by move
      : data_(other.data_), n_(other.n_) {
    other.data_ = nullptr;  // the source object gives up the array
    other.n_ = 0;
  }
```

`Buffer &&` は、「中身を移してよいオブジェクト」を受け取る参照です。
ムーブのコンストラクタは、`other` の配列のアドレスを受け取り、`other` 側を `nullptr` にします。
配列は 1 つのままで、持ち主だけが入れ替わります。
`nullptr` を `delete[]` しても何も起きないので、空になった `other` のデストラクタは何も解放しません。

デストラクタで、配列を持っているときだけ表示するようにして（`if (data_ != nullptr)`）、`size()` を加えたものが次の使用例です。
`std::move` は `#include <utility>` で使えます。

```cpp
int main() {
  Buffer a(10);
  // Buffer b = a;          // error: copying is forbidden
  Buffer c = std::move(a);  // moves a's array to c. No new allocation
  std::printf("a.size() = %d, c.size() = %d\n", a.size(), c.size());
}
```

```text
allocated (10 elements)
a.size() = 0, c.size() = 10
releasing (10 elements)
```

`std::move(a)` は、`a` を「中身を移してよいオブジェクト」として扱う指定で、それ自体は何も移しません。
移すのはムーブのコンストラクタです。
ムーブした後の `a` は空になっているので、配列として使いません。

関数の戻り値として返すときは、`std::move` を書かなくても、ムーブ（または移す処理そのものの省略）が使われます。

### MISO の例：Array3D と Fields

`miso/include/miso/array3d.hpp` の `Array3D` の末尾には、`Buffer` と同じ組み合わせがあります（抜粋。`//` のコメントは元のコードのものです）。

```cpp
  // Prohibit copy semantics
  Array3D(const Array3D &) = delete;
  Array3D &operator=(const Array3D &) = delete;

  // Allow move semantics (allow uninitialized state)
  Array3D(Array3D &&other) noexcept : data_(other.data_), shape_(other.shape_) {
    other.data_ = nullptr;
    other.shape_ = std::array<int, 3>{-1, -1, -1};
  }
```

ムーブした後の `Array3D` は、`data_` が `nullptr`、大きさが `-1` の「作っていない状態」になります。
コメントの `allow uninitialized state` はこの状態のことです。
ムーブの代入 `operator=(Array3D &&other)` も定義されていて、自分が持っていた配列を解放してから `other` の配列を受け取ります。

`miso/include/miso/mhd_fields.hpp` の `Fields` は、MHD の 9 つの変数（密度 `ro`、速度 `vx`, `vy`, `vz`、磁場 `bx`, `by`, `bz`、内部エネルギー `ei`、発散を抑えるための変数 `ph`）を `Array3D` で持つ構造体です。
CPU 用の定義の抜粋です。

```cpp
template <typename Real> struct Fields<Real, backend::Host> {
  Array3D<Real, backend::Host> ro, vx, vy, vz, bx, by, bz, ei, ph;

  Fields(const int i_total, const int j_total, const int k_total)
      : ro(i_total, j_total, k_total), vx(i_total, j_total, k_total),
        // ... (same from vy to ph)
        ph(i_total, j_total, k_total) {}
  // ...
  // Prohibit copy semantics
  Fields(const Fields &) = delete;
  Fields &operator=(const Fields &) = delete;

  // Allow move semantics
  Fields(Fields &&) = default;
  Fields &operator=(Fields &&) = default;
};
```

`Fields` 自身は配列を直接確保していません。メンバの `Array3D` のコンストラクタがそれぞれ確保し、デストラクタがそれぞれ解放します。
そのため `Fields` にはデストラクタを書いていません。
**`= default`** は「この関数を、コンパイラが用意する標準の形で使う」という指定です。
ムーブなら「メンバを 1 つずつムーブする」処理になり、9 つの `Array3D` のムーブがそれぞれ呼ばれます。

demo の中で、関数が `Array3D` を返す例が `demo/mhd2d_reconnection/src/main.cpp` の `make_eta` です。
抵抗率の配列を作って返し、`Model` のメンバ `eta` を初期化します（抜粋）。

```cpp
inline Array3D<Real, Backend> make_eta(const Grid<Real, backend::Host> &grid,
                                       const Parameters &par) {
  // ... (computes values on the CPU and copies them to the Backend array eta)
  return eta;
}

// part of the member initializer list of the Model constructor
      eta(make_eta(grid, par)),
```

`Array3D` はコピーできませんが、戻り値はムーブ（または移す処理の省略）で受け渡されるので、この書き方ができます。

<details>
<summary>コラム：コピー・ムーブ・デストラクタをまとめて考える規則</summary>

デストラクタ、コピーのコンストラクタと代入、ムーブのコンストラクタと代入の 5 つは、どれか 1 つを自分で定義したら、残りも定義するか `= delete`・`= default` で明示する、という指針があります。
C++ Core Guidelines の [C.21](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-five) で「rule of five」として説明されています。
`Array3D` は、デストラクタで解放し、コピーを禁止し、ムーブを定義しているので、この指針どおり 5 つすべてを決めています。
逆に、`Fields` のように資源を直接持たないクラスは、メンバのクラスに後片付けを任せ、自分ではほとんど書かずに済ませられます（同じ文書の [C.20](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-zero)）。

</details>

(edu-ch04-operator)=
## 4.7 演算子のオーバーロードと operator()

MISO では、3 次元配列の要素を `a(i, j, k)` と書きます。
Fortran の `a(i, j, k)` と似ていますが、C++ の `a(i, j, k)` は「オブジェクト `a` の `operator()` という名前のメンバ関数を、引数 `i, j, k` で呼ぶ」ことを意味します。
`+` や `[]` や `()` などの演算子に、クラスごとの意味を定義することを**演算子のオーバーロード**と呼びます。
定義する関数の名前は `operator` と演算子の記号を続けたもので、`operator()` や `operator[]`、前節の `operator=` がその例です。

説明用の 2 次元配列で確かめます。

```cpp
class Array2 {
public:
  Array2(int n0, int n1) : data_(new double[n0 * n1]), n0_(n0), n1_(n1) {}
  ~Array2() { delete[] data_; }
  Array2(const Array2 &) = delete;
  Array2 &operator=(const Array2 &) = delete;

  // writing a(i, j) calls this function
  double &operator()(int i, int j) { return data_[i * n1_ + j]; }

  int extent(int dim) const { return dim == 0 ? n0_ : n1_; }

private:
  double *data_;
  int n0_, n1_;
};
```

```cpp
Array2 a(2, 3);
for (int i = 0; i < a.extent(0); ++i) {
  for (int j = 0; j < a.extent(1); ++j) {
    a(i, j) = 10 * i + j;  // the return value is a reference, so an element can be assigned to
  }
}
std::cout << a(1, 2) << "\n";  // 12
double &e = a(0, 1);           // alias of element (0, 1)
e = -1.0;
std::cout << a(0, 1) << "\n";  // -1
```

`operator()` の戻り値の型 `double &` が重要です。
値（`double`）を返す定義にすると、`a(i, j)` は要素の値をコピーした一時的な値になり、`a(i, j) = ...` はコンパイルエラーになります。
参照を返すので、`a(i, j)` は配列の要素そのものの別名になり、読むことも書くこともできます。
`dim == 0 ? n0_ : n1_` は、`dim == 0` なら `n0_`、そうでなければ `n1_` を値とする式です（条件演算子）。

`data_[i * n1_ + j]` は、2 次元の添字を 1 次元の位置に直す式です。
`j` が 1 増えると隣の要素へ、`i` が 1 増えると `n1_` 個先へ進むので、**最後の添字が連続**しています。
Fortran は最初の添字が連続なので、並びが逆です。並びの違いとループの順番は {ref}`第 7 章 <edu-ch07>` で詳しく説明します。

### MISO の例：Array3D の operator()

`miso/include/miso/array3d.hpp` の CPU 用 `Array3D` の `operator()` と `operator[]` です。

```cpp
  /// @brief Return reference to the element at the given indices.
  T &operator()(int i0, int i1, int i2) const noexcept {
    assert(data_);
    assert(i0 >= 0 && i0 < shape_[0]);
    assert(i1 >= 0 && i1 < shape_[1]);
    assert(i2 >= 0 && i2 < shape_[2]);
    return data_[(i0 * shape_[1] + i1) * shape_[2] + i2];
  }

  /// @brief Return reference to the element at the given linear index.
  T &operator[](int idx) const noexcept {
    assert(data_);
    assert(idx >= 0 && idx < size());
    return data_[idx];
  }
```

- `(i0 * shape_[1] + i1) * shape_[2] + i2` は、`Array2` の式を 3 次元にしたもので、最後の添字 `i2` が連続です。
- `assert(...)` は、条件が成り立たなければプログラムを止める検査です。添字が範囲外なら止まります。ただし `assert` は Debug ビルドでだけ働き、MISO の既定の Release ビルドでは取り除かれます（{ref}`第 1 章 <edu-ch01>`）。
- `operator[]` は、1 次元の位置 `idx` で要素を指定します。`a[n]` と書けます。全要素を同じように処理するときに使います。

`miso/include/miso/config.hpp` の `Config` にも `operator[]` があり、`config["time"]` と書けます。

```cpp
  /// @brief Accessor for YAML configuration object
  const YAML::Node operator[](const std::string &key) const {
    return yaml_obj[key];
  }
```

メンバ `yaml_obj`（yaml-cpp のオブジェクト）から、キー `key` の項目を取り出して返します。
戻り値の `YAML::Node` も `[]` を持つので、`config["time"]["tend"]` と続けて書けます。
末尾の `const` があるので、`const Config &` で受け取った設定（`Time` のコンストラクタの引数など）からも呼べます。

(edu-ch04-view)=
## 4.8 所有する配列と所有しない View

### 所有と View の考え方

`Array2` や `Array3D` は、配列を確保し、寿命が終わると解放します。
このように、**資源の後片付けに責任を持つこと**を「**所有する**」と言います。
所有するオブジェクトはコピーを禁止しているので、関数に渡すときは参照で渡します。

一方、計算の関数に必要なのは、配列の先頭アドレスと大きさだけです。
そこで、**先頭アドレスと大きさだけを持ち、確保も解放もしない**軽いクラスを用意し、関数にはそれを渡します。
これを **View**（ビュー）と呼びます。

```cpp
// non-owning View: holds only the address of the first element and the size
struct Array2View {
  double *data;
  int n0, n1;
  double &operator()(int i, int j) const { return data[i * n1 + j]; }
};

// owning array: handles allocation and release
class Array2 {
public:
  // ... (the constructor, destructor, and copy prohibition are the same as before)
  Array2View view() { return Array2View{data_, n0_, n1_}; }
  // ...
};

// takes the View by value. Only the address and the size are copied
void fill(Array2View v, double value) {
  for (int i = 0; i < v.n0; ++i) {
    for (int j = 0; j < v.n1; ++j) {
      v(i, j) = value;  // modifies elements of the original Array2
    }
  }
}
```

```cpp
Array2 a(2, 3);
fill(a.view(), 7.0);
Array2View v = a.view();
std::cout << v(1, 2) << " " << sizeof(Array2View) << "\n";  // 7 16
```

`fill` は `Array2View` を値で受け取るので、関数の中の `v` はコピーです。
コピーしたのはアドレスと大きさだけなので、`v(i, j)` は元の `a` の要素を指し、`a` が書き換わります。
`sizeof(Array2View)` は、このオブジェクトの大きさ（バイト数）です。この確認では 16 バイト（アドレス 8 バイトと `int` 2 つ）で、配列の要素数には依存しません。

View は配列を所有しないので、**持ち主より長く使ってはいけません**。
持ち主の `Array2` の寿命が終わった後に View を使うと、解放済みのメモリを指すことになります（{ref}`第 3 章 <edu-ch03>` のぶら下がり参照と同じ問題です）。

### MISO の例：Array3DView

`miso/include/miso/array3d.hpp` の `Array3DView` の抜粋です。

```cpp
/// @brief Lightweight non-owning view of 3D array data.
template <typename T> class Array3DView {
private:
  T *data_ = nullptr;
  int shape_[3] = {-1, -1, -1};

  // Always constructed from Array3D
  // to ensure that the array size is within the numeric limits of int.
  template <typename, typename> friend class Array3D;
  Array3DView(T *data, int nx0, int nx1, int nx2) noexcept
      : data_(data), shape_{nx0, nx1, nx2} {}

public:
  // ... (data(), extent(), size(), operator[])
  __host__ __device__ T &operator()(int i0, int i1, int i2) const noexcept {
    // ... (check with assert)
    return data_[(i0 * shape_[1] + i1) * shape_[2] + i2];
  }

  // Allow copy semantics
  __host__ __device__ Array3DView(const Array3DView &) noexcept = default;
  __host__ __device__ Array3DView &
  operator=(const Array3DView &) noexcept = default;
};
```

- 持っているのは先頭アドレス `data_` と大きさ `shape_` だけです。デストラクタで解放しません。
- コピーを `= default` で許しています。`Array3D` がコピーを禁止しているのと対照的です。View は安くコピーして関数に渡すためのものです。
- コンストラクタは `private` です。`friend class Array3D` は「`Array3D` には private のメンバの使用を許す」という指定で、View を作れるのは `Array3D` の `view()` だけになります。元のコメントにあるとおり、大きさの検査を済ませた `Array3D` からだけ作らせるためです。
- `__host__ __device__` は、この関数を GPU でも使えるようにする印です。CPU だけのビルドでは `miso/include/miso/cuda_compat.hpp` が空の印として定義するので、読むときは無視できます（{ref}`第 9 章 <edu-ch09>`）。

`Array3D` 側には、View を返す関数が 3 つあります（抜粋）。

```cpp
  /// @brief Return a lightweight view of the array.
  Array3DView<T> view() noexcept { /* ... */ }

  /// @brief Return a constant lightweight view of the array.
  Array3DView<const T> view() const noexcept { /* ... */ }

  /// @brief Return a constant lightweight view of the array.
  Array3DView<const T> const_view() const noexcept { return view(); }
```

`Array3DView<T>` は要素を書き換えられる View、`Array3DView<const T>` は要素を読むだけの View です。
要素の型に `const` が付いているので、`operator()` が返す参照も `const T &` になり、代入するとコンパイルエラーになります。
1 つ目と 2 つ目は名前が同じで、末尾の `const` だけが違います。`const` でない `Array3D` からは 1 つ目、`const` の `Array3D`（や `const` 参照）からは 2 つ目が呼ばれます。
`const_view()` は、`const` でない配列からでも、読むだけの View を明示して取り出すための関数です。

### 動かして確かめる：Array3D と Array3DView

MISO のヘッダだけを使う短いプログラムで確かめます。
`miso/include/miso/array3d.hpp` は MPI や yaml-cpp を使わないので、インクルードパスを指定するだけでビルドできます。

```cpp
#include <iostream>
#include <utility>

#include <miso/array3d.hpp>

using Real = float;

// takes a writable View by value and modifies elements of the original array
void fill(miso::Array3DView<Real> a) {
  for (int i = 0; i < a.extent(0); ++i) {
    for (int j = 0; j < a.extent(1); ++j) {
      for (int k = 0; k < a.extent(2); ++k) {
        a(i, j, k) = 100 * i + 10 * j + k;
      }
    }
  }
}

// takes a read-only View and returns the sum
Real total(miso::Array3DView<const Real> a) {
  Real s = 0;
  for (int n = 0; n < a.size(); ++n) {
    s += a[n];
    // a[n] = 0;  // error: cannot assign to an element of const Real
  }
  return s;
}

int main() {
  miso::Array3D<Real> a(2, 3, 4);  // allocates a 2 x 3 x 4 array
  fill(a.view());
  std::cout << a(1, 2, 3) << " " << a[23] << "\n";  // the same element
  std::cout << total(a.const_view()) << "\n";

  // miso::Array3D<Real> b = a;          // error: copying is forbidden
  miso::Array3D<Real> b = std::move(a);  // moves ownership of the array from a to b
  std::cout << a.extent(0) << " " << b.extent(0) << "\n";
}
```

```bash
g++ -std=c++17 -I/path/to/MISO/miso/include check_array3d.cpp -o check_array3d
./check_array3d
```

```text
123 123
1476
-1 2
```

- `miso::Array3D<Real>` のように 2 つ目の型の指定を省略すると、`backend::Host`（CPU）になります。
- `a(1, 2, 3)` は 1 次元の位置で `(1 * 3 + 2) * 4 + 3 = 23` 番目なので、`a[23]` と同じ要素です。
- ムーブした後の `a` は大きさ `-1` の「作っていない状態」で、`b` が 2 x 3 x 4 の配列を持っています。

`Array3D` の `operator()` は末尾に `const` が付いていても `T &` を返します。
そのため、`const Array3D<Real> &` で受け取った配列からでも、`a(i, j, k) = ...` で要素を書き換えられます（試すとコンパイルが通ります）。
`Array3D` 側の `const` は「配列の大きさや持ち主を変えない」ことまでしか保証しません。
**関数が配列を読むだけであることを型で示したいときは、`const_view()` で作った `Array3DView<const Real>` を渡します。**

### MISO の例：Fields と FieldsView

`miso/include/miso/mhd_fields.hpp` では、9 つの変数をまとめた `Fields` にも、対応する View `FieldsView` があります（抜粋）。

```cpp
template <typename T>  // Use T to represent Real or const Real
struct FieldsView {
  Array3DView<T> ro, vx, vy, vz, bx, by, bz, ei, ph;
  // ... (explicit constructor taking 9 Views, extent(), size())
};
```

`Fields` の `view()` は、9 つの `Array3D` の `view()` をまとめて `FieldsView<Real>` を作り、`const_view()` は `FieldsView<const Real>` を作ります。
元のコメントのとおり、`T` には `Real`（書き換えられる）か `const Real`（読むだけ）が入ります。

`FieldsView` を使う側が、demo の初期条件です。
`miso/include/miso/mhd_solver.hpp` の `MHD::apply_initial_condition` は、CPU 上に作業用の `Fields` を作り、その View を初期条件の `apply` に渡します（抜粋）。

```cpp
    Fields<Real, backend::Host> qq_h(grid.i_total, grid.j_total, grid.k_total);
    ic.apply(qq_h.view(), grid_h.const_view());
    qq.copy_from(qq_h);
```

`qq_h` が配列を所有し、`apply` には View を値で渡しています。
`apply` の中で View を通して値を書き込むと、`qq_h` の配列が書き換わります。
その後、`copy_from` で計算に使う `qq` へ写します（GPU で計算するときは、ここで GPU のメモリへ転送されます）。
`grid_h.const_view()` は格子の座標などを読むだけの View `GridView<const Real>` です。

(edu-ch04-initial-condition)=
## 4.9 demo の InitialCondition を読む

この章で学んだことを使って、`demo/hd1d_shock_tube/src/main.cpp` の `InitialCondition` を読みます。
衝撃波管問題の左右の状態（密度・圧力・速度）を設定ファイルから読み、初期条件を与える構造体です（`apply` の中の一部を省略しています）。

```cpp
struct InitialCondition {
  eos::IdealEOS<Real> &eos;
  Real rol, prl, vvl;
  Real ror, prr, vvr;

  explicit InitialCondition(Config &config, eos::IdealEOS<Real> &eos)
      : eos(eos), rol(config["shock_tube"]["rol"].as<Real>()),
        prl(config["shock_tube"]["prl"].as<Real>()),
        vvl(config["shock_tube"]["vvl"].as<Real>()),
        ror(config["shock_tube"]["ror"].as<Real>()),
        prr(config["shock_tube"]["prr"].as<Real>()),
        vvr(config["shock_tube"]["vvr"].as<Real>()) {}

  // The signature must not be changed as it is called inside miso::mhd::MHD.
  void apply(mhd::FieldsView<Real> qq, GridView<const Real> grid) const {

    for (int k = 0; k < grid.k_total; ++k) {
      for (int j = 0; j < grid.j_total; ++j) {
        for (int i = 0; i < grid.i_total; ++i) {
          // ... (sets velocity, magnetic field, etc. to 0 and selects the coordinates xyz)
          if (xyz < 0.5) {
            qq.ro(i, j, k) = rol;
            qq.ei(i, j, k) = prl / (eos.gm - 1.0) / qq.ro(i, j, k);
            qq.vx(i, j, k) = vvl;
          } else {
            // ... (right-hand state)
          }
        }
      }
    }
  }
};
```

| 部分 | 読み方 |
|---|---|
| `struct InitialCondition` | 公開範囲を書かないので、メンバはすべて public です |
| `eos::IdealEOS<Real> &eos;` | 状態方程式を**参照**で持つメンバです。コピーを持たず、`Model` が持つ `eos` を参照します |
| `Real rol, prl, vvl;` など | 左右の密度・圧力・速度を値で持つメンバです |
| `explicit InitialCondition(Config &config, eos::IdealEOS<Real> &eos)` | 設定と状態方程式を受け取るコンストラクタです |
| `: eos(eos), rol(...), ...` | 参照メンバ `eos` は、初期化子リストでなければ参照先を決められません。他のメンバは宣言順に設定ファイルの値で初期化します |
| `void apply(...) const` | 初期条件を書き込む関数です。末尾の `const` は、`InitialCondition` 自身のメンバ（`rol` など）を書き換えないという意味です |
| `mhd::FieldsView<Real> qq` | 書き込める View を値で受け取ります。`qq.ro(i, j, k) = rol;` で、呼び出し元の `Fields` の配列が書き換わります |
| `GridView<const Real> grid` | 格子を読むだけの View です。`grid.i_total` はゴーストセルを含む格子数、`grid.x[i]` はセル中心の座標です |
| `eos.gm` | 参照している状態方程式の比熱比です |

`apply` の末尾の `const` と、`qq` を通した書き込みは矛盾しません。
`const` が禁止するのは `InitialCondition` のメンバ変数の変更で、`qq` は引数として受け取った View だからです。
`apply` の引数の形が決まっている理由は、MISO の内部（前の節の `MHD::apply_initial_condition`）がこの形で呼ぶからです。元のコメントもそのことを述べています。

この構造体は、demo の `Model` のメンバ `ic` として作られ、MISO の時間発展の始めに使われます。
`Model` の読み方は {ref}`第 5 章 <edu-ch05>` で説明します。

(edu-ch04-exercises)=
## 練習

### 練習 1：Time のメンバが値を得る場所

`miso/include/miso/time.hpp` の `Time` のコンストラクタを読み、8 つのメンバ変数（`time`, `tend`, `dt_output`, `n_step`, `n_output`, `n_output_digits`, `time_save_dir`, `io_enabled`）が、それぞれ (a) 初期化子リスト、(b) コンストラクタの本体（本体から呼ぶ関数を含む）のどちらで値を得るかを分類してください。

<details>
<summary>解答・解説</summary>

| メンバ | 値を得る場所 |
|---|---|
| `tend`, `dt_output`, `n_output_digits`, `io_enabled` | (a) 初期化子リスト。設定ファイルの値で初期化します |
| `time`, `n_step`, `n_output` | (b) 本体から呼ぶ `initialize()` で 0 を代入します |
| `time_save_dir` | (b) 本体で、`config.save_dir` と設定ファイルの `io.time_save_dir` をつないだパスを代入します |

初期化子リストのメンバは、宣言順と同じ順に並んでいます。
本体で代入するメンバは、本体に入る前に一度、既定の方法で作られています（`Real` や `int` は値が不定、`std::string` は空の文字列）。
`tend` と `dt_output` は、本体の最初の検査で使うので、初期化子リストで値を決めておく必要があります。

</details>

### 練習 2：Array3D の要素の位置

`miso::Array3D<float> a(2, 3, 4);` について、次を求めてください。

1. `a(1, 0, 2)` は、配列の先頭から何番目の要素か（`a[n]` の `n`）。
2. `a(0, 0, 1)` と `a(0, 0, 0)`、`a(0, 1, 0)` と `a(0, 0, 0)`、`a(1, 0, 0)` と `a(0, 0, 0)` が、それぞれ何要素離れているかを答えてください。

<details>
<summary>解答・解説</summary>

`operator()` の式 `(i0 * shape_[1] + i1) * shape_[2] + i2` に、`shape_ = {2, 3, 4}` を入れます。

1. `(1 * 3 + 0) * 4 + 2 = 14` なので、`a[14]` です。
2. 最後の添字が 1 増えると 1 要素、真ん中の添字が 1 増えると 4 要素（`shape_[2]`）、最初の添字が 1 増えると 12 要素（`shape_[1] * shape_[2]`）離れます。

最後の添字 `i2` が連続です。Fortran の `a(2, 3, 4)` では最初の添字が連続なので、逆になります。
ループの順番との関係は {ref}`第 7 章 <edu-ch07>` で扱います。

</details>

### 練習 3：配列を受け取る関数の引数の型

`Array3D<Real>` の要素から最大値を求める関数 `max_value` と、すべての要素を 0 にする関数 `clear` を作るとします。
次の候補のうち、それぞれの関数の引数にふさわしい型を選び、理由を説明してください。

- (a) `Array3D<Real> a`
- (b) `const Array3D<Real> &a`
- (c) `Array3DView<Real> a`
- (d) `Array3DView<const Real> a`

<details>
<summary>解答・解説</summary>

- `max_value` には (d) `Array3DView<const Real>` がふさわしいです。読むだけであることが型に表れ、関数の中で誤って代入するとコンパイルエラーになります。呼ぶ側は `max_value(a.const_view())` と書きます。
- `clear` には (c) `Array3DView<Real>` がふさわしいです。View を値で渡しても、指す先は元の配列なので、元の要素を 0 にできます。呼ぶ側は `clear(a.view())` と書きます。

(a) は、`Array3D` のコピーを作ろうとするので、コピーが禁止されていてコンパイルエラーになります。
(b) はコンパイルでき、コピーもしません。ただし、`Array3D` の `operator()` は末尾が `const` でも `T &` を返すので、`const Array3D<Real> &` からでも要素を書き換えられます。読むだけであることを型で保証できない点で (d) に劣ります。
また、MISO の `for_each` などの並列ループの中では View を使います（{ref}`第 7 章 <edu-ch07>`）。関数の引数を View にしておくと、その書き方とそろいます。

</details>
