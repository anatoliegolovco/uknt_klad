# Klad-Reimagined

A clean-room re-imagining of the Soviet maze game **КЛАД** — built from scratch
in **C23**, compiled to **WebAssembly** so it runs in any browser, including on
phones. Retro look, modern engine, original code and assets.

> **Working title only.** The final game name is still TBD — see
> [`docs/design.md`](docs/design.md).

## What is this?

КЛАД ("Treasure") is a maze/platform game from the late-1980s/early-1990s for
Soviet PDP-11-compatible home computers (Электроника БК-0010 and the УКНЦ /
Электроника МС-0511). It is itself a port of *Rise Out From Dungeons* (ASCII
Corp., 1983). You climb through a dungeon, collect treasure, dodge guardians,
and avoid water — which is instant death.

This repository has two halves:

1. **Study** (`docs/reverse/`, `disassembly/`, `tools/`) — a personal,
   non-commercial reverse-engineering effort to *understand the mechanics* of
   the original from its binary, using tools written from scratch here.
2. **Reimplementation** (`src/`) — a brand-new game inspired by those
   mechanics, written clean-room: **no original code, art, or level data is
   copied.** Modern engine, retro aesthetic.

## ⚠️ Legal / originality

A **personal, non-commercial** project to understand and preserve a piece of
legacy Soviet PDP-11 software. The original material (binaries, extracted data,
disassembly) is tracked here as a personal study/preservation archive. See
[`docs/design/legal.md`](docs/design/legal.md).

- The reimplementation in `src/` is original work (own code; planned own art and
  levels), credited to its inspiration. See [`LICENSE`](LICENSE).

## Layout

```
assets/uknc/   the original blob + extracted data (tracked; see docs/design/legal.md)
assets/new/        our own art/levels
docs/              architecture, reverse-engineering notes, design
disassembly/raw/   linear-sweep disassembly (tooling output)
disassembly/annotated/  hand-annotated, symbolised routines
tools/             pdp11dis.py (our PDP-11 disassembler) + helper scripts
src/               the C23 / WASM reimplementation
```

## Quick start (study side)

```bash
# 1. fetch the original blob locally (not committed)
tools/fetch_original.sh

# 2. disassemble every variant into disassembly/raw/
tools/disasm.sh

# 3. inspect, e.g. the cleanest BK-0010 variant
python3 tools/pdp11dis.py assets/uknc/ex_klad3/KLAD3.BIN | less
```

## Quick start (game side)

See [`src/README.md`](src/README.md) for native and WebAssembly builds.

## Status

Early. The toolchain, disassembler, and first reverse-engineering notes exist;
the reimplementation is being scaffolded. See [`docs/reverse/00_overview.md`](docs/reverse/00_overview.md).
