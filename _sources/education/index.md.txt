(edu-index)=
# MISO education

Fortran で数値計算コードを書いてきた経験を足がかりに、MISO の C++ コードを読み、直し、自分で計算を足せるようになるための教材です。
変数・配列・ループ・サブルーチンの知識から始めます。Fortran の派生型やポインタ、C++ の知識は前提にしません。

MISO は GitHub（<https://github.com/is-isee/MISO>）で公開しています。本文に示すソースのパス（`miso/include/miso/execution.hpp` など）は、MISO を clone したフォルダから辿ってください。
コマンド例の `/path/to/MISO` は、自分が MISO を置いた場所に置き換えます。

## 本文

教材は 4 部・11 章です。C++ で小さな計算を書くことから始め、配列と並列計算を学び、熱伝導を作ってから MISO の時間発展を読みます。

### I．準備

1. {ref}`開発環境と CMake <edu-ch01>` — Ubuntu・Mac で道具を揃え、CMake の仕組みと MISO のビルド手順を学びます。

### II．C++ の基礎

2. {ref}`変数・関数・制御文 <edu-ch02>` — 短い逐次計算を書き、関数やファイルへ処理を分けます。
3. {ref}`参照・ポインタと寿命 <edu-ch03>` — どの変数を読み書きし、いつまで使えるかを追います。
4. {ref}`クラス・構造体 <edu-ch04>` — データと操作をまとめ、初期化と後片付けを学びます。
5. {ref}`継承と CRTP <edu-ch05>` — 共通の処理を基底クラスに置き、問題ごとの部品を派生クラスで与えます。
6. {ref}`テンプレート <edu-ch06>` — 型（`Real`）や計算場所（`Backend`）を指定した関数・クラスを読み書きします。

### III．MISO における並列計算

7. {ref}`配列・ラムダと CPU での並列ループ <edu-ch07>` — 配列を用意し、`for_each` と `reduce` で計算します。
8. {ref}`MPI による領域分割 <edu-ch08>` — 領域を分け、隣の値の交換や全体の集計を行います。
9. {ref}`GPU（CUDA）での計算とデータ転送 <edu-ch09>` — 計算場所とメモリを区別し、同じラムダを GPU で動かします。

### IV．総合実習と本体の読解

10. {ref}`熱伝導の計算 <edu-ch10>` — 差分・時間積分・境界処理を組み合わせ、逐次版を並列化します。
11. {ref}`MISO の時間発展を読む <edu-ch11>` — 自分で書いた小さな計算と、MISO 本体の役割分担を対応づけます。

Ubuntu と Mac の CPU だけでも、**第 1〜8 章 → 第 10 章の逐次・MPI 部分 → 第 11 章**と進められます。
第 9 章は GPU がなくても読めます。

章内の練習の答えは「解答・解説」として折りたたんであります。
まとまったプログラムを作る課題は {ref}`実習一覧 <edu-practice>` に置き、本文の該当箇所から案内しています。

## 必要なときに参照する資料

{ref}`参考資料一覧 <edu-ref>` には、用語集、Fortran との対応表、記号の早見表、テストと結果の比較方法、GPU のトラブルシューティングをまとめています。
本文を読むために、先に通読する必要はありません。

```{toctree}
:hidden:
:maxdepth: 1

01_environment_setup
02_cpp_basics
03_references_and_pointers
04_classes_and_structs
05_inheritance
06_templates
07_arrays_and_parallel
08_mpi
09_gpu
10_heat1d
11_reading_miso
practice/index
reference/index
```
