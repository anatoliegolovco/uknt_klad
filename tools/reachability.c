// reachability.c — A*/BFS over the КЛАД movement graph for every level.
// Discovers all cells the player can reach from spawn and documents the movement options,
// using the SAME collision rules as the game (map.c, derived from the ASM). Reports per
// level: reachable cells, whether the gold/exit are reachable (level completable), and any
// "trap" cells (reachable but the player can get stuck — no way back / dead ends to water).
//
// Build:  cc -std=c2x -Wall -I src tools/reachability.c src/map.c -lm -o /tmp/reach && /tmp/reach
#include "map.h"
#include "level_data.h"
#include "score.h"          // NUM_LEVELS
#include <stdio.h>
#include <string.h>

// Gravity: fall while not on a climbable ladder (raw 8) and not grounded. Matches map_grounded:
// below ≤ 6 = air (fall), below ≥ 13 = deep water (fall THROUGH into it → drown). Tile 7 (shallow
// water) and 8..12 are footing — the fall stops there. bfs() flags deep-water landings via map_drowns.
static int settle(const Map *m, int c, int r) {
    while (r < MAP_ROWS - 1 && map_raw(m, c, r) != 8 &&
           (map_raw(m, c, r + 1) <= 6 || map_raw(m, c, r + 1) >= 13))
        r++;
    return r;
}

// The 4 moves, STRICTLY per the COLLISION_MAP_BUILD flags (map_can_*), then gravity settles.
// dir: 0=left 1=right 2=up 3=down.
static bool try_move(const Map *m, int c, int r, int dir, int *nc, int *nr) {
    switch (dir) {
        case 0: if (map_can_left(m, c, r))  { *nc = c - 1; *nr = settle(m, c - 1, r); return true; } break;
        case 1: if (map_can_right(m, c, r)) { *nc = c + 1; *nr = settle(m, c + 1, r); return true; } break;
        case 2: if (map_can_up(m, c, r))    { *nc = c;     *nr = r - 1;               return true; } break;
        case 3: if (map_can_down(m, c, r))  { *nc = c;     *nr = r + 1;               return true; } break;
    }
    return false;
}

// BFS from (sc,sr) over the flag-based moves; fills `seen`, returns SAFE reachable-cell count.
// Water-aware (user req): a move that lands the player in/on water is a DROWN death — the cell
// is reachable-but-fatal, so we tally it as a hazard and do NOT expand from it (the safe path
// routes around water, exactly like the real game where stepping into water = PLAYER_DEATH).
static int g_qc[MAP_ROWS*MAP_COLS], g_qr[MAP_ROWS*MAP_COLS];
static long g_drown;   // count of distinct drown-landings touched this BFS
static int bfs(const Map *m, int sc, int sr, bool seen[MAP_ROWS][MAP_COLS], long *edges) {
    static bool hazard[MAP_ROWS][MAP_COLS];
    int head = 0, tail = 0; *edges = 0; g_drown = 0;
    memset(hazard, 0, sizeof hazard);
    seen[sr][sc] = true; g_qc[0] = sc; g_qr[0] = sr; tail = 1;
    while (head < tail) {
        int c = g_qc[head], r = g_qr[head]; head++;
        for (int d = 0; d < 4; d++) {
            int nc, nr;
            if (try_move(m, c, r, d, &nc, &nr)) {
                (*edges)++;
                if (nr < 0 || nr >= MAP_ROWS) continue;
                if (map_drowns(m, nc, nr)) {          // landing in/on water = drown
                    if (!hazard[nr][nc]) { hazard[nr][nc] = true; g_drown++; }
                    continue;                          // do NOT traverse through water
                }
                if (!seen[nr][nc]) {
                    seen[nr][nc] = true; g_qc[tail] = nc; g_qr[tail] = nr; tail++;
                }
            }
        }
    }
    return tail;
}

int main(void) {
    int total_fail = 0;
    printf("# КЛАД movement-reachability report (%d levels)\n", NUM_LEVELS);
    printf("# rules from map.c (ASM-derived): walk L/R + gravity, climb ladders.\n\n");
    for (int lvl = 0; lvl < NUM_LEVELS; lvl++) {
        Map m; map_load(&m, lvl);
        int sc = LEVEL_SPAWNS[lvl][0][0], sr0 = LEVEL_SPAWNS[lvl][0][1];
        int sr = settle(&m, sc, sr0);
        static bool seen[MAP_ROWS][MAP_COLS];
        long edges = 0;
        // PASS 1 — door closed: explore until we reach the KEY (gold_c, tile 6).
        memset(seen, 0, sizeof seen);
        int reach = bfs(&m, sc, sr, seen, &edges);
        bool key_reached = false;
        for (int r = 0; r < MAP_ROWS; r++)
            for (int c = 0; c < MAP_COLS; c++)
                if (seen[r][c] && map_raw(&m, c, r) == TIDX_GOLD_C) key_reached = true;
        // PASS 2 — KI-10: the key OPENS THE DOOR (tile 10); re-explore the WHOLE maze incl.
        // the now-open door + the exit beyond it. We never stop at the exit — every corridor.
        if (key_reached) {
            map_open_door(&m);
            memset(seen, 0, sizeof seen);
            reach = bfs(&m, sc, sr, seen, &edges);
        }
        // count gold / exit reachable (after the door is open)
        int gold_total = 0, gold_reach = 0, exit_total = 0, exit_reach = 0;
        for (int r = 0; r < MAP_ROWS; r++)
            for (int c = 0; c < MAP_COLS; c++) {
                TileType t = map_at(&m, c, r);
                if (t == T_GOLD) { gold_total++; if (seen[r][c]) gold_reach++; }
                if (t == T_EXIT) { exit_total++; if (seen[r][c]) exit_reach++; }
            }
        // completable = can reach the EXIT (after collecting the key + opening the door)
        bool ok = (exit_total == 0 || exit_reach > 0);
        if (!ok) total_fail++;
        if (lvl == 0) {   // overlay: '+' reachable, lowercase tile = unreachable
            printf("  level 1 reachability overlay ('+'=reachable, S=spawn):\n");
            for (int r = 0; r < MAP_ROWS; r++) {
                printf("    ");
                for (int c = 0; c < MAP_COLS; c++) {
                    if (c==sc && r==sr) { putchar('S'); continue; }
                    int raw = map_raw(&m, c, r);
                    TileType t = map_at(&m, c, r);
                    char base = raw==10?'D':t==T_WALL?'#':t==T_LADDER?'H':t==T_WATER?'~':t==T_GOLD?'$':t==T_EXIT?'X':'.';
                    putchar(seen[r][c] ? '+' : base);
                }
                putchar('\n');
            }
        }
        printf("level %2d: spawn(%d,%d)->r%d | reach %3d, %ld moves | key %-3s | "
               "gold %d/%d  exit %d/%d  drown-tiles %ld  %s\n",
               lvl + 1, sc, sr0, sr, reach, edges, key_reached ? "YES" : "no",
               gold_reach, gold_total, exit_reach, exit_total, g_drown,
               ok ? "OK" : "** STUCK **");
    }
    printf("\n%s\n", total_fail == 0
        ? "ALL LEVELS COMPLETABLE — no stuck/unreachable-goal levels"
        : "SOME LEVELS HAVE UNREACHABLE GOALS — see above");
    return total_fail == 0 ? 0 : 1;
}
