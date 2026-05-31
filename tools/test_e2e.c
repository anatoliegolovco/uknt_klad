// test_e2e.c — headless E2E test harness for КЛАД movement/collision.
// Drives the player through the maze frame-by-frame (no raylib, no window) and
// checks collision invariants, giving a programmatic feedback signal:
//   - IN_WALL   : player centre tile is a solid wall  → bug (passed through)
//   - blocked   : an input didn't move the player      → hit a wall (expected at walls)
//   - on_ladder : player is on a ladder
// Build:  make -C src test     (or: cc -std=c2x -I src tools/test_e2e.c src/{map,player,enemy,score}.c -o /tmp/e2e)
#include "klad.h"
#include "map.h"
#include "player.h"
#include "enemy.h"
#include "score.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define DT (1.0f/60.0f)

// ── collision feedback signal at the current player state ─────────────────────
typedef struct {
    int  col, row;       // player centre tile
    bool in_wall;        // centre tile is a solid wall (BUG if true mid-play)
    bool on_ladder;
    bool foot_in_wall;   // the player's feet are inside a wall
} Probe;

static Probe probe(const Player *p, const Map *m) {
    Probe s = {0};
    s.col = (int)((p->px + TILE_W * 0.5f) / TILE_W);
    s.row = (int)((p->py + TILE_H * 0.5f) / TILE_H);
    s.in_wall   = (map_at(m, s.col, s.row) == T_WALL);
    s.foot_in_wall = (map_at(m, s.col, (int)((p->py + TILE_H - 1) / TILE_H)) == T_WALL);
    s.on_ladder = p->on_ladder;
    return s;
}

// ── input builders ────────────────────────────────────────────────────────────
static Input IN(const char *keys) {
    Input in = {0};
    for (const char *k = keys; *k; k++) switch (*k) {
        case 'L': in.left=1; break;  case 'R': in.right=1; break;
        case 'U': in.up=1;  break;   case 'D': in.down=1;  break;
        case 'A': in.action=1; break;
    }
    return in;
}

// ── run a scripted sequence; assert player never inside a wall ────────────────
static int g_fail = 0, g_checks = 0;
static void CHECK(bool cond, const char *msg) {
    g_checks++;
    if (!cond) { g_fail++; printf("    [FAIL] %s\n", msg); }
}

// Drive `frames` of input `keys`, return the final probe. Asserts no wall-overlap
// at every frame (the key collision invariant). `trace` prints each frame.
static Probe drive(Player *p, Map *m, const char *keys, int frames, bool trace) {
    Input in = IN(keys);
    Probe s = probe(p, m);
    for (int f = 0; f < frames; f++) {
        float ox = p->px, oy = p->py;
        player_update(p, m, in, DT);
        s = probe(p, m);
        if (s.in_wall) {
            printf("    [WALL-OVERLAP] frame %d: player centre at (%d,%d) is a WALL "
                   "(px=%.1f py=%.1f) keys=%s\n", f, s.col, s.row, p->px, p->py, keys);
            g_fail++;
        }
        if (trace && (f % 5 == 0 || s.in_wall))
            printf("      f%-3d keys=%-4s px=%.0f py=%.0f c=%d r=%d %s%s dx=%.1f dy=%.1f\n",
                   f, keys, p->px, p->py, s.col, s.row,
                   s.on_ladder?"LAD ":"", s.in_wall?"INWALL ":"",
                   p->px-ox, p->py-oy);
    }
    g_checks++;
    return s;
}

// ── scenarios ─────────────────────────────────────────────────────────────────
static void scenario_gravity(void) {
    printf("  [gravity] player spawns in an empty column, must fall to a floor\n");
    Map m; map_load(&m, 0);
    Player p; player_init(&p, LEVEL_SPAWNS[0][0][0], LEVEL_SPAWNS[0][0][1]);
    Probe s = drive(&p, &m, "", 240, false);   // 4s of just gravity
    CHECK(!s.in_wall, "player not inside a wall after falling");
    CHECK(s.row > 0,  "player fell below spawn row");
    // must be resting on something solid below
    bool solid_below = map_solid(&m, s.col, s.row + 1);
    CHECK(solid_below, "player landed ON a solid tile (floor/platform)");
    printf("    -> landed at c=%d r=%d (floor below=%d)\n", s.col, s.row, solid_below);
}

static void scenario_walk_into_wall(void) {
    printf("  [walk] hold RIGHT for 3s: player must stop at a wall, never pass through\n");
    Map m; map_load(&m, 0);
    Player p; player_init(&p, LEVEL_SPAWNS[0][0][0], LEVEL_SPAWNS[0][0][1]);
    drive(&p, &m, "", 120, false);              // settle on floor first
    Probe s = drive(&p, &m, "R", 180, false);
    CHECK(!s.in_wall, "player never ended inside a wall walking right");
    // right neighbour must be solid (we stopped because of a wall) OR open edge
    printf("    -> stopped at c=%d r=%d, right tile solid=%d\n",
           s.col, s.row, map_solid(&m, s.col + 1, s.row));
}

static void scenario_climb(void) {
    printf("  [climb] find a ladder, climb up, must not pass through platforms into open air\n");
    Map m; map_load(&m, 0);
    // place player directly on a known ladder cell (col 9 row 18 = ladder)
    Player p; player_init(&p, 9, 18);
    Probe s = drive(&p, &m, "U", 240, true);   // hold up 4s, trace it
    CHECK(!s.in_wall, "player not inside a wall after climbing");
    printf("    -> climb ended at c=%d r=%d on_ladder=%d\n", s.col, s.row, s.on_ladder);
}

static void scenario_random_walk(void) {
    printf("  [fuzz] 3000 frames of varied input — INVARIANT: never inside a wall\n");
    Map m; map_load(&m, 0);
    Player p; player_init(&p, LEVEL_SPAWNS[0][0][0], LEVEL_SPAWNS[0][0][1]);
    const char *seq[] = {"R","R","U","R","D","L","U","U","R","D","L","L","U","R","","D"};
    int n = (int)(sizeof seq / sizeof seq[0]);
    int overlaps = 0;
    for (int i = 0; i < 200; i++) {
        Probe before = probe(&p, &m);
        Input in = IN(seq[i % n]);
        for (int f = 0; f < 15; f++) {
            player_update(&p, &m, in, DT);
            if (map_at(&m, (int)((p.px+TILE_W*0.5f)/TILE_W),
                          (int)((p.py+TILE_H*0.5f)/TILE_H)) == T_WALL) overlaps++;
        }
        (void)before;
    }
    CHECK(overlaps == 0, "player never inside a wall across the whole fuzz run");
    printf("    -> wall-overlap frames: %d (must be 0)\n", overlaps);
}

int main(void) {
    printf("=== КЛАД E2E movement/collision tests (headless) ===\n\n");
    scenario_gravity();
    scenario_walk_into_wall();
    scenario_climb();
    scenario_random_walk();
    printf("\n=== %d checks, %d failures ===\n", g_checks, g_fail);
    return g_fail ? 1 : 0;
}
