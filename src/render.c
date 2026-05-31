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

    // Font cu glife chirilice — port fidel: HUD/titlu în rusă ("Счет", "Попытки").
    // (УНКЦ ROM-font extraction = rafinare de fidelitate, notat în PORT_PLAN.md.)
    int cps[256]; int n = 0;
    for (int c = 32; c <= 126; c++) cps[n++] = c;          // ASCII
    for (int c = 0x0410; c <= 0x044F; c++) cps[n++] = c;   // А..я
    cps[n++] = 0x0401; cps[n++] = 0x0451;                  // Ё ё
    r->font = LoadFontEx("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 16, cps, n);
    SetTextureFilter(r->font.texture, TEXTURE_FILTER_POINT);
}

void render_shutdown(Renderer *r) {
    UnloadTexture(r->tileset);
    UnloadRenderTexture(r->target);
    UnloadFont(r->font);
}

// text rusesc cu fontul chirilic
void render_text(Renderer *r, const char *utf8, int x, int y, int size, Color c) {
    DrawTextEx(r->font, utf8, (Vector2){(float)x, (float)y}, (float)size, 0, c);
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
                DrawRectangleLines(col*TILE_W, row*TILE_H + PLAYFIELD_Y, TILE_W, TILE_H, fg);
                continue;
            }
            draw_slot(r, idx, col * TILE_W, row * TILE_H + PLAYFIELD_Y, false);
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
    draw_slot(r, slot, (int)p->px, (int)p->py + PLAYFIELD_Y, p->facing < 0);
}

void render_enemy(Renderer *r, const Enemy *e) {
    if (!e->active) return;
    int frame = (int)(e->anim_t / ENEMY_ANIM_DT) % ENEMY_ANIM_HORIZ;
    int slot = (frame & 1) ? CHAR_WALK1 : CHAR_WALK0;
    draw_slot(r, slot, e->col * TILE_W, e->row * TILE_H + PLAYFIELD_Y, e->dir < 0);
}

// HUD_RENDER (003652): bara de sus — "Счет {scor}   Попытки {vieți}" (text rusesc, fidel).
void render_hud(Renderer *r, const Score *s) {
    char buf[32];
    Color fg = render_fg();
    render_text(r, "Счет", 80, 3, 11, fg);
    snprintf(buf, sizeof buf, "%d", s->score);
    render_text(r, buf, 170, 3, 11, fg);
    render_text(r, "Попытки", 280, 3, 11, fg);                 // "Attempts" (vieți)
    snprintf(buf, sizeof buf, "%d", s->lives);
    render_text(r, buf, 400, 3, 11, fg);
}

// TITLE_SEQ (002072): titlu КЛАД + credite. NOTĂ fidelitate: originalul desenează "КЛАД"
// din tile-uri (un "title level"); aici e font mare ca prim pas — tile-art = task 3.
void render_title(Renderer *r) {
    Color fg = render_fg();
    const char *t = "КЛАД";
    int size = 72;
    Vector2 m = MeasureTextEx(r->font, t, (float)size, 0);
    render_text(r, t, (VW - (int)m.x) / 2, 26, size, fg);
    render_text(r, "Николаев 1987", VW/2 - 64, 120, 13, fg);
    render_text(r, "Баранов",       VW/2 - 36, 140, 13, fg);
    render_text(r, "нажмите клавишу", VW/2 - 74, 168, 11, fg);  // "press a key"
}

// DIFF_SELECT (003234): alegere viteză 1-4 (1 = rapid, 4 = lent).
void render_speed_select(Renderer *r, int speed) {
    Color fg = render_fg();
    render_text(r, "Скорость", VW/2 - 84, 64, 16, fg);          // "Speed"
    for (int i = 1; i <= 4; i++) {
        char b[4]; snprintf(b, sizeof b, "%d", i);
        Color c = (i == speed) ? (Color){255,230,120,255} : fg;
        render_text(r, b, VW/2 - 36 + (i-1)*24, 100, 18, c);
    }
    render_text(r, "1 - быстро   4 - медленно", VW/2 - 110, 140, 11, fg); // fast/slow
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
