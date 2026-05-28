#!/usr/bin/env bash
# extract_dsk.sh -- pull individual files out of the original archives/images.
#
# The gamgal.html archive ships each КЛАД variant as a RAR holding a single
# BK/УКНЦ raw ".BIN" memory image (header: load-addr word + length word, then
# bytes). For true RT-11 ".dsk" floppy images, use nzeemin's `rt11dsk` instead
# (build from https://github.com/nzeemin/ukncbtl-utils) -- this wrapper falls
# back to it when given a .dsk.
#
# Usage:
#   tools/extract_dsk.sh assets/original/klad.rar   # -> extracts the .BIN
#   tools/extract_dsk.sh path/to/floppy.dsk         # -> lists/extracts via rt11dsk
set -euo pipefail

in="${1:?usage: extract_dsk.sh <file.rar|file.dsk>}"
outdir="$(dirname "$in")/ex_$(basename "${in%.*}")"
mkdir -p "$outdir"

case "$in" in
  *.rar|*.RAR)
    unrar x -o+ "$in" "$outdir/" ;;
  *.dsk|*.DSK)
    if command -v rt11dsk >/dev/null; then
      echo "== directory =="; rt11dsk "$in" l
      echo "== extracting all =="; rt11dsk "$in" x "*.*" ;;
    else
      echo "rt11dsk not found. Build it from nzeemin/ukncbtl-utils." >&2; exit 1
    fi ;;
  *) echo "unknown input type: $in" >&2; exit 1 ;;
esac
echo "extracted into: $outdir"
ls -l "$outdir"
