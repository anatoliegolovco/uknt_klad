# LEVEL_FORMAT — КЛАД 1987 Баранов (УКНЦ МС-0511)

Documentat din assembler. Adrese octale.

---

## Locație în fișier

**Fișier:** `assets/uknc/KLAD_1987_Baranov.SAV`  
**RT-11 header:** 512 bytes (offset 0x000–0x1FF)  
**Program image:** loaded la 001000 (octal) = 512 dec → file offset = address decimal

| Nivel | Adresă (octal) | Offset fișier (dec) | Etichetă ASM |
|-------|----------------|---------------------|--------------|
| 1     | 022100         | 9280                | TBL_LEVEL_MAP_1 |
| 2     | 022640         | 9632                | TBL_LEVEL_MAP_2 |
| 3     | 023400         | 9984                | TBL_LEVEL_MAP_3 |
| 4     | 024140         | 10336               | TBL_LEVEL_MAP_4 |
| 5     | 024700         | 10688               | TBL_LEVEL_MAP_5 |
| 6     | 025440         | 11040               | TBL_LEVEL_MAP_6 |
| 7     | 026200         | 11392               | TBL_LEVEL_MAP_7 |
| 8     | 026740         | 11744               | TBL_LEVEL_MAP_8 |
| 9     | 027500         | 12096               | TBL_LEVEL_MAP_9 |
| 10    | 030240         | 12448               | TBL_LEVEL_MAP_10 |

**Stride între niveluri:** 352 bytes = 0o540  
Confirmat din cod la 001050: `ADD #540, @#VAR_CUR_MAP_ADDR` (LEVEL_COMPLETE).

---

## Dimensiuni nivel

| Parametru | Valoare | Note |
|-----------|---------|------|
| Coloane tile | 32 | 16 bytes × 2 tiles/byte |
| Rânduri tile | 22 | 22 rows × 16 bytes |
| Bytes per nivel | 352 | 22 × 16 |
| Tile index range | 0–15 | 4 biți per tile (nibble) |

Vizualizat ca grid: **32 coloane × 22 rânduri** de tile-uri 8×8 px = 256×176 px per nivel (lăsat 16 px pentru HUD la bas).

---

## Encoding per byte

Fiecare byte din tile map conține **2 tile-uri** stivuite:

```
byte = (high_nibble << 4) | low_nibble

tile la coloana 2k   = byte & 0x0F    (nibble low)
tile la coloana 2k+1 = (byte >> 4)    (nibble high)
```

Exemplu: byte `0xB9` = tile 9 (left) + tile 11/B (right).

Confirmat din codul de level render (LRND_BYTE la 005016):
```asm
005016: MOVB (R3)+, R2     ; R2 = byte din tile map
005022: BIC  #177760, R0   ; R0 = low nibble (tile stânga)
005026: ASL  R2 / ... / ASL R2 / ASL R2 / ASL R2  ; shift → tile dreapta
```

---

## Tabel tile (indici 0–15)

| Index | Hex | Adresă ASM | Tip | Solid |
|-------|-----|------------|-----|-------|
| 0     | 0x0 | DAT_TILE_0  (017450) | Aer / fond negru | Nu |
| 1     | 0x1 | DAT_TILE_1  (017470) | Scară (ladder) | Nu |
| 2     | 0x2 | DAT_TILE_2  (017510) | Ieșire nivel (exit) — **16 octeți = 0** (invizibil ca tile static; apare doar prin sprite/logica de final) | Nu |
| 3     | 0x3 | DAT_TILE_3  (017530) | Rezervat / gol | Nu |
| 4     | 0x4 | DAT_TILE_4  (017550) | Aur A → +scor (`SCORE_ADD`) | Nu |
| 5     | 0x5 | DAT_TILE_5  (017570) | Aur B → viață bonus (`BONUS_LIFE_ADD`) | Nu |
| 6     | 0x6 | DAT_TILE_6  (017610) | Aur C → trigger final nivel (`LEVEL_COMPLETE`) | Nu |

> **⚠ Corectură (2026-05-31):** tiles 4/5/6 au **octeți IDENTICI** (`00×8 FC 3F A8 2A FC 3F FC 3F`)
> — deci NU sunt 3 frame-uri de animație grafică (eticheta veche "animat" era o presupunere).
> Sunt 3 colectabile cu același desen (cufăr) dar **efecte diferite** la atingere, distinse prin
> INDEXUL tile-ului în hartă, nu prin grafică: 4=scor, 5=viață, 6=final nivel.
| 7     | 0x7 | DAT_TILE_7  (017630) | Apă (water, fatal) | Nu* |
| 8     | 0x8 | DAT_TILE_8  (017650) | Scară 2 (ladder variant) | Nu |
| 9     | 0x9 | DAT_TILE_9  (017670) | Zid A (brick) | **Da** |
| 10    | 0xA | DAT_TILE_10 (017710) | Rezervat | Nu |
| 11    | 0xB | DAT_TILE_11 (017730) | Zid B (border wall) | **Da** |
| 12    | 0xC | DAT_TILE_12 (017750) | Zid C (earth diagonal) | **Da** |
| 13    | 0xD | DAT_TILE_13 (017770) | Zid D (bottom border) | **Da** |
| 14    | 0xE | DAT_TILE_14 (020010) | Apă variant | Nu* |
| 15    | 0xF | DAT_TILE_15 (020030) | Tile extins | — |

*Apa: nu e solid (playerul cade prin ea), dar declanșează PLAYER_DEATH la contact.

**Prag soliditate (din COLLISION_MAP_BUILD):** tile index ≥ 8 (dec) = solid. Tiles 0–7 = passable.

---

## Layout rânduri

Datele sunt stocate **rând cu rând**, de sus în jos:

```
offset 0:   row 0  (rândul de sus,  16 bytes = 32 tile-uri)
offset 16:  row 1
...
offset 336: row 21 (rândul de jos)
```

Tile (col, row) → byte index = `row * 16 + col // 2`, nibble = `col % 2`.

---

## Acces din cod

**VAR_CUR_MAP_ADDR** (001300): pointer la nivelul curent, inițializat la 022100 (Level 1).

```asm
; LEVEL_RENDER (004776): iterează tile map
004776: MOV @#001300, R3   ; R3 → tile map curent
...
; LEVEL_COMPLETE (001034): avansează la nivel următor
001050: ADD #540, @#001300 ; += 352 bytes
```

**VAR_CUR_LEVEL_PTR** (001302): pointer la entity record al nivelului curent.  
**TBL_ENTITY_PTRS** (001230): 10 words, câte unul per nivel → adrese în 015230–017447.

---

## Poziții de start entități

Entitățile per nivel (player spawn + enemy spawns) sunt stocate separat față de tile map, în blocul **PER_LEVEL_ENTITY_DATA** (015230–017447): 10 records × ~116 bytes.

Câmpuri per record (ordine aproximativă din cod):
- Player X, Y (tile coords)
- Enemy 1 X, Y + stare inițială
- Enemy 2 X, Y + stare inițială
- Enemy 3 X, Y + stare inițială

Pointer la recordul curent: **VAR_CUR_LEVEL_PTR** (001302).

---

## Fișiere extrase

| Fișier | Conținut |
|--------|---------|
| `assets/uknc/levels/level_NN.json` | Tile map JSON (22×32), adresă, metadate |
| `assets/uknc/levels/level_NN.png` | Vizualizare PNG (color-coded, SCALE=8) |
| `tools/extract_uknc_gfx.py` | Script de extracție (secțiunea LEVELS) |

JSON format:
```json
{
  "level": 1,
  "address": "0o22100",
  "rows": 22,
  "cols": 32,
  "tiles": [[0, 11, 11, ...], ...]
}
```
