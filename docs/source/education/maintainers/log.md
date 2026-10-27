# 作業ログ

## 2026-10-27

- MISO_docs #25 に着手。R2D2_education の構成（4 部・11 章、参考資料、実習）に合わせて骨組み、執筆ルール（writing_rules.md）、執筆計画（plan.md）を作成。
- 全 11 章・参考資料 5 本・実習 2 題（execution, heat1d）を執筆。章ごとに分担して並行で書き、本文の例と解答例は g++/OpenMPI（CPU）と CUDA 12.9（RTX 4090）で実際にビルド・実行して確かめた。
- 用語集（約 150 語）を各章の初出から作成。
- 別のモデルで 2 本の独立レビュー（第 1〜6 章＋参考資料、第 7〜11 章＋実習）を行い、計 11 件を修正（コード例の改行、shared_ptr の使用箇所、練習の出力値、章の前提関係など）。
- Sphinx（MyST）で education の警告 0 件を確認（C++ API の生成を外した写しの conf でビルド）。
- 執筆中に見つけた MISO 本体の問題: #202（reduce の初期値）、#203（inline の欠落）、#204（重力の仕事）を起票。#189（util::max2 の float 丸め）は既存。教材では util::fmax を使い、reduce の初期値は単位元にすると書いた。#189・#202 が直ったら第 7・9 章と実習を見直す。
- 未確認: Mac（Homebrew）での手順、Open MPI 5 での openmpi_config.env、MISO_docs の本番 conf（Doxygen・exhale 込み）でのビルド。
