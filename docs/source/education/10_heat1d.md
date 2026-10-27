(edu-ch10)=
# 10. 熱伝導の計算

ここまでに学んだ配列、`for_each`・`reduce`、MPI、GPU を、1 次元の熱伝導の計算に組み合わせます。
この章では、方程式を差分で解く手順と、結果が正しいかを確かめる方法を説明します。
プログラムは {ref}`実習 heat1d <edu-practice-heat1d>` の仕様に沿って、自分で書きます。

作る順番は次の 3 段階です。

1. 逐次版：1 プロセスの CPU で、`Array1D` と `backend::Host` の `for_each`・`reduce` を使って解きます。
2. MPI 版：領域を分け、隣のランクとゴーストセルの値を `MPI_Sendrecv` で交換します。
3. GPU 版：配列を `Array1D<Real, backend::CUDA>` に、ループを `backend::CUDA` の `for_each` に置き換えます。

1 と 2 は Ubuntu・Mac の CPU だけで完了できます。3 も、GPU がなければ同じソースを CPU 用にビルドして、構成が正しいことまで確かめられます。

## 目次

- {ref}`edu-ch10-equation`
- {ref}`edu-ch10-discretization`
- {ref}`edu-ch10-stability`
- {ref}`edu-ch10-boundary`
- {ref}`edu-ch10-verification`
- {ref}`edu-ch10-serial`
- {ref}`edu-ch10-mpi`
- {ref}`edu-ch10-gpu`
- {ref}`edu-ch10-miso`

(edu-ch10-equation)=
## 熱伝導方程式と解析解

温度 $u(x, t)$ が拡散してなだらかになる様子は、次の **熱伝導方程式**（heat equation）で表されます。

$$\frac{\partial u}{\partial t} = \kappa\,\frac{\partial^2 u}{\partial x^2}$$

$\kappa$ は正の定数（熱拡散率）です。この章では、次の問題を解きます。

| 項目 | 設定 |
|---|---|
| 領域 | $0 < x < 1$ |
| 境界条件 | 両端で $u = 0$（温度を固定） |
| 初期条件 | $u(x, 0) = \sin(\pi x)$ |
| 定数 | $\kappa = 1$ |
| 終了時刻 | $t_\mathrm{end} = 0.1$ |

この問題には式で書ける解（**解析解**、analytical solution）があります。

$$u(x, t) = e^{-\pi^2 \kappa t}\sin(\pi x)$$

$u = e^{-\pi^2\kappa t}\sin(\pi x)$ を方程式に代入すると、左辺も右辺も $-\pi^2\kappa u$ になり、両端で $u = 0$ も満たします。
形は $\sin(\pi x)$ のまま、振幅だけが指数関数的に減ります。$t = 0.1$ での振幅は $e^{-\pi^2 \times 0.1} \approx 0.3727$ です。

解析解があるので、計算した値がどれだけ正しいかを数値で比べられます。
MISO で解く MHD の問題では、多くの場合このような解はありません。
そのため、解析解のある簡単な問題で、プログラムの部品（差分、境界、通信）が正しいことを先に確かめておきます。

(edu-ch10-discretization)=
## 格子と差分

### セルと格子幅

領域 $[0, 1]$ を $N$ 個の等しい区間（**セル**、cell）に分けます。セルの幅（格子幅）は $\Delta x = 1/N$ です。
値はセルの中心で持ちます。$j$ 番目のセル（$j = 0, 1, \dots, N-1$）の中心は次の位置です。

$$x_j = \left(j + \tfrac12\right)\Delta x$$

MISO の格子も、値をセルの中心で持ちます。境界（$x = 0$ と $x = 1$）は、端のセルの面にあたります。

### 2 階微分の中心差分

2 階微分 $\partial^2 u/\partial x^2$ を、隣どうしの値の差で近似します。
$u_j$ をセル $j$ の値とすると、次の **中心差分**（central difference）を使います。

$$\left.\frac{\partial^2 u}{\partial x^2}\right|_{x_j} \approx \frac{u_{j-1} - 2u_j + u_{j+1}}{\Delta x^2}$$

テイラー展開すると、この近似の誤差は $\Delta x^2$ に比例します（2 次精度）。
左右 1 つずつの隣を使うので、端のセル（$j = 0$ と $j = N-1$）では、領域の外の値が 1 つずつ必要になります。
これは {ref}`edu-ch10-boundary` で扱います。

### 時間方向の陽解法

時刻を $t^n = n\,\Delta t$ と刻み、時刻 $t^n$ の値を $u_j^n$ と書きます。
時間微分を前進差分 $(u_j^{n+1} - u_j^n)/\Delta t$ で近似すると、次の更新式になります。

$$u_j^{n+1} = u_j^n + r\left(u_{j-1}^n - 2u_j^n + u_{j+1}^n\right),\qquad r = \frac{\kappa\,\Delta t}{\Delta x^2}$$

右辺は時刻 $n$ の値だけで計算できるので、新しい値を順に求められます。
このような方法を **陽解法**（explicit method）と呼び、この時間方向の近似を前進オイラー法（forward Euler method）と呼びます。
$r$ は拡散数（diffusion number）と呼ばれる無次元の数で、次節の安定条件に現れます。

Fortran で書くと、1 ステップは次のようになります（説明用の例です）。

```fortran
do j = 1, n
  u_new(j) = u(j) + r * (u(j-1) - 2.0d0 * u(j) + u(j+1))
end do
u(1:n) = u_new(1:n)
```

右辺で読む `u` と、左辺に書く `u_new` を別の配列にしています。
同じ配列に上書きすると、`u(j)` を更新した後で `u(j+1)` を計算するときに、更新済みの `u(j)` を読んでしまいます。
すると全セルが同じ時刻の値から計算されたことにならず、上の更新式とは違う計算になります。
**読む配列と書く配列を分けること**は、逐次でも並列でも必要です。並列に実行すると、どのセルが先に更新されるかが決まらないので、なおさら重要です。

(edu-ch10-stability)=
## 陽解法の安定条件

陽解法では、$\Delta t$ を大きくしすぎると、値が振動しながら増えて計算が壊れます。
どこまで大きくしてよいかを調べます。

いちばん細かい波、つまりセルごとに符号が入れ替わる分布 $u_j = (-1)^j$ を考えます。
$u_{j-1} - 2u_j + u_{j+1} = -4u_j$ なので、1 ステップで次のようになります。

$$u_j^{n+1} = (1 - 4r)\,u_j^n$$

1 ステップごとに $(1 - 4r)$ 倍されるので、$|1 - 4r| \le 1$、すなわち

$$r = \frac{\kappa\,\Delta t}{\Delta x^2} \le \frac12$$

でないと、この成分は増え続けます。これが前進オイラー法と中心差分を組み合わせたときの **安定条件**（stability condition）です。
他の波長の成分は、この成分ほど速く増えないので、この条件で全体が安定になります。
1 ステップで掛かる係数を **増幅率**（amplification factor）と呼びます。一般の波長については {ref}`edu-ch10-verification` で求めます。

格子幅を半分にすると、$\Delta t$ の上限は 4 分の 1 になります。
同じ終了時刻まで進めるには、ステップ数が 4 倍、セル数が 2 倍なので、計算量は 8 倍になります。

実習では、$r$ の上限を $r_\mathrm{max} = 0.4$ とし、終了時刻にちょうど着くように $\Delta t$ を決めます。

$$n_\mathrm{steps} = \left\lceil \frac{t_\mathrm{end}}{r_\mathrm{max}\,\Delta x^2/\kappa} \right\rceil,\qquad \Delta t = \frac{t_\mathrm{end}}{n_\mathrm{steps}}$$

$\lceil\ \rceil$ は切り上げ（C++ の `std::ceil`）です。こうすると $r \le r_\mathrm{max}$ が保たれます。

**練習 10.1** $N = 64$、$r_\mathrm{max} = 0.4$ のとき、$n_\mathrm{steps}$ と $r$ を求めてください。
また、$r_\mathrm{max} = 0.6$ にすると何が起きるかを、上の増幅率から説明してください。

<details>
<summary>解答・解説</summary>

$\Delta x = 1/64$、$\Delta x^2 = 1/4096$ なので、$r_\mathrm{max}\Delta x^2/\kappa = 0.4/4096$ です。
$n_\mathrm{steps} = \lceil 0.1 \times 4096 / 0.4 \rceil = 1024$、$\Delta t = 0.1/1024$、$r = 0.4$ です。

$r_\mathrm{max} = 0.6$ では $n_\mathrm{steps} = 683$、$r \approx 0.5997$ になり、セルごとに符号が入れ替わる成分の増幅率は $1 - 4r \approx -1.4$ です。
初期条件 $\sin(\pi x)$ にはこの成分はほとんど含まれませんが、丸め誤差（$10^{-16}$ 程度）として必ず混ざります。
それが 1 ステップごとに 1.4 倍され、683 ステップ後には $1.4^{683} \approx 10^{100}$ 倍になるので、計算が壊れます。
実際に実習の逐次版で `r = 0.6` を与えると、最大誤差は $10^{82}$ 程度になります。

</details>

(edu-ch10-boundary)=
## 境界条件とゴーストセル

### ゴーストセル

端のセル $j = 0$ の差分には $u_{-1}$ が、$j = N-1$ の差分には $u_N$ が必要です。
そこで配列の両端に 1 つずつ余分なセルを足し、そこに境界条件から決まる値を入れておきます。
この余分なセルを **ゴーストセル**（ghost cell）と呼びます。MISO の格子も、同じ考え方で `i_margin` 個のゴーストセルを持ちます（{ref}`第 8 章 <edu-ch08>`）。

配列の添字は次のようにします。C++ の配列は 0 始まりなので、物理セル $j$ は配列の添字 `i = j + 1` に入ります。

```text
配列の添字 i :   0  |  1   2   ...   N  |  N+1
               ghost|   物理セル N 個     | ghost
物理セル   j :      |  0   1   ...  N-1  |
```

ゴーストセルの値を先に入れておけば、物理セルの更新は、すべて同じ式

```text
u_new[i] = u[i] + r * (u[i-1] - 2 u[i] + u[i+1])     (i = 1, ..., N)
```

で書けます。端のセルだけ別の式を書く必要がなくなります。

### $u = 0$ の境界条件

境界 $x = 0$ は、ゴーストセル（中心 $x = -\Delta x/2$）と最初の物理セル（中心 $x = \Delta x/2$）のちょうど中間にあります。
2 つの値を直線でつないだとき、中間で 0 になるようにするには、ゴーストセルに符号を反転した値を入れます。

$$u_\mathrm{ghost,\,left} = -u_0,\qquad u_\mathrm{ghost,\,right} = -u_{N-1}$$

配列の添字で書けば `u[0] = -u[1]`、`u[N+1] = -u[N]` です。
ゴーストセルの値は、物理セルの値が変わるたびに入れ直す必要があります。**各ステップの差分の前に**ゴーストセルを埋めます。

**練習 10.2** 初期条件 $\sin(\pi x)$ では、ゴーストセルに $-u_0$ を入れることが、$\sin(\pi x)$ を $x = -\Delta x/2$ で評価した値とちょうど等しくなります。
このことを確かめてください。右端についても同様に確かめてください。

<details>
<summary>解答・解説</summary>

左端：$\sin(\pi \cdot (-\Delta x/2)) = -\sin(\pi\Delta x/2) = -u_0$ です（$\sin$ は奇関数）。

右端：ゴーストセルの中心は $x = 1 + \Delta x/2$ です。$\sin(\pi + \theta) = -\sin\theta$、$\sin(\pi - \theta) = \sin\theta$ より、
$\sin(\pi(1 + \Delta x/2)) = -\sin(\pi\Delta x/2) = -\sin(\pi(1 - \Delta x/2)) = -u_{N-1}$ です。

つまり $\sin(\pi x)$ は、ゴーストセルまで含めて同じ式で表せます。次節で、このことを使って差分の結果を式で求めます。

</details>

(edu-ch10-verification)=
## 計算結果を期待値と比べる方法

計算した値の正しさは、次の 3 つの比較で確かめます。目的が違うので、一致の基準も違います。

| 比較 | 確かめること | 期待される差 |
|---|---|---|
| 離散解との比較 | 差分・境界・時間積分を式どおりに計算できているか | 丸め誤差程度 |
| 解析解との比較と収束次数 | 差分の近似が正しい精度を持つか | 格子を 2 倍にすると約 1/4 |
| ランク数・計算場所を変えた比較 | 領域分割、通信、GPU への置き換えが正しいか | 0（ビット単位で一致）または丸め誤差程度 |

### 離散解との比較

$\sin(\pi x_j)$ に中心差分を当てはめると、三角関数の加法定理 $\sin(a - b) + \sin(a + b) = 2\sin a\cos b$ から

$$\sin(\pi x_{j-1}) - 2\sin(\pi x_j) + \sin(\pi x_{j+1}) = -4\sin^2\!\left(\frac{\pi\Delta x}{2}\right)\sin(\pi x_j)$$

となり、形は $\sin(\pi x_j)$ のまま、一定の係数が掛かります。
練習 10.2 のとおり境界でもこの関係が成り立つので、更新式の 1 ステップで、全セルの値に同じ係数

$$G = 1 - 4r\sin^2\!\left(\frac{\pi\Delta x}{2}\right)$$

が掛かります。これが $\sin(\pi x)$ の成分に対する増幅率です。$n$ ステップ後の値は、次の式で計算できます。

$$u_j^n = G^n \sin(\pi x_j)$$

これを **離散解** と呼ぶことにします。差分と時間積分を式どおりに計算していれば、数値解と離散解の違いは丸め誤差だけです。
倍精度（`double`）で数千ステップ程度なら、全セルの最大の差は $10^{-12}$ 程度以下になります。
添字を 1 つずらした、ゴーストセルを埋め忘れた、といった誤りがあると、この差は丸め誤差よりずっと大きくなります。

### 解析解との比較と収束次数

解析解との差には、差分による近似の誤差（打ち切り誤差、truncation error）が含まれます。
全セルの最大の差

$$E_N = \max_j \left| u_j^{n_\mathrm{steps}} - e^{-\pi^2\kappa t_\mathrm{end}}\sin(\pi x_j)\right|$$

を、$N$ を 2 倍ずつ変えて計算します。
空間は 2 次精度、時間は 1 次精度ですが、$\Delta t \propto \Delta x^2$ としているので、時間の誤差も $\Delta x^2$ に比例します。
したがって、$N$ を 2 倍にすると誤差は約 1/4 になるはずです。
2 つの格子での誤差の比から、**収束次数**（order of convergence）$p$ を次の式で求めます。

$$p = \log_2\frac{E_N}{E_{2N}}$$

$p \approx 2$ になれば、差分の近似が期待どおりの精度を持っていることになります。
実習の条件（$r_\mathrm{max} = 0.4$）での値は次のとおりです。

| $N$ | $n_\mathrm{steps}$ | $E_N$ | $E_{N/2}/E_N$ | $p$ |
|---|---|---|---|---|
| 32 | 256 | $4.1368 \times 10^{-4}$ | — | — |
| 64 | 1024 | $1.0341 \times 10^{-4}$ | 4.0004 | 2.0001 |
| 128 | 4096 | $2.5852 \times 10^{-5}$ | 4.0001 | 2.0000 |
| 256 | 16384 | $6.4630 \times 10^{-6}$ | 4.0000 | 2.0000 |
| 512 | 65536 | $1.6158 \times 10^{-6}$ | 4.0000 | 2.0000 |

誤差は丸め誤差よりずっと大きいので、離散解との比較のように丸め誤差程度の一致は求めません。
見るのは「格子を細かくすると、決まった割合で誤差が減るか」です。

**練習 10.3** 表の $N = 32$ と $N = 64$ の $E_N$ から、収束次数 $p$ を計算してください。
また、誤差を $10^{-6}$ 以下にするには、$N$ をおよそいくつにすればよいかを、表から見積もってください。

<details>
<summary>解答・解説</summary>

$E_{32}/E_{64} = 4.1368\times10^{-4} / 1.0341\times10^{-4} \approx 4.0004$、$p = \log_2 4.0004 \approx 2.000$ です。

$E_N \propto N^{-2}$ なので、$E_{512} = 1.6\times10^{-6}$ から $10^{-6}$ にするには $N$ を $\sqrt{1.6} \approx 1.27$ 倍、つまり $N \approx 650$ 程度にします。
このときステップ数は $N^2$ に比例して増え、計算量は $N^3$ に比例します。

</details>

<details>
<summary>コラム：誤差の主要項と r = 1/6</summary>

離散解 $G^n\sin(\pi x_j)$ と解析解の振幅の違いを $\Delta x$ について展開すると、主要な項は次のようになります（$\kappa = 1$ の場合）。

$$E_N \approx t_\mathrm{end}\,\pi^4\,\Delta x^2\left(\frac{r}{2} - \frac{1}{12}\right)e^{-\pi^2 t_\mathrm{end}}$$

$-1/12$ は空間の中心差分の誤差、$r/2$ は前進オイラー法の誤差から来ます。
$r = 0.4$、$N = 32$ を代入すると $4.136\times10^{-4}$ となり、上の表とよく合います。

2 つの項は符号が逆なので、$r = 1/6$ にすると主要項が打ち消し合い、誤差は $\Delta x^4$ に比例するようになります。
実習の逐次版で `r = 0.16666666666666667` を与えると、$N = 64$ で誤差は $1.6\times10^{-8}$ 程度まで下がります
（$n_\mathrm{steps}$ を切り上げるので $r$ がわずかに 1/6 からずれ、きれいな 4 次にはなりません）。
この打ち消しは、この方程式と差分の組み合わせに特有のもので、一般の問題では期待できません。

</details>

### ランク数・計算場所を変えた比較

MPI 版では、全体の格子、初期条件、$\Delta t$、更新式を、ランク数によらず同じにします。
各セルの更新は、そのセルと左右の隣の値だけで決まり、計算の順番（足し算の順序）も変わりません。
そのため、ゴーストセルに正しい値が届いていれば、**ランク数を変えても結果はビット単位で一致します**。
逐次版とも一致します。

一致を確かめるには、全セルの値を 17 桁で書き出したファイルどうしを比べます。
`cmp` はファイルが 1 バイトでも違えば、最初に違う場所を表示します。何も表示されなければ一致です。

```bash
cmp result_N64_serial.txt result_N64_P4.txt
```

最大誤差の比較だけでは、ずれがあっても気づかないことがあります。全セルの値を比べてください。

GPU 版と CPU 版の比較では、ビット単位の一致は保証されません。
GPU のコンパイラは掛け算と足し算を 1 回の丸めで計算する命令（FMA、fused multiply-add）を使うことがあり、`sin` などの数学関数の実装も CPU と違います。
最後の桁が違うことがあるので、全セルの最大の差を見て、値の大きさに対して $10^{-12}$ 程度以下なら一致とみなします。
最大の差は、たとえば次のように `awk` で求められます（2 つのファイルとも 1 列目が $x$、2 列目が $u$）。

```bash
paste result_N64_P1.txt result_N64_P1_gpu.txt \
  | awk '{d = $2 - $4; if (d < 0) d = -d; if (d > m) m = d} END {print m}'
```

計算結果の比べ方は {ref}`参考資料「テストと結果の比較」 <edu-ref-testing>` にもまとめています。

(edu-ch10-serial)=
## 逐次版の組み立て

{ref}`実習 01 <edu-practice-heat1d-01>` では、ここまでの式を MISO の `Array1D`・`for_each`・`reduce` で書きます。
配列とループの基本は {ref}`第 7 章 <edu-ch07>` で説明したとおりです。ここでは、この実習で気をつける点を説明します。

### 値の型

`using Real = double;` として倍精度で計算します。
MISO のデモは `using Real = float;`（単精度）で動かしていますが、単精度の丸め誤差は $10^{-7}$ 程度あり、
離散解との比較（$10^{-12}$ 程度）や、$N = 512$ での誤差（$10^{-6}$ 程度）を見分けられません。

### 配列とビュー

物理セル $N$ 個とゴーストセル 2 個で、`Array1D<Real> u(N + 2)` を確保します。更新先の `u_new` も同じ大きさで別に確保します。

`for_each` に渡すラムダの中では、`Array1D` そのものではなく、`view()` で取り出したビューを使います。
`MISO_LAMBDA` は値のコピーで変数を取り込む（`[=]`）ので、`Array1D` を直接使うと、コピーが禁止された `Array1D` をコピーしようとしてコンパイルエラーになります。

```text
error: use of deleted function 'miso::Array1D<T, miso::backend::Host>::Array1D(const miso::Array1D<T, miso::backend::Host>&) [with T = double]'
```

ビューは、データの場所（ポインタ）と大きさだけを持つ軽いオブジェクトなので、コピーしても配列の中身は複製されません（{ref}`第 4 章 <edu-ch04>`）。

### 1 ステップの流れ

1 ステップは次の順に進めます。

1. ゴーストセルに境界条件の値を入れる（`u[0] = -u[1]`、`u[N+1] = -u[N]`）。
2. 物理セル（添字 `1` から `N`）について、`u` を読んで `u_new` に書く（`for_each`、範囲は `Range1D{1, N + 1}`）。
3. `u` と `u_new` を入れ替える。

ゴーストセルの 2 つの値は、逐次版では `u[0] = ...` と直接書いても動きます。
ただし後で GPU に移すときは、GPU 上の配列にホストから添字で書き込めません。
はじめから `Range1D{0, 1}`（1 回だけ回るループ）の `for_each` の中で書いておくと、GPU 版にそのまま移せます。

3 の入れ替えには `std::swap(u, u_new);`（`<utility>`）が使えます。
`Array1D` はムーブ（中身のポインタの付け替え）ができるので、要素をコピーせずに 2 つの配列の中身が入れ替わります。
ビューは入れ替え前のデータを指したままなので、**入れ替えの後は、次のステップでビューを取り直します**。
ステップのループの中で毎回 `view()` を呼べば、この間違いは起きません。

### 最大誤差を reduce で求める

全セルの最大誤差は `reduce` で求めます。初期値 `Real(0)`、各セルの値を返すラムダ、2 つの値から大きい方を返すラムダを渡します。
誤差は 0 以上なので、`Real(0)` が最大値の単位元の代わりになります（{ref}`第 7 章 <edu-ch07-reduce>`）。

```cpp
auto max_op = MISO_LAMBDA(Real a, Real b) { return a > b ? a : b; };
```

各セルの値を返すラムダでは、$x_j$ と解析解（または離散解）を計算し、差の絶対値を返します。
数学関数は `miso::util::sin`、`miso::util::exp`、`miso::util::fabs`（`miso/include/miso/utility.hpp`）を使うと、GPU 版でも同じ書き方で使えます。
円周率は `miso::pi<Real>`（`miso/include/miso/constants.hpp`）です。

MISO の実例として、`miso/include/miso/execution.hpp` の Host 版の `for_each` と `reduce` をもう一度見ておきます。
どちらも、ただの `for` 文でラムダを呼んでいるだけです。

```cpp
/// @brief 1D parallel (concurrent) for loop in the Host backend.
template <class F> inline void for_each(backend::Host, Range1D range, F f) {
  for (int i = range.begin; i < range.end; ++i) {
    f(i);
  }
}
```

```cpp
/// @brief 1D parallel reduction in the Host backend.
template <class T, class F, class Op>
inline T reduce(backend::Host, Range1D range, T init, F map, Op op) {
  T acc = init;
  for (int i = range.begin; i < range.end; ++i)
    acc = op(acc, map(i));
  return acc;
}
```

### 部分ごとに確かめる

一度に全体を書いてから動かすと、どこが間違っているのか分かりにくくなります。次の順に、確かめながら進めます。

| 作る段階 | 確かめること |
|---|---|
| 初期条件 | 物理セルの値を書き出し、$\sin(\pi x_j)$ になっていること |
| 1 ステップ | 離散解 $G\sin(\pi x_j)$ との最大の差が丸め誤差程度であること |
| $n_\mathrm{steps}$ ステップ | 離散解 $G^{n}\sin(\pi x_j)$ との差が $10^{-12}$ 程度以下、解析解との差が上の表と合うこと |
| $N$ を変える | 収束次数が 2 に近いこと |

(edu-ch10-mpi)=
## MPI 版：領域分割とゴーストセルの交換

{ref}`実習 02 <edu-practice-heat1d-02>` では、逐次版を $P$ ランクで分担します。
MPI の基本（ランク、`MPI_Sendrecv`、`MPI_Allreduce`）は {ref}`第 8 章 <edu-ch08>` で説明しました。
この実習では、MISO の `mpi::Shape` は使わず、1 次元の分割と交換を自分で書きます。

### 各ランクの格子

全体のセル数 $N$ を $P$ ランクで等分し、各ランクは $n = N/P$ 個の物理セルを持ちます（$N$ が $P$ で割り切れる場合だけを扱います）。
ランク $p$ の配列は `n + 2` 要素で、ローカルな添字 `i`（1〜`n`）の物理セルは、全体の番号 $j = p\,n + (i - 1)$ にあたります。

```text
全体（N = 8, P = 2）:   j = 0 1 2 3 | 4 5 6 7
ランク 0 の配列      :  [g] 0 1 2 3 [g]      g はゴーストセル
ランク 1 の配列      :          [g] 4 5 6 7 [g]
```

初期条件の $x_j$ は、全体の番号 $j$ から逐次版と同じ式 $x_j = (j + 1/2)\Delta x$ で求めます。
$\Delta x$、$\Delta t$、$n_\mathrm{steps}$ も全体の $N$ から決め、ランク数によって変えません。

### 隣のランクとゴーストセルの交換

ランク $p$ の左隣はランク $p - 1$、右隣はランク $p + 1$ です。
領域の左端（ランク 0）の左と、右端（ランク $P - 1$）の右には隣がありません。
そこでは隣のランク番号の代わりに `MPI_PROC_NULL` を使います。
`MPI_PROC_NULL` を相手にした送受信は何もせずにすぐ終わり、受信用の場所の値も変わりません。
こうすると、端のランクも他のランクと同じ呼び出しを書けます。

各ステップの差分の前に、次の 2 回の `MPI_Sendrecv` で、左右のゴーストセルを埋めます。

| 呼び出し | 送る値 → 相手 | 受け取る場所 ← 相手 |
|---|---|---|
| 1 回目（タグ 0） | 最初の物理セル `u[1]` → 左隣 | 右のゴーストセル `u[n+1]` ← 右隣 |
| 2 回目（タグ 1） | 最後の物理セル `u[n]` → 右隣 | 左のゴーストセル `u[0]` ← 左隣 |

1 回目では、全ランクがそろって「左へ送り、右から受け取る」ので、ランク $p$ が左へ送った値は、ランク $p - 1$ の右のゴーストセルに入ります。
`MPI_Sendrecv` は送信と受信を組にして行うので、全ランクが同時に送信だけを始めて互いに待ち合う（デッドロック）ことがありません。

交換の後、領域の端を持つランクだけが、物理境界の値（`u[0] = -u[1]` または `u[n+1] = -u[n]`）を入れます。
左隣が `MPI_PROC_NULL` かどうかで、自分が左端を持つかを判断できます。
内部の境界（ランクとランクの間）では隣から受け取った値を、物理境界では境界条件の値を使う、という区別は MISO 本体でも同じです。

`Array1D<Real>`（ホストの配列）の要素のアドレスは `&u[1]` のように取れるので、それをそのまま `MPI_Sendrecv` に渡せます。

### 誤差の集計と結果の収集

各ランクは、自分の物理セルについて `reduce` で最大誤差を求めます。
全体の最大誤差は、それらの最大値です。`MPI_Allreduce` に `MPI_MAX` を指定して求めます。

ファイルに書き出すときは、`MPI_Gather` で全ランクの物理セルをランク 0 に集めます。
各ランクが同じ個数（$n$ 個）を送ると、ランク 0 の受信用の配列にランク番号の順に並ぶので、全体の番号 $j$ の順になります。

```cpp
// 引数: 送る場所, 送る個数, 型, 受け取る場所, 1 ランクから受け取る個数, 型, 受け取るランク, コミュニケータ
MPI_Gather(&u[1], n, MPI_DOUBLE, u_all.data(), n, MPI_DOUBLE, 0, comm);
```

受け取る場所 `u_all` は、ランク 0 だけが $N$ 要素を用意すれば足ります（他のランクでは使われません）。
なお、実習で $N$ が $P$ で割り切れない場合に使う `MPI_Abort(comm, 1)` は、そのコミュニケータのすべてのプロセスを終了させる関数です。

**練習 10.4** MPI 版で、全体の最大誤差はランク数によらず逐次版とビット単位で一致します。
一方、全セルの値の合計（$\sum_j u_j$）を各ランクで計算して `MPI_Allreduce`（`MPI_SUM`）で足し合わせると、ランク数によって最後の桁が変わることがあります。
この違いが生じる理由を説明してください。

<details>
<summary>解答・解説</summary>

各セルの値そのものはランク数によらずビット単位で一致します（{ref}`edu-ch10-verification`）。

最大値は、どの順に比べても同じ値が選ばれ、比べるときに丸めも起きません。したがって集計の順序によらず同じ結果になります。

合計は、浮動小数点の足し算が結合法則を満たさない（$(a + b) + c$ と $a + (b + c)$ の丸めが違う）ため、足す順序で結果が変わります。
逐次版は $u_0$ から順に足しますが、MPI 版は各ランクの部分和を先に作ってから足すので、ランク数が変わると足す順序が変わります。
その差は丸め誤差程度です。ランク数を変えた比較で合計を使うときは、ビット単位の一致を求めず、丸め誤差程度の差を許します。

</details>

(edu-ch10-gpu)=
## GPU 版：配列とループを GPU へ

{ref}`実習 03 <edu-practice-heat1d-03>` では、MPI 版の配列とループを GPU へ移します。
ホストとデバイスのメモリ、`MISO_LAMBDA`、`this` の取り込みについては {ref}`第 9 章 <edu-ch09>` で説明しました。
GPU がない場合も、同じソースを CPU 用にビルドして MPI 版と同じ結果になることを確かめられます。

### 計算場所を型で選ぶ

MISO のデモと同じく、計算場所を表す型 `Backend` を 1 か所で決めます。

```cpp
#ifdef __CUDACC__
using Backend = backend::CUDA;
#else
using Backend = backend::Host;
#endif
```

`__CUDACC__` は、ソースを CUDA のコンパイラ（`nvcc`）でコンパイルしているときにだけ定義されるマクロです。
配列は `Array1D<Real, Backend>`、ループは `for_each(Backend{}, ...)`、縮約は `reduce(Backend{}, ...)` と書きます。
`g++` でビルドすれば `Backend` は `backend::Host` になり、MPI 版と同じ計算をします。
ラムダを書く `MISO_LAMBDA` も、`miso/include/miso/execution.hpp` で同じように切り替わります（CUDA では `[=] __host__ __device__`、CPU では `[=]`）。

### GPU 上の配列への読み書き

`Array1D<Real, backend::CUDA>` の中身は GPU のメモリにあります。
ホストの配列と違って `operator[]` を持たないので、ホストのコードから `u[i]` と書くとコンパイルエラーになります。
GPU 上の値を読み書きする方法は 2 つです。

- `for_each` のラムダの中で、ビューを通して読み書きする（ラムダは GPU で実行されます）。
- ホストの `Array1D<Real>` との間で `copy_from` で丸ごと写す。

`miso/include/miso/array1d.hpp` の `Array1D<T, backend::CUDA>::copy_from` は、ホストの配列から GPU へ `cudaMemcpy` で写します。

```cpp
  /// @brief Copy data from another Array1D in host memory.
  void copy_from(const Array1D<T, backend::Host> &other) {
    assert(other.data() && data());
    assert(other.size() == size());
    MISO_CUDA_CHECK(cudaMemcpy(data(), other.data(), sizeof(T) * size(),
                               cudaMemcpyHostToDevice));
  }
```

逆向き（GPU からホストへ）は、ホストの配列の `copy_from` に GPU の配列を渡します。
初期条件、境界条件、更新、誤差の計算は、すべて `for_each`・`reduce` の中で GPU 上で行います。
ホストへ写すのは、ゴーストセルの交換に使う値と、最後に書き出す結果だけです。

### ゴーストセルの交換

`MPI_Sendrecv` に GPU のメモリのアドレスを渡せるかは、MPI が GPU に対応しているか（CUDA-aware MPI、第 9 章）によります。
この実習では、どの MPI でも動くように、送受信する値をいったんホストの配列に写します。

1. 送る 2 つの値（最初と最後の物理セル）を、`for_each` で GPU 上の小さな配列 `send`（2 要素）に詰める。
2. `send` をホストの配列 `h_send` へ `copy_from` で写す。
3. `h_send` と `h_recv`（ホスト、2 要素）で、MPI 版と同じ 2 回の `MPI_Sendrecv` を行う。
4. `h_recv` を GPU の配列 `recv` へ `copy_from` で写す。
5. `for_each` で、`recv` の値（隣がない側は境界条件の値）をゴーストセルに入れる。

ストリームを指定しない `copy_from`（`cudaMemcpy`）は、それより前に起動した GPU の処理が終わってから写し、写し終わるまで戻りません。
そのため、1 で詰めた値が 2 で写される前に変わることはありません。

### this を取り込まない

交換の手順を構造体のメンバ関数にまとめると、メンバ変数（物理セル数 `n` など）をラムダの中で使いたくなります。
しかしラムダの中でメンバ変数 `n` を使うと、実際には `this->n` として、`this`（ホストのメモリにある構造体を指すポインタ）が取り込まれます。
GPU はそのポインタの先を読めないので、実行時に失敗します。
エラーはラムダの場所ではなく、その後の CUDA の呼び出し（たとえば次の `copy_from`）で、次のように報告されます。

```text
CUDA Error: an illegal memory access was encountered .../miso/include/miso/array1d.hpp ...
```

メンバ変数は、ラムダの前でローカル変数にコピーしてから使います。
MISO 本体でも同じことをしています。`miso/include/miso/mhd_integrator_cuda.cuh` の `Integrator<Real, backend::CUDA>::cfl` から抜粋します。

```cpp
    // device lambdas must not capture `this`
    const Real cfl_number_ = cfl_number;
```

**練習 10.5** 次の構造体のメンバ関数を GPU 版でビルドして実行すると、`illegal memory access` で止まります（説明用の例です）。
原因の行を挙げ、直し方を説明してください。

```cpp
struct Boundary {
  int n;  // 物理セル数
  void apply(Array1D<Real, Backend> &u) {
    auto uv = u.view();
    for_each(Backend{}, Range1D{0, 1}, MISO_LAMBDA(int) {
      uv[0] = -uv[1];
      uv[n + 1] = -uv[n];
    });
  }
};
```

<details>
<summary>解答・解説</summary>

`uv[n + 1] = -uv[n];` の行です。`n` はメンバ変数なので、ラムダの中では `this->n` として読まれ、`this` が取り込まれます。
GPU で実行されるラムダが、ホストのメモリにある構造体を読もうとして失敗します。

`uv` はローカル変数（ビュー）なので、値のコピーで取り込まれ、問題ありません。

ラムダの前に `const int n_ = n;` を置き、ラムダの中では `n_` を使います。

```cpp
    const int n_ = n;
    for_each(Backend{}, Range1D{0, 1}, MISO_LAMBDA(int) {
      uv[0] = -uv[1];
      uv[n_ + 1] = -uv[n_];
    });
```

CPU 用にビルドしたときは `this` の先をそのまま読めるので、この誤りに気づきません。
GPU 用に書き換えるときは、ラムダの中にメンバ変数がないかを確かめてください。

</details>

### CPU と GPU の比較

GPU 版を、MPI 版と同じ $N$ とランク数で実行し、全セルの値を比べます。
{ref}`edu-ch10-verification` で述べたとおり、最後の桁が違うことがあるので、最大の差が $10^{-12}$ 程度以下なら一致とみなします。
GPU 版の中で、ランク数を変えた比較（1 ランクと複数ランク）は、MPI 版と同じくビット単位で一致します。

GPU 版をビルドするには、CMake に `-DMISO_USE_CUDA=ON` を与え、ソースを CUDA としてコンパイルさせます。
設定の詳細は実習の仕様に書いています。GPU で困ったときは {ref}`参考資料「GPU で困ったとき」 <edu-ref-gpu-troubleshooting>` を参照してください。

(edu-ch10-miso)=
## MISO 本体との対応

この章で書いた処理は、MISO 本体では次の場所にあります。

| この章の処理 | MISO 本体 |
|---|---|
| 配列 `u` と `u_new` | `Array3D` をまとめた `mhd::Fields`（`miso/include/miso/mhd_fields.hpp`） |
| 各ステップのゴーストセルの交換 | `mhd::HaloExchanger::apply`（`miso/include/miso/mhd_halo_exchange.hpp`） |
| 隣がないときの `MPI_PROC_NULL` | `mpi::Shape` の `x_procs_pos` などが `MPI_PROC_NULL` かを調べてから送受信する |
| 物理境界の値 | 境界条件（`miso/include/miso/boundary_condition.hpp` と各デモの境界条件） |
| `reduce` と `MPI_Allreduce` による集計 | `Integrator::cfl`（$\Delta t$ を、CUDA 版は `reduce`、Host 版は `for` 文で求め、`MPI_MIN` で全体の最小値を取る） |
| ホストを経由した交換 | MISO の GPU 版は、GPU のメモリのアドレスをそのまま MPI に渡す（CUDA-aware MPI を使う） |

MISO の `HaloExchanger::apply` は、`MPI_Sendrecv` の代わりに `MPI_Isend`・`MPI_Irecv` と `MPI_Waitall` を使い、
3 方向・9 つの変数をまとめて送ります。送る値をバッファに詰め、受け取った値をゴーストセルに戻す流れは、この章の GPU 版と同じです。

{ref}`第 11 章 <edu-ch11>` では、MISO の時間発展を `main()` から順に読み、ここで書いた処理と本体のクラス・関数を対応づけます。
