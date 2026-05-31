# Rutine identificate — Crocodile КЛАД (unpacked)

Sursa: `disassembly/annotated/crocodile_klad.asm` + `disassembly/annotated/_unpacked_raw.asm`  
Toate adresele sunt **octale** din RAM-ul decomprimat (org=0).  
Status: **35/35 rutine documentate** — Obiectiv 2 COMPLET.

## Index rutine (complete)

| Adresă | Nume | Apeluri | Descriere scurtă |
|--------|------|---------|-----------------|
| `001000` | `RESTART` | boot | JMP @#GAME_INIT — trampoline de restart complet |
| `001004` | `GAME_OVER_SOFT` | 1 | Reinit parțial (CLR R0 + JMP 004160) |
| `001016` | `PLAYER_DEATH` | 1 | Moartea jucătorului — apelează animație, resetează |
| `001034` | `LEVEL_COMPLETE` | 1 | Nivel terminat — avansează pointer în tabela de niveluri |
| `001112` | *(reload)* | 1 | Toate nivelurile terminate → JMP RESTART |
| `001142` | `KEY_DIFFICULTY` | 1 | Procesează taste 1-4 → setează viteza @#001312 |
| `001306` | `DELAY_SPIN` | inline | Busy-wait (burn cycles, R5 iterații) |
| `001344` | `GAME_LOOP` | — | Entry per-frame → JMP 004674 |
| `001354` | `GAME_LOOP_MENU` | — | Poll keyboard meniu (@#177662), tabelă taste 001732 |
| `001436` | `ACT_DISPATCH` | 1 | Dispatch acțiune tastatură (R0=action code, R4→player) |
| `001602` | `GAME_TICK` | — | Per-frame update: delay → player state → enemies → collision |
| `002050` | `SOUND_WRAPPER_B` | 3 | Setează R5=#6156 → JMP @#002214 (ENTITY_HANDLER) |
| `002060` | `SOUND_WRAPPER_A` | 3 | Setează R5=#6134 → JSR @#006204 (ENTITY_HANDLER) |
| `002072` | `TITLE_SEQ` | 1 | Secvența title screen: randare tile-uri + text + wait |
| `002264` | `TITLE_WAIT` | — | Loop blocat pe EMT 6, așteaptă LF (012 octal) |
| `003234` | `DIFF_SELECT` | 1 | Selectare dificultate: taste '1'-'4', setează @#001312 |
| `003274` | `GAME_OVER_WAIT` | 1 | Afișează Game Over, block pe Enter → RESTART |
| `003372` | `LIVES_DISPLAY` | 2 | Re-randează contorul de vieți în HUD |
| `003444` | `DEATH_SCORE` | 1 | Penalizare scor + redesenare HUD la moarte |
| `003576` | `GAME_LEVEL_LOOP` | — | Loop principal de nivel: render + sound entity per frame |
| `003652` | `HUD_RENDER` | 3 | Randează scor + vieți în zona HUD |
| `003746` | `BONUS_LIFE_ADD` | 1 | State 5 (bonus life tile): +1 viață + vsync + redesenare |
| `003764` | `SCORE_ADD` | 1 | State 4 (gold tile): +10 la scor + vsync |
| `004000` | `GAME_INIT` | — | Cold start: vieți=5, scor=0, init display, JMP 005754 |
| `004210` | `NUM_RENDER` | 7 | Randează număr zecimal pe ecran (scor/vieți) |
| `004640` | `PLAYER_DEATH_TRIGGER` | 1 | Setează flag coliziune tile + JMP 012614 |
| `004674` | `KBD_GAME_POLL` | — | Poll keyboard gameplay: citește @#177714, rotește biți |
| `004776` | `LEVEL_RENDER` | 1 | Randează tile map complet (22 rânduri × 16 bytes, 2 tiles/byte) |
| `005002` | `LEVEL_RENDER_R4` | 1 | Identic cu 004776 dar folosit din GAME_LEVEL_LOOP |
| `005106` | `TILE_BLIT_FWD` | 2 | Blit tile forward: 8 cuvinte → 046000(R1), pas=100 |
| `005754` | `HW_INIT` | 1 | Init hardware: scroll=001330, EMT-uri video, JMP 002072 |
| `006054` | `PLAYER_SPRITE_INIT` | 3 | Inițializează sprite player: HUD_RENDER + SPRITE_DRAW × 2 |
| `006204` | `ENTITY_HANDLER` | 6 | Motor de sunet: generează ton prin toggle @#177716; și delay pentru entități inactive |
| `006444` | `ENTITY_STATE_INIT` | 1 | Resetează contoare animație + apelează PLAYER_SPRITE_INIT |
| `006462` | `WATER_COLLISION` | 1 | Verifică dacă player e pe tile apă (state 15); erase + restore dacă da |
| `006552` | `ENEMY2_TICK` | 1 | State machine inamic 2: throttle @#17376, mișcare spre player |
| `007136` | `ENEMY2_MOVE_UP` | 1 | Ramura de mișcare sus pentru inamic 2 (apelat din ENEMY2_TICK) |
| `007202` | `SPRITE_ANIM_C` | 1 | Throttle animație 8-frame pentru inamic 2 (mișcare dreaptă) |
| `007242` | `SPRITE_ANIM_D` | 1 | Throttle animație 5-frame pentru inamic 2 (mișcare jos) |
| `007306` | `ENTITY1_RESTORE` | 1 | Dacă inamic 1 state=15: erase sprite, restore din @#17430 |
| `007376` | `ENEMY1_TICK` | 1 | Throttle inamic 1: counter @#17372, apelează 012740 + 012024 |
| `007432` | `ANIM_THROTTLE_PLAYER` | 2 | Throttle animație player: counter @#17374, draw la ≥3 |
| `007462` | `ENEMY3_TICK` | 1 | State machine inamic 3: throttle @#17360, mișcare spre player |
| `010036` | `ENEMY3_MOVE_UP` | 1 | Ramura mișcare sus pentru inamic 3 (apelat din ENEMY3_TICK) |
| `010102` | `SPRITE_ANIM_A` | 1 | Throttle animație 8-frame pentru inamic 3 (mișcare dreaptă) |
| `010142` | `SPRITE_ANIM_B` | 1 | Throttle animație 5-frame pentru inamic 3 (mișcare jos) |
| `010206` | `LEVEL_RESET` | 1 | Reset complet nivel: spawn positions, collision map, render, clear |
| `012326` | `VSYNC_WAIT` | 6 | Vsync: MOV #7,R0 / EMT 016 / RTS |
| `012342` | `KEY_ACTION_TBL` | — | Tabelă coduri acțiuni (12 intrări × 2 bytes = 24 bytes) |
| `012442` | `LEVEL_RENDER_FULL` | 1 | Randează din working buffer @#14550: 22 rânduri × 32 coloane |
| `012530` | `TILE_BLIT_SUB` | inline | Blit 8×8 tile: copie din tile bank la framebuffer (variant 3) |
| `012570` | `PLAYER_STATE_CHECK` | 1 | Verifică state byte tile player: water/gold/life/level-exit |
| `012716` | `ENEMY_RESPAWN` | 2 | Setează inamic activ (state=11) + flag-uri vecini |
| `012740` | `PLAYER_MOVE_STEP` | 1 | Execută un pas de mișcare player: verificare coliziune, blit tile, update pos |
| `013524` | `COLLISION_MAP_BUILD` | 2 | Unpacks nivel packed → working buffer @#14550 + calculează flag-uri coliziune per tile |
| `014030` | `SPRITE_DRAW` | 7 | Desenează sprite 2-tile din entity record |
| `014302` | `TILE_BLIT_REV` | 8 | Blit tile cu index din R2: × 16 + bank 017450 → 046000(R1) |

**Total: 35 rutine documentate.**

---

## Variabile globale (RAM)

| Adresă | Nume | Init | Descriere |
|--------|------|------|-----------|
| `001300` | `CUR_MAP_ADDR` | — | Adresă start hartă nivel curent |
| `001302` | `CUR_LEVEL_PTR` | — | Pointer curent în tabela de niveluri |
| `001304` | `LEVEL_TBL_PTR` | — | Entry curent tabela niveluri |
| `001312` | `SPEED` | 1000 | Viteza jocului (400/1000/2000/4000) |
| `005146` | `SOUND_TIMING` | calc | Valoare delay calculată de sound engine |
| `014420` | `PLAYER_STATE` | 0 | Cuvântul de stare player |
| `014422` | `PLAYER_TILE_PTR` | — | Pointer la tile curent player în @#14550 |
| `014424` | `PLAYER_X` | 024 | Coloana screen player (bytes) |
| `014430` | `ENEMY1_STATE` | 0 | Stare inamic 1 |
| `014432` | `ENEMY1_TILE_PTR` | — | Pointer tile inamic 1 |
| `014436` | `ENEMY1_Y` | — | Offset Y ecran inamic 1 |
| `014440` | `ENEMY2_STATE` | 0 | Stare inamic 2 |
| `014442` | `ENEMY2_TILE_PTR` | — | Pointer tile inamic 2 |
| `014446` | `ENEMY2_Y` | — | Offset Y ecran inamic 2 |
| `017360` | `ENEMY3_TICK_CTR` | 0 | Contor throttle inamic 3 |
| `017362` | `ENEMY3_ANIM_A` | 0 | Contor animație 8-frame inamic 3 |
| `017364` | `ENEMY3_ANIM_B` | 0 | Contor animație 5-frame inamic 3 |
| `017366` | `ENEMY2_ANIM_A` | 0 | Contor animație 8-frame inamic 2 |
| `017370` | `ENEMY2_ANIM_B` | 0 | Contor animație 5-frame inamic 2 |
| `017372` | `ENEMY1_TICK_CTR` | 0 | Contor throttle inamic 1 |
| `017374` | `PLAYER_ANIM_CTR` | 0 | Contor throttle animație player |
| `017376` | `ENEMY2_TICK_CTR` | 0 | Contor throttle inamic 2 |
| `017400` | `SPRITE_BUF_BASE` | 011224 | Base sprite workspace (zona curățată) |
| `017420` | `SPAWN_TABLE_BASE` | — | Tabelă poziții spawn entități |
| `017424` | `ENEMY1_SPAWN_PTR` | — | Pointer spawn inamic 1 |
| `017426` | `ENEMY2_SPAWN_PTR` | — | Pointer spawn inamic 2 |
| `017430` | `GAME_STATE` | 010404 | Cuvânt stare globală (pointer table base) |
| `017436` | `LIVES` | 5 | Vieți rămase |
| `017440` | `SCORE` | 0 | Scor acumulat |

---

## Constante hardware BK-0010

| Adresă | Registru | Utilizare |
|--------|----------|-----------|
| `177662` | Keyboard data | Citit în meniuri (@#177662) |
| `177664` | Scroll register | Setat la 001330 = offset ecran |
| `177714` | Keyboard shift-reg | 11-bit, citit în gameplay loop |
| `177716` | System register | Bit 6 = gate vsync; bit 7 = speaker |
| `102064` | *(RAM constant)* | Valoare sys-reg „speaker HIGH" pentru sound engine |
| `102076` | *(RAM constant)* | Valoare sys-reg „speaker LOW" pentru sound engine |

---

## Constante grafice

| Valoare | Semnificație |
|---------|-------------|
| `017450` | Tile bank start (16 tiles × 16 bytes) |
| `046000` | Framebuffer playfield start (rândul 6 pe ecran) |
| `100` oct | Stride scanline = 64 bytes = 32 cuvinte |
| `1000` oct | Stride rând tile = 512 bytes = 8 scanlines × 64 bytes |
| `14550` | Working tile buffer (22×32 tiles expanded, 1 cuvânt/tile) |
| `21640` | Sprite workspace player |
| `21760` | Sprite workspace inamic |

---

## Structura entity record

```
@014420 player_state   : word  (stare player)
@014422 player_tile    : ptr   → working buffer @#14550 entry
@014424 player_x       : word  (coloana screen, bytes)
@014430 enemy1_state   : word
@014432 enemy1_tile    : ptr
@014436 enemy1_y       : word  (offset Y ecran)
@014440 enemy2_state   : word
@014442 enemy2_tile    : ptr
@014446 enemy2_y       : word
```

Tile pointer → cuvânt în @#14550 cu format:
- biți 0-3:  tile index (0-15)
- bit 8:     stânga blocată
- bit 9:     dreapta blocată
- bit 10:    solid jos (poate coborî)
- bit 11:    solid (stă pe loc)
- bit 12:    poate coborî pe scară
- bit 13:    intrare scară stânga
- bit 14:    intrare scară dreapta

---

## Valori tile (nibble 0-15)

| Valoare | Tile | Efect |
|---------|------|-------|
| 0 | background/gol | pasabil în toate direcțiile |
| 1 | perete | solid, blochează toate mișcările |
| 2 | scară | urcabil/coborît, nu mers lateral |
| 3 | apă | letal la contact (state=15) |
| 4 | aur | colectabil → +10 scor (state=4) |
| 5 | bonus viață | colectabil → +1 viață (state=5) |
| 6 | ieșire nivel | colectabil → nivel următor (state=6) |
| 7-9 | platforme variante | solid sus, pasabil lateral |
| 10-15 | rezervat / spawn | special, calculat de COLLISION_MAP_BUILD |

---

## Tabele de taste

### Gameplay (shift register @#177714, bit 0-10):
Citit în KBD_GAME_POLL (004674). Tabelă acțiuni la 012342.

| Bit | Cod acțiune | Mișcare |
|-----|-------------|---------|
| 0   | 012         | stânga |
| 1   | 022         | dreapta |
| 5   | 006         | săritură/foc |
| 9   | 002         | urcare scară |
| 10  | 004         | coborâre scară |

### Meniu (@#177662), tabelă taste 001732 + cod la 012342:

| Tasta BK-0010 | Cod octal | Acțiune |
|----------------|-----------|---------|
| ← (stânga)     | 017       | 012 (stânga) |
| → (dreapta)    | 016       | 022 (dreapta) |
| Ctrl-H         | 010       | 002 (urcare) |
| Ctrl-Z(?)      | 032       | 004 (coborâre) |
| ↑ (sus)        | 031       | 006 (foc) |

---

## Arhitectura sound engine

`ENTITY_HANDLER` (006204) generează sunet pentru BK-0010:
- Scrie alternativ două valori în @#177716 (system register bit 7 = speaker)
- Valorile speaker-HIGH/LOW sunt la @#102064 și @#102076 în RAM
- Bucle de burn (SOB R0, self) setează frecvența (R1 = semiperioadă)
- Counter extern R3 = numărul de cicluri complete
- Apelat cu R5 → tabelă entități sonore (006134 = set A, 006156 = set B)
- Entitățile inactive (bit 12=0) calculează delay de inter-notă în @#005146
