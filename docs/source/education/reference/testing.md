(edu-ref-testing)=
# テストと計算結果の比較

MISO を変更したときに、壊していないことを確かめる方法をまとめます。
前半は MISO のユニットテスト（doctest と ctest）の実行と読み方、後半はデモの出力を比べる方法と、`float` の丸め誤差の目安です。
ビルドの手順は {ref}`第 1 章 <edu-ch01>` を前提にします。

- {ref}`比較する対象 <edu-ref-testing-targets>`
- {ref}`ユニットテストを実行する <edu-ref-testing-run>`
- {ref}`テストの読み方 <edu-ref-testing-read>`
- {ref}`デモの出力を比べる <edu-ref-testing-compare>`
- {ref}`float の丸め誤差の目安 <edu-ref-testing-rounding>`
- {ref}`比較条件の記録 <edu-ref-testing-record>`

(edu-ref-testing-targets)=
## 比較する対象

何を比べるかによって、期待する一致の程度が変わります。

| 比較 | 確かめる内容 | 期待する一致 |
|---|---|---|
| 部品の結果と、式や手計算から求めた期待値 | 添字、係数、境界の扱い | 整数なら完全に一致。実数なら丸め誤差程度 |
| 同じ実行ファイル・同じ設定での 2 回の実行 | 結果が実行ごとに変わらないこと | ビット単位で同一 |
| 変更前と変更後（計算を変えないはずの変更） | 書き換えで計算が変わっていないこと | ビット単位で同一、または丸め誤差程度 |
| MPI の分割数・分割方向を変えた実行 | 領域分割と通信 | 全体の配列として比べてビット単位で同一、または丸め誤差程度 |
| CPU 版と GPU 版、コンパイラや最適化の違い | 演算の順序や命令の違いの影響 | 丸め誤差程度（時間発展とともに増えます） |
| 数値解と厳密解 | 離散化（格子・時間刻み）の誤差 | 格子を細かくすると小さくなること |

MISO の PR では、数値結果への影響を「ビット単位で同一」「丸め誤差程度」「離散化誤差」「物理的に変わる」のどれかで書きます（MISO の `dev/notes/rules_ai.md`）。
この資料の方法は、そのどれに当たるかを判断する材料になります。

期待値は、確かめたい関数をそのまま使わずに、式や別の簡単な計算から求めます。
同じ関数で期待値を作ると、関数の誤りが期待値にも入り、比較で見つけられません。

(edu-ref-testing-run)=
## ユニットテストを実行する

### まとめて実行する（ctest）

MISO のユニットテストは `miso/tests/` にあり、MISO 単独でビルドすると一緒に作られます（`ENABLE_TESTING` が既定で `ON`）。
ビルドフォルダは MISO のフォルダの外でも構いません。

```bash
cmake -S /path/to/MISO/miso -B build_miso -DMISO_USE_CUDA=OFF
cmake --build build_miso --parallel 4
cd build_miso
OMPI_MCA_btl_vader_single_copy_mechanism=none \
OMPI_MCA_rmaps_base_oversubscribe=yes \
ctest --output-on-failure
```

```text
      Start  1: test_array1d_cpp
 1/15 Test  #1: test_array1d_cpp .................   Passed    0.01 sec
...
15/15 Test #15: test_parallel_grid_cpp ...........   Passed    2.08 sec

100% tests passed, 0 tests failed out of 15
```

テストの数と名前は MISO の版によって変わります。
`--output-on-failure` は、失敗したテストの出力だけを表示する指定です。成功したテストの出力は表示しません。

`名前=値 コマンド` の形で書いた環境変数は、そのコマンドの間だけ有効です。2 つの環境変数は Open MPI の動作を変えます。

| 環境変数 | 意味 |
|---|---|
| `OMPI_MCA_btl_vader_single_copy_mechanism=none` | 同じ計算機の中のプロセス間通信で、コンテナなどでは使えないことがあるメモリコピーの仕組みを使わないようにします |
| `OMPI_MCA_rmaps_base_oversubscribe=yes` | CPU のコア数より多いプロセスの起動を許します。2 プロセスのテストを、コアの少ない計算機でも動かせます |

手元の計算機で付けなくても動く場合がありますが、MISO の手順（`dev/notes/usage_ai.md`）と GitHub の CI はこの形で実行しています。
複数プロセスのテストは、さらに `miso/shared/mpiexec_with_env.sh` を通して起動され、`miso/shared/openmpi_config.env` の設定も読み込みます。

### テストを選ぶ

| コマンド | 行うこと |
|---|---|
| `ctest -N` | 実行せずに、登録されたテストの一覧を表示します |
| `ctest -R array` | 名前に `array` を含むテストだけを実行します（正規表現） |
| `ctest -R execution -V` | 選んだテストを、実行コマンドと出力をすべて表示して実行します |

ctest に登録されているテストの名前は、ソースのファイル名に `_cpp`（GPU 版は `_cu`）を付けたものです。
`miso/tests/serial/CMakeLists.txt` と `miso/tests/parallel/CMakeLists.txt` は、`test_*.cpp` に当たるファイルを集めて自動で登録します。
新しいテストのファイルを `serial/` に置けば、次の設定のときに登録されます。

### テストの実行ファイルを直接動かす

各テストは独立した実行ファイルで、`build_miso/tests/serial/` と `build_miso/tests/parallel/` にあります。
直接動かすと、doctest のオプションで中身をさらに絞れます。

```bash
./tests/serial/test_execution_cpp --list-test-cases
./tests/serial/test_execution_cpp --test-case="Test reduce 1D CPU" --success
```

| オプション | 意味 |
|---|---|
| `--list-test-cases` | ファイルに含まれるテストケースの名前を表示します |
| `--test-case=名前` | 名前が一致するテストケースだけを実行します。`*` を使えます（例：`"Test reduce*"`） |
| `--success` | 成功した確認も表示します |
| `--no-colors` | 文字色の制御文字を出しません。ログをファイルに残すときに使います |

`parallel/` のテストは MPI の複数プロセスで動かします。ctest と同じ形で起動するには次のようにします。

```bash
/path/to/MISO/miso/shared/mpiexec_with_env.sh -n 2 ./tests/parallel/test_parallel_grid_cpp
```

各プロセスが自分の結果の要約を表示するので、要約が 2 回出ます。すべてのプロセスの要約を確かめます。

GPU 版のテスト（`*.cu`）は `-DMISO_USE_CUDA=ON` のときだけ作られます。{ref}`第 9 章 <edu-ch09>` を参照してください。

(edu-ref-testing-read)=
## テストの読み方

MISO のユニットテストは **doctest** というテスト用のライブラリで書かれています。
doctest はヘッダ 1 つでできていて、ビルドの設定のときに `miso/tests/CMakeLists.txt` の `FetchContent` が GitHub から取得します。

`miso/tests/serial/test_execution.cpp` の最初のテストを読みます（抜粋）。

```cpp
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <miso/array1d.hpp>
#include <miso/array3d.hpp>
#include <miso/execution.hpp>

using namespace miso;

TEST_CASE("Test for_each 1D CPU" * doctest::test_suite("execution")) {
  Range1D range{2, 4};
  Array1D<int, backend::Host> arr(5);

  auto view = arr.view();
  for_each(backend::Host{}, range, [&](int i) { view[i] = i * i; });

  // This also works, but users should use the view interface as API.
  // for_each<backend::Host>(range, [&](int i) { arr[i] = i * i; });

  for (int i = 2; i < 4; ++i)
    REQUIRE(view[i] == i * i);
}
```

| 書き方 | 意味 |
|---|---|
| `#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` | doctest に `main` 関数を作らせる指定です。テストのファイルには `main` を書きません |
| `TEST_CASE("名前" * doctest::test_suite("組"))` | テストケースを 1 つ定義します。`{ }` の中が、そのテストで実行される処理です。`test_suite` はテストケースをまとめる組の名前です |
| `REQUIRE(条件)` | 条件が成り立たなければ失敗とし、そのテストケースの残りを実行しません |
| `CHECK(条件)` | 条件が成り立たなければ失敗として記録し、続きも実行します |

このテストは、添字 2 と 3 だけに `i * i` を入れる `for_each` を実行し、その 2 つの値を確かめています。
整数の計算なので、`==` でぴったり一致することを求めます。
`for_each`、`Range1D`、`Array1D` とラムダ式は {ref}`第 7 章 <edu-ch07>` で説明します。

### 実数を比べる：`doctest::Approx`

実数の計算では、数学的に同じ式でも、計算の順序によって末尾の桁が変わります（{ref}`後の節 <edu-ref-testing-rounding>`）。
そのため実数の比較には、`==` ではなく `doctest::Approx` を使って「十分近い」ことを確かめます。

次は、`float` で 0.1 を 10 回足した結果を、2 つの方法で確かめる例です。
doctest のヘッダは、MISO のテストをビルドしたフォルダの `_deps/doctest-src/` にあります。

```cpp
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

TEST_CASE("Sum of 0.1f" * doctest::test_suite("example")) {
  float s = 0.0f;
  for (int i = 0; i < 10; ++i) {
    s += 0.1f;  // 0.1 を 10 回足す
  }
  CHECK(s == doctest::Approx(1.0));  // 相対 1e-5 程度まで近ければ成功
  CHECK(s == 1.0f);                  // ビット単位で同じなら成功
}
```

```bash
g++ -std=c++17 -I/path/to/build_miso/_deps/doctest-src test_sum.cpp -o test_sum
./test_sum --no-colors
```

```text
test_sum.cpp:10: ERROR: CHECK( s == 1.0f ) is NOT correct!
  values: CHECK( 1 == 1 )

===============================================================================
[doctest] test cases: 1 | 0 passed | 1 failed | 0 skipped
[doctest] assertions: 2 | 1 passed | 1 failed |
[doctest] Status: FAILURE!
```

`Approx` の確認は成功し、`==` の確認は失敗します。`s` は `1.00000012` で、1 とは最後の 1 桁だけ違います。
失敗の表示 `values: CHECK( 1 == 1 )` では両辺が同じに見えますが、これは表示の桁数が少ないためです。
実数の比較が失敗したときは、自分で桁数を増やして値を表示して確かめます。
テストが 1 つでも失敗すると、実行ファイルは 0 以外の終了コードを返し、ctest はそのテストを `Failed` と表示します。

`a == doctest::Approx(b)` は、次の条件が成り立つときに成功します（doctest 2.4.12 の実装）。

$$
|a - b| < \varepsilon \, \bigl(s + \max(|a|, |b|)\bigr)
$$

$\varepsilon$ は `.epsilon(値)` で、$s$ は `.scale(値)` で指定します。
既定は $\varepsilon$ が `float` の計算機イプシロンの 100 倍（約 $1.2\times10^{-5}$）、$s = 0$ で、相対誤差で比べることになります。
`.scale(1.0)` とすると、0 に近い値では $\varepsilon$ を絶対誤差の許容値として比べます。
期待値が 0 の点で相対誤差を使うと、どんな小さな差も失敗になるためです。

### MISO の統合テストの例

`miso/tests/parallel/test_hd1d_shock_tube.cpp` は、MISO で 1 次元の衝撃波管（Sod の問題）を実際に時間発展させるテストです。
2 種類の比較をしています（抜粋）。

```cpp
  CHECK(ro_diff_sum / ro_sum < 5.e-2);
  CHECK(vx_diff_sum / vx_sum < 5.e-2);

  constexpr double single_precision_epsilon = 1e-5;
  for (std::size_t i = 0; i < model_x.grid.i_total; ++i) {
    CHECK(model_x.mhd.qq.ro[i] == doctest::Approx(model_y.mhd.qq.ro[i])
                                      .epsilon(single_precision_epsilon)
                                      .scale(1.0));
```

1 つ目は厳密解との比較です。差の絶対値の和を値の和で割った量が 5% 未満であることを確かめます。
厳密解との差には離散化の誤差が含まれるので、許容値は大きめです。

2 つ目は、同じ問題を x・y・z の方向に置いた計算どうしの比較です。数学的には同じ計算なので、
丸め誤差程度（$\varepsilon = 10^{-5}$）の一致を求めています。

(edu-ref-testing-compare)=
## デモの出力を比べる

MISO の変更が計算結果を変えていないかは、変更前と変更後でデモを実行し、出力を比べて確かめます。

### 比べるための実行

デモの出力先 `io.save_dir`（既定は `data/`）は、`config.yaml` のあるフォルダからの相対パスです。
そこで、実行ごとに別のフォルダを作って `config.yaml` を写し、`--config` でそのファイルを指定すると、出力を分けられます。
`demo/mhd2d_vortex` をビルド済みとして、2 回実行する例です。

```bash
mkdir -p "$HOME/miso-work/compare/run_a" "$HOME/miso-work/compare/run_b"
cd "$HOME/miso-work/compare"
cp /path/to/MISO/demo/mhd2d_vortex/config.yaml run_a/
cp /path/to/MISO/demo/mhd2d_vortex/config.yaml run_b/
set -a
source /path/to/MISO/demo/shared/openmpi_config.env
set +a
mpiexec -np 2 /path/to/MISO/demo/mhd2d_vortex/build/mhd2d_vortex --config=run_a/config.yaml
mpiexec -np 2 /path/to/MISO/demo/mhd2d_vortex/build/mhd2d_vortex --config=run_b/config.yaml
```

`set -a` から `set +a` までは、`openmpi_config.env` に書かれた環境変数を、このターミナルで有効にする操作です（`app_run.sh` も同じことをしています）。

デモの `config.yaml` は `io.continue: true` なので、出力先にすでに結果があると、その続きから計算します。
比べるための実行は、毎回空のフォルダで行います。

変更前と変更後を比べるときは、それぞれの MISO でビルドした実行ファイルを使います。
同じ MISO のフォルダで比べる場合は、変更前の実行ファイルを別の名前で写しておくか、ビルドフォルダを分けます。

### ビット単位の比較（`cmp`, `diff -rq`）

2 つのファイルが 1 バイトも違わないかは `cmp` で確かめます。

```bash
cmp run_a/data/mhd/mhd.00000080.00000000.bin run_b/data/mhd/mhd.00000080.00000000.bin
```

同じなら何も表示せず、違えば最初に違うバイトの位置を表示します。フォルダ全体は `diff -rq` で比べられます。

```bash
diff -rq run_a/data run_b/data
```

何も表示されなければ、すべての出力がビット単位で同一です。

ビット単位の比較は、出力の並びが同じときだけ使えます。
MPI の分割数や分割方向を変えると、各ランクのファイル（`mhd.<出力番号>.<ランク>.bin`）が受け持つ範囲が変わるので、
中身の値が同じでもファイルは一致しません。その場合は、次のように全体の配列に組み立ててから比べます。

### 全体の配列で比べる（pymiso）

`pymiso.Data` は、各ランクのファイルを読んで全体の配列に組み立てます（ゴーストセルは除かれます）。
次のスクリプトは、2 つの出力の最後の出力番号どうしで、変数ごとに差の最大値を代表値（その変数の絶対値の最大）で割った量を表示します。
期待値が 0 になる点もあるので、点ごとの相対誤差ではなく、代表値で割っています。

```python
import sys

import numpy as np

import pymiso

dir_a, dir_b = sys.argv[1], sys.argv[2]
a = pymiso.Data(data_dir=dir_a)
b = pymiso.Data(data_dir=dir_b)
n = min(a.n_output, b.n_output)  # 両方にある最後の出力
a.load(n)
b.load(n)
print(f"n_output = {n}, time = {a.time.time:.6g}, {b.time.time:.6g}")

for var in ["ro", "vx", "vy", "vz", "bx", "by", "bz", "ei"]:
    qa = getattr(a, var).astype(np.float64)
    qb = getattr(b, var).astype(np.float64)
    scale = np.max(np.abs(qa))  # 代表値（最大の大きさ）で割る
    if scale == 0.0:
        print(f"{var}: 両方 0 の変数です")
        continue
    err = np.max(np.abs(qa - qb)) / scale
    print(f"{var}: max|a-b|/max|a| = {err:.3e}")
```

`compare.py` として保存し、{ref}`第 1 章 <edu-ch01-demo>` で pymiso を入れた仮想環境で実行します。

```bash
python compare.py run_a/data run_b/data
```

`a.time` は時刻のファイルを扱うオブジェクトで、時刻の値は `a.time.time` です。
差は `float64` に直してから計算します。`float32` のまま引き算すると、差そのものにも丸めが入るためです。
`vz` と `bz` は 2 次元のこの問題ではすべて 0 なので、代表値で割れません。

### 比較の例

`demo/mhd2d_vortex` の既定の設定（128×128、時刻 0.8 まで 354 ステップ、`Real` は `float`）で、条件を変えて比べた結果です
（Ubuntu 24.04、GCC 13.3、Open MPI 4.1）。

| 比べたもの | `diff -rq` | `compare.py`（時刻 0.8） |
|---|---|---|
| 同じ実行ファイルで 2 回実行 | 差なし | すべて 0 |
| Release と Debug でビルドした実行ファイル | 差なし | すべて 0 |
| MPI の分割を x 方向 2 つから y 方向 2 つに変更 | `mhd.*.bin` がすべて違う（`mpi/coords.csv` も） | すべて 0 |
| `-DCMAKE_CXX_FLAGS=-march=native` でビルドした実行ファイル | 初期値（出力番号 0）以外の `mhd.*.bin` と、一部の時刻のファイルが違う | 変数によって $1.7\times10^{-6}$〜$5.7\times10^{-6}$ |

分割を変えた計算は、ファイルとしては違っても、全体の配列としてはビット単位で同一でした。
`-march=native` は、使っている CPU 固有の命令（積和演算をまとめる FMA など）をコンパイラに使わせる指定です。
演算の丸め方が変わるため、結果は丸め誤差程度だけ変わります。CFL 条件で決まる時間刻みも末尾の桁が変わり、出力の時刻が 7 桁目で違うものがありました。この比較での差の時間変化は次のとおりでした。

| 出力番号 | 時刻 | `ro` の差（`compare.py` の値） |
|---|---|---|
| 1 | 0.01 | $5.9\times10^{-7}$ |
| 10 | 0.10 | $9.3\times10^{-7}$ |
| 40 | 0.40 | $1.6\times10^{-6}$ |
| 80 | 0.80 | $1.7\times10^{-6}$ |

Release と Debug が一致するかどうかは、コンパイラや計算機によって変わります。上の結果は一例です。

(edu-ref-testing-rounding)=
## float の丸め誤差の目安

MISO のデモとテストは `using Real = float;`（単精度）で計算しています。
実数は有限の桁で表すので、計算のたびに結果が表せる値に丸められます。
1 と、その次に大きい表せる数との差を**計算機イプシロン**（machine epsilon）と呼びます。

| 型 | 計算機イプシロン | 10 進でおよその有効桁 |
|---|---|---|
| `float`（単精度） | $2^{-23} \approx 1.19\times10^{-7}$ | 7 桁 |
| `double`（倍精度） | $2^{-52} \approx 2.22\times10^{-16}$ | 16 桁 |

1 回の四則演算での丸めの誤差は、相対的に計算機イプシロンの半分以下です。
上の例の `0.1f` を 10 回足した結果は `1.00000012` で、1 との差 $1.19\times10^{-7}$ は `float` の計算機イプシロン 1 つ分です。

差を判断するときの目安は次のとおりです。

| 相対的な差 | 判断の目安（`float` の場合） |
|---|---|
| 0 | ビット単位で同一。計算の順序も変わっていません |
| $10^{-7}$〜$10^{-5}$ 程度 | 丸め誤差程度。演算の順序や命令の違い（加算の順序、FMA、数学関数の実装、CPU と GPU の違い）で起こります |
| それより大きい | 計算の内容が変わった可能性があります。入力、格子、時間刻み、境界条件、変更した式を確かめます |

これは目安で、決まった許容値ではありません。
丸め誤差は時間発展のステップごとに積み重なり、乱流のように小さな差が育つ流れでは、長い時間の後に大きな差になることがあります。
そのような比較では、早い時刻で差が丸め誤差程度であることを確かめ、差が時間とともにどう増えるかも見ます。
桁落ち（近い値どうしの引き算で有効桁が減ること）のある式では、差が計算機イプシロンより大きくなることもあります。

(edu-ref-testing-record)=
## 比較条件の記録

後で同じ比較をやり直せるように、次を記録しておきます。

- MISO の版（`git log -1 --oneline` の表示）と、変更の内容
- コンパイラとその版、ビルド種別、CPU 版か GPU 版か
- 使ったデモと `config.yaml`、MPI のプロセス数と分割
- 比べた出力番号と変数、比較の方法（`cmp`、`compare.py` など）と得られた値

MISO に PR を出すときは、実行したコマンドと比べた数値を PR の本文に書きます（MISO の `dev/notes/rules_ai.md`）。
