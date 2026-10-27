(edu-practice-heat1d-01)=
# 01. 逐次版：CPU で熱伝導を解く

{ref}`共通の設定 <edu-practice-heat1d>` の問題を、1 プロセスの CPU で解くプログラム `heat1d_serial` を書きます。
式と確かめ方は {ref}`第 10 章 <edu-ch10>` で説明しています。

## 使うもの

| もの | ヘッダ |
|---|---|
| `miso::Array1D<Real>`（ホストの配列） | `miso/array1d.hpp` |
| `miso::for_each`、`miso::reduce`、`miso::Range1D`、`MISO_LAMBDA`、`miso::backend::Host` | `miso/execution.hpp` |
| `miso::pi<Real>` | `miso/constants.hpp` |
| `miso::util::sin`、`util::exp`、`util::fabs` | `miso/utility.hpp` |

`#include <miso/array1d.hpp>` のように、`miso/` を付けて読み込みます。

## 作るもの

### 配列

- 物理セル数 $N$ に両端のゴーストセルを足して、`Array1D<Real> u(N + 2)` と、更新先の `u_new(N + 2)` を確保します。
- 物理セル $j$（$j = 0, \dots, N-1$）は配列の添字 `j + 1` に入ります。物理セルの範囲は `Range1D{1, N + 1}` です。

### 手順

1. **初期条件**：`for_each` で、物理セルに $\sin(\pi x_j)$ を入れます。
2. **時間積分**：$n_\mathrm{steps}$ 回、次を繰り返します。
   1. ゴーストセルに境界条件の値を入れます：`u[0] = -u[1]`、`u[N+1] = -u[N]`。
      後で GPU に移せるよう、`for_each(backend::Host{}, Range1D{0, 1}, ...)` の中で書きます。
   2. `for_each` で、物理セルについて次の式を計算します。`u` を読み、`u_new` に書きます。

      ```text
      u_new[i] = u[i] + r * (u[i-1] - 2 u[i] + u[i+1])
      ```

      `2` は `Real(2)` と書きます。$r = \kappa\Delta t/\Delta x^2$ です。
   3. `std::swap(u, u_new);` で配列を入れ替えます。
3. **誤差**：`reduce` で、離散解との最大の差と、解析解との最大の差を求めます。
4. **出力**：{ref}`共通の設定 <edu-practice-heat1d>` の 3 行を表示し、結果を `result_N<N>_serial.txt`（$N = 64$ なら `result_N64_serial.txt`）に書き出します。

### 気をつけること

- ラムダの中では `Array1D` ではなくビュー（`auto uv = u.view();`）を使います。
- `std::swap` の後は、ビューが入れ替え前のデータを指しています。ビューはステップのループの中で毎回取り直します。
- `reduce` の最大値の演算は `MISO_LAMBDA(Real a, Real b) { return a > b ? a : b; }` と書きます。初期値は `Real(0)` です（差の絶対値は 0 以上なので、`Real(0)` が最大値の単位元の代わりになります）。
- 離散解の係数 $G^{n_\mathrm{steps}}$ は、ホストで `std::pow(G, n_steps)`（`<cmath>`）として 1 回だけ計算し、ラムダに取り込みます。

## 確かめること

1. **初期条件**：時間積分を 0 ステップにして（または初期条件の直後に）書き出し、$u_j = \sin(\pi x_j)$ になっていることを確かめます。
2. **1 ステップ**：$n_\mathrm{steps}$ の代わりに 1 ステップだけ進め、離散解 $G\sin(\pi x_j)$ との最大の差が $10^{-15}$ 程度であることを確かめます。
3. **既定の入力**（$N = 64$）：次の値になることを確かめます。

   ```text
   N = 64, steps = 1024, r = 0.4000
   max error (discrete) = 1.249e-14
   max error (exact)    = 1.034113e-04
   ```

   `max error (discrete)` は丸め誤差なので、コンパイラや書き方によって変わります。$10^{-12}$ より十分小さければ問題ありません。
   `max error (exact)` は 4〜5 桁目まで一致するはずです。
4. **収束次数**：$N = 32, 64, 128, 256, 512$ で実行し、`max error (exact)` を表にして、隣どうしの比と収束次数 $p = \log_2(E_N/E_{2N})$ を求めます。

   | $N$ | steps | `max error (exact)` |
   |---|---|---|
   | 32 | 256 | 4.136835e-04 |
   | 64 | 1024 | 1.034113e-04 |
   | 128 | 4096 | 2.585224e-05 |
   | 256 | 16384 | 6.463023e-06 |
   | 512 | 65536 | 1.615753e-06 |

   $p$ がどれも 2.000 に近くなります。`max error (discrete)` は、どの $N$ でも $10^{-11}$ 以下です。
5. **安定条件**（任意）：`./heat1d_serial 64 0.6` を実行し、誤差が非常に大きくなる（計算が壊れる）ことを確かめます。
   `./heat1d_serial 64 0.5` では壊れないことも確かめます。

離散解との差が大きいときは、ゴーストセルの値、物理セルの範囲（`Range1D` の上限は含まない）、$x_j$ の式（$+1/2$ と添字のずれ）を順に調べます。
離散解とは合うのに解析解との差が表と違うときは、$\Delta t$、$n_\mathrm{steps}$、$t_\mathrm{end}$ を調べます。

## 次へ進む目安

上の 3 と 4 の値が得られたら、{ref}`02. MPI 版 <edu-practice-heat1d-02>` へ進みます。
書き出した `result_N64_serial.txt` などは、MPI 版との比較に使うので残しておきます。
