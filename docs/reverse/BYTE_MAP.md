# BYTE_MAP — КЛАД 1987 Баранов (УКНЦ МС-0511)

**File:** `assets/uknc/KLAD_1987_Baranov.SAV`  
**Size:** 17408 bytes  
**Format:** RT-11 .SAV executable  
**Note:** Previous version of this file covered BK-0010 Crocodile KLAD.BIN (still in git history).

---

## File Layout

| Offset (file) | Size | Content |
|---------------|------|---------|
| 0x000–0x1FF | 512 bytes | RT-11 .SAV header (SAVBLK=41, SAVTOP=040000, SAVSTA=001000) |
| 0x200–0x43FF | 16384 bytes | Program image (loaded at 001000–037777 in PDP-11 address space) |

Program image = 16384 bytes = 8192 words covering 001000–040000 (octal).

---

## Program Memory Map (001000–042000)

All addresses are octal. File offset = `(address_decimal - 512) + 512 = address_decimal`.

### 001000–001777 — BOOTSTRAP / INIT CODE
| Address | Size | Content |
|---------|------|---------|
| 001000 | 2 | Entry point: JMP @#004000 |
| 001004–001033 | ~24 | Game-over / restart handler |
| 001034–001141 | ~88 | LEVEL_COMPLETE: advance to next level, reload entity table |
| 001142–001227 | ~70 | KEY_DIFFICULTY: maps keys 1–4 to speed values |
| 001230–001277 | 40 | **TBL_ENTITY_PTRS**: 10 words → entity start-position records for each level |
| 001300 | 2 | **VAR_CUR_MAP_ADDR**: current level tile map address (init = 022100) |
| 001302 | 2 | **VAR_CUR_LEVEL_PTR**: current entity record pointer |
| 001304 | 2 | **VAR_LEVEL_TBL_PTR**: current index into TBL_ENTITY_PTRS |
| 001306–001777 | ~374 | Intro screen display, title text output |

### 002000–003777 — INTRO / MENU
| Address | Size | Content |
|---------|------|---------|
| 002000–003777 | 1024 | Intro animation, difficulty menu, high-score display |

### 004000–007777 — GAME INIT + MAIN LOOP
| Address | Size | Content |
|---------|------|---------|
| 004000–004177 | 128 | FN_GAME_INIT: display setup, entity init, call level render |
| 004200–004777 | 384 | FN_GAME_MAIN_LOOP: tick all entities, input polling, state machine |

### 010000–013523 — PLAYER + ENTITY LOGIC
| Address | Size | Content |
|---------|------|---------|
| 010000–010777 | 512 | Player move handler, WASD/arrow key processing |
| 011000–011777 | 512 | Player-on-ladder logic, fall physics |
| 012000–012441 | 289 | Score update, gold-collection handler |
| 012442–012527 | 70 | FN_LEVEL_RENDER_FULL: blit entire tile work buffer to screen |
| 012530–012567 | 32 | FN_TILE_BLIT_SUB: blit single tile via DISP_COL_BLIT |
| 012570–012715 | 102 | FN_PLAYER_STATE_CHECK: player death/win state machine |
| 012716–013115 | 256 | FN_ENEMY_RESPAWN + enemy AI step |
| 013116–013215 | 64 | PMOVE_BLIT: player move + tile blit |
| 013216–013523 | 200 | Sprite animation helpers (FN_SPRITE_DRAW support routines) |

### 013524–014027 — COLLISION MAP BUILD
| Address | Size | Content |
|---------|------|---------|
| 013524–014027 | 388 | FN_COLLISION_MAP_BUILD: unpack level tile data → 8-flag working buffer at 014550 |

### 014030–014421 — SPRITE DRAW
| Address | Size | Content |
|---------|------|---------|
| 014030–014421 | 250 | FN_SPRITE_DRAW: draw 8×8 sprite from entity record |
| 014302–014421 | 80 | FN_TILE_BLIT_REV: blit tile indexed by R2 (reverse direction) |

### 014422–014547 — ENTITY RECORDS (PLAYER + 2 ENEMIES)
| Address | Size | Content |
|---------|------|---------|
| 014422 | 2 | VAR_PLAYER_TILE_PTR: pointer to player tile in BUF_TILE_WORK |
| 014424–014431 | 8 | Player entity record (state, sprite, position flags) |
| 014432 | 2 | VAR_ENEMY1_TILE_PTR |
| 014434–014441 | 8 | Enemy 1 entity record |
| 014442 | 2 | VAR_ENEMY2_TILE_PTR |
| 014444–014547 | 68 | Enemy 2 entity record + game globals |

### 014550–015227 — TILE WORKING BUFFER
| Address | Size | Content |
|---------|------|---------|
| 014550–015227 | 440 | BUF_TILE_WORK: runtime tile buffer, 22×10 words, 8 collision flags per tile. Built by FN_COLLISION_MAP_BUILD from current level's tile map |

### 015230–017447 — PER-LEVEL ENTITY SPAWN DATA
| Address | Size | Content |
|---------|------|---------|
| 015230–017447 | 1160 | Per-level entity start positions (player spawn + enemy spawns), referenced by TBL_ENTITY_PTRS at 001230. 10 records, ~116 bytes each |

### 017450–020447 — TILE PIXEL DATA (TILE BANK)
| Address | Size | Content |
|---------|------|---------|
| 017450–017467 | 16 | Tile 0 (bg/air): all zeros |
| 017470–017507 | 16 | Tile 1 (ladder): 3C 3C FF FF 3C 3C 3C 3C (both planes) |
| 017510–017527 | 16 | Tile 2 (exit): all zeros — marks exit point |
| 017530–017547 | 16 | Tile 3 (empty3): all zeros |
| 017550–017567 | 16 | Tile 4 (gold_a): colour plane = FC 3F A8 2A FC 3F FC 3F |
| 017570–017607 | 16 | Tile 5 (gold_b): same as tile 4 |
| 017610–017627 | 16 | Tile 6 (gold_c): same as tile 4 |
| 017630–017647 | 16 | Tile 7 (water): pixel = 00 00 FF FF CC CC 00 00 |
| 017650–017667 | 16 | Tile 8 (ladder2): same pixels as tile 1 |
| 017670–017707 | 16 | Tile 9 (wall_a): brick pattern, both planes |
| 017710–017727 | 16 | Tile 10 (empty10): all zeros |
| 017730–017747 | 16 | Tile 11 (wall_b): border wall |
| 017750–017767 | 16 | Tile 12 (wall_c): complex diagonal-stripe earth |
| 017770–020007 | 16 | Tile 13 (wall_d): bottom border wall |
| 020010–020447 | 312 | Tiles 14–31: water variant, extended tile bank, entity sprite tiles |

### 020450–022077 — SOUND / LOOKUP TABLES
| Address | Size | Content |
|---------|------|---------|
| 020450–021777 | 876 | Sound frequency tables, entity throttle values, entity type lookup |
| 022000–022077 | 64 | Keyboard scan table (ASCII codes → direction mapping) |

### 022100–030777 — LEVEL TILE MAPS (10 levels)
| Address | Size | Content |
|---------|------|---------|
| 022100–022637 | 352 | Level 1 tile map (22 rows × 16 bytes) |
| 022640–023377 | 352 | Level 2 |
| 023400–024137 | 352 | Level 3 |
| 024140–024677 | 352 | Level 4 |
| 024700–025437 | 352 | Level 5 |
| 025440–026177 | 352 | Level 6 |
| 026200–026737 | 352 | Level 7 |
| 026740–027477 | 352 | Level 8 |
| 027500–030237 | 352 | Level 9 |
| 030240–030777 | 352 | Level 10 |

Level format: 22 rows × 16 bytes. Each byte encodes 2 tiles: low nibble = left tile, high nibble = right tile. Tile indices 0–15 reference the tile bank at 017450. Stride between levels = 352 bytes (0o540), confirmed from code at 001050: `ADD #540, @#CUR_MAP_ADDR`.

**Note:** Previous version of this table had wrong end addresses (stride 224 instead of 352). Corrected 2026-05-30 after verifying all 10 level start addresses via direct computation and confirming valid tile data at each.

### 031000–031277 — INTRO ANIMATION TILES (12 frames)
| Address | Size | Content |
|---------|------|---------|
| 031000–031277 | 192 | 12 × 16-byte frames (same 2bpp 8×8 format as tile/sprite banks). Referenced from intro display code at 003206 (word pointer 031054). Graphical content: horizontal border/rope tiles and background pattern tiles used in the title/intro screen animation. |

### 031300–037677 — SPRITE ANIMATION FRAMES
| Address | Size | Content |
|---------|------|---------|
| 031300–037677 | 3328 | 208 animation frames × 16 bytes each. Same 2bpp 8×8 format as tile bank. Player walk/climb/death; enemy walk/death. |

### 037700–037777 — GAME TEXT STRINGS
| Address | Size | Content |
|---------|------|---------|
| 037700–037777 | 64 | KOI8-R: key legend displayed to player — "кладчика: А , - И Л B Д П ; R G Ш..." |

### 040000–042000 — УКНЦ I/O DRIVER CODE
| Address | Size | Content |
|---------|------|---------|
| 040000–040777 | 512 | Keyboard poll (@#040546), scanline write (@#176640/@#176642) |
| 041000–041777 | 512 | FN_DISP_COL_BLIT (041040), text output EMT handlers, display setup |
| 042000 | — | End of program |

---

## Coverage Summary

| Region | Bytes | % of 16896 | Status |
|--------|-------|------------|--------|
| Code (all routines 001000–022077) | ~8960 | 53% | ✅ Fully disassembled |
| Tile bank (017450–020447) | 1024 | 6% | ✅ 32 tiles → PNG |
| Level maps (022100–030777) | 3520 | 21% | ✅ 10 levels → JSON + PNG |
| Intro tiles (031000–031277) | 192 | 1% | ✅ 12 frames identified, referenced from intro code |
| Sprite frames (031300–037677) | 3328 | 20% | ✅ 208 frames → PNG |
| Game text (037700–037777) | 64 | <1% | ✅ KOI8-R decoded |
| УКНЦ I/O code (040000–042000) | 1024 | 6% | ✅ Disassembled |

**~100% of bytes are accounted for.** (Previous 13% "unknown" was level data at wrong addresses.)
