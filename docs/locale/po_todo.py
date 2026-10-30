"""訳し直しが要る項目（fuzzy と、日本語を含むのに訳が空の項目）を並べる。

使い方: make -C docs po-todo
1 件もなければ何も出さずに終わる。あれば一覧を出して終了コード 1 を返す。
"""

import re
import sys
from pathlib import Path

from babel.messages.pofile import read_po

JAPANESE = re.compile(r"[぀-ヿ一-鿿]")

root = Path(__file__).parent / "en" / "LC_MESSAGES"
todo = 0
for path in sorted(root.rglob("*.po")):
    with open(path, "rb") as f:
        catalog = read_po(f)
    for message in catalog:
        if not message.id:
            continue
        if message.fuzzy:
            kind = "fuzzy"
        elif not message.string and JAPANESE.search(message.id):
            kind = "未訳"
        else:
            continue
        todo += 1
        text = " ".join(message.id.split())
        print(f"{path.relative_to(root)}: [{kind}] {text[:60]}")

if todo:
    print(f"{todo} 件", file=sys.stderr)
    sys.exit(1)
