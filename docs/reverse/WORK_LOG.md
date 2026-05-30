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
