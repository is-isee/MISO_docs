# 2D HD Test: Rayleigh Taylor Instability

レイリー・テイラー不安定性問題。重力場において、密度の高い流体が密度の低い流体の上にあるときに発生する不安定性です。

## Location

`demo/hd2d_rayleigh_taylor/`

## Geometry

- $-0.25 \leq x \leq 0.25$
- $-0.5 \leq y \leq 0.5$.

## Force

負の$y$方向に一定の重力を加えます。重力加速度は$g=0.1$に設定します。

$$
\frac{\partial \rho v_y}{\partial t} = [...] - \rho g
$$

## Initial Conditions

初期条件は、$y=0.0$で分離された上側と下側の状態で記述されます。上側領域（$y \geq 0$）では密度$\rho = 2$、下側領域（$y < 0$）では密度$\rho =1$です。圧力は$p =p_0 - \rho g y$とします。比熱比は$\gamma = 1.4$とします。

## Boundary Conditions

$x$方向に周期境界条件を、$y$方向に対しては全ての物理量に対して対称境界条件を設定します。
ただし、$V_y$のみ$y=\pm0.5$で反対称に設定しています。
`config.yaml`において、$x$方向の周期境界条件フラグをtrueに設定すると、対称境界条件が機能しなくなることに注意してください。

```yaml
domain:
  periodic:
    x: true  # when this flag is true, the symmetric boundary condition does not work
```  

## Results

可視化用のPythonスクリプトが用意されています。
結果のプロットは `demo/hd2d_rayleigh_taylor/figs` に保存されます。

```shell
cd demo/hd2d_rayleigh_taylor
python plot_data.py
```

![hd2d_rayleigh_taylor](../_static/images/rayleigh_taylor.gif)

## 3D Version

3次元への拡張は、設定ファイル`config.yaml`で`k_size`を1より大きく設定するだけで実現できます。

```yaml
# config.yaml
grid:
  i_size: 512
  j_size: 512
  k_size: 512 # change from 1 to 512
```
