# 2D HD Test: Kelvin-Helmholtz Instability

ケルビン・ヘルムホルツ不安定性問題。設定した境界面を挟んで、上下で異なる速度を持つ流れが接しているときに発生する不安定性です。可視化のために、上下で密度を変えています。

## Location

`demo/hd2d_kelvin_helmholtz/`

## Geometry

- $-0.5 \leq x \leq 0.5$
- $-0.5 \leq y \leq 0.5$.

## Initial Conditions

初期条件は、$y=0.0$で分離された上側と下側の状態と一定の圧力$p=2.5$で記述されます。比熱比は$\gamma = 1.4$とします。

$$
\begin{align*}
\begin{pmatrix}
\rho_\mathrm{U} \\
v_\mathrm{U}
\end{pmatrix}
&=
\begin{pmatrix}
1.0 \\
0.5
\end{pmatrix} \\
\begin{pmatrix}
\rho_\mathrm{L} \\
v_\mathrm{L}
\end{pmatrix}
&=
\begin{pmatrix}
2.0 \\
-0.5
\end{pmatrix}
\end{align*}
$$

## Boundary Conditions

$x$方向に周期境界条件を、$y$方向に対しては全ての物理量に対して対称境界条件を設定します。
`config.yaml`において、$x$方向の周期境界条件フラグをtrueに設定すると、対称境界条件が機能しなくなることに注意してください。

```yaml
domain:
  periodic:
    x: true  # when this flag is true, the symmetric boundary condition does not work
```  

## Results

可視化用のPythonスクリプトが用意されています。
結果のプロットは `demo/hd2d_kelvin_helmholtz/figs` に保存されます。

```shell
cd demo/hd2d_kelvin_helmholtz
python plot_data.py
```

![hd2d_kelvin_helmholtz](../_static/images/kelvin_helmholtz.gif)

## 3D Version

3次元への拡張は、設定ファイル`config.yaml`で`k_size`を1より大きく設定するだけで実現できます。

```yaml
# config.yaml
grid:
  i_size: 512
  j_size: 512
  k_size: 512 # change from 1 to 512
```
