#!/usr/bin/env python3
"""
build_image_credits.py — Susun SUMBER_GAMBAR.md dari keterangan gambar di setiap BAB.

Setiap gambar di README BAB ditulis seperti ini:

    ![teks alternatif](img/nama.png)

    *Gambar 8.1 — Keterangan. Sumber: [Pembuat](URL), lisensi CC BY-SA 4.0.*

Script ini mengumpulkan semua gambar beserta keterangan sumbernya ke satu tabel.

Pemakaian:
    python tools/build_image_credits.py
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
IMG_RE = re.compile(r"!\[([^\]]*)\]\(([^)\s]+)\)")

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

HEADER = """# 🖼️ Sumber Gambar

Daftar semua gambar yang dipakai di materi ini beserta sumber dan lisensinya. Berkas ini dibuat otomatis oleh `python tools/build_image_credits.py` dari keterangan gambar di setiap BAB — jangan diedit manual.

## Gambar Kit Bluino

| Berkas | Keterangan | Sumber |
|---|---|---|
| [Produk1.jpeg](Produk1.jpeg), [Produk2.jpeg](Produk2.jpeg), [Produk3.jpeg](Produk3.jpeg), [bluino_kit.jpg](bluino_kit.jpg) | Foto kit ESP32 IoT Starter Shield | [Bluino](http://www.bluino.com) |
| [Schematic.jpg](Schematic.jpg) | Skematik ESP32 Starter Shield 3.2 | [Bluino](http://www.bluino.com) |
| [assets/kit/](assets/kit/) | Potongan skematik per komponen (dibuat dari Schematic.jpg) | [Bluino](http://www.bluino.com) |

## Gambar per BAB

| BAB | Gambar | Keterangan dan sumber |
|---|---|---|
"""


def strip_code(lines):
    inside = False
    for i, line in enumerate(lines):
        if line.strip().startswith("```"):
            inside = not inside
            continue
        if not inside:
            yield i, line


def rebase_links(caption, bab):
    """Ubah tautan relatif di keterangan agar tetap benar dari folder utama repositori."""
    def fix(m):
        target = m.group(2)
        if target.startswith(("http://", "https://", "mailto:")):
            return m.group(0)
        if target.startswith("../"):
            target = target[3:]
        elif target.startswith("#"):
            target = f"{bab}/README.md{target}"
        else:
            target = f"{bab}/{target}"
        return f"{m.group(1)}({target})"
    return re.sub(r"(\[[^\]]*\])\(([^)\s]+)\)", fix, caption)


def main():
    rows = []
    for readme in sorted(ROOT.glob("BAB-*/README.md")):
        bab = readme.parent.name
        lines = readme.read_text(encoding="utf-8").replace("\r\n", "\n").split("\n")
        for i, line in strip_code(lines):
            for m in IMG_RE.finditer(line):
                path = m.group(2)
                if path.startswith("http") or "shields.io" in path:
                    continue
                caption = ""
                for nxt in lines[i + 1:i + 4]:
                    if nxt.strip():
                        caption = nxt.strip().strip("*").strip()
                        break
                link = f"[{Path(path).name}]({bab}/{path})" if not path.startswith("../") else f"[{Path(path).name}]({path[3:]})"
                caption = rebase_links(caption, bab)
                rows.append(f"| [{bab}]({bab}/README.md) | {link} | {caption.replace('|', '/')} |")
    out = HEADER + "\n".join(rows) + "\n"
    (ROOT / "SUMBER_GAMBAR.md").write_text(out, encoding="utf-8", newline="\n")
    print(f"SUMBER_GAMBAR.md ditulis: {len(rows)} gambar")


if __name__ == "__main__":
    main()
