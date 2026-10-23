#!/usr/bin/env python3
"""
check_style.py — Cek format BAB dan ejaan (EYD) sesuai CONTRIBUTING.md.

Pemakaian:
    python tools/check_style.py                 # semua BAB
    python tools/check_style.py BAB-07 BAB-08   # BAB tertentu
    python tools/check_style.py --ringkas       # hanya jumlah temuan per BAB

Kategori temuan:
    FORMAT  struktur BAB tidak sesuai template
    EYD     kata tidak baku / penulisan tidak sesuai EYD
    SATUAN  penulisan angka desimal atau satuan di teks narasi
    GAMBAR  gambar tanpa keterangan/sumber
    KODE    program lengkap tanpa penanda sketch
"""

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

REQUIRED_HEADINGS = [
    "## 📌 Tujuan Pembelajaran",
    "## 🧰 Yang Perlu Disiapkan",
    "## 📝 Rangkuman",
    "## 💪 Latihan",
    "## 📚 Referensi",
]

NON_BAKU = {
    "silahkan": "silakan", "praktek": "praktik", "analisa": "analisis", "menganalisa": "menganalisis",
    "resiko": "risiko", "merubah": "mengubah", "dirubah": "diubah", "ijin": "izin",
    "aktifitas": "aktivitas", "efektifitas": "efektivitas", "sensitifitas": "sensitivitas",
    "produktifitas": "produktivitas", "kreatifitas": "kreativitas", "standart": "standar",
    "tehnik": "teknik", "obyek": "objek", "subyek": "subjek", "sistim": "sistem",
    "frekwensi": "frekuensi", "kwalitas": "kualitas", "sekedar": "sekadar", "antri": "antre",
    "jaman": "zaman", "nasehat": "nasihat", "mempengaruhi": "memengaruhi", "dipengaruhi": "dipengaruhi (baku)",
    "mengkonfigurasi": "mengonfigurasi", "dikonfigurasi": "dikonfigurasi (baku)",
    "mengkompilasi": "mengompilasi", "mengkoneksikan": "menghubungkan", "mengkonversi": "mengonversi",
    "mengkalibrasi": "mengalibrasi", "mengkontrol": "mengontrol", "mengkombinasikan": "mengombinasikan",
    "mengkomunikasikan": "mengomunikasikan", "menginstall": "menginstal", "terinstall": "terinstal",
    "diinstall": "diinstal", "install": "instal", "saklar": "sakelar", "konkrit": "konkret",
    "ekstrim": "ekstrem", "metoda": "metode", "nomer": "nomor", "kadaluarsa": "kedaluwarsa",
    "kadaluwarsa": "kedaluwarsa", "ketrampilan": "keterampilan", "trampil": "terampil",
    "faham": "paham", "hirarki": "hierarki", "jadual": "jadwal", "komplit": "lengkap/komplet",
    "telpon": "telepon", "restricsi": "restriksi", "kelembaban": "kelembapan", "lembab": "lembap", "apapun": "apa pun", "siapapun": "siapa pun",
    "kapanpun": "kapan pun", "dimanapun": "di mana pun", "manapun": "mana pun", "satupun": "satu pun",
    "sedikitpun": "sedikit pun", "itupun": "itu pun", "inipun": "ini pun",
    "didalam": "di dalam", "diatas": "di atas", "dibawah": "di bawah", "disini": "di sini",
    "disana": "di sana", "dimana": "di mana", "diluar": "di luar", "disamping": "di samping",
    "disebelah": "di sebelah", "dibelakang": "di belakang", "didepan": "di depan",
    "diantara": "di antara", "diakhir": "di akhir", "diawal": "di awal", "ditengah": "di tengah",
    "dikanan": "di kanan", "dikiri": "di kiri", "kedalam": "ke dalam", "kemana": "ke mana",
    "darimana": "dari mana", "diupload": "diunggah / di-upload", "mengupload": "mengunggah",
    "didownload": "diunduh", "mendownload": "mengunduh", "dicompile": "dikompilasi / di-compile",
    "direset": "di-reset / diatur ulang", "disetting": "diatur", "mensetting": "mengatur",
    "diupdate": "diperbarui", "mengupdate": "memperbarui", "dicopy": "disalin", "mengcopy": "menyalin",
    "discan": "dipindai / di-scan", "diprint": "dicetak", "diflash": "di-flash",
    "dipublish": "dipublikasikan / di-publish", "disubscribe": "di-subscribe",
}
WORD_RE = re.compile(r"[A-Za-zÀ-ÿ]+")
DECIMAL_UNIT_RE = re.compile(
    r"(?<![\w.])\d+\.\d+\s?(?:V|mV|mA|µA|uA|A|Hz|kHz|MHz|GHz|kΩ|MΩ|Ω|ohm|°C|ms|µs|us|mm|cm|dBm|dB|mW|W|µF|uF|nF|pF|mAh|KB|MB|GB|Mbps|kbps)\b(?!\.\d)"
)
NOSPACE_UNIT_RE = re.compile(r"(?<![\w.,])\d+(?:,\d+)?(?:mV|mA|µA|kHz|MHz|GHz|kΩ|MΩ|Ω|ms|µs|°C|mAh|dBm)\b")
ANDA_RE = re.compile(r"\bAnda\b")
IMAGE_RE = re.compile(r"!\[[^\]]*\]\([^)]+\)|<img\s")
MARKER_RE = re.compile(r"^\s*<!--\s*sketch:")


def strip_inline(line):
    line = re.sub(r"`[^`]*`", " ", line)
    line = re.sub(r"\*\*[A-Z][^*]{0,40}\*\*", " ", line)  # label menu/tombol, mis. **Install**
    line = re.sub(r"\]\([^)]*\)", "] ", line)          # URL tautan
    line = re.sub(r"<[^>]+>", " ", line)               # tag HTML
    line = re.sub(r"https?://\S+", " ", line)
    return line


def check_file(bab, path):
    issues = []
    lines = path.read_text(encoding="utf-8").replace("\r\n", "\n").split("\n")
    num = int(bab.split("-")[1])

    # ---------- FORMAT ----------
    if not re.match(rf"^# BAB {num}: \S", lines[0]):
        issues.append(("FORMAT", 1, f"Judul harus berformat '# BAB {num}: Judul' (sekarang: {lines[0][:60]!r})"))
    if not any(l.startswith("> ✅ **Prasyarat:**") for l in lines[:15]):
        issues.append(("FORMAT", 1, "Baris '> ✅ **Prasyarat:** ...' belum ada di awal BAB"))
    headings = [l.strip() for l in lines if l.startswith("## ")]
    for h in REQUIRED_HEADINGS:
        if h not in headings:
            issues.append(("FORMAT", 1, f"Judul wajib belum ada: {h}"))
    if len(lines) > 300 and "## 📑 Daftar Isi" not in headings:
        issues.append(("FORMAT", 1, "BAB panjang (> 300 baris) wajib punya '## 📑 Daftar Isi'"))

    # ---------- isi per baris ----------
    inside, fence_start, prev_marker = False, 0, False
    block = []
    for i, raw in enumerate(lines, 1):
        s = raw.strip()
        if s.startswith("```"):
            if not inside:
                inside, fence_start, block = True, i, []
                marker_before = any(MARKER_RE.match(lines[j]) for j in range(max(0, i - 3), i - 1))
            else:
                inside = False
                code = "\n".join(block)
                if "void setup" in code and "void loop" in code and not marker_before:
                    issues.append(("KODE", fence_start, "Program lengkap tanpa penanda <!-- sketch: ... -->"))
            continue
        if inside:
            block.append(raw)
            continue
        text = strip_inline(raw)
        if s.startswith("|") or True:
            for w in WORD_RE.findall(text):
                lw = w.lower()
                if lw in NON_BAKU and not NON_BAKU[lw].endswith("(baku)"):
                    issues.append(("EYD", i, f"'{w}' → {NON_BAKU[lw]}"))
            if ANDA_RE.search(text):
                issues.append(("EYD", i, "Pakai 'kamu', bukan 'Anda'"))
            for m in DECIMAL_UNIT_RE.finditer(text):
                issues.append(("SATUAN", i, f"'{m.group(0)}' → pakai koma desimal di teks narasi"))
            for m in NOSPACE_UNIT_RE.finditer(text):
                issues.append(("SATUAN", i, f"'{m.group(0)}' → beri spasi sebelum satuan"))
        if IMAGE_RE.search(raw) and "shields.io" not in raw:
            nxt = [l.strip() for l in lines[i:i + 3] if l.strip()]
            if not nxt or not (nxt[0].startswith("*Gambar") and "Sumber:" in nxt[0]):
                issues.append(("GAMBAR", i, "Gambar wajib diikuti keterangan '*Gambar N.x — ... Sumber: ...*'"))
    return issues


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("babs", nargs="*")
    ap.add_argument("--ringkas", action="store_true")
    a = ap.parse_args()
    babs = sorted(p.name for p in ROOT.glob("BAB-*") if (p / "README.md").is_file())
    if a.babs:
        want = {f"BAB-{int(b):02d}" if b.isdigit() else b.strip("/\\") for b in a.babs}
        babs = [b for b in babs if b in want]
    total = 0
    for bab in babs:
        issues = check_file(bab, ROOT / bab / "README.md")
        total += len(issues)
        if a.ringkas:
            cats = {}
            for c, _, _ in issues:
                cats[c] = cats.get(c, 0) + 1
            print(f"{bab}: {len(issues):4d}  " + "  ".join(f"{k}={v}" for k, v in sorted(cats.items())))
        else:
            for cat, ln, msg in issues:
                print(f"{bab}/README.md:{ln}: [{cat}] {msg}")
    print(f"\nTotal temuan: {total}")
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
