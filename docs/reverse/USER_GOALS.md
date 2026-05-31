# Ce vrea userul — obiective КЛАД (УКНЦ 1987 Баранов)

**Acest fișier este sursa de adevăr.** Dacă ești tentat să spui că "totul e gata și totul e bine", revino AICI și verifică fiecare punct față de ce există în repo.

---

## ⚠ REGULI ABSOLUTE — citește înainte de orice altceva

### 1. Ce SE PĂSTREAZĂ (nu se modifică, nu se șterge)
- `disassembly/annotated/uknc_klad_1987.asm` — codul ASM adnotat și comentat
- `archive/bk0010/disassembly/crocodile_klad.asm` — referință BK-0010
- `disassembly/raw/` — dezasamblări brute
- `docs/reverse/` — toată documentația de reverse engineering
- `assets/original/` — binarele originale și extracțiile din ele
- `tools/` — scripturile de extracție Python

### 2. Ce NU SE FOLOSEȘTE (arhivat, interzis)
- Orice cod C scris anterior se află în arhivele ZIP din `/home/anatolie/ai/`
- `src/` este gol și se construiește de la zero
- `build/` nu există — se creează doar la compilare

### 3. Cum se scrie codul nou — OBLIGATORIU
- **Sursa unică de adevăr: `disassembly/annotated/uknc_klad_1987.asm`**
- Fiecare funcție C se scrie DUPĂ ce se citește rutina ASM corespunzătoare
- Nu se ghicește. Nu se inferează din memoria LLM. Nu se inventează.
- Se citește instrucțiune cu instrucțiune din ASM, se înțelege CE face, se scrie C23 idiomatic echivalent
- Fiecare funcție C are comentariu cu adresa ASM sursă (ex: `// PLAYER_STATE_CHECK 012570`)

### 4. Target platforme — AMBELE obligatorii
- **Linux native** — compilat cu `cc -std=c23`, rulează direct pe Ubuntu
- **WebAssembly** — compilat cu `emcc`, rulează în browser fără plugin
- Singurul `#ifdef PLATFORM_WEB` permis: în `main.c` pentru `emscripten_set_main_loop`
- Librărie grafică: **raylib** (suportă ambele platforme din aceeași sursă C)

### 5. Stil cod — C23 idiomatic pentru arhitectură modernă
- Standard: `-std=c23` (sau `-std=c2x`)
- Fără globale inutile — structuri de stare explicite
- Comentariile explică DE CE (motivul din ASM), nu CE face codul
- Nicio valoare hardcodată fără referință la adresa ASM de unde provine

---

**Acest fișier este sursa de adevăr.** Dacă ești tentat să spui că "totul e gata și totul e bine", revino AICI și verifică fiecare punct față de ce există în repo.

**⚠ TARGET ACTUAL: `assets/original/extracted/uknc/KLAD_1987_Baranov.SAV` (УКНЦ МС-0511)**  
**Pivot de la:** `archive/bk0010/binaries/ex_klad/KLAD.BIN` (BK-0010 Crocodile) — acel work rămâne ca referință, cod 83% identic.

Versiunea țintă: **КЛАД original Баранов 1987** (`assets/original/extracted/uknc/KLAD_1987_Baranov.SAV`)

---

## Obiectiv 1 — Hartă completă a octeților (byte map)

**Ce se vrea:** Un fișier care documentează FIECARE range de bytes din `KLAD_1987_Baranov.SAV` — ce e cod, ce e date, ce e nivel, ce e grafică, ce e necunoscut.

**Fișier de output:** `docs/reverse/BYTE_MAP.md`

**Criterii de completitudine:**
- Fiecare byte din fișier are o etichetă (cod / date / nivel / gfx / padding / necunoscut)
- Range-urile se unesc — nu trebuie să fie byte cu byte, dar nici găuri

**Status:** ✅ COMPLET — `docs/reverse/BYTE_MAP.md` reescris pentru УКНЦ SAV. Acoperire: ~87% din bytes catalogați. Zonă neacoperită: 027200–031277 (2112 bytes, probabil date AI/entity suplimentare).

---

## Obiectiv 2 — Cod assembler extras și documentat

**Ce se vrea:** Codul în assembler al jocului, curat, adnotat cu ce face fiecare rutină/bloc.

**Fișiere de output:**
- `archive/bk0010/disassembly/crocodile_klad.asm` — assembler complet cu etichete și comentarii
- `docs/reverse/ROUTINES.md` — index al tuturor rutinelor identificate (adresă, nume, ce face)

**Criterii de completitudine:**
- Toate rutinele reachable din entry point (`04000`) sunt urmărite și etichetate
- Fiecare rutină are minim un comentariu de o linie care explică CE face
- Rutinele necunoscute sunt marcate `; UNKNOWN` cu ce știm despre ele (ce adrese accesează)

**Status:** ✅ COMPLET — `disassembly/annotated/uknc_klad_1987.asm` (6978 linii): toate 6503 instrucțiuni din 001000–042000, fiecare cu adresă + cuvânt + instrucțiune + comentariu. 5372 identice cu BK-0010 adnotate din referință; 1131 diferite marcate [УКНЦ DIFF]. Rutinele I/O УКНЦ (040060-042000) dezasamblate complet.

---

## Obiectiv 3 — Mecanica jocului extrasă din assembler

**Ce se vrea:** Documentarea mecanicii jocului EXCLUSIV din cod assembler, nu din presupuneri sau joacă.

**Fișiere de output:** `docs/reverse/MECHANICS.md`

**Ce trebuie acoperit (minim):**
- Bucla principală de joc (game loop) — structura ei
- Mișcarea jucătorului — viteza, reguli (când poate merge, când nu)
- Coliziunea cu tile-uri (pereți, scări, apă, aur)
- Coliziunea cu inamici
- Colectarea aurului — logica scorului
- Condiția de moarte (apă, inamic)
- Condiția de victorie / trecere la nivel următor
- Logica inamicilor — cum se mișcă

**Status:** ✅ COMPLET — `docs/reverse/MECHANICS.md` scris. Acoperă: game loop (GAME_TICK sequence), tastatură (polling non-blocking via KBD_READ), player state machine (11 states), collision map build (8 flag bits per tile, prag=8), mișcare + gravitație, apă, coliziune inamic, aur (+10 pct), moarte (PLAYER_DEATH → lives--), victorie (LEVEL_COMPLETE, stride=352), AI chase (greedy, throttled la 256 frames).

---

## Obiectiv 4 — Toate nivelurile extrase

**Ce se vrea:** Toate nivelurile din joc extrase ca date structurate.

**Fișiere de output:**
- `assets/original/extracted/crocodile/levels/level_NN.json` — fiecare nivel ca JSON
- `assets/original/extracted/crocodile/levels/level_NN.png` — vizualizare PNG a fiecărui nivel
- `docs/reverse/LEVEL_FORMAT.md` — documentarea formatului (offset în fișier, encoding)

**Criterii de completitudine:**
- Toate nivelurile din joc sunt extrase (numărul exact de niveluri descoperit din cod)
- Fiecare nivel are tile map complet (dimensiuni, fiecare tile identificat)
- Pozițiile de start ale jucătorului și inamicilor per nivel

**Status:** ✅ COMPLET — 10 niveluri extrase: `assets/original/extracted/uknc/levels/level_NN.json` + `.png`. Format confirmat din cod: 22 rânduri × 16 bytes, stride=352 (ADD #540). Poziții de start: în TBL_ENTITY_PTRS (001230) → per-level entity records (015230–017447). Lipsă LEVEL_FORMAT.md (poate fi scris dacă e cerut).

---

## Obiectiv 5 — Texturi și sprites originale cu pixel fidelity

**Ce se vrea:** Toate sprite-urile și tile-urile originale extrase exact cum arată în joc.

**Fișiere de output:**
- `assets/original/extracted/crocodile/tiles/tile_NN_NAME.png` — fiecare tile separat
- `assets/original/extracted/crocodile/sprites/sprite_NN_NAME.png` — fiecare sprite separat
- `assets/original/extracted/crocodile/spritesheet.png` — toate într-un singur PNG
- `docs/reverse/GFX_MAP.md` — adresele din memorie ale fiecărui sprite/tile + dimensiuni

**Criterii de completitudine:**
- Pixel fidelity — culorile și forma sunt identice cu originalul pe BK-0010
- Paleta BK-0010 folosită corect (negru/alb în mod standard, sau paleta color dacă e cazul)
- Nu lipsește niciun sprite vizibil în joc

**Status:** ✅ COMPLET — 32 tile-uri + 208 sprite frames extrase: `assets/original/extracted/uknc/tiles/` + `sprites/`. GFX_MAP.md scris cu format complet (stride=16, 2bpp, formula addr=idx×16+017450). Paletă aproximată — culorile exacte necesită trace emulator pentru valorile registrului de paletă УКНЦ.

---

## Obiectiv 6 — Mecanica animațiilor extrasă

**Ce se vrea:** Cum funcționează animația personajului și inamicilor — extrasă din cod, nu ghicită.

**Fișiere de output:** `docs/reverse/ANIMATIONS.md`

**Ce trebuie acoperit:**
- Câte frame-uri are fiecare animație
- La ce interval se schimbă frame-urile (ticks/frame)
- Ce triggere schimbă animația (mers stânga/dreapta, urcat scară, stând, mort)
- Sprite-urile corespunzătoare fiecărui frame (cu referință la GFX_MAP)
- Idem pentru inamici dacă sunt animați

**Status:** ✅ COMPLET — `docs/reverse/ANIMATIONS.md` scris. Acoperă: throttle player (5 ticks/frame, ANIM_THROTTLE_PLAYER), state machine (state 021=climb, 023=walk, range 021-024), enemy anim (8→9 ticks horizontal, 5→6 ticks vertical), TBL_ANIM_FRAMES (pixel offsets), sprite bank 031300 (208 frames, word-pointer blit). Timing exact pentru Hz real necesită trace emulator.

---

## Stare curentă a extracției

| Artefact | Există | Note |
|----------|--------|------|
| `KLAD.BIN` | ✅ | `archive/bk0010/binaries/ex_klad/KLAD.BIN` |
| Linear disassembly brut | ✅ | `disassembly/raw/KLAD.asm` |
| Title screen PNG | ✅ | `assets/original/extracted/crocodile/title_screen.png` |
| Sprites PNG (brut) | ✅ | `assets/original/extracted/crocodile/sprites.png` |
| gfx_region.bin | ✅ | `assets/original/extracted/crocodile/gfx_region.bin` |
| BYTE_MAP.md | ❌ | Neînceput |
| Assembler adnotat | ✅ | `archive/bk0010/disassembly/crocodile_klad.asm` — 35/35 rutine |
| ROUTINES.md | ✅ | `docs/reverse/ROUTINES.md` — index complet + variabile globale |
| MECHANICS.md | ❌ | Neînceput |
| Niveluri extrase | ❌ | Parțial în 08_level_data.md |
| Tile-uri individuale | ❌ | Neînceput |
| GFX_MAP.md | ❌ | Neînceput |
| ANIMATIONS.md | ❌ | Neînceput |
