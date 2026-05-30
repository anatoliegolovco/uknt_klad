# ANIMATIONS — КЛАД 1987 Баранов (УКНЦ МС-0511)

Documentat din assembler. Adrese octale.

---

## Arhitectura sistemului de animație

Jocul are **două straturi de animație** cu logici distincte:

1. **Throttle (ritm)** — câte game-ticks se scurg între avansarea unui frame
2. **State machine** — ce frame se afișează în funcție de starea entității (mers, urcat, mort)

Sprite-ul este blit-uit la ecran via `FN_SPRITE_DRAW` (014030), care apelează `TILE_BLIT_REV` (014302 sau 014354).

---

## Player — Animație

### Throttle

`FN_ANIM_THROTTLE_PLAYER` (007432), contor `VAR_PLAYER_ANIM_CTR` (017374):

```asm
007432: CMP  #3, @#017374     ; src=3, dst=counter → flags de la (3 - counter)
007440: BGE  7454             ; dacă 3 >= counter (counter ≤ 3): INC
007442: JSR  PC, @#014030     ; dacă counter > 3: SPRITE_DRAW
007446: CLR  @#017374         ; reset counter = 0
007454: INC  @#017374         ; increment
```

**Comportament:** counter merge 0→1→2→3→4, la 4 (>3): draw + reset.  
**Ritm:** player frame avansează la **fiecare 5 game-ticks**.

Apelat din `ACT_DISPATCH` (001472) o dată per frame, indiferent de tastă apăsată.

### State machine

`SPRITE_HELPERS` (013216) determină frame-ul afișat bazat pe starea entității:

| State (octal) | Valoare dec | Trigger | Frame afișat |
|---------------|-------------|---------|-------------|
| `014` | 12 | Detectat în rutina de verificare ladder | Branch special → `MOVB #21` |
| `021` | 17 | `MOVB #21, (R5)` la 013472 | **Urcat scară (climb frame A)** |
| `023` | 19 | `MOVB #23, (R5)` la 013516 | **Mers normal (walk frame)** |

Logica de selecție (013462):
```asm
013462: CMP  #12, 16(R4)      ; counter ladder == 12?
013470: BNE  13516            ; dacă nu: state = 023 (walk)
013472: MOVB #21, (R5)        ; dacă da:  state = 021 (climb)
```

Range valid animație: state ∈ {021, 022, 023, 024} (octal) = {17, 18, 19, 20 dec). Valori în afara acestui range returnează fără animație.

### Frame lookup (012410 table)

`SPRITE_HELPERS` calculează adresa frame-ului:
```
frame_ptr = (state - 021) × 2 + direction_offset + 012410
tile_index = BYTE @frame_ptr
tile_data  = tile_index × 16 + 017450
```

Tabelul la 012410 conține **indici de tile** (0–31) care referențiază banca de tile-uri la 017450. Tile-urile 16–31 (adrese 020050–020450) sunt sprite-uri de personaje.

---

## Inamici — Animație

Jocul are **3 inamici** cu throttle-uri și contoare separate.

### Enemy 1 (ENEMY1_TICK la 007376)

Combinat: throttle de **mutare** și animație în aceeași rutină.

```asm
007376: CMP  #4, @#017372     ; counter enemy1 anim
007404: BGE  7424             ; dacă counter ≤ 4: INC
007406: JSR  PC, @#012740     ; dacă counter > 4: PLAYER_MOVE_STEP (mișcă entitatea)
007412: JSR  PC, @#012024     ; + handler adițional
007416: CLR  @#017372         ; reset
007424: INC  @#017372
```

**Ritm mutare:** o mutare la fiecare **5 game-ticks**.  
Animația (frame-ul sprite) se actualizează simultan cu mutarea — nu are throttle separat de animație.

### Enemy 2 — SPRITE_ANIM_C + SPRITE_ANIM_D (007202, 007242)

**Mișcare orizontală** (`SPRITE_ANIM_C`, 007202), contor `VAR_ENEMY2_ANIM_A` (017366):
```asm
007212: CMPB #7, @#17366      ; compare 7 with counter
007220: BGE  7234             ; dacă counter ≤ 7: INC
007222: JSR  PC, @#014030     ; dacă counter > 7 (= 8): SPRITE_DRAW + CLEAR
```
**Ritm:** frame avansează la **fiecare 9 game-ticks** (counter 0→8→draw).

**Mișcare verticală** (`SPRITE_ANIM_D`, 007242), contor `VAR_ENEMY2_ANIM_B` (017370):
```asm
007256: CMPB #4, @#17370      ; compare 4 with counter
007264: BGE  7300             ; dacă counter ≤ 4: INC
007266: JSR  PC, @#014030     ; dacă counter > 4 (= 5): SPRITE_DRAW + CLEAR
```
**Ritm:** frame avansează la **fiecare 6 game-ticks** (counter 0→5→draw).

### Enemy 3 — SPRITE_ANIM_A + SPRITE_ANIM_B (010102, 010142)

**Mișcare orizontală** (`SPRITE_ANIM_A`, 010102), contor `VAR_ENEMY3_ANIM_A` (017362):  
- Threshold: `CMPB #7` → draw la counter = 8 → **9 game-ticks/frame**

**Mișcare verticală** (`SPRITE_ANIM_B`, 010142), contor `VAR_ENEMY3_ANIM_B` (017364):  
- Threshold: `CMPB #4` → draw la counter = 5 → **6 game-ticks/frame**

Aceeași logică ca Enemy 2. Workspace sprite: R2 = 021760 (enemy3) vs 021640 (player).

---

## Tabela de poziționare animație (TBL_ANIM_FRAMES)

`TBL_ANIM_FRAMES` (020270) mapează starea animației → offset pixel pentru blit.  
Indexat cu `state × 4` (byte offset), fiecare intrare = 3 words:

| Word | Offset | Conținut |
|------|--------|---------|
| 0 | +0 | Mască coliziune / flag bit |
| 1 | +2 | Delta X pixeli (adăugat la screen_X) |
| 2 | +4 | Delta Y pixeli (adăugat la screen_Y) |

Folosit în SPRITE_DRAW (014030):
```asm
014050: ASL R3 / ASL R3       — state × 4
014054: ADD #20270, R3        — R3 → entry
014060: ADD 2(R3), R2         — screen_X += delta_X
014064: ADD 4(R3), R1         — screen_Y += delta_Y
```

Primele 6 intrări (state 0-5):

| State | Mask | ΔX | ΔY |
|-------|------|----|----|
| 0 | 0o100000 | +2 | +2 |
| 1 | 0o040000 | −2 | −2 |
| 2 | 0o020000 | −64 | −256 |
| 3 | 0o010000 | +64 | +256 |
| 4 | 0o002000 | +64 | +256 |
| 5 | 0o005000 | +2560 | +2560 |

---

## Resetare contoare animație

`FN_ENTITY_STATE_INIT` (006444) la fiecare moarte/reset nivel:
```asm
006444: CLR @#017360     — VAR_ENEMY3_TICK_CTR (move counter enemy3)
006450: CLR @#017376     — VAR_ENEMY2_TICK_CTR (move counter enemy2)
```

Contoarele de animație (017362–017374) nu sunt resetate explicit de ENTITY_STATE_INIT — ele se resetează prin `CLRB` / `CLR` la fiecare draw ciclu.

---

## Sprite bank (031300–037677)

208 frames × 16 bytes (același format ca tile bank: 8 pixel-plane + 8 colour-plane, 8×8px 2bpp).

Accesate via **word pointer** (nu tile index byte) prin varianta `TILE_BLIT_REV` la 014354:
```asm
014370: MOV  (R2), R3    — WORD read (adresă directă, nu index)
014372: JSR  PC, @#041040 — DISP_COL_BLIT
```

**Folosire:** cel mai probabil pentru intro/titlu sau pentru sprite-uri mai complexe care depășesc range-ul 0–31 din banca principală. Patternele observate (BB 08 la top, B8 BB la bottom = siluetă personaj cu cap și picioare) sugerează frame-uri suplimentare de animație pentru player/inamici sau secvența de titlu.

**Notă:** Legătura exactă între frame-urile 031300+ și stările animate nu a fost complet trasată — necesită trace emulator (breakpoint pe 014354, watch R3) pentru confirmare.

---

## Rezumat timing

| Entitate | Mișcare (game-ticks/pas) | Animație (game-ticks/frame) |
|----------|--------------------------|------------------------------|
| Player | Imediat (per input) | 5 ticks/frame |
| Enemy 1 | 5 ticks/pas | Sincron cu mutarea |
| Enemy 2 horizontal | ~256 ticks/pas | 9 ticks/frame |
| Enemy 2 vertical | ~256 ticks/pas | 6 ticks/frame |
| Enemy 3 horizontal | ~256 ticks/pas | 9 ticks/frame |
| Enemy 3 vertical | ~256 ticks/pas | 6 ticks/frame |

La viteza maximă (difficulty 4, DELAY_SPIN_COUNT = 10000 octal), un game-tick ≈ 1 ciclu de bază al CPU УКНЦ (~1MHz efectiv per delay). Frecvența exactă în Hz depinde de clock-ul УКНЦ și de DELAY_SPIN_COUNT.
