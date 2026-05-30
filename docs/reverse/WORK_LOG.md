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

## 2026-05-30

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

`[2026-05-30 17:00 UTC] DONE` — Obiectiv 2: **COMPLET** — 35/35 rutine adnotate
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
