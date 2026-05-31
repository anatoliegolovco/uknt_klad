# archive/ — out-of-scope material

The working tree targets **one** game: КЛАД 1987 (Баранов), УКНЦ МС-0511. Everything
not about that version was moved here on 2026-05-31 to cut noise. Nothing here is
needed to build or study the target; kept only for reference/provenance.

## Contents

### `bk0010/` — the БК-0010 lineage (wrong platform)
- `binaries/` — `ex_klad*/` = `KLAD.BIN`, `KLAD2/3/4.BIN`, `klad10.bin` (BK raw images)
- `extracted/` — `bk0010/`, `crocodile/`, `klad3/` graphic/data extractions
- `disassembly/` — `crocodile_klad.asm` (BK annotated) + raw BK disassemblies
- `docs/` — BK/mixed docs: `00_overview` (old), `01_memory_map`, `02_io_registers`,
  `08_level_data`, `ROUTINES.md` (BK), `GAME_HISTORY_AND_REVIEWS.md`, `TODO.md`,
  `UKNC_BUILDS_mixed.md` (had a 1991 section; superseded by `KLAD_1987_BARANOV.md`)

BK-0010 code is ~83% identical to the УКНЦ target and was useful to bootstrap the
annotation, but it is a different machine (linear framebuffer vs УКНЦ planar video).

### `uknc_1991_crocodile/` — a different version
- `MKLAD_1991_Crocodile.GAM`, `newgames.dsk` — the 1991 Crocodile Software port.
  Same authors' lineage, different game build (18 named levels, different code).

### `build-deps/` — apt `.deb` cruft
- libX* + xdotool `.deb` files downloaded to extract dev headers/binaries without sudo.

### `misc/`
- `repo_tmp/` — stray pasted image, etc.

## Why kept (not deleted)
Personal preservation archive (see `design/decisions.md` D6/D7). If a question about
provenance or the BK lineage comes up, it's here. Day-to-day work ignores it.
