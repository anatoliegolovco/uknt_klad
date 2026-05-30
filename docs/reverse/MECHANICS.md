# MECHANICS — КЛАД 1987 Баранов (УКНЦ МС-0511)

Documentat exclusiv din assembler. Adresele sunt octale.

---

## 1. Bucla principală de joc

**Entry point:** 001000 → `JMP @#004000` (GAME_INIT)  
**Bucla:** 001344 (`GAME_LOOP`) → `JMP @#004674` (KBD_GAME_POLL)

Secvența per-frame (GAME_TICK la 001602):

```
DELAY_SPIN (001306)           — pauză de viteză (valoare din @#001312, setată de KEY_DIFFICULTY)
PLAYER_STATE_CHECK (012570)   — state machine player
ENEMY1_TICK (007376)          — avansează enemy 1 (cu throttle)
ENEMY2_TICK (006552)          — avansează enemy 2 (cu throttle)
ENEMY3_TICK (007462)          — avansează enemy 3 (cu throttle)
ENTITY1_RESTORE (007306)      — restaurează tile după enemy 1
WATER_COLLISION (006462)      — verifică dacă enemy 2 e în apă
LEVEL_END_CHECK (001636)      — CMP player_tile, enemy1_tile / enemy2_tile → coliziune inamic
→ JMP 002000 (continuă) sau JMP 002040 (eveniment)
```

**Viteze de joc:** `KEY_DIFFICULTY` (001142) scrie în `@#001312` (DELAY_SPIN_COUNT):
- Tasta 1: `#1000` (octal) = lent
- Tasta 2: `#2000`
- Tasta 3: `#4000`
- Tasta 4: `#10000` (octal) = rapid

---

## 2. Tastatură

**УКНЦ:** `FN_KBD_READ` (040660) citește din `@#REG_KBD_STATUS` (040546).  
`FN_KBD_POLL` (041020) = wrapper cu test bit "key ready" — **polling non-blocking**, nu interrupt-driven.

Tastatura УКНЦ suportă și interrupts hardware (prin controllerul de la 040712), dar jocul **nu le folosește** — verifică statusul o dată per frame și continuă indiferent.

**`KEY_CODE_TBL`** (001732) — 6 cuvinte = 6 taste mapate:

```
KEY_CODE_TBL:   cod_tastă_1, cod_tastă_2, ... (6 intrări × 2 bytes)
KEY_ACTION_TBL: acțiune_1,   acțiune_2,   ... (12 bytes offset de la KEY_CODE_TBL)
```

Mapare: `KEY_TABLE_SCAN` (001406) iterează 6 intrări, compară codul tastei cu R0. La match: `MOV 12(R5), R0` → încarcă codul de acțiune.

Coduri de acțiune:
- Acțiune ≤ 12 (octal = 10 dec): **direcțional** (stânga/dreapta/sus/jos)
- Acțiune > 12: **altă funcție** (pauză, restart etc.)

Dacă nici o tastă: R0 = 177777 = sentinel "no-key".

---

## 3. Player — Entity Record

Player entity record starts at **014420** (indirect via `VAR_PLAYER_TILE_PTR` = 014422):

| Offset | Size | Content |
|--------|------|---------|
| +0 | word | Pointer în BUF_TILE_WORK (014550) = poziția curentă pe tile map |
| +2 | word | Flags: bit 15 = block flag, bit 12 = ground flag, bit 11 = alive/active flag, bit 10 = state bits |
| +4 | word | Direcție / acțiune curentă |
| +6 | word | Sprite X position |
| +10 | word | Sprite Y position |

Verificare activă (ACT_DISPATCH la 001436):
```
CMP #10, (R4)           — entity type == 10 (= player active)?
BIT #2000, @2(R4)       — collision/active flag set?
→ dacă nu: skip input
CMPB #15, @2(R4)        — state == 15 (dead)?
→ dacă da: nu procesa input
```

---

## 4. Player — State Machine

`PLAYER_STATE_CHECK` (012570) este apelat o dată per frame. Citește `VAR_PLAYER_TILE_PTR` → citește state byte de la acea adresă.

| State (octal) | Semnificație | Acțiune |
|---------------|-------------|---------|
| `011` (= 9 dec) | Coliziune letală | → state = 017 (15), `JMP DEATH_TRIGGER` (004640) |
| `4` | Aur colectat | → state = 0, `JSR SCORE_ADD` (+10 puncte) |
| `5` | Viață bonus colectată | → state = 0, `JSR 003746` (handler viață) |
| `6` | Exit nivel atins | → state = 020 (16), `ENEMY_RESPAWN` (×2), `JSR 002060` (LEVEL_COMPLETE?) |
| altele | Neutru | → RTS (fără acțiune) |

**`PLAYER_DEATH_TRIGGER`** (004640):
```
004640: BIS #2000, 177700(R3)   — setează bit de moarte în entity control word (offset -100 de la player tile)
```
Acest bit declanșează death animation la următorul frame.

---

## 5. Coliziunea cu tile-uri (COLLISION_MAP_BUILD)

`FN_COLLISION_MAP_BUILD` (013524) este apelat la:
- LEVEL_COMPLETE (001034) — înainte de nivel nou
- LEVEL_RESET (010206) — la moarte

**Unpack:** Citește harta de nivel (22×16 bytes, 2 tile-uri/byte) → `BUF_TILE_WORK` (014550). Fiecare tile devine 1 word cu:
- Low byte: tile index (0–15)
- High byte: 8 flag-uri de coliziune (construite în pasul CMAP_FLAGS)

**CMAP_FLAGS** (013570) — construit per tile (R2 = pointer curent în BUF_TILE_WORK):

| Flag bit | Mask | Condiție (praguri în decimal) | Semnificație |
|----------|------|-------------------------------|-------------|
| bit 0 | `#1000` | tile_dreapta (offset +2) ≥ 8 | Solid la dreapta |
| bit 1 | `#400` | tile_stânga (offset −2) ≥ 8 | Solid la stânga |
| bit 2 | `#4000` | tile_curent ≥ 8 SAU tile_jos ≥ 6 | Curent solid / jos lethal |
| bit 6 | `#100000` | tile_dreapta ≥ 9 | Perete dur dreapta |
| bit 5 | `#40000` | tile_stânga ≥ 9 | Perete dur stânga |
| bit 4 | `#20000` | curent = 8 ȘI tile_sus (offset −100) ≥ 8 | Pod suspendat |
| bit 3 | `#10000` | tile_jos ≥ 8 SAU (curent=8 ȘI jos≥6) | Sol solid sub |
| bit 10 | `#2000` | tile_jos ∈ [7, 15] | Sub tile e apă / letal |

**Prag critic:** tile index ≥ 8 (octal 10) = **solid** (wall/earth). Tiles 0–7 = pasabile (aer, scară, aur, apă).

Excepție pentru tile 7 (apă): are flag `#2000` setat (letal), dar poate fi și pasabil pentru deplasare (prag separat ≥ 6).

---

## 6. Mișcarea player-ului (PLAYER_MOVE_STEP)

`PLAYER_MOVE_STEP` (012740):
1. Copiază poziția și sprite din entity record în R4 (move buffer la 012372)
2. `TST 4(R4)` — are direcție de mișcare?
   - Nu: `JMP @#005724` (fall/gravity check)
   - Da: intră în PMOVE_EXEC (013024)

**PMOVE_EXEC** (013024):
```
R3 = R4 + 4(R4)              — pointer spre tile vecin în direcția de mișcare
BIT 2(R3), @0(R4)            — verifică dacă flag-ul tile-ului vecin blochează mișcarea
BEQ → mișcare liberă          — dacă flag = 0: permite
```
La mișcare permisă:
1. 4× ASL R2 → adresă tile în tile bank (tile_index × 16 + 017450)
2. `JSR PC, @#041436` — erase sprite curent (blit tile gol în locul vechi)
3. `ADD (R3), (R4)` / `ADD (R3), 2(R4)` — actualizează poziția (tile pointer + offset)
4. `JSR PC, @#041314` — draw sprite la noua poziție

**Gravitație:** dacă nu există direcție și tile-ul de sub player nu e solid (flag `#10000` absent): player cade un tile în jos.

---

## 7. Coliziunea cu apa (WATER_COLLISION)

`FN_WATER_COLLISION` (006462):
```
MOV @#14442, R0          — pointer entity record
CMPB #15, (R0)           — state == 15?
BNE → return              — dacă nu: ignoră
```
State 15 = "entitate în apă/lethal". Dacă enemy2 e în starea 15:
- Se execută rutina de distrugere (probabil play sound + respawn)

**Cum ajunge un entity la state=15:** tile-ul de sub entitate are flag `#2000` (apă letal), iar COLLISION_MAP_BUILD a marcat acel tile. Player-ul: dacă pasul duce spre un tile cu apă, state player devine 011 → PLAYER_STATE_CHECK îl detectează → state = 017 (dead) → DEATH_TRIGGER.

---

## 8. Coliziunea cu inamici

`LEVEL_END_CHECK` (001636):
```
CMP @#014422, @#014432   — player_tile_ptr == enemy1_tile_ptr?
BNE → check enemy2
JMP 002040               — coliziune: declanșează eveniment
CMP @#014422, @#014442   — player_tile_ptr == enemy2_tile_ptr?
BNE → continue
JMP 002040               — coliziune
```

Coliziunea = player și inamic sunt pe **același tile** (pointer identic în BUF_TILE_WORK). Comparația e la nivel de word pointer, nu pixel. Rezoluție: 1 tile (8×8 px).

La coliziune: salt la 002040:
```
002040: JSR PC, @#002050     — play death sound (SOUND_WRAPPER_B: R5=6156)
002044: JMP @#002254
002254: MOV #1000, R0
002260: JMP @#001016         — PLAYER_DEATH (scade viață, reset nivel)
```
Lanțul complet: sunet moarte → scade viață (`SUB #1, @#017436`) → dacă `LIVES==0`: `JMP @#003576` (GAME_OVER / titlu) → altfel: LEVEL_RESET.

---

## 9. Colectarea aurului

La contact cu aur (tile 4, 5 sau 6), state player devine `4` (gold). PLAYER_STATE_CHECK detectează:
```
CMPB #4, (R3)
BNE → skip
CLRB (R3)               — state = 0
JSR @#003764             — SCORE_ADD
```

**`SCORE_ADD`** (003764):
```
003764: JSR PC, @#012326      — actualizează display scor
003770: ADD #12, @#017440     — SCORE += 012 (octal) = 10 (decimal)
003776: RTS PC
```

Scor: +10 puncte per aur. Variabila `VAR_SCORE` = 017440. Afișarea se face via `JSR @#012326` (NUM_RENDER sau echivalent).

---

## 10. Condiția de moarte

**Cauze de moarte player:**
1. **Apă:** player tile flag `#2000` → state 011 → DEATH_TRIGGER
2. **Inamic:** `player_tile_ptr == enemy_tile_ptr` (aceeași celulă) → JMP 002040
3. **State direct:** orice cod care setează state = 011 (lethal contact)

**DEATH_TRIGGER** (004640):
```
BIS #2000, 177700(R3)    — setează "dying" bit pe tile-ul player
```

**PLAYER_DEATH** (001016):
```
JSR PC, @#010206         — LEVEL_RESET: death animation + reinitializare nivel
```

**LEVEL_RESET** (010206): joacă animația de moarte, scade o viață, reinitializează entitățile, redă nivelul.

---

## 11. Condiția de victorie / trecere la nivel

**Trigger victorie:** state player devine `6` (exit tile atins). PLAYER_STATE_CHECK:
```
CMPB #6, (R3)
BNE → skip
MOVB #020, (R3)          — set state = 020 (16)
MOV @#017424, R5
JSR @#012716             — ENEMY_RESPAWN(enemy1)
MOV @#017426, R5
JSR @#012716             — ENEMY_RESPAWN(enemy2)
JSR @#002060             — handler (probabil LEVEL_COMPLETE)
```

**LEVEL_COMPLETE** (001034):
```
JSR PC, @#013524         — rebuild collision map (pentru noul nivel)
MOV (R5), @#001302       — actualizează entity pointer din table
ADD #540, @#001300       — avansează map pointer cu 352 bytes (un nivel)
ADD #2, @#001304         — avansează level table pointer
CMP #001302, @#001304    — am parcurs toate 10 niveluri?
BEQ → ALL_LEVELS_DONE    → JMP 001000 (restart complet)
JSR PC, @#012442         — LEVEL_RENDER_FULL (desenează noul nivel)
JMP @#001344             — GAME_LOOP
```

**10 niveluri** → la terminarea ultimului: restart joc de la 001000 (GAME_INIT).

---

## 12. Logica inamicilor (AI chase)

**Throttling:** Fiecare inamic are un contor de throttle (`VAR_ENEMY1_TICK_CTR` = 017372 etc.).

`ENEMY2_TICK` (006552):
```
CMP #400, @#017376      — counter == 0o400 (256 dec)?
BNE → increment + return — nu: crește counter și iese
→ counter == 256: execută mutarea
```
Inamicul se mișcă **1 dată la 256 de frame-uri** = la viteza maximă (difficulty 4), o mutare la ~256/50Hz ≈ 5 sec? Nu — DELAY_SPIN adaugă extra delay, deci ritmul real depinde de ambele.

**ENEMY2_MOVE** (006602) — algoritm chase:
1. `R0 = enemy2_tile_ptr - 014550` (offset inamic în tile buffer)
2. `R1 = player_tile_ptr - 014550` (offset player în tile buffer)
3. `BIC #177700, R0` → bits 0-5 = **coloana** inamic (offset mic = poziția în rândul curent)
4. `CMP R0, R1` → dacă inamic_col == player_col: **mișcare verticală** (urmărire pe coloană)
5. Dacă inamic_col ≠ player_col → **BGT / BLT** → mișcare stânga sau dreapta
6. Verificare coliziune: `BIT #100000, (R1)` — dacă tile destinație are flag solid: skip (blochează)
7. Verificare overlap cu enemy1: `CMP R1, @#14432` — dacă destinație == enemy1 tile: skip (inamicii nu se suprapun)

**3 inamici:** Enemy1, Enemy2, Enemy3 — fiecare cu propria rutină tick + contor throttle. Inamicii urmăresc player-ul pe tile grid cu algoritm greedy (nu pathfinding complet).

---

## 13. Sunet

Motorul de sunet (`ENTITY_HANDLER` / ENT_SPEAKER_LOOP la 006366) generează ton prin toggle bit 7 al `@#177716` (REG_SYSREG):
- Frecvența = controlată de tabelele de date din 020450–021777
- Burn loop intern pentru durata tonului
- `[УКНЦ DIFF]`: valorile HIGH/LOW pentru speaker diferă față de BK-0010 (același registru 177716, constante diferite din cauza clock-ului diferit al УКНЦ)

---

## Note și incertitudini

- **Starea 6 (exit):** din cod, state=6 triggere ENEMY_RESPAWN + handler (002060). Confirmat că 002060 duce spre LEVEL_COMPLETE, dar codul de la 002040-002060 nu a fost citit complet.
- **Tile index 8 ca prag:** `CMPB #10, ...` = comparație cu 010 octal = 8 decimal. Tiles 0–7 = pasabile, tiles 8–15 = solid. Tile 7 = apă (pasabilă dar letală).
- **Vieți:** `GAME_INIT` scrie `#333` (octal = 219 decimal) în `VAR_LIVES` (017436). Decrementul e `SUB #1` per moarte, deci tehnic există 219 "vieți". Probabil valoarea e afișată trunchiată sau NUM_RENDER afișează mod 10 → jocul arată "9" inițial. Alternativ: un joc educațional sovietic putea oferi intenționat multe vieți pentru copii. Confirmat: LIVES==0 → `JMP @#003576` (GAME_OVER).
- **Zona 027200–031277:** necitită complet — poate conține date AI suplimentare sau tabele de nivel.
