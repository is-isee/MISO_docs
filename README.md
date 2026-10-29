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
make -C docs po-todo       # 訳し直しが要る項目を並べる
make -C docs html-en       # 英語版を docs/_build/html/en に作る
```

訳語と訳し方の決まりは `docs/locale/glossary.md` にある。訳すとき（LLM で訳すときも）はこのファイルを渡す。
**LLM で訳すときは、必ず Claude Sonnet 以下のモデル（Sonnet・Haiku）を使う。**

### 日本語を直してから PR を出すまで

1. **日本語の原文を直す**（`docs/source/` の `.md`・`.rst`）。
   コードブロックの中（コメント・出力例・図）は英語で書く。コードブロックは訳の対象にならないので、日本語版でも英語のまま出る。
2. **日本語版を確かめる**：`make -C docs html` を実行し、`docs/_build/html/` を見る。
3. **`.po` に反映する**：`make -C docs update-po` を実行する。
   直した段落の訳は fuzzy（`#, fuzzy` の印がつき、前の訳が残る）か、未訳（`msgstr ""`）になる。fuzzy の訳はビルドで使われない。
4. **訳し直す**：`make -C docs po-todo` で、訳し直しが要る項目を見る。その項目だけを訳し、`#, fuzzy` の行を消す（`#|` で始まる前の原文の行があれば、それも消す）。
   Claude Code で訳すときは、たとえば次のように頼む。

   > `make -C docs po-todo` に出る項目を、`docs/locale/glossary.md` に従って訳して。訳は Sonnet のサブエージェントで行い、msgstr だけを変えること。終わったら `make -C docs html-en` で警告が増えないことを確かめて。

   訳さずに PR を出してもよい。そのときは、その段落は英語版でも日本語のまま出る。PR の説明にそう書く。
5. **英語版を確かめる**：`make -C docs html-en` を実行し、`docs/_build/html/en/` を見る。`make -C docs po-todo` が何も出さなければ、訳し残しはない。
6. **原文と `.po` を同じ PR に入れる**。`git diff docs/locale` で、変わったのが直した段落の `msgid` と `msgstr` だけであることを確かめる。
   PR を出すと、日本語版は `https://is-isee.github.io/MISO_docs/pr-<番号>/`、英語版は `.../pr-<番号>/en/` で見られる。
   訳のレビューは、直した段落だけを見ればよい。

GitHub Pagesで公開する場合は、`.github/workflows/deploy_pages.yml`を利用する。mainブランチにpushすると、`gh-pages`ブランチに自動的にデプロイされる。正式版は<https://is-isee.github.io/MISO_docs/>（英語版は<https://is-isee.github.io/MISO_docs/en/>）、PR版は<https://is-isee.github.io/MISO_docs/pr-${pr}/>からアクセス出来る。

