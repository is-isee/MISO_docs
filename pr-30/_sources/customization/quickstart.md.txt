# Quick Start

MISOではモデル作成のベースとなるクラス `miso::mhd::ModelBase` を用意しており、これを編集することで様々なテスト問題を実現しています。
`ModelBase`で想定されている問題から大きく離れるほど、ユーザが記述するコード量は増えていきます。

以下では、テスト問題 `demo/mhd2d_vortex` のメインプログラムを例に説明します。
この問題は、以下のように初期条件を設定するだけで実装されています。

```c++
struct InitialCondition {
  eos::IdealEOS<Real> &eos;

  explicit InitialCondition(eos::IdealEOS<Real> &eos) : eos(eos) {}

  // The signature must not be changed as it is called inside miso::mhd::MHD.
  void apply(mhd::FieldsView<Real> qq, GridView<const Real> grid) const {
    const Real pr = 1.0 / eos.gm;
    const Real b0 = util::sqrt(4.0 * pi<Real>) / eos.gm;
    const Real v0 = 1.0;

    for (int k = 0; k < grid.k_total; ++k) {
      for (int j = 0; j < grid.j_total; ++j) {
        for (int i = 0; i < grid.i_total; ++i) {
          qq.ro(i, j, k) = 1.0;
          qq.ei(i, j, k) = eos.roprtoei(qq.ro(i, j, k), pr);
          qq.vx(i, j, k) = -v0 * util::sin(2.0 * pi<Real> * grid.y[j]);
          qq.vy(i, j, k) = +v0 * util::sin(2.0 * pi<Real> * grid.x[i]);
          qq.vz(i, j, k) = 0.0;
          qq.bx(i, j, k) = -b0 * util::sin(2.0 * pi<Real> * grid.y[j]);
          qq.by(i, j, k) = +b0 * util::sin(4.0 * pi<Real> * grid.x[i]);
          qq.bz(i, j, k) = 0.0;
          qq.ph(i, j, k) = 0.0;
        }
      }
    }
  }
};
```

ここで、`miso::eos::IdealEOS<Real>`は理想気体の状態方程式を計算するために用意されているクラスです。
`miso::mhd::FieldsView`は密度や速度場などのMHD方程式の基本変数をメンバに持つクラス、`miso::GridView`は計算格子に関するクラスです。
両者は上記のように `qq.ro(i, j, k)` や `grid.x[i]` のように配列の各要素にアクセスすることが出来ます。
関数`apply`は`ModelBase`から呼び出されるため、シグネチャ(引数や返り値など)を変更することが出来ないことに注意してください。

この初期条件の`ModelBase`への組み込みは、以下のように書くことができます。

```c++
struct Model : public mhd::ModelBase<Model, Real, Backend> {
  eos::IdealEOS<Real> eos;
  InitialCondition ic;
  mhd::EmptyBoundaryCondition<Real> bc;
  mhd::EmptySourceTerm<Real> src;

  Model(Config &config) : ModelBase(config), eos(config), ic(eos), bc(), src() {}
};
```

ここで、`miso::mhd::EmptyBoundaryCondition`は何もしない境界条件です。全方向が周期境界条件で済む場合のみこれを利用してください。
計算領域が周期境界か否かは設定ファイル `config.yaml` の `domain` セクションで以下のように指定できます。

```yaml
domain:
  periodic:
    x: true
    y: true
    z: true
```

MHDシミュレーションでは、方程式の右辺にソース項がある場合、例えば重力などの外力や輻射冷却などの加熱項を加えたい場合、があります。
上記の`miso::mhd::EmptySourceTerm`は、右辺にソース項を何も持たない場合に利用するクラスです。

`Model`の親クラスである`miso::mhd::ModelBase`は、実際には以下のような初期化を行います。

- 設定ファイル(`config.yaml`など)の読み込み
- MPIやCUDAの設定
- 計算格子 `miso::Grid` の初期化
- MHDクラス `miso::mhd::MHD` の初期化

テスト問題 `demo/mhd2d_vortex` の `Model` では、これに加えて状態方程式や初期条件、境界条件やソース項の初期化を行っています。

メイン関数では、以下のように`miso`の初期化、設定ファイル `config.yaml` の読み込み、`Model`の実行を行います。

```c++
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

`ModelBase.run()`は、具体的には以下のような計算を実行します。

- 初期条件 `ic` のMHD変数への適用
- 設定ファイルの情報や計算格子 `grid` (`miso::Grid`) などのファイル出力
- 初期状態及び各時間ステップでのMHD変数 `qq` (`miso::mhd::Fields`) のファイル出力
- (既存計算データがあれば) 既存計算データのロード
- メインループ
  - CFL条件の評価と時間ステップサイズの決定
  - MHD変数の時間積分 (境界条件 `bc` の適用やソース項 `src` の右辺への付加、状態方程式 `eos` の呼び出しを含む)
  - 時刻 `time` (`miso::Time`) の更新

ここで、初期条件 `ic` 、境界条件 `bc` 、ソース項 `src` 、状態方程式 `eos` はユーザが設定することを想定しており、`ModelBase`を継承した子クラスで定義された変数を利用します。

それぞれのカスタム方法については、`demo`内にある各テスト問題を参考にしてください。
特に磁気圏問題 `demo/mhd3d_magnetosphere` は、初期条件、境界条件、ソース項、加えてCFL条件も変更しておりおすすめです。
