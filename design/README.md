# design/

Project decision records, limitations, and work log for **Klad-Reimagined** —
a clean-room re-imagining of the КЛАД maze game, built in C23 and targeting
WebAssembly (browser + phone).

This folder is the "why" of the project. Code lives in `src/`,
reverse-engineering notes in `docs/reverse/`, platform/design overviews in
`docs/`.

## Index

| file | what's in it |
|------|--------------|
| [`decisions.md`](decisions.md) | Decision log (ADR-style): every significant choice and its rationale |
| [`limitations.md`](limitations.md) | Known limitations & constraints (environment, tooling, unknowns) |
| [`worklog.md`](worklog.md) | Chronological log of work performed |
| [`roadmap.md`](roadmap.md) | What's next, ordered |
| [`legal.md`](legal.md) | Copyright analysis, licensing, and the commercial-use question |

## TL;DR of current state

- **Target platform of the studied blob:** BK-0010 (not УКНЦ) — confirmed.
- **Tooling:** custom PDP-11 disassembler (`tools/pdp11dis.py`), data extractor
  (`tools/extract_data.py`), helper scripts. All work headless.
- **Reimplementation:** C23 + raylib scaffold, builds native + WASM, Romanian
  UI by default. Playable vertical slice (maze, gravity, ladders, gold, water).
- **Blocked on:** confirming the original level/sprite encoding (needs a GUI
  emulator, must be done on a local desktop).
- **Open commercial/legal item:** see [`legal.md`](legal.md). The original
  game's assets/levels are **not** shipped; only original work is.
