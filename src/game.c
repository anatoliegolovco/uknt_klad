// game.c — КЛАД reimplementation (C23 + raylib).
// Mechanics derived from УКНЦ КЛАД 1987 (Баранов) via disassembly.
// Tile and sprite graphics decoded pixel-perfect from KLAD_1987_Baranov.SAV.
// No original code is reproduced; graphics are reverse-engineered pixel data.
#include "raylib.h"
#include "game.h"
#include "i18n.h"
#include "levels_data.h"
#include "gfx_data.h"
#include <string.h>
#include <stdio.h>

// ---- УКНЦ 2bpp palette (exact from binary decoder) ----
// Index 0=black, 1=green(water), 2=yellow(gold), 3=white(walls/ladders/sprites)
static const Color UKNC_COL[4] = {
    {  0,   0,   0, 255},   // 0 black
    {  0, 192,   0, 255},   // 1 green
    {210, 200,   0, 255},   // 2 yellow
    {236, 236, 236, 255},   // 3 white
};
#define C_BG    UKNC_COL[0]
#define C_WHITE UKNC_COL[3]
#define C_YELLOW UKNC_COL[2]

// ---- tile types (game logic) ----
typedef enum { T_EMPTY=0, T_WALL, T_LADDER, T_WATER, T_GOLD, T_EXIT } Tile;

static Tile orig_to_tile(uint8_t idx) {
    switch (idx) {
        case 1: case 8:                    return T_LADDER;
        case 2:                            return T_EXIT;
        case 4: case 5: case 6:           return T_GOLD;
        case 7: case 14:                   return T_WATER;
        case 9: case 10: case 11:
        case 12: case 13:                  return T_WALL;
        default:                           return T_EMPTY;
    }
}

// ---- tileset texture ----
// Layout (each slot = 8px wide, 8px tall, in a single horizontal strip):
//   Slots  0-15  = map tiles (TILE_GFX[0..15]) — original tile indices
//   Slots 16-19  = player walk frames (SPRITE_GFX[0..3])
//   Slot   20    = player climb frame (SPRITE_GFX[8])
//   Slot   21    = player death frame (SPRITE_GFX[12])
//   Slots 22-25  = enemy walk frames  (SPRITE_GFX[16..19])
// TILE_SLOT_* macros address each slot.
enum {
    TILE_SLOT_MAP    = 0,    // slots 0-15: map tiles indexed by orig index
    TILE_SLOT_PLW    = 16,   // slots 16-19: player walk (4 frames)
    TILE_SLOT_PLC    = 20,   // slot 20: player climb
    TILE_SLOT_PLD    = 21,   // slot 21: player death
    TILE_SLOT_ENW    = 22,   // slots 22-25: enemy walk (4 frames)
    TILE_SLOT_COUNT  = 26,
};

// Sprite frame indices into SPRITE_GFX[] to put in each slot:
static const int SPRITE_SLOT_SRC[TILE_SLOT_COUNT - 16] = {
    0, 1, 2, 3,   // player walk 0-3   → slots 16-19
    8,            // player climb       → slot 20
    12,           // player death       → slot 21
    16, 17, 18, 19, // enemy walk 0-3  → slots 22-25
};

static Texture2D tileset;

static void build_tileset(void) {
    Image img = GenImageColor(8 * TILE_SLOT_COUNT, 8, (Color){0,0,0,0});

    // Map tiles from TILE_GFX (pixel-perfect from binary)
    for (int t = 0; t < 16; t++)
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++) {
                uint8_t pal = TILE_GFX[t][r][c];
                if (pal > 0)
                    ImageDrawPixel(&img, t * 8 + c, r, UKNC_COL[pal]);
            }

    // Sprite frames from SPRITE_GFX (pixel-perfect from binary)
    int n_spr = (int)(sizeof(SPRITE_SLOT_SRC) / sizeof(SPRITE_SLOT_SRC[0]));
    for (int s = 0; s < n_spr; s++) {
        int src_frame = SPRITE_SLOT_SRC[s];
        int slot = 16 + s;
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++) {
                uint8_t pal = SPRITE_GFX[src_frame][r][c];
                if (pal > 0)
                    ImageDrawPixel(&img, slot * 8 + c, r, UKNC_COL[pal]);
            }
    }

    tileset = LoadTextureFromImage(img);
    SetTextureFilter(tileset, TEXTURE_FILTER_POINT);
    UnloadImage(img);
}

static void draw_slot(int slot, int x, int y) {
    DrawTextureRec(tileset,
                   (Rectangle){(float)(slot * 8), 0, 8, 8},
                   (Vector2){(float)x, (float)y}, WHITE);
}

// ---- render target ----
static RenderTexture2D target;

// ---- map ----
static Tile     map[LEVEL_ROWS][COLS];      // game logic types
static uint8_t  raw_map[LEVEL_ROWS][COLS];  // original tile indices for rendering
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
typedef struct { int tx, ty; bool active; float cd; float anim_t; } Enemy;
static Enemy enemies[MAX_ENEMIES];
static const float ENEMY_CD = 0.42f;

// ---- animation ----
static float player_anim_t;
static bool  player_climbing;
static bool  player_dead;

static int player_walk_slot(void) {
    int f = (int)(player_anim_t / 0.15f) % 4;
    return TILE_SLOT_PLW + f;
}
static int enemy_walk_slot(float t) {
    int f = (int)(t / 0.18f) % 4;
    return TILE_SLOT_ENW + f;
}

// ---- game state ----
typedef enum { GS_PLAYING, GS_DEAD, GS_LEVEL_WIN, GS_GAME_OVER, GS_ALL_WIN } GameState;
static GameState gstate;
static float state_timer;
static int score, lives, cur_level;

// ---- level load ----
static void load_level(int lvl) {
    gold_remaining = 0;
    for (int r = 0; r < LEVEL_ROWS; r++)
        for (int c = 0; c < COLS; c++) {
            uint8_t idx = LEVEL_TILES[lvl][r][c];
            raw_map[r][c] = idx;
            Tile t = orig_to_tile(idx);
            map[r][c] = t;
            if (t == T_GOLD) gold_remaining++;
        }
    // Player spawn
    px = LEVEL_SPAWNS[lvl][0][0] * (float)TILE;
    py = LEVEL_SPAWNS[lvl][0][1] * (float)TILE;
    vy = 0;
    player_anim_t = 0;
    player_climbing = false;
    player_dead = false;
    // Enemy spawns
    for (int i = 0; i < MAX_ENEMIES; i++) {
        int8_t ec = LEVEL_SPAWNS[lvl][i + 1][0];
        int8_t er = LEVEL_SPAWNS[lvl][i + 1][1];
        enemies[i].active = (ec >= 0);
        enemies[i].tx = ec;
        enemies[i].ty = er;
        enemies[i].cd = ENEMY_CD * (float)(i + 1);
        enemies[i].anim_t = (float)i * 0.3f;
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
    bool moving = in.left || in.right;

    // Horizontal
    float nx = px + (in.right - in.left) * SPEED * dt;
    int chk_x = (int)((nx + (in.right ? TILE - 1 : 0)) / TILE);
    int mid_y  = (int)((py + TILE * 0.5f) / TILE);
    if (!solid(chk_x, mid_y)) px = nx;

    // Vertical: ladder vs gravity
    player_climbing = on_ladder();
    if (player_climbing) {
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
    if (py < 0) { py = 0; vy = 0; }
    if (py > (LEVEL_ROWS - 1) * (float)TILE) py = (LEVEL_ROWS - 1) * (float)TILE;

    // Animation counter
    if (moving || (player_climbing && (in.up || in.down)))
        player_anim_t += dt;

    // Tile interactions at player centre
    int tx = (int)((px + TILE * 0.5f) / TILE);
    int ty = (int)((py + TILE * 0.5f) / TILE);
    Tile ct = tile_at(tx, ty);
    if (ct == T_GOLD) {
        map[ty][tx] = T_EMPTY;
        raw_map[ty][tx] = 0;
        gold_remaining--;
        score += 10;
    }
    if (ct == T_WATER) {
        lives--;
        gstate = (lives <= 0) ? GS_GAME_OVER : GS_DEAD;
        state_timer = 1.5f;
        player_dead = true;
    }
    if (ct == T_EXIT) {
        if (cur_level >= 9) { gstate = GS_ALL_WIN; }
        else { gstate = GS_LEVEL_WIN; state_timer = 1.2f; }
    }
}

// ---- enemy update ----
static void enemy_gravity(Enemy *e) {
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
        enemies[i].anim_t += dt;
        enemies[i].cd -= dt;
        if (enemies[i].cd <= 0) {
            enemies[i].cd = ENEMY_CD;
            enemy_step(&enemies[i]);
        }
        if (enemies[i].tx == ptx && enemies[i].ty == pty) {
            lives--;
            gstate = (lives <= 0) ? GS_GAME_OVER : GS_DEAD;
            state_timer = 1.5f;
            return;
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
                int sv_score = score, sv_lives = lives;
                load_level(cur_level);
                score = sv_score; lives = sv_lives;
            }
            gstate = GS_PLAYING;
        }
        return;
    }
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

    // Level tiles — use raw_map (original tile indices) for pixel-accurate rendering.
    // Tile 0 (air) = skip.
    // Tile 2 (exit) = invisible in original; draw a subtle marker.
    for (int r = 0; r < LEVEL_ROWS; r++)
        for (int c = 0; c < COLS; c++) {
            uint8_t idx = raw_map[r][c];
            if (idx == 0) continue;  // air: nothing
            if (idx == 2) {          // exit: original is invisible; draw faint white border
                DrawRectangleLines(c * TILE, r * TILE, TILE, TILE,
                                   (Color){255,255,255,60});
                continue;
            }
            draw_slot(TILE_SLOT_MAP + idx, c * TILE, r * TILE);
        }

    // Enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        int slot = enemy_walk_slot(enemies[i].anim_t);
        draw_slot(slot, enemies[i].tx * TILE, enemies[i].ty * TILE);
    }

    // Player (blinks during death state)
    bool show = (gstate == GS_PLAYING || gstate == GS_LEVEL_WIN || gstate == GS_ALL_WIN)
             || (gstate == GS_DEAD && (int)(state_timer * 8) % 2 == 0);
    if (show) {
        int slot;
        if (player_dead)       slot = TILE_SLOT_PLD;
        else if (player_climbing) slot = TILE_SLOT_PLC;
        else                   slot = player_walk_slot();
        draw_slot(slot, (int)px, (int)py);
    }

    // HUD bar (bottom 16 px)
    int hy = LEVEL_ROWS * TILE + 2;
    char hud[64];
    snprintf(hud, sizeof hud, "%s:%d  %s:%d  %s:%d",
             T(STR_SCORE), score, T(STR_LEVEL), cur_level + 1, T(STR_LIVES), lives);
    DrawText(hud, 2, hy, 6, C_WHITE);

    // Overlay messages
    int mx = VW / 2, my = LEVEL_ROWS * TILE / 2;
    if (gstate == GS_LEVEL_WIN)
        DrawText(T(STR_LEVEL_CLEAR), mx - 38, my - 4, 8, C_WHITE);
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
