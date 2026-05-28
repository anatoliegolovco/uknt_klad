#!/usr/bin/env bash
# disasm.sh -- disassemble every extracted .BIN into disassembly/raw/.
# Uses our self-contained PDP-11 decoder (tools/pdp11dis.py); no external deps.
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p disassembly/raw
shopt -s nullglob nocaseglob
for bin in assets/original/ex_*/*.bin; do
  name="$(basename "${bin%.*}")"
  out="disassembly/raw/${name}.asm"
  echo "disasm $bin -> $out"
  python3 tools/pdp11dis.py "$bin" > "$out"
done
echo "done."
