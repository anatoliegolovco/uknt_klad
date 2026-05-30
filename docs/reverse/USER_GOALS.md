# Ce vrea userul — obiective reverse-engineering КЛАД (УКНЦ 1987 Баранов)

**Acest fișier este sursa de adevăr.** Dacă ești tentat să spui că "totul e gata și totul e bine", revino AICI și verifică fiecare punct față de ce există în repo.

**⚠ TARGET ACTUAL: `assets/original/extracted/uknc/KLAD_1987_Baranov.SAV` (УКНЦ МС-0511)**  
**Pivot de la:** `assets/original/ex_klad/KLAD.BIN` (BK-0010 Crocodile) — acel work rămâne ca referință, cod 83% identic.

Versiunea țintă: **КЛАД original Баранов 1987** (`assets/original/extracted/uknc/KLAD_1987_Baranov.SAV`)

---

## Obiectiv 1 — Hartă completă a octeților (byte map)

**Ce se vrea:** Un fișier care documentează FIECARE range de bytes din `KLAD_1987_Baranov.SAV` — ce e cod, ce e date, ce e nivel, ce e grafică, ce e necunoscut.

**Fișier de output:** `docs/reverse/BYTE_MAP.md`

**Criterii de completitudine:**
- Fiecare byte din fișier are o etichetă (cod / date / nivel / gfx / padding / necunoscut)
- Range-urile se unesc — nu trebuie să fie byte cu byte, dar nici găuri

**Status:** ❌ De refăcut pentru УКНЦ SAV (BK-0010 BYTE_MAP complet dar target s-a schimbat)

---

## Obiectiv 2 — Cod assembler extras și documentat

**Ce se vrea:** Codul în assembler al jocului, curat, adnotat cu ce face fiecare rutină/bloc.

**Fișiere de output:**
- `disassembly/annotated/crocodile_klad.asm` — assembler complet cu etichete și comentarii
- `docs/reverse/ROUTINES.md` — index al tuturor rutinelor identificate (adresă, nume, ce face)

**Criterii de completitudine:**
- Toate rutinele reachable din entry point (`04000`) sunt urmărite și etichetate
- Fiecare rutină are minim un comentariu de o linie care explică CE face
- Rutinele necunoscute sunt marcate `; UNKNOWN` cu ce știm despre ele (ce adrese accesează)

**Status:** 🔄 PARȚIAL — BK-0010 complet (35/35 rutine în `crocodile_klad.asm`). УКНЦ: `uknc_klad_1987.asm` creat cu diferențele I/O documentate; structura identică e cross-referențiată. Rutinele УКНЦ I/O (040060-042000) adnotate la nivel de bloc, nu instrucțiune cu instrucțiune.

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

**Status:** ❌ Neînceput

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

**Status:** ❌ Parțial (avem `docs/reverse/08_level_data.md` cu format de bază, fără extracție completă)

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

**Status:** ❌ Parțial (avem `sprites.png` și `title_screen.png` extras, dar fără catalog complet + GFX_MAP)

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

**Status:** ❌ Neînceput

---

## Stare curentă a extracției

| Artefact | Există | Note |
|----------|--------|------|
| `KLAD.BIN` | ✅ | `assets/original/ex_klad/KLAD.BIN` |
| Linear disassembly brut | ✅ | `disassembly/raw/KLAD.asm` |
| Title screen PNG | ✅ | `assets/original/extracted/crocodile/title_screen.png` |
| Sprites PNG (brut) | ✅ | `assets/original/extracted/crocodile/sprites.png` |
| gfx_region.bin | ✅ | `assets/original/extracted/crocodile/gfx_region.bin` |
| BYTE_MAP.md | ❌ | Neînceput |
| Assembler adnotat | ✅ | `disassembly/annotated/crocodile_klad.asm` — 35/35 rutine |
| ROUTINES.md | ✅ | `docs/reverse/ROUTINES.md` — index complet + variabile globale |
| MECHANICS.md | ❌ | Neînceput |
| Niveluri extrase | ❌ | Parțial în 08_level_data.md |
| Tile-uri individuale | ❌ | Neînceput |
| GFX_MAP.md | ❌ | Neînceput |
| ANIMATIONS.md | ❌ | Neînceput |
