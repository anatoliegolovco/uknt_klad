#!/usr/bin/env bash
# fetch_original.sh -- download the original КЛАД blob(s) into assets/original/.
#
# The blob is git-ignored and must NOT be committed. We pull from the
# pdp-11.org.ru game archive, which is reachable where r-games.net often is not.
# Each archive is a RAR containing one BK/УКНЦ raw ".BIN" memory image.
#
# Known SHA-256 (verified 2026-05-28) -- detect tampering / wrong file:
#   klad.rar    ab0466cb4ccc854e752b5e7c3157d8e011dfd5e4ddce84d6822d4d97da746f6f
#   klad2.rar   a47736646d4e08d3d0cf47a8db911ee17a40b2beaa150f3adc41c1cdd1980f8d
#   klad3.rar   26a4a1f1266ef36d7dfaef16b0ca60f05505df579b92e3ef7c7ce7f6621013e7
#   klad4.rar   23caed9df2aa8c6e5c48aaecdc1e64ba2452b68f41706e6c5881317579915c28
#   klad10.rar  930e75364596a1fec952248e2279c6cc62403b9fb7f010de122974dad948c582
set -euo pipefail
cd "$(dirname "$0")/.."
dst="assets/original"
base="https://archive.pdp-11.org.ru/tmp"
mkdir -p "$dst"

for f in klad.rar klad2.rar klad3.rar klad4.rar klad10.rar; do
  if [[ ! -f "$dst/$f" ]]; then
    echo "downloading $f ..."
    for attempt in 1 2 3 4; do
      if curl -fSL -m 60 -o "$dst/$f" "$base/$f"; then break; fi
      echo "  retry $attempt"; sleep $((2**attempt))
    done
  fi
  echo "extracting $f ..."
  out="$dst/ex_${f%.rar}"
  mkdir -p "$out"
  unrar x -o+ -inul "$dst/$f" "$out/"
done

echo; echo "== contents =="
find "$dst" -name '*.bin' -o -name '*.BIN' | sort
echo
echo "Reminder: assets/original/ is git-ignored. Do not commit these files."
