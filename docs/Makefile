# Minimal makefile for Sphinx documentation
#

# You can set these variables from the command line, and also
# from the environment for the first two.
SPHINXOPTS    ?=
SPHINXBUILD   ?= sphinx-build
SOURCEDIR     = source
BUILDDIR      = _build

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: help Makefile html-en update-po

# 英語版。先に make html で Doxygen の XML と source/api を作っておく。
# 出力は $(BUILDDIR)/html/en（公開先は /MISO_docs/en/）。
html-en:
	@echo "=== Building English docs ==="
	@$(SPHINXBUILD) -b html -D language=en "$(SOURCEDIR)" "$(BUILDDIR)/html/en" $(SPHINXOPTS) $(O)

# 原文から .pot を作り、locale/en/LC_MESSAGES/*.po を更新する。
# 変わった段落は fuzzy または未訳になる。api/（Doxygen と automodapi の出力）は訳さない。
update-po: gettext
	@rm -rf "$(BUILDDIR)/gettext/api"
	@sphinx-intl update -p "$(BUILDDIR)/gettext" -d locale -l en --line-width -1

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%: Makefile
	@echo "Doxygen preprocessing..."
	@doxygen ../docs/doxygen/Doxyfile
	@echo "=== Building C++ docs ==="
	@DOCS_TARGET=CPP $(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)/cpu" $(SPHINXOPTS) $(O)
	@echo "=== Merging CPU and GPU docs ==="
	@DOCS_TARGET=MERGE $(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
	@echo "=== Cleaning up ==="
	@rm -rf "$(BUILDDIR)/cpu" "$(BUILDDIR)/gpu"

clean:
	rm -rf _build source/api doxygen/_build
