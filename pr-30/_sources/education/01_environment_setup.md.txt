(edu-ch01)=
# 1. 開発環境と CMake

この章では、手元の計算機で C++ のプログラムをコンパイルして動かし、MISO をビルドして実行できるようにします。
Ubuntu と Mac で道具を揃え、コンパイルとリンクの仕組み、CMake の使い方、MISO のビルド・テスト・デモの実行、
自分のプログラムから MISO を使う設定までを順に確かめます。
C++ の文法は {ref}`第 2 章 <edu-ch02>` から学びます。この章のプログラムは、文法を気にせずそのまま保存して構いません。

- {ref}`必要な道具 <edu-ch01-tools>`
- {ref}`Ubuntu での準備 <edu-ch01-ubuntu>`
- {ref}`Mac での準備 <edu-ch01-mac>`
- {ref}`MISO を取得する <edu-ch01-get-miso>`
- {ref}`コンパイルとリンク <edu-ch01-compile>`
- {ref}`CMake の仕組み <edu-ch01-cmake>`
- {ref}`MISO のビルドとユニットテスト <edu-ch01-miso-build>`
- {ref}`デモの実行と出力の読み込み <edu-ch01-demo>`
- {ref}`自分のプログラムから MISO を使う <edu-ch01-own-project>`
- {ref}`うまくいかないとき <edu-ch01-trouble>`
- {ref}`GPU を使う段階になったら <edu-ch01-gpu>`

(edu-ch01-tools)=
## 必要な道具

この教材は CPU だけで進められます。GPU がなくても、C++ の練習、MISO の配列と並列ループ、MPI による領域分割、
1 次元熱伝導の実習、MISO のデモの実行まで行えます。

| 道具 | この教材での役割 |
|---|---|
| C++ コンパイラ（`g++` または `clang++`） | C++ のソースを機械語に翻訳します。MISO は C++17 を使います |
| CMake | 作るもの・使うソース・必要なライブラリを `CMakeLists.txt` から読み、ビルドの手順を作ります。MISO は 3.18 以上を求めます |
| MPI（Open MPI） | 複数のプロセスでデータをやり取りするライブラリと、実行コマンド `mpiexec` です |
| yaml-cpp | 設定ファイル `config.yaml`（YAML 形式）を読むライブラリです |
| git | MISO を取得し、更新を取り込みます |
| Python と pymiso | 計算結果を読み込んで図にします。pymiso は MISO に含まれる Python パッケージです |

**MPI**（Message Passing Interface）は、同じプログラムを複数のプロセスで動かし、プロセスの間でデータを送受信するための規格です。
MISO は計算領域を分けて各プロセスに担当させるために MPI を使います。詳しくは {ref}`第 8 章 <edu-ch08>` で説明します。
1 台の計算機の中で複数のプロセスを起動して練習できます。

(edu-ch01-ubuntu)=
## Ubuntu での準備

Ubuntu 24.04 LTS を基準に説明します。「端末」を開き、次を実行してください。

```bash
sudo apt update
sudo apt install build-essential cmake git openmpi-bin libopenmpi-dev \
  libyaml-cpp-dev python3 python3-venv
```

`sudo` は、管理者の権限でソフトウェアを入れるためのコマンドです。パスワードを入力しても画面には文字が出ません。
共有の計算機でインストールの権限がない場合は、管理者が用意した同等の環境を使います。

| パッケージ | 入るもの |
|---|---|
| `build-essential` | C++ コンパイラ GCC（`g++`）と、ビルドに使う `make` など |
| `cmake` | CMake |
| `git` | git |
| `openmpi-bin`・`libopenmpi-dev` | MPI の実行コマンド（`mpiexec`）と、コンパイルに必要なヘッダ・ライブラリ |
| `libyaml-cpp-dev` | yaml-cpp のヘッダとライブラリ |
| `python3`・`python3-venv` | Python と、Python の仮想環境（後述）を作る機能 |

入ったことを確かめます。

```bash
g++ --version
cmake --version
mpiexec --version
```

GCC、CMake、Open MPI のバージョンが表示されれば準備完了です。

(edu-ch01-mac)=
## Mac での準備

Homebrew が対応している macOS を使います。Apple Silicon と Intel Mac では Homebrew の保存先が違うため、
以下では `brew --prefix` で自分の環境の場所を調べます。

### コマンドライン開発ツール

「ターミナル」を開き、次を実行します。画面の案内に沿ってインストールを終えます。
「すでにインストールされています」と表示された場合は、そのまま次へ進めます。

```bash
xcode-select --install
```

これで Apple の C++ コンパイラ（Clang、コマンド名 `clang++`。`c++` や `g++` という名前でも呼べます）が入ります。
MISO の CPU 版は、このコンパイラでビルドします。

### Homebrew で道具を入れる

Homebrew は、開発用のソフトウェアをまとめて入れ・更新する道具です。`brew --version` が動かない場合は、
[Homebrew 公式サイト](https://brew.sh/) に掲載されているインストールコマンドを実行し、
終了時に表示される **Next steps** のコマンドも実行してください（ターミナルから `brew` を見つけられるようにする設定です）。
その後、次を実行します。

```bash
brew install git cmake open-mpi yaml-cpp python
```

入ったことを確かめます。

```bash
clang++ --version
cmake --version
mpiexec --version
brew --prefix yaml-cpp
```

Homebrew で入れたライブラリは `brew --prefix` が表示する場所（Apple Silicon では通常 `/opt/homebrew`）の下にあります。
CMake は、`PATH` に入っている Homebrew の `bin` から、この場所も探します。

(edu-ch01-get-miso)=
## MISO を取得する

作業用のフォルダを決め、MISO を取得します。以下では `$HOME/miso-work` を使いますが、別の場所でも構いません。

```bash
mkdir -p "$HOME/miso-work"
cd "$HOME/miso-work"
git clone https://github.com/is-isee/MISO.git
```

これで `$HOME/miso-work/MISO` ができます。この教材では、MISO を置いたフォルダを `/path/to/MISO` と書きます。
コマンドを入力するときは、自分の保存先の**絶対パス**（`/` で始まるパス。この例では `$HOME/miso-work/MISO`）に置き換えてください。

MISO のフォルダの主な中身は次のとおりです。

| 場所 | 中身 |
|---|---|
| `miso/include/miso/` | MISO 本体のヘッダファイル（`.hpp`）。計算の部品はすべてここにあります |
| `miso/CMakeLists.txt` | MISO 本体と、そのユニットテストのビルド設定 |
| `miso/tests/` | ユニットテスト（`serial/` は 1 プロセス、`parallel/` は MPI の複数プロセス） |
| `demo/` | デモ問題。`demo/mhd2d_vortex/` などの問題ごとのフォルダに、`src/main.cpp` と `config.yaml` があります |
| `pymiso/` | 計算結果を読む Python パッケージ |
| `dev/notes/` | 開発のルールと手順 |

(edu-ch01-compile)=
## コンパイルとリンク

### コンパイラで実行ファイルを作る

Fortran と同じく、C++ のソースはコンパイラで**実行ファイル**に変換してから動かします。
作業用のフォルダを作り、次の内容を `hello.cpp` という名前で保存してください。

```bash
mkdir -p "$HOME/miso-work/hello"
cd "$HOME/miso-work/hello"
```

```cpp
#include <cmath>
#include <iostream>

int main() {
  const double x = 2.0;
  std::cout << "sqrt(" << x << ") = " << std::sqrt(x) << '\n';
  return 0;
}
```

`main` はプログラムの入口となる関数で、Fortran の `program` に当たります。
コンパイルして実行します（Mac では `g++` を `clang++` に置き換えます）。

```bash
g++ -std=c++17 -O2 hello.cpp -o hello
./hello
```

```text
sqrt(2) = 1.41421
```

`-std=c++17` は C++17 の規格で翻訳する指定、`-O2` は最適化の強さ、`-o hello` は実行ファイルの名前です。

### コンパイルとリンクの 2 段階

上のコマンドは、実際には 2 つの段階をまとめて行っています。

1. **コンパイル**：`.cpp` を 1 つずつ翻訳し、**オブジェクトファイル**（`.o`）を作ります。
2. **リンク**：オブジェクトファイルと、使っているライブラリの中身を結び付け、実行ファイルを作ります。

段階を分けて実行することもできます。`-c` は「コンパイルだけを行う」指定です。

```bash
g++ -std=c++17 -O2 -c hello.cpp -o hello.o   # コンパイル：hello.o ができる
g++ hello.o -o hello                          # リンク：hello ができる
```

ここでの「オブジェクトファイル」はコンパイル結果のファイルを指す用語で、クラスの実体（オブジェクト）とは別の意味です。

### ヘッダと `#include`

`#include <cmath>` は、`cmath` という**ヘッダファイル**の中身をその場所に読み込む指示です。
ヘッダには、関数や型の**宣言**（名前と引数の型など、使い方の情報）が書かれています。
コンパイラは宣言を見て、`std::sqrt(x)` の呼び出しが正しいかを確かめます。
Fortran で `use` したモジュールの情報を見て引数を確かめるのに近い役割です。

ヘッダは決まったフォルダから探されます。標準ライブラリのヘッダはコンパイラが場所を知っていますが、
それ以外のヘッダは `-I フォルダ` で探す場所を足します。たとえば MISO のヘッダは `#include <miso/core.hpp>` のように書くので、
`/path/to/MISO/miso/include` を `-I` で渡すと `miso/core.hpp` が見つかります。

宣言だけでは、関数の**実体**（計算の中身）はありません。実体がライブラリの中にある場合は、リンクの段階でそのライブラリを指定します。
yaml-cpp を使う短いプログラムで確かめます。次を `yaml_check.cpp` として保存してください。

```cpp
#include <iostream>

#include <yaml-cpp/yaml.h>

int main() {
  const YAML::Node node = YAML::Load("grid: {i_size: 128}");
  std::cout << "i_size = " << node["grid"]["i_size"].as<int>() << '\n';
  return 0;
}
```

`-lyaml-cpp` は「リンクのときに `libyaml-cpp` というライブラリを使う」指定です。

```bash
g++ -std=c++17 yaml_check.cpp -o yaml_check -lyaml-cpp
./yaml_check
```

```text
i_size = 128
```

Mac では Homebrew の場所を `-I` と `-L`（ライブラリを探すフォルダ）で教えます。

```bash
clang++ -std=c++17 yaml_check.cpp -o yaml_check \
  -I"$(brew --prefix)/include" -L"$(brew --prefix)/lib" -lyaml-cpp
```

MISO は**ヘッダオンリー**のライブラリです。計算の部品はすべてヘッダ（`miso/include/miso/*.hpp`）に実体ごと書かれていて、
MISO 自身を前もってライブラリにコンパイルしておく必要はありません。
ただし、MISO が使う MPI と yaml-cpp は、ヘッダの場所とリンクするライブラリの指定が必要です。
MPI については、Open MPI が用意する `mpicxx` というコマンドが、MPI のヘッダとライブラリの指定を足して `g++`（Mac では `clang++`）を呼びます。
`mpicxx --showme` で、足される指定を表示できます（Open MPI の場合）。

### 練習 1-1

`yaml_check.cpp` を、`-lyaml-cpp` を付けずにビルドしてください。
エラーがコンパイルとリンクのどちらの段階で起きたかを、表示から判断して説明してください。

<details>
<summary>解答・解説</summary>

```bash
g++ -std=c++17 yaml_check.cpp -o yaml_check
```

Ubuntu では次のような表示が出ます（一部を抜粋）。

```text
yaml_check.cpp:(.text+0x34): undefined reference to `YAML::Load(char const*)'
```

`undefined reference`（未定義の参照）は**リンク**の段階のエラーです。
`#include <yaml-cpp/yaml.h>` で宣言は見えているので、コンパイルは通ります。
リンクのときに `YAML::Load` の実体を持つライブラリが指定されていないため、実行ファイルを作れません。
Mac の Clang では `Undefined symbols for architecture ...` と表示されます。これもリンクのエラーです。

一方、ヘッダが見つからないときは `fatal error: yaml-cpp/yaml.h: No such file or directory` のように表示されます。
これはコンパイルの段階のエラーで、`-I` の指定かパッケージのインストールを確かめます。

</details>

(edu-ch01-cmake)=
## CMake の仕組み

### 設定・ビルド・実行の 3 段階

ソースやライブラリが増えると、コンパイルとリンクのコマンドを手で書くのは大変です。
Fortran でも使われる `make` は、`Makefile` に書かれた手順に従ってビルドします。
**CMake** は、作るもの・使うソース・必要なライブラリを `CMakeLists.txt` に書くと、その計算機に合った `Makefile` などを生成します。
CMake はビルドの手順を組み立てる道具で、C++ の翻訳そのものは GCC や Clang が行います。

| 段階 | コマンドの形 | 行うこと |
|---|---|---|
| 設定・生成 | `cmake -S . -B build` | `CMakeLists.txt` を読み、コンパイラやライブラリを探し、ビルドの手順を `build/` に生成します |
| ビルド | `cmake --build build` | 生成した手順でコンパイルとリンクを行います |
| 実行 | `./build/hello` | できた実行ファイルを動かします |

`-S .` は「ソース（`CMakeLists.txt`）は今いるフォルダにある」、`-B build` は「生成物は `build/` に置く」という指定です。
生成物をソースと別のフォルダに置くので、`build/` を消せばいつでもやり直せます。
設定の結果は `build/CMakeCache.txt` に記録され、次のビルドでも使われます。

### 最小の `CMakeLists.txt`

`hello.cpp` と `yaml_check.cpp` を保存したフォルダに、次の `CMakeLists.txt` を保存します。

```cmake
cmake_minimum_required(VERSION 3.18)
project(hello LANGUAGES CXX)

find_package(yaml-cpp REQUIRED)

add_executable(hello hello.cpp)
target_compile_features(hello PRIVATE cxx_std_17)

add_executable(yaml_check yaml_check.cpp)
target_compile_features(yaml_check PRIVATE cxx_std_17)
target_link_libraries(yaml_check PRIVATE yaml-cpp::yaml-cpp)
```

`CMakeLists.txt` は CMake 用の言語で、`命令(引数 ...)` の形で書きます。

| 命令 | 役割 |
|---|---|
| `cmake_minimum_required` | 必要な CMake の最低バージョンを指定します |
| `project(... LANGUAGES CXX)` | プロジェクトに名前を付け、C++ のコンパイラを使うことを伝えます |
| `find_package(yaml-cpp REQUIRED)` | yaml-cpp を探し、その使い方（ヘッダの場所、ライブラリ）を取り込みます。`REQUIRED` は「見つからなければ止める」指定です |
| `add_executable(hello hello.cpp)` | `hello.cpp` から実行ファイル `hello` を作る**ターゲット**を定義します |
| `target_compile_features(... cxx_std_17)` | このターゲットに C++17 以上が必要だと伝えます |
| `target_link_libraries(yaml_check PRIVATE yaml-cpp::yaml-cpp)` | `yaml_check` に yaml-cpp を結び付けます |

**ターゲット**（target）は、実行ファイルやライブラリなど、ビルドで作るものに付ける名前です。
ソースや設定をターゲットごとにまとめるので、1 つの `CMakeLists.txt` で複数のプログラムを区別して作れます。

`yaml-cpp::yaml-cpp` は `find_package` が用意するターゲットで、ライブラリの場所だけでなく、ヘッダの場所などの情報も持っています。
このため `target_link_libraries` は、リンクの指定に加えて、コンパイルに必要な `-I` などもまとめて伝えます。
手で `-I` や `-l` を並べる必要がなくなるのが、CMake を使う大きな利点です。

`PRIVATE` は、その設定をこのターゲット自身だけに使う指定です。
ライブラリ側のターゲットで `PUBLIC` や `INTERFACE` を使うと、そのライブラリを使う側のターゲットにも設定が伝わります（MISO はこの仕組みを使います）。
C++ のクラスの `private`・`public`（{ref}`第 4 章 <edu-ch04>`）とは別のものです。

設定・ビルド・実行します。

```bash
cmake -S . -B build
cmake --build build --parallel 4
./build/hello
./build/yaml_check
```

`--parallel 4` はコンパイルを最大 4 つ同時に進める指定で、計算を実行するときのプロセス数とは関係ありません。
`cmake --build build --verbose` とすると、CMake が実際に呼んだコンパイラのコマンドが表示されます。

### ビルド種別

**ビルド種別**（build type）は、最適化やデバッグ情報の付け方をまとめて選ぶ設定です。設定の段階で `-D` を使って渡します。

```bash
cmake -S . -B build_debug -DCMAKE_BUILD_TYPE=Debug
```

| 種別 | GCC・Clang に渡る主な指定 | 使う場面 |
|---|---|---|
| `Release` | `-O3 -DNDEBUG` | 計算の実行。速さを優先します |
| `Debug` | `-g` | 不具合を調べるとき。最適化をせず、デバッガで変数を見られるようにします |

`-D名前=値` は、CMake の設定項目（**キャッシュ変数**）に値を渡す書き方です。
ビルド種別を何も指定しないと、上の `hello` のような CMake のプロジェクトは最適化の指定なしでビルドされます。
MISO とデモの `CMakeLists.txt` は、指定がなければ `Release` を選ぶように書かれています。

`-DNDEBUG` が付くと、C++ の `assert`（条件が成り立たなければ止める確認）は取り除かれます。
MISO の既定は `Release` なので、`assert` による確認は既定のビルドでは働きません。

種別やコンパイラを変えるときは、`build_debug` のように別のビルドフォルダを使います。
同じフォルダで設定を変えると、前の設定が `CMakeCache.txt` に残って混ざることがあります。

### ソースを変更したときのビルド

ソースを編集したら、`cmake --build build` をもう一度実行します。
変更されたファイルと、それを `#include` しているファイルだけが翻訳し直されます。
`CMakeLists.txt` を変更した場合も、次の `cmake --build` のときに CMake が自動で設定をやり直します。
生成された `build/Makefile` を手で直すことはありません。

(edu-ch01-miso-build)=
## MISO のビルドとユニットテスト

### `miso/CMakeLists.txt` の中身

MISO 本体のビルド設定は `miso/CMakeLists.txt` にあります。主な部分を抜粋します（説明のため一部を省略しています）。

```cmake
# CUDA
option(MISO_USE_CUDA "Enable CUDA support" OFF)

# MISO as a header-only library
add_library(MISO INTERFACE)
add_library(MISO::MISO ALIAS MISO)
target_include_directories(MISO INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# MPI
find_package(MPI REQUIRED)
target_link_libraries(MISO INTERFACE MPI::MPI_CXX)

# yaml-cpp
find_package(yaml-cpp QUIET)
if(NOT yaml-cpp_FOUND)
    message(STATUS "yaml-cpp not found; fetching with FetchContent")
    include(FetchContent)
    FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG 0.8.0)
    FetchContent_MakeAvailable(yaml-cpp)
endif(NOT yaml-cpp_FOUND)
target_link_libraries(MISO INTERFACE yaml-cpp::yaml-cpp)

# Unit tests
option(ENABLE_TESTING "Enable Testing" ON)
if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif(ENABLE_TESTING)
```

| 設定 | 意味 |
|---|---|
| `add_library(MISO INTERFACE)` | 中身をコンパイルしないライブラリのターゲット `MISO` を作ります。ヘッダオンリーなので、コンパイルするものがありません |
| `add_library(MISO::MISO ALIAS MISO)` | `MISO` に `MISO::MISO` という別名を付けます。使う側はこの名前でリンクします |
| `target_include_directories(MISO INTERFACE .../include)` | `MISO::MISO` を使うターゲットに、`miso/include` を `-I` で渡します |
| `target_link_libraries(MISO INTERFACE MPI::MPI_CXX)` | `MISO::MISO` を使うターゲットに、MPI のヘッダとライブラリも伝えます |
| `find_package(yaml-cpp QUIET)` と `FetchContent` | 計算機に yaml-cpp があればそれを使い、なければ GitHub から取得して一緒にビルドします |
| `option(MISO_USE_CUDA ... OFF)` | GPU（CUDA）を使うかどうかの切り替えです。既定は使いません |
| `option(ENABLE_TESTING ... ON)` | ユニットテストもビルドするかどうかの切り替えです。既定はビルドします |

`INTERFACE` は「`MISO::MISO` を使う側のターゲットに伝える」という指定です。
このため、`target_link_libraries(自分のプログラム PRIVATE MISO::MISO)` と 1 行書くだけで、
MISO のヘッダの場所、MPI、yaml-cpp の指定がまとめて自分のプログラムに伝わります。

**FetchContent** は、必要なソースを CMake の設定の段階でインターネットから取得する仕組みです。
yaml-cpp を入れていない場合や、ユニットテスト用の doctest（{ref}`テストと計算結果の比較 <edu-ref-testing>` で説明します）を取得する場合に使われるので、
最初の設定にはインターネットへの接続が必要です。

### ビルドとテストの実行

ビルドフォルダは MISO のフォルダの外に作っても構いません。ここでは `$HOME/miso-work/build_miso` を使います。

```bash
cd "$HOME/miso-work"
cmake -S /path/to/MISO/miso -B build_miso -DMISO_USE_CUDA=OFF
cmake --build build_miso --parallel 4
```

`-S` には `miso/CMakeLists.txt` のあるフォルダ（`/path/to/MISO/miso`）を渡します。
テストの実行ファイルがいくつも作られ、数分かかることがあります。

ビルドが終わったら、**ユニットテスト**を実行します。ユニットテストは、配列や並列ループなどの部品ごとに、期待どおりに動くかを確かめるプログラムです。
**ctest** は、CMake に登録されたテストをまとめて実行する道具です。

```bash
cd build_miso
OMPI_MCA_btl_vader_single_copy_mechanism=none \
OMPI_MCA_rmaps_base_oversubscribe=yes \
ctest --output-on-failure
```

最後に次のように表示されれば成功です（テストの数は MISO の版によって変わります）。

```text
100% tests passed, 0 tests failed out of 15
```

先頭の 2 行は、このコマンドの間だけ Open MPI の動作を変える**環境変数**です。
テストの一部は MPI の 2 プロセスで動くため、CPU のコア数が少ない計算機やコンテナでも実行できるようにしています。
意味と、テストの読み方、テストを絞って実行する方法は {ref}`テストと計算結果の比較 <edu-ref-testing>` にまとめています。

(edu-ch01-demo)=
## デモの実行と出力の読み込み

### デモをビルドして実行する

MISO の `demo/` には、問題ごとのデモがあります。2 次元の Orszag–Tang 渦（`demo/mhd2d_vortex`）を動かします。
デモの `CMakeLists.txt` は、MISO 本体（`../../miso`）を取り込んで `src/main.cpp` から実行ファイルを作ります。

```bash
cd /path/to/MISO/demo/mhd2d_vortex
cmake -S . -B build -DUSE_CUDA=OFF
cmake --build build --parallel 4
./app_run.sh
```

デモでは GPU の切り替えを `USE_CUDA` という名前で渡します（MISO 本体の `MISO_USE_CUDA` とは名前が違います）。
`app_run.sh` は、Open MPI 用の環境変数を `demo/shared/openmpi_config.env` から読み込み、2 プロセスで計算を実行するスクリプトです。
実行するコマンドを表示してから計算を始めます。

```text
+ mpiexec -np 2 /path/to/MISO/demo/mhd2d_vortex/build/mhd2d_vortex --config=/path/to/MISO/demo/mhd2d_vortex/config.yaml
time =   0.00;  n_step =        0;  n_output =        0
time =   0.01;  n_step =        4;  n_output =        1
...
time =   0.80;  n_step =      354;  n_output =       80
```

計算の条件は `demo/mhd2d_vortex/config.yaml` に書かれています。格子点数、終了時刻（`time.tend`）、出力の間隔（`time.dt_output`）、
MPI での分割数（`mpi.x_procs` など）を変えられます。

### 出力されるファイル

結果は `config.yaml` と同じフォルダの `data/` に書かれます（`config.yaml` の `io.save_dir` で変えられます）。

| ファイル | 中身 |
|---|---|
| `data/config.yaml` | 既定値を補った、実際に使った設定 |
| `data/grid.bin` | 全体の格子の座標 |
| `data/mpi/coords.csv` | 各プロセス（ランク）が担当する領域の位置 |
| `data/time/time.NNNNNNNN.txt` | 各出力の時刻、出力番号、ステップ数 |
| `data/mhd/mhd.<出力番号>.<ランク>.bin` | 各プロセスが担当する領域の MHD の変数（密度、速度、磁場など 9 つ） |

`mhd.*.bin` はプロセスごとに分かれたバイナリファイルです。
`config.yaml` の `io.continue: true` により、`data/` に結果があると、そこから計算を続けます。
最初からやり直すときは `./app_clean.sh` で `build/`、`data/`、`figs/` を消してから、ビルドと実行をやり直します。

### pymiso で読み込む

pymiso は、プロセスごとのファイルをつなげて全体の配列として読み込む Python パッケージです。
Python のパッケージは、MISO 用の**仮想環境**（その中に入れたパッケージだけを使う、独立した Python の環境）に入れます。

```bash
python3 -m venv "$HOME/miso-work/venv"
source "$HOME/miso-work/venv/bin/activate"
cd /path/to/MISO
pip install -e ".[viz]"
```

`source .../activate` は、このターミナルで仮想環境の Python を使うようにするコマンドです。
新しいターミナルを開いたら、もう一度実行します。
`pip install -e ".[viz]"` は、MISO のフォルダにある pymiso と、図を描くための matplotlib などを入れます。

デモには図を描くスクリプト `plot_data.py` があります。

```bash
cd /path/to/MISO/demo/mhd2d_vortex
python plot_data.py
```

`figs/` に、各時刻の密度の図（`py_00000000.png` など）が保存されます。
`plot_data.py` の中では、次のように結果を読み込んでいます（`demo/mhd2d_vortex/plot_data.py` から抜粋。`...` は省略した部分です）。

```python
d = pymiso.Data(data_dir=this_dir / "data")
...
d.load(n)
ax.pcolormesh(d.x, d.y, d.ro.T, shading="auto", vmin=0.2, vmax=2.5, cmap="inferno")
```

`pymiso.Data` に `data/` の場所を渡し、`d.load(n)` で出力番号 `n` の結果を読みます。
`d.ro` が密度、`d.x`・`d.y` が座標の NumPy 配列です。
ほかのデモ、設定の変え方、GPU 版の実行方法は、MISO_docs の {doc}`Quick Start </quickstart>` にまとめられています。

(edu-ch01-own-project)=
## 自分のプログラムから MISO を使う

第 7 章と第 10 章の実習では、MISO のフォルダの外に自分のフォルダを作り、MISO の部品を使うプログラムを書きます。
そのための最小の CMake の設定を作り、MISO が使えることを確かめます。

### 作るもの

MISO のフォルダの外に、作業用のフォルダを作ります。

```bash
mkdir -p "$HOME/miso-work/miso_check"
cd "$HOME/miso-work/miso_check"
```

このフォルダに、確認用のプログラム `main.cpp` と `CMakeLists.txt` を保存します。

`main.cpp` は、MPI を始め、MISO の配列に 0〜99 を入れて合計を求めるプログラムです。
`miso::Array1D`、`miso::for_each`、`miso::reduce`、`MISO_LAMBDA` は {ref}`第 7 章 <edu-ch07>` で説明します。ここではそのまま保存してください。

```cpp
#include <iostream>

#include <miso/core.hpp>

int main(int argc, char **argv) {
  miso::Env env(argc, argv);  // MPI を始め、終わりに片付ける

  const int n = 100;
  miso::Array1D<float, miso::backend::Host> a(n);
  auto av = a.view();
  miso::for_each(miso::backend::Host{}, miso::Range1D{0, n},
                 MISO_LAMBDA(int i) { av[i] = float(i); });

  const float sum = miso::reduce(
      miso::backend::Host{}, miso::Range1D{0, n}, 0.0f,
      MISO_LAMBDA(int i) { return av[i]; },
      MISO_LAMBDA(float x, float y) { return x + y; });

  if (miso::mpi::is_root()) {
    std::cout << "MPI processes: " << miso::mpi::size() << '\n'
              << "sum of 0..99: " << sum << " (expected 4950)\n";
  }
  return 0;
}
```

`#include <miso/core.hpp>` は、配列・並列ループ・MPI の環境など、MISO の共通の部品をまとめて読み込むヘッダ（`miso/include/miso/core.hpp`）です。

### `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.18)
project(miso_check LANGUAGES CXX)

set(MISO_DIR "" CACHE PATH "Root of the MISO repository")
if(NOT EXISTS "${MISO_DIR}/miso/CMakeLists.txt")
  message(FATAL_ERROR "Set -DMISO_DIR=/path/to/MISO")
endif()

set(ENABLE_TESTING OFF CACHE BOOL "Build the unit tests of MISO")
add_subdirectory("${MISO_DIR}/miso" miso)

add_executable(miso_check main.cpp)
target_compile_features(miso_check PRIVATE cxx_std_17)
target_link_libraries(miso_check PRIVATE MISO::MISO)
```

| 設定 | 役割 |
|---|---|
| `set(MISO_DIR "" CACHE PATH ...)` | MISO を置いた場所を入れる設定項目 `MISO_DIR` を用意します。値は設定のときに `-DMISO_DIR=...` で渡します。`${MISO_DIR}` は変数の値を取り出す書き方です |
| `if(NOT EXISTS ...)` と `message(FATAL_ERROR ...)` | `MISO_DIR` の下に `miso/CMakeLists.txt` がなければ、説明を表示して止めます |
| `set(ENABLE_TESTING OFF CACHE BOOL ...)` | MISO のユニットテストをビルドしないようにします。`add_subdirectory` より前に書きます |
| `add_subdirectory("${MISO_DIR}/miso" miso)` | MISO の `miso/CMakeLists.txt` を読み込み、`MISO::MISO` を使えるようにします。2 つ目の `miso` は、MISO 用の生成物を置く `build/` の中のフォルダ名です |
| `add_executable(miso_check main.cpp)` | `main.cpp` から実行ファイル `miso_check` を作ります |
| `target_link_libraries(miso_check PRIVATE MISO::MISO)` | MISO のヘッダの場所、MPI、yaml-cpp をまとめて `miso_check` に伝えます |

`add_subdirectory` は、別のフォルダの `CMakeLists.txt` を、自分のプロジェクトの一部として読み込む命令です。
MISO のフォルダがプロジェクトの外にあるときは、2 つ目の引数（生成物の置き場）が必要です。

`ENABLE_TESTING` を `OFF` にするのは、自分のプログラムだけを速くビルドするためです。
MISO のユニットテストは、{ref}`前の節 <edu-ch01-miso-build>` のように MISO 単独のビルドで実行します。
`option(ENABLE_TESTING ...)` は、同じ名前の設定項目がすでにあればその値を使うので、先に `OFF` を入れておくとテストが外れます。

ビルド種別を指定しなければ、MISO の `CMakeLists.txt` が `Release` を選びます。

### ビルドと実行

`/path/to/MISO` を、MISO を置いた場所の絶対パスに置き換えて設定します。

```bash
cmake -S . -B build -DMISO_DIR=/path/to/MISO
cmake --build build --parallel 4
mpiexec -np 2 ./build/miso_check
```

`mpiexec -np 2` は、同じプログラムを 2 つのプロセスで起動するコマンドです。次の表示が出れば準備完了です。

```text
MPI processes: 2
sum of 0..99: 4950 (expected 4950)
```

どちらのプロセスも同じ計算をしますが、表示は 1 つのプロセス（ランク 0）だけが行います。

MISO のデモの `CMakeLists.txt` は、`add_subdirectory` の代わりに `FetchContent_Declare(MISO SOURCE_DIR ...)` と `FetchContent_MakeAvailable(MISO)` で MISO を取り込んでいます。
手元のフォルダを指定する使い方では、どちらも `miso/CMakeLists.txt` を読み込んで `MISO::MISO` を作る点は同じです。

### 練習 1-2

`-DMISO_DIR=...` を付けずに、新しいビルドフォルダ `build_nodir` で設定してください。
表示されたメッセージを確かめ、設定・ビルド・実行のどの段階で止まったかと、その理由を `CMakeLists.txt` の行と対応づけて説明してください。

<details>
<summary>解答・解説</summary>

```bash
cmake -S . -B build_nodir
```

```text
CMake Error at CMakeLists.txt:6 (message):
  Set -DMISO_DIR=/path/to/MISO
```

**設定**の段階で止まります。`MISO_DIR` の値は空のままなので、`"${MISO_DIR}/miso/CMakeLists.txt"` は `/miso/CMakeLists.txt` になり、
`if(NOT EXISTS ...)` の条件が成り立ちます。その中の `message(FATAL_ERROR ...)` が説明を表示して設定を止めます。
この確認がないと、`add_subdirectory` が「フォルダがない」という分かりにくいエラーを出します。
確かめたら、`build_nodir` は消して構いません。

</details>

### 練習 1-3

`miso_check` を `Debug` 種別で、`build_debug` という別のビルドフォルダに作ってください。
`cmake -L -N build_debug` で設定項目の一覧を表示し、`CMAKE_BUILD_TYPE`、`MISO_DIR`、`ENABLE_TESTING`、`MISO_USE_CUDA` の値を確かめてください。

<details>
<summary>解答・解説</summary>

```bash
cmake -S . -B build_debug -DMISO_DIR=/path/to/MISO -DCMAKE_BUILD_TYPE=Debug
cmake --build build_debug --parallel 4
cmake -L -N build_debug | grep -E "BUILD_TYPE|MISO|ENABLE_TESTING"
```

```text
CMAKE_BUILD_TYPE:STRING=Debug
ENABLE_TESTING:BOOL=OFF
MISO_DIR:PATH=/path/to/MISO
MISO_USE_CUDA:BOOL=OFF
```

`-L` は設定項目の一覧を表示し、`-N` は設定をやり直さずに `CMakeCache.txt` を読むだけにする指定です。
`MISO_USE_CUDA` は `MISO` 側の `option` が既定値 `OFF` で作った項目です。
`cmake --build build_debug --verbose` で、コンパイルのコマンドに `-g` が付き、`-O3` が付いていないことも確かめられます。
`build`（Release）と `build_debug`（Debug）はフォルダが別なので、互いの設定は混ざりません。

</details>

### 手でコンパイルする場合との対応

CMake が行っていることを、手で書いたコマンドと比べておきます。Ubuntu では次のコマンドでも `miso_check` を作れます。

```bash
mpicxx -std=c++17 -O2 -I/path/to/MISO/miso/include main.cpp -o miso_check_manual -lyaml-cpp
```

`mpicxx` が MPI の指定を足し、`-I` が MISO のヘッダの場所、`-lyaml-cpp` が yaml-cpp を指定します。
`MISO::MISO` をリンクすると、これらを CMake がまとめて指定します。
GPU 用のビルドや計算機ごとの違いがあっても `CMakeLists.txt` を変えずに済むので、この教材では CMake を使います。

(edu-ch01-trouble)=
## うまくいかないとき

最初のエラーから順に確かめます。設定が成功してからビルドし、ビルドが成功してから実行します。

| 表示・症状 | 確かめること |
|---|---|
| `Could NOT find MPI` | Ubuntu では `libopenmpi-dev`、Mac では `open-mpi` が入っているかを確かめます。Mac では `-DMPI_CXX_COMPILER="$(brew --prefix open-mpi)/bin/mpicxx"` を設定のコマンドに足すと、使う MPI を明示できます |
| `yaml-cpp not found; fetching with FetchContent` の後に取得の失敗 | yaml-cpp が見つからず、インターネットから取得しようとしています。yaml-cpp を入れる（Ubuntu は `libyaml-cpp-dev`、Mac は `yaml-cpp`）か、ネットワークの接続を確かめます |
| ユニットテストの設定で doctest の取得に失敗する | doctest はいつもインターネットから取得します。接続を確かめます |
| `fatal error: miso/core.hpp: No such file or directory` | `MISO::MISO` をリンクしているか、`-DMISO_DIR` が MISO のフォルダの根（`miso/` を含むフォルダ）を指しているかを確かめます |
| `fatal error: mpi.h: No such file or directory` | 手でコンパイルするときは `g++` ではなく `mpicxx` を使います。CMake では `MISO::MISO` が MPI の指定を伝えます |
| `undefined reference to 'YAML::...'` | yaml-cpp がリンクされていません。手でコンパイルするときは `-lyaml-cpp` を付けます |
| `There are not enough slots available` | CPU のコア数より多いプロセスを起動しようとしています。動作確認だけなら `mpiexec --oversubscribe -np 2 ...` とします |
| `CMakeLists.txt` が見つからない | `pwd` と `ls` で、ファイルを保存したフォルダにいるかを確かめます |
| コンパイラを変えたのに前のコンパイラが使われる | 前の設定が `CMakeCache.txt` に残っています。別のビルドフォルダで設定し直します |

(edu-ch01-gpu)=
## GPU を使う段階になったら

MISO は NVIDIA の GPU で計算する CUDA 版も作れます。
MISO 本体では `-DMISO_USE_CUDA=ON`、デモでは `-DUSE_CUDA=ON` を設定のときに渡します。
CUDA の開発環境（CUDA Toolkit と対応するドライバ）が必要で、Mac の GPU では動きません。
GPU 用のビルドと、自分のプログラムを GPU で動かす設定は {ref}`第 9 章 <edu-ch09>` で説明します。

## 次に読むところ

準備ができたら、{ref}`第 2 章：変数・関数・制御文 <edu-ch02>` へ進みます。
この章で作った `miso_check` のフォルダと `CMakeLists.txt` の形は、{ref}`第 7 章 <edu-ch07>` と {ref}`第 10 章 <edu-ch10>` の実習で使います。
