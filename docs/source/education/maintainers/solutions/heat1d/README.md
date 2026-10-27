# 実習 heat1d の解答例

`practice/heat1d/`（01 逐次、02 MPI、03 GPU）の解答例です。本文と実習のページからは案内しません。
実習の仕様を変えたら、ここも直してビルド・実行し直します。

| ファイル | 内容 |
|---|---|
| `CMakeLists.txt` | `add_subdirectory` で MISO を取り込み、3 つの実行ファイルを作る。`-DMISO_USE_CUDA=ON` で 03 を CUDA でビルド |
| `01_serial.cpp` | 逐次版（`Array1D`、`backend::Host` の `for_each`・`reduce`） |
| `02_mpi.cpp` | MPI 版（`MPI_Sendrecv` によるゴーストセルの交換、`MPI_Allreduce`、`MPI_Gather`） |
| `03_gpu.cpp` | GPU 版（`Backend` を `__CUDACC__` で切り替え。交換は `Halo` 構造体でホストを経由） |

## ビルドと実行

```bash
cmake -B build -S . -DMISO_DIR=/path/to/MISO
cmake --build build -j
cd build
./heat1d_serial 64
mpiexec -np 2 ./heat1d_mpi 64
mpiexec -np 2 ./heat1d_gpu 64
# GPU
cmake -B build-gpu -S . -DMISO_DIR=/path/to/MISO -DMISO_USE_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES=89
cmake --build build-gpu -j
```

## 検算結果（2026-10-27、MISO main e9fe8ed）

CPU: g++ 13.3、OpenMPI、Release（`-O3`）。GPU: RTX 4090 ×2、NVHPC 26.5 のコンテナの CUDA 12.9（nvcc）、`sm_89`、
同梱の OpenMPI 5.0 に `demo/shared/openmpi_config.env` を設定して実行。

### 01 逐次版（$r_\mathrm{max} = 0.4$、$t_\mathrm{end} = 0.1$）

| N | steps | max error (discrete) | max error (exact) | 比 | 次数 |
|---|---|---|---|---|---|
| 32 | 256 | 1.055e-15 | 4.136835e-04 | — | — |
| 64 | 1024 | 1.249e-14 | 1.034113e-04 | 4.00037 | 2.00013 |
| 128 | 4096 | 2.442e-15 | 2.585224e-05 | 4.00009 | 2.00003 |
| 256 | 16384 | 3.209e-13 | 6.463023e-06 | 4.00002 | 2.00001 |
| 512 | 65536 | 1.198e-12 | 1.615753e-06 | 4.00001 | 2.00000 |

- 誤差の主要項 $t\pi^4\Delta x^2(r/2 - 1/12)e^{-\pi^2 t}$ は N=32 で 4.1363e-04、N=64 で 1.0341e-04 で、実測と合う。
- 1 ステップだけ進めたときの離散解との差は 2.2e-16（N=32, 64）、3.3e-16（N=512）。
- 安定条件：N=64 で r=0.4995（`r_max=0.5`）は正常、r=0.5095 は離散解との差 9.4e-05 に増え始め、r=0.5997（`r_max=0.6`）は 5.7e+82 で発散。
  N=16, r_max=0.6 は 43 ステップしかないので発散が目立たない（解析解との差 3.1e-03）。
- r = 1/6（`0.16666666666666667`）：N=32, 64, 128 で 3.5e-07, 1.6e-08, 1.4e-09（ステップ数の切り上げで r がずれ、きれいな 4 次にはならない）。

### 02 MPI 版

- N=64 で P=1, 2, 4、N=256 で P=1, 2, 4、N=120 で P=1, 2, 3, 4：表示される誤差は逐次版と同じ、
  書き出したファイルは逐次版と `cmp` でビット単位一致（N=120 の max error (exact) は 2.941414e-05）。
- N=64, P=3：「N = 64 is not divisible by P = 3」を表示して `MPI_Abort`。

### 03 GPU 版

- CPU 用ビルド（`Backend = backend::Host`）：N=64 で P=1, 2, 4 の結果が逐次版とビット単位一致。
- GPU 用ビルド、1 ランク：N=32〜512 の max error (exact) は逐次版と同じ値。max error (discrete) は 1.059e-15, 1.247e-14, 2.439e-15, 3.208e-13, 1.198e-12（CPU と最後の桁が違う）。
- GPU 用ビルド、N=64 で P=1, 2, 4（2 GPU、P=4 は 1 GPU を 2 ランクで共有）：ファイルはビット単位一致。
- CPU と GPU の場：N=32〜512 の全セルでビット単位一致した。更新式の SASS には DFMA があり（`r * (...) + u` が FMA）、
  g++ に `-mfma` を付けて FMA を使わせても一致したので、この問題の条件ではたまたま丸めが変わらないと考えられる。
  教材では「一致は保証されない、1e-12 程度以下を目安」としている。誤差の計算（`reduce` の中の `sin` と FMA）は最後の桁が違う。
- `this` の取り込みの確認：`Halo::apply` のラムダでメンバ変数 `n` を直接使うように変えると、GPU で
  `CUDA Error: an illegal memory access was encountered .../miso/array1d.hpp 109`（次の `copy_from` の `cudaMemcpy`）で停止した。
- ラムダで `Array1D` を直接使うと、g++ で `use of deleted function 'miso::Array1D<T, miso::backend::Host>::Array1D(const ...&)'` のコンパイルエラーになることも確認した。
