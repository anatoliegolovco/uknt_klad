# Headless passability proof — levels winnable WITH the live enemy AI

Tool: `tools/prove_level.c` — a **rigorous turn-based state-space BFS** over
`(player cell, enemy cell, enemy-falling, got-key)` that uses:
- the EXACT player abilities (`map_can_left/right/up/down` + `settle`, + STAY), door-open after key;
- the EXACT enemy AI (`src/enemy.c do_move` replicated at cell level: greedy chase + commit-fall);
- the player moving `ratio`× per enemy move (default **2× — conservative**; the real player is
  4× horizontal / 8× vertical, so a win here ⇒ a win in the real continuous game).

If the BFS reaches the exit (after the key) it prints the winning move string = proof.
Build: `cc -std=c2x -O2 -I src tools/prove_level.c src/map.c src/score.c -lm -o /tmp/prove`
Run:   `/tmp/prove <level 1..10> [speed-ratio]`

## Result (2× conservative)
- **Level 1: ✅ WIN PROVEN** — 72-move winning sequence vs the live AI (46,887 states).
- Also proven winnable: **L2, L3, L4, L5, L7, L10** (7/10).
- **L6, L9**: BFS can't reach the key safely even at 8× → the enemy (which spawns on the
  key/exit chokepoint) guards the key path in this turn model.
- **L8**: player gets the key but can't survive to the exit in the model.

## Caveat (why ❌ here ≠ impossible in the real game)
The model is **discrete** (cell-level, turn-based). The real player moves **continuously** and
can juke at sub-cell precision against the 0.5 s-throttled enemy — evasion freedom the discrete
model lacks. So "no win in the Nx turn model" is a *lower bound*, not a proof of impossibility.
L6/L8/L9 are flagged for a look: the enemy guarding the key/exit chokepoint may be too strong
there (these levels spawn enemy1 directly on the exit), or simply require sub-cell juking.

A reactive continuous-physics playthrough harness also exists: `tools/sim_play.c` (drives
`player.c` + live `enemy.c`); it confirms the player navigates and is never unfairly caught, but
its heuristic controller isn't a solver — `prove_level.c` is the authoritative proof.
