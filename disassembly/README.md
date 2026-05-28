# disassembly/

Study artifacts derived from the original КЛАД binary. **Tracked** in this
personal, non-commercial preservation repo (see `design/legal.md`).

- `raw/` — linear-sweep output of `tools/pdp11dis.py` over the original blob.
  Regenerate any time with:

  ```bash
  tools/fetch_original.sh   # (re)fetch the blob into assets/original/
  tools/disasm.sh           # writes raw/*.asm here
  ```

- `annotated/` — hand-symbolised, commented routines as we understand them.
  The prose understanding they produce is also written up under
  `docs/reverse/`.

## Note

If this project ever became commercial or widely distributed, treat a full
disassembly as a derivative of a copyrighted work and reconsider distributing
it (and keep the reimplementation in `src/` clean-room — written from specs in
`docs/reverse/`, not from these files). For personal study that constraint does
not apply.
