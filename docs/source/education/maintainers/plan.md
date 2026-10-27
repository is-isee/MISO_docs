# MISO education の執筆計画（4 部・11 章）

各章の「扱うこと」は、その章で初めて説明する項目です。前の章で説明した項目は既知として使えます。
後の章の項目を使うときは、その章へ案内します。

## 本文

### I．準備

**第 1 章 開発環境と CMake**（`01_environment_setup.md`, ラベル `edu-ch01`）
- Ubuntu・Mac で揃える道具：C++ コンパイラ（g++/clang++）、CMake、MPI（OpenMPI）、yaml-cpp、git、Python（pymiso）。
- コンパイルとリンク、ヘッダとソース、`#include` の役割（最小の確認プログラムを手で `g++` でビルド）。
- CMake の仕組み：`CMakeLists.txt`、`cmake -B build -S .`、`cmake --build build`、ビルド種別（Release/Debug）、`target_link_libraries`。
- MISO のビルド：`miso/CMakeLists.txt` がヘッダオンリーの `MISO::MISO` を作ること、`MISO_USE_CUDA`、yaml-cpp の FetchContent。
- ユニットテスト（`ctest`）とデモ（`demo/mhd2d_vortex`）の実行、出力の読み込み（`pymiso`）。
- 自分のプログラムから MISO を使う最小の CMake 設定（`add_subdirectory` で `MISO::MISO` をリンク）。実習で使う。

### II．C++ の基礎

**第 2 章 変数・関数・制御文**（`02_cpp_basics.md`, `edu-ch02`）
- 型と変数の宣言、初期化、`const`、`auto`、整数と浮動小数点、型変換、`float`/`double` と MISO の `Real`（`using Real = float;`）。
- 演算子、`if`・`for`・`while`、0 始まりと上限を含まないループ、`break`/`continue`。
- 関数の宣言と定義、値渡し、戻り値、オーバーロード（同じ名前で引数の型が違う関数）。
- ファイル分割、ヘッダ、`#pragma once`、`inline`（ヘッダオンリーで必要な理由）、名前空間（`namespace miso`, `miso::mhd`, `using namespace`）。
- 標準ライブラリの入出力（`std::cout`）、`std::string`、`std::vector` の初歩、例外（`throw std::runtime_error`）。
- プリプロセッサ（`#define`, `#ifdef USE_CUDA`）の初歩。
- MISO の実例：`miso/include/miso/constants.hpp`、`utility.hpp` の小さな関数、demo の `main.cpp` の冒頭。

**第 3 章 参照・ポインタと寿命**（`03_references_and_pointers.md`, `edu-ch03`）
- コピーと参照（`T&`）、`const T&`、参照渡し（Fortran の引数との対比）。
- アドレスとポインタ（`&x`, `*p`, `T*`, `nullptr`）、配列とポインタ、ポインタ演算。
- 宣言と式で `*`・`&` の意味が変わること、`const` の位置（`const T*` と `T* const`）。
- 変数の寿命とスコープ、ぶら下がり参照、`new`/`delete` と、それを直接書かない理由（スマートポインタ `std::unique_ptr`/`std::shared_ptr` の初歩）。
- MISO の実例：コンストラクタ引数 `Config &config`、メンバとして持つ参照（`Grid<Real, Backend> &grid`）、View が持つポインタ。

**第 4 章 クラス・構造体**（`04_classes_and_structs.md`, `edu-ch04`）
- `struct` と `class`、メンバ変数、メンバ関数、`public`/`private`、`const` メンバ関数。
- コンストラクタと初期化子リスト、メンバの初期化順、デフォルト引数、`explicit`。
- デストラクタと後片付け（RAII）、コピーの禁止（`= delete`）、ムーブの初歩。
- 演算子のオーバーロード（`operator()` で `a(i, j, k)` と書ける仕組み）。
- 所有する配列と所有しない View：`Array3D` と `Array3DView`、`view()`/`const_view()`。
- MISO の実例：`miso/include/miso/array3d.hpp`、`mhd_fields.hpp`（`Fields` と `FieldsView`）、`time.hpp`、`config.hpp`、demo の `InitialCondition`。

**第 5 章 継承と CRTP**（`05_inheritance.md`, `edu-ch05`）
- 継承の基本（基底クラス・派生クラス、メンバの引き継ぎ、基底のコンストラクタ呼び出し）。
- 仮想関数（`virtual`）による実行時の切り替えを短く説明し、MISO では使っていないことを示す。
- CRTP（基底クラスのテンプレート引数に派生クラスを渡す形）で、派生クラスのメンバを基底から呼ぶ仕組み。`static_cast<Derived&>(*this)`。
- MISO の実例：`miso/include/miso/mhd_model_base.hpp` の `ModelBase<Derived, Real, Backend>` と demo の `struct Model : public mhd::ModelBase<Model, Real, Backend>`、`run()`/`update()` の上書き。
- 注：テンプレートの文法は第 6 章。この章では「`<>` の中に型を渡す」ことだけを使い、詳しくは第 6 章へ案内する。

**第 6 章 テンプレート**（`06_templates.md`, `edu-ch06`）
- 関数テンプレート、クラステンプレート、型の推論、テンプレート引数としての型（`Real`, `Backend`）。
- タグ型によるオーバーロードの選択（`backend::Host{}` と `backend::CUDA{}`）。
- 特殊化（部分特殊化）、`if constexpr`、`static_assert`。
- 変数テンプレート（`pii4<Real>`）。
- 型の性質を調べる仕組み（`std::is_same_v`、`std::void_t` による「メンバがあるか」の検査）を読める程度に。
- MISO の実例：`backend.hpp`、`constants.hpp`、`mhd_model_base.hpp` の `has_eos` / `has_dt_limit`、`mhd_integrator.hpp` の `MISO_MHD_DEFINE_SOURCE_TERM`（マクロとテンプレート）、`Integrator<Real, Backend>` の host/CUDA の特殊化。

### III．MISO における並列計算

**第 7 章 配列・ラムダと CPU での並列ループ**（`07_arrays_and_parallel.md`, `edu-ch07`）
- `Array1D`〜`Array4D`：確保、添字、メモリ上の並び（最後の添字が連続。Fortran と逆）、View。
- ラムダ式（名前のない関数）、キャプチャ（`[=]` と `[&]`）、「値を抱えた小さな構造体」というモデル、メンバ変数は `this` 経由でキャプチャされること。
- `MISO_LAMBDA`（`execution.hpp` で定義。CPU ビルドでは `[=]`）。
- `Range1D`/`Range3D`、`for_each`（Host 版はただの `for` 文）、`reduce`（縮約：最小値・合計）。
- MISO の実例：`execution.hpp`、`eos.hpp` の `gas_pressure`、`Integrator::cfl`。
- 実習 `practice/execution`（ベクトル加算、縮約、1 次元移流）への案内。
- OpenMP は MISO では使っていないことを短く述べる（コラム可）。

**第 8 章 MPI による領域分割**（`08_mpi.md`, `edu-ch08`）
- MPI の考え方（同じプログラムを複数のプロセスで動かす、ランク）、`MPI_Init`/`MPI_Finalize` と MISO の `Env`。
- 領域分割と `mpi::Shape`（`config.yaml` の `mpi` セクション）、全体格子と各ランクの格子、ゴーストセル（`i_margin`）。
- 隣との値の交換（ハロー交換）：`MPI_Sendrecv` の考え方と `mhd_halo_exchange.hpp` の読み方。
- 全体の集計：`MPI_Allreduce`（`Integrator::cfl` の dt）。
- 物理境界と内部境界：`boundary_condition::is_physical_boundary`。
- 実行：`mpiexec -np 2`、OpenMPI の環境変数（`demo/shared/openmpi_config.env`）。

**第 9 章 GPU（CUDA）での計算とデータ転送**（`09_gpu.md`, `edu-ch09`）
- GPU の考え方（ホストとデバイス、別々のメモリ、多数のスレッドでの並列）。
- CUDA のカーネル、`__global__`/`__device__`/`__host__`、スレッドとブロック、`<<<grid, block>>>`。
- デバイスメモリの確保と転送（`cudaMalloc`, `cudaMemcpy`）と、MISO で `Array3D<Real, backend::CUDA>` と `copy_from` がそれを隠していること。
- `for_each` の CUDA 版：ラムダがカーネル引数として GPU にコピーされ、各スレッドが `f(i)` を呼ぶ。`MISO_LAMBDA` が `[=] __host__ __device__` になること、`--expt-extended-lambda`。
- `this` キャプチャの落とし穴（メンバ変数をラムダの前でローカル変数にコピーする、MISO #170）。
- `reduce` の CUDA 版（ブロック内の共有メモリでの縮約）と同期（`cudaDeviceSynchronize`）。
- MPI と GPU（CUDA-aware MPI）、ビルド（`-DMISO_USE_CUDA=ON`, `CMAKE_CUDA_ARCHITECTURES`）。
- GPU がない読者も読めるようにする。

### IV．総合実習と本体の読解

**第 10 章 熱伝導の計算**（`10_heat1d.md`, `edu-ch10`）
- 1 次元熱伝導方程式、差分、陽解法の安定条件、境界条件、期待値（解析解）との比較方法。
- 逐次版 → MPI 版 → GPU 版の順で、MISO の `Array1D`、`for_each`、`reduce`、MPI を使って受講者が書く。
- 実習 `practice/heat1d` への案内。

**第 11 章 MISO の時間発展を読む**（`11_reading_miso.md`, `edu-ch11`）
- demo の `main()` から `Model`、`ModelBase::run()`、`update()`、`MHD::update()`、`Integrator` へ。
- `config.yaml` と `config_defaults.hpp`（`merge_yaml`）。
- 1 ステップの中身：CFL による dt、RK4 の 4 段（`update_sc4`）、境界条件とハロー交換、人工粘性、出力（`mhd_checkpoint.hpp`）。
- 4 次中心差分（`space_centered_4th`）と方程式の対応、ソース項（`Source` の `ro`〜`ei`）、EOS。
- 自分で問題を作るときに触る場所（初期条件・境界条件・ソース項）と、MISO_docs の「カスタマイズ」ページへの案内。

## 参考資料（`reference/`）

- `glossary.md`（`edu-ref-glossary`）：用語集。英語併記。
- `fortran_map.md`（`edu-ref-fortran-map`）：Fortran と C++/MISO の対応表。
- `cpp_reference.md`（`edu-ref-cpp`）：`*`・`&`・`::`・`<>`・`[]`・`->` などの記号の早見表と本文への直接リンク。
- `testing.md`（`edu-ref-testing`）：MISO のユニットテスト（doctest、`ctest`）と計算結果の比較方法。
- `gpu_troubleshooting.md`（`edu-ref-gpu-troubleshooting`）：GPU で困ったとき。

## 実習（`practice/`）

- `practice/execution/`（`edu-practice-execution`）：01 ベクトル加算、02 縮約、03 1 次元移流。第 7 章から案内。CPU で作り、第 9 章の後に CUDA でも動かす。
- `practice/heat1d/`（`edu-practice-heat1d`）：01 逐次、02 MPI、03 GPU。第 10 章から案内。
- 解答例は `maintainers/solutions/<題材>/` に置き、実際にビルド・実行して確かめる。
