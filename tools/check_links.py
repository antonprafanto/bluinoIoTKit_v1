#!/usr/bin/env python3
"""
check_links.py — Cek tautan (link) dan gambar di semua file Markdown.

Yang dicek:
  - Tautan relatif ke file/folder lain (misalnya ../BAB-08/README.md) harus ada.
  - Tautan ke judul (#anchor) harus cocok dengan judul yang benar-benar ada
    (memakai aturan pembuatan anchor ala GitHub).
  - Gambar ![...](path) harus ada filenya.
  - (opsional) --external: tautan http/https dicek apakah masih hidup.

Pemakaian:
    python tools/check_links.py                 # semua file .md
    python tools/check_links.py BAB-07 BAB-08   # hanya BAB tertentu
    python tools/check_links.py --external      # ikut cek tautan internet (lambat)
"""

import argparse
import re
import sys
import unicodedata
import urllib.parse
import urllib.request
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SKIP_DIRS = {".git", ".venv", "UAS", "node_modules", ".github"}

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

LINK_RE = re.compile(r"(!?)\[((?:[^\[\]]|\[[^\]]*\])*)\]\(\s*<?([^)\s>]+)>?(?:\s+\"[^\"]*\")?\s*\)")
HTML_IMG_RE = re.compile(r"<img\s[^>]*src=\"([^\"]+)\"", re.I)
HTML_ANCHOR_RE = re.compile(r"<a\s+(?:name|id)=\"([^\"]+)\"", re.I)
HEADING_RE = re.compile(r"^(#{1,6})\s+(.*?)\s*#*\s*$")


def md_files(selected):
    files = []
    for p in sorted(ROOT.rglob("*.md")):
        rel = p.relative_to(ROOT)
        if any(part in SKIP_DIRS for part in rel.parts):
            continue
        if selected and not any(rel.parts[0] == s for s in selected):
            continue
        files.append(p)
    return files


def strip_code(text):
    """Hapus blok kode berpagar dan kode inline agar tautan di dalam kode tidak ikut dicek."""
    out, inside = [], False
    for line in text.split("\n"):
        if line.strip().startswith("```"):
            inside = not inside
            out.append("")
            continue
        out.append("" if inside else re.sub(r"`[^`]*`", "", line))
    return "\n".join(out)


def heading_text_to_slug(text):
    text = re.sub(r"!\[[^\]]*\]\([^)]*\)", "", text)          # gambar
    text = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", text)       # tautan -> teks
    text = re.sub(r"<[^>]+>", "", text)                        # tag HTML
    text = text.replace("`", "").replace("**", "").replace("__", "")
    text = re.sub(r"(?<!\w)[*_](.+?)[*_](?!\w)", r"\1", text)
    text = text.strip().lower()
    kept = []
    for ch in text:
        cat = unicodedata.category(ch)
        if cat[0] in "LMN" or cat == "Pc" or ch in " -":
            kept.append(ch)
    return "".join(kept).replace(" ", "-")


def anchors_of(path, cache={}):
    if path in cache:
        return cache[path]
    anchors, counts, inside = set(), {}, False
    for line in path.read_text(encoding="utf-8").replace("\r\n", "\n").split("\n"):
        if line.strip().startswith("```"):
            inside = not inside
            continue
        if inside:
            continue
        m = HEADING_RE.match(line)
        if m:
            slug = heading_text_to_slug(m.group(2))
            n = counts.get(slug, 0)
            anchors.add(slug if n == 0 else f"{slug}-{n}")
            counts[slug] = n + 1
        for a in HTML_ANCHOR_RE.findall(line):
            anchors.add(a.lower())
    cache[path] = anchors
    return anchors


def check_external(urls):
    def probe(url):
        headers = {"User-Agent": "Mozilla/5.0 (link-check bluino-kurikulum)"}
        for method in ("HEAD", "GET"):
            try:
                req = urllib.request.Request(url, headers=headers, method=method)
                with urllib.request.urlopen(req, timeout=20) as r:
                    return url, r.status
            except urllib.error.HTTPError as e:
                if method == "GET" or e.code not in (403, 405, 429, 400, 501):
                    return url, e.code
            except Exception as e:  # noqa: BLE001
                if method == "GET":
                    return url, f"ERR {type(e).__name__}"
        return url, "?"
    with ThreadPoolExecutor(max_workers=12) as ex:
        return list(ex.map(probe, sorted(urls)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("babs", nargs="*")
    ap.add_argument("--external", action="store_true")
    a = ap.parse_args()
    selected = [f"BAB-{int(b):02d}" if b.isdigit() else b.strip("/\\") for b in a.babs]
    problems, external = [], {}
    for f in md_files(selected):
        raw = f.read_text(encoding="utf-8").replace("\r\n", "\n")
        text = strip_code(raw)
        rel = f.relative_to(ROOT).as_posix()
        for lineno, line in enumerate(text.split("\n"), 1):
            targets = [(m.group(1) == "!", m.group(3)) for m in LINK_RE.finditer(line)]
            targets += [(True, s) for s in HTML_IMG_RE.findall(line)]
            for is_img, target in targets:
                if target.startswith(("http://", "https://")):
                    external.setdefault(target, []).append(f"{rel}:{lineno}")
                    continue
                if target.startswith(("mailto:", "tel:")):
                    continue
                path_part, _, frag = target.partition("#")
                path_part = urllib.parse.unquote(path_part)
                dest = f if not path_part else (f.parent / path_part).resolve()
                if path_part and not dest.exists():
                    problems.append(f"{rel}:{lineno}: {'gambar' if is_img else 'tautan'} tidak ditemukan -> {target}")
                    continue
                if frag:
                    md = dest / "README.md" if dest.is_dir() else dest
                    if md.suffix.lower() == ".md" and md.exists():
                        if urllib.parse.unquote(frag).lower() not in anchors_of(md):
                            problems.append(f"{rel}:{lineno}: anchor tidak ada -> {target}")
    for p in problems:
        print("[RUSAK]", p)
    if a.external:
        print(f"\nMengecek {len(external)} tautan eksternal ...")
        for url, status in check_external(external):
            if status != 200 and not (isinstance(status, int) and 200 <= status < 400):
                print(f"[EKSTERNAL {status}] {url}  <- {', '.join(external[url][:3])}")
    print(f"\n{len(problems)} tautan/gambar internal bermasalah")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
