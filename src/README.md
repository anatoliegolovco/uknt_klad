# src — the reimplementation (C23 → WebAssembly)

Clean-room game. Same source builds **native** (fast dev) and **web** (ship).

## Prerequisites

- **C23 compiler** (gcc ≥ 13 or clang ≥ 16).
- **raylib** for native: `sudo apt install libraylib-dev` (or build from source).
- **Emscripten** for web: install the [emsdk](https://emscripten.org), then a
  **web build of raylib** (`make PLATFORM=PLATFORM_WEB` in raylib's `src/`,
  producing `libraylib.web.a`).

## Build & run

```bash
# native (a window, for development)
make run

# webassembly (the real target: browser + phone)
make web
python3 -m http.server -d ../build/web   # then open http://localhost:8000
```

Point the web build at your raylib if it isn't in the default place:

```bash
make web RAYLIB_SRC=/path/to/raylib/src RAYLIB_WEB=/path/to/libraylib.web.a
```

## What works now

A minimal vertical slice: tile maze, gravity, walk + ladder climb, gold pickup,
water-death respawn, fixed 256×192 render target upscaled crisply, keyboard +
touch input, and the Emscripten main-loop split in `main.c`. Guards, scoring,
levels, title/clear screens, and audio are next (see `../docs/design.md`).

## Files

- `main.c` — platform glue + main loop (only target-specific file).
- `game.h` — public API + constants (render size, tiles, `Input`).
- `game.c` — logic + rendering (all original).
- `web/shell.html` — responsive Emscripten HTML shell.
- `Makefile` — native + web targets.
