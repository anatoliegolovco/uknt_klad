// game.c — game loop principal
// Sursa ASM: GAME_INIT (004000), GAME_TICK (001602), GAME_LOOP (001344),
//            ACT_DISPATCH (001436), KBD_GAME_POLL (004674)
#include "game.h"
#include "level_data.h"
#include <string.h>

// ── input (KBD_GAME_POLL 004674) ─────────────────────────────────────────────
// Originalul: polling non-blocking de taste УКНЦ, coduri mapate via KEY_CODE_TBL
// Reimplementare: raylib keyboard + touch (identic pe Linux și WASM)
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
        if      (t.x < w / 3)       in.left  = true;
        else if (t.x > 2.0f * w/3)  in.right = true;
        if      (t.y < h / 4)       in.up    = true;
        else if (t.y > 3.0f * h/4)  in.down  = true;
    }
    return in;
}

// ── load nivel ───────────────────────────────────────────────────────────────
// LEVEL_COMPLETE (001034) + COLLISION_MAP_BUILD (013524)
static void load_level(Game *g) {
    int lvl = g->score.level;
    map_load(&g->map, lvl);

    // Spawn player din LEVEL_SPAWNS (derivat din entity records ASM)
    player_init(&g->player,
                LEVEL_SPAWNS[lvl][0][0],
                LEVEL_SPAWNS[lvl][0][1]);

    // Spawn inamici (entity records la 014430, 014440)
    for (int i = 0; i < MAX_ENEMIES; i++)
        enemy_init(&g->enemies[i],
                   LEVEL_SPAWNS[lvl][i+1][0],
                   LEVEL_SPAWNS[lvl][i+1][1]);
}

// ── GAME_INIT (004000) ────────────────────────────────────────────────────────
// MOV #0333, @#LIVES; CLR @#SCORE; JMP @#HW_INIT → TITLE_SEQ
void game_init(Game *g) {
    render_init(&g->renderer);
    score_init(&g->score);
    g->state       = GS_PLAYING;
    g->state_timer = 0.0f;
    load_level(g);
}

// ── GAME_TICK (001602) ────────────────────────────────────────────────────────
// Secvența exactă din ASM:
//   JSR DELAY_SPIN
//   JSR PLAYER_STATE_CHECK  ← player_update()
//   JSR ENEMY1_TICK         ← enemy_tick() × MAX_ENEMIES
//   JSR ENEMY2_TICK
//   JSR ENEMY3_TICK
//   JSR ENTITY1_RESTORE
//   JSR WATER_COLLISION     ← inclus în player_update()
//   CMP @#PLAYER_TILE, @#ENEMY1_TILE → level end check
//   CMP @#PLAYER_TILE, @#ENEMY2_TILE → level end check
static void game_tick(Game *g, Input in, float dt) {
    // PLAYER_STATE_CHECK + PLAYER_MOVE_STEP + WATER_COLLISION
    PlayerResult pr = player_update(&g->player, &g->map, in, dt);

    switch (pr) {
        case PR_GOLD:
            map_clear(&g->map,
                (int)((g->player.px + TILE_W*0.5f) / TILE_W),
                (int)((g->player.py + TILE_H*0.5f) / TILE_H));
            score_add_gold(&g->score);
            break;
        case PR_BONUS:
            map_clear(&g->map,
                (int)((g->player.px + TILE_W*0.5f) / TILE_W),
                (int)((g->player.py + TILE_H*0.5f) / TILE_H));
            score_add_life(&g->score);
            break;
        case PR_LEVEL_WIN:
        case PR_EXIT:
            map_clear(&g->map,
                (int)((g->player.px + TILE_W*0.5f) / TILE_W),
                (int)((g->player.py + TILE_H*0.5f) / TILE_H));
            if (score_level_advance(&g->score))
                g->state = GS_ALL_WIN;
            else {
                g->state       = GS_LEVEL_WIN;
                g->state_timer = 1.2f;
            }
            return;
        case PR_WATER:
        case PR_ENEMY:
            g->player.dead = true;
            if (score_lose_life(&g->score)) g->state = GS_GAME_OVER;
            else { g->state = GS_DEAD; g->state_timer = 1.5f; }
            return;
        case PR_NONE:
            break;
    }

    // ENEMY1_TICK / ENEMY2_TICK / ENEMY3_TICK + LEVEL_END_CHECK (001636)
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemy_tick(&g->enemies[i], &g->map, &g->player, dt)) {
            // CMP @#PLAYER_TILE, @#ENEMY_TILE → egal → GAME_OVER sequence
            g->player.dead = true;
            if (score_lose_life(&g->score)) g->state = GS_GAME_OVER;
            else { g->state = GS_DEAD; g->state_timer = 1.5f; }
            return;
        }
    }
}

// ── game_frame — entry point per-cadru (FN_GAME_LOOP 001344) ─────────────────
void game_frame(Game *g, float dt) {
    if (dt > 0.1f) dt = 0.1f;  // cap la 100ms (fereastră în background)

    Input in = read_input();

    // Comutare paletă color/mono (tasta M) — emulator УКНЦ vs monitor monocrom
    if (IsKeyPressed(KEY_M)) render_toggle_mono();

    // State machine joc
    switch (g->state) {
        case GS_PLAYING:
            game_tick(g, in, dt);
            break;

        case GS_DEAD:
        case GS_LEVEL_WIN:
            g->state_timer -= dt;
            if (g->state_timer <= 0.0f) {
                // LEVEL_RESET (010206): reload nivel, păstrează scor + vieți
                if (g->state == GS_LEVEL_WIN) {
                    // scorul a fost deja avansat în game_tick()
                }
                load_level(g);
                g->state = GS_PLAYING;
            }
            break;

        case GS_GAME_OVER:
        case GS_ALL_WIN:
            if (in.action) {
                score_init(&g->score);
                load_level(g);
                g->state = GS_PLAYING;
            }
            break;
    }

    // ── render ───────────────────────────────────────────────────────────────
    BeginTextureMode(g->renderer.target);
    ClearBackground(render_bg());

    render_map(&g->renderer, &g->map);
    for (int i = 0; i < MAX_ENEMIES; i++)
        render_enemy(&g->renderer, &g->enemies[i]);
    render_player(&g->renderer, &g->player, g->state, g->state_timer);
    render_hud(&g->renderer, &g->score);
    render_debug_player(&g->player);

    // Overlay mesaje
    int mx = VW/2, my = MAP_ROWS*TILE_H/2;
    Color cw = {236,236,236,255};
    Color cy = {210,200,0,255};
    if (g->state == GS_LEVEL_WIN)
        DrawText(T(STR_LEVEL_CLEAR), mx-38, my-4, 8, cw);
    if (g->state == GS_GAME_OVER) {
        DrawText(T(STR_GAME_OVER), mx-36, my-8, 8, cw);
        DrawText("SPACE / ENTER",  mx-38, my+4,  6, cw);
    }
    if (g->state == GS_ALL_WIN) {
        DrawText("FELICITARI!", mx-30, my-8, 8, cy);
        DrawText("SPACE / ENTER", mx-38, my+4, 6, cw);
    }

    EndTextureMode();
    render_present(&g->renderer);
}

void game_shutdown(Game *g) {
    render_shutdown(&g->renderer);
}
