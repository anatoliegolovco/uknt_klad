// game.c — clean-room game logic + rendering. C23.
//
// This is ORIGINAL code. Mechanics are inspired by КЛАД (collect treasure,
// avoid water/guards, dig blocks) but no original code/art/levels are used.
// Right now it is a minimal vertical slice: a tile grid, a gravity-bound
// player you can walk/climb, and a placeholder level — enough to build and
// run on native + web. Guards, treasure scoring and death come next.
#include "raylib.h"
#include "game.h"
#include "i18n.h"
#include <string.h>
#include <stdio.h>

// ---- our own palette (inspired by, not copied from, the BK/УКНЦ GRB look) ----
static const Color PAL_BG    = {  16,  16,  24, 255 };
static const Color PAL_WALL  = { 120,  96,  64, 255 };
static const Color PAL_LADDER= { 200, 180,  90, 255 };
static const Color PAL_WATER = {  40, 120, 220, 255 };
static const Color PAL_GOLD  = { 240, 210,  60, 255 };
static const Color PAL_PLAYER= { 230,  70,  70, 255 };

// Tile kinds.
typedef enum { T_EMPTY=0, T_WALL, T_LADDER, T_WATER, T_GOLD } Tile;

// Placeholder hand-authored level (our own layout). '.'=empty #=wall
// H=ladder ~=water $=gold. One screenful: COLS x ROWS = 32 x 24.
static const char *LEVEL[ROWS] = {
"................................",
"................................",
"....$.................$.........",
"...###...............###........",
"......H.........................",
"......H.....######..............",
"......H.........................",
"...####.........H...........$...",
".........$......H..........###..",
"........###.....H...............",
"................####............",
"....H...........................",
"....H.......$...................",
"....H......###..................",
"....H...........................",
"...####.................H.......",
"........................H.......",
".....$..................H.......",
"....###.........~~~~~~~~~~~......",
"................~~~~~~~~~~~......",
"#####################...########",
"################################",
"................................",
"................................",
};

static Tile map[ROWS][COLS];
static RenderTexture2D target;   // fixed VWxVH; upscaled to the window

// Player in pixel coords.
static float px, py;
static const float SPEED = 60.0f;     // px/s
static const float GRAV  = 220.0f;
static float vy = 0;
static int score = 0;
static int level = 1;

static Tile tile_at(int tx, int ty) {
    if (tx < 0 || tx >= COLS || ty < 0 || ty >= ROWS) return T_WALL;
    return map[ty][tx];
}
static Tile tile_at_px(float x, float y) {
    return tile_at((int)(x / TILE), (int)(y / TILE));
}

static void load_placeholder_level(void) {
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++) {
            char c = LEVEL[y][x];
            map[y][x] = c=='#'?T_WALL : c=='H'?T_LADDER : c=='~'?T_WATER
                      : c=='$'?T_GOLD : T_EMPTY;
        }
    px = 4 * TILE; py = 2 * TILE; vy = 0;
}

static Input read_input(void) {
    Input in = {0};
    in.left   = IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A);
    in.right  = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
    in.up     = IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W);
    in.down   = IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S);
    in.action = IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_Z);

    // Touch: left third = left, right third = right, top = up, bottom = down.
    if (GetTouchPointCount() > 0) {
        Vector2 t = GetTouchPosition(0);
        float w = (float)GetScreenWidth(), h = (float)GetScreenHeight();
        if (t.x < w/3) in.left = true; else if (t.x > 2*w/3) in.right = true;
        if (t.y < h/3) in.up = true;   else if (t.y > 2*h/3) in.down = true;
    }
    return in;
}

static bool on_ladder(void) {
    return tile_at_px(px + TILE/2.0f, py + TILE/2.0f) == T_LADDER;
}

static void update(float dt, Input in) {
    // Horizontal
    float nx = px + (in.right - in.left) * SPEED * dt;
    if (tile_at_px(nx + (in.right?TILE-1:0), py + TILE/2.0f) != T_WALL) px = nx;

    // Ladder climbing overrides gravity
    if (on_ladder()) {
        vy = 0;
        py += (in.down - in.up) * SPEED * dt;
    } else {
        vy += GRAV * dt;
        float ny = py + vy * dt;
        if (tile_at_px(px + TILE/2.0f, ny + TILE) == T_WALL) { vy = 0; }
        else py = ny;
    }

    // Collect gold under the player.
    int tx = (int)((px + TILE/2.0f)/TILE), ty = (int)((py + TILE/2.0f)/TILE);
    if (tile_at(tx, ty) == T_GOLD) { map[ty][tx] = T_EMPTY; score += 10; }

    // Water = death -> respawn for now.
    if (tile_at(tx, ty) == T_WATER) load_placeholder_level();
}

// ---- pixel-art tiles (8x8), BK/КЛАД retro style ------------------------------
// Each tile is 8 rows of 8 chars; the legend maps chars -> colours, ' ' = clear.
// Indices: 0 wall(brick) 1 ladder 2 water 3 gold(gem) 4 player.
enum { TI_WALL, TI_LADDER, TI_WATER, TI_GOLD, TI_PLAYER, TI_COUNT };

static const char TILE_ART[TI_COUNT][8][9] = {
  { // brick wall (running bond)
    "hrrrRrrr","rrrrRrrr","rrrrRrrr","RRRRRRRR",
    "rRrrrrrr","rRrrrrrh","rRrrrrrr","RRRRRRRR" },
  { // ladder (transparent gaps)
    " L    L "," L    L "," LLLLLL "," L    L ",
    " L    L "," LLLLLL "," L    L "," L    L " },
  { // water (wavy surface + ripples)
    "ffwwffww","wwwwwwww","wdwwwwdw","wwwwwwww",
    "wwwwdwww","wwwwwwww","dwwwwwww","wwwwwwww" },
  { // gold gem
    "   gg   ","  gyyg  "," gyyyyg ","gyyyyyyg",
    "oyyyyyyo"," oyyyyo ","  oggo  ","   oo   " },
  { // player (little digger)
    "  kkkk  "," ssssss "," s ss s ","  pppp  ",
    " pppppp "," p pp p ","  pp pp "," kk  kk " },
};

static Color tile_legend(char c) {
    switch (c) {
        case 'r': return (Color){150, 95, 60,255};   // brick body
        case 'R': return (Color){ 70, 45, 30,255};   // mortar
        case 'h': return (Color){185,130, 85,255};   // brick highlight
        case 'L': return (Color){205,175, 95,255};   // ladder
        case 'w': return (Color){ 40,110,210,255};   // water
        case 'f': return (Color){130,185,250,255};   // foam
        case 'd': return (Color){ 25, 75,175,255};   // deep water
        case 'g': return (Color){235,200, 55,255};   // gold
        case 'y': return (Color){255,240,150,255};   // gold bright
        case 'o': return (Color){175,135, 20,255};   // gold dark
        case 'p': return (Color){220, 60, 60,255};   // player body
        case 's': return (Color){240,200,160,255};   // skin
        case 'k': return (Color){110, 25, 25,255};   // dark
        default:  return (Color){0,0,0,0};           // ' ' transparent
    }
}

static Texture2D tileset;  // TI_COUNT tiles laid out horizontally, 8px each

static void build_tileset(void) {
    Image img = GenImageColor(8 * TI_COUNT, 8, (Color){0,0,0,0});
    for (int t = 0; t < TI_COUNT; t++)
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++) {
                Color c = tile_legend(TILE_ART[t][y][x]);
                if (c.a) ImageDrawPixel(&img, t * 8 + x, y, c);
            }
    tileset = LoadTextureFromImage(img);
    SetTextureFilter(tileset, TEXTURE_FILTER_POINT);
    UnloadImage(img);
}

static void draw_tile(int idx, int x, int y) {
    DrawTextureRec(tileset, (Rectangle){(float)idx * 8, 0, 8, 8},
                   (Vector2){(float)x, (float)y}, WHITE);
}

static void draw_scene(void) {
    BeginTextureMode(target);
    ClearBackground(PAL_BG);
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++) {
            int idx;
            switch (map[y][x]) {
                case T_WALL:   idx = TI_WALL;   break;
                case T_LADDER: idx = TI_LADDER; break;
                case T_WATER:  idx = TI_WATER;  break;
                case T_GOLD:   idx = TI_GOLD;   break;
                default: continue;   // empty: leave background
            }
            draw_tile(idx, x * TILE, y * TILE);
        }
    draw_tile(TI_PLAYER, (int)px, (int)py);

    // HUD (Romanian by default via i18n): "Scor 0   Nivel 1"
    char hud[64];
    snprintf(hud, sizeof hud, "%s %d   %s %d", T(STR_SCORE), score, T(STR_LEVEL), level);
    DrawText(hud, 2, 1, 6, WHITE);
    EndTextureMode();

    // Upscale the fixed target to the window, nearest-neighbour, flipped Y.
    BeginDrawing();
    ClearBackground(BLACK);
    float sx = (float)GetScreenWidth()/VW, sy = (float)GetScreenHeight()/VH;
    float s = sx < sy ? sx : sy;
    Rectangle src = { 0, 0, (float)VW, -(float)VH };
    Rectangle dst = { (GetScreenWidth()-VW*s)/2, (GetScreenHeight()-VH*s)/2, VW*s, VH*s };
    DrawTexturePro(target.texture, src, dst, (Vector2){0,0}, 0, WHITE);
    EndDrawing();
}

void game_init(void) {
    target = LoadRenderTexture(VW, VH);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT); // crisp pixels
    build_tileset();
    load_placeholder_level();
}

void game_frame(float dt) {
    if (dt > 0.1f) dt = 0.1f;          // clamp after tab-switch / first frame
    update(dt, read_input());
    draw_scene();
}

void game_shutdown(void) {
    UnloadTexture(tileset);
    UnloadRenderTexture(target);
}
