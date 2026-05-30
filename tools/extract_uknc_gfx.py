#!/usr/bin/env python3
"""
Extract tiles, sprites, and levels from KLAD_1987_Baranov.SAV (УКНЦ МС-0511).

Tile format (confirmed from code: 4× ASL then ADD #17450):
  16 bytes per tile = 8 bytes pixel-plane + 8 bytes colour-plane
  8×8 pixels, 2bpp, 4 colours via (pixel_bit, colour_bit) index.

Tile bank:   017450, stride 16, 32 entries (tiles 0-15 = level tiles, 16-31 = extra)
Sprite bank: 031300, stride 16, 208 frames (player + enemy animation)
Level maps:  022100+, 22 rows × 16 bytes (2 tiles/byte, low nibble = left, high = right)
"""

import struct, sys, os
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("Need Pillow: pip install Pillow")
    sys.exit(1)

SAV  = Path(__file__).parent.parent / "assets/original/extracted/uknc/KLAD_1987_Baranov.SAV"
OUT  = Path(__file__).parent.parent / "assets/original/extracted/uknc"

SCALE = 8  # pixels per game-pixel in output PNG

# УКНЦ 4-colour palette: (pixel_bit, colour_bit) → RGB
# Derived from УКНЦ standard colour mode. Palette 0 = black, 1 = blue/cyan,
# 2 = yellow/gold, 3 = white.
PALETTE = {
    (0, 0): (0,   0,   0),    # colour index 0 → black (background/earth)
    (1, 0): (0,   200, 80),   # colour index 1 → green (water in this game)
    (0, 1): (220, 180, 0),    # colour index 2 → yellow (gold, patterns)
    (1, 1): (240, 240, 240),  # colour index 3 → white (ladders, player)
}

TILE_NAMES = {
    0: "bg_empty",
    1: "ladder",
    2: "empty2",
    3: "empty3",
    4: "gold_a",
    5: "gold_b",
    6: "gold_c",
    7: "water",
    8: "ladder2",
    9: "wall_pat_a",
    10: "empty10",
    11: "wall_pat_b",
    12: "empty12",
    13: "wall_pat_c",
    14: "water2",
    15: "empty15",
}

# Confirmed from code: ADD #0o540 (=352 bytes) per level, base 022100
# 22 rows × 16 bytes = 352 bytes per level, 10 levels
_L1 = 0o22100
LEVEL_ADDRS = [_L1 + i * 352 for i in range(10)]

def load_prog(path):
    raw = path.read_bytes()
    assert len(raw) == 17408, f"Expected 17408 bytes, got {len(raw)}"
    return raw[512:]   # strip 512-byte RT-11 header; prog starts at 001000

def decode_tile(data16):
    """Decode a 16-byte tile (8 pixel-plane + 8 colour-plane) → 8×8 RGB image."""
    assert len(data16) == 16
    img = Image.new("RGB", (8 * SCALE, 8 * SCALE))
    pix = img.load()
    for row in range(8):
        p_byte = data16[row]
        c_byte = data16[row + 8]
        for col in range(8):
            bit = 7 - col  # MSB = leftmost pixel
            p_bit = (p_byte >> bit) & 1
            c_bit = (c_byte >> bit) & 1
            colour = PALETTE[(p_bit, c_bit)]
            for dy in range(SCALE):
                for dx in range(SCALE):
                    pix[col * SCALE + dx, row * SCALE + dy] = colour
    return img

def extract_tiles(prog):
    bank_off = 0o17450 - 0o1000
    tiles_dir = OUT / "tiles"
    tiles_dir.mkdir(exist_ok=True)

    tile_imgs = []
    for t in range(32):
        off = bank_off + t * 16
        data = prog[off:off + 16]
        img = decode_tile(data)
        name = TILE_NAMES.get(t, f"tile{t:02d}")
        img.save(tiles_dir / f"tile_{t:02d}_{name}.png")
        tile_imgs.append((t, name, img))

    # Spritesheet: 8 tiles per row
    per_row = 8
    rows = (len(tile_imgs) + per_row - 1) // per_row
    sheet = Image.new("RGB", (per_row * 8 * SCALE, rows * 8 * SCALE), (40, 40, 40))
    for t, name, img in tile_imgs:
        x = (t % per_row) * 8 * SCALE
        y = (t // per_row) * 8 * SCALE
        sheet.paste(img, (x, y))
    sheet.save(OUT / "tiles" / "tileset.png")
    print(f"Tiles: {len(tile_imgs)} tiles → tiles/tile_NN_name.png + tileset.png")

def extract_sprites(prog):
    spr_off = 0o31300 - 0o1000
    spr_end = 0o37677 - 0o1000
    spr_data = prog[spr_off:spr_end + 1]
    n_frames = len(spr_data) // 16
    sprites_dir = OUT / "sprites"
    sprites_dir.mkdir(exist_ok=True)

    frame_imgs = []
    for i in range(n_frames):
        data = spr_data[i * 16:(i + 1) * 16]
        img = decode_tile(data)
        img.save(sprites_dir / f"frame_{i:03d}.png")
        frame_imgs.append(img)

    # Spritesheet: 16 frames per row
    per_row = 16
    rows = (n_frames + per_row - 1) // per_row
    sheet = Image.new("RGB", (per_row * 8 * SCALE, rows * 8 * SCALE), (40, 40, 40))
    for i, img in enumerate(frame_imgs):
        x = (i % per_row) * 8 * SCALE
        y = (i // per_row) * 8 * SCALE
        sheet.paste(img, (x, y))
    sheet.save(OUT / "sprites" / "spritesheet.png")
    print(f"Sprites: {n_frames} frames → sprites/frame_NNN.png + spritesheet.png")

def decode_level(prog, addr, level_num):
    off = addr - 0o1000
    rows = 22
    cols_bytes = 16   # 16 bytes → 32 tile columns (2 per byte)
    tile_map = []
    for r in range(rows):
        row = []
        for b in range(cols_bytes):
            byte = prog[off + r * cols_bytes + b]
            row.append(byte & 0x0F)         # low nibble = left tile
            row.append((byte >> 4) & 0x0F)  # high nibble = right tile
        tile_map.append(row)
    return tile_map

TILE_COLOURS_FLAT = {
    0:  (0,   0,   0),    # bg/empty → black
    1:  (200, 200, 200),  # ladder → grey/white
    2:  (20,  20,  20),   # empty2 → near-black
    3:  (20,  20,  20),   # empty3
    4:  (220, 180, 0),    # gold a → yellow
    5:  (200, 160, 0),    # gold b
    6:  (180, 140, 0),    # gold c
    7:  (0,   100, 200),  # water → blue
    8:  (200, 200, 200),  # ladder2
    9:  (100, 60,  20),   # wall pat a → brown
    10: (20,  20,  20),
    11: (80,  50,  15),   # wall pat b
    12: (20,  20,  20),
    13: (60,  40,  10),   # wall pat c
    14: (0,   80,  180),  # water2
    15: (20,  20,  20),
}

def extract_levels(prog):
    levels_dir = OUT / "levels"
    levels_dir.mkdir(exist_ok=True)
    import json

    TILE_SIZE = 8
    ROWS = 22
    COLS = 32

    for i, addr in enumerate(LEVEL_ADDRS):
        num = i + 1
        tile_map = decode_level(prog, addr, num)

        # JSON
        data = {
            "level": num,
            "address": f"0o{addr:o}",
            "rows": ROWS,
            "cols": COLS,
            "tiles": tile_map,
        }
        json_path = levels_dir / f"level_{num:02d}.json"
        json_path.write_text(json.dumps(data, indent=2))

        # PNG: each tile = TILE_SIZE px, no upscale (keep it compact)
        TS = TILE_SIZE * 2
        img = Image.new("RGB", (COLS * TS, ROWS * TS), (80, 50, 20))  # earth bg
        pix = img.load()
        for r, row in enumerate(tile_map):
            for c, tile_idx in enumerate(row):
                colour = TILE_COLOURS_FLAT.get(tile_idx, (128, 0, 128))
                for dy in range(TS):
                    for dx in range(TS):
                        pix[c * TS + dx, r * TS + dy] = colour
        img.save(levels_dir / f"level_{num:02d}.png")
        print(f"  Level {num}: addr={oct(addr)}, {ROWS}×{COLS} tiles → level_{num:02d}.json + .png")

def main():
    prog = load_prog(SAV)
    print(f"Loaded {SAV.name}: {len(prog)} bytes program image\n")

    print("=== Tiles ===")
    extract_tiles(prog)

    print("\n=== Sprites ===")
    extract_sprites(prog)

    print("\n=== Levels ===")
    extract_levels(prog)

    print("\nDone.")

if __name__ == "__main__":
    main()
