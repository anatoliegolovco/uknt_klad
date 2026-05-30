# ROUTINES — КЛАД 1987 Баранов (УКНЦ МС-0511)

Index complet al rutinelor identificate în `disassembly/annotated/uknc_klad_1987.asm`.  
Adrese octale. Diferențele față de BK-0010 Crocodile marcate **[УКНЦ DIFF]**.

Referință BK-0010: `docs/reverse/ROUTINES.md` (35 rutine core, ~83% identice).

---

## 1. Lifecycle joc

| Adresă | Etichetă | Ce face |
|--------|----------|---------|
| 001000 | `RESTART` | Entry point; dacă LIVES=0 → GAME_OVER, altfel reinițializează nivelul |
| 001004 | `GAME_OVER_SOFT` | Decrementează LIVES; dacă 0 → menu, altfel → RESTART |
| 001016 | `PLAYER_DEATH` | Setează state moarte, sunet, ENTITY_STATE_INIT, repornește entitățile |
| 001024 | `BONUS_LIFE` | Acordă viață bonus; INC LIVES; sunet bonus |
| 001034 | `LEVEL_COMPLETE` | Avansează VAR_CUR_MAP_ADDR += 0o540 (352); entity pointer++; → GAME_INIT |
| 001112 | `ALL_LEVELS_DONE` | La nivel 11: resetează tot, JMP @#001000 de la zero |
| 001116 | `GAME_STATE_INIT` | Inițializează LIVES, SCORE=0, difficulty, VAR_CUR_MAP_ADDR=022100 |
| 001130 | `LEVEL_PTR_INIT` | Resetează VAR_CUR_LEVEL_PTR la TBL_ENTITY_PTRS[0] |

---

## 2. Bucla principală

| Adresă | Etichetă | Ce face |
|--------|----------|---------|
| 001142 | `KEY_DIFFICULTY` | Meniu selectare dificultate (1–4); scrie DELAY_SPIN_COUNT |
| 001306 | `DELAY_SPIN` | Busy-wait calibrat la viteza aleasă; determină frame rate efectiv |
| 001344 | `GAME_LOOP` | Bucla principală: DELAY_SPIN → GAME_TICK, repetă |
| 001354 | `GAME_LOOP_MENU` | Varianta cu menu check; ESC → TITLE_SEQ |
| 001406 | `KEY_TABLE_SCAN` | Scanează KEY_CODE_TBL pentru tastă → acțiune |
| 001424 | `NO_KEY` | Handler fără tastă; avansează animație pasiv |
| 001432 | `KEY_HIT` | Handler tastă validă; dispatch la acțiunea corespunzătoare |
| 001436 | `ACT_DISPATCH` | Dispatch central: ANIM_THROTTLE_PLAYER + 4 entități + efecte |
| 001602 | `GAME_TICK` | Un frame complet: DELAY_SPIN + 5 entități + LEVEL_END_CHECK |
| 001636 | `LEVEL_END_CHECK` | Verifică dacă tot aurul e colectat → LEVEL_COMPLETE |
| 001732 | `KEY_CODE_TBL` | Tabel ASCII → direcție/acțiune (date, nu cod) |

---

## 3. Intro și meniu

| Adresă | Etichetă | Ce face |
|--------|----------|---------|
| 002000 | `GAME_BRANCHES` | Tabel salt pentru stări joc (gold, death, bonus, exit) |
| 002050 | `SOUND_WRAPPER_B` | Apelează ENTITY_HANDLER pentru sunet tip B (moarte) |
| 002060 | `SOUND_WRAPPER_A` | Apelează ENTITY_HANDLER pentru sunet tip A (aur) |
| 002072 | `TITLE_SEQ` | Ecran titlu; animație intro cu DAT_INTRO_TILES (031000) |
| 002264 | `TITLE_WAIT` | Așteaptă tastă după ecranul de titlu |
| 003234 | `DIFF_SELECT` | Citește tastă 1–4 pentru dificultate; EMT 6 (read char) |
| 003274 | `GAME_OVER_WAIT` | Afișează "GAME OVER", așteaptă tastă |
| 003372 | `LIVES_DISPLAY` | Desenează indicatorul de vieți în HUD |
| 003444 | `DEATH_SCORE` | Afișează scorul la moarte |
| 003576 | `GAME_LEVEL_LOOP` | Bucla de nivel: init → render → rulează până la victorie |
| 003652 | `HUD_RENDER` | Desenează HUD: scor, vieți, nivel curent |
| 003746 | `BONUS_LIFE_ADD` | Logica acordare viață bonus (threshold scor) |
| 003764 | `SCORE_ADD` | Adaugă puncte la SCORE; verifică bonus life threshold |

---

## 4. Inițializare și input

| Adresă | Etichetă | Ce face |
|--------|----------|---------|
| 004000 | `GAME_INIT` | Init complet nivel: HW_INIT + PLAYER_SPRITE_INIT + LEVEL_RENDER + COLLISION_MAP_BUILD |
| 004210 | `NUM_RENDER` | Afișează număr (scor/vieți) ca text ASCII la coordonate |
| 004640 | `PLAYER_DEATH_TRIGGER` | Detectează coliziune player–inamic; dacă overlap → PLAYER_DEATH |
| 004660 | `KEY_ACTION_DATA` | Date acțiuni per tastă (date, nu cod) |
| 004674 | `KBD_GAME_POLL` | Polling tastatură non-blocking în joc; returnează ASCII sau 0 |
| 004724 | `KBD_SCAN` | Iterează tabelul de taste; returnează acțiunea |
| 004740 | `KBD_HIT` | Handler tastă detectată; setează direcție în entity record |

---

## 5. Render nivel

| Adresă | Etichetă | Ce face |
|--------|----------|---------|
| 004776 | `LEVEL_RENDER` | Randează tile map complet la prima afișare a nivelului |
| 005002 | `LEVEL_RENDER_R4` | Varianta cu R4 ca base pointer (din GAME_INIT) |
| 005010 | `LRND_ROW` | Iterează 22 rânduri tile map |
| 005016 | `LRND_BYTE` | Decodează byte → 2 tile-uri; apelează TILE_BLIT_FWD pentru fiecare |
| 005106 | `TILE_BLIT_FWD` | Blit tile 8×8 forward via DISP_COL_BLIT **[УКНЦ DIFF]** |
| 012442 | `LEVEL_RENDER_FULL` | Randează tot tile work buffer la ecran (după reset nivel) |
| 012452 | `LFULL_ROW` | Iterează rândurile work buffer |
| 012462 | `LFULL_COL` | Iterează coloanele; blit via TILE_BLIT_SUB |
| 012530 | `TILE_BLIT_SUB` | Blit tile indexat din work buffer |
| 012540 | `TSUB_ROW` | Rând intern al blit-ului din work buffer |

---

## 6. Player

| Adresă | Etichetă | Ce face |
|--------|----------|---------|
| 005754 | `HW_INIT` | Init hardware УКНЦ: display setup, scroll, clear **[УКНЦ DIFF]** |
| 006054 | `PLAYER_SPRITE_INIT` | Setează sprite work buffer player; poziție inițială din entity record |
| 007432 | `ANIM_THROTTLE_PLAYER` | Throttle animație player: counter 0→4; la 4 → SPRITE_DRAW + reset |
| 012570 | `PLAYER_STATE_CHECK` | State machine player: mort/aur/exit/bonus → dispatch |
| 012716 | `ENEMY_RESPAWN` | Respawn inamici după moarte player |
| 012740 | `PLAYER_MOVE_STEP` | Verifică flag-uri coliziune; actualizează PLAYER_TILE_PTR + gravitație |
| 013024 | `PMOVE_EXEC` | Execută mutarea efectivă (update X/Y) |
| 013116 | `PMOVE_BLIT` | Mută player și blit-ează sprite la noua poziție |

---

## 7. Inamici

| Adresă | Etichetă | Ce face |
|--------|----------|---------|
| 006462 | `WATER_COLLISION` | Verifică tile sub player; apă → PLAYER_DEATH |
| 006552 | `ENEMY2_TICK` | Throttle + dispatch mișcare Enemy 2 |
| 006602 | `ENEMY2_MOVE` | Chase greedy Enemy 2: compară col → row cu player |
| 007136 | `ENEMY2_MOVE_UP` | Mișcare verticală Enemy 2 pe scară |
| 007202 | `SPRITE_ANIM_C` | Animație Enemy 2 orizontal: CMP #7, 9 ticks/frame |
| 007242 | `SPRITE_ANIM_D` | Animație Enemy 2 vertical: CMP #4, 6 ticks/frame |
| 007306 | `ENTITY1_RESTORE` | Restaurează Enemy 1 după ce a fost "capturat" de player |
| 007376 | `ENEMY1_TICK` | Enemy 1: mutare + draw la fiecare 5 ticks (combinat) |
| 007556 | `ENEMY3_TICK` | Throttle + dispatch mișcare Enemy 3 |
| 007572 | `ENEMY3_MOVE` | Chase greedy Enemy 3 (identic cu Enemy 2) |
| 007646 | `ENEMY3_MOVE_UP` | Mișcare verticală Enemy 3 |
| 007702 | `SPRITE_ANIM_A` | Animație Enemy 3 orizontal: CMP #7, 9 ticks/frame |
| 007742 | `SPRITE_ANIM_B` | Animație Enemy 3 vertical: CMP #4, 6 ticks/frame |

---

## 8. Reset nivel și sprite

| Adresă | Etichetă | Ce face |
|--------|----------|---------|
| 010000 | `LEVEL_RESET` | Resetează nivelul după moarte: copiază tile map + respawn entități |
| 010010 | `LRESET_COPY` | Copiază tile map original în work buffer |
| 010024 | `LRESET_ENEMIES` | Resetează pozițiile inamicilor la spawn per nivel |
| 010062 | `LRESET_CLR` | Curăță sprite work buffers |
| 013216 | `SPRITE_HELPERS` | Determină frame animație din state (021=climb, 023=walk) |
| 013524 | `COLLISION_MAP_BUILD` | Construiește BUF_TILE_WORK: 8 flag bits per tile din tile map |
| 013536 | `CMAP_UNPACK` | Decodează nibble-uri tile map → work buffer |
| 013570 | `CMAP_FLAGS` | Setează flag-uri soliditate per tile (prag index=8) |
| 014030 | `SPRITE_DRAW` | Draw sprite 8×8 din entity record + TBL_ANIM_FRAMES **[УКНЦ DIFF]** |
| 014120 | `SDRAW_BLIT2` | Blit al doilea tile al sprite-ului extins |
| 014302 | `TILE_BLIT_REV` | Blit tile reverse (index byte din tile bank 017450) |
| 014334 | `TBREV_ROW` | Rând intern TILE_BLIT_REV via DISP_COL_BLIT **[УКНЦ DIFF]** |

---

## 9. Sunet

| Adresă | Etichetă | Ce face |
|--------|----------|---------|
| 006134 | `SOUND_TBL_A` | Tabel frecvențe sunet A (colectare aur, efecte scurte) |
| 006156 | `SOUND_TBL_B` | Tabel frecvențe sunet B (moarte, tranziții) |
| 006204 | `ENTITY_HANDLER` | Motor sunet: toggle @#177716 bit 7 la frecvența din tabel **[УКНЦ DIFF]** |
| 006324 | `ENT_INACTIVE` | Handler entitate inactivă (skip frame) |
| 006366 | `ENT_SPEAKER_LOOP` | Bucla de generare ton: toggle speaker bit |
| 006434 | `ENT_DONE` | Finalizare sunet; restaurează registre |
| 006444 | `ENTITY_STATE_INIT` | Resetează contoare animație și stare entități |

---

## 10. УКНЦ I/O — Display (040060–041740)

**[УКНЦ DIFF]** — toate aceste rutine lipsesc din BK-0010 (BK scrie direct în VRAM 040000–077777).  
УКНЦ folosește porturile hardware @#176640 (pixel) și @#176642 (culoare).

| Adresă | Etichetă | Ce face |
|--------|----------|---------|
| 040060 | `DISP_SCANLINE_WRITE` | Scrie o linie de scanare la portul pixel @#176640 |
| 040220 | `DISP_XY_CONVERT` | Convertește coordonate tile X/Y → adresă port display |
| 040320 | `DISP_COL_SETUP` | Inițializează pointer coloană pentru scriere |
| 040420 | `DISP_SERIAL_WAIT` | Busy-wait ready bit de la portul display |
| 040520 | `DISP_BLIT_INNER` | Bucla internă blit: pixel-plane + colour-plane la port |
| 040560 | `DISP_COL_ADDR` | Calculează adresa de coloană în display buffer |
| 040600 | `DISP_COL_LOOP` | Iterează coloanele în blit |
| 040640 | `DISP_SWAP` | Swap buffer display (double-buffering УКНЦ) |
| 040660 | `KBD_READ` | Citește tastă din @#040546; returnează 0 dacă nu e tastă |
| 040700 | `KBD_STATUS_READ` | Verifică status port tastatură |
| 040760 | `KBD_WAIT` | Busy-wait până la tastă disponibilă |
| 041000 | `DISP_INIT_ENTRY` | Entry init display УКНЦ; setează mod grafic |
| 041020 | `KBD_POLL` | Polling non-blocking tastatură; folosit în GAME_LOOP |
| 041040 | `DISP_COL_BLIT` | **Rutina centrală de blit**: scrie tile 8×8 la @#176640/@#176642 |
| 041060 | `DCOL_ADVANCE` | Avansează pointer coloană display după blit |
| 041100 | `EMT_TEXT` | EMT trap 14: afișează string KOI8-R pe display |
| 041120 | `EMT_TEXT2` | Varianta EMT text cu format alternativ |
| 041140 | `DISP_SETUP` | Setup parametri display (rezoluție, scroll) |
| 041160 | `DISP_SETUP_RTS` | DISP_SETUP cu RTS explicit la final |
| 041200 | `DISP_MODE_CHECK` | Verifică/setează modul curent display |
| 041260 | `DISP_STATE_TST` | Test stare display: verifică ready |
| 041340 | `DISP_COL_RENDER` | Randează o coloană din sprite la display |
| 041360 | `DISP_STRIP_COLOR` | Scrie banda de culoare la @#176642 |
| 041400 | `VSYNC_WAIT_LOOP` | Așteaptă sync vertical display УКНЦ |
| 041460 | `DISP_PIXEL_PORT` | Scrie word la portul pixel @#176640 |
| 041500 | `DISP_LINE_ADV` | Avansează la linia display următoare |
| 041520 | `DLINE_INNER` | Bucla internă scriere linie |
| 041600 | `DLINE_ROW_WRITE` | Scrie un rând de pixeli la port |
| 041620 | `DLINE_SYNC_BIT` | Setează/curăță bitul de sync |
| 041640 | `DLINE_COUNT` | Contor linii display |
| 041660 | `DLINE_DONE` | Finalizare scriere linie; restaurează registre |
| 041740 | `DISP_WRITE_COL` | Scrie date culoare la @#176642 |

---

## 11. Date și tabele (nu cod)

| Adresă | Etichetă | Conținut |
|--------|----------|---------|
| 001230 | `TBL_ENTITY_PTRS` | 10 words → pointeri entity spawn records per nivel |
| 001300 | `VAR_CUR_MAP_ADDR` | Adresă tile map nivel curent (init: 022100) |
| 001302 | `VAR_CUR_LEVEL_PTR` | Pointer entity record curent |
| 012134 | `KEY_ACTION_TBL` | Tabel acțiuni tastatură |
| 014420 | `PLAYER_STATE_WORD` | Flags + state player |
| 014422 | `PLAYER_TILE_PTR` | Pointer tile curent player în TILE_WORK_BUF |
| 014430 | `ENEMY1_STATE_WORD` | Stare Enemy 1 |
| 014432 | `ENEMY1_TILE_PTR` | Pointer tile Enemy 1 |
| 014440 | `ENEMY2_STATE_WORD` | Stare Enemy 2 |
| 014442 | `ENEMY2_TILE_PTR` | Pointer tile Enemy 2 |
| 014550 | `TILE_WORK_BUF` | Buffer runtime tile map: 440 bytes, 8 flag bits per tile |
| 017430 | `GAME_STATE` | Word stare globală joc |
| 017436 | `LIVES` | Byte vieți rămase |
| 017440 | `SCORE` | Word scor curent |
| 017450 | `TILE_BANK` | 32 tile-uri × 16 bytes (2bpp 8×8) |
| 020270 | `TBL_ANIM_FRAMES` | Offset-uri animație: state × 4 → (mask, ΔX, ΔY) |
| 021640 | `SPRITE_WS_PLAYER` | Workspace sprite player |
| 021760 | `SPRITE_WS_ENEMY` | Workspace sprite inamici |
| 022100 | `LEVEL_MAPS` | Start tile maps (10 × 352 bytes, stride=0o540) |
| 031000 | `DAT_INTRO_TILES` | 12 frame-uri intro × 16 bytes (2bpp 8×8) |
| 031300 | `DAT_SPRITE_BANK` | 208 frame-uri animație × 16 bytes; acces word-pointer |
| 037700 | `DAT_STRINGS` | Strings KOI8-R: legenda taste |

---

## Statistici

| Categorie | Rutine |
|-----------|--------|
| Lifecycle / meniu | 21 |
| Bucla principală | 11 |
| Player | 8 |
| Inamici | 13 |
| Reset nivel + sprite | 12 |
| Render nivel | 10 |
| Sunet | 7 |
| УКНЦ I/O display | 32 |
| Date / tabele | 22 |
| **Total** | **136** |

BK-0010 Crocodile (referință): 35 rutine core.  
УКНЦ adaugă 32 rutine I/O dedicate (porturile 176640/176642) față de scrierea directă VRAM la BK.
