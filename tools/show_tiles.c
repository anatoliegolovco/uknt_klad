// show_tiles.c — print TILE_GFX as ASCII (undistorted view of exactly what we render).
// Build: cc -I src tools/show_tiles.c -o /tmp/show && /tmp/show
#include "gfx_data.h"
#include <stdio.h>

static const char *NAME[32] = {
  "bg","ladder","exit","empty3","gold_a","gold_b","gold_c","water",
  "ladder2","wall_a","empty10","wall_b","wall_c","wall_d","water2","empty15",
  "char0","char1","char2","char3","char4","char5","char6","char7",
  "char8","char9","char10","char11","char12","char13","char14","char15"
};

static void show(int t) {
    // structural tiles: byte[2r]++byte[2r+1] (side-by-side); gold: byte[2r]|byte[2r+1] (OR)
    printf("tile %2d (%s):  [16 wide x 8 tall]\n", t, NAME[t]);
    for (int r = 0; r < 8; r++) {
        printf("    ");
        for (int c = 0; c < 16; c++) {
            if (c == 8) printf("|");                 // mark the byte boundary (8px split)
            putchar(TILE_GFX[t][r][c] ? '#' : '.');
        }
        putchar('\n');
    }
    putchar('\n');
}

int main(int argc, char **argv) {
    if (argc > 1) { for (int i = 1; i < argc; i++) show(atoi(argv[i])); return 0; }
    int tiles[] = {1, 8, 4, 5, 6, 7, 12, 11};   // ladder, ladder2, gold x3, water, walls
    for (unsigned i = 0; i < sizeof(tiles)/sizeof(tiles[0]); i++) show(tiles[i]);
    return 0;
}
