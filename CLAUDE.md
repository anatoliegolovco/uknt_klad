# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

Two parallel tracks in one repo, targeting **one** game: **КЛАД 1987 (Баранов), УКНЦ МС-0511**.

1. **Study** (`docs/reverse/`, `disassembly/`, `tools/`) — reverse-engineering the УКНЦ
   binary `assets/original/extracted/uknc/KLAD_1987_Baranov.SAV`. Start at
   `docs/reverse/00_overview.md`. Ground truth = the УКНЦ emulator (`docs/reverse/EMULATOR.md`).
2. **Reimplementation** (`src/`) — clean-room C23 + raylib, native + WebAssembly.
   **No original code, art, or level data is copied.**

Out-of-scope material (BK-0010 lineage, 1991 Crocodile port, build-dep cruft) lives in
`archive/` — see `archive/README.md`. Don't work from it.

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

# Disassemble the УКНЦ target (raw mode, load address 001000)
python3 tools/pdp11dis.py --raw --org 01000 assets/original/extracted/uknc/KLAD_1987_prog.bin | less
```

The annotated disassembly already exists: `disassembly/annotated/uknc_klad_1987.asm`.
(BK-0010 `.BIN` binaries + their disassembly are archived under `archive/bk0010/`.)

## Platform: УКНЦ МС-0511 / PDP-11

Target = **Электроника УКНЦ (МС-0511)**, two KM1801VM2 (PDP-11) CPUs:
- **All constants octal** (`01000` = 512, `040000` = 16384). 16-bit little-endian.
- **Video**: planar, driven by a peripheral CPU (PPU) — NOT a direct CPU framebuffer
  (that's the BK-0010 model). Tiles are 2bpp (8 pixel-plane + 8 colour-plane bytes).
- **Game at** `001000`–`037777`, init entry `004000`. Tile bank `017450`.
- BK-0010 (linear framebuffer at `040000`) is a *different* machine — archived reference only.

## Dynamic analysis (emulator + debugger)

### ⭐ УКНЦ emulator (THE ground truth — use this, not BK-0010/MAME)

The target is УКНЦ КЛАД 1987 Баранов. Run it in **QtUkncBtl** (UKNCBTL, Qt version),
which has the real УКНЦ video, a built-in **Debug menu** (disasm + memory + console),
and the bundled УКНЦ ROM. Setup (no sudo, FUSE not needed):

```bash
# one-time: download AppImage and extract (FUSE unavailable here)
cd /tmp && curl -sL -o QtUkncBtl.AppImage \
  https://github.com/nzeemin/ukncbtl-qt/releases/download/preview-468/UKNCBTL_Qt-a808c28-x86_64.AppImage
chmod +x QtUkncBtl.AppImage && ./QtUkncBtl.AppImage --appimage-extract   # -> /tmp/squashfs-root

# run КЛАД directly (OPTIONCHAR on Linux is '-', NOT '/')
cd /tmp/squashfs-root
DISPLAY=:0 ./AppRun "-disk0:$PWD/../../home/anatolie/ai/klad/assets/original/extracted/uknc/fodos_games.dsk" \
  -autostart -boot1
# then at the ФОДОС prompt type:  R KLAD
```

**Driving it (this is the part that's fiddly — follow exactly):**
- **Screenshots:** `python3 tools/shot.py <WID> out.png` — decodes XWD channel masks → correct color.
- **Keyboard:** `xdotool key/type` **WITHOUT** `--window` (XTEST = trusted events; Qt
  ignores synthetic `--window` events). Click the emulator screen first to give it focus.
- Find WID: `xwininfo -root -children | grep "UKNC Back"`.
- Color modes RGB/GRB/Gray — school monitors were mono, so **shape > color**.

References captured from the running game: `assets/original/extracted/uknc/reference_emu/`.

### ⚠ Lives value — worked example of "ASM over display"
The emulator HUD shows "Попытки **219**". Do NOT take that at face value. ASM:
- BK-0010 `GAME_INIT`: `MOV #5,@#17436` → 5 lives (intended).
- УКНЦ `GAME_INIT` (004000): `MOV #333,@#17436` → 0o333 = 219 `[УКНЦ DIFF]`.
- Per-death: `SUB #1` (002220), game over at 0. `DEATH_SCORE` (003444) is the
  end-game bonus tally (each leftover life → +0o764=500 pts), not per-death.
219 is a real data value in this school-disk build, not a display bug — but it's
anomalous vs BK's 5. Pick fidelity (219) vs intended (5) deliberately; never invent (was 9).

### Other (BK-0010 only — secondary reference, 83% identical code)
```bash
mame bk001001 -rompath ~/mame/roms -window -autoboot_script /tmp/bk_enter.lua  # B&W
pdp11 < tools/simh_script.ini      # simh scriptable PDP-11
ghidraRun                          # static decompiler (PDP-11 built-in)
```

Traces go into `disassembly/traces/` (text). Findings get promoted into `disassembly/annotated/` and documented in `docs/reverse/`. **Fidelity tracked in `docs/reverse/FIDELITY_KPI.md`** — the scorecard; an item is ✅ only after a side-by-side with an emulator capture.

## Architecture: reimplementation (`src/`)

Modular C23 (each function cites the ASM routine it derives from):

| File | Role |
|------|------|
| `main.c` | Platform glue: window + loop (native) or `emscripten_set_main_loop` (web). Only `#ifdef PLATFORM_WEB` here. |
| `klad.h` | Shared types/constants (tile indices, `TileType`, `Input`, `GameState`, dims). |
| `map.c/.h` | Level load + collision (`COLLISION_MAP_BUILD` 013524). |
| `player.c/.h` | Movement, state machine, tile interactions (`PLAYER_*` 012570/012740). |
| `enemy.c/.h` | AI chase + collision (`ENEMY*_TICK`, `LEVEL_END_CHECK`). |
| `score.c/.h` | Score / lives / level (`SCORE_ADD`, `GAME_INIT`). ⚠ lives: see EMULATOR.md. |
| `render.c/.h` | Tileset build + tile/sprite/HUD draw (`LEVEL_RENDER`, `SPRITE_DRAW`). |
| `game.c/.h` | Game loop + dispatch (`GAME_INIT`, `GAME_TICK`, `GAME_LOOP`). |
| `gfx_data.h` | Pixel data decoded from the binary (gen: `tools/gen_gfx_c.py`). |
| `level_data.h` | Level maps + spawns from binary (gen: `tools/gen_levels_c.py`). |
| `i18n.h` | String table; `T(STR_FOO)`. Romanian default. |

**Rendering model**: draw into a `256×192` `RenderTexture2D`, upscaled nearest-neighbour to
the window. All game coords are pixels in the 256×192 space. Tileset built from the
binary's tile data (`gfx_data.h`), not hand-drawn art.

**⚠ Known fidelity gaps** (tracked in `docs/reverse/FIDELITY_KPI.md`, root cause in
`TILE_FIDELITY.md`): tile planar decode (ladders/gold/character render wrong), lives value.
Validate every fix against an emulator capture before marking ✅.

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
Adnotat BK-0010 (bază): `archive/bk0010/disassembly/crocodile_klad.asm` (35 rutine complete)  
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
- **Next (obiectiv 2)**: recursive-descent disassembly din entry `04000`, adnotare rutine în `archive/bk0010/disassembly/crocodile_klad.asm`.
