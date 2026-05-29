#!/usr/bin/env bash
# Reproducible extraction of the packed КЛАД (Crocodile) graphics.
#
# The Crocodile builds (KLAD/KLAD2/KLAD4/klad10) are packed executables: a
# two-stage self-unpacker (stage 0 @01000 -> loader @076710 -> backward LZ)
# rebuilds the real program in RAM at run time. We run the binary through the
# *real* bk-emulator CPU core (Eric Edwards' PDP-11 core + Leonid Broukhis' BK
# port) headlessly — no SDL/ROM — letting it unpack and draw, then dump RAM and
# render the screen + tiles.
#
# Usage: tools/extract_crocodile.sh [path-to-KLAD.BIN]
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${1:-$ROOT/assets/original/ex_klad/KLAD.BIN}"
WORK="$(mktemp -d)"

echo "[1/4] cloning bk-emulator (CPU core only is used)…"
git clone --depth 1 https://github.com/emestee/bk-emulator "$WORK/bk" >/dev/null 2>&1

echo "[2/4] building headless harness…"
cp "$ROOT/tools/bk_unpack_harness.c" "$WORK/bk/harness.c"
cd "$WORK/bk"
gcc -std=gnu89 -DSHIFTS_ALLOWED -DEIS_ALLOWED -w -c ea.c single.c double.c branch.c weird.c itab.c
gcc -std=gnu11 -DSHIFTS_ALLOWED -DEIS_ALLOWED -w -c harness.c
gcc harness.o ea.o single.o double.o branch.o weird.o itab.o -o harness

echo "[3/4] running unpacker (skips MONITOR EMTs, runs until it draws)…"
./harness "$BIN" 01000 20000000

echo "[4/4] rendering artifacts…"
cd "$ROOT"
python3 tools/render_crocodile.py /tmp/emu2_dump.bin assets/original/extracted/crocodile
echo "done -> assets/original/extracted/crocodile/"
