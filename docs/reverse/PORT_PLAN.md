# PORT_PLAN — faithful 1:1 transposition of КЛАД 1987 to C23

Goal: make `src/` a **faithful 1:1 port** of the original КЛАД 1987 (Баранов, УКНЦ), screen
for screen, verified **side-by-side** against the headless emulator
(`vendor/ukncbtl-qt/headless`, see `docs/headless/README.md`). The headless original is the
visual ground truth; `tools/uknc_emu.c` + the E2E tests guard logic/collision.

> **Internationalisation (Romanian) is the LAST step** — scoped here, NOT started now. Only
> after the faithful 1:1 port (original Russian text) is complete and verified do we add a
> Romanian option via `i18n.h`, keeping the original strings intact. See task #6.

## Original flow (from ASM)

| Step | ASM | Screen | Our state (target) |
|------|-----|--------|--------------------|
| 1 | `TITLE_SEQ` 002072 | Title: big **КЛАД** tile-art + `Николаев 1987` + `Баранов` + school banner | `GS_TITLE` |
| 2 | `TITLE_WAIT` 002264 | wait for a key | (in `GS_TITLE`) |
| 3 | `DIFF_SELECT` 003234 / `KEY_DIFFICULTY` 001142 | Speed select **1–4** (1=fast … 4=slow), sets `VAR_SPEED` | `GS_SPEED_SELECT` |
| 4 | `GAME_INIT` 004000 | start: lives `0o333`=219, score 0, level 1 | `game_init` |
| 5 | `GAME_LOOP`/`GAME_TICK` | gameplay (maze) | `GS_PLAYING` |
| 6 | `GAME_OVER_*` 001004/003274 | game over → back to title | `GS_GAME_OVER` |

Current `src/` starts straight in `GS_PLAYING` — title and speed-select are missing.

## Work items (faithful first, RO last)

1. **Scoping doc** (this file). ✅
2. **Side-by-side harness** — capture original (headless PPM→PNG) and our impl (raylib shot)
   for the same screen, compose side-by-side. `tools/compare_screens.sh`.
3. **Title screen** `GS_TITLE` — render big КЛАД + credits faithfully (original Russian text),
   advance on key. Verify side-by-side vs `reference_emu/headless/01_title.png`.
4. **Speed-select** `GS_SPEED_SELECT` — keys 1–4 → `VAR_SPEED`; wire speed into enemy/anim
   timing (`KEY_DIFFICULTY` sets the per-tick speed). Verify vs original.
5. **Gameplay parity pass** — tiles (ladder side-by-side / gold OR — already fixed), sprites,
   HUD (`Счет` / `Попытки 219`), level layout, death/level-win — all matched frame-by-frame.
6. **[LAST] i18n Romanian** — translate via `i18n.h`, keep Russian original selectable.

## Fidelity rules
- Text stays **original Russian** during the faithful port (RO comes last).
- Every screen validated against a headless capture before it's marked done.
- Logic/collision stays green in the E2E harness (`make -C src test`).
- Constants octal, lives `0o333`=219 (matches the original HUD "Попытки 219").
