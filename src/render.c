// render.c — randare 2-culori (КЛАД 1987 e 1bpp), tile-uri 16×8.
// LEVEL_RENDER (004776) + DISP_SCANLINE_WRITE (040060): tile = 16px lat × 8 înalt, 1bpp.
#include "render.h"
#include "gfx_data.h"
#include <stdio.h>

// ── paletă 2-culori cu comutare color/mono ───────────────────────────────────
// КЛАД 1987 randează doar 2 culori. Mod color = ca emulatorul УКНЦ (fundal albastru
// + alb). Mod mono = ca monitoarele monocrome de școală (negru + alb).
typedef struct { Color bg, fg; } Palette;
static const Palette PAL_COLOR = { {0,0,255,255},  {255,255,255,255} };  // УКНЦ (emulator)
static const Palette PAL_MONO  = { {0,0,0,255},    {236,236,236,255} };  // monocrom
static bool g_mono = false;

Color render_bg(void) { return g_mono ? PAL_MONO.bg : PAL_COLOR.bg; }
Color render_fg(void) { return g_mono ? PAL_MONO.fg : PAL_COLOR.fg; }
void  render_toggle_mono(void) { g_mono = !g_mono; }

// Tile-uri caracter (16-31), 16×8 1bpp. Indicii vin din tabela de animație 012410
// (indexată (state-0o21)*2 + dir): climb=18 (verificat: table[10]=18), walk=20/21/22.
// SPRITE_HELPERS (013216) scrie indexul în celula player; SPRITE_DRAW (014030) îl blit-uie.
// Figura УКНЦ e crudă (16×8) — om alb pe albastru (красный/зелёный doar în mod RGB).
#define CHAR_STAND  20   // walk frame A (nu există tile "stand" separat în tabelă)
#define CHAR_WALK0  20
#define CHAR_WALK1  21
#define CHAR_CLIMB  18

// tileset: pixeli fg = alb opac, fond = transparent. La desenare aplicăm tenta fg.
static void draw_slot(const Renderer *r, int slot, int x, int y, bool flip) {
    float w = flip ? -(float)TILE_W : (float)TILE_W;
    DrawTexturePro(r->tileset,
        (Rectangle){(float)(slot * TILE_W), 0, w, (float)TILE_H},
        (Rectangle){(float)x, (float)y, (float)TILE_W, (float)TILE_H},
        (Vector2){0, 0}, 0.0f, render_fg());
}

void render_init(Renderer *r) {
    // Tileset: 32 sloturi × 16×8 px. fg=alb opac, fond=transparent (tentă la desenare).
    Image img = GenImageColor(TILE_W * TILESET_SLOTS, TILE_H, (Color){0,0,0,0});
    for (int t = 0; t < TILESET_SLOTS; t++)
        for (int row = 0; row < TILE_H; row++)
            for (int col = 0; col < TILE_W; col++)
                if (TILE_GFX[t][row][col])
                    ImageDrawPixel(&img, t * TILE_W + col, row, (Color){255,255,255,255});
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

// LEVEL_RENDER (004776): 22 rânduri × 32 tile-uri. Tile 0 = aer.
// Tile 2 (exit) = invizibil în original; marcaj subtil.
void render_map(Renderer *r, const Map *m) {
    for (int row = 0; row < MAP_ROWS; row++)
        for (int col = 0; col < MAP_COLS; col++) {
            TileIdx idx = map_raw(m, col, row);
            if (idx == TIDX_AIR) continue;
            if (idx == TIDX_EXIT) {
                Color fg = render_fg(); fg.a = 60;
                DrawRectangleLines(col*TILE_W, row*TILE_H, TILE_W, TILE_H, fg);
                continue;
            }
            draw_slot(r, idx, col * TILE_W, row * TILE_H, false);
        }
}

// SPRITE_DRAW (014030): figură caracter în funcție de starea animației.
void render_player(Renderer *r, const Player *p, GameState gs, float state_timer) {
    bool show = (gs == GS_PLAYING || gs == GS_LEVEL_WIN || gs == GS_ALL_WIN)
             || (gs == GS_DEAD && (int)(state_timer * 8) % 2 == 0);
    if (!show) return;
    int slot;
    switch (p->anim) {
        case PA_CLIMB: slot = CHAR_CLIMB; break;
        case PA_WALK:  slot = (p->anim_frame & 1) ? CHAR_WALK1 : CHAR_WALK0; break;
        default:       slot = CHAR_STAND; break;
    }
    draw_slot(r, slot, (int)p->px, (int)p->py, p->facing < 0);
}

void render_enemy(Renderer *r, const Enemy *e) {
    if (!e->active) return;
    int frame = (int)(e->anim_t / ENEMY_ANIM_DT) % ENEMY_ANIM_HORIZ;
    int slot = (frame & 1) ? CHAR_WALK1 : CHAR_WALK0;
    draw_slot(r, slot, e->col * TILE_W, e->row * TILE_H, e->dir < 0);
}

// HUD_RENDER (003652): scor + nivel + vieți.
void render_hud(Renderer *r, const Score *s) {
    (void)r;
    char buf[96];
    snprintf(buf, sizeof buf, "%s:%d  %s:%d  %s:%d",
             T(STR_SCORE), s->score, T(STR_LEVEL), s->level + 1, T(STR_LIVES), s->lives);
    DrawText(buf, 2, MAP_ROWS * TILE_H + 2, 6, render_fg());
}

void render_debug_player(const Player *p) {
    char buf[64];
    int col = (int)((p->px + TILE_W*0.5f) / TILE_W);
    int row = (int)((p->py + TILE_H*0.5f) / TILE_H);
    snprintf(buf, sizeof buf, "px=%.0f py=%.0f c=%d r=%d lad=%d",
             p->px, p->py, col, row, (int)p->on_ladder);
    DrawText(buf, 240, MAP_ROWS * TILE_H + 2, 6, (Color){0,255,0,255});
}

// Upscale la fereastră. Pixelii УКНЦ sînt ne-pătrați (2:1): un tile 16×8 din date
// se afișează PĂTRAT. Deci lățimea efectivă = VW/2, raportul de afișare = 256:192 (4:3).
void render_present(Renderer *r) {
    BeginDrawing();
    ClearBackground(BLACK);
    const float aspect_w = VW * 0.5f;          // 256 — pixeli orizontali pe jumătate
    const float aspect_h = (float)VH;          // 192
    float sx = (float)GetScreenWidth()  / aspect_w;
    float sy = (float)GetScreenHeight() / aspect_h;
    float s  = (sx < sy) ? sx : sy;
    float dw = aspect_w * s, dh = aspect_h * s;
    Rectangle src = {0, 0, (float)VW, -(float)VH};   // flip Y
    Rectangle dst = {
        (GetScreenWidth()  - dw) * 0.5f,
        (GetScreenHeight() - dh) * 0.5f,
        dw, dh,
    };
    DrawTexturePro(r->target.texture, src, dst, (Vector2){0,0}, 0, WHITE);
    EndDrawing();
}
