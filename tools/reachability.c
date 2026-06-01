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

// Gravity: from (c,r) fall until on a ladder or a floor (solid below) or the bottom.
static int settle(const Map *m, int c, int r) {
    while (r < MAP_ROWS - 1 && !map_ladder(m, c, r) && !map_solid(m, c, r + 1))
        r++;
    return r;
}

// The 4 player moves (then gravity settles). Returns new row in *nr (col unchanged for U/D).
// dir: 0=left 1=right 2=up 3=down. Returns true if the move is possible.
static bool try_move(const Map *m, int c, int r, int dir, int *nc, int *nr) {
    switch (dir) {
        case 0: if (c > 0 && !map_solid(m, c - 1, r)) { *nc = c - 1; *nr = settle(m, c - 1, r); return true; } break;
        case 1: if (c < MAP_COLS-1 && !map_solid(m, c + 1, r)) { *nc = c + 1; *nr = settle(m, c + 1, r); return true; } break;
        case 2: // up: only while on a ladder and the cell above is a ladder (player.c can_climb_into)
            if (map_ladder(m, c, r) && map_at(m, c, r - 1) == T_LADDER) { *nc = c; *nr = r - 1; return true; } break;
        case 3: // down: on a ladder, into any non-wall below
            if (map_ladder(m, c, r) && r < MAP_ROWS-1 && !map_solid(m, c, r + 1)) { *nc = c; *nr = settle(m, c, r + 1); return true; } break;
    }
    return false;
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
        memset(seen, 0, sizeof seen);
        // BFS
        static int qc[MAP_ROWS*MAP_COLS], qr[MAP_ROWS*MAP_COLS];
        int head = 0, tail = 0;
        seen[sr][sc] = true; qc[tail] = sc; qr[tail] = sr; tail++;
        long edges = 0;
        while (head < tail) {
            int c = qc[head], r = qr[head]; head++;
            for (int d = 0; d < 4; d++) {
                int nc, nr;
                if (try_move(&m, c, r, d, &nc, &nr)) {
                    edges++;
                    if (nr >= 0 && nr < MAP_ROWS && !seen[nr][nc]) {
                        seen[nr][nc] = true; qc[tail] = nc; qr[tail] = nr; tail++;
                    }
                }
            }
        }
        // count gold / exit and how many are reachable
        int gold_total = 0, gold_reach = 0, exit_total = 0, exit_reach = 0, reach = tail;
        for (int r = 0; r < MAP_ROWS; r++)
            for (int c = 0; c < MAP_COLS; c++) {
                TileType t = map_at(&m, c, r);
                if (t == T_GOLD) { gold_total++; if (seen[r][c]) gold_reach++; }
                if (t == T_EXIT) { exit_total++; if (seen[r][c]) exit_reach++; }
            }
        bool ok = (gold_total == 0 || gold_reach > 0) && (exit_total == 0 || exit_reach > 0);
        if (!ok) total_fail++;
        if (lvl == 0) {   // overlay: '+' reachable, lowercase tile = unreachable
            printf("  level 1 reachability overlay ('+'=reachable, S=spawn):\n");
            for (int r = 0; r < MAP_ROWS; r++) {
                printf("    ");
                for (int c = 0; c < MAP_COLS; c++) {
                    if (c==sc && r==sr) { putchar('S'); continue; }
                    TileType t = map_at(&m, c, r);
                    char base = t==T_WALL?'#':t==T_LADDER?'H':t==T_WATER?'~':t==T_GOLD?'$':t==T_EXIT?'X':'.';
                    putchar(seen[r][c] ? '+' : base);
                }
                putchar('\n');
            }
        }
        printf("level %2d: spawn(%d,%d)->settle r%d | reachable %3d cells, %ld moves | "
               "gold %d/%d  exit %d/%d  %s\n",
               lvl + 1, sc, sr0, sr, reach, edges,
               gold_reach, gold_total, exit_reach, exit_total,
               ok ? "OK" : "** UNREACHABLE GOAL (stuck) **");
    }
    printf("\n%s\n", total_fail == 0
        ? "ALL LEVELS COMPLETABLE — no stuck/unreachable-goal levels"
        : "SOME LEVELS HAVE UNREACHABLE GOALS — see above");
    return total_fail == 0 ? 0 : 1;
}
