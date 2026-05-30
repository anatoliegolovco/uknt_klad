# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

Two parallel tracks in one repo:

1. **Study** (`docs/reverse/`, `disassembly/`, `tools/`) — reverse-engineering the original Soviet game **КЛАД** (BK-0010 / PDP-11 binary) to understand its mechanics.
2. **Reimplementation** (`src/`) — a clean-room C23 + raylib game inspired by those mechanics, compiled to WebAssembly for the browser. **No original code, art, or level data is copied.**

## Build commands (reimplementation — `src/`)

All commands run from `src/`:

```bash
# Native dev build (needs libraylib-dev)
make -C src

# Run native
make -C src run

# WebAssembly build (needs emscripten + web-built libraylib.a)
# Set RAYLIB_WEB and RAYLIB_SRC env vars if not in ~/raylib/src/
make -C src web

make -C src clean
```

The web build outputs to `build/web/index.html`. Serve with:
```bash
python3 -m http.server -d build/web
```

The pre-built wasm artifact in `play/` is the deployed version for GitHub Pages (`https://anatoliegolovco.github.io/uknt_klad/`).

## Study-side commands (`tools/`)

```bash
# Download and verify the original blobs (SHA-256 checked)
tools/fetch_original.sh

# Regenerate all raw disassemblies into disassembly/raw/
tools/disasm.sh

# Disassemble a single binary (octal output, BK .BIN format)
python3 tools/pdp11dis.py assets/original/ex_klad3/KLAD3.BIN | less

# JSON output for programmatic analysis
python3 tools/pdp11dis.py assets/original/ex_klad3/KLAD3.BIN --json

# Raw mode (no .BIN header, explicit load address)
python3 tools/pdp11dis.py --raw --org 01000 dump.bin
```

## Platform: BK-0010 / PDP-11

The studied binaries target the **Электроника БК-0010**, a Soviet PDP-11 clone:
- **CPU**: KM1801VM2, PDP-11 compatible. All registers general-purpose; R6=SP, R7=PC.
- **All constants are octal** (`01000` = 512 decimal, `040000` = 16384).
- **16-bit little-endian words**, 64 KiB flat address space.
- **Video**: linear framebuffer at `040000`–`077777` (16 KiB, 256×256 px). Sprite blitting = word writes into this region.
- **I/O registers**: `0177662` = keyboard data, `0177664` = scroll, `0177716` = system register.
- **Game lives at** `001000`–`037777`. Primary study target is `KLAD3.BIN` (load `01000`, entry `JMP @#4000`).

## Dynamic analysis (emulator + debugger)

The installed stack for CLI-driven dynamic analysis:

```bash
# MAME — BK-0010 emulator with built-in PDP-11 debugger
mame bk0010 -debug -flop1 assets/original/ex_klad3/KLAD3.BIN

# simh — scriptable PDP-11 simulator
pdp11 < tools/simh_script.ini

# Ghidra — static decompiler (PDP-11 built-in, no plugin needed)
# Import KLAD3.BIN as: Raw Binary, language PDP-11, load address 0x0200 (= 01000 octal)
ghidraRun
```

Key breakpoints for tracing:
- `04000` — real game init entry
- reads of `@#177662` — keyboard polling loop
- writes into `040000`+ — sprite blit routine + data tables
- the water-death transition — collision logic

Traces go into `disassembly/traces/` (text). Findings get promoted into `disassembly/annotated/` and documented in `docs/reverse/03_boot_init.md`…`09_sound.md`.

## Architecture: reimplementation (`src/`)

Three source files only:

| File | Role |
|------|------|
| `main.c` | Platform glue: raylib init, main loop (native) or `emscripten_set_main_loop` (web). The only `#ifdef PLATFORM_WEB` lives here. |
| `game.c` | All game logic + rendering. Render target is a fixed `256×192` px `RenderTexture2D` upscaled nearest-neighbour to the window. |
| `game.h` | Public API: `game_init`, `game_frame`, `game_shutdown` + `Input` struct + `VW/VH/TILE/COLS/ROWS` constants. |
| `i18n.h` | String table: add key to `StrId` enum + row to `STRINGS[]`. Use `T(STR_FOO)` everywhere. Romanian (`LANG_RO`) is default. |

**Rendering model**: `game.c` draws into a `RenderTexture2D target` (VW=256, VH=192, TILE=8). `main.c` upscales that to the window at nearest-neighbour. All game coordinates are in pixels within the 256×192 space.

**Tileset**: built procedurally at startup from `TILE_ART` char-arrays → `build_tileset()` → single `Texture2D`. Tile indices: `TI_WALL`, `TI_LADDER`, `TI_WATER`, `TI_GOLD`, `TI_PLAYER`.

**Input**: `read_input()` in `game.c` maps keyboard (WASD/arrows/space) and touch (thirds of screen) to the `Input` struct. Platform-agnostic.

## Key decisions (see `design/decisions.md` for full ADRs)

- **D1**: Custom `tools/pdp11dis.py` instead of radare2 (Ubuntu radare2 has no PDP-11 plugin).
- **D3**: Study target pivoted la УКНЦ (МС-0511) — versiunea originală Баранов 1987. BK-0010 Crocodile era repacked din același cod (83% identic). УКНЦ folosește video planar prin porturi @#176640/176642.
- **D4**: C23 + raylib → WebAssembly. Single codebase, native dev + web ship.
- **D5**: Romanian UI strings by default via `i18n.h`.
- **D6/D7**: Original blobs and disassembly ARE tracked in the repo (personal preservation archive). `.gitignore` excludes only build artifacts.

## Ce vrea userul — obiective obligatorii (УКНЦ КЛАД 1987 Баранов)

**⚠ TARGET ACTUAL: `assets/original/extracted/uknc/KLAD_1987_Baranov.SAV` (УКНЦ МС-0511)**  
**NU** `KLAD.BIN` (BK-0010) — acela era reperul anterior, acum lucrăm cu originalul УКНЦ.

**IMPORTANT:** Dacă ești tentat să spui că totul e gata și totul e bine, oprește-te și citești mai întâi `docs/reverse/USER_GOALS.md`. Acel fișier conține lista exactă de livrabile cu statusuri. Dacă vreun status e ❌, NU e gata.

Context: BK-0010 Crocodile КЛАД era repacked din același cod (83% identic cu УКНЦ 1987).  
Adnotat BK-0010 (bază): `disassembly/annotated/crocodile_klad.asm` (35 rutine complete)  
Adnotat УКНЦ (target): `disassembly/annotated/uknc_klad_1987.asm`

Cele 6 obiective (detalii + criterii în `docs/reverse/USER_GOALS.md`):
1. **BYTE_MAP** — hartă completă a octeților din `KLAD_1987_Baranov.SAV` → `docs/reverse/BYTE_MAP.md`
2. **Assembler adnotat** — cod + doc → `disassembly/annotated/uknc_klad_1987.asm` + `docs/reverse/ROUTINES.md`
3. **Mecanica jocului** — exclusiv din cod assembler → `docs/reverse/MECHANICS.md`
4. **Toate nivelurile** — extrase ca JSON + PNG → `assets/original/extracted/uknc/levels/`
5. **Sprites/texturi** — pixel fidelity → `assets/original/extracted/uknc/tiles/` + `docs/reverse/GFX_MAP.md`
6. **Animații** — frame-uri, timing, triggere → `docs/reverse/ANIMATIONS.md`

Log de lucru (cu timestamp, pentru continuitate după crash): `docs/reverse/WORK_LOG.md`

## Reverse-engineering progress

- **Done**: download + verify blobs, BK-0010 platform ID, linear-sweep disassembly (`disassembly/raw/`), memory map, I/O register map, level data format (`docs/reverse/08_level_data.md`), graphics extraction via BK CPU core (`title_screen.png`, `sprites.png`, `gfx_region.bin`).
- **Next (obiectiv 2)**: recursive-descent disassembly din entry `04000`, adnotare rutine în `disassembly/annotated/crocodile_klad.asm`.
