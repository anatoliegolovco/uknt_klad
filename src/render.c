// render.c — randare pixel-perfect, derivat din LEVEL_RENDER (004776)
#include "render.h"
#include "gfx_data.h"
#include <stdio.h>

// УКНЦ 2bpp palette: (pixel_plane, color_plane) → culoare
// Confirmat din extract_uknc_gfx.py + tile PNGs extrase din binar
static const Color PAL[4] = {
    {  0,   0,   0, 255},   // 0: negru  (fond)
    {  0, 192,   0, 255},   // 1: verde  (apă)
    {210, 200,   0, 255},   // 2: galben (aur)
    {236, 236, 236, 255},   // 3: alb    (pereți, scări, sprite-uri)
};

// Frame-uri din SPRITE_GFX[] pentru fiecare slot sprite
static const int SPR_SLOT_SRC[] = {
    0, 1, 2, 3,   // RS_PLW (16-19): player walk
    8,            // RS_PLC (20):    player climb — SPRITE_GFX[8]
    12,           // RS_PLD (21):    player death — SPRITE_GFX[12]
    16, 17, 18, 19 // RS_ENW (22-25): enemy walk
};

static void draw_slot(const Renderer *r, int slot, int x, int y) {
    DrawTextureRec(r->tileset,
        (Rectangle){(float)(slot * TILE_PX), 0, (float)TILE_PX, (float)TILE_PX},
        (Vector2){(float)x, (float)y}, WHITE);
}

void render_init(Renderer *r) {
    // Build tileset din TILE_GFX + SPRITE_GFX (date pixel-perfect din binar)
    Image img = GenImageColor(TILE_PX * RS_COUNT, TILE_PX, (Color){0,0,0,0});

    // LEVEL_RENDER (004776): ASL×4 + ADD #17450 → tile_addr = idx*16 + 017450
    // TILE_GFX[idx][row][col] conține valoarea paletei 0-3
    for (int t = 0; t < 16; t++)
        for (int row = 0; row < TILE_PX; row++)
            for (int col = 0; col < TILE_PX; col++) {
                uint8_t p = TILE_GFX[t][row][col];
                if (p) ImageDrawPixel(&img, t * TILE_PX + col, row, PAL[p]);
            }

    // SPRITE_DRAW (014030): sprite frames din binar
    int n = (int)(sizeof(SPR_SLOT_SRC) / sizeof(SPR_SLOT_SRC[0]));
    for (int s = 0; s < n; s++) {
        int fr   = SPR_SLOT_SRC[s];
        int slot = 16 + s;
        for (int row = 0; row < TILE_PX; row++)
            for (int col = 0; col < TILE_PX; col++) {
                uint8_t p = SPRITE_GFX[fr][row][col];
                if (p) ImageDrawPixel(&img, slot * TILE_PX + col, row, PAL[p]);
            }
    }

    r->tileset = LoadTextureFromImage(img);
    SetTextureFilter(r->tileset, TEXTURE_FILTER_POINT);
    UnloadImage(img);

    r->target = LoadRenderTexture(VW, VH);
    SetTextureFilter(r->target.texture, TEXTURE_FILTER_POINT);
}

void render_shutdown(Renderer *r) {
    UnloadTexture(r->tileset);
    UnloadRenderTexture(r->target);
}

// LEVEL_RENDER (004776): iterație 22 rânduri × 16 bytes (32 tile-uri)
// Tile 0 = aer → nimic; tile 2 (exit) = invizibil în original → marcaj subtil
void render_map(Renderer *r, const Map *m) {
    for (int row = 0; row < MAP_ROWS; row++)
        for (int col = 0; col < MAP_COLS; col++) {
            TileIdx idx = map_raw(m, col, row);
            if (idx == TIDX_AIR) continue;
            if (idx == TIDX_EXIT) {
                // EXIT = tile invizibil în original; marcaj subtil pentru jucător
                DrawRectangleLines(col*TILE_PX, row*TILE_PX, TILE_PX, TILE_PX,
                                   (Color){255,255,255,40});
                continue;
            }
            draw_slot(r, RS_MAP + idx, col * TILE_PX, row * TILE_PX);
        }
}

// SPRITE_DRAW (014030): alege frame în funcție de starea animației
void render_player(Renderer *r, const Player *p, GameState gs, float state_timer) {
    // Blink în GS_DEAD (8 Hz)
    bool show = (gs == GS_PLAYING || gs == GS_LEVEL_WIN || gs == GS_ALL_WIN)
             || (gs == GS_DEAD && (int)(state_timer * 8) % 2 == 0);
    if (!show) return;

    int slot;
    if (p->dead)       slot = RS_PLD;
    else if (p->on_ladder) slot = RS_PLC;
    else               slot = RS_PLW + (p->anim_frame % 4);

    draw_slot(r, slot, (int)p->px, (int)p->py);
}

void render_enemy(Renderer *r, const Enemy *e) {
    if (!e->active) return;
    int frame = (int)(e->anim_t / ENEMY_ANIM_DT) % ENEMY_ANIM_HORIZ;
    draw_slot(r, RS_ENW + (frame % 4), e->col * TILE_PX, e->row * TILE_PX);
}

// HUD_RENDER (003652): scor + vieți la EMT 024 (УКНЦ text output)
// Reimplementare: DrawText pe bara de jos (16px sub nivelul de 176px)
void render_hud(Renderer *r, const Score *s) {
    (void)r;
    char buf[96];
    snprintf(buf, sizeof buf, "%s:%d  %s:%d  %s:%d",
             T(STR_SCORE), s->score,
             T(STR_LEVEL), s->level + 1,
             T(STR_LIVES), s->lives);
    DrawText(buf, 2, MAP_ROWS * TILE_PX + 2, 6, (Color){236,236,236,255});
}

// debug: afișează px,py playerului în al doilea rând HUD
void render_debug_player(const Player *p) {
    char buf[64];
    int col = (int)((p->px + TILE_PX*0.5f) / TILE_PX);
    int row = (int)((p->py + TILE_PX*0.5f) / TILE_PX);
    snprintf(buf, sizeof buf, "px=%.0f py=%.0f col=%d row=%d lad=%d",
             p->px, p->py, col, row, (int)p->on_ladder);
    DrawText(buf, 2, MAP_ROWS * TILE_PX + 10, 5, (Color){0,255,0,255});
}

// Upscale nearest-neighbor la dimensiunea ferestrei (păstrează aspect ratio)
void render_present(Renderer *r) {
    BeginDrawing();
    ClearBackground(BLACK);
    float sx = (float)GetScreenWidth()  / (float)VW;
    float sy = (float)GetScreenHeight() / (float)VH;
    float s  = (sx < sy) ? sx : sy;
    Rectangle src = {0, 0, (float)VW, -(float)VH};
    Rectangle dst = {
        (GetScreenWidth()  - (float)VW * s) * 0.5f,
        (GetScreenHeight() - (float)VH * s) * 0.5f,
        (float)VW * s, (float)VH * s,
    };
    DrawTexturePro(r->target.texture, src, dst, (Vector2){0,0}, 0, WHITE);
    EndDrawing();
}
