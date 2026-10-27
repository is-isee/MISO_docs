(edu-ref-glossary)=
# 用語集

本文に出てくる用語を、初めて説明する章の順に並べています。詳しい説明は「初出」の章を読んでください。
元の資料やコンパイラのエラーは英語なので、英語も併記します。

## I．準備

| 語 | 英語 | 説明 | 初出 |
|---|---|---|---|
| コンパイル | compile | ソースファイルを 1 つずつ機械語に翻訳し、オブジェクトファイルを作る段階 | {ref}`1 <edu-ch01>` |
| リンク | link | オブジェクトファイルとライブラリを結び付けて実行ファイルを作る段階 | {ref}`1 <edu-ch01>` |
| オブジェクトファイル | object file | コンパイルの結果できるファイル（`.o`） | {ref}`1 <edu-ch01>` |
| ヘッダ（ヘッダファイル） | header (file) | 宣言や定義を書き、`#include` で読み込むファイル（`.hpp`） | {ref}`1 <edu-ch01>` |
| ヘッダオンリー | header-only | 定義をすべてヘッダに書き、前もってライブラリを作らずに使える形。MISO はこの形 | {ref}`1 <edu-ch01>` |
| ターゲット | target | CMake でビルドする対象（実行ファイルやライブラリ）の名前 | {ref}`1 <edu-ch01>` |
| キャッシュ変数 | cache variable | `cmake -D` で渡し、`CMakeCache.txt` に残る設定項目 | {ref}`1 <edu-ch01>` |
| ビルド種別 | build type | Release・Debug など、最適化とデバッグ情報の組み合わせ | {ref}`1 <edu-ch01>` |
| FetchContent | FetchContent | 設定の段階で依存ライブラリのソースを取得する CMake の仕組み | {ref}`1 <edu-ch01>` |
| ユニットテスト | unit test | 部品ごとに動作を確かめるプログラム | {ref}`1 <edu-ch01>` |
| 環境変数 | environment variable | シェルからコマンドへ渡る「名前=値」の設定 | {ref}`1 <edu-ch01>` |
| 仮想環境 | virtual environment (venv) | ほかと独立した Python の実行環境 | {ref}`1 <edu-ch01>` |
| doctest | doctest | MISO のユニットテストに使っている C++ のテスト用ライブラリ | {ref}`テスト <edu-ref-testing>` |
| 計算機イプシロン | machine epsilon | 1 と、その次に表せる浮動小数点数との差 | {ref}`テスト <edu-ref-testing>` |
| 丸め誤差 | rounding error | 有限の桁に丸めることで生じる誤差 | {ref}`テスト <edu-ref-testing>` |
| 桁落ち | cancellation | 近い値どうしの引き算で有効桁が減ること | {ref}`テスト <edu-ref-testing>` |

## II．C++ の基礎

| 語 | 英語 | 説明 | 初出 |
|---|---|---|---|
| 宣言 | declaration | 名前と型をコンパイラに知らせること | {ref}`2 <edu-ch02>` |
| 初期化 | initialization | 変数を作るときに最初の値を与えること | {ref}`2 <edu-ch02>` |
| 暗黙の型変換 | implicit conversion | 型の違う値を混ぜた式などで、コンパイラが自動で行う変換 | {ref}`2 <edu-ch02>` |
| static_cast | static_cast | コンパイル時に型を変換する書き方 | {ref}`2 <edu-ch02>` |
| 型の別名 | type alias | `using Real = float;` のように既存の型に付ける別の名前 | {ref}`2 <edu-ch02>` |
| 列挙型 | enumeration (`enum class`) | 決まった選択肢から 1 つを表す型（`Direction::X` など） | {ref}`2 <edu-ch02>` |
| ブロック | block | `{ }` で囲んだ文のまとまり | {ref}`2 <edu-ch02>` |
| 短絡評価 | short-circuit evaluation | `&&`・`\|\|` で、左側だけで結果が決まれば右側を評価しないこと | {ref}`2 <edu-ch02>` |
| 条件演算子 | conditional operator | `c ? a : b`。`c` が真なら `a`、偽なら `b` | {ref}`2 <edu-ch02>` |
| 戻り値 | return value | 関数が `return` で返す値 | {ref}`2 <edu-ch02>` |
| 値渡し | pass by value | 関数の引数に値のコピーを渡すこと | {ref}`2 <edu-ch02>` |
| デフォルト引数 | default argument | 省略したときに使う引数の値 | {ref}`2 <edu-ch02>` |
| オーバーロード | overload | 同じ名前で引数の型や数が違う関数を複数定義すること | {ref}`2 <edu-ch02>` |
| inline | inline | 同じ定義が複数のソースファイルに現れてもよいとする指定。ヘッダの関数に必要 | {ref}`2 <edu-ch02>` |
| 名前空間 | namespace | 名前をグループに分ける仕組み（`miso::`、`std::`） | {ref}`2 <edu-ch02>` |
| 標準ライブラリ | standard library | C++ に付属する部品群（`std::vector`、`std::string` など） | {ref}`2 <edu-ch02>` |
| 例外 | exception | `throw` で投げ、`catch` で受け取るエラーの知らせ | {ref}`2 <edu-ch02>` |
| プリプロセッサ・マクロ | preprocessor / macro | コンパイルの前に行う文字列の置き換え（`#define`、`#ifdef`） | {ref}`2 <edu-ch02>` |
| 参照 | reference | 既にある変数の別名（`T&`） | {ref}`3 <edu-ch03>` |
| 参照渡し | pass by reference | 関数の引数を参照で受け取ること | {ref}`3 <edu-ch03>` |
| 出力引数 | output parameter | 関数が結果を書き込む先として受け取る引数 | {ref}`3 <edu-ch03>` |
| アドレス | address | メモリ上の場所を表す番号 | {ref}`3 <edu-ch03>` |
| ポインタ | pointer | アドレスを値として持つ変数（`T*`） | {ref}`3 <edu-ch03>` |
| 間接参照 | dereference | `*p` のように、ポインタが指す先を使うこと | {ref}`3 <edu-ch03>` |
| ヌルポインタ | null pointer (`nullptr`) | どこも指していないポインタ | {ref}`3 <edu-ch03>` |
| ポインタ演算 | pointer arithmetic | `p + i` で `i` 個先の要素を指すこと | {ref}`3 <edu-ch03>` |
| 未定義動作 | undefined behavior | 言語が結果を保証しない操作。何が起きても不思議はない | {ref}`3 <edu-ch03>` |
| スコープ | scope | 名前を使える範囲 | {ref}`3 <edu-ch03>` |
| 寿命 | lifetime | 変数やオブジェクトの実体が存在する期間 | {ref}`3 <edu-ch03>` |
| ぶら下がり参照・ポインタ | dangling reference / pointer | 寿命が終わった対象を指したままの参照・ポインタ | {ref}`3 <edu-ch03>` |
| メモリリーク | memory leak | 確保したメモリを解放し忘れること | {ref}`3 <edu-ch03>` |
| スマートポインタ | smart pointer | 寿命が終わると自動で解放するポインタ（`std::unique_ptr`、`std::shared_ptr`） | {ref}`3 <edu-ch03>` |
| クラス・構造体 | class / struct | データと関数をまとめた型。`struct` は既定の公開範囲が public | {ref}`4 <edu-ch04>` |
| メンバ変数・メンバ関数 | member variable / member function | クラスの中で宣言した変数・関数 | {ref}`4 <edu-ch04>` |
| オブジェクト・インスタンス | object / instance | 型に従って作られた、値を保存する実体 | {ref}`4 <edu-ch04>` |
| カプセル化 | encapsulation | 公開する操作を決め、中のデータを直接触らせないこと | {ref}`4 <edu-ch04>` |
| コンストラクタ | constructor | オブジェクトを作るときに自動で呼ばれる関数 | {ref}`4 <edu-ch04>` |
| 初期化子リスト | member initializer list | コンストラクタの `:` の後に書く、メンバの初期化の並び。初期化はメンバの宣言順 | {ref}`4 <edu-ch04>` |
| explicit | explicit | コンストラクタによる暗黙の型変換を禁止する指定 | {ref}`4 <edu-ch04>` |
| const メンバ関数 | const member function | メンバを書き換えないと約束した関数（末尾の `const`） | {ref}`4 <edu-ch04>` |
| this | this | メンバ関数を呼んだオブジェクトを指すポインタ | {ref}`4 <edu-ch04>` |
| デストラクタ | destructor | 寿命が終わるときに自動で呼ばれる後片付けの関数 | {ref}`4 <edu-ch04>` |
| RAII | Resource Acquisition Is Initialization | 資源の確保と解放をオブジェクトの寿命に結び付ける方法 | {ref}`4 <edu-ch04>` |
| コピーコンストラクタ | copy constructor | 同じ型のオブジェクトをもとに作るコンストラクタ | {ref}`4 <edu-ch04>` |
| ムーブ | move | 中身の持ち主を移すこと（`std::move`） | {ref}`4 <edu-ch04>` |
| `= delete`・`= default` | deleted / defaulted function | 関数を使用禁止にする指定／コンパイラの標準の定義を使う指定 | {ref}`4 <edu-ch04>` |
| 演算子のオーバーロード | operator overloading | `()`・`[]` などの演算子に、クラスごとの意味を定義すること | {ref}`4 <edu-ch04>` |
| 所有 | ownership | 資源の後片付けに責任を持つこと | {ref}`4 <edu-ch04>` |
| View | view | 確保も解放もせず、先頭アドレスと大きさだけを持つ軽いオブジェクト（`Array3DView` など） | {ref}`4 <edu-ch04>` |
| friend | friend | 指定したクラスや関数に private のメンバの使用を許す宣言 | {ref}`4 <edu-ch04>` |
| 継承・基底クラス・派生クラス | inheritance / base class / derived class | 既存のクラス（基底）のメンバを引き継いで新しいクラス（派生）を作ること | {ref}`5 <edu-ch05>` |
| protected | protected | 外からは使えず、派生クラスからは使える公開範囲 | {ref}`5 <edu-ch05>` |
| 名前の隠蔽 | name hiding | 派生クラスの同じ名前の関数が、基底クラスの関数を隠すこと | {ref}`5 <edu-ch05>` |
| 仮想関数・オーバーライド | virtual function / override | 実際のオブジェクトの型に応じて実行時に選ばれる関数と、その定義し直し。MISO では使っていない | {ref}`5 <edu-ch05>` |
| CRTP | Curiously Recurring Template Pattern | 基底クラスのテンプレート引数に派生クラス自身を渡す形。MISO の `ModelBase` が使う | {ref}`5 <edu-ch05>` |
| テンプレート | template | 型や値を後から指定して関数・クラス・変数を作る仕組み | {ref}`6 <edu-ch06>` |
| テンプレート引数 | template argument | `<>` の中で与える型や値（`Array3D<float, backend::Host>`） | {ref}`6 <edu-ch06>` |
| テンプレート引数の推論 | template argument deduction | 関数の引数の型からテンプレート引数を決めること | {ref}`6 <edu-ch06>` |
| 実体化 | instantiation | 型を当てはめて、具体的な関数やクラスをコンパイル時に作ること | {ref}`6 <edu-ch06>` |
| 変数テンプレート | variable template | 型ごとに値を持つ定数（`pii4<Real>`） | {ref}`6 <edu-ch06>` |
| メンバ関数テンプレート | member function template | テンプレートにしたメンバ関数 | {ref}`6 <edu-ch06>` |
| タグ型 | tag type | 中身がなく、型だけで選択の目印になる構造体（`backend::Host`） | {ref}`6 <edu-ch06>` |
| タグディスパッチ | tag dispatch | タグ型の引数で、呼ぶ関数（オーバーロード）を選ぶ書き方 | {ref}`6 <edu-ch06>` |
| 特殊化 | specialization | 特定のテンプレート引数のときだけ使う別の定義 | {ref}`6 <edu-ch06>` |
| 完全な特殊化 | explicit (full) specialization | すべての引数を決めた特殊化（`template <>`） | {ref}`6 <edu-ch06>` |
| 部分特殊化 | partial specialization | 一部の引数だけを決めた特殊化 | {ref}`6 <edu-ch06>` |
| 基本のテンプレート | primary template | 特殊化の元になる最初の宣言 | {ref}`6 <edu-ch06>` |
| 型特性 | type trait | 型の性質を `::value` で返すクラステンプレート（`std::is_same`） | {ref}`6 <edu-ch06>` |
| SFINAE | Substitution Failure Is Not An Error | テンプレート引数の置き換えに失敗した候補を、エラーにせず候補から外す規則 | {ref}`6 <edu-ch06>` |

## III．MISO における並列計算

| 語 | 英語 | 説明 | 初出 |
|---|---|---|---|
| 行優先・列優先 | row-major / column-major | 最後の添字（C++・MISO）か、最初の添字（Fortran）がメモリ上で連続する並び | {ref}`7 <edu-ch07>` |
| ラムダ式 | lambda expression | 式の位置にその場で書く、名前のない関数 | {ref}`7 <edu-ch07>` |
| キャプチャ | capture | ラムダ式が外側の変数を、値（`[=]`）または参照（`[&]`）で取り込むこと | {ref}`7 <edu-ch07>` |
| 半開区間 | half-open interval | 始まりを含み終わりを含まない範囲 `[begin, end)` | {ref}`7 <edu-ch07>` |
| 縮約 | reduction | 全要素から和・最大値などの 1 つの値を求める操作（`reduce`） | {ref}`7 <edu-ch07>` |
| 単位元 | identity element | `op` で合わせても相手を変えない値（和なら 0）。`reduce` の初期値に使う | {ref}`7 <edu-ch07>` |
| FMA | fused multiply-add | 乗算と加算を 1 回の丸めで行う命令。CPU と GPU で結果が少し違う原因になる | {ref}`7 <edu-ch07>` |
| プロセス | process | 自分のメモリを持って動くプログラムの実行単位 | {ref}`8 <edu-ch08>` |
| ランク | rank | コミュニケータの中でプロセスに付く 0 始まりの番号 | {ref}`8 <edu-ch08>` |
| コミュニケータ | communicator | 通信に参加するプロセスの集まり（`MPI_COMM_WORLD` など） | {ref}`8 <edu-ch08>` |
| SPMD | Single Program, Multiple Data | 同じプログラムを、各プロセスが別のデータで動かす形 | {ref}`8 <edu-ch08>` |
| 領域分割 | domain decomposition | 計算領域を分けて、各プロセスに割り当てること | {ref}`8 <edu-ch08>` |
| ゴーストセル（のりしろ） | ghost cell | 隣の値や境界条件の値を置くために、配列の両端に足すセル | {ref}`8 <edu-ch08>` |
| ハロー交換 | halo exchange | ゴーストセルを隣のランクの値で埋める通信 | {ref}`8 <edu-ch08>` |
| タグ | tag | MPI で、送信と受信を対応させる整数 | {ref}`8 <edu-ch08>` |
| デッドロック | deadlock | 互いに相手を待って、先へ進めなくなる状態 | {ref}`8 <edu-ch08>` |
| ノンブロッキング通信 | non-blocking communication | 開始だけして戻り、後で完了を待つ通信（`MPI_Isend`） | {ref}`8 <edu-ch08>` |
| 集団通信 | collective communication | コミュニケータの全プロセスが参加する通信（`MPI_Allreduce`） | {ref}`8 <edu-ch08>` |
| パック・アンパック | pack / unpack | 値を送信用のバッファへ詰めること／受信したバッファから配列へ戻すこと | {ref}`8 <edu-ch08>` |
| 物理境界・内部境界 | physical / internal boundary | 計算領域の端の境界／隣のランクとの境界 | {ref}`8 <edu-ch08>` |
| MPI_PROC_NULL | MPI_PROC_NULL | 相手がいないことを表すランクの値。送受信しても何も起きない | {ref}`8 <edu-ch08>` |
| MCA | Modular Component Architecture | Open MPI の内部部品を選ぶ仕組み（環境変数 `OMPI_MCA_*`） | {ref}`8 <edu-ch08>` |
| ホスト | host | CPU 側と、そのメモリ | {ref}`9 <edu-ch09>` |
| デバイス | device | GPU 側と、そのメモリ | {ref}`9 <edu-ch09>` |
| カーネル | kernel | `__global__` を付けた、GPU の多数のスレッドで実行される関数 | {ref}`9 <edu-ch09>` |
| スレッド | thread | カーネルを実行する最小の単位 | {ref}`9 <edu-ch09>` |
| ブロック（スレッドブロック） | (thread) block | スレッドの組。共有メモリと `__syncthreads` はこの中で働く | {ref}`9 <edu-ch09>` |
| グリッド | grid | 1 回の起動で並べるブロックの全体 | {ref}`9 <edu-ch09>` |
| 共有メモリ | shared memory | 同じブロックのスレッドが共有する、小さく速いメモリ | {ref}`9 <edu-ch09>` |
| 拡張ラムダ | extended lambda | `__host__ __device__` を付けたラムダ。nvcc の `--expt-extended-lambda` が必要 | {ref}`9 <edu-ch09>` |
| 非同期・同期 | asynchronous / synchronization | 起動した GPU の処理を待たずにホストが先へ進むこと／その完了を待つこと | {ref}`9 <edu-ch09>` |
| ストリーム | stream | GPU への仕事の待ち行列。中では入れた順に実行される | {ref}`9 <edu-ch09>` |
| CUDA-aware MPI | CUDA-aware MPI | GPU メモリのアドレスを直接送受信できる MPI | {ref}`9 <edu-ch09>` |
| compute capability | compute capability | GPU の世代を表す番号。`CMAKE_CUDA_ARCHITECTURES` に使う | {ref}`9 <edu-ch09>` |
| HMM | Heterogeneous Memory Management | GPU からホストのメモリを読めるようにする仕組み。有効な環境では `this` の取り込みの誤りが表に出ない | {ref}`9 <edu-ch09>` |

## IV．総合実習と本体の読解

| 語 | 英語 | 説明 | 初出 |
|---|---|---|---|
| 熱伝導方程式 | heat equation | $\partial u/\partial t = \kappa\,\partial^2 u/\partial x^2$。温度が拡散する式 | {ref}`10 <edu-ch10>` |
| 解析解 | analytical solution | 式で書ける厳密な解 | {ref}`10 <edu-ch10>` |
| 中心差分 | central difference | 左右の隣の値で微分を近似する差分 | {ref}`10 <edu-ch10>` |
| 陽解法 | explicit method | 今の時刻の値だけから次の時刻の値を求める方法 | {ref}`10 <edu-ch10>` |
| 前進オイラー法 | forward Euler method | 時間微分を前進差分で近似する 1 次精度の方法 | {ref}`10 <edu-ch10>` |
| 拡散数 | diffusion number | $r = \kappa\Delta t/\Delta x^2$。安定条件に現れる | {ref}`10 <edu-ch10>` |
| 安定条件 | stability condition | 計算が発散しない $\Delta t$ の上限（熱伝導の陽解法では $r \le 1/2$） | {ref}`10 <edu-ch10>` |
| 増幅率 | amplification factor | 1 ステップで波の振幅に掛かる係数 | {ref}`10 <edu-ch10>` |
| 離散解 | discrete solution | 差分式そのものの厳密な解 | {ref}`10 <edu-ch10>` |
| 打ち切り誤差 | truncation error | 微分を差分で近似したことによる誤差 | {ref}`10 <edu-ch10>` |
| 収束次数 | order of convergence | 格子を細かくしたとき、誤差が減る割合の指数 | {ref}`10 <edu-ch10>` |
| 状態方程式 | equation of state (EOS) | 密度と内部エネルギーから、圧力や音速を求める関係 | {ref}`11 <edu-ch11>` |
| 発散クリーニング | (hyperbolic) divergence cleaning | 補助変数 $\phi$ で $\nabla\cdot\boldsymbol{B}$ の誤差を波として運び出し、減衰させる方法 | {ref}`11 <edu-ch11>` |
| CFL 条件 | CFL condition | 最も速い波が 1 ステップで格子間隔を越えないように $\Delta t$ を決める条件 | {ref}`11 <edu-ch11>` |
| Alfvén 速度 | Alfvén speed | 磁場に沿って伝わる波の速さ $|\boldsymbol{B}|/\sqrt{4\pi\rho}$ | {ref}`11 <edu-ch11>` |
| 人工粘性 | artificial viscosity | 急な変化の近くだけに拡散を加えて、数値的な振動を抑える処理 | {ref}`11 <edu-ch11>` |
| 制限子 | limiter | 傾きの符号がそろうときだけ最小の傾きを採るなど、勾配の大きさを制限する関数（minmod など） | {ref}`11 <edu-ch11>` |
| 流束 | flux | セルの境界を通って移る量 | {ref}`11 <edu-ch11>` |
| ソース項 | source term | 方程式の右辺に足す外力や加熱 | {ref}`11 <edu-ch11>` |
| 静水圧平衡 | hydrostatic equilibrium | 重力と圧力勾配がつり合った状態 | {ref}`11 <edu-ch11>` |
| チェックポイント | checkpoint | 計算を再開するために保存する状態 | {ref}`11 <edu-ch11>` |
| 生文字列リテラル | raw string literal | `R"x( ... )x"` の形で、改行や `"` をそのまま含められる文字列 | {ref}`11 <edu-ch11>` |
