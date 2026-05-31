# UKNCBTL-Qt — vendored fork (headless extension)

## Provenance
- **Upstream:** https://github.com/nzeemin/ukncbtl-qt
- **Forked at commit:** `7f49f778` (2024-12-15, "Save/restore debugger breakpoints.")
- **License:** LGPL-3.0 (see `LICENSE`). Our changes below stay LGPL; the rest of this
  repo is unaffected (LGPL permits linking/using without relicensing the whole project).
- **Author of upstream:** Nikita Zimin (nzeemin) and contributors.

## Why it's here
The full УКНЦ МС-0511 machine (two KM1801VM2 CPUs, planar video, FDD/HDD, sound) as
**ground-truth for VISUAL fidelity** — and now driveable **headless**, with no Qt, no GUI,
no X11 mouse/keyboard injection. Complements `tools/uknc_emu.c` (our own C23 emulator,
which is the fast LOGIC tracer). Kept in-repo so future games can reuse it.

## What we added (`headless/`)
The emubase core (`emulator/emubase/*.cpp`: Processor, Memory, Board, Floppy, Hard,
Disasm, SoundAY) has **zero Qt calls** — only `stdafx.h`→`Common.h` pulled in `<QtGlobal>`
and `<QColor>` for integer typedefs. We supply those without Qt:

- `headless/qtshim/QtGlobal`, `headless/qtshim/QColor` — Qt-free typedef shims
  (`quint8/16/32/64`, `uint/ushort/...`).
- `headless/main.cpp` — instantiates `CMotherboard`, loads ROM (`emulator/uknc_rom.bin`,
  32256 bytes) + floppy, calls `CProcessor::Init()`, runs frames via `SystemFrame()`,
  reads planar RAM via `GetRAMWord(plane, offset)`, can inject keys via `KeyboardEvent()`.
  Stubs the few external symbols the core references (`DebugLog`, `PrintOctalValue`,
  `AssertFailedLine`).
- `headless/Makefile` — builds with plain g++ (`-std=c++17`), no Qt.

## Build & run
```bash
make -C vendor/ukncbtl-qt/headless
cd vendor/ukncbtl-qt/headless
./uknc_headless emulator/../emulator/uknc_rom.bin \
    /home/anatolie/ai/klad/assets/uknc/fodos_games.dsk 600
```
Boots ФОДОС from the disk and runs headless. (To launch КЛАД itself, inject the keystrokes
`R KLAD\n` via `CMotherboard::KeyboardEvent(scancode, pressed)` — next iteration.)

## Status
- ✅ emubase core compiles Qt-free (7/7 files)
- ✅ headless harness links (137 KB, no Qt) and **runs** — boots ФОДОС, executes frames,
  reads planar RAM
- ⬜ keystroke injection to auto-run КЛАД headless
- ⬜ planar→RGB screen dump (replicate `Emulator_PrepareScreenRGB32`) for headless PNG
  screenshots (pixel-exact visual ground truth, scriptable)

## Upstream sync
This is a vendored snapshot (no nested `.git`). To re-sync: clone upstream at a newer
commit, re-apply the `headless/` dir (it touches nothing in `emulator/`).
