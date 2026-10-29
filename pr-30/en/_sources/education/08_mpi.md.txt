(edu-ch08)=
# 8. MPI による領域分割

Fortran の数値計算コードでは、計算領域をいくつかに分け、各プロセスが自分の部分を計算し、境界の値だけを MPI で受け渡す書き方が広く使われています。
MISO も同じ考え方で並列計算をします。この章では、MPI の基本の呼び出しを短い確認プログラムで動かしてから、MISO が領域をどう分け、隣と値をどう交換し、時間刻みをどう揃えるかを読みます。

第 7 章で見たとおり、MISO の `for_each` の Host 版はただの `for` 文で、OpenMP も使っていません。
CPU で MISO を並列に動かす手段は、この章で扱う MPI だけです。
この章の内容は CPU だけで試せます。GPU を使う場合の通信は第 9 章で扱います。

## 目次

- {ref}`1. プロセスとランク <edu-ch08-ranks>`
- {ref}`2. MISO の Env：MPI の開始と終了 <edu-ch08-env>`
- {ref}`3. ランクの並びと隣のランク：mpi::Shape <edu-ch08-shape>`
- {ref}`4. 全体格子とランクごとの格子 <edu-ch08-grid>`
- {ref}`5. 隣との値の交換：MPI_Sendrecv <edu-ch08-sendrecv>`
- {ref}`6. 通信を始めてから完了を待つ：MPI_Isend と MPI_Irecv <edu-ch08-nonblocking>`
- {ref}`7. MISO のハロー交換 <edu-ch08-halo>`
- {ref}`8. 物理境界と内部境界 <edu-ch08-physical-boundary>`
- {ref}`9. 全ランクで値をまとめる：MPI_Allreduce <edu-ch08-allreduce>`
- {ref}`10. MISO のデモを MPI で動かす <edu-ch08-run>`
- {ref}`11. まとめ <edu-ch08-summary>`

(edu-ch08-ranks)=
## 1. プロセスとランク

### 同じプログラムを複数のプロセスで動かす

`mpiexec -np 4 ./program` は、同じプログラム `./program` を 4 つの**プロセス**として同時に起動します。
各プロセスは自分の変数と配列を持ちます。あるプロセスが配列を書き換えても、他のプロセスの配列は変わりません。
他のプロセスの値が必要なときは、MPI の関数で明示的に送り、受け取ります。
同じ計算機の中で動かす場合も、この区別は変わりません。

通信に参加するプロセスの集まりを**コミュニケータ**（communicator）と呼びます。
`MPI_COMM_WORLD` は、`mpiexec` で起動した全プロセスの集まりです。
コミュニケータの中で各プロセスに付く 0 始まりの番号が**ランク**（rank）です。
4 プロセスなら、ランクは 0, 1, 2, 3 です。

各プロセスは同じプログラムを実行しますが、自分のランクを調べて、担当する領域や処理を変えます。
この形を SPMD（Single Program, Multiple Data）と呼びます。

### 確認プログラム：ランク番号の表示

次のプログラムは、各プロセスが自分のランクと全体のプロセス数を表示します。
`hello_mpi.cpp` として保存します。

```cpp
#include <iostream>
#include <mpi.h>

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);  // MPI を使い始める（全ランクが呼ぶ）

  int rank = 0;  // 自分の番号
  int size = 0;  // 全体のランク数
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // rank に書き込まれる
  MPI_Comm_size(MPI_COMM_WORLD, &size);  // size に書き込まれる

  std::cout << "rank " << rank << " / size " << size << std::endl;

  MPI_Finalize();  // MPI を使い終える（全ランクが呼ぶ）
  return 0;
}
```

`#include <mpi.h>` で MPI の関数の宣言を読み込みます。
MPI の関数は `MPI_Init` の後、`MPI_Finalize` の前で呼びます。
`&rank` は第 3 章で学んだ変数のアドレスです。`MPI_Comm_rank` はそのアドレスへランク番号を書き込みます。

C++ からは、MPI の C 言語用の関数を呼びます。Fortran との違いは次のとおりです。

| | Fortran | C++（C 用の関数） |
|---|---|---|
| 初期化 | `call MPI_Init(ierr)` | `MPI_Init(&argc, &argv);` |
| ランク | `call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)` | `MPI_Comm_rank(MPI_COMM_WORLD, &rank);` |
| エラーコード | 最後の引数 `ierr` に返る | 関数の戻り値（`int`）として返る |
| 型の指定 | `MPI_DOUBLE_PRECISION`, `MPI_REAL` | `MPI_DOUBLE`, `MPI_FLOAT` |

Fortran の引数は参照渡しなので `rank` をそのまま渡しますが、C の関数に書き込んでもらうにはアドレス `&rank` を渡します。
戻り値のエラーコードは、使わなければ受け取らなくてかまいません。

第 1 章で入れた OpenMPI の `mpicxx` でコンパイルし、`mpiexec` で起動します。
`mpicxx` は、MPI のヘッダとライブラリの場所を補って `g++` を呼ぶコマンドです。

```bash
mpicxx -std=c++17 -o hello_mpi hello_mpi.cpp
mpiexec -np 2 ./hello_mpi
mpiexec -np 4 ./hello_mpi
```

4 プロセスで動かした出力の例です。

```text
rank 0 / size 4
rank 2 / size 4
rank 1 / size 4
rank 3 / size 4
```

行の順番は実行のたびに変わることがあります。各プロセスが別々に画面へ書き込むからです。
`-np` に計算機のコア数より大きい数を指定すると、OpenMPI は既定では起動を拒みます。
練習で少ないコアの計算機を使うときは、環境変数 `OMPI_MCA_rmaps_base_oversubscribe=yes` を付けて起動します（{ref}`10 節 <edu-ch08-run>`）。

(edu-ch08-env)=
## 2. MISO の Env：MPI の開始と終了

MISO のデモの `main` は、最初に `Env` を作ります（`demo/mhd2d_vortex/src/main.cpp`）。

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

`MPI_Init` も `MPI_Finalize` も、ここには現れません。
どちらも `Env` が受け持ちます（`miso/include/miso/env.hpp`）。
MPI の部分は `miso::mpi::Env` です。抜粋します。
この章の抜粋で `// ...（説明）` の形のコメントは、省いた部分を示すためにこの教材で足したものです。

```cpp
struct Env {
  bool owns_mpi = false;

  void setup(int *argc, char ***argv) {
    int is_mpi_initialized = false;
    MPI_Initialized(&is_mpi_initialized);
    if (!is_mpi_initialized) {
      owns_mpi = true;
      MPI_Init(argc, argv);
    }
    world::comm = MPI_COMM_WORLD;
    MPI_Comm_rank(world::comm, &world::rank);
    MPI_Comm_size(world::comm, &world::size);
    // ...（ノード内のランクを調べる処理。第 9 章で使います）
    world::is_initialized = true;
  }

  Env(int &argc, char **&argv) { setup(&argc, &argv); }
  // ...（引数のないコンストラクタもある）

  ~Env() noexcept {
    // ...（ノード内のコミュニケータを解放する処理）
    int is_mpi_finalized = false;
    MPI_Finalized(&is_mpi_finalized);
    if (owns_mpi && !is_mpi_finalized) {
      MPI_Finalize();
    }
  }
};
```

コンストラクタで `MPI_Init` を呼び、デストラクタで `MPI_Finalize` を呼びます。
第 4 章で学んだ RAII の形です。`main` の最後で `env` の寿命が終わるとデストラクタが動くので、`MPI_Finalize` を書き忘れることがありません。
`MPI_Initialized` で「すでに初期化済みか」を調べ、自分が初期化したとき（`owns_mpi`）だけ終了処理をします。
テストのように、別の場所で MPI を始めた場合にも使えるようにするためです。

ランク番号は、同じファイルの名前空間 `mpi::world` に `inline int rank = -1;` のように宣言した変数に保存します。
関数の外に置いた変数は、プログラムの始めから終わりまで残ります（Fortran のモジュール変数に近いものです）。
`inline` は、複数のファイルから読み込まれても変数が 1 つにまとまるようにする指定です（第 2 章の `inline` 関数と同じ理由です）。

MISO の他の場所では、これらの変数を直接読まず、次の関数を使います。

| 関数 | 返す値 |
|---|---|
| `mpi::rank()` | `MPI_COMM_WORLD` での自分のランク |
| `mpi::size()` | 全体のランク数 |
| `mpi::is_root()` | ランク 0 なら `true`（出力や表示をランク 0 だけで行うときに使う） |
| `mpi::comm()` | 全体のコミュニケータ（`MPI_COMM_WORLD`） |

どの関数も、`Env` を作る前に呼ぶと例外 `std::runtime_error` を投げます。
「`Env` を `main` の最初に作る」という約束を守らなかったことを、実行時に知らせるためです。

`main` で使う `miso::Env` は、`mpi::Env` と、CUDA ビルドのときだけ `cuda::Env`（使う GPU の選択）をまとめたものです。
CPU ビルドでは `mpi::Env` だけが動きます。`cuda::Env` は第 9 章で扱います。

(edu-ch08-shape)=
## 3. ランクの並びと隣のランク：mpi::Shape

### config.yaml の mpi セクション

MISO は 3 次元の格子を、x, y, z の各方向に何個ずつ分けるかを設定ファイルで指定します。
`demo/mhd2d_vortex/config.yaml` の該当部分です。

```yaml
grid:
  i_size: 128
  j_size: 128
  k_size: 1
  margin: 2
  # ...

mpi:
  x_procs: 2
  y_procs: 1
  z_procs: 1

domain:
  periodic:
    x: true
    y: true
    z: true
```

`x_procs: 2` は x 方向に 2 つに分けることを表します。
`x_procs * y_procs * z_procs` が、`mpiexec -np` で指定するプロセス数と等しくなければなりません。
`mpi` セクションには既定値がない（`miso/include/miso/config_defaults.hpp` に `mpi` の項目がない）ので、どのデモの設定ファイルにも書かれています。
他のデモでは、`demo/hd2d_rayleigh_taylor/config.yaml` が y 方向に 2 分割（`y_procs: 2`）、1 次元の衝撃波管のデモ（`demo/hd1d_shock_tube/config/config_x.yaml` など）は分割なし（すべて 1）です。

### MPI_Cart_create と MPI_Cart_shift

この設定を読んでランクの並びを作るのが `mpi::Shape` です（`miso/include/miso/mpi_util.hpp`）。
コンストラクタの中心部分を抜粋します。

```cpp
    x_procs = config["mpi"]["x_procs"].as<int>();
    y_procs = config["mpi"]["y_procs"].as<int>();
    z_procs = config["mpi"]["z_procs"].as<int>();

    int dims[ndims] = {x_procs, y_procs, z_procs};
    int periods[ndims];
    periods[0] = config["domain"]["periodic"]["x"].as<bool>() ? 1 : 0;
    periods[1] = config["domain"]["periodic"]["y"].as<bool>() ? 1 : 0;
    periods[2] = config["domain"]["periodic"]["z"].as<bool>() ? 1 : 0;

    MPI_Cart_create(MPI_COMM_WORLD, ndims, dims, periods, 0, &cart_comm);
    MPI_Comm_rank(cart_comm, &myrank);
    MPI_Comm_size(cart_comm, &n_procs);
    // ...（プロセス数の確認）

    MPI_Cart_coords(cart_comm, myrank, ndims, coord);
    MISO_MPI_CHECK(MPI_Cart_shift(cart_comm, 0, 1, &x_procs_neg, &x_procs_pos));
    MISO_MPI_CHECK(MPI_Cart_shift(cart_comm, 1, 1, &y_procs_neg, &y_procs_pos));
    MISO_MPI_CHECK(MPI_Cart_shift(cart_comm, 2, 1, &z_procs_neg, &z_procs_pos));
```

使っている MPI の関数の役割は次のとおりです。

- `MPI_Cart_create`：全プロセスを `dims` の形（ここでは $2 \times 1 \times 1$）の格子に並べた新しいコミュニケータ `cart_comm` を作ります。`periods` の方向は端と端がつながります（周期境界）。5 番目の引数 `0` は「ランク番号を並べ替えない」指定です。
- `MPI_Cart_coords`：自分のランクが格子のどこにあるか（`coord[0]`, `coord[1]`, `coord[2]`）を求めます。
- `MPI_Cart_shift`：指定した方向（0 が x）に 1 つずれた隣のランクを求めます。負の側が `x_procs_neg`、正の側が `x_procs_pos` に入ります。

周期でない方向の端には隣がありません。そのとき隣のランクとして `MPI_PROC_NULL` が入ります。
`MPI_PROC_NULL` は「相手なし」を表す特別な値で、これを相手にした送受信は何もせずに終わります。
MISO はこの値を、後の節で見るハロー交換と物理境界の判定に使います。

`MISO_MPI_CHECK` は、MPI の関数の戻り値（エラーコード）が成功でなければ、エラーの説明を表示して `MPI_Abort` で全プロセスを止めるマクロです（同じファイルの `check_error`）。

コンストラクタの「プロセス数の確認」は、`x_procs * y_procs * z_procs` と実際のプロセス数が違うときに説明を表示して止める処理です。
ただし OpenMPI 4.1 で確かめると、数が違う場合はその前の `MPI_Cart_create`（または直後の `MPI_Comm_rank`）が MPI のエラーで止まり、この説明は表示されません。
`MPI_Cart_create` や `MPI_Comm_rank` のエラーが出たら、まず `mpi` セクションと `-np` の数が合っているかを確かめてください。

### ランク番号の並び

`MPI_Cart_create` は、ランク番号を「最後の次元（z）がいちばん速く変わる」順に並べます。
x 方向 2 つ、y 方向 2 つに分けた場合、ランクと位置の対応は次のとおりです。

```text
         y
         ^
 coord_y=1 |  rank 1   rank 3
 coord_y=0 |  rank 0   rank 2
           +--------------------> x
             coord_x=0 coord_x=1
```

第 7 章で学んだ C++ の配列の並び（最後の添字が連続）と同じ規則です。Fortran の配列の並びとは逆なので注意してください。
MISO は実行の最初に、この対応を `io.save_dir` の下の `mpi/coords.csv` に書き出します（`Shape::save`）。
$2 \times 2$ に分けたときの中身は次のとおりです。

```text
rank,x,y,z
0,0,0,0
1,0,1,0
2,1,0,0
3,1,1,0
```

`pymiso` はこのファイルを読み、ランクごとの出力をつなげて全体の配列を作ります。

(edu-ch08-grid)=
## 4. 全体格子とランクごとの格子

### 担当するセルとゴーストセル

1 次元で考えます。全体 16 セルを 2 ランクで分けると、各ランクは 8 セルを担当します。

```text
全体の添字   0  1  2  3  4  5  6  7 | 8  9 10 11 12 13 14 15
担当                ランク 0         |         ランク 1
```

MISO の空間微分は 4 次の中心差分で、セル `i` の微分に `i-2` から `i+2` までの値を使います。
担当範囲の端のセルでは、隣のランクが持つ値が 2 つ必要です。
そこで、配列の両端に余分なセルを `margin` 個ずつ用意し、隣から受け取った値を置きます。
これを**ゴーストセル**（ghost cell）または**のりしろ**と呼びます。隣から値を受け取って埋める処理を**ハロー交換**（halo exchange）と呼びます。

`margin: 2` のとき、ランク 1 の x 方向の配列は次のようになります。

```text
ランク 1 のローカルの添字  0  1 | 2  3  4  5  6  7  8  9 | 10 11
役割                     ゴースト |     担当する 8 セル     | ゴースト
全体の添字               6  7 | 8  9 10 11 12 13 14 15 | 16 17
```

右端のゴーストの全体の添字 16, 17 は、周期境界ならランク 0 の 0, 1 にあたります。
周期でない端（物理境界）のゴーストは、隣から受け取らず、境界条件で埋めます（{ref}`8 節 <edu-ch08-physical-boundary>`）。

### Grid の各ランクでの値

MISO の `Grid<Real, backend::Host>`（`miso/include/miso/grid.hpp`）は、**自分のランクの**格子を持ちます。
全体の格子ではありません。コンストラクタの抜粋です。

```cpp
    const int margin = config["grid"]["margin"].as<int>();
    const int i_size_g = config["grid"]["i_size"].as<int>();
    // ...（j_size_g, k_size_g も同様）

    i_size = i_size_g / mpi_shape.x_procs;
    j_size = j_size_g / mpi_shape.y_procs;
    k_size = k_size_g / mpi_shape.z_procs;
    // ...
    const int i_offset = mpi_shape.coord[0] * i_size;
    // ...
    auto x_grid = AxisGrid<Real>(i_size, margin, i_offset, x_provider);
```

設定ファイルの `i_size` は全体のセル数で、`Grid` のメンバ `i_size` は 1 ランクが担当するセル数です。
名前は同じでも意味が違うので、コードでは全体の数に `_g`（global）を付けて区別しています。
`i_offset` は、このランクの最初の担当セルが全体で何番目かを表します。

1 方向ぶんの格子 `AxisGrid` のコンストラクタでは、次の値が決まります。

```cpp
    size = size_;
    stride = size > 1 ? 1 : 0;
    margin = margin_ * stride;
    total = size + 2 * margin;
    begin = margin;
    end = margin + size;
    offset = offset_;
    // ...
    for (int i = 0; i < total; ++i) {
      const int ig = i - begin + offset;
      s[i] = ap.loc(ig);
      // ...
    }
```

`Grid` はこれを `i_total`（ゴースト込みのセル数）、`i_margin`（ゴーストの数）として持ちます。
担当するセルのローカルの添字は `i_margin` 以上 `i_total - i_margin` 未満です。
第 7 章の `Integrator::cfl` などで、ループが `grid.i_margin` から始まっていたのはこのためです。

`size` が 1 の方向（2 次元計算の z 方向など）は `stride` が 0 になり、`margin` も 0 になります。
その方向にはゴーストセルを作らず、分割もしません。

座標 `s[i]` は、ローカルの添字 `i` を全体の添字 `ig` に直してから計算します。
ゴーストセルの `ig` は担当範囲の外（負の値や全体のセル数以上）になることがありますが、等間隔の格子の式でそのまま座標を求めます。

全体のセル数が分割数で割り切れないときは、`check_divisibility` が例外を投げて止まります。
MISO は、全ランクが同じ数のセルを担当する分け方だけを扱います。

### 確認プログラム：MISO の Shape と Grid

MISO のヘッダを使って、各ランクの位置・隣のランク・格子の値を表示します。
第 1 章で作った「自分のプログラムから MISO を使う」CMake 設定（`add_subdirectory` で `MISO::MISO` をリンク）を使い、次の `main.cpp` をビルドします。

```cpp
#include <iostream>
#include <miso/core.hpp>

using namespace miso;
using Real = float;

int main(int argc, char **argv) {
  Env env(argc, argv);                // MPI_Init（デストラクタで MPI_Finalize）
  Config config("shape_check.yaml");  // 設定ファイルを読む
  mpi::Shape mpi_shape(config);       // ランクの並びと隣のランク
  Grid<Real, backend::Host> grid(config, mpi_shape);  // このランクの格子

  namespace bc = miso::boundary_condition;
  const bool x_inner = bc::is_physical_boundary(Direction::X, Side::INNER, mpi_shape);
  const bool y_inner = bc::is_physical_boundary(Direction::Y, Side::INNER, mpi_shape);

  for (int r = 0; r < mpi::size(); ++r) {  // 表示が混ざらないよう 1 ランクずつ
    if (r == mpi::rank()) {
      std::cout << "rank " << mpi::rank() << ": coord = (" << mpi_shape.coord[0]
                << ", " << mpi_shape.coord[1] << ")"
                << ", x_neg/x_pos = " << mpi_shape.x_procs_neg << "/"
                << mpi_shape.x_procs_pos
                << ", y_neg/y_pos = " << mpi_shape.y_procs_neg << "/"
                << mpi_shape.y_procs_pos << "\n"
                << "        i_size = " << grid.i_size
                << ", i_total = " << grid.i_total << ", x[i_margin] = "
                << grid.x[grid.i_margin] << ", y[j_margin] = "
                << grid.y[grid.j_margin] << "\n"
                << "        physical boundary: x inner = " << x_inner
                << ", y inner = " << y_inner << std::endl;
    }
    MPI_Barrier(mpi::comm());
  }
}
```

`MPI_Barrier` は、全ランクがここに来るまで待つ関数です。ランク 0 から順に表示させるために使っています。
`is_physical_boundary` は {ref}`8 節 <edu-ch08-physical-boundary>` で説明します。

設定ファイル `shape_check.yaml` は、実行するフォルダに置きます。x 方向は周期、y 方向は周期でない設定です。
`io.enabled: false` で、ファイルの出力をしないようにしています。

```yaml
io:
  enabled: false

grid:
  i_size: 8
  j_size: 8
  k_size: 1
  margin: 2
  x_min: 0.0
  x_max: 1.0
  y_min: 0.0
  y_max: 1.0
  z_min: 0.0
  z_max: 1.0

mpi:
  x_procs: 2
  y_procs: 2
  z_procs: 1

domain:
  periodic:
    x: true
    y: false
    z: true
```

4 プロセスで動かした出力です（`build` の下に実行ファイルができた場合）。

```bash
mpiexec -np 4 ./build/shape_check
```

```text
rank 0: coord = (0, 0), x_neg/x_pos = 2/2, y_neg/y_pos = -2/1
        i_size = 4, i_total = 8, x[i_margin] = 0.0625, y[j_margin] = 0.0625
        physical boundary: x inner = 0, y inner = 1
rank 1: coord = (0, 1), x_neg/x_pos = 3/3, y_neg/y_pos = 0/-2
        i_size = 4, i_total = 8, x[i_margin] = 0.0625, y[j_margin] = 0.5625
        physical boundary: x inner = 0, y inner = 0
rank 2: coord = (1, 0), x_neg/x_pos = 0/0, y_neg/y_pos = -2/3
        i_size = 4, i_total = 8, x[i_margin] = 0.5625, y[j_margin] = 0.0625
        physical boundary: x inner = 0, y inner = 1
rank 3: coord = (1, 1), x_neg/x_pos = 1/1, y_neg/y_pos = 2/-2
        i_size = 4, i_total = 8, x[i_margin] = 0.5625, y[j_margin] = 0.5625
        physical boundary: x inner = 0, y inner = 0
```

読み取れることをまとめます。

- ランクの位置は {ref}`3 節 <edu-ch08-shape>` の図のとおりです（ランク 1 が `(0, 1)`）。
- x 方向は周期で 2 つに分けたので、左右の隣は同じランクです（ランク 0 の左右はどちらもランク 2）。
- y 方向は周期でないので、下端のランク 0, 2 の `y_neg` と、上端のランク 1, 3 の `y_pos` が `-2` です。`-2` は、この OpenMPI での `MPI_PROC_NULL` の値です（値は MPI の実装によって違うので、コードでは必ず `MPI_PROC_NULL` と比べます）。
- 全体 8 セルを 2 つに分けたので `i_size = 4`、両側に 2 個ずつゴーストを足して `i_total = 8` です。
- 最初の担当セルの中心の座標は、ランクの位置に応じて $0.5/8 = 0.0625$ と $4.5/8 = 0.5625$ です。

`mpiexec -np 2` で動かすと、`x_procs * y_procs * z_procs = 4` と合わないので、`MPI_Cart_create` のエラーで止まります。

### 練習 1：ランクごとの格子の値

`demo/mhd2d_vortex/config.yaml`（`i_size: 128`, `margin: 2`, `x_min: 0.0`, `x_max: 1.0`, `x_procs: 2`）で、`coord[0] = 1` のランクの `grid.i_size`、`grid.i_total`、`i_offset` を求めてください。
さらに、そのランクのローカルの添字 `i = 2` と `i = 0` のセルについて、全体の添字 `ig` と座標 `grid.x[i]` を求めてください。

<details>
<summary>解答・解説</summary>

`i_size = 128 / 2 = 64`、`i_total = 64 + 2 * 2 = 68`、`i_offset = 1 * 64 = 64` です。

`AxisGrid` の `ig = i - begin + offset` で、`begin = margin = 2` です。
`i = 2` は `ig = 2 - 2 + 64 = 64` で、最初の担当セルです。座標は $(64 + 0.5) \times (1/128) = 0.50390625$ です。
`i = 0` は `ig = 62` で、左のゴーストセルです。ランク 0 の最後から 2 番目の担当セルにあたり、座標は $(62 + 0.5)/128 = 0.48828125$ です。

</details>

(edu-ch08-sendrecv)=
## 5. 隣との値の交換：MPI_Sendrecv

### 送信と受信の対応

1 対 1 の通信では、送る側と受け取る側がそれぞれ次のものを指定します。

| 引数 | 意味 |
|---|---|
| 置き場所 | 送る値・受け取る場所の先頭のアドレス（`&u[0]` など） |
| 個数 | 要素の数 |
| 型 | `MPI_DOUBLE`（C++ の `double`）、`MPI_FLOAT`（`float`）など |
| 相手 | 相手のランク |
| タグ | 通信を区別する整数 |
| コミュニケータ | どのプロセスの集まりの中での通信か |

受け取る側が指定した「相手」と「タグ」が、送る側のランクとタグに一致したとき、その送信と受信が対応します。
同じ相手から 2 種類の値を受け取るとき、タグで区別できます。

### 確認プログラム：周期境界での隣との交換

1 次元で各ランクが 4 セルを担当し、両端に 1 個ずつゴーストセルを持つ場合を作ります。
各セルに、その全体の添字を値として入れ、左右の隣と交換します。`sendrecv.cpp` として保存します。

```cpp
#include <iostream>
#include <vector>
#include <mpi.h>

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);
  int rank = 0, size = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  const int n = 4;                    // 各ランクが担当するセル数
  std::vector<double> u(n + 2, -1.0); // u[0] と u[n+1] がゴーストセル
  for (int i = 1; i <= n; ++i) {
    u[i] = rank * n + (i - 1);        // 値 = 全体の添字
  }

  // 周期境界での左右の隣
  const int left = (rank - 1 + size) % size;
  const int right = (rank + 1) % size;

  // (1) 右へ u[n] を送り、左から u[0] へ受け取る
  MPI_Sendrecv(&u[n], 1, MPI_DOUBLE, right, 100,
               &u[0], 1, MPI_DOUBLE, left, 100,
               MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  // (2) 左へ u[1] を送り、右から u[n+1] へ受け取る
  MPI_Sendrecv(&u[1], 1, MPI_DOUBLE, left, 200,
               &u[n + 1], 1, MPI_DOUBLE, right, 200,
               MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  std::cout << "rank " << rank << ": ghost left = " << u[0]
            << ", inner = " << u[1] << ".." << u[n]
            << ", ghost right = " << u[n + 1] << std::endl;

  MPI_Finalize();
  return 0;
}
```

`MPI_Sendrecv` は、1 回の呼び出しで「送る」と「受け取る」を同時に行います。
前半の 5 つの引数が送信、次の 5 つが受信、最後がコミュニケータと受信の詳しい情報の置き場所です。
`MPI_STATUS_IGNORE` は「詳しい情報は要らない」という指定です。

`% size` は割り算の余りで、ランク 0 の左を最後のランクに、最後のランクの右をランク 0 につなぎます（周期境界）。
`(rank - 1 + size)` と `size` を足しているのは、C++ では負の数の余りが負になる（`-1 % 4` は `-1`）からです。

(1) では、全ランクが「右へ送り、左から受け取る」を同時に行います。
ランク 1 が右へ送った値は、ランク 2 が「左から受け取る」として受け取ります。両方ともタグ 100 なので対応します。
(2) は向きを逆にした交換です。

```bash
mpicxx -std=c++17 -o sendrecv sendrecv.cpp
mpiexec -np 2 ./sendrecv
mpiexec -np 4 ./sendrecv
```

4 プロセスの出力です（ランクの順に並べ替えています）。

```text
rank 0: ghost left = 15, inner = 0..3, ghost right = 4
rank 1: ghost left = 3, inner = 4..7, ghost right = 8
rank 2: ghost left = 7, inner = 8..11, ghost right = 12
rank 3: ghost left = 11, inner = 12..15, ghost right = 0
```

各ランクの左のゴーストに左隣の最後のセルの値、右のゴーストに右隣の最初のセルの値が入っています。
ランク 0 の左には、周期境界で最後のランクの値 15 が入りました。

### 送信と受信を別々に書く場合の注意

`MPI_Send`（送る）と `MPI_Recv`（受け取る）を別々に書くこともできます。
このとき、全ランクが先に `MPI_Send` を呼ぶと、送る値が大きい場合に、全員が「相手が受け取るまで待つ」状態になって先へ進めなくなることがあります。
この状態を**デッドロック**（deadlock）と呼びます。
`MPI_Sendrecv` は送受信をまとめて MPI に任せるので、この形の待ち合いを避けられます。

### 練習 2：ゴーストセルに入る値

上の `sendrecv` を 2 プロセスで動かすと、ランク 0 の左右のゴーストに入る値はいくつになるか、それぞれどのランクからどのタグで届くかを説明してください。
さらに、1 プロセス（`mpiexec -np 1`）で動かしたときの出力を予想し、実行して確かめてください。

<details>
<summary>解答・解説</summary>

2 プロセスでは、ランク 0 が全体の 0〜3、ランク 1 が 4〜7 を担当します。
ランク 0 の左隣も右隣もランク 1 です。
左のゴーストには、ランク 1 が (1) で右へ送った最後のセル 7 がタグ 100 で届きます。
右のゴーストには、ランク 1 が (2) で左へ送った最初のセル 4 がタグ 200 で届きます。
相手が同じランクでも、(1) と (2) は別の呼び出しで、タグも違うので取り違えません。

```text
rank 0: ghost left = 7, inner = 0..3, ghost right = 4
rank 1: ghost left = 3, inner = 4..7, ghost right = 0
```

1 プロセスでは `left` も `right` も自分自身（0）になり、自分に送って自分で受け取ります。

```text
rank 0: ghost left = 3, inner = 0..3, ghost right = 0
```

自分の右端の値 3 が左のゴーストに、左端の値 0 が右のゴーストに入ります。
これは周期境界条件そのものです。{ref}`7 節 <edu-ch08-halo>` で見るとおり、MISO は分割しない方向の周期境界も、この「自分との交換」で処理します。

</details>

(edu-ch08-nonblocking)=
## 6. 通信を始めてから完了を待つ：MPI_Isend と MPI_Irecv

MISO のハロー交換は `MPI_Sendrecv` ではなく、`MPI_Isend`、`MPI_Irecv`、`MPI_Waitall` を使います。
`MPI_Isend` と `MPI_Irecv` は、通信を**始める**だけで、完了を待たずに戻ります。
この形を**ノンブロッキング通信**（non-blocking communication）と呼びます（名前の `I` は immediate の頭文字です）。
戻るときに、後で完了を確かめるための札を `MPI_Request` 型の変数に書き込みます。
`MPI_Waitall` は、渡した札の通信がすべて完了するまで待ちます。

前節の (1) と (2) を、ノンブロッキング通信で書き直すと次のようになります。

```cpp
  MPI_Request reqs[4];
  MPI_Isend(&u[n], 1, MPI_DOUBLE, right, 100, MPI_COMM_WORLD, &reqs[0]);
  MPI_Irecv(&u[0], 1, MPI_DOUBLE, left, 100, MPI_COMM_WORLD, &reqs[1]);
  MPI_Isend(&u[1], 1, MPI_DOUBLE, left, 200, MPI_COMM_WORLD, &reqs[2]);
  MPI_Irecv(&u[n + 1], 1, MPI_DOUBLE, right, 200, MPI_COMM_WORLD, &reqs[3]);
  // ここまでは「始めた」だけ。u[0], u[n+1] はまだ読まない
  MPI_Waitall(4, reqs, MPI_STATUSES_IGNORE);
```

`sendrecv.cpp` の 2 つの `MPI_Sendrecv` をこれに置き換えても、出力は同じになります。

4 つの通信を始めてからまとめて待つので、送受信の順番による待ち合いが起きません。
守る約束は 2 つです。

- 送信が完了するまで、送る値（`u[n]`, `u[1]`）を書き換えません。
- 受信が完了するまで、受け取る場所（`u[0]`, `u[n + 1]`）を読み書きしません。

`MPI_Waitall` の後なら、ゴーストセルの値を使って差分計算を始められます。

(edu-ch08-halo)=
## 7. MISO のハロー交換

### HaloExchanger の全体の形

MHD の 9 つの変数のハロー交換は、`miso/include/miso/mhd_halo_exchange.hpp` の `HaloExchanger` が行います。
第 6 章で学んだ部分特殊化で、`backend::Host` 用と `backend::CUDA` 用が別々に書かれています。
ここでは Host 版を読みます。CUDA 版は、値をバッファへ詰める処理を GPU のカーネルで行う点が違います。第 9 章で扱います。

Host 版は、送受信用のバッファを方向と向きごとに持ちます。

```cpp
template <typename Real> struct HaloExchanger<Real, backend::Host> {
  // MPI communication buffers
  Array4D<Real> recv_x_pos, recv_x_neg;
  // ...（y, z と send_* も同様）

  Grid<Real, backend::Host> &grid;
  mpi::Shape &mpi_shape;

  HaloExchanger(Grid<Real, backend::Host> &grid,
                ExecContext<Real, backend::Host> &exec_ctx)
      : recv_x_pos(grid.i_margin, grid.j_total, grid.k_total, n_fields),
        // ...
```

x 方向のバッファの形は `(i_margin, j_total, k_total, n_fields)` です。
x 方向に送るのは端の `i_margin` 枚の面で、それを 9 つの変数（`n_fields = 9`）ぶんまとめて 1 つの配列に詰めます。
変数ごとに 9 回通信するより、まとめて 1 回送る方が通信の回数が減ります。

### 値を詰めて送る

`apply` の x 方向・正の向きの部分です。

```cpp
    if (mpi_shape.x_procs_pos != MPI_PROC_NULL && grid.i_size > 1) {
      for (int i = 0; i < grid.i_margin; ++i) {
        for (int j = 0; j < grid.j_total; ++j) {
          for (int k = 0; k < grid.k_total; ++k) {
            for (int v = 0; v < 9; ++v) {
              send_x_pos(i, j, k, v) =
                  (*vars[v])(grid.i_total - 2 * grid.i_margin + i, j, k);
            }
          }
        }
      }
      MPI_Isend(send_x_pos.data(), send_x_pos.size(), mpi::data_type<Real>(),
                mpi_shape.x_procs_pos, 100, mpi_shape.cart_comm,
                &reqs[req_count++]);
      MPI_Irecv(recv_x_pos.data(), recv_x_pos.size(), mpi::data_type<Real>(),
                mpi_shape.x_procs_pos, 200, mpi_shape.cart_comm,
                &reqs[req_count++]);
    }
```

読み方を順に説明します。

- `vars` は、`apply` の最初で作る 9 つの変数（`ro`, `vx`, ..., `ph`）へのポインタの配列です。`(*vars[v])(i, j, k)` は `v` 番目の変数の `(i, j, k)` の値です。
- 正の側の隣がいて（`MPI_PROC_NULL` でない）、x 方向が 1 セルでない（`i_size > 1`）ときだけ交換します。
- 詰めるのは `i_total - 2 * i_margin` から `i_total - i_margin - 1` まで、つまり担当範囲の右端の `i_margin` 枚です。
- `send_x_pos.data()` はバッファの先頭のポインタ、`send_x_pos.size()` は要素数です。
- `mpi::data_type<Real>()` は、`Real` が `float` なら `MPI_FLOAT`、`double` なら `MPI_DOUBLE` を返す関数テンプレートです（`miso/include/miso/mpi_util.hpp`。第 6 章の特殊化の例）。
- 正の側へはタグ 100 で送り、正の側からはタグ 200 で受け取ります。

負の向きの部分は、詰める範囲が `i_margin` からの `i_margin` 枚（担当範囲の左端）で、負の側へタグ 200 で送り、負の側からタグ 100 で受け取ります。
タグ 100 は「正の向きへ送った値」、タグ 200 は「負の向きへ送った値」を表します。
あるランクが正の側へタグ 100 で送った値は、受け取る側から見ると負の側から届く値なので、`recv_x_neg` がタグ 100 で受け取ります。
前節の確認プログラムの (1)（右へ送るタグ 100）と (2)（左へ送るタグ 200）と同じ対応です。

`req_count++` は、今の `req_count` を添字に使ってから 1 増やす書き方です。始めた通信の札が `reqs[0]`, `reqs[1]`, ... と順に入ります。

### 完了を待って配列へ戻す

両方の向きの通信を始めたら、`MPI_Waitall` で完了を待ち、受け取った値をゴーストセルへ戻します。

```cpp
    if (req_count > 0) {
      MPI_Waitall(req_count, reqs, MPI_STATUSES_IGNORE);
    }
    req_count = 0;

    // positive x-direction
    if (mpi_shape.x_procs_pos != MPI_PROC_NULL && grid.i_size > 1) {
      // ...（v, i, j, k の 4 重ループ）
              (*vars[v])(grid.i_total - grid.i_margin + i, j, k) =
                  recv_x_pos(i, j, k, v);
```

正の側から受け取った値は、右端のゴースト（`i_total - i_margin` から `i_total - 1`）へ入ります。
負の側から受け取った値は、左端のゴースト（`0` から `i_margin - 1`）へ入ります。

値をバッファへ詰める処理を**パック**（pack）、バッファから配列へ戻す処理を**アンパック**（unpack）と呼びます。
CUDA 版の `pack_x_send`, `unpack_x_recv` という名前は、この処理を表しています。

### x, y, z の順に 1 方向ずつ交換する理由

`apply` は、x 方向の交換を完了させてから y 方向、その後に z 方向を交換します。
y 方向のバッファは `(i_total, j_margin, k_total, n_fields)` の形で、x 方向については**ゴーストも含めた**全体（`i_total`）を送ります。
x 方向の交換で埋まったゴーストの値が、y 方向の交換で斜めの隣のランクへ届くので、角（x と y の両方でゴーストになるセル）にも正しい値が入ります。
コードの冒頭のコメント（`Exchange one direction at a time ...`）は、このことを述べています。

### 周期境界と分割しない方向

{ref}`3 節 <edu-ch08-shape>` で見たとおり、周期境界の方向では、端のランクの隣は反対側の端のランクです。
その方向を分割しない（`x_procs: 1`）場合、隣は自分自身になり、ハロー交換は自分との交換になります（練習 2 の 1 プロセスの場合と同じです）。
このため MISO では、周期境界条件を境界条件の関数ではなく、ハロー交換で実現しています。
`demo/mhd2d_vortex` が `mhd::EmptyBoundaryCondition`（何もしない境界条件）を使っているのは、x, y とも周期境界で、ゴーストセルがすべてハロー交換で埋まるからです。

輻射輸送（RT）の計算には、別のハロー交換（`miso/include/miso/rt_host.hpp` の `mpi_exchange_halo`）があります。考え方は同じです。

### 練習 3：送る範囲と受け取る範囲

`i_total = 68`, `i_margin = 2` のランクを考えます。
このランクが x の正の側へ送るのはローカルの添字 `i` がいくつのセルか、それを受け取った正の側の隣のランクはどの添字のセルへ値を入れるかを答えてください。
また、受け取る側がタグ 100 を指定するのは `recv_x_pos` と `recv_x_neg` のどちらかを、理由とともに説明してください。

<details>
<summary>解答・解説</summary>

送るのは `i_total - 2 * i_margin + i`（`i = 0, 1`）、つまり `i = 64, 65` です。担当範囲（2〜65）の右端の 2 枚です。

正の側の隣のランクから見ると、この値は負の側から届きます。
`recv_x_neg` で受け取り、左端のゴースト `i = 0, 1` へ入れます。
こちらのランクの 64, 65 が、隣のランクの 0, 1 になるので、隣の担当範囲の最初のセル（2）のすぐ左に、こちらの最後の 2 セルが並びます。

正の側へ送るときのタグは 100 です。その値を受け取るのは、受け取る側にとって負の側との通信なので、タグ 100 を指定するのは `recv_x_neg` です。
送り手は「正の向きへ送る」、受け手は「負の側から受け取る」と、同じ通信を逆の立場から見ています。

</details>

(edu-ch08-physical-boundary)=
## 8. 物理境界と内部境界

ゴーストセルには 2 種類あります。

- **内部境界**：隣にランクがいる境界。ゴーストは隣のランクの値で埋めます（ハロー交換）。周期境界の端もこちらに入ります。
- **物理境界**：計算領域そのものの端で、周期でない境界。隣のランクはいない（`MPI_PROC_NULL`）ので、ゴーストは境界条件（対称・反対称など）で埋めます。

分割したとき、物理境界に面しているのは端のランクだけです。
内側のランクが境界条件の関数でゴーストを書き換えると、隣から受け取るべき値を壊してしまいます。
そこで、境界条件を適用する前に、自分の面が物理境界かどうかを調べます。
その判定が `boundary_condition::is_physical_boundary` です（`miso/include/miso/boundary_condition.hpp`）。

```cpp
bool is_physical_boundary(const Direction direction, const Side side,
                          const mpi::Shape &mpi_shape) {
  switch (direction) {
  case Direction::X:
    return (side == Side::INNER) ? (mpi_shape.x_procs_neg == MPI_PROC_NULL)
                                 : (mpi_shape.x_procs_pos == MPI_PROC_NULL);
  // ...（Y, Z も同様）
  }
  return false;
}
```

`Side::INNER` は座標の小さい側（負の側）、`Side::OUTER` は大きい側（正の側）です。
その側の隣が `MPI_PROC_NULL` なら物理境界です。
`条件 ? A : B` は、条件が真なら `A`、偽なら `B` を値とする式です。

y 方向が周期でない `demo/hd2d_kelvin_helmholtz/src/main.cpp` の境界条件は、次のように使っています（抜粋）。

```cpp
  void apply(mhd::FieldsView<Real> qq, GridView<const Real> grid) const {
    namespace bc = miso::boundary_condition;
    Backend btag{};

    if (bc::is_physical_boundary(Direction::Y, Side::INNER, mpi_shape)) {
      bc::symmetric(btag, qq.ro, grid, Sign::Pos, Direction::Y, Side::INNER);
      // ...（他の 8 変数も同様。Side::OUTER の側も同じ形）
    }
  }
```

`namespace bc = miso::boundary_condition;` は、長い名前空間に短い別名 `bc` を付ける書き方です。
x 方向は周期境界なので、x 方向の境界条件は書かれていません。

境界条件とハロー交換を呼ぶ順番は、`miso/include/miso/mhd_integrator_host.hpp` の `apply_boundary_condition` で決まっています。

```cpp
  void apply_boundary_condition(const BoundaryCondition &bc,
                                Fields<Real, backend::Host> &qq) {
    bc.apply(qq.view(), grid.const_view());
    halo_exchanger.apply(qq);
  }
```

先に物理境界のゴーストを境界条件で埋め、次にハロー交換で内部境界のゴーストを埋めます。
y 方向の物理境界で埋めた値は、x 方向のハロー交換（`j_total` 全体を送る）で隣のランクにも届きます。
この関数が時間発展のどこで呼ばれるかは、第 11 章で読みます。

{ref}`4 節 <edu-ch08-grid>` の確認プログラムの出力で、`y inner = 1` になったのは y 方向の下端のランク 0 と 2 だけでした。
x 方向は周期境界なので、どのランクでも `x inner = 0` です。

(edu-ch08-allreduce)=
## 9. 全ランクで値をまとめる：MPI_Allreduce

### 集団通信

各ランクが自分の領域で時間刻みの候補 `dt` を求めても、ランクごとに違う `dt` で進むと時刻がずれてしまいます。
全ランクの候補の最小値を求め、全員がそれを使う必要があります。
このように、コミュニケータの全プロセスが参加する通信を**集団通信**（collective communication）と呼びます。
`MPI_Allreduce` は、全ランクの値を演算（最小値、合計など）でまとめ、結果を全ランクに配ります。

### 確認プログラム：最小値と合計

`allreduce.cpp` として保存します。

```cpp
#include <iostream>
#include <mpi.h>

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);
  int rank = 0, size = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  // 各ランクが自分の領域で求めた時間刻みの候補（ここでは作り物の値）
  double dt_local = 0.1 / (rank + 1);
  double dt_global = 0.0;
  MPI_Allreduce(&dt_local, &dt_global, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);

  int one = 1;
  int count = 0;
  MPI_Allreduce(&one, &count, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

  std::cout << "rank " << rank << ": dt_local = " << dt_local
            << ", dt_global = " << dt_global << ", count = " << count
            << std::endl;

  MPI_Finalize();
  return 0;
}
```

引数は、送る値のアドレス、結果を受け取るアドレス、個数、型、演算、コミュニケータの順です。
`MPI_MIN` を `MPI_SUM` に替えると合計になります。2 つ目の呼び出しでは、各ランクの 1 を合計して、ランク数を数えています。

```bash
mpicxx -std=c++17 -o allreduce allreduce.cpp
mpiexec -np 4 ./allreduce
```

```text
rank 0: dt_local = 0.1, dt_global = 0.025, count = 4
rank 1: dt_local = 0.05, dt_global = 0.025, count = 4
rank 2: dt_local = 0.0333333, dt_global = 0.025, count = 4
rank 3: dt_local = 0.025, dt_global = 0.025, count = 4
```

`dt_local` はランクごとに違いますが、`dt_global` は全ランクで同じ最小値になりました。

### Integrator::cfl の dt

第 7 章で読んだ `Integrator::cfl`（`miso/include/miso/mhd_integrator_host.hpp`）は、自分の担当範囲のセルだけで CFL 条件の `dt` を求め、最後に全ランクの最小値をとります。

```cpp
    Real dt_global;
    MPI_Allreduce(&dt, &dt_global, 1, mpi::data_type<Real>(), MPI_MIN,
                  mpi::comm());
    return dt_global;
```

第 7 章の `reduce` は 1 つのプロセスの中の配列の値をまとめ、`MPI_Allreduce` はプロセスごとの結果をさらにまとめます。
2 段階で全体の値を求めると考えると読みやすくなります。

### 全ランクが同じ順序で呼ぶ

集団通信は、コミュニケータの**全ランクが、同じ順序で**呼ぶ必要があります。
1 つのランクだけが呼ばなかったり、別の集団通信を先に呼んだりすると、他のランクは来ない相手を待ち続けて止まります。

`miso/include/miso/mhd_model_base.hpp` の `ModelBase::update` には、この約束を守るための工夫があります。
ソース項が時間刻みの上限 `dt_limit()` を持つ場合の処理です。

```cpp
      Real dt_src = static_cast<Real>(d.src.dt_limit());
      if (!(dt_src > Real(0))) {
        dt_src = Real(-1);
      }
      Real dt_src_g;
      MPI_Allreduce(&dt_src, &dt_src_g, 1, mpi::data_type<Real>(), MPI_MIN,
                    mpi::comm());
      if (dt_src_g <= Real(0)) {
        throw std::runtime_error(/* ...（エラーの説明）... */);
      }
```

あるランクで `dt_limit()` が正でない（不正な）値を返しても、そのランクはすぐに例外を投げず、`-1` にして `MPI_Allreduce` に参加します。
最小値をとると全ランクが `-1` を受け取るので、全ランクが同時に例外を投げます。
1 つのランクだけが先に止まって、他のランクが `MPI_Allreduce` で待ち続けることを防いでいます。

MISO では他に、`Config` のコンストラクタ（`miso/include/miso/config.hpp`）が `MPI_Bcast`（ランク 0 が読んだ設定ファイルの内容を全ランクへ配る）を、`Shape::save` が `MPI_Gather`（全ランクの `coord` をランク 0 に集める）を使っています。

### 練習 4：集団通信を呼ぶ条件

`Integrator::cfl` の `MPI_Allreduce` を、`if (mpi::is_root()) { ... }` の中に移したとします。
2 プロセスで動かしたとき何が起きるかを説明してください（実際に書き換えて動かす必要はありません）。

<details>
<summary>解答・解説</summary>

ランク 0 だけが `MPI_Allreduce` を呼び、ランク 1 は呼びません。
ランク 0 はランク 1 の値が届くのを待ち続け、計算が先へ進みません。
ランク 1 は初期化されていない `dt_global` を返して先へ進み、次のハロー交換や集団通信で、ランク 0 と対応しない通信を待つことになります。
どちらにしても、全ランクが同じ集団通信を同じ順序で呼ぶという約束が破れ、プログラムは止まるか、正しくない結果になります。

ランク 0 だけで行ってよいのは、ファイルへの書き出しや画面への表示のように、通信を含まない処理です。

</details>

(edu-ch08-run)=
## 10. MISO のデモを MPI で動かす

### app_run.sh と OpenMPI の環境変数

デモのビルドと実行の基本は第 1 章で扱いました。
`demo/mhd2d_vortex/app_run.sh` の実行部分を見ると、MPI での起動の仕方が分かります（抜粋）。

```bash
# Number of processes
NUM_PROCS=2
# ...
# Load environment variables
set -a
source "${THIS_DIR}"/../shared/openmpi_config.env
set +a
# ...
mpiexec -np ${NUM_PROCS} "${APP_NAME}" --config="${CONFIG_PATH}"
```

`NUM_PROCS=2` は、`config.yaml` の `x_procs: 2`, `y_procs: 1`, `z_procs: 1` の積と一致しています。
設定ファイルの分割を変えたら、この数も合わせて変えます。

`set -a` から `set +a` の間で読み込んだ変数は、環境変数として `mpiexec` に引き継がれます。
読み込むファイル `demo/shared/openmpi_config.env` の中身は次のとおりです。

```bash
# Environment variables for OpenMPI
# * Suppress errors when using MPI in containers
# Usage (shell): set -a && source ./openmpi_config.env && set +a
# Usage (docker): docker run --env-file openmpi_config.env
OMPI_MCA_pml=ob1
OMPI_MCA_btl=tcp,self,vader
OMPI_MCA_coll=^hcoll
OMPI_MCA_osc=^ucx
OMPI_MCA_hwloc_base_binding_policy=none
```

`OMPI_MCA_` で始まる環境変数は、OpenMPI の内部の部品（MCA：Modular Component Architecture）の選び方を指定します。
コメントにあるとおり、コンテナの中など、特定の通信ライブラリが使えない環境でのエラーを避けるための設定です。
`pml` と `btl` は 1 対 1 通信の層と経路（TCP、自分自身、同じ計算機内の共有メモリ `vader`）、`coll` は集団通信、`osc` は片側通信の部品の指定で、`^` は「除く」を表します。
最後の行は、プロセスを特定の CPU コアに固定しない指定です。

環境によっては、次の 2 つも必要になります。MISO のユニットテストを `ctest` で動かすときに付けるのはこのためです。

- `OMPI_MCA_btl_vader_single_copy_mechanism=none`：同じ計算機内のプロセス間でメモリを直接コピーする機能が使えない環境（コンテナなど）で、その警告やエラーを避けます。
- `OMPI_MCA_rmaps_base_oversubscribe=yes`：計算機のコア数より多いプロセスを起動できるようにします。

いずれも OpenMPI だけの設定で、他の MPI（MPICH など）では効果がありません。
各部品の詳しい説明は、OpenMPI の文書（[Open MPI 4.1 の FAQ](https://www.open-mpi.org/faq/?category=tuning)）にあります。

### 手で起動する

`app_run.sh` を使わずに、手で起動することもできます。
デモをビルドした後、MISO のフォルダで次のように実行します。

```bash
cd /path/to/MISO
set -a; source demo/shared/openmpi_config.env; set +a
mpiexec -np 2 demo/mhd2d_vortex/build/mhd2d_vortex --config=demo/mhd2d_vortex/config.yaml
```

`--config=` で設定ファイルを指定します。出力先 `io.save_dir`（既定は `data/`）は設定ファイルのあるフォルダからの相対パスなので、`demo/mhd2d_vortex/data/` に書かれます。
短く試すときは、設定ファイルを別のフォルダに写して `time.tend` を小さくし（例：`0.02`）、そのファイルを `--config=` に渡します。出力は写したフォルダの `data/` に書かれます。
`tend: 0.02` で動かしたときの画面の表示です。

```text
time =   0.00;  n_step =        0;  n_output =        0
time =   0.01;  n_step =        4;  n_output =        1
time =   0.02;  n_step =        9;  n_output =        2
```

表示はランク 0 だけが行います。

### 出力ファイルとランク

`data/` の下の `mhd/mhd.<出力番号>.<ランク>.bin`（例：`mhd.00000001.00000000.bin`）は、ランクごとの MHD 変数です。各ランクは自分の配列（ゴーストセル込み）だけを書きます。
`grid.bin` は全体の格子で、ランク 0 だけが書きます。
`mpi/coords.csv` は、{ref}`3 節 <edu-ch08-shape>` で見たランクと位置の対応で、`x_procs: 2` なら `0,0,0,0` と `1,1,0,0` の 2 行が並びます。
`pymiso.Data` は `coords.csv` と各ランクのファイルを読み、ゴーストを除いて全体の配列（`ro` など）を組み立てます。
読み手は分割の仕方を意識せずに解析できます。

### 分割の仕方を変えて結果を比べる

領域分割は計算の手順を変えないので、分割の仕方を変えても結果は変わらないはずです。
これは、MPI の部分を書き換えたときの確かめ方としても使えます。
`demo/mhd2d_vortex` を `tend: 0.02` にして、`mpi` セクションを $1 \times 1$（`-np 1`）、$2 \times 1$（`-np 2`）、$2 \times 2$（`-np 4`）の 3 通りにして、別々のフォルダで動かしたとします。
`pymiso` で出力番号 2 の値の差を調べます。

```bash
python3 - run_1x1/data run_2x1/data run_2x2/data <<'EOF'
import sys
import numpy as np
import pymiso

ref = pymiso.Data(sys.argv[1])
ref.load(2)
for d in sys.argv[2:]:
    da = pymiso.Data(d)
    da.load(2)
    for v in ["ro", "vx", "bx", "ei"]:
        print(d, v, float(np.max(np.abs(getattr(ref, v) - getattr(da, v)))))
EOF
```

`run_1x1` などは、それぞれの設定ファイルを置いたフォルダです。
確かめた結果では、どの変数も差は `0.0` で、ビット単位で同じでした。
ハロー交換は値をそのまま写すだけで、`MPI_MIN` による最小値も計算の順序によらず同じ値になるからです。
`MPI_SUM` で浮動小数点数の合計をとる処理があると、足す順序が分割によって変わるので、丸め誤差程度の差が出ることがあります。

### 練習 5：y 方向に分割して動かす

`demo/mhd2d_vortex` の設定ファイルを写し、`tend` を `0.02` に、`mpi` セクションを `x_procs: 1`, `y_procs: 2` に変えて、2 プロセスで動かしてください。
`data/mpi/coords.csv` の中身を確かめ、`x_procs: 2`, `y_procs: 1` で動かした結果と、上の方法で比べてください。
また、ランクごとの出力ファイル `mhd.00000000.00000000.bin` の大きさ（バイト数）を計算で求め、実際の大きさと比べてください。
ファイルは、先頭に要素の大きさを表す 4 バイトの整数があり、その後に 9 変数のゴースト込みの値が `Real = float`（4 バイト）で続きます。

<details>
<summary>解答・解説</summary>

`coords.csv` は次のようになります。y 方向に並ぶので、ランク 1 の y 座標が 1 です。

```text
rank,x,y,z
0,0,0,0
1,0,1,0
```

`x_procs: 2` の結果との差は、どの変数も `0.0` になります。

1 ランクの格子は、x 方向が分割なしで `i_total = 128 + 2 * 2 = 132`、y 方向が `j_total = 64 + 2 * 2 = 68`、z 方向は `k_size = 1` なのでゴーストがなく `k_total = 1` です。

$$
4 + 9 \times 132 \times 68 \times 1 \times 4 = 323140 \ \text{バイト}
$$

`ls -l data/mhd` で表示される大きさと一致します。
`x_procs: 2` のときは `i_total = 68`, `j_total = 132` なので、同じ大きさになります。

</details>

<details>
<summary>コラム：MPI の規格と実装</summary>

MPI は、特定のソフトウェアの名前ではなく、関数の名前・引数・動作を定めた規格です。
規格は MPI Forum が定めていて、最新の文書は [MPI Forum の Web ページ](https://www.mpi-forum.org/docs/) で公開されています。
OpenMPI や MPICH は、この規格を実装したライブラリです。
規格に書かれた関数（`MPI_Sendrecv`, `MPI_Allreduce` など）の意味はどの実装でも同じですが、`MPI_PROC_NULL` の具体的な値や、`OMPI_MCA_` のような実行時の設定は実装ごとに違います。
この章の出力で `MPI_PROC_NULL` が `-2` と表示されたのは、OpenMPI 4.1 での値です。

C++ から使う関数は C 言語用のものです。MPI 2.x の規格には C++ 用の関数（`MPI::COMM_WORLD.Get_rank()` など）もありましたが、MPI 3.0 で削除されました。
現在の C++ のプログラムは、MISO と同じように C 用の関数を呼びます。

</details>

(edu-ch08-summary)=
## 11. まとめ

- MPI は同じプログラムを複数のプロセスで動かし、プロセスはランクで区別します。プロセスの間では、値を明示的に送受信します。
- MISO では `Env` のコンストラクタとデストラクタが `MPI_Init` と `MPI_Finalize` を受け持ちます。ランクは `mpi::rank()` で調べます。
- `config.yaml` の `mpi` セクションで分割数を決め、`mpi::Shape` が `MPI_Cart_create` と `MPI_Cart_shift` でランクの並びと隣のランクを求めます。隣がいなければ `MPI_PROC_NULL` です。
- `Grid` は自分のランクの格子です。担当するセルの両側に `i_margin` 個のゴーストセルを持ち、ゴースト込みの数が `i_total` です。
- `HaloExchanger` は 9 変数をバッファに詰め、`MPI_Isend`/`MPI_Irecv`/`MPI_Waitall` で隣と交換し、x, y, z の順に 1 方向ずつゴーストを埋めます。周期境界もこの交換で処理します。
- 物理境界かどうかは `is_physical_boundary` で調べ、物理境界のゴーストだけを境界条件で埋めます。
- 時間刻みは `MPI_Allreduce`（`MPI_MIN`）で全ランクの最小値に揃えます。集団通信は全ランクが同じ順序で呼びます。

GPU を使う場合のハロー交換と、ランクと GPU の対応づけは第 9 章で扱います。
CPU だけで進める場合は、第 10 章の熱伝導の実習で、この章の 1 次元のハロー交換と `MPI_Allreduce` を自分で書きます。
