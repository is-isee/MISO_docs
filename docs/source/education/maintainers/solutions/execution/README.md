# practice/execution の解答例

`practice/execution/`（ベクトル加算・縮約・1 次元移流）の解答例です。
各ページの「結果の確認」に書いた比較を自分で行って、結果を表示します。
受講者が自分のコードと見比べたり、指導者が答え合わせをしたりするために置いています。
本文と実習のページからは案内しません。

| 課題 | ファイル |
|---|---|
| 01 ベクトル加算 | `01_vector_add.cpp` |
| 02 縮約（二乗和と最大値） | `02_reduction.cpp` |
| 03 1 次元移流 | `03_advection_1d.cpp` |

3 本とも、`#ifdef __CUDACC__` で `Backend` を `backend::CUDA` と `backend::Host` から選びます。
ホストで結果を読むときは、`Array1D<double, backend::Host>` を用意して `copy_from` で写します。
このため、同じソースを CPU と CUDA の両方でビルドできます。

## ビルド

`CMakeLists.txt` は、第 1 章の「自分のプログラムから MISO を使う最小の CMake 設定」と同じ形
（`-DMISO_DIR` で MISO の場所を渡し、`add_subdirectory` で `MISO::MISO` をリンクする）です。
`ENABLE_TESTING` を `OFF` にして、MISO のユニットテストは作りません。

```bash
# CPU（backend::Host）
cmake -S . -B build_host -DMISO_DIR=/path/to/MISO
cmake --build build_host -j
# CUDA（第 9 章の後）。89 は対象 GPU の番号に置き換える。省略すると native
cmake -S . -B build_cuda -DMISO_DIR=/path/to/MISO -DUSE_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES=89
cmake --build build_cuda -j
```

ビルドフォルダの中の `01_vector_add` などを実行します。
このフォルダの中にビルドフォルダを作った場合は、コミットしないでください。

## 確認した結果（2026-10-27）

MISO の main（e9fe8ed）、g++ 13.3、CMake 3.28、OpenMPI、Release ビルド（CPU）で組んで実行しました。
CUDA でのビルド・実行は未確認です。

```text
$ ./01_vector_add
z[0] = 0, z[999] = 1668.330000
max abs error = 0, within tolerance = 1000/1000, exact = 1000/1000
$ ./02_reduction
N = 1000: sum = 272.321682359234 (diff 1.13687e-13, ok), max = 0.84093026185662145 (exact)
N = 1000000: sum = 499767.148336521 (diff 9.19681e-09, ok), max = 0.99999999999820111 (exact)
$ ./03_advection_1d
one step, NX = 256: max abs error = 0 (ok)
NX =  256, steps =  200: relative L2 error = 0.1050
NX =  512, steps =  400: relative L2 error = 0.0566
NX = 1024, steps =  800: relative L2 error = 0.0295
```

## 注意

- 02 で最大値をまとめる関数に `util::max2` を使うと、`double` が `float` に丸められて最大値が一致しません（MISO #189、2026-10 時点で open）。
  解答例では `util::fmax` を使っています。#189 が直ったら、本文と実習の注意書きも見直します。
- `reduce` の初期値は、まとめる演算の単位元（和なら 0、最大値なら `std::numeric_limits<double>::lowest()`）にします。
  Host 版は初期値を 1 回だけ結果に加え、CUDA 版は範囲外のスレッドの値として何回も使うためです（`miso/include/miso/execution.hpp`）。
- 03 の周期の折り返しは、`util` に `round` がないため `util::fmod` で書いています。
