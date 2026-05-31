# КЛАД 1987 (Баранов) — canonical version facts

The single version this project targets. (The 1991 Crocodile port and BK-0010 builds
are in `/archive` and are **not** part of this work.)

| Field | Value |
|-------|-------|
| Title | КЛАД |
| Author | Баранов Д.Г. (Николаев) |
| Year | 1987 |
| Platform | Электроника УКНЦ (МС-0511) |
| Distribution | school disk "Н-Шангская СШ" |
| Game file on disk | `KLAD.SAV` (FODOS, on `fodos_games.dsk`) |
| Extracted | `assets/uknc/KLAD_1987_Baranov.SAV` (17 408 B, 34 blocks) |
| Load address | `001000` (octal); entry runs into init at `004000` |

## Identification strings (KOI8-R, in the binary)
- `"Николаев 1987"` @ byte 2532
- `"Баранов"` @ byte 2546
- `"Д.Г."` @ byte 2554

## Intro text (KOI8-R, verbatim)
```
КЛАД
Игра заключается в том,чтобы
пройти все лабиринты,собирая
все клады и избегая встреч
с зелеными человечками,стараться
не упасть в воду и набрать
максимальное число очков
Управлять движением красного
человечка можно при помощи клавиш
Стрельба влево - <Q>
Стрельба вправо- <S>
Выберите скорость,нажав любую
из клавиш:1,2,3,4(1-max,4-min)
```

## Controls
- Move the **red man**: direction keys (movement).
- Shoot left: **Q**. Shoot right: **S**.
- Speed select at start: **1–4** (1 = max speed, 4 = min).

## HUD
- `"Счет"` — score.
- `"Попытки"` — attempts/lives. ⚠ See lives note below.

## Lives — ASM truth vs display
- `GAME_INIT` (004000): `MOV #333,@#17436` → **0o333 = 219** `[УКНЦ DIFF]`.
- BK-0010 equivalent inits to **5** — so 219 is anomalous to this school-disk build.
- Per-death: `SUB #1` (002220); game over at 0. `DEATH_SCORE` (003444) is the
  end-game bonus tally (each remaining life → `+0o764` = 500 pts), not per-death.
- The emulator faithfully displays "Попытки 219" → it is a **data** value, not a
  display bug. Reimplementation must choose deliberately: replicate 219 (strict
  fidelity to this build) or use 5 (the evidently-intended design). **Do not invent.**

## Confirmed mechanics constants (from ASM)
- Score per gold: `ADD #12` (octal) = **+10** (`SCORE_ADD` 003764).
- Map: **22 rows × 32 cols**, 2 tiles/byte (nibble), stride `0o540` = 352 B/level.
- Tile bank @ `017450`, 16 B/tile (8 pixel-plane + 8 colour-plane, 8×8 2bpp).
- Levels: 10 (`TBL_LEVEL_PTRS`).

See `MECHANICS.md`, `ROUTINES_UKNC.md`, `GFX_MAP.md` for detail.
