# GFX_MAP — КЛАД 1987 Баранов (УКНЦ МС-0511)

**Target binary:** `assets/original/extracted/uknc/KLAD_1987_Baranov.SAV`  
**Extracted to:** `assets/original/extracted/uknc/tiles/`, `assets/original/extracted/uknc/sprites/`  
**Extraction script:** `tools/extract_uknc_gfx.py`

---

## Tile Format (confirmed from disassembly)

Each tile is **16 bytes = 8 bytes pixel-plane + 8 bytes colour-plane**, representing an **8×8 pixel, 2bpp** graphic.

**Address formula** (from code at 005024–005034):
```
tile_addr = tile_index × 16 + 017450
```
Confirmed via four consecutive `ASL R2` instructions (×16) followed by `ADD #17450, R2`.

**Colour encoding:**

| pixel_bit | colour_bit | Palette index | Visual |
|-----------|------------|---------------|--------|
| 0 | 0 | 0 | Black (background/air) |
| 1 | 0 | 1 | Green (water, ground) |
| 0 | 1 | 2 | Yellow (gold, patterns) |
| 1 | 1 | 3 | White (ladders, bright) |

Note: actual hardware palette depends on the УКНЦ palette register state, which the game initialises at startup. The colours above are a reasonable approximation for display purposes.

---

## Tile Bank (017450)

**Location:** `DAT_TILE_PIXELS = 017450`  
**Count:** 32 tiles  
**Stride:** 16 bytes  
**Address range:** 017450–020450

| Index | Address | Name | Pixel plane | Colour plane | Notes |
|-------|---------|------|-------------|--------------|-------|
| 0  | 017450 | `bg_empty` | all zeros | all zeros | Background / air (never blitted) |
| 1  | 017470 | `ladder` | 3C 3C FF FF 3C 3C 3C 3C | same | Cross pattern = ladder rungs. Both planes identical → colour 3 (white) |
| 2  | 017510 | `exit` | all zeros | all zeros | Exit marker — same pixels as air but collision code treats it as EXIT |
| 3  | 017530 | `empty3` | all zeros | all zeros | Unused or platform marker |
| 4  | 017550 | `gold_a` | all zeros | FC 3F A8 2A FC 3F FC 3F | Gold — only colour plane active → colour 2 (yellow) |
| 5  | 017570 | `gold_b` | all zeros | FC 3F A8 2A FC 3F FC 3F | Gold animation frame B (identical to A in this version) |
| 6  | 017610 | `gold_c` | all zeros | FC 3F A8 2A FC 3F FC 3F | Gold animation frame C |
| 7  | 017630 | `water` | 00 00 FF FF CC CC 00 00 | all zeros | Water — pixel plane only → colour 1 (green/blue). Wavy pattern rows 2–5 |
| 8  | 017650 | `ladder2` | 3C 3C FF FF 3C 3C 3C 3C | same | Same pixels as tile 1 — appears as structural platform/second ladder type |
| 9  | 017670 | `wall_a` | FC 3F A8 2A FC 3F FC 3F | same | Wall/earth — brick pattern, both planes active → colour 3 (white on bg) |
| 10 | 017710 | `empty10` | all zeros | all zeros | Unused |
| 11 | 017730 | `wall_b` | FC 3F A8 2A FC 3F FC 3F | same | Wall (level border — right/bottom border tile) |
| 12 | 017750 | `wall_c` | AA A0 2A 8A... | A2 AA 88... | Earth/fill — complex pattern; dominant wall tile in level interiors |
| 13 | 017770 | `wall_d` | 2A 2A A8 A8... | A2 A2 88... | Wall (level border — bottom row variant) |
| 14 | 020010 | `water2` | 00 00 50 50 14 14 55 55... | all zeros | Water variant / animated water |
| 15 | 020030 | `empty15` | all zeros | all zeros | Unused |
| 16–31 | 020050–020430 | sprite tiles | various | various | Extended tile bank — sprite/entity graphics (player, enemies) |

**Observed tile usage in levels (from 10 level maps):**
- Most frequent: 0 (air), 11, 12, 13 (wall/earth), 1, 8 (ladder)
- Level borders: tile 11 (left/right) and tile 13 (bottom)
- Collectables: 4, 5, 6 (gold)
- Hazards: 7, 14 (water)
- Special: tile 2 (exit — appears once per level, at top row near a ladder)

---

## Sprite / Animation Frame Bank (031300)

**Location:** 031300  
**Count:** 208 frames  
**Stride:** 16 bytes  
**Address range:** 031300–037677

Each frame is 16 bytes (same format as tiles: 8 pixel + 8 colour, 8×8px 2bpp).

**Frame structure** (observed from byte patterns):
- All frames share a common outer border: `BB 08` at top, `B8 BB` at bottom (character silhouette outline)
- Inner 6 rows vary = animation content (legs, body movement)
- Groups of ~4 consecutive similar frames = one animation cycle

**Estimated frame groups** (requires visual inspection to confirm boundaries):
| Frame range | Estimated content |
|-------------|------------------|
| 000–003 | Player walk cycle (frame A1-A4) |
| 004–007 | Player walk alternate direction |
| 008–011 | Player on ladder (climbing) |
| 012–015 | Player death / fall |
| 016–207 | Enemy variants + additional player poses |

**Spritesheet:** `assets/original/extracted/uknc/sprites/spritesheet.png` (16 frames/row)

---

## Output Files

```
assets/original/extracted/uknc/
├── tiles/
│   ├── tile_00_bg_empty.png  … tile_31_*.png   (32 files, 64×64px each)
│   └── tileset.png                              (256×64px, 8 tiles/row)
├── sprites/
│   ├── frame_000.png  …  frame_207.png          (208 files, 64×64px)
│   └── spritesheet.png                          (1024×832px, 16/row)
└── levels/
    ├── level_01.json  …  level_10.json
    └── level_01.png   …  level_10.png
```

---

## Known Limitations

1. **Palette**: exact УКНЦ palette values (palette register contents at game init) not yet confirmed — would require emulator trace. Current colours are approximations.
2. **Tile 12 pixel data**: complex pattern at T12 (`2A 2A A8 A8 22 22 8A 8A | A2 A2 88 88 2A 2A 88 A8`) renders as a diagonal-stripe earth texture.
3. **Frame groupings**: sprite animation groupings are estimated. FN_SPRITE_DRAW (014030) + SPRITE_HELPERS (013216) control frame selection — see ANIMATIONS.md (not yet written) for details.
4. **Tile 2 = EXIT**: identified from level data (single occurrence at top row near ladder) but not yet confirmed against collision code.
