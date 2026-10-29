(edu-ch11)=
# 11. MISO の時間発展を読む

{ref}`第 10 章 <edu-ch10>` で書いた熱伝導の計算には、時間刻みの決定、右辺の差分、時間積分、境界の処理、出力という部品がありました。
MISO の MHD 計算も同じ部品でできています。この章では、デモの `main()` から始めて、1 ステップの中身と出力までを MISO のソースでたどり、方程式と差分式をコードに対応づけます。
最後に、自分で問題を作るときに書き換える場所を確かめます。

本文のパスは、MISO を clone したフォルダから辿ってください。GPU の章（{ref}`第 9 章 <edu-ch09>`）を済ませていなくても、CPU 版のソースだけで読み進められます。CUDA 版は、CPU 版との対応だけを示します。

## 目次

- {ref}`1. 基礎方程式と MISO の変数 <edu-ch11-equations>`
- {ref}`2. main() と Model <edu-ch11-main>`
- {ref}`3. 設定ファイルと既定値 <edu-ch11-config>`
- {ref}`4. ModelBase の構築と run() <edu-ch11-run>`
- {ref}`5. 1 ステップの流れ <edu-ch11-step>`
- {ref}`6. CFL 条件による時間刻み <edu-ch11-cfl>`
- {ref}`7. 4 段の Runge–Kutta 法 <edu-ch11-rk4>`
- {ref}`8. 4 次中心差分と右辺の計算 <edu-ch11-sc4>`
- {ref}`9. 境界条件とハロー交換 <edu-ch11-boundary>`
- {ref}`10. 人工粘性 <edu-ch11-viscosity>`
- {ref}`11. 出力と再開 <edu-ch11-output>`
- {ref}`12. 自分で問題を作るときに触る場所 <edu-ch11-customize>`
- {ref}`13. ソースを読んで確かめる練習 <edu-ch11-exercises>`

(edu-ch11-equations)=
## 1. 基礎方程式と MISO の変数

### 1.1 状態変数

MISO は、各格子点に次の 9 つの変数を持ちます。`miso/include/miso/mhd_fields.hpp` の `Fields` が、これらを `Array3D` のメンバとしてまとめています（{ref}`第 4 章 <edu-ch04>`）。

| メンバ名 | 記号 | 意味 |
|---|---|---|
| `ro` | $\rho$ | 密度 |
| `vx`, `vy`, `vz` | $v_x, v_y, v_z$ | 速度 |
| `bx`, `by`, `bz` | $B_x, B_y, B_z$ | 磁場 |
| `ei` | $e$ | 単位質量あたりの内部エネルギー |
| `ph` | $\phi$ | 磁場の発散を減らすための補助変数（divB クリーニングのポテンシャル） |

保存する量は運動量 $\rho\boldsymbol{v}$ や全エネルギーですが、配列として持つのは速度 $\boldsymbol{v}$ と単位質量あたりの内部エネルギー $e$ です。
時間積分では、いったん保存量を作って進め、最後に $\rho$ で割って速度と $e$ に戻します（8 節）。

### 1.2 基礎方程式

MISO は理想 MHD 方程式を、Gauss 単位系（磁場の項に $1/4\pi$ が付く形）で解きます。
$\partial_j$ は $x, y, z$ 方向の偏微分、同じ添字 $j$ が 2 回出たら $j = x, y, z$ で和を取ります。
$S_\rho$ などはソース項（外力や加熱）で、ユーザーが与えます（12 節）。

$$
\begin{aligned}
\frac{\partial \rho}{\partial t} &= -\partial_j(\rho v_j) + S_\rho, \\
\frac{\partial (\rho v_i)}{\partial t} &= -\partial_j(\rho v_i v_j) - \partial_i p
  - \partial_i\left(\frac{B^2}{8\pi}\right) + \frac{1}{4\pi}\partial_j(B_i B_j) + S_{v_i}, \\
\frac{\partial B_i}{\partial t} &= \partial_j(v_i B_j - v_j B_i) - \partial_i \phi + S_{B_i}, \\
\frac{\partial \phi}{\partial t} &= -c_h^2\, \partial_j B_j - \frac{\phi}{\tau}, \\
\frac{\partial E}{\partial t} &= -\partial_j\left[\left(\rho e + p + \frac{1}{2}\rho v^2 + \frac{B^2}{4\pi}\right) v_j
  - \frac{(\boldsymbol{v}\cdot\boldsymbol{B})}{4\pi} B_j\right] + S_E, \\
E &= \rho e + \frac{1}{2}\rho v^2 + \frac{B^2}{8\pi}.
\end{aligned}
$$

磁場の式の $-\partial_i\phi$ と $\phi$ の式は、数値計算で生じる $\nabla\cdot\boldsymbol{B}$ の誤差を波として運び出し（速さ $c_h$）、時間 $\tau$ で減衰させる仕組みです。
この方法は双曲型の発散クリーニング（hyperbolic divergence cleaning）と呼ばれます（Dedner et al. 2002, J. Comput. Phys. 175, 645）。

### 1.3 状態方程式

圧力 $p$ と音速 $c_s$ は状態方程式（EOS: equation of state）から求めます。
MISO が用意している理想気体の EOS は `miso/include/miso/eos.hpp` の `eos::IdealEOS` です。比熱比を $\gamma$（設定の `eos.gm`）として、

$$
p = (\gamma - 1)\rho e, \qquad c_s = \sqrt{\gamma(\gamma - 1) e} = \sqrt{\gamma p/\rho}
$$

を、`gas_pressure` と `sound_speed` の 2 つのメンバ関数で計算します。
どちらも `for_each` で全格子点を処理します（{ref}`第 7 章 <edu-ch07>`）。

```cpp
// miso/include/miso/eos.hpp: IdealEOS::gas_pressure (excerpt)
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

メンバ変数 `gm` をローカル変数 `gm_` に写してからラムダで使うのは、{ref}`第 9 章 <edu-ch09>` で説明した `this` キャプチャを避けるためです。

(edu-ch11-main)=
## 2. main() と Model

`demo/mhd2d_vortex/src/main.cpp` の `main()` は、次の 4 行だけです。

```cpp
// demo/mhd2d_vortex/src/main.cpp
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

| 行 | 役割 | Fortran で書くなら |
|---|---|---|
| `Env env(argc, argv);` | MPI を初期化します。`env` の寿命が終わる `main()` の末尾で、デストラクタが `MPI_Finalize` を呼びます（{ref}`第 8 章 <edu-ch08>`） | `call MPI_Init(ierr)` と、最後の `call MPI_Finalize(ierr)` |
| `parse_config_filepath(argc, argv)` | コマンド行の `--config=<path>` または `--config <path>` を探します。なければ空の `std::optional` を返します | `get_command_argument` で引数を読む処理 |
| `Config config(...)` | 設定ファイルを読み、既定値で補います（3 節）。`value_or("./config.yaml")` は、指定がなければ `./config.yaml` を使うという意味です | `namelist` を読む処理 |
| `Model model(config);` | 格子、配列、時間積分の部品をまとめて作ります（4 節） | 各 `*_init` の呼び出しと `allocate` |
| `model.run();` | 時間ループを回します（4 節） | `do while (time < tend)` の時間ループ |

`Model` は、デモごとに書く構造体です。

```cpp
// demo/mhd2d_vortex/src/main.cpp
struct Model : public mhd::ModelBase<Model, Real, Backend> {
  eos::IdealEOS<Real> eos;
  InitialCondition ic;
  mhd::EmptyBoundaryCondition<Real> bc;
  mhd::EmptySourceTerm<Real> src;

  Model(Config &config) : ModelBase(config), eos(config), ic(eos), bc(), src() {}
};
```

`Model` は `mhd::ModelBase` を継承し、テンプレート引数に自分自身（`Model`）を渡しています。{ref}`第 5 章 <edu-ch05>` で学んだ CRTP です。
派生クラスが持つのは、問題ごとに変わる 4 つの部品です。

| メンバ | 役割 | この例での型 |
|---|---|---|
| `eos` | 状態方程式 | `eos::IdealEOS<Real>`（理想気体） |
| `ic` | 初期条件 | `InitialCondition`（同じファイルで定義） |
| `bc` | 境界条件 | `mhd::EmptyBoundaryCondition<Real>`（何もしない。全方向が周期境界のとき） |
| `src` | ソース項 | `mhd::EmptySourceTerm<Real>`（ソース項なし） |

`Real` と `Backend` は、ファイルの先頭の `using Real = float;` と `#ifdef USE_CUDA` で決めています。`Real = float` で単精度、`USE_CUDA` が定義されていれば `backend::CUDA`（GPU）、なければ `backend::Host`（CPU）で計算します（{ref}`第 2 章 <edu-ch02>`、{ref}`第 6 章 <edu-ch06>`）。

(edu-ch11-config)=
## 3. 設定ファイルと既定値

### 3.1 `config.yaml` のセクション

`demo/mhd2d_vortex/config.yaml` には、次のセクションがあります。

| セクション | 内容 | 読む場所 |
|---|---|---|
| `io` | 出力の有無、出力先、再開するか（`continue`） | `config.hpp`, `time.hpp`, `mhd_checkpoint.hpp` |
| `time` | 終了時刻 `tend`、出力間隔 `dt_output` | `time.hpp` の `Time` |
| `grid` | 格子数 `i_size` など（全体の格子数）、ゴーストセルの幅 `margin`、領域 `x_min` など | `grid.hpp` の `Grid` |
| `mpi` | 各方向のプロセス数 `x_procs` など | `mpi_util.hpp` の `mpi::Shape` |
| `domain` | 周期境界にする方向 `periodic` | `mpi_util.hpp` の `mpi::Shape` |
| `mhd` | CFL 数 `cfl_number`、人工粘性のパラメータ | `mhd_integrator_host.hpp`, `mhd_artificial_viscosity_host.hpp` |
| `eos` | 比熱比 `gm` | `eos.hpp` の `IdealEOS` |

値を読むときは、`config["time"]["tend"].as<Real>()` のように、セクション名とキーを順に `[]` で指定し、`as<型>()` で型を決めて取り出します。
`as<Real>()` の `<>` はテンプレート引数です（{ref}`第 6 章 <edu-ch06>`）。

### 3.2 既定値と merge_yaml

すべてのキーを毎回書かなくてよいように、MISO は既定値を `miso/include/miso/config_defaults.hpp` に YAML の文字列として持っています。

```cpp
// miso/include/miso/config_defaults.hpp(excerpt; some comments omitted)
inline constexpr const char *config_default_yaml = R"yaml(
io:
  enabled: true
  continue: true
  save_dir: data/
  ...
mhd:
    # CFL number for time-stepping
    cfl_number: 0.5
    artificial_viscosity:
        ep: 1.0
        fh: 1.0
        ...
    )yaml";
```

`R"yaml( ... )yaml"` は生文字列リテラル（raw string literal）です。`R"区切り(` から `)区切り"` までを、改行や `"` も含めてそのまま 1 つの文字列にします。
YAML をそのまま C++ のソースに埋め込むために使っています。

`miso/include/miso/config.hpp` の `Config` のコンストラクタは、次の順に処理します。

1. ランク 0 だけがファイルを読み（`YAML::LoadFile`）、文字列にして `MPI_Bcast` で全ランクに配ります。
2. 全ランクで既定値の文字列を読み（`YAML::Load(config_default_yaml)`）、`merge_yaml` でユーザーの設定に足します。
3. `io.save_dir` を、設定ファイルのあるフォルダからの相対パスとして解釈し、出力先を作ります。

```cpp
// miso/include/miso/config.hpp
inline void merge_yaml(YAML::Node &target, const YAML::Node &defaults) {
  if (!defaults.IsMap()) {
    return;
  }

  if (!target || !target.IsMap()) {
    target = YAML::Node(YAML::NodeType::Map);
  }

  for (const auto &it : defaults) {
    const std::string key = it.first.as<std::string>();
    const YAML::Node &default_value = it.second;

    if (default_value.IsMap()) {
      YAML::Node child = target[key];
      merge_yaml(child, default_value);  // fill nested sections recursively
      target[key] = child;
    } else {
      if (!target[key]) {               // fill only keys the user did not write
        target[key] = default_value;
      }
    }
  }
}
```

（`merge_yaml` の 2 つのコメントは説明のために足しました。）
ユーザーが書いた値は上書きされず、書いていないキーだけが既定値で埋まります。セクションの中のセクション（`mhd.artificial_viscosity` など）も、キーごとに補われます。
補った結果は、出力先に `config.yaml` として保存されます（11 節）。実際に使われた値は、この保存されたファイルで確かめられます。

(edu-ch11-run)=
## 4. ModelBase の構築と run()

### 4.1 メンバの構築順

`miso/include/miso/mhd_model_base.hpp` の `ModelBase` は、次のメンバを持ちます。

```cpp
// miso/include/miso/mhd_model_base.hpp(excerpt)
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
  ...
```

メンバは宣言の順に作られます（{ref}`第 4 章 <edu-ch04>`）。後のメンバは、前に作ったメンバを使います。

| 順 | メンバ | 作るもの |
|---|---|---|
| 1 | `config` | 設定への参照 |
| 2 | `mpi_shape` | `MPI_Cart_create` による直交格子状のプロセス配置と、各方向の隣のランク（{ref}`第 8 章 <edu-ch08>`） |
| 3 | `time` | 時刻、ステップ数、出力番号 |
| 4 | `grid` | このランクが受け持つ格子（ゴーストセルを含む座標と格子間隔）。CPU 上に作ります |
| 5 | `exec_ctx` | 計算場所ごとの付加情報（CPU では `mpi_shape` への参照だけ。GPU ではカーネルの起動形状も持ちます） |
| 6 | `mhd` | 状態変数 `qq`、時間積分の `Integrator`、出力の `Checkpoint` |

`mhd` の型 `MHD` は `miso/include/miso/mhd_solver.hpp` にあります。

```cpp
// miso/include/miso/mhd_solver.hpp(excerpt)
template <typename Real, typename Backend> struct MHD {
  Grid<Real, Backend> grid;
  Fields<Real, Backend> qq;
  ExecContext<Real, Backend> &exec_ctx;
  Integrator<Real, Backend> integrator;
  Checkpoint<Real> checkpoint;
  ...
  template <typename EOS, typename BoundaryCondition, typename Source>
  void update(Real dt, const EOS &eos, const BoundaryCondition &bc,
              const Source &src) {
    integrator.update(dt, eos, bc, src, qq);
  }
  ...
};
```

`MHD` は、状態変数 `qq` と計算の部品を持ち、仕事を `Integrator` に渡す窓口です。`Grid<Real, Backend>` は、CPU 上の格子 `grid_h` を計算場所（`Backend`）へ写したものです。

派生クラス `Model` のメンバ（`eos`, `ic`, `bc`, `src`）は、基底クラス `ModelBase` の構築が終わってから作られます。
そのため `ModelBase` のコンストラクタはこれらを使わず、`run()` の中で `derived()` を通して受け取ります。

### 4.2 run()：時間ループ

```cpp
// miso/include/miso/mhd_model_base.hpp(excerpt)
void run() {
  // Ensure to call methods in the derived class
  auto &d = derived();

  // Assert that d has eos, ic, bc, and src as member variables.
  static_assert(has_eos<Derived>::value,
                "Derived must have member variable 'eos'.");
  ...
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
```

| 処理 | 内容 |
|---|---|
| `derived()` | `static_cast<Derived &>(*this)` で、自分を派生クラス `Model` として扱います（{ref}`第 5 章 <edu-ch05>`） |
| `static_assert(has_eos<Derived>::value, ...)` | `Model` に `eos` などのメンバがなければ、コンパイル時にエラーにします（{ref}`第 6 章 <edu-ch06>`） |
| `mhd.apply_initial_condition(d.ic, d.bc)` | CPU 上の一時配列に `ic.apply` で初期値を入れ、計算場所の `qq` へ写し、境界条件とハロー交換を 1 回行います |
| `load_state()` | `io.continue` が真で、前回の出力が残っていれば、最新の出力から再開します（11 節） |
| `save_metadata()` | 設定（既定値を補ったもの）、格子、MPI の配置を書き出します |
| `save_if_needed()` | 出力する時刻に達していれば状態を書き出します |
| `d.update()` | 1 ステップ進めます |

`config["io"]["continue"].template as<bool>()` の `.template` は、テンプレートの中で、依存する型のメンバテンプレートを呼ぶときに必要な書き方です。`as<bool>()` と同じ意味に読んで構いません。

`d.update()` は、`Model` が `update()` を定義していればそれを呼び、なければ基底の `ModelBase::update()` を呼びます。
`demo/mhd3d_magnetosphere/src/main.cpp` の `Model` は自分の `update()` を定義し、時間刻みの決め方を変えています。

### 4.3 update()：1 ステップの入口

```cpp
// miso/include/miso/mhd_model_base.hpp(excerpt)
void update() {
  auto &d = derived();
  using Source = decltype(d.src);
  ...
  auto dt = mhd.cfl(d.eos);
  if constexpr (has_dt_limit<Source>::value) {
    // dt_limit() returns the limit of this rank: take the global minimum.
    ...
    Real dt_src = static_cast<Real>(d.src.dt_limit());
    ...
    MPI_Allreduce(&dt_src, &dt_src_g, 1, mpi::data_type<Real>(), MPI_MIN,
                  mpi::comm());
    ...
    dt = std::min(dt, dt_src_g);
  }
  mhd.update(dt, d.eos, d.bc, d.src);
  time.update(dt);
}
```

1. `mhd.cfl(d.eos)` で CFL 条件から時間刻みを決めます（6 節）。
2. ソース項が `dt_limit()` を持っていれば、その制限（全ランクの最小値）も合わせます。持っていなければ `if constexpr` の中はコンパイルされません。
3. `mhd.update` で状態を 1 ステップ進めます（5 節）。
4. `time.update(dt)` で時刻とステップ数を進めます。

(edu-ch11-step)=
## 5. 1 ステップの流れ

`MHD::update` から呼ばれる `Integrator::update` が、1 ステップの本体です。
`Integrator` は `miso/include/miso/mhd_integrator.hpp` で宣言され、CPU 版と GPU 版が別々のファイルで特殊化されています（{ref}`第 6 章 <edu-ch06>`）。

| ファイル | 中身 |
|---|---|
| `miso/include/miso/mhd_integrator.hpp` | `Integrator` の宣言、ソース項を取り出す `source_ro` など、4 次中心差分 `space_centered_4th` |
| `miso/include/miso/mhd_integrator_host.hpp` | `Integrator<Real, backend::Host>`（CPU 版） |
| `miso/include/miso/mhd_integrator_cuda.cuh` | `Integrator<Real, backend::CUDA>`（GPU 版。`nvcc` でコンパイルするときだけ読み込みます） |

CPU 版の `update` は 3 行です。

```cpp
// miso/include/miso/mhd_integrator_host.hpp
template <typename EOS, typename BoundaryCondition, typename Source>
void update(Real dt, const EOS &eos, const BoundaryCondition &bc,
            const Source &src, Fields<Real, backend::Host> &qq) {
  divb_parameters_set(dt);
  runge_kutta_4step(dt, eos, bc, src, qq);
  apply_artificial_viscosity(dt, eos, bc, qq);
}
```

1 ステップ全体を、関数の呼び出し順に並べると次のようになります。

```text
ModelBase::update
├─ MHD::cfl → Integrator::cfl                      time step dt (section 6)
├─ MHD::update → Integrator::update
│   ├─ divb_parameters_set(dt)                     set c_h and τ
│   ├─ runge_kutta_4step                           4-stage time integration (section 7)
│   │   ├─ update_sc4(dt/4) → boundary conditions and halo exchange
│   │   ├─ update_sc4(dt/3) → boundary conditions and halo exchange
│   │   ├─ update_sc4(dt/2) → boundary conditions and halo exchange
│   │   └─ update_sc4(dt)   → boundary conditions and halo exchange
│   └─ apply_artificial_viscosity                   artificial viscosity (section 10)
│       ├─ x direction → boundary conditions and halo exchange
│       ├─ y direction → boundary conditions and halo exchange
│       └─ z direction → boundary conditions and halo exchange
└─ Time::update                                    advance the time
```

`divb_parameters_set` は、発散クリーニングの速さ $c_h$ と減衰時間 $\tau$ を、時間刻みに合わせて決めます。

```cpp
// miso/include/miso/mhd_integrator_host.hpp
void divb_parameters_set(Real dt) {
  ch_divb = 0.8 * cfl_number * grid.min_dxyz / dt;
  ch_divb_square = ch_divb * ch_divb;
  tau_divb = 2.0 * dt;
}
```

$c_h \Delta t = 0.8\, C \min(\Delta x, \Delta y, \Delta z)$（$C$ は CFL 数）なので、$\phi$ の波も CFL 条件を満たす速さで伝わります。

CPU 版と GPU 版は、関数の名前と呼び出し順が同じです。違うのは、格子点ごとの計算を `for` 文で書くか、CUDA のカーネルで書くかです。

| 処理 | CPU 版（`mhd_integrator_host.hpp`） | GPU 版（`mhd_integrator_cuda.cuh`） |
|---|---|---|
| $B^2$, $h_t$, $\boldsymbol{v}\cdot\boldsymbol{B}$ の準備 | `update_sc4` の最初の三重ループ | `bb_ht_vb_kernel` |
| 各変数の更新 | `update_sc4` の 2 つ目の三重ループ | `update_ro_kernel`, `update_vx_kernel` 〜 `update_ei_kernel` |
| CFL 条件 | 三重ループで最小値を取り、`MPI_Allreduce` | `reduce(backend::CUDA{}, ...)` と `MPI_Allreduce` |
| `runge_kutta_4step`, `apply_boundary_condition` | 同じ順序 | 同じ順序 |
| 人工粘性 | `impl_host::ArtificialViscosity`（`mhd_artificial_viscosity_host.hpp`） | `impl_cuda::ArtificialViscosity`（`mhd_artificial_viscosity_cuda.cuh`） |

CPU 版の `update_sc4` は、`for_each` ではなく、ふつうの三重の `for` 文で書かれています。
`i` が外側、`k` が内側のループで、最後の添字が連続する `Array3D` の並び（{ref}`第 7 章 <edu-ch07>`）に沿ってメモリを読みます。

(edu-ch11-cfl)=
## 6. CFL 条件による時間刻み

時間刻みは、どの格子点でも、最も速い波が 1 ステップで格子間隔を越えないように決めます。

$$
\Delta t = C \min_{i,j,k} \frac{\min(\Delta x_i, \Delta y_j, \Delta z_k)}{c_s + |\boldsymbol{v}| + c_A},
\qquad c_A = \frac{|\boldsymbol{B}|}{\sqrt{4\pi\rho}}
$$

$C$ は設定の `mhd.cfl_number`、$c_A$ は Alfvén 速度です。

```cpp
// miso/include/miso/mhd_integrator_host.hpp: Integrator::cfl (excerpt)
Real dt = 1.e10;
eos.sound_speed(backend::Host{}, qq.const_view(), cs.view());
for (int i = grid.i_margin; i < grid.i_total - grid.i_margin; ++i) {
  for (int j = grid.j_margin; j < grid.j_total - grid.j_margin; ++j) {
    for (int k = grid.k_margin; k < grid.k_total - grid.k_margin; ++k) {
      ...
      Real total_vel = cs(i, j, k) + vv + ca;
      dt = util::min2(dt, cfl_number
        * util::min3<Real>(grid.dx[i], grid.dy[j], grid.dz[k])/total_vel);
    }
  }
}
Real dt_global;
MPI_Allreduce(&dt, &dt_global, 1, mpi::data_type<Real>(), MPI_MIN,
              mpi::comm());
return dt_global;
```

ループの範囲は `i_margin` から `i_total - i_margin` の手前まで、つまりゴーストセルを除いた、このランクが受け持つ格子点です。
各ランクで最小値を求めたあと、`MPI_Allreduce` の `MPI_MIN` で全ランクの最小値をそろえます（{ref}`第 8 章 <edu-ch08>`）。
すべてのランクが同じ $\Delta t$ で進まないと、隣と交換する値の時刻がずれるためです。

(edu-ch11-rk4)=
## 7. 4 段の Runge–Kutta 法

### 7.1 runge_kutta_4step

方程式を $\partial q/\partial t = R(q)$ と書きます。$q$ は保存量（$\rho$, $\rho\boldsymbol{v}$, $\boldsymbol{B}$, $\phi$, $E$）、$R$ は右辺（空間微分とソース項）です。
MISO は、次の 4 段で $q^n$ から $q^{n+1}$ を作ります。

$$
\begin{aligned}
q^{(1)} &= q^n + \frac{\Delta t}{4} R(q^n), \\
q^{(2)} &= q^n + \frac{\Delta t}{3} R(q^{(1)}), \\
q^{(3)} &= q^n + \frac{\Delta t}{2} R(q^{(2)}), \\
q^{n+1} &= q^n + \Delta t\, R(q^{(3)}).
\end{aligned}
$$

各段は「元の値 $q^n$ に、直前の段の値で評価した右辺を足す」形です。必要な配列は、元の値、直前の段の値、結果の 3 組だけです。

```cpp
// miso/include/miso/mhd_integrator_host.hpp
template <typename EOS, typename BoundaryCondition, typename Source>
void runge_kutta_4step(const Real dt, const EOS &eos,
                       const BoundaryCondition &bc, const Source &src,
                       Fields<Real, backend::Host> &qq) {
  update_sc4(dt / 4.0, eos, src, qq, qq, qq_rslt);
  qq_argm.copy_from(qq_rslt);
  apply_boundary_condition(bc, qq_argm);

  update_sc4(dt / 3.0, eos, src, qq, qq_argm, qq_rslt);
  qq_argm.copy_from(qq_rslt);
  apply_boundary_condition(bc, qq_argm);

  update_sc4(dt / 2.0, eos, src, qq, qq_argm, qq_rslt);
  qq_argm.copy_from(qq_rslt);
  apply_boundary_condition(bc, qq_argm);

  update_sc4(dt, eos, src, qq, qq_argm, qq_rslt);
  qq.copy_from(qq_rslt);
  apply_boundary_condition(bc, qq);
}
```

`update_sc4(段の dt, eos, src, 元の値, 右辺を評価する値, 結果)` の引数と、式の対応は次のとおりです。

| 引数 | 配列 | 式の記号 |
|---|---|---|
| `qq_orgn`（3 番目の `Fields`） | `qq` | $q^n$（ステップの最初の値。4 段の間は変えません） |
| `qq_argm`（4 番目） | 1 段目は `qq`、2 段目以降は `qq_argm` | $R(\cdot)$ に入れる直前の段の値 |
| `qq_rslt`（5 番目） | `qq_rslt` | その段の結果 |

各段の結果は `qq_rslt` に書かれ、`copy_from` で `qq_argm` へ写してから境界条件を適用します。
`update_sc4` はゴーストセルを除いた格子点だけを更新するので、次の段の右辺を計算する前に、ゴーストセルを埋め直す必要があります。
{ref}`第 10 章 <edu-ch10>` の熱伝導で、時間積分の各段の前に境界の値を用意したのと同じ順序です。
最後の段だけは結果を `qq` に戻し、これが $q^{n+1}$ になります。

### 7.2 段の係数を確かめる

線形の方程式 $dq/dt = \lambda q$ では、$R(q) = \lambda q$ なので、$h = \lambda\Delta t$ として順に代入すると

$$
q^{n+1} = \left(1 + h + \frac{h^2}{2} + \frac{h^3}{6} + \frac{h^4}{24}\right) q^n
$$

となり、厳密解 $e^{h} q^n$ の Taylor 展開と 4 次の項まで一致します。
次の短いプログラムは、$dq/dt = -q$ を同じ 4 段で $t = 1$ まで解き、厳密解 $e^{-1}$ との差を表示します。

```cpp
// rk4_linear.cpp: solve dq/dt = -q with the same 4-stage coefficients as MISO (illustrative example)
#include <cmath>
#include <iostream>

double solve(int n_step) {
  const double dt = 1.0 / n_step;
  double q = 1.0;                       // value at time t (qq in MISO)
  for (int n = 0; n < n_step; ++n) {
    double q_argm = q;                  // value used to evaluate the right-hand side (qq_argm in MISO)
    q_argm = q + dt / 4.0 * (-q_argm);  // stage 1
    q_argm = q + dt / 3.0 * (-q_argm);  // stage 2
    q_argm = q + dt / 2.0 * (-q_argm);  // stage 3
    q = q + dt * (-q_argm);             // stage 4 updates q
  }
  return q;
}

int main() {
  for (int n : {10, 20, 40}) {
    std::cout << "n_step = " << n
              << "  error = " << std::abs(solve(n) - std::exp(-1.0)) << "\n";
  }
}
```

```bash
g++ -std=c++17 -O2 -o rk4_linear rk4_linear.cpp
./rk4_linear
```

```text
n_step = 10  error = 3.33241e-07
n_step = 20  error = 1.99761e-08
n_step = 40  error = 1.22274e-09
```

ステップ数を 2 倍にすると誤差がおよそ $1/16 = 1/2^4$ になり、4 次精度であることが分かります。

<details>
<summary>コラム：この 4 段の方法と非線形の方程式</summary>

この形の多段法は、Jameson, Schmidt, Turkel (1981, AIAA Paper 81-1259) が圧縮性流体の計算に用いたことで知られています。
各段で保存する配列が少なくて済むのが利点です。

係数 $1/4, 1/3, 1/2, 1$ は、線形の方程式に対して 4 次精度になるように選ばれています。
非線形の方程式 $dq/dt = f(q)$ では、$\Delta t^3$ の項のうち $f''f^2$ に掛かる係数を展開して比べると、この方法では $1/8$、厳密解では $1/6$ となり一致しません。
そのため、非線形の方程式に対する形式上の精度は 2 次です。
一方、線形の安定領域は古典的な 4 段 Runge–Kutta 法と同じです。
`miso/include/miso/mhd_resistivity.hpp` のコメントにある「RK4 の安定条件 $\lambda\Delta t < 2.78$」も、この安定領域から来ています。

</details>

(edu-ch11-sc4)=
## 8. 4 次中心差分と右辺の計算

### 8.1 space_centered_4th

空間微分は、4 次精度の中心差分で近似します。等間隔の格子で、

$$
\left.\frac{\partial f}{\partial x}\right|_i \approx
\frac{-f_{i+2} + 8 f_{i+1} - 8 f_{i-1} + f_{i-2}}{12\, \Delta x}
$$

です。誤差は $\Delta x^4$ に比例します。片側に 2 点ずつ使うので、ゴーストセルの幅（`grid.margin`）は 2 以上必要です。

```cpp
// miso/include/miso/mhd_integrator.hpp
template <typename Real>
__host__ __device__ inline Real
space_centered_4th(Array3DView<const Real> qq, Real dxyzi, int i, int j, int k,
                   int is, int js, int ks) {
  // clang-format off
  return (
    -     qq(i + 2*is, j + 2*js, k + 2*ks)
    + 8.0*qq(i +   is, j +   js, k +   ks)
    - 8.0*qq(i -   is, j -   js, k -   ks)
    +     qq(i - 2*is, j - 2*js, k - 2*ks)
  )*inv12<Real>*dxyzi;
  // clang-format on
};
```

| 引数 | 意味 |
|---|---|
| `qq` | 微分する配列（`const` の View） |
| `dxyzi` | 格子間隔の逆数 $1/\Delta x$（`grid.dxi[i]` など） |
| `i, j, k` | 微分を求める格子点 |
| `is, js, ks` | 微分の方向。$x$ 方向なら `(grid.is, 0, 0)` |

方向を `is, js, ks` で選ぶので、1 つの関数で 3 方向の微分を書けます。
`grid.is` は、$x$ 方向の格子数が 2 以上なら 1、1 なら 0 です。格子数が 1 の方向では添字がずれず、その方向の微分は 0 になります（練習 4）。
`inv12<Real>` は $1/12$ を `Real` 型で表す変数テンプレートです（`miso/include/miso/constants.hpp`、{ref}`第 6 章 <edu-ch06>`）。

同じ名前で、2 つまたは 3 つの配列の積を微分する関数もあります（オーバーロード、{ref}`第 2 章 <edu-ch02>`）。
`space_centered_4th(c_ro, c_vx, dxi, ...)` は $\partial_x(\rho v_x)$、`space_centered_4th(c_ro, c_vx, c_vy, dyi, ...)` は $\partial_y(\rho v_x v_y)$ です。
各格子点で積を作ってから差分を取ります。

### 8.2 update_sc4 の準備

`update_sc4` は、最初に右辺で何度も使う量を配列に用意します。

```cpp
// miso/include/miso/mhd_integrator_host.hpp: update_sc4 (excerpt; formulas omitted, explanatory comments added)
eos.gas_pressure(backend::Host{}, qq_argm.const_view(), pr.view());

for (int i = 0; i < grid.i_total; ++i) {
  for (int j = 0; j < grid.j_total; ++j) {
    for (int k = 0; k < grid.k_total; ++k) {
      bb(i, j, k) = ...;  // B^2
      ht(i, j, k) = ...;  // ρe + p + B^2/4π + ρv^2/2
      vb(i, j, k) = ...;  // v·B
    }
  }
}
```

| 配列 | 式 | 使う場所 |
|---|---|---|
| `pr` | $p = (\gamma-1)\rho e$ | 運動方程式の圧力勾配 |
| `bb` | $B^2$ | 運動方程式の磁気圧 $B^2/8\pi$ |
| `ht` | $\rho e + p + \frac{1}{2}\rho v^2 + \frac{B^2}{4\pi}$ | エネルギーの流束 |
| `vb` | $\boldsymbol{v}\cdot\boldsymbol{B}$ | エネルギーの流束 |

差分を取るので、ゴーストセルを含む全格子点で計算します。どれも `qq_argm`（右辺を評価する値）から作ります。

続いて、`const_view()` で読み取り専用の View を作り、名前を短くしています（`c_ro`, `c_vx`, ... `c_ph`）。

### 8.3 方程式とコードの対応

2 つ目の三重ループで、ゴーストセルを除いた格子点を更新します。
段の時間刻みを $\Delta t'$（`dt` 引数。$\Delta t/4$ など）、`space_centered_4th` を $D_x, D_y, D_z$ と書きます。

**連続の式**

```cpp
// miso/include/miso/mhd_integrator_host.hpp: update_sc4 (excerpt)
qq_rslt.ro(i, j, k) = qq_orgn.ro(i, j, k) + dt * (
-space_centered_4th(c_ro, c_vx, dxi, i, j, k, grid.is, 0, 0)
-space_centered_4th(c_ro, c_vy, dyi, i, j, k, 0, grid.js, 0)
-space_centered_4th(c_ro, c_vz, dzi, i, j, k, 0, 0, grid.ks)
+source_ro(src, c_qq, i, j, k)
);
```

$$
\rho^{\text{rslt}} = \rho^{n} + \Delta t' \left[ -D_x(\rho v_x) - D_y(\rho v_y) - D_z(\rho v_z) + S_\rho \right]
$$

右辺の $\rho v_x$ などは `qq_argm` の値、左辺に足す $\rho^n$ は `qq_orgn` の値です。7 節の式の形と一致します。

**運動方程式（$x$ 成分）**

```cpp
// miso/include/miso/mhd_integrator_host.hpp: update_sc4 (excerpt)
qq_rslt.vx(i, j, k) = (
    qq_orgn.ro(i, j, k) * qq_orgn.vx(i, j, k) + dt * (
    -space_centered_4th(c_ro, c_vx, c_vx, dxi, i, j, k, grid.is, 0, 0)
    -space_centered_4th(c_ro, c_vx, c_vy, dyi, i, j, k, 0, grid.js, 0)
    -space_centered_4th(c_ro, c_vx, c_vz, dzi, i, j, k, 0, 0, grid.ks)
    -space_centered_4th(c_pr, dxi, i, j, k, grid.is, 0, 0)
    -space_centered_4th(c_bb, dxi, i, j, k, grid.is, 0, 0)*pii8<Real>
    +pii4<Real>*(
        +space_centered_4th(c_bx, c_bx, dxi, i, j, k, grid.is, 0, 0)
        +space_centered_4th(c_bx, c_by, dyi, i, j, k, 0, grid.js, 0)
        +space_centered_4th(c_bx, c_bz, dzi, i, j, k, 0, 0, grid.ks)
        )
    +source_vx(src, c_qq, i, j, k)
    )
)/qq_rslt.ro(i, j, k);
```

$$
v_x^{\text{rslt}} = \frac{1}{\rho^{\text{rslt}}}\left\{ \rho^n v_x^n + \Delta t' \left[
-D_j(\rho v_x v_j) - D_x p - \frac{1}{8\pi} D_x(B^2) + \frac{1}{4\pi} D_j(B_x B_j) + S_{v_x}
\right]\right\}
$$

保存量 $\rho v_x$ を進めてから、新しい密度 $\rho^{\text{rslt}}$ で割って速度に戻しています。
そのため、連続の式（`qq_rslt.ro`）を先に計算しておく必要があります。
`pii4<Real>` と `pii8<Real>` は $1/4\pi$ と $1/8\pi$ です（`miso/include/miso/constants.hpp`）。$y, z$ 成分も同じ形です。

**誘導方程式（$x$ 成分）**

```cpp
// miso/include/miso/mhd_integrator_host.hpp: update_sc4 (excerpt)
qq_rslt.bx(i, j, k) = qq_orgn.bx(i, j, k) + dt * (
    -space_centered_4th(c_vy, c_bx, dyi, i, j, k, 0, grid.js, 0)
    -space_centered_4th(c_vz, c_bx, dzi, i, j, k, 0, 0, grid.ks)
    +space_centered_4th(c_vx, c_by, dyi, i, j, k, 0, grid.js, 0)
    +space_centered_4th(c_vx, c_bz, dzi, i, j, k, 0, 0, grid.ks)
    -space_centered_4th(c_ph, dxi, i, j, k, grid.is, 0, 0)
    +source_bx(src, c_qq, i, j, k)
);
```

$$
B_x^{\text{rslt}} = B_x^n + \Delta t' \left[ D_y(v_x B_y - v_y B_x) + D_z(v_x B_z - v_z B_x) - D_x\phi + S_{B_x} \right]
$$

1.2 節の誘導方程式で $i = x$ とした式です（$j = x$ の項は $v_x B_x - v_x B_x = 0$ なので書いていません）。最後の $-D_x\phi$ が発散クリーニングの項です。

**発散クリーニングの $\phi$**

`qq_rslt.ph` の式は、`ch_divb_square`（$c_h^2$）と `util::exp(-dt/tau_divb)` を使って次の形になっています。

$$
\phi^{\text{rslt}} = \left[ \phi^n - \Delta t'\, c_h^2 \left( D_x B_x + D_y B_y + D_z B_z \right) \right] e^{-\Delta t'/\tau}
$$

$-c_h^2\nabla\cdot\boldsymbol{B}$ の項を差分で進め、減衰の項 $-\phi/\tau$ は、厳密解の因子 $e^{-\Delta t'/\tau}$ を掛けて扱っています。

**エネルギーの式**

```cpp
// miso/include/miso/mhd_integrator_host.hpp: update_sc4 (excerpt; parts elided with ...)
// Et: total energy per unit volume
// ei: is the internal energy per unit mass
const Real Et =
    + qq_orgn.ro(i,j,k)*qq_orgn.ei(i,j,k)
    + 0.5*qq_orgn.ro(i, j, k)*( ... )   // v^2 of qq_orgn
    + pii8<Real>*( ... );               // B^2 of qq_orgn

qq_rslt.ei(i, j, k) = ( Et + dt * (
    -space_centered_4th(c_ht, c_vx, dxi, i, j, k, grid.is, 0, 0)
    -space_centered_4th(c_ht, c_vy, dyi, i, j, k, 0, grid.js, 0)
    -space_centered_4th(c_ht, c_vz, dzi, i, j, k, 0, 0, grid.ks)
    + pii4<Real>* (
        +space_centered_4th(c_vb, c_bx, dxi, i, j, k, grid.is, 0, 0)
        +space_centered_4th(c_vb, c_by, dyi, i, j, k, 0, grid.js, 0)
        +space_centered_4th(c_vb, c_bz, dzi, i, j, k, 0, 0, grid.ks)
        )
    )
    -0.5*qq_rslt.ro(i, j, k)*( ... )   // v^2 of qq_rslt (subtract the new kinetic energy)
    - pii8<Real>*( ... )               // B^2 of qq_rslt (subtract the new magnetic energy)
    + dt * source_ei(src, c_qq, i, j, k)
)/qq_rslt.ro(i, j, k);
```

$$
e^{\text{rslt}} = \frac{1}{\rho^{\text{rslt}}}\left\{
E^n + \Delta t' \left[ -D_j(h_t v_j) + \frac{1}{4\pi} D_j\big((\boldsymbol{v}\cdot\boldsymbol{B}) B_j\big) + S_E \right]
- \frac{1}{2}\rho^{\text{rslt}} (v^{\text{rslt}})^2 - \frac{(B^{\text{rslt}})^2}{8\pi}
\right\}
$$

全エネルギー $E$ を進め、同じ段で求めた新しい $\rho$, $\boldsymbol{v}$, $\boldsymbol{B}$ の運動エネルギーと磁気エネルギーを引いて、内部エネルギーに戻します。
新しい速度と磁場を使うので、エネルギーの式はループの最後に置かれています。
ソース項 `source_ei` は、全エネルギーの式の右辺 $S_E$（単位体積・単位時間あたり）です。

### 8.4 ソース項の取り出し

`source_ro(src, c_qq, i, j, k)` などは、`miso/include/miso/mhd_integrator.hpp` のマクロ `MISO_MHD_DEFINE_SOURCE_TERM` が作る関数です。

次は、マクロが `vy` について作る関数の本体を、マクロの引数 `name` を `vy` に置き換えて示したものです（説明のために展開しました）。

```cpp
// miso/include/miso/mhd_integrator.hpp: MISO_MHD_DEFINE_SOURCE_TERM expanded for vy (excerpt)
if constexpr (impl_source::has_vy<Source, Real>::value) {
  return src.vy(qq, i, j, k);
} else {
  return Real(0);
}
```

ソース項の型 `Source` が、たとえば `vy(FieldsView<const Real>, int, int, int) const` というメンバ関数を持っていればそれを呼び、持っていなければ 0 を返します（{ref}`第 6 章 <edu-ch06>`）。
`mhd::EmptySourceTerm` は何も持たないので、8 つのソース項がすべて 0 になります。
判定はコンパイル時に行われるので、定義していない項のために計算が増えることはありません。
ソース項は、右辺を評価する値 `c_qq`（`qq_argm`）で各段ごとに呼ばれます。

(edu-ch11-boundary)=
## 9. 境界条件とハロー交換

### 9.1 apply_boundary_condition

RK4 の各段と人工粘性の各方向のあとで、次の関数が呼ばれます。

```cpp
// miso/include/miso/mhd_integrator_host.hpp
template <typename BoundaryCondition>
void apply_boundary_condition(const BoundaryCondition &bc,
                              Fields<Real, backend::Host> &qq) {
  bc.apply(qq.view(), grid.const_view());
  halo_exchanger.apply(qq);
}
```

1. `bc.apply` で、ユーザーが与えた物理境界条件を適用します。
2. `halo_exchanger.apply` で、隣のランクとゴーストセルの値を交換します（ハロー交換、{ref}`第 8 章 <edu-ch08>`）。

`bc.apply` の引数の形は、`Model` の `bc` が必ず持つ約束です。

```cpp
void apply(mhd::FieldsView<Real> qq, GridView<const Real> grid) const;
```

### 9.2 ハロー交換と周期境界

ハロー交換は `miso/include/miso/mhd_halo_exchange.hpp` の `HaloExchanger` にあります。
CPU 版の `apply` は、$x$ → $y$ → $z$ の順に、方向ごとに次を行います。

1. 9 変数のうち送る範囲（端から `margin` 枚）を送信用の `Array4D` に詰めます。
2. `MPI_Isend` と `MPI_Irecv` で正の側と負の側に同時に送受信し、`MPI_Waitall` で待ちます。
3. 受け取った値をゴーストセルへ書き込みます。

方向を 1 つずつ交換するので、$y$ 方向で送る範囲には、直前に $x$ 方向で受け取ったゴーストセルも含まれます。
そのため、角のゴーストセルにも正しい値が入ります。

隣のランクは、`mpi::Shape` が `MPI_Cart_shift` で求めた `x_procs_pos` などです。
`domain.periodic` で周期境界にした方向では、領域の端のランクの隣が反対側の端のランクになります。
その方向のプロセス数が 1 なら、隣は自分自身です。
つまり、**周期境界はハロー交換が担います**。全方向が周期境界の問題では、`bc` に何もしない `mhd::EmptyBoundaryCondition` を使えます。

周期境界にしない方向の端では、隣が `MPI_PROC_NULL` になり、交換は行われません。そのゴーストセルを埋めるのが `bc.apply` の役目です。

### 9.3 物理境界の例

`demo/hd2d_rayleigh_taylor/src/main.cpp` の `BoundaryCondition` は、$y$ 方向の両端に対称境界を置きます。

```cpp
// demo/hd2d_rayleigh_taylor/src/main.cpp: BoundaryCondition::apply (excerpt)
namespace bc = miso::boundary_condition;
Backend btag{};

if (bc::is_physical_boundary(Direction::Y, Side::INNER, mpi_shape)) {
  bc::symmetric(btag, qq.ro, grid, Sign::Pos, Direction::Y, Side::INNER);
  bc::symmetric(btag, qq.vx, grid, Sign::Pos, Direction::Y, Side::INNER);
  bc::symmetric(btag, qq.vy, grid, Sign::Neg, Direction::Y, Side::INNER);
  ...
}
```

`is_physical_boundary`（`miso/include/miso/boundary_condition.hpp`）は、このランクの $y$ 方向の負の側（`Side::INNER`）に隣のランクがない、つまり領域の端を受け持っているかを調べます。
MPI で分割していると、端を受け持たないランクもあるので、この判定が必要です。

`symmetric` は、境界をはさんで鏡に映した位置の値をゴーストセルに写します。`Sign::Neg` なら符号を反転します。
境界に垂直な速度 `vy` だけを反転させ、境界を通り抜ける流れを 0 にしています。

(edu-ch11-viscosity)=
## 10. 人工粘性

中心差分だけでは、衝撃波などの急な変化の近くで数値振動が生じます。
MISO は、RK4 のあとに人工粘性（人工拡散）を 1 回かけて、急な変化の近くだけを選んでならします。
実装は `miso/include/miso/mhd_artificial_viscosity_host.hpp` の `impl_host::ArtificialViscosity` で、方法は Rempel (2014, ApJ 789, 132) に基づきます（`config_defaults.hpp` のコメント）。

### 10.1 方向ごとの更新

```cpp
// miso/include/miso/mhd_integrator_host.hpp(excerpt)
template <typename EOS, typename BoundaryCondition>
void apply_artificial_viscosity(const Real dt, const EOS &eos,
                                const BoundaryCondition &bc,
                                Fields<Real, backend::Host> &qq) {
  artdiff.characteristic_velocity_eval(qq, eos);

  // x direction
  artdiff.update(qq, qq_rslt, Direction::X, dt);
  qq.copy_from(qq_rslt);
  apply_boundary_condition(bc, qq);

  ...  // same 3 lines for y and z (Direction::Y, Direction::Z)
}
```

最初に特性速度 $c = f_{cs} c_s + f_{vv}|\boldsymbol{v}| + f_{ca} c_A$ を全格子点で求めます（係数は設定の `mhd.artificial_viscosity.cs_fac` など）。
そのあと $x, y, z$ の順に 1 方向ずつ更新し、そのたびに境界条件とハロー交換を行います。

### 10.2 拡散流束

1 方向の更新は、セル境界 $i+1/2$ での流束 $F_{i+1/2}$ の差です。

$$
q_i^{\text{new}} = q_i - \frac{\Delta t}{\Delta x}\left(F_{i+1/2} - F_{i-1/2}\right)
$$

対象は $\rho$, $\rho\boldsymbol{v}$, $\boldsymbol{B}$, $\phi$, $\rho e$ です。流束は `miso/include/miso/limiter.hpp` の 2 つの関数で作ります。

| 関数 | 計算 |
|---|---|
| `dqq_eval(q_{i-1}, q_i, q_{i+1}, ep)` | 傾き $\Delta q_i$。$\varepsilon(q_i - q_{i-1})$、$\varepsilon(q_{i+1} - q_i)$、$(q_{i+1} - q_{i-1})/2$ の符号がそろっていれば絶対値が最小のもの、そろっていなければ 0（一般化 minmod 制限子）。$\varepsilon$ は設定の `ep` |
| `flux_core(q_i, q_{i+1}, Δq_i, Δq_{i+1}, c, fh)` | セル境界の左右の値 $q_L = q_i + \Delta q_i/2$, $q_R = q_{i+1} - \Delta q_{i+1}/2$ を作り、$F = -\frac{1}{2} c\, \Phi\, (q_R - q_L)$ を返す |

$\Phi$ は、$r = \min\left(1, (q_R - q_L)/(q_{i+1} - q_i)\right)$ を使って、$r \ge 0$ なら $\max(0, 1 + h(r - 1))$、$r < 0$ なら 0 です。$h$ は設定の `fh` です。
変化がなめらかなところでは $q_R \approx q_L$ なので流束はほぼ 0 です。急な変化があって $q_R - q_L$ が大きく残るところだけで拡散が働きます。

エネルギーは、$\rho e$ の流束で全エネルギーを更新したあと、新しい運動エネルギーと磁気エネルギーを引いて内部エネルギーに戻します。
人工粘性で失われた運動エネルギーと磁気エネルギーは、内部エネルギーに移ります。

(edu-ch11-output)=
## 11. 出力と再開

### 11.1 出力の時刻

```cpp
// miso/include/miso/mhd_model_base.hpp
void save_if_needed() {
  if (time.time >= time.dt_output * time.n_output) {
    save_state();
    time.log();
    time.n_output++;
  }
}
```

時刻が `dt_output * n_output` 以上になった最初のステップで出力します。時間刻みを出力時刻に合わせて調整しないので、出力時刻はちょうど `dt_output` の倍数にはなりません（練習 6）。
`time.log()` は、ランク 0 で次のような行を表示します（`demo/mhd2d_vortex` を `tend: 0.03` にして 2 プロセスで実行した例）。

```text
time =   0.00;  n_step =        0;  n_output =        0
time =   0.01;  n_step =        4;  n_output =        1
time =   0.02;  n_step =        9;  n_output =        2
time =   0.03;  n_step =       13;  n_output =        3
```

### 11.2 出力されるファイル

すべて `io.save_dir`（既定は設定ファイルと同じフォルダの `data/`）の下に書かれます。

| ファイル | 書く関数 | 中身 |
|---|---|---|
| `config.yaml` | `Config::save` | 既定値を補った設定 |
| `grid.bin` | `Grid::save` | 全体格子の座標 |
| `mpi/coords.csv` | `mpi::Shape::save` | ランクごとの MPI 座標 |
| `time/time.NNNNNNNN.txt` | `Time::save`（`miso/include/miso/time.hpp`） | 時刻、出力番号、ステップ数を 1 行ずつ |
| `time/n_output.txt` | `Time::save` | 最新の出力番号 |
| `mhd/mhd.<n_output>.<rank>.bin` | `Checkpoint::save`（`miso/include/miso/mhd_checkpoint.hpp`） | ランクごとの 9 変数 |

`Checkpoint::save` は、計算場所の `qq` を CPU 上の配列へ写し（`copy_from`）、先頭に要素の大きさ（`float` なら 4）を `std::uint32_t` で書いてから、`ro, vx, vy, vz, bx, by, bz, ei, ph` の順に、ゴーストセルを含む配列をそのまま書き出します。
並びは `Array3D` のメモリ上の並び（最後の添字 `k` が連続）です。
上の例では、各ランクの格子が $(64 + 2\cdot 2) \times (128 + 2\cdot 2) \times 1$ なので、ファイルの大きさは $4 + 68 \times 132 \times 9 \times 4 = 323140$ バイトです。
Python では `pymiso.Data` で読みます（{ref}`第 1 章 <edu-ch01>`）。

### 11.3 再開

`io.continue` が真（既定値も真）で、`time/n_output.txt` が残っていれば、`run()` は初期条件を作ったあとで `load_state()` を呼び、最新の出力から計算を続けます。
`Time::load` はランク 0 で時刻ファイルを読んで `MPI_Bcast` で配り、`Checkpoint::load` は各ランクが自分の `mhd.<n_output>.<rank>.bin` を読みます。
出力をそのまま再開用のデータ（チェックポイント）として使う設計です。最初から計算し直すときは、`data/` を消すか `io.continue: false` にします。

(edu-ch11-customize)=
## 12. 自分で問題を作るときに触る場所

新しい問題は、多くの場合、デモの `main.cpp` を写して `Model` の 4 つの部品と `config.yaml` を書き換えて作ります。
`miso/include/miso/` の中を書き換える必要はありません。

| 変えたいもの | 書く場所 | 約束（呼ばれる形） | 参考になるデモ |
|---|---|---|---|
| 初期条件 | `ic` の型 | `void apply(mhd::FieldsView<Real> qq, GridView<const Real> grid) const`。ゴーストセルを含む全格子点に値を入れます。CPU 上の配列に対して 1 回だけ呼ばれます | `demo/mhd2d_vortex` |
| 境界条件 | `bc` の型 | `void apply(mhd::FieldsView<Real> qq, GridView<const Real> grid) const`。周期境界でない方向の端のゴーストセルを埋めます。1 ステップに何回も、計算場所の配列に対して呼ばれます | `demo/hd2d_rayleigh_taylor` |
| ソース項 | `src` の型 | 必要な項だけ `__host__ __device__ Real vy(mhd::FieldsView<const Real> qq, int i, int j, int k) const` の形で定義します（`ro`, `vx`〜`vz`, `bx`〜`bz`, `ei`）。時間刻みの上限があれば `Real dt_limit() const` | `demo/hd2d_rayleigh_taylor`, `demo/mhd2d_reconnection` |
| 状態方程式 | `eos` の型 | `gas_pressure(btag, qq, pr)` と `sound_speed(btag, qq, cs)` を `IdealEOS` と同じ引数で持ちます | `miso/include/miso/eos.hpp` |
| 時間刻みの決め方 | `Model::update()` | `ModelBase::update()` と同じ形で上書きします | `demo/mhd3d_magnetosphere` |
| 問題のパラメータ | `config.yaml` の独自セクション | コンストラクタで `config["rayleigh_taylor"]["g_grav"].as<Real>()` のように読みます | `demo/hd2d_rayleigh_taylor` |

注意する点は次のとおりです。

- 境界条件とソース項は、GPU 版では GPU 上の配列に対して呼ばれます。ソース項のメンバ関数には `__host__ __device__` を付け、GPU から読める値（数値のメンバや、計算場所の配列の View）だけを使います（{ref}`第 9 章 <edu-ch09>`）。
- 境界条件で `for` 文を直接書くと CPU でしか動きません。`boundary_condition::symmetric` のように計算場所のタグ `btag` を受け取る関数や、`for_each` を使います。
- 全方向を周期境界にする場合は、`config.yaml` の `domain.periodic` をすべて `true` にし、`bc` に `mhd::EmptyBoundaryCondition<Real>` を使えます。

部品ごとの書き方と例は、MISO のドキュメントの {doc}`カスタマイズ </customization/index>` にまとめています。

(edu-ch11-exercises)=
## 13. ソースを読んで確かめる練習

(edu-ch11-ex-bc-count)=
### 練習 1：1 ステップで境界条件が呼ばれる回数

`demo/mhd2d_vortex` の設定で 1 ステップ進めるとき、`bc.apply` が何回呼ばれるかを、`miso/include/miso/mhd_integrator_host.hpp` を読んで数えてください。
初期条件を作るときに呼ばれる回数と、1 ステップで `MPI_Allreduce` が呼ばれる回数も数えてください。

<details>
<summary>解答・解説</summary>

1 ステップで **7 回**です。

- `runge_kutta_4step` の 4 段で、それぞれ `apply_boundary_condition` を 1 回ずつ（4 回）。
- `apply_artificial_viscosity` の $x, y, z$ の各方向で 1 回ずつ（3 回）。

`apply_boundary_condition` は `bc.apply` のあとに `halo_exchanger.apply` を呼ぶので、ハロー交換も 1 ステップで 7 回です。
2 次元の計算でも、$z$ 方向の人工粘性の呼び出しは省かれないので回数は同じです。

初期条件を作るときは、`MHD::apply_initial_condition` の最後で 1 回呼ばれます。

`MPI_Allreduce` は、`Integrator::cfl` で 1 回です。ソース項が `dt_limit()` を持つ場合は、`ModelBase::update` でもう 1 回呼ばれます。

回数は、`apply` の中で呼び出し回数を数える境界条件を `bc` に使い、`update()` を直接呼んでも確かめられます。
`apply` は `const` のメンバ関数なので、中でメンバ変数を書き換えられません。数える変数は境界条件の構造体の外（たとえば `main.cpp` で関数の外に宣言した変数）に置きます。
この方法で数えると、初期条件で 1、1 ステップで 7、さらに 2 ステップで 14 になります。

</details>

(edu-ch11-ex-defaults)=
### 練習 2：既定値の補完

`demo/hd2d_rayleigh_taylor/config.yaml` には `mhd.cfl_number` が書かれていません。
この計算で使われる CFL 数と人工粘性のパラメータ `ep` の値を、`config_defaults.hpp` と `merge_yaml` を読んで答えてください。
実際に使われた値を、計算後のどのファイルで確かめられるかも説明してください。

<details>
<summary>解答・解説</summary>

CFL 数は既定値の **0.5**、`ep` はユーザーが書いた **2.0** です。

`merge_yaml` は既定値の `mhd` セクションに入り、`cfl_number` はユーザーの設定にないので 0.5 を足します。
`artificial_viscosity` はセクション（YAML の Map）なので再帰的に入り、キーごとに補います。
`ep` はユーザーが書いているので上書きしません。ユーザーが書いていない `mhd.resistivity.cfl_number` も 0.5 で補われます。

補った結果は、`save_metadata()` が出力先に書く `data/config.yaml` に残ります。
`demo/hd2d_rayleigh_taylor` の設定では `io.save_dir` が既定の `data/` なので、`demo/hd2d_rayleigh_taylor/data/config.yaml` です。

</details>

(edu-ch11-ex-gravity)=
### 練習 3：重力のソース項

`demo/hd2d_rayleigh_taylor/src/main.cpp` の `SourceTerm` は、$-y$ 方向の一様な重力（大きさ $g$）を与えます。
このソース項を使うと、`update_sc4` のどの式のどの項が変わるかを説明してください。
また、全エネルギーを保存させるには、`ei` のソース項として何を返す必要があるかを、8.3 節のエネルギーの式から説明してください。

<details>
<summary>解答・解説</summary>

`SourceTerm` が定義しているのは `vx`, `vy`, `vz`, `ei` で、0 以外を返すのは `vy` の $-g\rho$ だけです。

```cpp
// demo/hd2d_rayleigh_taylor/src/main.cpp
__host__ __device__ inline Real vy(mhd::FieldsView<const Real> qq, int i, int j,
                                   int k) const noexcept {
  return -g_grav * qq.ro(i, j, k);
}
```

変わるのは、$y$ 方向の運動方程式の `+source_vy(src, c_qq, i, j, k)` の項です。

$$
\rho^{\text{rslt}} v_y^{\text{rslt}} = \rho^n v_y^n + \Delta t' \left[ \cdots - g\rho \right]
$$

$\rho$ は `c_qq`、つまりその段の `qq_argm` の値で、RK4 の 4 段のそれぞれで評価されます。
`ro`, `bx`, `by`, `bz` は定義されていないので、`if constexpr` で 0 になります。

エネルギーについて：MISO のエネルギーの式は全エネルギー $E$ の式で、内部エネルギーは $e = (E - \rho v^2/2 - B^2/8\pi)/\rho$ で戻します。
重力が流体にする仕事 $\rho\boldsymbol{v}\cdot\boldsymbol{g} = -g\rho v_y$ を $S_E$ に入れないと、重力で増えた運動エネルギーの分が $E$ に足されないまま引かれ、内部エネルギーが減ります。
全エネルギーを保存させるには、`ei` で $-g\rho v_y$ を返します。

```cpp
// example of the source term for ei (illustrative)
__host__ __device__ inline Real ei(mhd::FieldsView<const Real> qq, int i, int j,
                                   int k) const noexcept {
  return -g_grav * qq.ro(i, j, k) * qq.vy(i, j, k);
}
```

`demo/hd2d_rayleigh_taylor` の `ei` は 0 を返しているので、このデモでは、重力がした仕事の分が内部エネルギーの側で差し引かれます。

重力を足すときは、ソース項のほかに、初期条件を静水圧平衡にすること（このデモでは $p = p_0 - g\rho y$）と、$y$ 方向の端の境界条件も考える必要があります。

</details>

(edu-ch11-ex-2d)=
### 練習 4：2 次元計算での z 方向の差分

`demo/mhd2d_vortex` は `k_size: 1` の 2 次元計算です。
このとき、`update_sc4` の $z$ 方向の差分、CFL 条件、ハロー交換、人工粘性の $z$ 方向の更新がどうなるかを、`miso/include/miso/grid.hpp` の `AxisGrid` を読んで説明してください。

<details>
<summary>解答・解説</summary>

`AxisGrid` は、格子数 `size` が 1 の方向で `stride = 0`、`margin = 0`、`total = 1` とし、格子間隔 `ds` を `1.e30`、その逆数 `dsi` を 0 にします。
`grid.ks` は `stride` なので 0、`grid.k_margin` は 0、`grid.k_total` は 1 です。

- **差分**：`space_centered_4th(..., 0, 0, grid.ks)` は `ks = 0` なので 4 点がすべて同じ格子点 `k` になり、$(-1 + 8 - 8 + 1)$ の組み合わせで 0 になります。さらに `dzi` も 0 です。$z$ 方向の微分の項は 0 になります。
- **CFL 条件**：`min3(dx[i], dy[j], dz[k])` で `dz` が `1.e30` なので、$z$ 方向は時間刻みを制限しません。
- **ハロー交換**：`HaloExchanger` の $z$ 方向は `grid.k_size > 1` が偽なので行いません（GPU 版は `grid.k_total > 1` で判定します）。
- **人工粘性**：`Direction::Z` では `ks = 0` なので左右の値がすべて同じになり、流束の差 $F_{i+1/2} - F_{i-1/2}$ が 0 です。掛ける `dzi` も 0 なので値は変わりません。

ループの範囲はどれも `k = 0` だけになり、配列の範囲外を読むことはありません。

</details>

(edu-ch11-ex-accuracy)=
### 練習 5：4 次中心差分の精度を確かめる（発展）

MISO を使うプログラムを自分で書き、`mhd::space_centered_4th` で $f(x) = \sin x$ の微分を求めて、$\cos x$ との差の最大値を調べてください。
$0 \le x < 2\pi$ を $n$ 個のセルに分け、セル中心 $x_i = (i + 1/2)\Delta x$ に値を置きます。
$n = 16, 32, 64$ で誤差がどう変わるかを確かめてください。
ビルドは、{ref}`第 1 章 <edu-ch01>` の「自分のプログラムから MISO を使う最小の CMake 設定」を使います。

<details>
<summary>解答・解説</summary>

作るものは次のとおりです。

1. `#include <miso/core.hpp>` と `#include <miso/mhd.hpp>` を書きます（`space_centered_4th` は `mhd.hpp` から読み込まれます）。
2. ゴーストセルを両側に 2 つずつ付けた `Array3D<double>`（大きさ `n + 4, 1, 1`）を作り、添字 `i` の位置 $x = (i - 2 + 1/2)\Delta x$ に $\sin x$ を入れます。ゴーストセルにも同じ式で値を入れると、周期境界と同じになります。
3. `i = 2` から `n + 1` まで、`mhd::space_centered_4th(f.const_view(), 1.0 / dx, i, 0, 0, 1, 0, 0)` と $\cos x$ の差の最大値を求めます。

`space_centered_4th` は MPI を使わないので、このプログラムでは `Env` を作らなくても動きます。

結果の例は次のとおりです。

```text
n = 16  error = 0.000763339
n = 32  error = 4.90805e-05
n = 64  error = 3.08927e-06
ratio 16/32 = 15.5528
ratio 32/64 = 15.8874
```

格子数を 2 倍にすると誤差がおよそ $1/16 = 1/2^4$ になり、4 次精度であることが分かります。
差分式の誤差の主要項は $-\frac{\Delta x^4}{30} f^{(5)}$ で、$f = \sin x$ では最大値が $\Delta x^4/30$ です。$n = 16$ では $\Delta x = \pi/8$ なので $(\pi/8)^4/30 \approx 7.9\times 10^{-4}$ となり、結果と合います。

</details>

(edu-ch11-ex-output-time)=
### 練習 6：出力の時刻

`demo/mhd2d_vortex` を `dt_output: 0.01` で実行すると、`data/time/time.00000001.txt` の 1 行目は `0.0100069` のように、0.01 からわずかにずれた値になります。
このずれが生じる理由を、`ModelBase::save_if_needed` と `ModelBase::run` を読んで説明してください。

<details>
<summary>解答・解説</summary>

`run()` は 1 ステップ進めるたびに `save_if_needed()` を呼び、`time.time >= dt_output * n_output` が成り立った最初のステップで出力します。
時間刻み $\Delta t$ は CFL 条件だけで決まり、出力時刻にちょうど届くように縮めることはしません。
そのため、出力されるのは 0.01 を初めて越えたステップの時刻で、0.01 より最大 $\Delta t$ だけ後になります。

出力時刻の正確な値は、`time/time.NNNNNNNN.txt` の 1 行目に書かれています。解析で時刻を使うときは、`dt_output` の倍数ではなく、このファイルの値（`pymiso` が読む値）を使います。

</details>

---

{ref}`目次 <edu-index>` · {ref}`前の章 <edu-ch10>` · {ref}`Fortran との対応表 <edu-ref-fortran-map>`
