#!/usr/bin/env python3
"""
normalize_chapters.py — Seragamkan footer setiap BAB.

Footer standar = garis navigasi (⬅️ BAB sebelumnya · 🏠 Daftar Isi · BAB berikutnya ➡️)
diikuti bagian "☕ Dukung Proyek Ini". Footer lama (dalam format apa pun) dihapus
lalu diganti footer standar. Judul BAB untuk navigasi diambil dari baris pertama
tiap README ("# BAB N: Judul").

Pemakaian:
    python tools/normalize_chapters.py           # terapkan ke semua BAB
    python tools/normalize_chapters.py --cek     # hanya laporkan BAB yang footernya belum standar
"""

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

SUPPORT_BLOCK = """## ☕ Dukung Proyek Ini

Bab ini disusun dengan dedikasi waktu dan keahlian yang tidak sedikit. Kalau materi ini bermanfaat untuk proses belajar, riset, atau proyekmu, bentuk apresiasi terbaik yang bisa kamu berikan adalah mentraktir penulis secangkir kopi:

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
Punya kritik, saran, atau butuh pendampingan khusus ESP32 & IoT? Silakan hubungi:
- ✉️ **Email:** [antonprafanto@unmul.ac.id](mailto:antonprafanto@unmul.ac.id)
- 📞 **WhatsApp:** [0811-553-393](https://wa.me/62811553393) / [0852-5069-0673](https://wa.me/6285250690673)
"""

NAV_HINT_RE = re.compile(
    r"(⬅️|➡️|Kembali ke Daftar Isi|Daftar Isi\]\(\.\./README\.md\)|^\*?(Sebelumnya|Selanjutnya)\s*→)"
)
TITLE_RE = re.compile(r"^#\s+BAB\s+(\d+):\s*(.+?)\s*$")


def chapter_titles():
    titles = {}
    for p in sorted(ROOT.glob("BAB-*/README.md")):
        first = p.read_text(encoding="utf-8").split("\n", 1)[0]
        m = TITLE_RE.match(first)
        if m:
            titles[int(m.group(1))] = m.group(2)
        else:
            print(f"[PERINGATAN] Judul {p.parent.name} belum berformat '# BAB N: Judul': {first[:70]!r}")
            titles[int(p.parent.name.split('-')[1])] = re.sub(r"^#\s*", "", first).strip()
    return titles


def nav_line(n, titles):
    parts = []
    if n - 1 in titles:
        parts.append(f"[⬅️ BAB {n - 1}: {titles[n - 1]}](../BAB-{n - 1:02d}/README.md)")
    parts.append("[🏠 Daftar Isi](../README.md)")
    if n + 1 in titles:
        parts.append(f"[BAB {n + 1}: {titles[n + 1]} ➡️](../BAB-{n + 1:02d}/README.md)")
    else:
        parts.append(f"BAB {n + 1} segera hadir 🚧")
    return " · ".join(parts)


def footer_start(lines):
    """Cari indeks baris awal footer lama (navigasi atau blok dukungan) di bagian akhir file."""
    start = None
    limit = max(0, len(lines) - 60)
    inside = False
    fence_state = []
    for i, l in enumerate(lines):
        if l.strip().startswith("```"):
            inside = not inside
        fence_state.append(inside)
    for i in range(len(lines) - 1, limit - 1, -1):
        l = lines[i]
        if fence_state[i]:
            continue
        if l.startswith("## ☕ Dukung Proyek Ini") or ("../BAB-" in l and NAV_HINT_RE.search(l)) \
                or l.strip() in ("**💬 Konsultasi & Kolaborasi:**",) or "trakteer.id" in l:
            start = i
    if start is None:
        return None
    # mundur melewati baris kosong dan pemisah '---' tepat sebelum footer
    while start > 0 and lines[start - 1].strip() in ("", "---"):
        start -= 1
    return start


def build(n, text, titles):
    lines = text.replace("\r\n", "\n").rstrip("\n").split("\n")
    start = footer_start(lines)
    body = lines if start is None else lines[:start]
    while body and body[-1].strip() in ("", "---"):
        body.pop()
    footer = ["", "---", "", nav_line(n, titles), "", "---", "", SUPPORT_BLOCK.rstrip("\n")]
    return "\n".join(body + footer) + "\n"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cek", action="store_true")
    a = ap.parse_args()
    titles = chapter_titles()
    changed = 0
    for p in sorted(ROOT.glob("BAB-*/README.md")):
        n = int(p.parent.name.split("-")[1])
        old = p.read_text(encoding="utf-8")
        new = build(n, old, titles)
        if new != old.replace("\r\n", "\n"):
            changed += 1
            if a.cek:
                print(f"[BELUM STANDAR] {p.parent.name}")
            else:
                p.write_text(new, encoding="utf-8", newline="\n")
                print(f"[DIPERBARUI] {p.parent.name}")
    print(f"{changed} BAB {'perlu diseragamkan' if a.cek else 'diperbarui'}")
    return 1 if (a.cek and changed) else 0


if __name__ == "__main__":
    sys.exit(main())
