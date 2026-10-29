# 英訳の用語集と決まり

`locale/en/LC_MESSAGES/*.po` を訳すときに使う。訳す人（人でも LLM でも）にこのファイルを渡す。

**LLM で訳すときは、必ず Claude Sonnet か、それより安いモデル（Haiku）を使う。** Opus などの上位モデルでは訳さない。
Claude Code の上位モデルで作業しているときも、訳はサブエージェント（model に sonnet 以下を指定）に任せる。
上位モデルがしてよいのは、`.po` の準備・ビルド・訳の点検まで。

## 訳語の出典

1. **`source/education/reference/glossary.md` の「英語」の列**（C++・並列計算・数値計算の語、約 150 語）。
   その表にある語は、表の英語で訳す。表に語を足すときは、表と英訳を同じ PR で直す。
2. **下の「追加の語」**（デモ・物理・文書の語）。
3. どちらにもない語は、分野で普通に使う語で訳し、よく出る語ならこのファイルに足す。

## 変えないもの

- インラインコード（`` `...` ``、rst の ``` ``...`` ```）、数式（`$...$`、`$$...$$`、`:math:`）、URL。
- `{ref}`、`{doc}`、`:ref:`、`:cite:` などのロールの**ターゲット**（`<edu-ch02>` の部分）。表示文は訳す。
- `|today|` などの置換、`**` や `*` の強調の記号（中の語は訳す）。
- 英語のままの見出し（`Quick Start`、`Boundary Condition` など）は `msgstr` を空のままにする。空なら原文が出る。

## 書き方

- アメリカ英語（color、behavior、normalization）。
- 本文は平叙文で、読者には "you"、著者側は "we" を使う。手順は命令形（"Run the following command."）。
- 「以下のように」「次のように」で終わる文は、"as follows." または末尾のコロンで受ける。
- 日本語の「〜してください」は命令形にする。"please" は付けない。
- 単位・数字・記号のあいだの空白は原文どおり。
- 固有名詞は原論文の綴り（Orszag-Tang、Kelvin-Helmholtz、Rayleigh-Taylor、Petschek、Sod、Alfvén）。

## 追加の語

| 日本語 | 英語 | 注 |
|---|---|---|
| 磁気流体力学（MHD） | magnetohydrodynamics (MHD) | 形容詞は magnetohydrodynamic |
| 流体力学（HD） | hydrodynamics (HD) | |
| 輻射輸送（RT） | radiative transfer (RT) | |
| テスト問題・デモ問題 | test problem / demo problem | |
| 衝撃波管 | shock tube | Sod's shock tube |
| Orszag-Tang 渦 | Orszag-Tang vortex | |
| ケルビン・ヘルムホルツ不安定 | Kelvin-Helmholtz instability | |
| レイリー・テイラー不安定 | Rayleigh-Taylor instability | |
| 磁気リコネクション | magnetic reconnection | |
| 地球磁気圏 | Earth's magnetosphere | |
| 太陽風 | solar wind | |
| 双極子磁場 | dipole magnetic field | |
| 基礎方程式 | basic equations | |
| 運動方程式 | equation of motion | |
| 誘導方程式 | induction equation | |
| 輻射輸送方程式 | radiative transfer equation | |
| 境界条件 | boundary condition | |
| 周期境界条件 | periodic boundary condition | |
| 対称境界条件・反対称境界条件 | symmetric / antisymmetric boundary condition | |
| 固定境界条件 | fixed boundary condition | |
| アウトフロー境界 | outflow boundary | |
| 初期条件 | initial condition | |
| 格子・計算格子 | grid / computational grid | |
| 格子点（数） | (number of) grid points | |
| 格子幅 | grid spacing | |
| 物理量 | physical quantity / variable | 変数の並びを指すときは variable |
| 密度・圧力・速度 | density / pressure / velocity | |
| 内部エネルギー・運動エネルギー・磁気エネルギー・全エネルギー | internal / kinetic / magnetic / total energy | |
| 比熱比 | ratio of specific heats | 記号だけなら adiabatic index $\gamma$ も可 |
| 重力加速度 | gravitational acceleration | |
| 電気抵抗 | (electrical) resistivity | 係数は resistivity、現象は resistance |
| 特性速度 | characteristic speed | |
| 輻射強度・平均強度・入射強度 | intensity / mean intensity / incident intensity | |
| 源泉関数 | source function | |
| 吸収係数 | absorption coefficient | |
| 規格化 | normalization | |
| 設定ファイル | configuration file | |
| 実行ファイル | executable | |
| 可視化用のスクリプト | visualization script | |
| 計算を再開する | restart (a run) | |
| 名古屋大学宇宙地球環境研究所（ISEE） | Institute for Space-Earth Environmental Research (ISEE), Nagoya University | |
| 総合解析研究部 | Division of Integrated Studies | |
