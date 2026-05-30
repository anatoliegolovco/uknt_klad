// game.c — КЛАД reimplementation (C23 + raylib).
// Mechanics derived from УКНЦ КЛАД 1987 (Баранов) via disassembly.
// No original code, art, or level data is reproduced.
#include "raylib.h"
#include "game.h"
#include "i18n.h"
#include "levels_data.h"
#include <string.h>
#include <stdio.h>

// ---- palette — УКНЦ МС-0511 2bpp confirmed from disassembly ----
// (pixel_plane=P, color_plane=C):
//   (P=0,C=0)=BLACK  (P=1,C=0)=GREEN  (P=0,C=1)=YELLOW  (P=1,C=1)=WHITE
// Wall/ladder tiles: both planes same pattern → WHITE
// Water tiles: pixel plane only → GREEN
// Gold tiles: color plane only → YELLOW
static const Color C_BG     = {  0,   0,   0, 255};  // black
static const Color C_WHITE  = {236, 236, 236, 255};  // walls, ladders (pixel=1,color=1)
static const Color C_GREEN  = {  0, 192,   0, 255};  // water (pixel=1,color=0)
static const Color C_GREEN2 = {  0, 110,   0, 255};  // water wave variant
static const Color C_YELLOW = {210, 200,   0, 255};  // gold (pixel=0,color=1)
static const Color C_EXIT   = {236, 236, 236, 255};  // exit = white (same plane combo)
// Player/enemy: УКНЦ sprite tiles mix WHITE + GREEN for human figure
static const Color C_PLAYER = {236, 236, 236, 255};  // white body (dominant)
static const Color C_PLGRN  = {  0, 192,   0, 255};  // green limb accent
static const Color C_ENEMY  = {  0, 192,   0, 255};  // enemy body green
static const Color C_ENWHIT = {236, 236, 236, 255};  // enemy white accent

// ---- tile types ----
typedef enum { T_EMPTY=0, T_WALL, T_LADDER, T_WATER, T_GOLD, T_EXIT } Tile;

static Tile orig_to_tile(uint8_t idx) {
    switch (idx) {
        case 1: case 8:              return T_LADDER;
        case 2:                      return T_EXIT;
        case 4: case 5: case 6:     return T_GOLD;
        case 7: case 14:            return T_WATER;
        case 9: case 11: case 12: case 13: return T_WALL;
        default:                    return T_EMPTY;
    }
}

// ---- tileset (procedural pixel art) ----
enum { TI_WALL, TI_LADDER, TI_WATER, TI_GOLD, TI_EXIT, TI_PLAYER, TI_ENEMY, TI_COUNT };

// Pixel patterns derived from actual УКНЦ tile data (GFX_MAP.md).
// Each row is 8 chars; ' '=transparent (black bg shows through).
// W=white  G=green  g=dark-green  Y=yellow  X=exit  P=player  E=enemy
static const char TILE_ART[TI_COUNT][8][9] = {
    { // TI_WALL — tile 9/11: both planes = FC 3F A8 2A FC 3F FC 3F → white brick
        "WWWWWW..", "..WWWWWW", "W.W.W...", "..W.W.W.",
        "WWWWWW..", "..WWWWWW", "WWWWWW..", "WWWWWW.." },
    { // TI_LADDER — tile 1: both planes = 3C 3C FF FF 3C 3C 3C 3C → white cross
        "..WWWW..", "..WWWW..", "WWWWWWWW", "WWWWWWWW",
        "..WWWW..", "..WWWW..", "..WWWW..", "..WWWW.." },
    { // TI_WATER — tile 7: pixel = 00 00 FF FF CC CC 00 00, color = 0 → green band
        "........", "........", "GGGGGGGG", "GGGGGGGG",
        "GG..GG..", "GG..GG..", "........", "........" },
    { // TI_GOLD — tile 4: pixel = 0, color = FC 3F A8 2A FC 3F FC 3F → yellow brick
        "YYYYYY..", "..YYYYYY", "Y.Y.Y...", "..Y.Y.Y.",
        "YYYYYY..", "..YYYYYY", "YYYYYY..", "YYYYYY.." },
    { // TI_EXIT — original tile 2 = all zeros (invisible); use same cross as ladder
        "..WWWW..", "..WWWW..", "WWWWWWWW", "WWWWWWWW",
        "..WWWW..", "..WWWW..", "..WWWW..", "..WWWW.." },
    { // TI_PLAYER — white body + green limbs (matches УКНЦ sprite palette)
        "..PP....", ".PPPP...", "..PP....", "..gg....",
        ".gggg...", ".g..g...", "..g.g...", "........" },
    { // TI_ENEMY — green body + white accent (inverted from player)
        "..EE....", ".EEEE...", "..EE....", "..ww....",
        ".wwww...", ".w..w...", "..w.w...", "........" },
};

static Color tile_col(char c) {
    switch (c) {
        case 'W': return C_WHITE;
        case 'G': return C_GREEN;
        case 'g': return C_GREEN2;
        case 'Y': return C_YELLOW;
        case 'X': return C_EXIT;
        case 'P': return C_PLAYER;
        case 'p': return C_PLGRN;
        case 'E': return C_ENEMY;
        case 'w': return C_ENWHIT;
        default:  return (Color){0,0,0,0};
    }
}

static Texture2D tileset;

static void build_tileset(void) {
    Image img = GenImageColor(8 * TI_COUNT, 8, (Color){0,0,0,0});
    for (int t = 0; t < TI_COUNT; t++)
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++) {
                Color c = tile_col(TILE_ART[t][y][x]);
                if (c.a) ImageDrawPixel(&img, t * 8 + x, y, c);
            }
    tileset = LoadTextureFromImage(img);
    SetTextureFilter(tileset, TEXTURE_FILTER_POINT);
    UnloadImage(img);
}

static void draw_tile(int idx, int x, int y) {
    DrawTextureRec(tileset,
                   (Rectangle){(float)(idx * 8), 0, 8, 8},
                   (Vector2){(float)x, (float)y}, WHITE);
}

// ---- render target ----
static RenderTexture2D target;

// ---- map ----
static Tile map[LEVEL_ROWS][COLS];
static int gold_remaining;

static Tile tile_at(int tx, int ty) {
    if (tx < 0 || tx >= COLS || ty < 0 || ty >= LEVEL_ROWS) return T_WALL;
    return map[ty][tx];
}
static bool solid(int tx, int ty) { return tile_at(tx, ty) == T_WALL; }

// ---- player ----
static float px, py, vy;
static const float SPEED = 64.0f;
static const float GRAV  = 320.0f;

static bool on_ladder(void) {
    int cx  = (int)((px + TILE * 0.5f) / TILE);
    int ty1 = (int)(py / TILE);
    int ty2 = (int)((py + TILE - 1) / TILE);
    return tile_at(cx, ty1) == T_LADDER || tile_at(cx, ty2) == T_LADDER;
}

// ---- enemies ----
#define MAX_ENEMIES 2
typedef struct { int tx, ty; bool active; float cd; } Enemy;
static Enemy enemies[MAX_ENEMIES];
static const float ENEMY_CD = 0.42f;  // seconds between tile moves

// ---- game state ----
typedef enum { GS_PLAYING, GS_DEAD, GS_LEVEL_WIN, GS_GAME_OVER, GS_ALL_WIN } GameState;
static GameState gstate;
static float state_timer;
static int score, lives, cur_level;   // cur_level is 0-based

// ---- level load ----
static void load_level(int lvl) {
    gold_remaining = 0;
    for (int r = 0; r < LEVEL_ROWS; r++)
        for (int c = 0; c < COLS; c++) {
            Tile t = orig_to_tile(LEVEL_TILES[lvl][r][c]);
            map[r][c] = t;
            if (t == T_GOLD) gold_remaining++;
        }
    // Player spawn
    px = LEVEL_SPAWNS[lvl][0][0] * (float)TILE;
    py = LEVEL_SPAWNS[lvl][0][1] * (float)TILE;
    vy = 0;
    // Enemy spawns
    for (int i = 0; i < MAX_ENEMIES; i++) {
        int8_t ec = LEVEL_SPAWNS[lvl][i + 1][0];
        int8_t er = LEVEL_SPAWNS[lvl][i + 1][1];
        enemies[i].active = (ec >= 0);
        enemies[i].tx = ec;
        enemies[i].ty = er;
        enemies[i].cd = ENEMY_CD * (float)(i + 1);  // stagger start
    }
}

// ---- input ----
static Input read_input(void) {
    Input in = {0};
    in.left   = IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A);
    in.right  = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
    in.up     = IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W);
    in.down   = IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S);
    in.action = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER);
    if (GetTouchPointCount() > 0) {
        Vector2 t = GetTouchPosition(0);
        float w = (float)GetScreenWidth(), h = (float)GetScreenHeight();
        if (t.x < w / 3)       in.left  = true;
        else if (t.x > 2*w/3)  in.right = true;
        if (t.y < h / 4)       in.up    = true;
        else if (t.y > 3*h/4)  in.down  = true;
    }
    return in;
}

// ---- player update ----
static void update_player(float dt, Input in) {
    // Horizontal
    float nx = px + (in.right - in.left) * SPEED * dt;
    int chk_x = (int)((nx + (in.right ? TILE - 1 : 0)) / TILE);
    int mid_y  = (int)((py + TILE * 0.5f) / TILE);
    if (!solid(chk_x, mid_y)) px = nx;

    // Vertical: ladder vs gravity
    if (on_ladder()) {
        vy = 0;
        py += (in.down - in.up) * SPEED * dt;
    } else {
        vy += GRAV * dt;
        float ny = py + vy * dt;
        int ftx = (int)((px + TILE * 0.5f) / TILE);
        int fty = (int)((ny + TILE) / TILE);
        if (solid(ftx, fty)) { vy = 0; ny = (float)(fty * TILE) - TILE; }
        py = ny;
    }
    // Clamp vertical to level bounds
    if (py < 0) { py = 0; vy = 0; }
    if (py > (LEVEL_ROWS - 1) * (float)TILE) py = (LEVEL_ROWS - 1) * (float)TILE;

    // Tile interactions at player centre
    int tx = (int)((px + TILE * 0.5f) / TILE);
    int ty = (int)((py + TILE * 0.5f) / TILE);
    Tile ct = tile_at(tx, ty);
    if (ct == T_GOLD) {
        map[ty][tx] = T_EMPTY;
        gold_remaining--;
        score += 10;
    }
    if (ct == T_WATER) {
        lives--;
        gstate = (lives <= 0) ? GS_GAME_OVER : GS_DEAD;
        state_timer = 1.5f;
    }
    if (ct == T_EXIT) {
        if (cur_level >= 9) { gstate = GS_ALL_WIN; }
        else { gstate = GS_LEVEL_WIN; state_timer = 1.2f; }
    }
}

// ---- enemy update ----
static void enemy_gravity(Enemy *e) {
    // Fall until floor, ladder, or water edge (enemies don't enter water)
    while (e->ty + 1 < LEVEL_ROWS) {
        Tile below = tile_at(e->tx, e->ty + 1);
        if (below == T_WALL || below == T_LADDER || below == T_WATER) break;
        e->ty++;
    }
}

static void enemy_step(Enemy *e) {
    int ptx = (int)((px + TILE * 0.5f) / TILE);
    int pty = (int)((py + TILE * 0.5f) / TILE);
    int dx = (ptx > e->tx) ? 1 : (ptx < e->tx) ? -1 : 0;
    int dy = (pty > e->ty) ? 1 : (pty < e->ty) ? -1 : 0;
    // Prefer horizontal; use vertical (ladder) if blocked; never enter water
    Tile htile = tile_at(e->tx + dx, e->ty);
    if (dx && !solid(e->tx + dx, e->ty) && htile != T_WATER) {
        e->tx += dx;
    } else if (dy && tile_at(e->tx, e->ty) == T_LADDER && !solid(e->tx, e->ty + dy)) {
        e->ty += dy;
    }
    enemy_gravity(e);
}

static void update_enemies(float dt) {
    if (gstate != GS_PLAYING) return;
    int ptx = (int)((px + TILE * 0.5f) / TILE);
    int pty = (int)((py + TILE * 0.5f) / TILE);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        enemies[i].cd -= dt;
        if (enemies[i].cd <= 0) {
            enemies[i].cd = ENEMY_CD;
            enemy_step(&enemies[i]);
        }
        // Collision: same tile as player
        if (enemies[i].tx == ptx && enemies[i].ty == pty) {
            lives--;
            gstate = (lives <= 0) ? GS_GAME_OVER : GS_DEAD;
            state_timer = 1.5f;
            return;  // one death per frame
        }
    }
}

// ---- state machine ----
static void update(float dt, Input in) {
    if (gstate == GS_PLAYING) {
        update_player(dt, in);
        update_enemies(dt);
        return;
    }
    if (gstate == GS_DEAD || gstate == GS_LEVEL_WIN) {
        state_timer -= dt;
        if (state_timer <= 0) {
            if (gstate == GS_LEVEL_WIN) {
                cur_level++;
                load_level(cur_level);
            } else {
                // Respawn: reload level, preserve score + lives
                int sv_score = score, sv_lives = lives;
                load_level(cur_level);
                score = sv_score; lives = sv_lives;
            }
            gstate = GS_PLAYING;
        }
        return;
    }
    // GS_GAME_OVER or GS_ALL_WIN: wait for SPACE/ENTER to restart
    if (in.action) {
        score = 0; lives = 9; cur_level = 0;
        load_level(0);
        gstate = GS_PLAYING;
    }
}

// ---- draw ----
static void draw_scene(void) {
    BeginTextureMode(target);
    ClearBackground(C_BG);

    // Level tiles (top 176 px = LEVEL_ROWS × TILE)
    for (int r = 0; r < LEVEL_ROWS; r++)
        for (int c = 0; c < COLS; c++) {
            int idx = -1;
            switch (map[r][c]) {
                case T_WALL:   idx = TI_WALL;   break;
                case T_LADDER: idx = TI_LADDER; break;
                case T_WATER:  idx = TI_WATER;  break;
                case T_GOLD:   idx = TI_GOLD;   break;
                case T_EXIT:   idx = TI_EXIT;   break;
                default: break;
            }
            if (idx >= 0) draw_tile(idx, c * TILE, r * TILE);
        }

    // Enemies
    for (int i = 0; i < MAX_ENEMIES; i++)
        if (enemies[i].active)
            draw_tile(TI_ENEMY, enemies[i].tx * TILE, enemies[i].ty * TILE);

    // Player (blinks during death state)
    bool show = (gstate == GS_PLAYING || gstate == GS_LEVEL_WIN || gstate == GS_ALL_WIN)
             || (gstate == GS_DEAD && (int)(state_timer * 8) % 2 == 0);
    if (show) draw_tile(TI_PLAYER, (int)px, (int)py);

    // HUD bar (bottom 16 px)
    int hy = LEVEL_ROWS * TILE + 2;
    char hud[64];
    snprintf(hud, sizeof hud, "%s:%d  %s:%d  %s:%d",
             T(STR_SCORE), score, T(STR_LEVEL), cur_level + 1, T(STR_LIVES), lives);
    DrawText(hud, 2, hy, 6, C_WHITE);

    // Overlay messages
    int mx = VW / 2, my = LEVEL_ROWS * TILE / 2;
    if (gstate == GS_LEVEL_WIN) {
        DrawText(T(STR_LEVEL_CLEAR), mx - 38, my - 4, 8, C_EXIT);
    }
    if (gstate == GS_GAME_OVER) {
        DrawText(T(STR_GAME_OVER), mx - 36, my - 8, 8, C_WHITE);
        DrawText("SPACE / ENTER",   mx - 38, my + 4,  6, C_WHITE);
    }
    if (gstate == GS_ALL_WIN) {
        DrawText("FELICITARI!", mx - 30, my - 8, 8, C_YELLOW);
        DrawText("SPACE / ENTER", mx - 38, my + 4, 6, C_WHITE);
    }

    EndTextureMode();

    // Upscale to window (nearest-neighbour, keep aspect)
    BeginDrawing();
    ClearBackground(BLACK);
    float sx = (float)GetScreenWidth() / VW;
    float sy = (float)GetScreenHeight() / VH;
    float s  = sx < sy ? sx : sy;
    Rectangle src = {0, 0, (float)VW, -(float)VH};
    Rectangle dst = {(GetScreenWidth()  - VW * s) * 0.5f,
                     (GetScreenHeight() - VH * s) * 0.5f, VW * s, VH * s};
    DrawTexturePro(target.texture, src, dst, (Vector2){0,0}, 0, WHITE);
    EndDrawing();
}

// ---- public API ----
void game_init(void) {
    target = LoadRenderTexture(VW, VH);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);
    build_tileset();
    score = 0; lives = 9; cur_level = 0;
    gstate = GS_PLAYING;
    load_level(0);
}

void game_frame(float dt) {
    if (dt > 0.1f) dt = 0.1f;
    update(dt, read_input());
    draw_scene();
}

void game_shutdown(void) {
    UnloadTexture(tileset);
    UnloadRenderTexture(target);
}
