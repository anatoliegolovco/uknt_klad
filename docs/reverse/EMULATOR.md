# Running & driving the УКНЦ emulator (ground truth)

The single source of visual/behavioral truth for КЛАД 1987 Баранов. Use **QtUkncBtl**
(UKNCBTL, Qt build) — real УКНЦ video, bundled ROM, built-in **Debug** menu
(disasm + memory + console). Not MAME/BK-0010.

## One-time setup (no sudo; FUSE not needed)
```bash
cd /tmp
curl -sL -o QtUkncBtl.AppImage \
  https://github.com/nzeemin/ukncbtl-qt/releases/download/preview-468/UKNCBTL_Qt-a808c28-x86_64.AppImage
chmod +x QtUkncBtl.AppImage
./QtUkncBtl.AppImage --appimage-extract     # -> /tmp/squashfs-root
```
The УКНЦ ROM is bundled in the build (`emulator/uknc_rom.bin`), no separate firmware.

## Launch straight into КЛАД
```bash
cd /tmp/squashfs-root
DISK=/home/anatolie/ai/klad/assets/uknc/fodos_games.dsk
DISPLAY=:0 setsid ./AppRun "-disk0:$DISK" -autostart -boot1 >/tmp/qt.log 2>&1 < /dev/null & disown
```
- **OPTIONCHAR on Linux is `-`, NOT `/`** (`-disk0:` `-autostart` `-boot1`).
- `-boot1` auto-boots from disk 0 → FODOS (ФОДОС Ф В03.00).
- At the ФОДОС prompt (blank screen + cursor), type: **`R KLAD`** then Enter.
- Title → press a speed key **1–4** to start.

## Driving it (the fiddly part — follow exactly)
- **Window id:** `xwininfo -root -children | grep "UKNC Back"` → e.g. `0x8008ec`.
- **Screenshot (correct color):** `python3 tools/shot.py <WID> /tmp/x.png`
  (decodes XWD red/green/blue channel masks; naive byte-order reads come out magenta).
- **Keyboard:** `xdotool key/type` **WITHOUT** `--window`.
  - `--window` uses XSendEvent (synthetic) → Qt **ignores** it.
  - No `--window` uses **XTEST** (trusted) → works.
  - **Click the emulator screen first** to give it focus:
    `xdotool mousemove <screenX> <screenY> click 1`.
- Example — list disk files, then run the game:
  ```bash
  XD=/tmp/xtools/usr/bin/xdotool         # local xdotool (LD_LIBRARY_PATH set)
  DISPLAY=:0 $XD windowactivate --sync <WID>
  DISPLAY=:0 $XD mousemove 480 320 click 1      # focus the screen
  DISPLAY=:0 $XD type "R KLAD"; DISPLAY=:0 $XD key Return
  ```

## Notes
- Disk `fodos_games.dsk` contains many games; КЛАД = `KLAD.SAV` (`DIR` to list).
- Color modes RGB / GRB / Gray. School monitors were **monochrome** → compare
  **shape**, not color. УКНЦ blue background is the emulator's color rendering.
- The CLI tried first with `/` prefix silently no-ops (Windows OPTIONCHAR) — always `-`.

## References captured
`assets/uknc/reference_emu/` — title.png, level1_full.png,
hud_and_field.png, player_zoom.png. Regenerate/extend when validating KPI items.
