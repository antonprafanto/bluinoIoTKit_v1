#!/usr/bin/env python3
"""
sketch_tool.py — Alat bantu kode program (sketch) kurikulum ESP32 Bluino.

README.md tiap BAB adalah SATU-SATUNYA sumber kode. Setiap program lengkap
ditandai dengan komentar HTML tepat sebelum blok kodenya:

    <!-- sketch: p1_blink -->
    ```cpp
    ... kode ...
    ```

File tambahan untuk sketch multi-file (misalnya config.h):

    <!-- sketch: p3_multi_file/config.h -->
    ```cpp
    ...
    ```

Atribut opsional (dipisah spasi, setelah nama):
    fqbn=esp32:esp32:esp32:PartitionScheme=huge_app   board/opsi khusus
    nocompile                                          jangan di-compile

Perintah:
    python tools/sketch_tool.py list    [BAB-07 ...|all]
    python tools/sketch_tool.py extract [BAB-07 ...|all] [--prune]
    python tools/sketch_tool.py check   [BAB-07 ...|all]
    python tools/sketch_tool.py compile [BAB-07 ...|all] [--only p1_blink]

Variabel lingkungan (opsional):
    ARDUINO_CLI         path ke arduino-cli (default: "arduino-cli")
    ARDUINO_CLI_CONFIG  path ke file konfigurasi arduino-cli
    SKETCH_WORKDIR      folder kerja sementara untuk compile
    SKETCH_SLOTS        jumlah compile paralel maksimum (default: 3)
    SKETCH_DOCKER_IMAGE (opsional) compile di dalam container Docker, misalnya python:3.12-slim
    SKETCH_DOCKER_VOLUME volume Docker berisi arduino-cli di /ard (default: bluino-arduino)
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_FQBN = "esp32:esp32:esp32"

MARKER_RE = re.compile(
    r"^\s*<!--\s*sketch:\s*([A-Za-z0-9][A-Za-z0-9_]*)(?:/([A-Za-z0-9_.\-]+))?((?:\s+[a-z]+(?:=\S+)?)*)\s*-->\s*$"
)
FENCE_OPEN_RE = re.compile(r"^```\s*(cpp|c\+\+|c|arduino|ino|h)?\s*$")

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")


class Sketch:
    def __init__(self, bab, name):
        self.bab = bab
        self.name = name
        self.files = {}  # nama file -> isi
        self.lines = {}  # nama file -> nomor baris marker di README
        self.attrs = {}

    @property
    def folder(self):
        return ROOT / self.bab / self.name

    @property
    def fqbn(self):
        return self.attrs.get("fqbn", DEFAULT_FQBN)

    @property
    def nocompile(self):
        return "nocompile" in self.attrs


def chapter_dirs(args_babs):
    all_babs = sorted(p.name for p in ROOT.glob("BAB-*") if (p / "README.md").is_file())
    if not args_babs or args_babs == ["all"]:
        return all_babs
    out = []
    for b in args_babs:
        b = b.strip("/\\")
        if re.fullmatch(r"\d+", b):
            b = f"BAB-{int(b):02d}"
        if b not in all_babs:
            sys.exit(f"[ERROR] {b} tidak ditemukan")
        out.append(b)
    return out


def parse_readme(bab):
    """Kembalikan (dict nama->Sketch, list error)."""
    path = ROOT / bab / "README.md"
    lines = path.read_text(encoding="utf-8").replace("\r\n", "\n").split("\n")
    sketches, errors = {}, []
    i = 0
    while i < len(lines):
        m = MARKER_RE.match(lines[i])
        if not m:
            i += 1
            continue
        name, extra, attr_str = m.group(1), m.group(2), m.group(3) or ""
        marker_line = i + 1
        j = i + 1
        while j < len(lines) and lines[j].strip() == "":
            j += 1
        if j >= len(lines) or not FENCE_OPEN_RE.match(lines[j].strip()):
            errors.append(f"{bab}/README.md:{marker_line}: marker '{name}' tidak diikuti blok kode ```cpp")
            i += 1
            continue
        k = j + 1
        body = []
        while k < len(lines) and lines[k].strip() != "```":
            body.append(lines[k])
            k += 1
        if k >= len(lines):
            errors.append(f"{bab}/README.md:{marker_line}: blok kode '{name}' tidak ditutup")
            break
        sk = sketches.setdefault(name, Sketch(bab, name))
        fname = extra or f"{name}.ino"
        if fname in sk.files:
            errors.append(f"{bab}/README.md:{marker_line}: file '{name}/{fname}' didefinisikan dua kali")
        sk.files[fname] = "\n".join(body).rstrip() + "\n"
        sk.lines[fname] = marker_line
        for token in attr_str.split():
            if "=" in token:
                key, val = token.split("=", 1)
                sk.attrs[key] = val
            else:
                sk.attrs[token] = True
        i = k + 1
    for sk in sketches.values():
        if f"{sk.name}.ino" not in sk.files:
            errors.append(f"{bab}/README.md: sketch '{sk.name}' tidak punya file utama {sk.name}.ino")
    return sketches, errors


def existing_sketch_folders(bab):
    return sorted(p.parent.name for p in (ROOT / bab).glob("*/*.ino") if p.stem == p.parent.name)


def cmd_list(babs):
    total = 0
    for bab in babs:
        sketches, errors = parse_readme(bab)
        for e in errors:
            print("[ERROR]", e)
        for sk in sketches.values():
            total += 1
            extra = [f for f in sk.files if f != f"{sk.name}.ino"]
            info = f" (+{', '.join(extra)})" if extra else ""
            flags = []
            if sk.fqbn != DEFAULT_FQBN:
                flags.append(f"fqbn={sk.fqbn}")
            if sk.nocompile:
                flags.append("nocompile")
            print(f"{bab}/{sk.name}{info}  baris {sk.lines[f'{sk.name}.ino']}  {' '.join(flags)}")
    print(f"Total: {total} sketch")


def cmd_extract(babs, prune):
    status = 0
    for bab in babs:
        sketches, errors = parse_readme(bab)
        for e in errors:
            print("[ERROR]", e)
            status = 1
        for sk in sketches.values():
            sk.folder.mkdir(parents=True, exist_ok=True)
            for fname, content in sk.files.items():
                target = sk.folder / fname
                old = target.read_text(encoding="utf-8") if target.exists() else None
                if old != content:
                    target.write_text(content, encoding="utf-8", newline="\n")
                    print(f"[TULIS] {bab}/{sk.name}/{fname}")
        stale = [n for n in existing_sketch_folders(bab) if n not in sketches]
        for n in stale:
            if prune:
                shutil.rmtree(ROOT / bab / n)
                print(f"[HAPUS] {bab}/{n} (tidak ada di README)")
            else:
                print(f"[PERINGATAN] {bab}/{n} tidak punya marker di README (pakai --prune untuk menghapus)")
    return status


def cmd_check(babs):
    problems = 0
    for bab in babs:
        sketches, errors = parse_readme(bab)
        for e in errors:
            print("[ERROR]", e)
            problems += 1
        for sk in sketches.values():
            for fname, content in sk.files.items():
                target = sk.folder / fname
                if not target.exists():
                    print(f"[BEDA] {bab}/{sk.name}/{fname} belum ada (jalankan: extract)")
                    problems += 1
                elif target.read_text(encoding="utf-8").replace("\r\n", "\n") != content:
                    print(f"[BEDA] {bab}/{sk.name}/{fname} tidak sama dengan README")
                    problems += 1
        for n in existing_sketch_folders(bab):
            if n not in sketches:
                print(f"[BEDA] {bab}/{n} ada di folder tetapi tidak ada marker di README")
                problems += 1
    print("OK — semua sketch sinkron" if problems == 0 else f"{problems} masalah ditemukan")
    return 1 if problems else 0


def acquire_slot(workdir):
    slots = int(os.environ.get("SKETCH_SLOTS", "3"))
    lockdir = workdir / "locks"
    lockdir.mkdir(parents=True, exist_ok=True)
    while True:
        for n in range(slots):
            slot = lockdir / f"slot{n}"
            try:
                slot.mkdir()
                return slot
            except FileExistsError:
                try:
                    if time.time() - slot.stat().st_mtime > 1800:
                        shutil.rmtree(slot, ignore_errors=True)
                except OSError:
                    pass
        time.sleep(2)


def compile_one(sk, workdir, cli, config):
    src = workdir / "src" / sk.bab / sk.name
    if src.exists():
        shutil.rmtree(src)
    src.mkdir(parents=True)
    for fname, content in sk.files.items():
        (src / fname).write_text(content, encoding="utf-8", newline="\n")
    docker_image = os.environ.get("SKETCH_DOCKER_IMAGE")
    if docker_image:
        # Compile di dalam container Linux (jauh lebih cepat daripada Windows).
        # Volume berisi arduino-cli + core esp32 + library di /ard.
        volume = os.environ.get("SKETCH_DOCKER_VOLUME", "bluino-arduino")
        cmd = ["docker", "run", "--rm", "-v", f"{volume}:/ard",
               "-v", f"{src.as_posix()}:/sk/{sk.name}:ro", docker_image,
               "/ard/bin/arduino-cli", "--config-file", "/ard/arduino-cli.yaml", "compile",
               "--fqbn", sk.fqbn, "--build-path", f"/ard/build/{sk.bab}/{sk.name}",
               "--warnings", "default", f"/sk/{sk.name}"]
    else:
        build = workdir / "build" / sk.bab / sk.name
        cmd = [cli]
        if config:
            cmd += ["--config-file", config]
        cmd += ["compile", "--fqbn", sk.fqbn, "--build-path", str(build), "--warnings", "default", str(src)]
    slot = acquire_slot(workdir)
    try:
        t0 = time.time()
        proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
        dt = time.time() - t0
    finally:
        shutil.rmtree(slot, ignore_errors=True)
    out = (proc.stdout or "") + (proc.stderr or "")
    return proc.returncode == 0, out, dt


def cmd_compile(babs, only):
    cli = os.environ.get("ARDUINO_CLI", "arduino-cli")
    config = os.environ.get("ARDUINO_CLI_CONFIG", "")
    workdir = Path(os.environ.get("SKETCH_WORKDIR", Path(tempfile.gettempdir()) / "bluino-sketch-work"))
    workdir.mkdir(parents=True, exist_ok=True)
    results = []
    for bab in babs:
        sketches, errors = parse_readme(bab)
        for e in errors:
            print("[ERROR]", e)
            results.append((f"{bab}/(marker)", False))
        for sk in sketches.values():
            if only and sk.name not in only:
                continue
            label = f"{bab}/{sk.name}"
            if sk.nocompile:
                print(f"[LEWATI] {label} (nocompile)")
                continue
            ok, out, dt = compile_one(sk, workdir, cli, config)
            results.append((label, ok))
            if ok:
                size = re.search(r"Sketch uses (\d+) bytes \((\d+)%\)", out)
                info = f"{size.group(2)}% flash" if size else ""
                print(f"[LULUS] {label}  ({dt:.0f} dtk) {info}")
                warns = [l for l in out.splitlines() if "warning:" in l and str(Path(sk.name)) in l]
                for w in warns[:10]:
                    print("    peringatan:", w.strip()[:300])
            else:
                print(f"[GAGAL] {label}  ({dt:.0f} dtk)")
                keep = [l for l in out.splitlines() if ("error" in l.lower() or "note:" in l) and "Error during build" not in l]
                for l in (keep or out.splitlines()[-25:])[:40]:
                    print("    ", l.rstrip()[:400])
    n_ok = sum(1 for _, ok in results if ok)
    print(f"\nRingkasan: {n_ok}/{len(results)} lulus")
    return 0 if n_ok == len(results) else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("command", choices=["list", "extract", "check", "compile"])
    ap.add_argument("babs", nargs="*", help="BAB-07 BAB-08 ... atau 'all' (default: all)")
    ap.add_argument("--prune", action="store_true", help="extract: hapus folder sketch yang tidak ada di README")
    ap.add_argument("--only", nargs="*", help="compile: hanya sketch dengan nama ini")
    a = ap.parse_args()
    babs = chapter_dirs(a.babs)
    if a.command == "list":
        cmd_list(babs)
        return 0
    if a.command == "extract":
        return cmd_extract(babs, a.prune)
    if a.command == "check":
        return cmd_check(babs)
    return cmd_compile(babs, a.only)


if __name__ == "__main__":
    sys.exit(main())
