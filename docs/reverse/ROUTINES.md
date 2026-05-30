# Rutine identificate — Crocodile КЛАД (unpacked)

Sursa: `disassembly/annotated/crocodile_klad.asm` + `/tmp/emu2_dump.bin`  
Toate adresele sunt **octale** din RAM-ul decomprimat (org=0).

## Index rutine

| Adresă | Nume | Apeluri | Descriere scurtă |
|--------|------|---------|-----------------|
| `001000` | `RESTART` | restart | JMP @#GAME_INIT — trampoline de restart complet |
| `001004` | `GAME_OVER_SOFT` | 1 | Reinit parțial (CLR R0 + JMP 004160) |
| `001016` | `PLAYER_DEATH` | 1 | Moartea jucătorului — apelează animație, resetează |
| `001034` | `LEVEL_COMPLETE` | 1 | Nivel terminat — avansează pointer în tabela de niveluri |
| `001112` | *(reload)* | 1 | Toate nivelurile terminate → JMP RESTART |
| `001142` | `KEY_DIFFICULTY` | 1 | Procesează taste 1-4 → setează viteza @#001312 |
| `001306` | `DELAY_SPIN` | inline | Busy-wait (burn cycles, R5 iterații) |
| `001344` | `GAME_LOOP` | — | Entry per-frame → JMP 004674 |
| `001436` | `ACT_DISPATCH` | 1 | Dispatch acțiune tastatură (R0=adresă acțiune, R4→player) |
| `002072` | `TITLE_SEQ` | 1 | Secvența title screen: randare tile-uri + text + wait |
| `002264` | `TITLE_WAIT` | — | Loop blocat pe EMT 6, așteaptă LF (012 octal) |
| `003234` | `DIFF_SELECT` | 1 | Selectare dificultate: taste '1'-'4', setează @#001312 |
| `004000` | `GAME_INIT` | — | Cold start: vieți=5, scor=0, init display, JMP 005754 |
| `004210` | `NUM_RENDER` | 7 | Randează număr zecimal pe ecran (scor/vieți) |
| `004674` | `KBD_GAME_POLL` | — | Poll keyboard gameplay: citește @#177714, rotește biți |
| `004776` | `LEVEL_RENDER` | 1 | Randează tile map complet (22 rânduri × 16 bytes, 2 tiles/byte) |
| `005106` | `TILE_BLIT_FWD` | 2 | Blit tile forward: 8 cuvinte → framebuffer la 046000(R1), pas=100 |
| `005754` | `HW_INIT` | 1 | Init hardware: scroll=001330, EMT-uri video, JMP 002072 |
| `006204` | `ENTITY_HANDLER` | 6 | State machine entitate: mișcare, coliziune, animație |
| `007432` | *(movement)* | 2 | Rutina de mișcare jucător (TBD) |
| `010206` | *(death anim)* | 1 | Animație moarte jucător (TBD) |
| `012326` | `VSYNC_WAIT` | 6 | Vsync: MOV #7,R0 / EMT 016 / RTS |
| `012442` | *(load level)* | 2 | Încarcă date nivel următor în RAM (TBD) |
| `013524` | *(level trans)* | 2 | Tranziție între niveluri (TBD) |
| `014030` | `SPRITE_DRAW` | 7 | Desenează sprite 2-tile din entity record |
| `014302` | `TILE_BLIT_REV` | 8 | Blit tile cu index din R2: × 16 + bank 017450 → 046000(R1) |

## Variabile globale (RAM)

| Adresă | Nume | Tip | Valoare inițială | Descriere |
|--------|------|-----|-----------------|-----------|
| `017430` | `GAME_STATE` | word | 010404 | Stare globală joc |
| `017436` | `LIVES` | word | 5 | Număr vieți rămase |
| `017440` | `SCORE` | word | 0 | Scor acumulat |
| `001300` | `CUR_MAP_ADDR` | ptr | — | Adresă curentă în harta de nivel |
| `001302` | `CUR_LEVEL_PTR` | ptr | — | Pointer curent în tabela de niveluri |
| `001304` | `LEVEL_TBL_PTR` | ptr | — | Entry curent în tabela de niveluri |
| `001312` | `SPEED` | word | 1000 | Viteza jocului (400/1000/2000/4000) |

## Constante hardware BK-0010

| Adresă | Registru | Utilizare |
|--------|----------|-----------|
| `177662` | Keyboard data | Citit în meniuri (poll direct) |
| `177664` | Scroll | Setat la 001330 = offset ecran |
| `177714` | Keyboard shift-reg | Citit în gameplay (biți per tastă) |
| `177716` | System reg | Bit 6 = gate vsync în game loop |

## Constante grafice

| Valoare | Semnificație |
|---------|-------------|
| `017450` | Tile bank start (16 tiles × 16 bytes) |
| `046000` | Framebuffer playfield start (rândul 6 = 6×8 scanlines deasupra) |
| `100` octal | Stride scanline = 64 dec bytes = 32 cuvinte × 2 bytes |
| `1000` octal | Stride rând tile = 512 dec bytes = 8 scanlines × 64 bytes |
| `010` octal | Tile height = 8 scanlines |

## Structura entity record (la `014420` = player)

```
offset +0:  word  tip/flags  (bit 12 = activ, bit 10:8 = stare, bit 7:0 = sub-tip)
offset +2:  word  X position
offset +4:  word  Y position  
offset +6:  word  sprite index
offset +10: word  animation frame / direcție
... (câmpuri suplimentare TBD din 006204)
```

## Ce mai rămâne (Obiectiv 2 în progres)

- [ ] `007432` — rutina de mișcare completă (JMP-uri din ACT_DISPATCH)
- [ ] `010206` — animație moarte
- [ ] `012442` — loader nivel
- [ ] `013524` — tranziție nivel
- [ ] Tabela de taste `012342` — mapare completă bit → acțiune
- [ ] Rutine inamici (entitățile de la 014422+)
- [ ] Logica de coliziune (în ENTITY_HANDLER 006204)
