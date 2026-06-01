# KNOWN ISSUES — КЛАД 1:1 port

Tracked deviations / open bugs. Each has a clear path to resolution; none blocks the build.

## KI-1 — РУС keyboard mode in the headless emulator
**What:** `vendor/ukncbtl-qt/headless` "glyphs" mode types the missing Cyrillic letters at
the ФОДОС prompt to capture their glyphs, but pressing **АЛФ (0106)** does not switch the
keyboard from ЛАТ to РУС — Latin still echoes (И→I, Ж→V, Ш→[, У→U). Б and 8 do come through.
**Impact:** can't yet capture И й ж з ш у Ч via typing. The font is missing those 6 glyphs.
**Path:** find the correct РУС-mode switch (likely not АЛФ — try ГРАФ 0066 / FIKS 0107 /
a Shift+АЛФ combo, or read how ФОДОС sets the mode). Alternatively capture these letters
from КЛАД's own win/lose screens ("Уровень пройден" → й, "Игра окончена" → И, "нажмите
клавишу" → ж ш у, "Поздравляем" → з) by driving the emulator to those states.

## KI-2 — title credits: Н mis-mapped, Б/8 missing
**What:** the title renders "−иколаев 19 7" / "аранов" — `Н` shows as a dash (wrong index
in `tools/extract_uknc_font.py` GLYPH_MAP for the title source) and `Б`, `8` are absent.
**Impact:** cosmetic, title credit line only. HUD and speed-select are correct.
**Path:** re-dump the title (`extract_uknc_font.py dump 01_title.png`), read the correct
index for `Н`, fix the map; extract `Б`/`8` from the "Баранов"/"1987" credit lines
(group-detection restricted to the centre columns, away from the КЛАД tile-art).

## KI-4 — character sprite decode (tiles 16-31) — IN PROGRESS
**What:** the player/enemy sprite tiles (char 16-31) decoded with the current map rules
come out as scattered dither, not a figure.
**Reference captured:** the headless emulator's `sprite` mode (frame-diff: screenshot at
spawn, move right, screenshot again — the changed pixels isolate the player) gives the real
player figure (`assets/uknc/reference_emu/sprites/player_spawn.png`):
```
....####..##
....####..##
##....##..##
##########..   ← arms out
....####....
....##..##..   ← legs
....##..##..
....##......
```
**Open puzzle:** the figure renders **12 px wide** on screen (tiles are 16 px), so it does
NOT map cleanly onto an 8-px char tile under OR/planes/side-by-side (best brute-force match
only 44/64). The УКНЦ sprite blit (`SPRITE_DRAW` 014030, XOR-based 014772/015072/…) may use
a different width/scale or a different data layout than the map tiles.
**Path:** trace `SPRITE_DRAW` to see how many bytes/scale per sprite row; re-extract a clean
16-px figure from frame B; brute-force tile×decode against it; then fix `gen_gfx_c.py` char
decode + `render.c` CHAR_* slots. Same method that solved the map-tile decode.

**Progress (traced `SPRITE_DRAW` 014030):** the player is drawn as **TWO tiles** side by
side (`JSR TILE_BLIT_REV` twice — "left tile" + right), each 8 px → 16 px sprite (figure
fills ~12). Table @20270 holds per-direction X/Y offsets (not tile indices); the tile index
is the entity record `6(R4)`, set by `SPRITE_HELPERS` 013216. **Decode still open:** char
tile 16 OR-decodes to a figure-like half, but tiles 17-23 OR-decode to a `.#.#.#.` dither
(same symptom the map tiles had before the side-by-side/OR split was found). Next: determine
the sprite plane layout (likely NOT the same as map tiles), then which tile-pair = which
animation frame, then render the player as 2 tiles in `render.c`.

**Traced `TILE_BLIT_REV` 014302 (the sprite blit):** it computes the tile data address as
`index*16 + 017450`, loops `R5=8` rows, and calls the **same `DISP_SCANLINE_WRITE`** (entry
040140) the map tiles use (2 bytes/row). So sprites share the map-tile pixel format → each
sprite tile is 8 px (OR of the two row bytes, like gold), and the player = 2 such tiles =
16 px wide (figure fills ~12). **Remaining:** the figure-to-tile match is still fuzzy (tile
16 OR ≈ a figure half; 17-23 OR ≈ dither), so the open question is *which char tiles are the
standing/walking/climbing player frames* (via `SPRITE_HELPERS` 013216 + the entity record),
not the decode rule. Implementation: char tiles 16-31 → OR(8px) in `gen_gfx_c.py`; render the
player as the correct 2-tile pair in `render.c`.

**Traced `SPRITE_HELPERS` 013216 + table 012410:** the player tile index for state s is
`tile = byte[012410 + (s-0o21)*2 + dir]`; the non-zero entries are tiles **18 and 20**
(0o22/0o24) — climb=18, walk=20 (matches the indices `render.c` already uses). BUT a
brute-force of the captured player figure against every char tile × {OR, plane0, plane1,
side-by-side L/R, plane-OR} tops out at **42/64** — no clean match. Two complications make
the raw tile ≠ the on-screen figure:
  1. `SPRITE_DRAW` composes the sprite from **two tiles at X/Y offsets** (table 020270 holds
     per-direction dx/dy), so the figure is not a simple col-0/col-8 split of one decode.
  2. The blit is **XOR onto the background** (`074437 XOR ...` in the 040140 path, and
     014772/015072), so a captured figure = sprite ⊕ background unless the player stands on
     pure blue (rare in КЛАД).
**Next (deeper):** model the exact `SPRITE_DRAW` offset+XOR composition (or run the two char
tiles through the C23 `uknc_emu` against a blank framebuffer) to reconstruct the true sprite,
then set `render.c` to draw tiles 18/20 (+ their partner) at the right offset. This is a
multi-step modeling task — the architecture is fully traced, the pixel reconstruction isn't.

**✅ Video model implemented in `uknc_emu` (the authoritative renderer).** From `emubase`:
port 176640 = plane address, 176642→plane1, 176643→plane2, pixel = 3-bit plane combo.
`uknc_emu blit <tile>` now executes the REAL `TILE_BLIT_REV` against this model and reads
plane1. Results (authoritative — the actual game code rendering):
- ladder (1) → `..####....####..` = **2 rails** ✓
- gold (4)  → `######....######` = **2 blocks** (NOT a solid chest!) — so map tiles are
  unambiguously **side-by-side 16px**. The solid chest the player sees is the GOLD drawn as
  a **sprite** (2 tiles + offsets), not as a map tile. (Our OR gold happens to match the
  sprite look, so it stays.)
- player tiles 18/20 (from table 012410) → still render as sparse **dither** via plain
  `TILE_BLIT_REV`, so the clean captured figure is NOT a plain blit of those tiles → the
  player sprite must use a different composition (XOR path 040220, or different tiles).
**Open:** use `uknc_emu` to execute the full `SPRITE_DRAW` (set up the player entity record)
and read plane1 → the true player sprite, frame by frame.

**✅ RESOLVED understanding (executed SPRITE_DRAW in uknc_emu).** Drove the real game to
gameplay inside the C23 emulator and read the player from plane1 at its entity position
(player entity @014420, `[6]`=plane pos). The raw plane1 player is **dither** (`#.#.#.#.`) —
NOT a clean figure. The clean figure only appears after the УКНЦ **display transform**
(3-plane combine + per-line scale + palette in `Emulator_PrepareScreenRGB32`). So the
faithful player sprite is the **DISPLAYED output** (captured from the Qt/headless emulator,
`reference_emu/sprites/player_spawn.png`), not the raw char-tile/plane bytes — those are
dither by design. Conclusion: bake the captured displayed sprite into `render.c` (like the
font, extracted from rendered output). Walk/climb frames: capture the same way at those poses.

**Walk/climb animation (captured via the `sprite` mode, frame-diff at multiple phases):**
- **Walk** — the player figure is the SAME across walk frames (captured at several phases) →
  КЛАД uses a single figure for stand/walk (no visible walk animation). The single
  `PLAYER_ART` already baked is faithful. ✓
- **Climb** — there IS a distinct climbing pose, but the player is ON a ladder, so the
  capture = sprite ⊕ ladder rungs (garbled). A clean climb frame needs subtracting the ladder
  pattern or the display-transform path. Minor refinement; current code uses the walk figure
  while climbing (acceptable). Frames saved: `/tmp/spr_{rest,w1..w3,c1,c2}.ppm`.

## Verified OK
- **Level 1 layout** — our maze tile-grid matches the original (`02_gameplay.png`) modulo a
  ~2-row alignment offset in the diff; the structure (borders, ladder columns, platforms)
  coincides. Level data (`level_data.h` from the binary) is faithful.

## KI-3 — font coverage incomplete (43 / ~51 glyphs)
**What:** `()-012345679:АГЗКНПРСУШавгдеиклмнопрстфчыья`. Missing: `Б И й ж з ш у 8`.
**Path:** KI-1 / KI-2 resolve these. The game text falls back to a blank advance for
missing glyphs (no crash).

## KI-5 — player not animated like the original
**What:** in our impl the player figure doesn't animate the way the original does. (Earlier
frame-diff captures of walk looked static, but the user observes the original animates.)
**Impact:** visual fidelity. **Path:** re-capture player frames at finer anim phases
(ANIM_THROTTLE_PLAYER 007432 changes frame every 4 ticks) via the headless `sprite` mode;
the original may cycle 2 leg-position frames. Investigate later.

## KI-6 — enemy chase AI not working visibly
**What:** `enemy.c do_move` has greedy chase code (toward player, horiz-first, vertical on
ladders, from ENEMY2_MOVE 006602), but the adversary doesn't appear to chase in-game.
**Impact:** gameplay. **Path:** verify enemy_init spawns + enemy_tick runs each frame; check
speed/throttle; compare to ENEMY1/2/3_TICK. Investigate.

## KI-7 — enemy sprite identical to the player
**What:** `render_enemy` now draws PLAYER_ART, so enemies look exactly like the player. The
original enemies were **hatched/different** (distinct texture).
**Impact:** can't tell player from enemy. **Path:** capture the enemy figure via frame-diff
(the enemy moves on its own — diff two frames isolates it), bake a separate ENEMY_ART.

## KI-8 — player can't reach level goals (gets stuck) — FOUND BY REACHABILITY E2E
**What:** `tools/reachability.c` (BFS over the movement graph, same collision as map.c) shows
the player reaches only the bottom rows + a few ladder stubs in EVERY level; the exit is
unreachable (0/1) in all 10 levels and most gold is unreachable. The player gets stuck.
**Overlay (level 1):** climbs e.g. the col-9 ladder to its top (row 17) but is walled in
left/right there — ladders dead-end instead of connecting to platforms.
**Likely causes (to investigate):**
  1. Movement too strict — the climb-up rule (`player.c can_climb_into`, up only into a
     T_LADDER cell) stops at a ladder top and can't step onto the platform above. (Loosening
     it risks the old "climb through ceiling" bug — delicate.)
  2. Level-data vertical offset — the earlier tile-grid diff vs the original showed a ~2-row
     offset; if real, ladders wouldn't line up with platforms.
**Ground truth needed (KI/T8):** extract the real per-level reachability from uknc_emu
(COLLISION_MAP_BUILD flags) — if the real game reaches everything but we don't, it's our
movement/data. This is the concrete next step before changing the climb rule.

### KI-8 ground-truth result (uknc_emu) — CONCLUSIVE: our movement is the bug
Extracted the REAL collision flags from uknc_emu (BUF_TILE_WORK 014550 after
COLLISION_MAP_BUILD) and BFS'd them from the real player cell (014422 → cell 642 = col2,
row20). With the documented direction bits (right=0o1000, left=0o400, up=0o20000,
down=0o10000) the REAL game reaches **363/704 cells, exit 1/1, gold 6/8** — i.e. it
navigates to the exit and gold. Our impl reaches ~53 cells, exit 0/1.
**Root cause:** the original moves by **per-cell direction flags** (R/L/U/D computed by
COLLISION_MAP_BUILD); our `map.c`/`player.c` use heuristics (map_solid + climb-only-into-
ladder) that are far too strict. **Faithful fix:** reimplement COLLISION_MAP_BUILD's flag
computation (or bake the per-level flag maps extracted from uknc_emu) and make player
movement obey the flags — then the player can traverse like the original. This is the fix
for KI-8 (the player getting stuck) and supersedes the delicate ad-hoc climb rule.

### KI-8 FIX applied — player movement now uses the faithful collision flags
`map.c` now exposes `map_can_right/left/up/down/grounded` computed EXACTLY like
COLLISION_MAP_BUILD (013570): right/left = neighbour raw tile ≤ 8; up = current==8 (ladder2)
&& above ≤ 8; down = below==8; grounded = current==8 || below>6. `player.c player_update`
was rewritten to move STRICTLY by these flags (climb → walk-on-support → gravity), with
grid snapping. Result: the player navigates (reachability: level 1 212 cells incl. exit
1/1, was 53/exit-0; levels 4 & 10 also reach the exit). E2E collision invariants still 10/10.
**Still open:** levels 2,3,5,6,7,8,9 show the EXIT unreachable — but the КЛАД win condition
is collecting gold (gold_c=tile6 → LEVEL_COMPLETE), not touching the exit tile; and those
levels' DATA may be mis-extracted. Next: validate per-level reachability vs uknc_emu ground
truth for all 10 (T8 done only for level 1) + use gold_c as the completability metric.

## KI-9 — aspect ratio / font distortion (TO INVESTIGATE)
**Dims:** emulator УКНЦ screen = **640×288 px** (32×22 tiles → ~16×9.5 px/tile, non-square).
Our render target = **512×192** (tiles 16×8) + render_present stretches vertically **×1.19**
(8→9.5) to match the УКНЦ tile shape.
**Distortion:** that global ×1.19 vertical stretch is applied to EVERYTHING, including the
8×8 font → glyphs become 8×9.5 (non-square, distorted). The maze tiles look right but text
is squashed/stretched. Also we only render 512 wide (the maze), not the full 640 (УКНЦ has
side margins).
**To investigate:** render at a target that matches the УКНЦ pixel grid (e.g. 640×288, or
draw glyphs/UI at their true 8×8 aspect un-stretched while only the tile art carries the 2:1
pixel ratio), so tiles AND font keep correct proportions. Decide the canonical internal
resolution (likely 640×288 to match the УКНЦ exactly) and map tiles/sprites/font onto it.

## KI-10 — level completion: KEY → opens DOOR → exit up-right (NOT auto-advance)
**What (user-confirmed mechanic):** in our impl, collecting the key (gold_c, tile 6) instantly
advances level 1→2 (`player.c` → `PR_LEVEL_WIN`, `game.c` → GS_LEVEL_WIN). **WRONG.**
**Correct:** collecting the key **OPENS A DOOR** (does NOT complete the level). The player
must then go **through the opened door** and **climb up the ladder on the right** to the exit;
only THEN does the level complete.
**Ties together earlier findings:**
  - `SPRITE_HELPERS`/`PLAYER_STATE_CHECK` 012656: collecting tile 6 calls routine 12716 on
    cells @17424 / @17426 — i.e. it **rewrites two map cells = opens the door** (not a win).
  - KI-8: the EXIT (tile 2) was unreachable in 7/10 levels because **the door is closed until
    the key is taken** — the reachability analyzer didn't open it. With the door opened the
    exit should become reachable. So those levels are likely NOT mis-extracted.
**To implement (later):**
  1. `gold_c` (tile 6) → run the 12716 cell-rewrite (open the door: set @17424/@17426 cells
     passable), play a sound — do NOT advance the level.
  2. keep the real `T_EXIT` (tile 2) as the level-complete trigger (reach it after the door).
  3. update the reachability E2E to open the door before checking exit-reachability.

## KI-11 — 7 levels not completable: spawn/navigation, NOT map mis-extraction
**Verified:** the level maps are extracted FAITHFULLY (level 6 dumped direct from the binary
@025440 == our level_data.h). So the maps are correct.
**Real issue:** the player can't navigate to the goal in levels 2,3,5,6,7,8,9. Example —
level 6: the player spawns at (2,20) inside a vertical shaft (cols 2-3) with **no ladder** and
walled on the right (col4) except at row 0 → it can't climb out → stuck.
**Suspect:** LEVEL_SPAWNS look wrong/defaulted — player spawns are mostly (2,20)/(3,20)
(bottom-left) while the per-level enemy spawns sit at the TOP (row 0). Level 1 player spawn is
(2,0) (top). The bottom-left shaft is unnavigable in some levels → the player likely spawns
elsewhere in the real game (e.g. on a top platform, descending).
**Visual (done):** the door (tile 10) now renders closed=solid block / open=empty frame; the
key chest (gold_c) disappears on collection (map_clear) and opens the door.
**To reach "10 levels OK":** extract the REAL per-level player spawn from uknc_emu (drive
through levels / set CUR_MAP_ADDR 001300 + LEVEL_TBL_PTR 001304 per level and read the player
entity 014420), fix LEVEL_SPAWNS, then re-run reachability (door-open) — exits should become
reachable. Levels = base TBL_LEVEL_MAP_1 022100, stride 0o540=352 bytes.

## KI-12 — Water mechanic (RESOLVED, ground-truth-verified) + water-aware routing
**User report:** "când caracterul cade în apă nu se întâmplă nimic" — water wasn't lethal.
**Root cause (verified against the real УКНЦ collision buffer /tmp/cmap.bin):**
- Water is split: **tile 7 = shallow water** (passable, NOT lethal — you wade through/over it),
  **tiles 13/14 = deep water** (LETHAL). We previously had it backwards (7/14 lethal, 13 = wall).
- `ACT_DISPATCH` (001500) `CMPB #15,@2(R4)` → death iff the **current cell tile == 0o15 (13)**.
- `CMAP_FLAGS` (014012) **clears #4000 (grounded) when the tile below ≥ 13** → you are NOT
  grounded above deep water, so you fall INTO it → current tile becomes 13/14 → drown.
- Confirmed on the dumped buffer: row 20 cols 2–9 (above tile-11 border) have GND=1; cols 10–17
  (above tile-13 border) have GND=0, H2O(#2000)=1 — i.e. the real game makes that footing fatal.
**Fix (src/):**
- `klad.h`: tile 7 → T_EMPTY (passable shallow water); tiles 13/14 → T_WATER (lethal); tile 12
  stays T_WALL (tile 13 is no longer a wall).
- `map.c`: `map_grounded` clears grounding when below ≥ 13 (fall into deep water); `map_drowns`
  = current cell is deep water (13/14).
- `player.c`: entering/falling into deep water → PR_WATER → PLAYER_DEATH.
- **E2E unchanged: 10 checks / 0 failures, 0 wall-overlaps.** Verified collision model: tiles
  0/704 mismatch, flags only differ on the bottom border row 21 (off-playfield).
**Router (A*) now accounts for water (user req):** `tools/reachability.c` `settle()` falls THROUGH
deep water; `bfs()` treats a deep-water landing as a DROWN death — reachable-but-fatal, NOT
traversed (the safe path routes around it). New per-level `drown-tiles` column reports avoided
water. With the corrected, water-aware model: **5/10 completable (1,4,6,9,10)**; level 1
(ground-truth-known-good) is completable again (199 cells, key+exit) — validating the fix.
NOTE: the prior "5/10 OK" was OPTIMISTIC — those paths walked along the bottom row OVER the
tile-13 water border (= death in the real game). Water-awareness makes the router honest.

## KI-13 — RESOLVED: there is NO exit-tile level-completion in this build
**Investigated exhaustively (static + emulator probe). Findings (ground truth):**
- `LEVEL_COMPLETE` (001034) — the only code that writes the level pointers `@1300`/`@1304` —
  is **DEAD CODE**: the value `0o1034` appears NOWHERE in the binary (no JSR/JMP/jump-table
  entry reaches it). Confirmed by full-binary word scan.
- `PLAYER_STATE_CHECK` (012570) **never** advances the level. Emulator probe (`uknc_emu psc`)
  over every tile 0..14: no tile changes `@1300`/`@1304`. Per-tile effects, empirically:
    - tile 4/5 (gold/bonus) → cell cleared to 0 (collected, score/life).
    - tile 6 (gold_c) → cell set to **16 (0o20)** + `ENEMY_RESPAWN` (012716) rewrites the two
      "door" cells `@17424`/`@17426` to tile 9 + sets their neighbours' ≤9 flags (the "door").
    - tile 9 (0o11) → cell set to **15 (0o17)** + `JMP @#4640` = **DEATH** (tile 9 is lethal).
    - tile 2 (exit) → **nothing** (not checked anywhere).
- The ONLY level-advance path: `GAME_LOOP_MENU` (001354) reads the keyboard; if the code ==
  **`0o55`** → `JMP @#1004` (mislabelled GAME_OVER_SOFT) → `CLR R0` → `@#4160` checks
  `@17430 >= 011200` (all 10 spawn records consumed → real game-over/tally), else `JMP 1016`
  → `LEVEL_RESET` (010206) with **R0==0** → `ADD #24,10(R4)` advances the spawn-table pointer
  → next level. (Death reaches `LEVEL_RESET` with R0=1000 → reload SAME level.)
**Meaning:** this школьный build advances levels via a **key press (УКНЦ code 0o55)**, NOT by
reaching an exit. The "exit tile" (2) is inert; the in-code per-level GOAL is the gold_c "key"
(opens the door + spawns two enemies). So "all 10 exits reachable by climbing" is NOT a fidelity
requirement — it was our added design. (Emulator key-injection couldn't reproduce it because
gameplay reads the УКНЦ keyboard *hardware ports* 41020/40660, which uknc_emu doesn't yet feed;
the static path is unambiguous.)
**Decision needed (see chat):** keep our designed exit→advance (then make door connect to the
exit for the 5 stuck levels), adopt the faithful key-advance, or make WIN = collect the gold_c.

### KI-13 RESOLUTION (decizia userului: design B — cheie→ușă→ieșire)
Userul a ales **B**: cheia (gold_c) deschide ușa, iar IEȘIREA (tile 2, urcată pe scară) termină
nivelul — consideră avansul-cu-tastă (0o55) un artefact al portării pe discul școlar. Implementat:
- `map.c map_can_up`: poți urca ÎN ieșire (tile 2) din celula de dedesubt (ușa de sus). Ieșirea
  e unică/nivel → fără victorii false.
- `map.c map_can_down`: adăugat cazul CMAP #10000 lipsă (cur==8 && jos≤6 → cobori de pe scară în
  aur/aer) — verificat vs bufferul real.
- `game.c` (deja): gold_c → `map_open_door` + continuă; PR_EXIT (tile 2) → `score_level_advance`.
**REZULTAT: toate 10 nivele completabile** (reachability `/tmp/reach`: exit 1/1 pe fiecare,
conștient de apă). E2E 10/10, 0 wall-overlaps. Diferența completabil/blocat venea din: ieșirile
celor 5 nivele „blocate" plutesc cu aer dedesubt (vs zid/scară la cele 5 OK) — se ajunge urcând
în ele de pe rândul-platformă de sub ele.

## KI-14 — DEFERRED DECISION: mid-fall horizontal control (2 uncollectable chests, lvl 1)
**Finding (verified):** this build's fall is **column-locked** — straight down, keyboard ignored
while ungrounded (`ACT_DISPATCH` 001562 forces `MOV #10,R0`=down and loops without re-polling;
`uknc_emu fall` probe: col2→falls to row20, col unchanged). So there is NO mid-fall steering.
**Consequence:** level 1's 2 chests at `(4,15)` (bonus) + `(4,17)` (gold) sit in walled pockets
reachable only by drifting left mid-fall (col5→col4) → **uncollectable bonus** in this build.
Score-only (tile 4/5); does NOT affect completion (key+exit reachable).
**Decision DEFERRED (user, 2026-06):** revisit later whether to ADD mid-fall horizontal control
to our reimplementation (hold ←/→ while falling to drift one column) — a deliberate design
deviation (like KI-13 design B) that would make those chests reachable and match the remembered
mechanic. Options: (a) stay fidel/column-locked; (b) add drift-while-falling. If (b): then
re-check level 1 reaches 8/8 chests via the reachability tool. NOT decided yet — noted for later.

## KI-6 — UPDATE: enemy AI was "too smart"; made it faithfully primitive
Verified the original `ENEMY2_MOVE` (006602): it is a **primitive greedy chase, NOT pathfinding**.
- Different column → one horizontal step toward the player's column; same column → climb a ladder
  toward the player's row; then fall ONE cell if ungrounded (006712). Enemies can't step on tile 9
  (door/floor) — net passable = ≤8 (player is ≤9). Per-enemy cadence: ENEMY1 every ~5 ticks,
  ENEMY2/3 every tick after a 256-tick warmup.
Our version was too aggressive because: (a) **instant gravity** (apply_gravity fell to the floor
in one tick → snapped to the player's row and cornered them); (b) it climbed toward the player even
when not in their column-ish. Fixed `enemy.c do_move` to the faithful logic: column-gated ladder
climb + **one-cell-per-tick gravity** → it now gets stuck easily and descends slowly (evadable),
like the original. (KI-7 distinct hatched enemy sprite still open.)

## KI-15 — enemy spawn camping the exit (L6/L8/L9) + missing warmup — FIXED
Binar: unii inamici apar FIX pe celula de ieșire (originalul nu folosea ieșirea, deci nu conta;
designul nostru B o folosește): L8 e1(15,0)+e2(16,0) PE exit(15,0); L9 e1(9,8) PE exit(9,8);
L6 e1(8,0) lângă exit(7,0). → camp-uiau goal-ul de la start.
**Fix (src/game.c `enemy_spawn_override`):** îi mutăm pe poziții NEUTRALE (max-min distanță BFS de
ieșire/cheie/spawn): L6 e1→(28,20); L8 e1→(29,20), e2→(22,10); L9 e1→(27,1). Restul rămân ca în binar.
**Plus (src/enemy.c) — WARMUP fidel** (ENEMY2_TICK 006562: ~256 tickuri ≈ 2s): inamicul stă pe loc
la începutul nivelului, apoi pornește → jucătorul are un avans. Lipsea complet în reimplementare.
**Notă onestă:** modelul discret de proof (`prove_level.c`) tot NU certifică L6/L8/L9 nici cu
relocare nici la 8× — un chaser perfect la nivel-de-celulă e imbatabil pe grile dense 1-lat în
modelul turn-based. Asta e o limită a MODELULUI (jocul real e continuu, inamicul throttled la 0.5s,
jucătorul poate face juke sub-celulă), nu o dovadă că nivelul e imposibil. Fix-ul elimină camp-uirea
ieșirii de la start; corectitudinea fină se judecă la joc.

## KI-7 — RESOLVED: distinct hatched enemy sprite
The enemy reused PLAYER_ART (identical look). The raw sprite bank is dither (KI-4: clean figures
only via the УКНЦ display transform), so — per the user's memory of a *hașurat* adversary — the
enemy now uses a distinct silhouette (creature with horns/legs) rendered with a checkerboard
HATCH (`render.c` ENEMY_ART + draw_sprite_art hatch=true). Also blinks during the 2s warmup.
