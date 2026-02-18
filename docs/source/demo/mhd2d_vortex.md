# 2D MHD Test: Orszag-Tang Vortex

Orszag-Tang渦問題 {cite}`orszag_1979JFM....90..129O` は、磁気流体力学（MHD）コードの標準的なテスト問題です。この問題は、磁化された媒体における2つの渦流の相互作用を含み、複雑な構造と乱流を形成します。

## Location

`demo/mhd2d_vortex/`

## Geometry

- $0 \leq x \leq 1$
- $0 \leq y \leq 1$.

## Initial Conditions

初期条件は以下のように設定されます。比熱比は$\gamma = 5/3$とします。

$$
\begin{align*}
\rho &= 1, \\
p &= \frac{1}{\gamma}, \\
v_x &= -v_0 \sin(2\pi y), \\
v_y & = v_0 \sin(2\pi x), \\
v_z & = 0, \\
B_x & = B_0 \sin(2\pi y),\\
B_y & = B_0 \sin(4\pi x),\\
B_z & = 0,
\end{align*}
$$

ここで$v_0 = 1$、$B_0 = \sqrt{4\pi}/\gammaです。

## Boundary Conditions

周期境界条件を全ての境界に対して全ての物理量に設定します。

## Results

可視化用のPythonスクリプトが用意されています。
結果のプロットは `demo/mhd2d_vortex/figs` に保存されます。

```shell
cd demo/mhd2d_vortex
python plot_data.py
```

![mhd2d_vortex](../_static/images/mhd_vortex_2d.gif)
