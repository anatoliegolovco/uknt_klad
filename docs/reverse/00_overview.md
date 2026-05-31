# КЛАД (1987, Баранов) — УКНЦ МС-0511 — project overview

This repository reverse-engineers **one specific game** and reimplements it in C23:

> **КЛАД** — Николаев / Баранов Д.Г., **1987**, for the **Электроника УКНЦ (МС-0511)**.
> School distribution: "Н-Шангская СШ". Validated running in the УКНЦ emulator.

Everything BK-0010, the 1991 Crocodile port, and other versions has been moved to
[`/archive`](../../archive/) — see `archive/README.md`. The working tree is this
version **only**.

---

## The target binary

| | |
|---|---|
| Game file | `assets/uknc/KLAD.SAV` (on disk) |
| Extracted image | `assets/uknc/KLAD_1987_Baranov.SAV` (17 408 B = 34 RT-11 blocks) |
| Program image | `assets/uknc/KLAD_1987_prog.bin` (load `001000`) |
| Source disk | `assets/uknc/fodos_games.dsk` (FODOS filesystem) |
| Disassembly | `disassembly/annotated/uknc_klad_1987.asm` (full, annotated) |
| Author strings | `"Николаев 1987"` @2532, `"Баранов"` @2546, `"Д.Г."` @2554 |

**Platform:** УКНЦ = two KM1801VM2 (PDP-11) CPUs; planar video via a peripheral CPU
(no direct CPU framebuffer like BK-0010). All constants octal. Game at `001000`–`037777`.

---

## Run it (ground truth)

See [`EMULATOR.md`](EMULATOR.md). Short version — QtUkncBtl, then `R KLAD` at the
ФОДОС prompt. Reference captures: `assets/uknc/reference_emu/`.

---

## Game rules (from the binary's own intro text, KOI8-R)

- Goal: traverse all mazes, collect all treasure (клады), avoid the **green men**
  (зелёные человечки), don't fall in water, maximise score.
- You control the **red man** (красный человечек).
- Shoot left = **Q**, shoot right = **S**.
- Speed select at start: keys **1–4** (1 = max, 4 = min).
- HUD: `"Счет"` (score) + `"Попытки"` (attempts/lives).

---

## Document map

| Doc | Content |
|-----|---------|
| `EMULATOR.md` | How to run + drive the УКНЦ emulator, capture references |
| `KLAD_1987_BARANOV.md` | Canonical version facts (this build's specifics) |
| `MECHANICS.md` | Game mechanics extracted from ASM |
| `ROUTINES_UKNC.md` | Index of all routines (address → name → role) |
| `GFX_MAP.md` | Tile/sprite memory map + format |
| `TILE_FIDELITY.md` | Ladder/gold/character shape discrepancies + root cause |
| `ANIMATIONS.md` | Animation timing + sprite frames |
| `BYTE_MAP.md` | Byte-range catalogue of the SAV |
| `LEVEL_FORMAT.md` | Level data format + extraction |
| `FIDELITY_KPI.md` | **Scorecard** — reimplementation vs emulator, ✅ only when validated |
| `REIMPL_PLAN.md` | C23 reimplementation plan (ASM-first) |
| `WORK_LOG.md` | Timestamped progress log |
| `USER_GOALS.md` | The objectives + absolute rules |

---

## Reimplementation (`src/`)

Clean-room C23 + raylib → native (Linux) **and** WebAssembly. Logic derived strictly
from `uknc_klad_1987.asm`; graphics decoded from the binary. No original code copied.
Fidelity is tracked objectively in `FIDELITY_KPI.md` against emulator captures.
