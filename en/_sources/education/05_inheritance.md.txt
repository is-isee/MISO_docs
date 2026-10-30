(edu-ch05)=
# 5. 継承と CRTP

MISO の demo の `main.cpp` には、次の形の定義が必ずあります（`demo/mhd2d_vortex/src/main.cpp`）。

```cpp
struct Model : public mhd::ModelBase<Model, Real, Backend> {
  eos::IdealEOS<Real> eos;
  InitialCondition ic;
  mhd::EmptyBoundaryCondition<Real> bc;
  mhd::EmptySourceTerm<Real> src;

  Model(Config &config) : ModelBase(config), eos(config), ic(eos), bc(), src() {}
};
```

時間発展のループ、出力、MHD の計算は MISO 本体の `ModelBase` が受け持ち、問題ごとに違う部品（状態方程式 `eos`、初期条件 `ic`、境界条件 `bc`、ソース項 `src`）は demo の `Model` が与えます。
この章では、この 1 行目を読めるようになることを目標に、**継承**の基本、比較のための**仮想関数**、MISO が使っている **CRTP** の順に学びます。

- {ref}`edu-ch05-inheritance`
- {ref}`edu-ch05-hiding`
- {ref}`edu-ch05-virtual`
- {ref}`edu-ch05-crtp`
- {ref}`edu-ch05-modelbase`
- {ref}`edu-ch05-model`
- {ref}`edu-ch05-update`
- {ref}`edu-ch05-exercises`

この章でも、山括弧 `<>` は「型を後から指定する印」として読みます（{ref}`第 6 章 <edu-ch06>`）。

(edu-ch05-inheritance)=
## 5.1 継承の基本

### 基底クラスと派生クラス

時刻とステップ数を管理するクラスを用意し、それをもとに、常微分方程式 $du/dt = -u$ を解くクラスを作ります。

```cpp
class Simulation {
public:
  explicit Simulation(double tend) : tend(tend) {}

  double time = 0.0;  // current time
  double tend;        // end time
  int n_step = 0;     // number of steps advanced

protected:
  void advance(double dt) {  // advances the time and the step count
    time += dt;
    ++n_step;
  }
};

class Decay : public Simulation {
public:
  double u;  // member added in Decay

  Decay(double tend, double u0) : Simulation(tend), u(u0) {}

  void step(double dt) {
    u -= dt * u;   // advances du/dt = -u by one step with the Euler method
    advance(dt);   // can call the protected function of the base class
  }
};
```

`class Decay : public Simulation` は、「`Simulation` のメンバをすべて引き継いだうえで、`Decay` を定義する」という宣言です。
この関係を**継承**（inheritance）と呼び、もとになる `Simulation` を**基底クラス**（base class）、新しく作る `Decay` を**派生クラス**（derived class）と呼びます。
`Decay` のオブジェクトは、`Simulation` の部分（`time`, `tend`, `n_step`）と、`Decay` で加えた部分（`u`）を両方持ちます。

```cpp
Decay d(1.0, 1.0);
d.step(0.125);
d.step(0.125);
std::cout << d.time << " " << d.n_step << " " << d.u << "\n";  // 0.25 2 0.765625

Simulation &s = d;  // refers to the Simulation part of d
std::cout << s.time << " " << s.tend << "\n";                  // 0.25 1
// s.u = 2.0;       // error: Simulation has no u
```

`d.time` のように、引き継いだメンバは `Decay` のメンバと同じように使えます。
`public` を付けて継承すると、派生クラスのオブジェクトを基底クラスの参照（`Simulation &`）やポインタで扱えます。
`s` を通して見えるのは `Simulation` のメンバだけです。

継承の `public` は、基底クラスの public なメンバを、派生クラスでも public のままにする指定です。
`struct` で定義した派生クラスでは、書かなければ `public` の継承になりますが、MISO の demo のように `public` と明示するのが普通です。

### protected

`advance` は `protected:` の後に書いています。
**`protected` は、外からは使えず、そのクラスと派生クラスのメンバ関数からは使える**という公開範囲です。
`Decay::step` の中から `advance(dt)` を呼べますが、`main()` から `d.advance(0.1);` と書くと `'void Simulation::advance(double)' is protected within this context` というエラーになります。
`private` のメンバは、派生クラスからも使えません。

### 基底クラスのコンストラクタの呼び出し

`Decay` のコンストラクタの初期化子リストの先頭に、`Simulation(tend)` と書いています。
これは**基底クラスの部分を、基底クラスのコンストラクタで初期化する**指定です。
`Simulation` には引数なしのコンストラクタがないので、この指定を省略するとエラーになります。

初期化は、次の順に行われます。

1. 基底クラスの部分（`Simulation(tend)`。その中で、`Simulation` のメンバが宣言順に初期化されます）
2. 派生クラスで加えたメンバ（宣言順。ここでは `u`）
3. 派生クラスのコンストラクタの本体

基底クラスは、初期化子リストのどこに書いても最初に初期化されます。
後片付けは逆の順で、派生クラスのデストラクタが先、基底クラスのデストラクタが後に呼ばれます。

`Simulation(double tend) : tend(tend)` は、メンバ `tend` を、同じ名前の引数 `tend` で初期化する書き方です（{ref}`第 4 章 <edu-ch04>` の `IdealEOS` の `gm(gm)` と同じです）。

### MISO の例：Model の継承とメンバの初期化

章の冒頭の `Model` を、この節の言葉で読みます。

- `struct Model : public mhd::ModelBase<Model, Real, Backend>` の基底クラスは `mhd::ModelBase<Model, Real, Backend>` です。山括弧の中に `Model` 自身が入っている理由は {ref}`5.4 節 <edu-ch05-crtp>` で説明します。
- `Model` は、`ModelBase` のメンバ（`config`, `mpi_shape`, `time`, `grid`, `exec_ctx`, `mhd`）を引き継ぎ、`eos`, `ic`, `bc`, `src` を加えます。
- 初期化子リストの `ModelBase(config)` が、基底クラスのコンストラクタの呼び出しです。`Model` の中では、基底クラスを山括弧なしの `ModelBase` という名前で書けます。

初期化の順番から、次のことが分かります。
`demo/hd1d_shock_tube/src/main.cpp` の `Model` のコンストラクタは、次のとおりです。

```cpp
  Model(Config &config)
      : ModelBase(config), eos(config), ic(config, eos), bc(mpi_shape), src() {}
```

`bc(mpi_shape)` の `mpi_shape` は、基底クラス `ModelBase` のメンバです。
基底クラスの部分は最初に初期化されるので、`bc` を作る時点で `mpi_shape` はすでにできています。
同じように、`ic(config, eos)` に渡す `eos` は、`ic` より前に宣言されたメンバなので、`ic` より先にできています。
{ref}`第 4 章 <edu-ch04-initial-condition>` で読んだとおり、`InitialCondition` は `eos` を参照で持つので、この順番が意味を持ちます。

(edu-ch05-hiding)=
## 5.2 同じ名前の関数と基底クラスの関数の呼び出し

基底クラスに、時間発展のループ `run()` と、既定の 1 ステップ `update()` を置きます。
派生クラスでは、同じ名前の `update()` を定義して、自分の計算に置き換えたいとします。

```cpp
class Simulation {
public:
  explicit Simulation(double tend) : tend(tend) {}

  double time = 0.0;
  double tend;
  int n_step = 0;

  void update() { advance(0.25); }  // default single step: advances only the time

  void run() {
    while (time < tend) {
      update();  // note which update() is called
    }
  }

protected:
  void advance(double dt) {
    time += dt;
    ++n_step;
  }
};

class Decay : public Simulation {
public:
  double u;

  Decay(double tend, double u0) : Simulation(tend), u(u0) {}

  void update() {  // defines a function with the same name as in the base class
    const double dt = 0.125;
    u -= dt * u;
    advance(dt);
  }
};
```

```cpp
Decay a(1.0, 1.0);
a.update();              // Decay::update() is called
a.Simulation::update();  // calls the base class update() by name
std::cout << a.time << " " << a.n_step << " " << a.u << "\n";  // 0.375 2 0.875

Decay b(1.0, 1.0);
b.run();                 // update() inside run() is Simulation::update()
std::cout << b.time << " " << b.n_step << " " << b.u << "\n";  // 1 4 1
```

1 つ目の結果から、次のことが分かります。

- `a.update()` は `Decay::update()` を呼びます。派生クラスで同じ名前の関数を定義すると、派生クラスのオブジェクトから呼んだときには、派生クラスの関数が使われます。基底クラスの同じ名前の関数は隠れます（**名前の隠蔽**）。
- 隠れた基底クラスの関数も、`Simulation::update()` のように `クラス名::` を付けると呼べます。派生クラスのメンバ関数の中から、基底クラスの処理を呼んでから処理を足す、という使い方ができます（{ref}`5.7 節 <edu-ch05-update>`）。

2 つ目の結果は、期待と違います。
`b.run()` は 4 ステップで終わり、`u` は 1 のままです。`Decay::update()` は一度も呼ばれていません。

理由は、`run()` が `Simulation` のメンバ関数だからです。
`Simulation::run()` を書いた時点で、コンパイラが知っている `update()` は `Simulation::update()` だけです。
`run()` の中の `update();` は `this->update();` のことで（{ref}`第 4 章 <edu-ch04-const-member>`）、`this` の型は `Simulation *` なので、`Simulation::update()` が呼ばれます。
`Simulation` は、自分を継承するクラスがあることも、そのクラスに `update()` があることも知りません。

**基底クラスに書いた共通の手順（`run()`）から、派生クラスの関数（`Decay::update()`）を呼ぶ**には、別の仕組みが必要です。
C++ には 2 つの方法があります。実行時に切り替える**仮想関数**と、コンパイル時に決める **CRTP** です。
MISO は CRTP を使っています。比較のため、先に仮想関数を短く説明します。

(edu-ch05-virtual)=
## 5.3 仮想関数による実行時の切り替え（比較のため）

前の節の `Simulation` と `Decay` を、次の 3 か所だけ変えます。

```cpp
class Simulation {
public:
  // ...
  virtual void update() { advance(0.25); }  // add virtual
  virtual ~Simulation() = default;          // add a virtual destructor
  // ... (run() and advance() are the same)
};

class Decay : public Simulation {
public:
  // ...
  void update() override {  // add override
    // ... (the body is the same)
  }
};
```

すると、`b.run()` の結果は `1 8 0.343609` になり、`Decay::update()` が 8 回呼ばれます（$0.875^8 \approx 0.3436$）。

`virtual` を付けたメンバ関数を**仮想関数**（virtual function）と呼びます。
仮想関数を呼ぶと、ポインタや参照の宣言上の型（ここでは `Simulation *`）ではなく、**実際のオブジェクトの型**（ここでは `Decay`）に合った関数が、実行時に選ばれます。
派生クラスで仮想関数を定義し直すことを**オーバーライド**（override）と呼び、末尾の `override` は、基底クラスに対応する仮想関数があることをコンパイラに確かめさせる指定です。
基底クラスを通して派生クラスのオブジェクトを片付ける場合に備えて、デストラクタにも `virtual` を付けます。

実行時の選択のために、仮想関数を持つクラスのオブジェクトには、「どの関数を呼ぶかの表」を指す隠れたポインタが加わり、呼び出しのたびにその表を引きます（多くのコンパイラの実装方法です）。
この仕組みのおかげで、たとえば設定ファイルを読んでから、どの派生クラスのオブジェクトを作るかを実行時に決められます。

**MISO では仮想関数を使っていません。**
`miso/include/miso/` と `demo/` のソースに `virtual` は出てきません。
MISO で必要なのは「demo ごとに、コンパイルするときに決まっている部品を、共通の手順に組み込む」ことで、実行時の切り替えは必要ありません。
そのうえで、MISO の部品の使われ方には、仮想関数と相性の悪い点があります。

- `ModelBase` は、派生クラスの**メンバ関数**だけでなく、**メンバ変数** `eos`, `ic`, `bc`, `src` を使います。これらの型は demo ごとに違い（`IdealEOS<Real>`、`InitialCondition`、`EmptyBoundaryCondition<Real>` など）、共通の基底クラスを持ちません。仮想関数で書くなら、部品ごとに共通の基底クラスを用意する必要があります。
- ソース項などは、GPU で動く関数（{ref}`第 9 章 <edu-ch09>`）の中から使われ、そのために CPU 側で作ったオブジェクトが GPU へコピーされます。CUDA では、仮想関数を持つクラスのオブジェクトを CPU と GPU の間でコピーすること（GPU の関数の引数として渡すことを含む）は未定義の動作とされています（[CUDA Programming Guide の Polymorphic Classes](https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/cpp-language-support.html#polymorphic-classes)）。

次の節の CRTP は、これらを「コンパイル時に型を決める」ことで解決します。

(edu-ch05-crtp)=
## 5.4 CRTP：基底クラスに派生クラスの型を渡す

### 仕組み

5.2 節の問題は、基底クラス `Simulation` が派生クラス `Decay` を知らないことでした。
それなら、**派生クラスの型を基底クラスに教えれば**、基底クラスの中から派生クラスの関数を呼べます。
型を後から渡す仕組みには、テンプレート（{ref}`第 6 章 <edu-ch06>`）を使います。

```cpp
template <class Derived>  // Derived is the type of the derived class, specified later
class Simulation {
public:
  explicit Simulation(double tend) : tend(tend) {}

  double time = 0.0;
  double tend;
  int n_step = 0;

  void update() { advance(0.25); }  // default single step

  void run() {
    while (time < tend) {
      derived().update();  // calls the derived class's update()
    }
  }

protected:
  void advance(double dt) {
    time += dt;
    ++n_step;
  }

  // treats *this as a reference to the derived class
  Derived &derived() { return static_cast<Derived &>(*this); }
};

class Decay : public Simulation<Decay> {  // passes its own type to the base
public:
  double u;

  Decay(double tend, double u0) : Simulation(tend), u(u0) {}

  void update() {
    const double dt = 0.125;
    u -= dt * u;
    advance(dt);
  }
};

class Clock : public Simulation<Clock> {  // does not define update()
public:
  explicit Clock(double tend) : Simulation(tend) {}
};
```

```cpp
Decay d(1.0, 1.0);
d.run();  // Decay::update() is called 8 times
std::cout << d.time << " " << d.n_step << " " << d.u << "\n";  // 1 8 0.343609

Clock c(1.0);
c.run();  // the base update() is called 4 times
std::cout << c.time << " " << c.n_step << "\n";                // 1 4
```

`virtual` を使わずに、仮想関数のときと同じ結果になりました。
順に読みます。

1. `template <class Derived>` は、`Simulation` が型を 1 つ後から受け取ることを表す印です。受け取った型を、定義の中では `Derived` という名前で使います。
2. `class Decay : public Simulation<Decay>` で、`Derived` に `Decay` 自身を渡しています。基底クラスの山括弧の中に派生クラス自身を書くこの形を、**CRTP**（Curiously Recurring Template Pattern、奇妙に再帰したテンプレートの形）と呼びます。
3. `Simulation<Decay>` の中では、`Derived` は `Decay` です。`derived()` は `Decay &` を返し、`derived().update()` は `Decay::update()` を呼びます。
4. `Clock` は `update()` を定義していません。`derived().update()` は `Clock` のメンバとして `update()` を探し、`Clock` になければ基底クラスの `Simulation<Clock>::update()` を見つけます。派生クラスで定義しなければ既定の処理、定義すれば派生クラスの処理になります。

### static_cast<Derived &>(*this)

`derived()` の中身を読みます。

- `*this` は、`run()` を呼んだオブジェクトです。`run()` は `Simulation<Decay>` のメンバ関数なので、`*this` の型は `Simulation<Decay>` です。
- しかし、`Simulation<Decay>` の部分は、実際には `Decay` のオブジェクトの一部です（`Decay` だけがこれを継承するように使います）。
- `static_cast<Decay &>(*this)` は、「この `Simulation<Decay>` を、それを含む `Decay` の参照として扱う」という変換です。`static_cast<型>(式)` は、コンパイル時に型を変換する書き方で、{ref}`第 4 章 <edu-ch04-destructor>` で見た `static_cast<size_t>(...)` と同じ記法です。

この変換は、オブジェクトが本当に `Decay` であるときだけ正しく働きます。
コンパイラは実行時に確かめません。CRTP では、`class Decay : public Simulation<Decay>` のように自分自身の型を渡す約束を守ることで、これが常に成り立つようにします。

### 仮想関数と CRTP の比較

| 比べる点 | 仮想関数 | CRTP |
|---|---|---|
| 呼ぶ関数が決まる時点 | 実行時 | コンパイル時 |
| 派生クラスの選び方 | 実行時に（設定を読んでから）選べます | ソースに書いた型で決まります |
| 基底クラスから使えるもの | 基底クラスで宣言した仮想関数 | 派生クラスのメンバ関数とメンバ変数（型も含めて） |
| 基底クラスの型 | 1 つの共通の型 `Simulation` | 派生クラスごとに別の型 `Simulation<Decay>`, `Simulation<Clock>` |
| MISO での使用 | 使っていません | `ModelBase` で使っています |

CRTP では、呼ぶ関数がコンパイル時に決まるので、表を引く手間がなく、コンパイラが関数を呼び出し元に展開する（インライン化する）こともできます。
派生クラスのメンバが足りなければ、実行する前にコンパイルエラーになります。
その代わり、`Simulation<Decay>` と `Simulation<Clock>` は別の型なので、「どちらかを入れておく 1 つの変数」は作れません。
MISO の demo は、1 つの実行ファイルで 1 つの問題を解くので、この制限は問題になりません。

(edu-ch05-modelbase)=
## 5.5 MISO の ModelBase

`miso/include/miso/mhd_model_base.hpp` の `ModelBase` の抜粋です。
`update()` は {ref}`5.7 節 <edu-ch05-update>` で読みます。保存・読み込みの関数は省略しています。

```cpp
template <class Derived, class Real, class Backend> class ModelBase {
public:
  Config &config;
  mpi::Shape mpi_shape;
  Time<Real> time;
  Grid<Real, backend::Host> grid;

  ExecContext<Real, Backend> exec_ctx;
  MHD<Real, Backend> mhd;

  explicit ModelBase(Config &cfg)
      : config(cfg), mpi_shape(cfg), time(cfg), grid(cfg, mpi_shape),
        exec_ctx(mpi_shape, grid), mhd(cfg, grid, exec_ctx) {}

  // ... (update(), save_metadata(), save_state(), load_state(), save_if_needed())

  void run() {
    // Ensure to call methods in the derived class
    auto &d = derived();

    // Assert that d has eos, ic, bc, and src as member variables.
    static_assert(has_eos<Derived>::value,
                  "Derived must have member variable 'eos'.");
    // ... (the same check for ic, bc, and src)

    mhd.apply_initial_condition(d.ic, d.bc);
    if (config["io"]["continue"].template as<bool>() &&
        fs::exists(time.n_output_filepath())) {
      load_state();
    }

    save_metadata();
    save_if_needed();

    while (time.time < time.tend) {
      d.update();
      save_if_needed();
    }
  }

protected:
  Derived &derived() { return static_cast<Derived &>(*this); }
};
```

### テンプレート引数とメンバ

- 山括弧の中は 3 つです。`Derived` が CRTP の派生クラス（demo の `Model`）、`Real` が実数の型、`Backend` が計算する場所（CPU か GPU）です。
- メンバは、設定 `config`（参照）、MPI の領域分割 `mpi_shape`、時刻 `time`（{ref}`第 4 章 <edu-ch04-struct>` の `Time`）、格子 `grid`、計算の実行に使う情報 `exec_ctx`、MHD の変数と計算 `mhd` です。
- コンストラクタの初期化子リストは宣言順に並んでいて、後のメンバの初期化に前のメンバを使っています（`grid(cfg, mpi_shape)` など）。

### run() の流れ

`run()` は、MISO の計算全体の手順です。

1. `auto &d = derived();` で、派生クラス `Model` の参照 `d` を得ます。`auto &` は、型を右辺から決める参照の宣言です（{ref}`第 2 章 <edu-ch02>` の `auto`）。
2. `static_assert(...)` で、`Model` がメンバ変数 `eos`, `ic`, `bc`, `src` を持つことを、コンパイル時に確かめます。仕組みは {ref}`第 6 章 <edu-ch06>` で説明します。
3. `mhd.apply_initial_condition(d.ic, d.bc)` で、派生クラスのメンバ `ic` と `bc` を使って初期条件を与えます。中で `ic.apply(...)` が呼ばれることは {ref}`第 4 章 <edu-ch04-view>` で読みました。
4. 設定の `io.continue` が真で、前の計算の出力があれば、続きから計算するために読み込みます。
5. 時刻が終了時刻に達するまで、`d.update()` で 1 ステップ進め、必要なら出力します。

5.4 節の `Simulation<Derived>::run()` と同じ形です。
`d.update()` は、`Model` が `update()` を定義していれば `Model::update()`、していなければ `ModelBase::update()` を呼びます。
`d.ic` や `d.bc` のように、基底クラスから派生クラスの**メンバ変数**を、その実際の型のまま使っている点が、仮想関数では書きにくいところです。

`if (...)` の中の `.template as<bool>()` の `template` は、テンプレートの中で関数テンプレートを呼ぶときに必要になる指定です。読むときは `.as<bool>()` と同じと考えてかまいません（{ref}`第 6 章 <edu-ch06>`）。

`Model` に必要なメンバが足りないと、コンパイルエラーになります。
たとえば `src` を書き忘れると、g++ のエラーの中に `static assertion failed: Derived must have member variable 'src'.` が現れます（その前に、`src` がないという別のエラーも出ます）。

(edu-ch05-model)=
## 5.6 demo の Model

章の冒頭の `demo/mhd2d_vortex/src/main.cpp` の `Model` と `main()` を、改めて読みます。

```cpp
struct Model : public mhd::ModelBase<Model, Real, Backend> {
  eos::IdealEOS<Real> eos;
  InitialCondition ic;
  mhd::EmptyBoundaryCondition<Real> bc;
  mhd::EmptySourceTerm<Real> src;

  Model(Config &config) : ModelBase(config), eos(config), ic(eos), bc(), src() {}
};

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

| メンバ | 型 | 役割 |
|---|---|---|
| `eos` | `eos::IdealEOS<Real>` | 理想気体の状態方程式。比熱比は設定ファイルの `eos.gm` |
| `ic` | `InitialCondition`（demo で定義） | 渦の初期条件。`apply` で `FieldsView` に値を書き込みます |
| `bc` | `mhd::EmptyBoundaryCondition<Real>` | 何もしない境界条件。全方向が周期境界なので、MPI の通信だけで足ります |
| `src` | `mhd::EmptySourceTerm<Real>` | ソース項なし |

`EmptyBoundaryCondition` と `EmptySourceTerm` は、`miso/include/miso/mhd_model_base.hpp` の末尾にある中身のほとんどない構造体です。
`EmptyBoundaryCondition` は、何もしない `apply` だけを持ちます。

```cpp
template <typename Real> struct EmptyBoundaryCondition {
  void apply(mhd::FieldsView<Real>, GridView<const Real>) const {}
};
```

引数に名前を付けていないのは、使わない引数だからです。
`ModelBase` から見ると、`bc.apply(...)` が呼べることだけが必要で、境界条件の型が何であるかは問いません。
自分で境界条件を作るときは、同じ形の `apply` を持つ構造体を書き、`Model` のメンバ `bc` の型をそれに変えます（`demo/hd1d_shock_tube/src/main.cpp` の `BoundaryCondition` がその例です）。

`main()` の流れは、MPI の準備（`Env`、{ref}`第 4 章 <edu-ch04-destructor>`）、設定ファイルの読み込み（`Config`）、`Model` の作成、`model.run()` の呼び出しです。
`model.run()` は、`Model` に `run()` がないので基底クラスの `ModelBase::run()` を呼び、その中で `Model` の部品が使われます。

(edu-ch05-update)=
## 5.7 update() の上書き

### ModelBase の update()

`ModelBase::update()` は 1 ステップの既定の処理です。
`miso/include/miso/mhd_model_base.hpp` から、ソース項に時間刻みの上限があるときの処理を省いて抜き出します。

```cpp
  /// @brief Default implementation of one timestep update.
  /// @details The derived class may provide its own `update()` method.
  void update() {
    auto &d = derived();
    // ... (preparation and checks for detecting the source term type)
    auto dt = mhd.cfl(d.eos);
    // ... (if the source term has the time step upper limit dt_limit(), limits dt to at most that)
    mhd.update(dt, d.eos, d.bc, d.src);
    time.update(dt);
  }
```

1. `mhd.cfl(d.eos)` で、CFL 条件から時間刻み `dt` を決めます。
2. `mhd.update(dt, d.eos, d.bc, d.src)` で、MHD の変数を 1 ステップ進めます。境界条件とソース項は派生クラスの部品を使います。
3. `time.update(dt)` で、時刻とステップ数を進めます（{ref}`第 4 章 <edu-ch04-const-member>` の `Time::update`）。

元のコメントに `The derived class may provide its own update() method.` とあるとおり、派生クラスは自分の `update()` を定義できます。
`run()` の中の `d.update()` は、5.4 節の `Decay` と同じ仕組みで、派生クラスの `update()` を呼びます。
これは仮想関数のオーバーライドではなく、5.2 節の名前の隠蔽と CRTP の組み合わせです。この教材では、派生クラスで同じ名前の関数を定義して処理を置き換えることを「上書き」と呼びます。

### MISO の例：磁気圏の demo

`demo/mhd3d_magnetosphere/src/main.cpp` の `Model` は、`update()` を上書きしています。

```cpp
struct Model : public mhd::ModelBase<Model, Real, Backend> {
  eos::IdealEOS<Real> eos;
  InitialCondition ic;
  BoundaryCondition bc;
  SourceTerm src;
  TimeStep timestep;

  void update() {
    const auto dt = timestep.cfl(mhd.qq, mhd.grid, eos);
    mhd.update(dt, eos, bc, src);
    time.update(dt);
  }

  Model(Config &config)
      : ModelBase(config), eos(config), ic(config, eos),
        bc(config, mpi_shape, grid, eos, ic), src(config, mhd.grid),
        timestep(config, mhd.grid) {}
};
```

`ModelBase::update()` との違いは、時間刻みの決め方だけです。
`TimeStep::cfl`（`demo/mhd3d_magnetosphere/src/timestep.hpp`）は、惑星の内側の領域（半径 `magnetosphere.radius` より内側）を CFL 条件の計算から除いて `dt` を決めます。
派生クラスの中なので、`mhd`, `time` などの基底クラスのメンバや、自分のメンバ `eos`, `bc`, `src` をそのまま使えます。
`derived()` を通す必要はありません。

`Model` が `run()` を定義すれば、`model.run()` は `Model::run()` を呼びます（名前の隠蔽）。
ただし、MISO の demo で `run()` を上書きしているものはありません。時間発展のループと出力の手順は `ModelBase::run()` に任せ、1 ステップの中身だけを必要に応じて上書きします。

(edu-ch05-exercises)=
## 練習

### 練習 1：Model のメンバの初期化順

`demo/hd1d_shock_tube/src/main.cpp` の `Model` について、次を説明してください。

1. `model` を作るときに、`ModelBase` のメンバ（`config`, `mpi_shape`, `time`, `grid`, `exec_ctx`, `mhd`）と `Model` のメンバ（`eos`, `ic`, `bc`, `src`）が初期化される順番。
2. `Model` の宣言で、`eos` と `ic` の宣言の順番を入れ替えたときに起きうる問題。

<details>
<summary>解答・解説</summary>

1. 基底クラスの部分が最初に、宣言順に `config`, `mpi_shape`, `time`, `grid`, `exec_ctx`, `mhd` と初期化されます。その後、`Model` のメンバが宣言順に `eos`, `ic`, `bc`, `src` と初期化されます。最後に `Model` のコンストラクタの本体（空）が実行されます。`bc(mpi_shape)` が使える理由は、`mpi_shape` が基底クラスのメンバで、先に初期化されているからです。
2. `ic` が `eos` より先に初期化されるようになります。`ic(config, eos)` は、まだ作られていない `eos` を参照として受け取ることになります。今の `InitialCondition` のコンストラクタは `eos` を参照として記録するだけで値を読まないので、実際には `apply` を呼ぶ時点で `eos` はできています。しかし、コンストラクタの中で `eos.gm` を読むように変更すると、値の入っていないメンバを読むことになります。初期化子リストの順番（`eos`, `ic`）は変わらないので見落としやすい誤りです。g++ に `-Wall` を付けると、順番の違い（`-Wreorder`）と、値の入っていない `eos` を使っていること（`-Wuninitialized`）を警告します。

</details>

### 練習 2：既定の update() に処理を足す

`demo/mhd2d_vortex/src/main.cpp` の `Model` に `update()` を加え、`ModelBase` の既定の 1 ステップを実行した後、10 ステップごとにステップ数と時刻を表示するようにしてください。
{ref}`第 1 章 <edu-ch01>` の、自分のプログラムから MISO を使う CMake 設定でビルドし、MPI のプロセス数 1 で動かして確かめてください。
設定ファイルは、`demo/mhd2d_vortex/config.yaml` をもとに、格子数を 64 x 64、`mpi` の分割を 1 x 1 x 1、`time.tend` を 0.1 にし、`io.enabled` を `false` にすると短時間で終わり、ファイルも書きません。

<details>
<summary>解答・解説</summary>

`Model` に次の関数を加えます（表示に `std::cout` を使うので、`#include <iostream>` も加えます）。

```cpp
  void update() {
    ModelBase::update();  // default single step (computes dt, updates MHD, advances time)
    if (time.n_step % 10 == 0) {
      std::cout << "n_step = " << time.n_step << ", time = " << time.time << "\n";
    }
  }
```

- `ModelBase::update()` は、5.2 節の `a.Simulation::update()` と同じく、隠れた基底クラスの関数を名指しで呼ぶ書き方です。`Model` の中では基底クラスを `ModelBase` と書けます。
- `ModelBase::run()` の中の `d.update()` は、この `Model::update()` を呼びます。
- `update()` の中で `ModelBase::update()` を付けずに `update();` と書くと、`Model::update()` 自身を呼び続けることになり、プログラムが止まらなくなる（最後はスタックの不足で異常終了する）ので注意します。

64 x 64 の格子、`time.tend: 0.1`、`time.dt_output: 0.05` の設定で、CPU（`float`）で実行した結果の例です。

```text
n_step = 10, time = 0.0496095
n_step = 20, time = 0.0984416
```

時間刻みは CFL 条件で毎ステップ決まるので、時刻の値は格子数や設定によって変わります。

</details>

### 練習 3：CRTP で呼ばれる関数

5.4 節の `Simulation<Derived>`、`Decay`、`Clock` について、次の 3 つの呼び出しで、`run()` の中の `derived().update()` がどのクラスの `update()` を呼ぶかを説明してください。
3 つ目では、`Decay` から `update()` の定義を消したものとします。

1. `Decay d(1.0, 1.0); d.run();`
2. `Clock c(1.0); c.run();`
3. （`Decay::update()` を消した）`Decay d(1.0, 1.0); d.run();`

<details>
<summary>解答・解説</summary>

1. `Decay::update()` です。`Simulation<Decay>` の中で `Derived` は `Decay` なので、`derived()` は `Decay &` を返し、`Decay` のメンバの `update()` が見つかります。
2. `Simulation<Clock>::update()` です。`derived()` は `Clock &` を返しますが、`Clock` は `update()` を定義していないので、基底クラスの `update()` が使われます。
3. `Simulation<Decay>::update()` です。2 と同じ理由です。結果は `time` が 1、`n_step` が 4 になり、`u` は変わりません。

どれも、呼ぶ関数はコンパイル時に決まります。
MISO の `Model` も同じで、`update()` を定義すれば `Model::update()`、しなければ `ModelBase::update()` が呼ばれます。

</details>
