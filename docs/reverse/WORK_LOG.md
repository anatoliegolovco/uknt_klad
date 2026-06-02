# Work Log — Reverse-engineering Crocodile КЛАД

Format: `[YYYY-MM-DD HH:MM UTC] STATUS — descriere`

Statusuri: `DONE` / `IN_PROGRESS` / `BLOCKED` / `CRASHED` / `RESUMED`

---

## 2026-05-29

`[2026-05-29 ~14:00 UTC] DONE` — Extracție grafică via BK CPU core real  
- Tools adăugate: `tools/bk_unpack_harness.c`, `tools/render_crocodile.py`, `tools/extract_crocodile.sh`  
- Artefacte: `assets/original/extracted/crocodile/title_screen.png`, `sprites.png`, `gfx_region.bin`  
- Commit: `93e4c7c`

---

## 2026-05-31

`[2026-05-31 UTC] DONE` — КЛАД 1987 Баранов rulează în emulatorul УКНЦ REAL (color) + referințe capturate

**Emulator УКНЦ funcțional (QtUkncBtl):**
- AppImage: `github.com/nzeemin/ukncbtl-qt` release preview-468, extras în `/tmp/squashfs-root/`
  (FUSE indisponibil → `./QtUkncBtl.AppImage --appimage-extract`, rulează `./AppRun`)
- ROM УКНЦ inclus în build (`emulator/uknc_rom.bin`) — nu necesită ROM separat
- Lansare directă în КЛАД:
  `./AppRun "-disk0:<abs>/fodos_games.dsk" -autostart -boot1`
  (OPTIONCHAR pe Linux = `-`, NU `/`). Apoi la promptul ФОДОС: tastează `R KLAD`.
- **Input:** `xdotool key/type` FĂRĂ `--window` (XTEST, evenimente trusted). Cu `--window`
  Qt ignoră (XSendEvent sintetic). Click pe ecranul emulatorului întâi pt focus.
- **Screenshot:** `tools/shot.py <WID> out.png` (citește măștile XWD → culori corecte).
- Disk КЛАД: `fodos_games.dsk`, fișier `KLAD.SAV` (confirmat din `DIR`).

**Constatări GROUND TRUTH din emulator (corectează presupuneri anterioare):**
1. **Vieți = 219** afișat literal ("Попытки 219"), NU "9". VAR_LIVES=0o333=219 se
   afișează ca număr zecimal complet. Reimplementarea C arată "9" → GREȘIT.
2. **Player = omuleț alb (красный человечек)** — figură umanoidă ~8px, NU crocodil,
   NU pătrat. Vezi `reference_emu/player_zoom.png`.
3. **Scări = șine verticale + trepte** (rails+rungs), NU stâlp solid. Confirmă
   `TILE_FIDELITY.md` (decode planar greșit upstream).
4. УКНЦ randează color (fundal albastru, mod GRB); pe monitoare mono de școală =
   tonuri de gri. Userul a jucat B&W — forma contează, nu culoarea.

**Referințe salvate:** `assets/uknc/reference_emu/`
(title.png, level1_full.png, hud_and_field.png, player_zoom.png)

`[2026-05-30 UTC] RESUMED` — User a raportat crash. Context recuperat din git log + starea repo.

`[2026-05-30 UTC] DONE` — Creat `docs/reverse/USER_GOALS.md` cu cele 6 obiective exacte ale userului.

`[2026-05-30 UTC] DONE` — Creat `docs/reverse/WORK_LOG.md` (acest fișier) pentru continuitate după crash.

`[2026-05-30 UTC] DONE` — Actualizat `CLAUDE.md` cu secțiunea de obiective utilizator și referințe la log.

---

## 2026-05-30 (continuat)

`[2026-05-30 13:00 UTC] DONE` — Confirmat emulator funcțional (MAME bk001001)  
- ROM-uri BK-0010 descărcate de pe archive.org → `~/mame/roms/bk0010.zip`  
- Mecanism de injecție Lua creat: `tools/bk_inject.lua` injectează binar la frame 180 (3s)  
- `ex_klad/KLAD.BIN` rulează în MAME — title screen КЛАД cu diamond-mesh walls confirmat  
- Convertor BIN→WAV creat (`/tmp/bkbin2wav.py`) pentru referință viitoare  
- bk0010 (fără -01) este BROKEN în MAME 0.264; varianta funcțională = `bk001001`

`[2026-05-30 14:30 UTC] IN_PROGRESS` — Obiectiv 2: assembler adnotat — prima trecere (25 rutine)

`[2026-05-30 17:00 UTC] DONE` — Obiectiv 2 BK-0010: **COMPLET** — 35/35 rutine adnotate
- `disassembly/annotated/crocodile_klad.asm` — 1393 linii, toate rutinele acoperite
- `docs/reverse/ROUTINES.md` — index complet 35 rutine, toate variabilele globale, tile values, key table
- Descoperiri noi: ENTITY_HANDLER (006204) = sound engine (toggle @#177716 bit 7 = speaker)
- Structura completă: 3 inamici independenți cu throttle-uri separate (17360/17366/17372/17376)
- PLAYER_TILE_PTR (014422) = pointer în working buffer @#14550 (nu poziție directă)
- COLLISION_MAP_BUILD (013524) = unpack + 8 tipuri de flag-uri coliziune per tile
- Tabele taste complete: meniu (001732) + gameplay (012342) cu toate mapările
- **Next:** Obiectiv 1 (BYTE_MAP) sau Obiectiv 3 (MECHANICS)

`[2026-05-30 13:00 UTC] DONE` — Identificat corect versiunea Crocodile  
- `ex_klad/KLAD.BIN` = Crocodile 1991 (packed, load=0732, entropy=7.42)  
- Titlul arată „Николаев 1987 Баранов" = creditul originalilor autori, nu branding Crocodile  
- KLAD2/KLAD4/klad10 = alte versiuni packed (probabil secvele)  
- KLAD3.BIN = necomprimat, load=01000, entropy=5.66 (versiunea Баранов neambalată)  
- **Next:** Obiectiv 2 — disassembly adnotat din entry 01000 (depacker) → joc

## 2026-05-30 (sesiunea УКНЦ search)

`[2026-05-30 17:30 UTC] DONE` — Găsite și extrase ambele versiuni УКНЦ ale jocului КЛАД

**Sursă 1 — versiunea Баранов 1987 (ORIGINAL):**
- Fișier: `KLAD.SAV` (17408 bytes = 34 blocuri RT-11)
- Sursa disk: `hobot.pdp-11.ru/ukdwk_archive/ukncbtlwebcomplekt/FODOS_GAMES/disk_10_fix.dsk`
- Confirmat prin string-uri în KOI8-R în binar: `"Николаев 1987"`, `"Баранов"`, `"Д.Г."`
- Text intro complet: `"КЛАД"`, instrucțiuni în rusă, speed selector `1,2,3,4`
- Salvat la: `assets/uknc/KLAD_1987_Baranov.SAV`

**Sursă 2 — versiunea Crocodile Software 1991:**
- Fișier: `MKLAD.GAM` (19968 bytes = 39 blocuri RT-11)
- Sursa disk: `hobot.pdp-11.ru/ukdwk_archive/ukncbtlwebcomplekt/UKNCgames_NEW/newgames.dsk`
- Confirmat prin string ASCII: `"@ 1991 CROCODILE SOFTWARE"`
- Conține 18 level names (transliterate rusă): podzemelxe, zmea, nachalo, uhvati, lestnica, etc.
- Salvat la: `assets/uknc/MKLAD_1991_Crocodile.GAM`

**Disk images complete:**
- `assets/uknc/fodos_games.dsk` (800K — sursa KLAD_1987)
- `assets/uknc/newgames.dsk` (800K — sursa MKLAD_1991)

**Metodă de căutare:**
- Yandex.ru → hobot.pdp-11.ru → galerie jocuri УКНЦ
- Parser RT-11 custom (Python) cu status 0x8400/0x0400 pentru permanent files
- Brute-force RAD50 search în 134 imagini de dischetă
- Confirmat din tis.kz (arhiva Novosibirsk = soft educațional, fără jocuri КЛАД)

## 2026-05-30 (pivot УКНЦ)

`[2026-05-30 18:00 UTC] DONE` — Pivot target: BK-0010 → УКНЦ МС-0511
- Găsite ambele versiuni УКНЦ: KLAD_1987_Baranov.SAV + MKLAD_1991_Crocodile.GAM
- Analiză binară: УКНЦ 1987 e 83% identic cu BK-0010 (7080/8448 cuvinte)
- BK-0010 Crocodile = repack LZ al aceluiași cod УКНЦ 1987
- Diferențele: keyboard (@#040546 nu @#177714), video (port @#176640 nu FB direct), vsync (041400 nu EMT 016)
- Creat `disassembly/annotated/_uknc1987_raw.asm` (6505 linii) + `uknc_klad_1987.asm`
- Actualizat CLAUDE.md + USER_GOALS.md cu noul target
- **Next:** Obiectiv 2 УКНЦ (completare adnotare rutine I/O УКНЦ pas cu pas)

## Template pentru intrări viitoare

```
`[YYYY-MM-DD HH:MM UTC] IN_PROGRESS` — Obiectiv N: <ce se lucrează>
`[YYYY-MM-DD HH:MM UTC] DONE` — Obiectiv N: <ce s-a terminat> — artefacte: <lista fișiere>
`[YYYY-MM-DD HH:MM UTC] BLOCKED` — Obiectiv N: <de ce e blocat> — next step: <ce trebuie făcut>
`[YYYY-MM-DD HH:MM UTC] CRASHED` — Ultimul lucru în progres era: <descriere> — de reluat de la: <punct>
```

---

## Checkpoint pentru crash-recovery

La fiecare crash, citește în ordine:
1. `docs/reverse/USER_GOALS.md` — ce vrea userul (sursa de adevăr)
2. `docs/reverse/WORK_LOG.md` — ultima intrare spune unde s-a oprit
3. `git log --oneline -5` — confirmă ce s-a commis
4. Statusurile din tabelul din USER_GOALS.md — ce e ❌ = de făcut

## 2026-05-30 (sesiunea extracție grafică УКНЦ)

`[2026-05-30 UTC] DONE` — Obiectiv 5 + 1 + 4: extracție completă tile-uri, sprite-uri, niveluri  
- **Descoperire critică**: tile stride = 16 bytes (nu 8 cum era etichetat) — confirmat din cod: 4× ASL R2 (=×16) + ADD #17450  
- **Tile format**: 8 pixel-plane + 8 colour-plane, 8×8px 2bpp, (pixel_bit,colour_bit) → 4 culori  
- **Level stride**: 352 bytes = 22×16, confirmat din LEVEL_COMPLETE: `ADD #540, @#CUR_MAP_ADDR`  
- Corectate 9 adrese de nivel greșite în uknc_klad_1987.asm (aveau stride ~368 în loc de 352)  
- Corectate labels tile-uri individuale în asm (stride 8→16)  
- Artefacte: `tiles/` (32 PNG + tileset.png), `sprites/` (208 PNG + spritesheet.png), `levels/` (10 JSON + 10 PNG)  
- Docs: `GFX_MAP.md` + `BYTE_MAP.md` (УКНЦ, ~87% coverage)  
- Script: `tools/extract_uknc_gfx.py`  
- Commit: `6db6aaf`  
- **Next:** Obiectiv 3 (MECHANICS.md) sau Obiectiv 6 (ANIMATIONS.md)


`[2026-05-31 UTC] BUG` — User: coliziune verticală pe scară defectă — playerul urcă
PRIN podea/tavan în sus. Cauză: în on_ladder()/player_update mișcarea verticală pe
scară nu verifică coliziunea (vy=0; py += dir*speed, fără check tile destinație).
De reparat din PLAYER_MOVE_STEP (012740) + flagurile COLLISION_MAP_BUILD (013570).

`[2026-05-31 UTC] FIX` — Coliziune verticală scară: on_ladder() acum cere celula
centrală = scară (nu ry±1). Mișcarea verticală trece prin can_climb_into():
permite scară/gol, trece prin zid DOAR dacă scara continuă dincolo (climb-through
platforme ca în КЛАД), altfel blochează. Rebuild + pornit versiunea nouă.

`[2026-05-31 UTC] REVERT+OBSERVE` — User feedback: decodarea two-halves a stricat
scările (trepte rare), podurile (jumătate) și aurul. Revenit la 2r,2r+1 (mai bună).
Observat texturi originale (reference_emu): SCARA = 2 șine SUBȚIRI apropiate + trepte
FRECVENTE (~la 3-4px); AURUL = UN cufăr. NICIO decodare simplă din octeți nu
reproduce exact aceste texturi → următorul pas: EXTRAGERE tile-uri direct din
ecranul randat al emulatorului (singura cale fidelă). Format video УКНЦ prea complex
pt decodare statică din octeți. Coliziune: can_climb_into() acum scanează prin
platforme consecutive (trecere prin "două tavane").

## 2026-06-01 (sesiunea culori scări — firmware УКНЦ)

`[2026-06-01 UTC] DONE` — Culori scări/stairs: investigat firmware УКНЦ + capturi emulator color mode.
- **Concluzie**: jocul NU încarcă o paletă custom. Culoarea e atribut per-scanline:
  blit-ul (`DISP_SCANLINE_WRITE` 040060+) scrie bitmap-ul la `@#176640` apoi octetul
  colour-plane la `@#176642/3` (`MOVB (R2),@#176642` / `MOVB (R2)+,@#176643`). Niciun
  palette-latch load în 001000–041777 → render-ul color al emulatorului ESTE paleta ground-truth.
- **Scările = ALB pe ALBASTRU** (2 șine subțiri + trepte frecvente). Podea/apă = GALBEN.
  Confirmat din `reference_emu/compare/level1_emulator.png`, `ladder_emulator_zoom.png`,
  `sprites/uknc_emu_gameplay_render.png`.
- **Reimplementarea deja se potrivește**: `src/render.c` PAL_COLOR={bg(0,0,255), fg(255,255,255)}
  + render_water() galben (236,204,64). Niciun fix de culoare necesar.
- Docs: GFX_MAP.md secțiunea "Colour model — RESOLVED" + Known Limitations #1 marcat RESOLVED.
- **Next:** dacă se dorește fidelitate de FORMĂ scară (șine mai subțiri + trepte mai dese în
  gfx_data tile 1) — separat de culoare; sau exercitat build-ul WebAssembly.

## 2026-06-02 (sesiunea webfont + subpagină font + mobile landscape)

`[2026-06-02 UTC] DONE` — Font web-servabil + subpagină dedicată + folder font/ regândit:
- `tools/gen_webfont.py` = packager unic: din `font/uknc_font.h` (sursa de adevăr, 99 glife)
  generează TOATE formatele consistent: uknc.{ttf,woff2,woff,bdf} + uknc_font.{json,png}.
  Webfont vectorial (pixel→pătrate, runs orizontale unite) → clar la orice mărime.
- Reparate inconsistențe: .bdf/.json/.png erau stale (45 glife) → acum 99, la fel ca .h.
- Subpagină `web/font/index.html` → servită la /font/ (hero live în font, grilă glife,
  download-uri, @font-face howto). Hero folosește doar glife acoperite (lipsesc majuscule Л/Д).
- Workflow Pages deployează acum și /font/ + asseturile font.
- `font/README.md` rescris: single source of truth + toate formatele derivate.

`[2026-06-02 UTC] DONE` — Play page mobile landscape (observat cu Playwright pe live):
- Portret = jocul (lat ~2.24:1) apărea bandă mică. Acum canvas rotit 90° în portret (CSS
  `@media (orientation:portrait)`) → umple ecranul ca landscape. Landscape rămâne nerotat, încadrat.
- Touch: shell-ul (`src/web/shell.html`) preia touch-ul (capture + stopImmediatePropagation ca
  emscripten să nu-l mai vadă) și injectează taste săgeți, cu zone rotite corect în portret.
  Verificat că tastele sintetice mișcă playerul (Playwright). `src/game.c` touch nativ păstrat
  sub `#ifndef PLATFORM_WEB`.

## 2026-06-02 (verificare ipoteză coliziune pod — cod + E2E headless)

`[2026-06-02 UTC] DONE` — Ipoteză user: „mergi pe pod → fără gravitație; cazi peste pod →
treci prin el". Verificat pe DOUĂ căi independente:
- **Cod** (CMAP_FLAGS 013724–013756): flag sprijin #10000 setat dacă tile_jos==8 SAU
  (tile_curent==8 ȘI tile_jos≤6). A doua clauză = garanție anti-trecere-prin (pod suspendat
  cu aer dedesubt te sprijină tot).
- **E2E headless** (mod `bridge` adăugat în `vendor/ukncbtl-qt/headless/main.cpp`): citește CPU
  RAM via GetCPUMemoryController()->GetWordView, teleportează jucătorul deasupra unui tile și
  urmărește pointerul celulă 014422 sub gravitație. Drop pe tile8 și tile12 → ATERIZEAZĂ
  deasupra; stat pe tile8 → sprijinit.
- **Concluzie: Partea 1 CONFIRMATĂ (sprijinit), Partea 2 RESPINSĂ (NU trece prin pod).**
- Documentat în MECHANICS.md §6.1.

`[2026-06-02 UTC] DONE` — Verificat ipoteza chests stânga-jos (KI-14). REZULTAT: COLECTABILE
(KI-14 anterior era greșit). E2E headless `chest`: de pe scara c4 (r10) → pas dreapta în c5 →
cădere dreaptă prin apa MICĂ (tile7 @r13, ne-letală, vieți rămân 219) → aterizare r15 c5 → pas
STÂNGA → gold@(15,4) 5→0 (colectat); la fel (17,4) 4→0. Nu e nevoie de control în cădere.
Corectat KNOWN_ISSUES KI-14. Adăugate moduri E2E `chest` + `reach` în headless main.cpp.

`[2026-06-02 UTC] DONE` — Animație: condus jocul ORIGINAL prin comenzi injectate (mod `play`
în headless main.cpp: script "R20 U50 R7 ..." → taste reale, capturi PPM → GIF). Găsit:
spawn = sus-stânga într-un puț; teleport NU mută sprite-ul (doar logica) → animația corectă
necesită navigare reală. Hostat GIF la /demo/klad_original_play.gif + link pe landing.
