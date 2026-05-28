# disassembly/

Local study artifacts. **Not committed** (see `.gitignore`).

- `raw/` — linear-sweep output of `tools/pdp11dis.py` over the original blob.
  A full disassembly is a mechanical transform of a copyrighted binary, so we
  keep it local only. Regenerate any time with:

  ```bash
  tools/fetch_original.sh   # pull the blob into assets/original/ (git-ignored)
  tools/disasm.sh           # writes raw/*.asm here
  ```

- `annotated/` — hand-symbolised, commented routines as we understand them.
  The `.asm` files here are also git-ignored; the understanding they produce is
  written up *in our own words* under `docs/reverse/` (which IS committed).

## Clean-room note

Whoever writes the reimplementation in `src/` should work from the prose specs
in `docs/reverse/`, **not** from these disassembly files — that separation is
what keeps the reimplementation clean-room.
