// game.c — КЛАД reimplementare C23 + raylib.
// Obiectiv strategic: rulează pe Ubuntu (native) ȘI în browser (WebAssembly).
// Logica derivată strict din disassembly/annotated/uknc_klad_1987.asm.
// Grafică pixel-perfect din KLAD_1987_Baranov.SAV via gfx_data.h.
#include "raylib.h"
#include "game.h"
#include "i18n.h"
#include "uknc_defs.h"
#include "game_entity.h"
#include "game_map.h"
#include "game_player.h"
#include "game_enemy.h"
#include "levels_data.h"
#include "gfx_data.h"
#include <stdio.h>
#include <string.h>

// ── paleta УКНЦ 2bpp (PAL_* constants din uknc_defs.h) ──────────────────────
static const Color UKNC_COL[4] = {
    {  0,   0,   0, 255},  // PAL_BLACK
    {  0, 192,   0, 255},  // PAL_GREEN  (apă)
    {210, 200,   0, 255},  // PAL_YELLOW (aur)
    {236, 236, 236, 255},  // PAL_WHITE  (pereți, scări, sprite-uri)
};
#define C_BLACK  UKNC_COL[PAL_BLACK]
#define C_WHITE  UKNC_COL[PAL_WHITE]
#define C_YELLOW UKNC_COL[PAL_YELLOW]

// ── tileset texture ──────────────────────────────────────────────────────────
// Slot-uri în strip orizontal (fiecare = 8px lățime, 8px înălțime):
//   0-15  : tile-uri hartă (TILE_GFX[0..15]) — original tile indices
//  16-19  : player walk frames  (SPRITE_GFX[0..3])
//   20    : player climb frame  (SPRITE_GFX[8])
//   21    : player death frame  (SPRITE_GFX[12])
//  22-25  : enemy walk frames   (SPRITE_GFX[16..19])
enum {
    TS_MAP   = 0,   // 0-15: tile-uri hartă
    TS_PLW   = 16,  // 16-19: player walk (4 frame-uri, PLAYER_WALK_FRAMES)
    TS_PLC   = 20,  // player climb
    TS_PLD   = 21,  // player death
    TS_ENW   = 22,  // 22-25: enemy walk (4 frame-uri)
    TS_COUNT = 26,
};

// SPRITE_GFX frame index pentru fiecare slot sprite
static const int SPRITE_SLOT_SRC[] = {
    0, 1, 2, 3,        // player walk  → slots 16-19
    PLAYER_CLIMB_FRAME, // player climb → slot 20
    PLAYER_DEATH_FRAME, // player death → slot 21
    16, 17, 18, 19,    // enemy walk   → slots 22-25
};

static Texture2D tileset;

static void build_tileset(void) {
    Image img = GenImageColor(TILE_W * TS_COUNT, TILE_H, (Color){0,0,0,0});

    // Tile-uri hartă: TILE_GFX[0..15] pixel-perfect din binar
    for (int t = 0; t < 16; t++)
        for (int r = 0; r < TILE_H; r++)
            for (int c = 0; c < TILE_W; c++) {
                uint8_t pal = TILE_GFX[t][r][c];
                if (pal) ImageDrawPixel(&img, t * TILE_W + c, r, UKNC_COL[pal]);
            }

    // Sprite-uri: SPRITE_GFX frames din binar
    int n = (int)(sizeof(SPRITE_SLOT_SRC)/sizeof(SPRITE_SLOT_SRC[0]));
    for (int s = 0; s < n; s++) {
        int fr = SPRITE_SLOT_SRC[s];
        int slot = 16 + s;
        for (int r = 0; r < TILE_H; r++)
            for (int c = 0; c < TILE_W; c++) {
                uint8_t pal = SPRITE_GFX[fr][r][c];
                if (pal) ImageDrawPixel(&img, slot * TILE_W + c, r, UKNC_COL[pal]);
            }
    }

    tileset = LoadTextureFromImage(img);
    SetTextureFilter(tileset, TEXTURE_FILTER_POINT);
    UnloadImage(img);
}

static void draw_slot(int slot, int x, int y) {
    DrawTextureRec(tileset,
        (Rectangle){(float)(slot * TILE_W), 0, (float)TILE_W, (float)TILE_H},
        (Vector2){(float)x, (float)y}, WHITE);
}

// ── render target ─────────────────────────────────────────────────────────────
static RenderTexture2D target;

// ── stare joc ────────────────────────────────────────────────────────────────
#define MAX_ENEMIES 2   // jocul original are 2 inamici activi per nivel

static GameVars  gv;
static Player    player;
static Entity    enemies[MAX_ENEMIES];
static float     enemy_cd[MAX_ENEMIES];
static float     enemy_anim_t[MAX_ENEMIES];
static float     player_anim_t;
static int       player_anim_frame;

// ── input ────────────────────────────────────────────────────────────────────
// Sursa: FN_KBD_GAME_POLL (004674) — taste meniu + gameplay УКНЦ
static Input read_input(void) {
    Input in = {0};
    in.left   = IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A);
    in.right  = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
    in.up     = IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W);
    in.down   = IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S);
    in.action = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER);
    // Touch (1/3 ecran stânga = stânga, dreapta = dreapta, sus = sus, jos = jos)
    if (GetTouchPointCount() > 0) {
        Vector2 t = GetTouchPosition(0);
        float w = (float)GetScreenWidth(), h = (float)GetScreenHeight();
        if (t.x < w/3)       in.left  = true;
        else if (t.x > 2*w/3) in.right = true;
        if (t.y < h/4)       in.up    = true;
        else if (t.y > 3*h/4) in.down  = true;
    }
    return in;
}

// ── încărcare nivel ───────────────────────────────────────────────────────────
// Sursa: LEVEL_COMPLETE (001034) + COLLISION_MAP_BUILD (013524)
static void load_level(int lvl) {
    map_load(lvl);   // încarcă raw_tile + map_tile + gold_count

    // Spawn player — din LEVEL_SPAWNS (gen_levels_c.py din entity records ASM)
    int8_t pc = LEVEL_SPAWNS[lvl][0][0];
    int8_t pr = LEVEL_SPAWNS[lvl][0][1];
    player.ent.active = true;
    player.ent.col    = pc;
    player.ent.row    = pr;
    player.px         = pc * (float)TILE_W;
    player.py         = pr * (float)TILE_H;
    player.vy         = 0;
    player.on_ladder  = false;
    player.dead       = false;
    player.anim_ctr   = 0;

    // Spawn inamici
    for (int i = 0; i < MAX_ENEMIES; i++) {
        int8_t ec = LEVEL_SPAWNS[lvl][i+1][0];
        int8_t er = LEVEL_SPAWNS[lvl][i+1][1];
        enemy_init(&enemies[i], ec, er);
        enemy_cd[i]     = ENEMY_TICK_INTERVAL * (float)(i + 1);
        enemy_anim_t[i] = (float)i * 0.2f;
    }
    player_anim_t     = 0;
    player_anim_frame = 0;
}

// ── update player ─────────────────────────────────────────────────────────────
// Sursa: PLAYER_MOVE_STEP (012740) + PLAYER_STATE_CHECK (012570)
static void update_player(float dt, Input in) {
    bool moving = in.left || in.right;

    // Mișcare orizontală cu coliziune WALL
    float nx = player.px + (float)(in.right - in.left) * PLAYER_SPEED_PX * dt;
    int   ck  = (int)((nx + (in.right ? TILE_W - 1 : 0)) / TILE_W);
    int   my  = (int)((player.py + TILE_H * 0.5f) / TILE_H);
    if (!map_solid(ck, my)) player.px = nx;

    // Gravitație / scară — derivat din PLAYER_MOVE_STEP
    player.on_ladder = player_on_ladder(player.px, player.py);
    if (player.on_ladder) {
        player.vy = 0;
        player.py += (float)(in.down - in.up) * PLAYER_SPEED_PX * dt;
    } else {
        player.vy += PLAYER_GRAV_PX * dt;
        float ny  = player.py + player.vy * dt;
        int   ftx = (int)((player.px + TILE_W * 0.5f) / TILE_W);
        int   fty = (int)((ny + TILE_H) / TILE_H);
        if (map_solid(ftx, fty)) { player.vy = 0; ny = (float)(fty * TILE_H) - TILE_H; }
        player.py = ny;
    }
    // Clamp la limita nivelului
    if (player.py < 0) { player.py = 0; player.vy = 0; }
    if (player.py > (MAP_ROWS - 1) * (float)TILE_H)
        player.py = (MAP_ROWS - 1) * (float)TILE_H;

    // Animație player (ANIM_THROTTLE_PLAYER: la fiecare PLAYER_ANIM_TICKS frame-uri)
    if (moving || (player.on_ladder && (in.up || in.down))) {
        player_anim_t += dt;
        player_anim_frame = (int)(player_anim_t / (dt < 0.001f ? 0.15f : 0.15f)) % PLAYER_WALK_FRAMES;
    }

    // PLAYER_STATE_CHECK — verifică tile curent
    int col, row;
    player_center_tile(player.px, player.py, &col, &row);
    PlayerStateResult psr = player_state_check(col, row);

    switch (psr) {
        case PSR_COLLECT_GOLD:
            map_collect_gold(col, row);   // CLRB (R3) → tile → EMPTY
            gv.score += SCORE_PER_GOLD;   // ADD #12, @#017440 (octal 12 = dec 10)
            break;
        case PSR_BONUS_LIFE:
            map_collect_gold(col, row);
            gv.lives++;                   // BONUS_LIFE_ADD (003746)
            break;
        case PSR_LEVEL_WIN:
            // CMPB #6,(R3) → LEVEL_COMPLETE (001034)
            map_collect_gold(col, row);
            // cade prin → PSR_EXIT_REACHED logic
            // fall-through intentional
            gv.state = GS_LEVEL_WIN; gv.state_timer = 1.2f;
            break;
        case PSR_EXIT_REACHED:
            if (gv.level >= NUM_LEVELS - 1) gv.state = GS_ALL_WIN;
            else { gv.state = GS_LEVEL_WIN; gv.state_timer = 1.2f; }
            break;
        case PSR_WATER_DEATH:
        case PSR_ENEMY_HIT:
            gv.lives--;
            player.dead = true;
            gv.state = (gv.lives <= 0) ? GS_GAME_OVER : GS_DEAD;
            gv.state_timer = 1.5f;
            break;
        case PSR_NONE:
            break;
    }
}

// ── update inamici ────────────────────────────────────────────────────────────
// Sursa: ENEMY2_TICK (006552), ENEMY3_TICK (007462), LEVEL_END_CHECK (001636)
static void update_enemies(float dt) {
    if (gv.state != GS_PLAYING) return;
    int ptx, pty;
    player_center_tile(player.px, player.py, &ptx, &pty);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        enemy_anim_t[i] += dt;
        enemy_cd[i]     -= dt;
        if (enemy_cd[i] <= 0) {
            enemy_cd[i] = ENEMY_TICK_INTERVAL;
            enemy_step(&enemies[i], ptx, pty);  // greedy chase din ASM
        }
        // LEVEL_END_CHECK (001636): player_tile == enemy_tile → moarte
        if (enemy_hits_player(&enemies[i], ptx, pty)) {
            gv.lives--;
            player.dead  = true;
            gv.state     = (gv.lives <= 0) ? GS_GAME_OVER : GS_DEAD;
            gv.state_timer = 1.5f;
            return;
        }
    }
}

// ── state machine joc ─────────────────────────────────────────────────────────
// Sursa: GAME_TICK (001602) → PLAYER_STATE_CHECK + ENEMY*_TICK + LEVEL_END_CHECK
static void update(float dt, Input in) {
    if (gv.state == GS_PLAYING) {
        update_player(dt, in);
        update_enemies(dt);
        return;
    }
    if (gv.state == GS_DEAD || gv.state == GS_LEVEL_WIN) {
        gv.state_timer -= dt;
        if (gv.state_timer <= 0) {
            int sv_score = gv.score, sv_lives = gv.lives;
            if (gv.state == GS_LEVEL_WIN) gv.level++;
            load_level(gv.level);
            gv.score  = sv_score;
            gv.lives  = sv_lives;
            gv.state  = GS_PLAYING;
        }
        return;
    }
    // GS_GAME_OVER / GS_ALL_WIN: SPACE/ENTER restartează
    if (in.action) {
        gv.score = 0; gv.lives = 9; gv.level = 0;
        load_level(0);
        gv.state = GS_PLAYING;
    }
}

// ── render ────────────────────────────────────────────────────────────────────
// Sursa: LEVEL_RENDER (004776), SPRITE_DRAW (014030), HUD_RENDER (003652)
static void draw_scene(void) {
    BeginTextureMode(target);
    ClearBackground(C_BLACK);

    // Tile-uri hartă — LEVEL_RENDER (004776):
    // pentru fiecare tile non-zero în raw_tile → desenează TILE_GFX[raw_tile[r][c]]
    for (int r = 0; r < MAP_ROWS; r++)
        for (int c = 0; c < MAP_COLS; c++) {
            uint8_t idx = raw_tile[r][c];
            if (idx == 0) continue;  // TIDX_AIR: nimic
            if (idx == TIDX_EXIT) {  // exit = invizibil în original; marcaj subtil
                DrawRectangleLines(c*TILE_W, r*TILE_H, TILE_W, TILE_H,
                                   (Color){255,255,255,50});
                continue;
            }
            draw_slot(TS_MAP + idx, c * TILE_W, r * TILE_H);
        }

    // Inamici (SPRITE_DRAW: 2 tile-uri consecutive ca sprite 16×8)
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        int frame = (int)(enemy_anim_t[i] / ENEMY_ANIM_INTERVAL) % ENEMY_ANIM_HORIZ_FRAMES;
        int slot  = TS_ENW + (frame % 4);
        draw_slot(slot, enemies[i].col * TILE_W, enemies[i].row * TILE_H);
    }

    // Player (blink în GS_DEAD)
    bool show = (gv.state == GS_PLAYING || gv.state == GS_LEVEL_WIN || gv.state == GS_ALL_WIN)
             || (gv.state == GS_DEAD && (int)(gv.state_timer * 8) % 2 == 0);
    if (show) {
        int slot;
        if (player.dead)        slot = TS_PLD;
        else if (player.on_ladder) slot = TS_PLC;
        else                    slot = TS_PLW + (player_anim_frame % PLAYER_WALK_FRAMES);
        draw_slot(slot, (int)player.px, (int)player.py);
    }

    // HUD — HUD_RENDER (003652): scor + nivel + vieți
    char hud[64];
    snprintf(hud, sizeof hud, "%s:%d  %s:%d  %s:%d",
             T(STR_SCORE), gv.score,
             T(STR_LEVEL), gv.level + 1,
             T(STR_LIVES), gv.lives);
    DrawText(hud, 2, MAP_ROWS * TILE_H + 2, 6, C_WHITE);

    // Overlay mesaje
    int mx = VW/2, my = MAP_ROWS*TILE_H/2;
    if (gv.state == GS_LEVEL_WIN)
        DrawText(T(STR_LEVEL_CLEAR), mx-38, my-4, 8, C_WHITE);
    if (gv.state == GS_GAME_OVER) {
        DrawText(T(STR_GAME_OVER), mx-36, my-8, 8, C_WHITE);
        DrawText("SPACE / ENTER",  mx-38, my+4,  6, C_WHITE);
    }
    if (gv.state == GS_ALL_WIN) {
        DrawText("FELICITARI!", mx-30, my-8, 8, C_YELLOW);
        DrawText("SPACE / ENTER", mx-38, my+4, 6, C_WHITE);
    }

    EndTextureMode();

    // Upscale nearest-neighbour la fereastra curentă
    BeginDrawing();
    ClearBackground(BLACK);
    float sx = (float)GetScreenWidth()  / (float)VW;
    float sy = (float)GetScreenHeight() / (float)VH;
    float s  = sx < sy ? sx : sy;
    Rectangle src = {0, 0, (float)VW, -(float)VH};  // flip Y (OpenGL vs raylib)
    Rectangle dst = {
        (GetScreenWidth()  - (float)VW * s) * 0.5f,
        (GetScreenHeight() - (float)VH * s) * 0.5f,
        (float)VW * s, (float)VH * s
    };
    DrawTexturePro(target.texture, src, dst, (Vector2){0,0}, 0, WHITE);
    EndDrawing();
}

// ── API public ───────────────────────────────────────────────────────────────
void game_init(void) {
    target = LoadRenderTexture(VW, VH);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);
    build_tileset();
    // GAME_INIT (004000): LIVES=0o333→9, SCORE=0, nivel=0
    gv.score = VAR_SCORE_INIT;
    gv.lives = 9;
    gv.level = 0;
    gv.state = GS_PLAYING;
    load_level(0);
}

void game_frame(float dt) {
    if (dt > 0.1f) dt = 0.1f;   // cap dt (fereastră în background, etc.)
    update(dt, read_input());
    draw_scene();
}

void game_shutdown(void) {
    UnloadTexture(tileset);
    UnloadRenderTexture(target);
}
