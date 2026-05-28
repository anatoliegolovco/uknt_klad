# Work log

Chronological record of work performed. Newest at the bottom.

## Session 1 — 2026-05-28

### Recon & acquisition
- Probed sources: `r-games.net` does not resolve from the environment;
  `archive.pdp-11.org.ru`, `uknc.pdp-11.ru`, GitHub all reachable.
- Downloaded 5 variants (`klad`, `klad2`, `klad3`, `klad4`, `klad10`) from the
  pdp-11.org.ru archive. Each is a RAR holding one raw BK `.BIN` memory image
  (header `load+len`; all validate `len+4 == filesize`). SHA-256 recorded in
  `tools/fetch_original.sh`.
- Identified format as BK raw `.BIN`, not RT-11 `.dsk`.

### Platform identification
- Operand-level fingerprint (deliberate `#imm`/`@#abs` constants only) shows
  BK-0010 signatures: `#040000` framebuffer, `#0177716/177662/177664` registers.
  Concluded **BK-0010 build** (decision D3).
- `KLAD3.BIN` is the cleanest: loads at `01000`, entry `JMP @#4000`.

### Tooling
- Wrote `tools/pdp11dis.py` (PDP-11 disassembler) — validated against 14 known
  encodings (MOV/RTS/HALT/CLR/JSR/SOB/branches/double-operand, PC-relative).
- Wrote `tools/fetch_original.sh`, `tools/extract_dsk.sh`, `tools/disasm.sh`.
- Disassembled all 5 binaries into `disassembly/raw/` (git-ignored).

### Repository & docs
- Scaffolded repo: `README`, MIT `LICENSE` (+ originality note), `.gitignore`
  (blob, disassembly, extracted data, build artifacts).
- Wrote `docs/uknc-architecture.md`, `docs/reverse/00_overview.md`,
  `01_memory_map.md`, `02_io_registers.md`, `TODO.md`, `docs/design.md`.

### Reimplementation scaffold
- C23 + raylib, native + WebAssembly (`src/main.c`, `game.h`, `game.c`,
  `Makefile`, `web/shell.html`, `README`). Emscripten main-loop split, fixed
  256×192 target, keyboard + touch. Vertical slice: maze, gravity, ladders,
  gold pickup, water-death. Syntax-verified under `-std=c2x`.

### Level-data investigation
- Found the 20-entry / 352-byte table at `010406` (stride `0540`), corroborated
  by `ADD #540,@#1300` in init. Built `tools/extract_data.py` (dumps + 1-bpp
  PNG, dependency-free PNG writer). Rendered at widths 256/176/128/88/64 — does
  not resolve into 20 maze levels; likely a sprite/font bank or encoded data.
  Documented in `docs/reverse/08_level_data.md`. Output is git-ignored.

### Localisation
- `src/i18n.h`: Romanian-default UI strings (EN fallback); HUD via `T()`.

### Project documentation
- Created `design/` (this folder): decisions, limitations, worklog, roadmap,
  legal.

### Notable decision this session
- A typo ("u planific" vs "**nu** planific sa fac bani") briefly made the project
  look commercial; under that assumption committing the original levels was
  declined. Clarified as a **personal, non-commercial hobby** project →
  decision D7 updated: **track the original material** (binaries, extracted
  data, disassembly) in the repo. `.gitignore` reduced to build/tooling
  artifacts only. Design docs (`decisions.md`, `legal.md`, `limitations.md`)
  corrected to the non-commercial context.
