# Headless УКНЦ emulator — build & use guide (for humans and LLMs)

This explains how to **compile and drive the headless УКНЦ emulator** we built on top of our
LGPL fork of UKNCBTL-Qt. It runs the real КЛАД 1987 (Баранов) with **no GUI, no X11, no mouse
or keyboard injection through the desktop** — everything is programmatic, so an agent can boot
the game, inject keystrokes, capture pixel-exact screenshots, and read machine RAM.

> **Two emulators, two jobs** (see also `../reverse/EMULATOR.md`):
> - `tools/uknc_emu.c` — our own **C23** KM1801VM2 core. Fast **logic** tracer (call a routine,
>   diff RAM). Use for mechanics / reverse engineering. Validated: `GAME_INIT` → lives `0o333`.
> - `vendor/ukncbtl-qt/headless/` — the **full УКНЦ machine** (2 CPUs, planar video, FDD). Use
>   for **visual ground truth** (pixel-exact frames) and scripted playthroughs. ← this doc.

## 1. Build

```bash
make -C vendor/ukncbtl-qt/headless
# produces: vendor/ukncbtl-qt/headless/uknc_headless   (plain g++, NO Qt)
```
Why it builds without Qt: the emubase core (`vendor/ukncbtl-qt/emulator/emubase/*.cpp`) has no
Qt calls; it only pulled Qt for integer typedefs via `stdafx.h`→`Common.h`. We supply those
with a Qt-free shim in `headless/qtshim/` (`QtGlobal`, `QColor`). See `../../vendor/ukncbtl-qt/FORK.md`.

## 2. Run

```bash
cd vendor/ukncbtl-qt/headless
ROM=../emulator/uknc_rom.bin                                    # 32256-byte УКНЦ ROM (in the fork)
DISK=/home/anatolie/ai/klad/assets/uknc/fodos_games.dsk        # ФОДОС disk with KLAD.SAV

# (a) raw: run N frames from cold boot, optional screenshot (PPM)
./uknc_headless $ROM $DISK 600 /tmp/boot.ppm

# (b) scripted: auto-boot КЛАД to gameplay, then screenshot
./uknc_headless $ROM $DISK klad /tmp/klad.ppm
python3 -c "from PIL import Image; Image.open('/tmp/klad.ppm').save('/tmp/klad.png')"
```
Screen is **640×288 ARGB → PPM (P6)**. Convert to PNG with Pillow as above.

Reference captures (committed): `assets/uknc/reference_emu/headless/` — boot menu, title, gameplay.
The gameplay HUD shows **"Попытки 219"**, confirming the `0o333 = 219` lives value from the ASM.

## 3. How the scripted boot works (`boot_klad()` in `main.cpp`)

The УКНЦ keyboard uses **octal scancodes**; `CMotherboard::KeyboardEvent(scancode, pressed)`
injects them. The sequence:
1. wait for the `ЗАГРУЗКА` boot menu → press **`1`** (disk) + **Enter**
2. wait for the ФОДОС prompt → type **`R`,space,`K`,`L`,`A`,`D`** + **Enter** (runs `KLAD.SAV`)
3. wait for the title → **Enter**, then speed key **`1`** → into the maze

Scancodes (octal): `1`=030 `R`=074 `K`=052 `L`=056 `A`=072 `D`=057 space=0113 Enter=0153 ALF(lat)=0106.

## 4. Driving it programmatically (extend `main.cpp`)

```cpp
CProcessor::Init();                       // once: build the opcode dispatch table
CMotherboard* b = new CMotherboard();
g_pBoard = b; g_okEmulatorInitialized = true;
b->LoadROM(rombuf);                       // 32 KB buffer (ROM is 32256, zero-padded)
b->AttachFloppyImage(0, diskpath);
b->Reset();
for (int i=0;i<N;i++) b->SystemFrame();   // run N frames (1 frame = one video frame)
b->KeyboardEvent(scancode, true/false);   // inject a key (press / release)
uint16_t w = b->GetRAMWord(plane, off);   // read planar RAM (planes 0..2)
Emulator_PrepareScreenRGB32(buf, ScreenView_StandardRGBColors);  // 640x288 ARGB
```
To **capture gameplay frames** (e.g. to verify a tile/sprite or trace a mechanic visually):
boot with `boot_klad()`, then inject movement keys (arrows / the КЛАД control keys) between
`SystemFrame()` batches and screenshot after each — fully deterministic, no desktop interaction.

> ⚠ `GetRAMWord(plane,offset)` reads a **video plane** directly; it is **not** the CPU's mapped
> address space. For CPU-address logic (e.g. the tile work buffer at 014550), use the C23
> `tools/uknc_emu.c` instead, or go through the memory controller.

## 5. Roadmap — a pure-C23 full-УКНЦ emulator?

Open question (tracked): port the full УКНЦ **architecture** (planar memory map, peripheral PPU)
from the C++ `emubase/` into our C23 `tools/uknc_emu.c`, so the visual emulator is also C23 and
free of the LGPL C++ dependency. This is a **manual idiomatic port** (C++ classes → C structs +
functions), not an automatic transpile — `emubase/Memory.cpp` (planar model) and `Board.cpp`
(memory map, timers) are the reference. Until then, this fork is the visual ground truth and the
C23 core is the logic tracer; both live in the repo.
