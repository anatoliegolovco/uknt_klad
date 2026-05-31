# disassembly/

Study artifacts derived from the original КЛАД binary. **Tracked** in this
personal, non-commercial preservation repo (see `docs/design/legal.md`).

- `raw/` — linear-sweep output of `tools/pdp11dis.py` over the original blob.
  Regenerate any time with:

  ```bash
  tools/fetch_original.sh   # (re)fetch the blob into assets/original/
  tools/disasm.sh           # writes raw/*.asm here
  ```

- `annotated/` — hand-symbolised, commented routines as we understand them.
  The prose understanding they produce is also written up under
  `docs/reverse/`.

These files exist for personal understanding of how the original works. The
re-imagining in `src/` is written from our own notes and design, not copied
from the disassembly.
