(edu-practice-heat1d)=
# 実習 heat1d：熱伝導の計算

{ref}`第 10 章 <edu-ch10>` で説明した 1 次元熱伝導の計算を、自分で書きます。
前の課題で書いたプログラムを、次の課題へ広げます。

1. {ref}`逐次版：CPU で熱伝導を解く <edu-practice-heat1d-01>`
2. {ref}`MPI 版：領域を分けて解く <edu-practice-heat1d-02>`
3. {ref}`GPU 版：配列とループを GPU へ移す <edu-practice-heat1d-03>`

式と確かめ方の考え方は第 10 章、配列と `for_each`・`reduce` は {ref}`第 7 章 <edu-ch07>`、
MPI は {ref}`第 8 章 <edu-ch08>`、GPU は {ref}`第 9 章 <edu-ch09>` を参照してください。
Ubuntu・Mac の CPU だけで、課題 1・2 と、課題 3 の CPU 用ビルドまで進められます。

## 共通の設定

3 つの課題で、次の問題と入力を使います。

| 項目 | 値 |
|---|---|
| 方程式 | $\partial u/\partial t = \kappa\,\partial^2 u/\partial x^2$、$\kappa = 1$ |
| 領域と境界条件 | $0 < x < 1$、両端で $u = 0$ |
| 初期条件 | $u(x, 0) = \sin(\pi x)$ |
| 解析解 | $u(x, t) = e^{-\pi^2\kappa t}\sin(\pi x)$ |
| 終了時刻 | $t_\mathrm{end} = 0.1$ |
| 格子 | セル数 $N$（既定 64）、$\Delta x = 1/N$、セル中心 $x_j = (j + 1/2)\Delta x$ |
| ゴーストセル | 両端に 1 つずつ（配列の大きさは物理セル数 + 2） |
| 時間刻み | $n_\mathrm{steps} = \lceil t_\mathrm{end}/(r_\mathrm{max}\Delta x^2/\kappa)\rceil$、$\Delta t = t_\mathrm{end}/n_\mathrm{steps}$、$r_\mathrm{max}$ の既定は 0.4 |
| 値の型 | `using Real = double;` |

プログラムは、コマンドライン引数で $N$ と $r_\mathrm{max}$ を受け取れるようにします（省略したら 64 と 0.4）。
`main(int argc, char **argv)` の `argv[1]` を `std::atoi`、`argv[2]` を `std::atof`（`<cstdlib>`）で数に直します。

```bash
./heat1d_serial          # N = 64, r_max = 0.4
./heat1d_serial 128      # N = 128
./heat1d_serial 64 0.6   # 安定条件を超える r で試す
```

各課題のプログラムは、最後に次を表示します。

```text
N = 64, steps = 1024, r = 0.4000
max error (discrete) = ...
max error (exact)    = ...
```

`max error (discrete)` は離散解 $G^{n}\sin(\pi x_j)$（$G = 1 - 4r\sin^2(\pi\Delta x/2)$）との、
`max error (exact)` は解析解との、全物理セルでの最大の差です。

さらに、全物理セルの $x_j$ と $u_j$ を 1 行に 1 セルずつ、空白で区切ってファイルに書き出します。
ファイルどうしをビット単位で比べるため、17 桁で書きます。
`std::ofstream`（`<fstream>`）に `std::scientific` と `std::setprecision(17)`（`<iomanip>`）を指定すると、`1.23456789012345678e-01` の形で書けます。

## ビルドの設定

{ref}`第 1 章 <edu-ch01>` の「自分のプログラムから MISO を使う最小の CMake 設定」を使います。
作業用のフォルダ（たとえば `heat1d/`）に `CMakeLists.txt` とソースを置き、MISO の場所を `-DMISO_DIR=/path/to/MISO` で渡します。

```bash
cmake -B build -S . -DMISO_DIR=/path/to/MISO
cmake --build build -j
```

実行ファイルは課題ごとに 1 つずつ（`heat1d_serial`、`heat1d_mpi`、`heat1d_gpu`）作り、それぞれ `MISO::MISO` をリンクします。
MISO を `add_subdirectory` で取り込むと MPI と yaml-cpp も一緒にリンクされるので、MPI のための設定を別に足す必要はありません。
GPU 用のビルドに必要な追加の設定は {ref}`課題 3 <edu-practice-heat1d-03>` で説明します。

MPI のプログラムは `mpiexec -np 2 ./build/heat1d_mpi` のように実行します。
コンテナの中などで OpenMPI がエラーを出すときは、{ref}`第 8 章 <edu-ch08>` で説明した環境変数（`demo/shared/openmpi_config.env`）を設定します。

## 記録すること

各課題で、次を記録しておくと、後の課題で比べられます。

- $N$ ごとの `max error (exact)` と、そこから求めた収束次数
- `max error (discrete)` の大きさ
- 書き出したファイル（逐次版の結果は、MPI 版・GPU 版と比べるときに使います）

```{toctree}
:hidden:

01_serial
02_mpi
03_gpu
```
