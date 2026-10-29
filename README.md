# MISO (Mhd ISee Open source code)

[![Deploy Pages](https://github.com/is-isee/MISO_docs/actions/workflows/deploy_pages.yml/badge.svg)](https://github.com/is-isee/MISO_docs/actions/workflows/deploy_pages.yml)

Documents of MISO (Mhd ISee Open source code), an open-source software for performing magnetohydrodynamics (MHD) simulations.

## ドキュメント生成

C++のAPIドキュメントをdoxygenで、pythonのAPIドキュメントをsphinxで生成する。全体のフォーマットを揃えるためにdoxygendenではxml形式で出力し、sphinxで取り込む。

sphinx + breathe + exhaleを利用

以下のパッケージをインストール

```shell
sudo apt update && sudo apt install -y doxygen
pip install sphinx sphinx-rtd-theme sphinx-automodapi sphinx-multiversion breathe exhale sphinx-copybutton sphinxcontrib-bibtex myst-parser 
```

```shell
make -C docs html
```

これで`docs/build/index.html`にHTMLが生成される。

`sphinxcontrib-bibtex` を利用して、参考文献を管理している。
参考文献は `docs/source/reference.bib` に記載して、

```rst
:cite:`sod_1978JCoPh..27....1S`
```

として引用する。

## 英語版（翻訳）

原文は日本語で書き、英語版は Sphinx の gettext で別にビルドする。訳は `docs/locale/en/LC_MESSAGES/*.po` に置き、git で管理する。
`api/`（Doxygen と automodapi の出力）は訳さない。訳がない段落は日本語のまま出る。

```shell
pip install sphinx-intl
make -C docs html          # 日本語版（Doxygen の XML と source/api もここで作る）
make -C docs update-po     # 原文の変更を .po に反映する
make -C docs html-en       # 英語版を docs/_build/html/en に作る
```

原文を直したあとに `make -C docs update-po` を実行すると、変わった段落の訳は fuzzy（`#, fuzzy` の印がつく）か未訳（`msgstr ""`）になる。
その段落だけを訳し直し、`#, fuzzy` の行を消す。fuzzy の訳はビルドで使われない。

訳語と訳し方の決まりは `docs/locale/glossary.md` にある。訳すとき（LLM で訳すときも）はこのファイルを渡す。

GitHub Pagesで公開する場合は、`.github/workflows/deploy_pages.yml`を利用する。mainブランチにpushすると、`gh-pages`ブランチに自動的にデプロイされる。正式版は<https://is-isee.github.io/MISO_docs/>（英語版は<https://is-isee.github.io/MISO_docs/en/>）、PR版は<https://is-isee.github.io/MISO_docs/pr-${pr}/>からアクセス出来る。

