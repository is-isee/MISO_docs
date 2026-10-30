# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- import modules
import pymiso

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = "MISO"
copyright = "2025, Integrated Studies, ISEE, Nagoya University"
author = "Hideyuki Hotta, Haruhisa Iijima"
release = "0.0"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    "sphinx.ext.autodoc",  # automatic documentation from docstrings
    "sphinx.ext.napoleon",  # support for Google style docstrings
    "sphinx.ext.viewcode",  # ソースコードのリンク
    "sphinx.ext.intersphinx",  # 他のドキュメントへのリンク
    "sphinx.ext.todo",  # todo directives
    "sphinx_automodapi.automodapi",  # automatic API documentation
    "sphinx_multiversion",  # multiple version support
    "sphinx_copybutton",  # copy button for code blocks
    "breathe",  # Doxygen integration
    "sphinxcontrib.bibtex",  # bibliography management
    "myst_parser",  # Markdown support
    "sphinx.ext.mathjax",  # MathJax for math rendering
]

mathjax_path = "https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js"
mathjax3_config = {
    "tex": {
        "macros": {
            "bm": ["{\\boldsymbol{#1}}", 1],
        }
    }
}

myst_enable_extensions = [
    "dollarmath",  # $...$ や $$...$$ の数式を使えるようにする
    "amsmath",  # align環境などを有効化
]

autodoc_default_options = {
    "special-members": "__init__",
}

breathe_projects = {"CPP": "../doxygen/_build/xml"}
breathe_default_project = "CPP"

extensions.append("exhale")
exhale_args = {
    "containmentFolder": "./api/cpp",
    "rootFileName": "root.rst",
    "rootFileTitle": "MISO C++ API Reference",
    "doxygenStripFromPath": "..",
    "createTreeView": True,
}

# 原文は日本語で書き、英語版は gettext の訳（locale/en/LC_MESSAGES/*.po）で
# `-D language=en` を付けてビルドする（make html-en）。
language = "ja"
locale_dirs = ["../locale/"]
gettext_compact = False  # ページごとに .po を分ける
gettext_location = False  # .po に行番号を書かない（原文の行がずれるたびに差分が出るのを防ぐ）
gettext_additional_targets = ["raw"]  # <details><summary>…</summary> などの生の HTML も訳す

templates_path = ["_templates"]
exclude_patterns = ["education/maintainers/**"]

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

# pip install sphinx-rtd-theme
html_theme = "sphinx_rtd_theme"
html_static_path = ["_static"]
html_css_files = [
    "custom.css",
]
#

bibtex_bibfiles = ["reference.bib"]
bibtex_reference_style = "author_year"
