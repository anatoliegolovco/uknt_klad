# BYTE MAP — Crocodile КЛАД (KLAD.BIN, BK-0010)

Fișier: `assets/original/ex_klad/KLAD.BIN`  
SHA-256: (verificat la download)  
Dimensiune totală: **10433 bytes** (0o24301 octal)  
Format: **BK-0010 .BIN** — 4-byte header + payload binar

Toate adresele și valorile sunt **octale** cu excepția celor marcate [dec] sau [hex].

---

## SECȚIUNEA 1 — FIȘIERUL KLAD.BIN (packed binary)

### 1.1 Header BK .BIN (bytes 0–3)

| Offset fișier | Dimensiune | Tip | Conținut |
|---------------|------------|-----|---------|
| 0–1 | 2 bytes | HEADER | Load address = `0o000732` (474 dec) |
| 2–3 | 2 bytes | HEADER | Length = `0o024275` (10429 dec) |

### 1.2 Padding (bytes 4–41 = adrese 0o0732–0o0776)

| Offset fișier | Adresă RAM | Dimensiune | Tip | Conținut |
|---------------|------------|------------|-----|---------|
| 4–41 | 0o0732–0o0776 | 38 bytes (19 cuvinte) | PAD | Toate cuvintele = `001000` (instrucțiunea `BNE +0` = NOP echivalent). Umple golul între load address 0732 și entry point 01000. |

### 1.3 Depacker Stage 1 (bytes 42–113 = adrese 0o1000–0o1106)

| Offset fișier | Adresă RAM | Dimensiune | Tip | Conținut |
|---------------|------------|------------|-----|---------|
| 42–113 | 0o1000–0o1106 | 72 bytes (36 cuvinte) | CODE | Rutina de decompresie stage 1 — backward-LZ. Intrare la 0o1000. Citește biți dinspre 0o1237 în jos; citește pachete de back-reference dinspre 0o1236 în sus; scrie output backward din 0o100000 spre 0o77000. Ultimă instrucțiune: `JMP (R3)` la adresa 0o1106. |

**Instrucțiuni stage 1 (adrese RAM):**
- `001000`: `MOV PC, R4` — R4 = 001002
- `001002`: `ADD #0236, R4` — R4 = 001240 (start date comprimate)
- `001006`: `MOV R4, R0` — R0 = R4 = 001240 (cursor citire înapoi)
- `001010`: `MOV #100000, R3` — R3 = 077000 + 01000 (cursor scriere, merge înapoi)
- `001014`: `CLR R1` — acumulator LZ
- `001016`: `MOV #20, R2` — 16 dec biți per cuvânt
- `001022..001063`: **DEPACK1_LOOP** — bucla LZ (TST -(R0), ROL, BCC, MOVB, SUB, SWAB)
- `001064..001075`: **DEPACK1_COPY2** — copie 28 cuvinte de cod stage 2 în 077000
- `001076..001105`: `ADD #24117, R0; MOV #552, R2` — setare cursor payload principal
- `001106`: `JMP (R3)` — salt la stage 2 la adresa 077000

### 1.4 Date cod Stage 2 (bytes 114–199 = adrese 0o1107–0o1237)

| Offset fișier | Adresă RAM | Dimensiune | Tip | Conținut |
|---------------|------------|------------|-----|---------|
| 114–199 | 0o1107–0o1237 | 86 bytes (43 cuvinte) | DATA | Cod stage 2 decomprimat și comprimat staggered. Stage 1 citește BACKWARD 28 cuvinte din această regiune și le scrie la 077000. Restul = overhead format LZ. |

### 1.5 Payload principal LZ-comprimat (bytes 200–10432 = adrese 0o1240–0o25227)

| Offset fișier | Adresă RAM | Dimensiune | Tip | Conținut |
|---------------|------------|------------|-----|---------|
| 200–10432 | 0o1240–0o25227 | 10233 bytes | LZ | Payload principal al jocului comprimat cu LZ backward. Stage 2 (la 077000) decomprimă această regiune în adresele 001000–076710. Conține tot codul jocului, toate datele, tile-uri și niveluri. |

---

## SECȚIUNEA 2 — RAM DECOMPRIMATĂ (emu2_dump.bin, 64KB flat)

Adresele de mai jos sunt în **RAM-ul BK-0010 după decomprimare**.  
Sursa: `disassembly/annotated/_unpacked_raw.asm` + analiză disassembly.

### 2.1 Vectori de întrerupere sistem (0o000000–0o000777)

| Adrese RAM | Dimensiune | Tip | Conținut |
|------------|------------|-----|---------|
| 0o000000–0o000003 | 4 bytes | SYS | Vector bus-error / illegal instruction (scris la 004116) |
| 0o000004–0o000777 | ~510 bytes | SYS | BK-0010 sistem: vectori trap, ROM workspace. Nu sunt modificate de joc. |

### 2.2 Cod joc (0o001000–0o001343)

| Adrese RAM | Dimensiune | Tip | Rutina | Descriere |
|------------|------------|-----|--------|-----------|
| 0o001000–0o001003 | 4 bytes | CODE | `RESTART` | JMP @#GAME_INIT — trampoline restart complet |
| 0o001004–0o001015 | 10 bytes | CODE | `GAME_OVER_SOFT` | Reinit parțial + trampolines game over |
| 0o001016–0o001033 | 14 bytes | CODE | `PLAYER_DEATH` | Cheamă animație moarte, reset |
| 0o001034–0o001111 | 54 bytes | CODE | `LEVEL_COMPLETE` | Avansează pointer în tabela de niveluri |
| 0o001112–0o001113 | 2 bytes | CODE | *(reload)* | JMP RESTART (toate nivelurile terminate) |
| 0o001116–0o001141 | 18 bytes | CODE | *(game state init)* | MOV #010404, @#17430 + JMP 001014 |
| 0o001142–0o001227 | 54 bytes | CODE | `KEY_DIFFICULTY` | Taste 1-4 → setare viteză @#001312 |

### 2.3 Date: tabele pointer nivel + variabile (0o001230–0o001343)

| Adrese RAM | Dimensiune | Tip | Conținut |
|------------|------------|-----|---------|
| 0o001230–0o001277 | 40 bytes (20 cuvinte) | DATA | **Level pointer table**: 10 intrări × 2 bytes, fiecare = adresa unui bloc hartă de 352 bytes |
| 0o001300–0o001301 | 2 bytes | DATA | `CUR_MAP_ADDR` — adresa start hartă nivel curent |
| 0o001302–0o001303 | 2 bytes | DATA | `CUR_LEVEL_PTR` — pointer curent în tabela niveluri |
| 0o001304–0o001305 | 2 bytes | DATA | `LEVEL_TBL_PTR` — entry curent tabela niveluri |
| 0o001306–0o001311 | 4 bytes | CODE | `DELAY_SPIN` — busy-wait (burn cycles) |
| 0o001312–0o001313 | 2 bytes | DATA | `SPEED` — viteza jocului (400/1000/2000/4000) |

### 2.4 Game loop principal (0o001344–0o001771)

| Adrese RAM | Dimensiune | Tip | Rutina | Descriere |
|------------|------------|-----|--------|-----------|
| 0o001344–0o001353 | 8 bytes | CODE | `GAME_LOOP` | Entry per-frame → JMP @#004674 |
| 0o001354–0o001435 | 54 bytes | CODE | `GAME_LOOP_MENU` | Poll keyboard meniu + scan tabelă taste |
| 0o001436–0o001601 | 90 bytes | CODE | `ACT_DISPATCH` | Dispatch acțiune: entitate player + apel mișcare |
| 0o001602–0o001671 | 54 bytes | CODE | `GAME_TICK` | Secvența per-frame: delay → player → inamici → coliziune |
| 0o001672–0o001731 | 40 bytes | DATA | *(padding)* | Cuvinte HALT — neutilizate (spațiu rezervat) |
| 0o001732–0o001777 | 38 bytes | DATA | `KEY_CODE_TBL` | Coduri taste pentru keyboard meniu (12 intrări) |

### 2.5 Secvența title screen + menu (0o002000–0o003233)

| Adrese RAM | Dimensiune | Tip | Rutina | Descriere |
|------------|------------|-----|--------|-----------|
| 0o002000–0o002049 | 32 bytes | CODE | *(game branches)* | Salt-uri game loop + verificare scor |
| 0o002050–0o002051 | 2 bytes | CODE | `SOUND_WRAPPER_B` | MOV #6156, R5 → JMP ENTITY_HANDLER |
| 0o002052–0o002071 | 14 bytes | CODE | `SOUND_WRAPPER_A` | MOV #6134, R5 → JSR ENTITY_HANDLER |
| 0o002072–0o002263 | 114 bytes | CODE | `TITLE_SEQ` | Randare title screen, text, afișare EMT |
| 0o002264–0o002327 | 44 bytes | CODE | `TITLE_WAIT` | EMT 006 loop, așteaptă Enter (LF=012) |
| 0o002330–0o003233 | 450 bytes | DATA | *(strings)* | Șiruri text (titlu, selectare dificultate, instrucțiuni) |

### 2.6 Meniu dificultate + game over (0o003234–0o003777)

| Adrese RAM | Dimensiune | Tip | Rutina | Descriere |
|------------|------------|-----|--------|-----------|
| 0o003234–0o003273 | 32 bytes | CODE | `DIFF_SELECT` | EMT 006 loop, taste '1'-'4' |
| 0o003274–0o003355 | 54 bytes | CODE | `GAME_OVER_WAIT` | Afișaj game over, wait Enter → RESTART |
| 0o003356–0o003575 | 144 bytes | CODE | *(HUD routines)* | LIVES_DISPLAY (003372), DEATH_SCORE (003444) |
| 0o003576–0o003651 | 44 bytes | CODE | `GAME_LEVEL_LOOP` | Loop nivel: render + sound entity per frame |
| 0o003652–0o003745 | 60 bytes | CODE | `HUD_RENDER` | Randare scor + vieți în HUD |
| 0o003746–0o003763 | 14 bytes | CODE | `BONUS_LIFE_ADD` | +1 viață + vsync + redesenare |
| 0o003764–0o003777 | 12 bytes | CODE | `SCORE_ADD` | +10 scor + vsync |

### 2.7 Game init + randare numere (0o004000–0o004673)

| Adrese RAM | Dimensiune | Tip | Rutina | Descriere |
|------------|------------|-----|--------|-----------|
| 0o004000–0o004161 | 114 bytes | CODE | `GAME_INIT` | Cold start: vieți=5, scor=0 |
| 0o004162–0o004207 | 22 bytes | CODE | *(init tail)* | Reset SP, vector bus-error, JMP HW_INIT |
| 0o004210–0o004635 | 278 bytes | CODE | `NUM_RENDER` | Conversie număr → cifre zecimale pe ecran |
| 0o004640–0o004663 | 18 bytes | CODE | `PLAYER_DEATH_TRIGGER` | Setare flag-uri coliziune tile + JMP |
| 0o004664–0o004673 | 8 bytes | DATA | *(key data)* | Valori acțiuni tastatură (6, 12, 22 etc.) |

### 2.8 Keyboard poll + tile blit (0o004674–0o005155)

| Adrese RAM | Dimensiune | Tip | Rutina | Descriere |
|------------|------------|-----|--------|-----------|
| 0o004674–0o004775 | 68 bytes | CODE | `KBD_GAME_POLL` | Citire @#177714 shift-reg, 11 biți |
| 0o004776–0o005001 | 2 bytes | CODE | *(entry)* | Prima instrucțiune LEVEL_RENDER |
| 0o005002–0o005105 | 68 bytes | CODE | `LEVEL_RENDER_R4` | Randare 22×16 tile map din R4 |
| 0o005106–0o005145 | 32 bytes | CODE | `TILE_BLIT_FWD` | Blit 8×8 tile forward la framebuffer |
| 0o005146–0o005147 | 2 bytes | DATA | `SOUND_TIMING` | Valoare timing inter-notă pentru sound engine |

### 2.9 Rutine sunet + init hardware (0o005150–0o006203)

| Adrese RAM | Dimensiune | Tip | Rutina | Descriere |
|------------|------------|-----|--------|-----------|
| 0o005150–0o005753 | 390 bytes | CODE | *(misc)* | Alte rutine game loop, tile data helpers |
| 0o005754–0o006053 | 64 bytes | CODE | `HW_INIT` | Init scroll=001330, EMT-uri video, JMP TITLE_SEQ |
| 0o006054–0o006133 | 56 bytes | CODE | `PLAYER_SPRITE_INIT` | HUD_RENDER + SPRITE_DRAW × 2 |
| 0o006134–0o006155 | 18 bytes | DATA | `SOUND_TBL_A` | Tabelă entități sonore A (frecvențe/stări) |
| 0o006156–0o006203 | 26 bytes | DATA | `SOUND_TBL_B` | Tabelă entități sonore B (frecvențe/stări) |

### 2.10 Sound engine + enemy state machines (0o006204–0o007461)

| Adrese RAM | Dimensiune | Tip | Rutina | Descriere |
|------------|------------|-----|--------|-----------|
| 0o006204–0o006443 | 160 bytes | CODE | `ENTITY_HANDLER` | **Motor de sunet**: toggle @#177716 bit 7 cu burn loops |
| 0o006444–0o006461 | 14 bytes | CODE | `ENTITY_STATE_INIT` | Resetare contoare animație + init sprite |
| 0o006462–0o006551 | 56 bytes | CODE | `WATER_COLLISION` | Detectare tile letal (state=15), erase sprite |
| 0o006552–0o007135 | 246 bytes | CODE | `ENEMY2_TICK` | State machine inamic 2: throttle + mișcare spre player |
| 0o007136–0o007201 | 36 bytes | CODE | `ENEMY2_MOVE_UP` | Ramura mișcare sus inamic 2 |
| 0o007202–0o007241 | 32 bytes | CODE | `SPRITE_ANIM_C` | Throttle 8-frame inamic 2 (dreapta) |
| 0o007242–0o007305 | 36 bytes | CODE | `SPRITE_ANIM_D` | Throttle 5-frame inamic 2 (jos) |
| 0o007306–0o007375 | 56 bytes | CODE | `ENTITY1_RESTORE` | Erase + restore sprite inamic 1 la moarte |
| 0o007376–0o007431 | 28 bytes | CODE | `ENEMY1_TICK` | Throttle animație inamic 1 (counter @#17372) |
| 0o007432–0o007461 | 24 bytes | CODE | `ANIM_THROTTLE_PLAYER` | Throttle animație player (counter @#17374) |

### 2.11 Enemy 3 + level reset (0o007462–0o010403)

| Adrese RAM | Dimensiune | Tip | Rutina | Descriere |
|------------|------------|-----|--------|-----------|
| 0o007462–0o010035 | 236 bytes | CODE | `ENEMY3_TICK` | State machine inamic 3: throttle + mișcare spre player |
| 0o010036–0o010101 | 36 bytes | CODE | `ENEMY3_MOVE_UP` | Ramura mișcare sus inamic 3 |
| 0o010102–0o010141 | 32 bytes | CODE | `SPRITE_ANIM_A` | Throttle 8-frame inamic 3 (dreapta) |
| 0o010142–0o010205 | 36 bytes | CODE | `SPRITE_ANIM_B` | Throttle 5-frame inamic 3 (jos) |
| 0o010206–0o010403 | 126 bytes | CODE | `LEVEL_RESET` | Reset complet nivel: spawn, collision map, render, init |

### 2.12 Animație moarte + rutine misc (0o010404–0o012325)

| Adrese RAM | Dimensiune | Tip | Rutina | Descriere |
|------------|------------|-----|--------|-----------|
| 0o010404–0o012325 | ~978 bytes | CODE | *(death anim + misc)* | Animații moarte player, rutine auxiliare nedocumentate complet |

### 2.13 Rutine keyboard + tile blit (0o012326–0o012441)

| Adrese RAM | Dimensiune | Tip | Rutina | Descriere |
|------------|------------|-----|--------|-----------|
| 0o012326–0o012341 | 12 bytes | CODE | `VSYNC_WAIT` | MOV #7, R0 / EMT 016 / RTS |
| 0o012342–0o012441 | 64 bytes | DATA | `KEY_ACTION_TBL` | 12 coduri acțiuni pentru biți tastatură shift-reg |

### 2.14 Level render + state machines (0o012442–0o013523)

| Adrese RAM | Dimensiune | Tip | Rutina | Descriere |
|------------|------------|-----|--------|-----------|
| 0o012442–0o012527 | 54 bytes | CODE | `LEVEL_RENDER_FULL` | Randare working buffer @#14550 → framebuffer |
| 0o012530–0o012567 | 32 bytes | CODE | `TILE_BLIT_SUB` | Blit 8×8 tile (varianta 3) |
| 0o012570–0o012715 | 86 bytes | CODE | `PLAYER_STATE_CHECK` | State machine tile player: apă/aur/viață/exit nivel |
| 0o012716–0o012737 | 18 bytes | CODE | `ENEMY_RESPAWN` | Set enemy activ (state=11) + flag-uri vecini |
| 0o012740–0o013215 | 174 bytes | CODE | `PLAYER_MOVE_STEP` | Pas mișcare player: verificare coliziune + blit tile |
| 0o013216–0o013523 | 198 bytes | CODE | *(sprite + enemy helpers)* | Rutine auxiliare animație sprite + mișcare inamici |

### 2.15 Level collision map builder + sprite draw (0o013524–0o014417)

| Adrese RAM | Dimensiune | Tip | Rutina | Descriere |
|------------|------------|-----|--------|-----------|
| 0o013524–0o014027 | 196 bytes | CODE | `COLLISION_MAP_BUILD` | Unpack level data → @#14550 + flag-uri coliziune |
| 0o014030–0o014301 | 170 bytes | CODE | `SPRITE_DRAW` | Desenează sprite 2-tile din entity record |
| 0o014302–0o014417 | 78 bytes | CODE | `TILE_BLIT_REV` | Blit tile cu index din R2 (varianta 2) |

### 2.16 Date entități + working buffer (0o014420–0o017777)

| Adrese RAM | Dimensiune | Tip | Conținut |
|------------|------------|-----|---------|
| 0o014420–0o014421 | 2 bytes | DATA | `PLAYER_STATE` — cuvânt stare player |
| 0o014422–0o014423 | 2 bytes | DATA | `PLAYER_TILE_PTR` — pointer tile player în @#14550 |
| 0o014424–0o014425 | 2 bytes | DATA | `PLAYER_X` — coloana screen player (inițial 024) |
| 0o014426–0o014427 | 2 bytes | DATA | *(player misc)* |
| 0o014430–0o014431 | 2 bytes | DATA | `ENEMY1_STATE` — cuvânt stare inamic 1 |
| 0o014432–0o014433 | 2 bytes | DATA | `ENEMY1_TILE_PTR` — pointer tile inamic 1 |
| 0o014434–0o014435 | 2 bytes | DATA | `ENEMY1_X` — coloana screen inamic 1 (inițial 024) |
| 0o014436–0o014437 | 2 bytes | DATA | `ENEMY1_Y` — offset Y ecran inamic 1 |
| 0o014440–0o014441 | 2 bytes | DATA | `ENEMY2_STATE` |
| 0o014442–0o014443 | 2 bytes | DATA | `ENEMY2_TILE_PTR` |
| 0o014444–0o014445 | 2 bytes | DATA | `ENEMY2_X` (inițial 024) |
| 0o014446–0o014447 | 2 bytes | DATA | `ENEMY2_Y` |
| 0o014450–0o014547 | 64 bytes | DATA | *(entity misc fields)* |
| 0o014550–0o017425 | 1502 bytes | DATA | **Working tile buffer** — COLLISION_MAP_BUILD output: 22×32 tile entries, câte un cuvânt per tile cu nibble index + flag-uri coliziune |
| 0o017426–0o017427 | 2 bytes | DATA | `ENEMY1_SPAWN_PTR` — pointer spawn inamic 1 |
| 0o017430–0o017431 | 2 bytes | DATA | `GAME_STATE` — cuvânt stare globală (inițial 010404, folosit ca bază pointer table) |
| 0o017432–0o017433 | 2 bytes | DATA | *(game state field 2)* |
| 0o017434–0o017435 | 2 bytes | DATA | *(game state field 3)* |
| 0o017436–0o017437 | 2 bytes | DATA | `LIVES` — vieți rămase (inițial 5) |
| 0o017440–0o017441 | 2 bytes | DATA | `SCORE` — scor acumulat (inițial 0) |
| 0o017450–0o017647 | 128 bytes | GFX | **Tile pixel bank** — 16 tiles × 16 bytes fiecare (8×8 px @ 2bpp). Tile 0=background (017450), Tile 1=wall (017460)..Tile 15 (017650-017667) |
| 0o017650–0o017777 | 90 bytes | DATA | Contoare animație: ENEMY3_TICK(017360), ENEMY2_ANIM(017366-017370), ENEMY1_TICK(017372), PLAYER_ANIM(017374), ENEMY2_TICK(017376), SPAWN_TABLE(017420+) etc. |

### 2.17 Sprite tables + animation workspace (0o020000–0o022077)

| Adrese RAM | Dimensiune | Tip | Conținut |
|------------|------------|-----|---------|
| 0o020270–0o021237 | ~400 bytes | DATA | **Sprite animation table** — per (entity × direction × frame): index tile + delta-X + delta-Y |
| 0o021224–0o022023 | 384 bytes | DATA | **Sprite workspace** — cleared region (CLR @#11224..12024 în LEVEL_RESET). Player sprite workspace la 021640; enemy sprite workspace la 021760. |
| 0o022024–0o022077 | 44 bytes | DATA | *(misc workspace)* |

### 2.18 Hărți niveluri (0o022100–0o031277)

| Adrese RAM | Dimensiune | Tip | Conținut |
|------------|------------|-----|---------|
| 0o022100–0o022657 | 352 bytes | DATA | **Nivel 1** — 22 rânduri × 16 bytes (2 tiles/byte = 32 coloane) |
| 0o022660–0o023237 | 352 bytes | DATA | **Nivel 2** |
| 0o023240–0o023817 | 352 bytes | DATA | **Nivel 3** |
| 0o024000–0o024557 | 352 bytes | DATA | **Nivel 4** |
| 0o024560–0o025137 | 352 bytes | DATA | **Nivel 5** |
| 0o025140–0o025717 | 352 bytes | DATA | **Nivel 6** |
| 0o025720–0o026277 | 352 bytes | DATA | **Nivel 7** |
| 0o026300–0o026657 | 352 bytes | DATA | **Nivel 8** |
| 0o026660–0o027237 | 352 bytes | DATA | **Nivel 9** |
| 0o027240–0o027617 | 352 bytes | DATA | **Nivel 10** |
| 0o027620–0o037777 | ~4192 bytes | DATA | *(date suplimentare: inamici spawn tables, misc)* |

### 2.19 Framebuffer video (0o040000–0o077777)

| Adrese RAM | Dimensiune | Tip | Conținut |
|------------|------------|-----|---------|
| 0o040000–0o077777 | 16384 bytes (16 KB) | GFX | **Framebuffer BK-0010** — 256×256 px @ 2bpp color. Hardware video RAM. Jocul scrie tiles și sprite-uri direct în această zonă prin blit routines. Playfield-ul începe la 046000 (rândul 6 de tile-uri = 48 scanlines de la vârf). |

---

## REZUMAT PACKED (KLAD.BIN)

| Offset fișier | Dimensiune | Tip | Descriere |
|---------------|------------|-----|-----------|
| 0–3 | 4 bytes | HEADER | BK .BIN header (load + length) |
| 4–41 | 38 bytes | PAD | Padding 0732..0776 (BNE +0 filler) |
| 42–113 | 72 bytes | CODE | Depacker Stage 1 (backward-LZ decompressor) |
| 114–199 | 86 bytes | DATA | Stage 2 loader code data (28 words, citite backward) |
| 200–10432 | 10233 bytes | LZ | Payload principal LZ-comprimat (toate datele jocului) |
| **Total** | **10433 bytes** | | |

## REZUMAT UNPACKED (RAM la execuție)

| Adrese RAM | Tip | Descriere |
|------------|-----|-----------|
| 000000–000777 | SYS | Vectori sistem BK-0010 |
| 001000–003777 | CODE+DATA | Game logic, title, meniu, variabile, tabele |
| 004000–014417 | CODE | Rutine principale: render, keyboard, inamici, sprite, coliziune |
| 014420–022077 | DATA | Entity records, working buffer, tile bank, animation tables |
| 022100–037777 | DATA | Level maps (10 niveluri × 352 bytes) + spawn data |
| 040000–077777 | GFX | Framebuffer video (16KB hardware RAM) |
| **Total joc** | | 001000–037777 = ~15KB cod+date |
