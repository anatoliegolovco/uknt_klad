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

// ── Perechi de tile-uri half-plane pentru caracter (SPRITE_DRAW 014030) ───────
// Caracterul = 2 tile-uri half-plane suprapuse 1px. Indici confirmați din
// tabela 012410 (states 0o21-0o24 → tiles {16,18,20,21,22}) + ANIMATIONS.md.
//   climb  = 18+19  (state 0o21, verificat: table[10]=18)
//   walk0  = 20+21  (state 0o23, cadru A)
//   walk1  = 22+23  (state 0o23, cadru B)
//   stand  = 16+17
typedef struct { int a, b; } CharPair;
static const CharPair CHAR_CLIMB = {18, 19};
static const CharPair CHAR_WALK0 = {20, 21};
static const CharPair CHAR_WALK1 = {22, 23};
static const CharPair CHAR_STAND = {16, 17};
// Inamic: aceleași tile-uri de caracter (jocul folosește același set), cadru distinct
static const CharPair ENEMY_WALK0 = {20, 21};
static const CharPair ENEMY_WALK1 = {22, 23};

static void draw_slot(const Renderer *r, int slot, int x, int y, bool flip) {
    float w = flip ? -(float)TILE_PX : (float)TILE_PX;
    DrawTexturePro(r->tileset,
        (Rectangle){(float)(slot * TILE_PX), 0, w, (float)TILE_PX},
        (Rectangle){(float)x, (float)y, (float)TILE_PX, (float)TILE_PX},
        (Vector2){0, 0}, 0.0f, WHITE);
}

// SPRITE_DRAW (014030): blit 2 tile-uri half-plane suprapuse 1px → figură completă.
// ASM: prima blit la (X,Y), a doua la (X+ΔX, Y+ΔY) din TBL_ANIM_FRAMES (ΔX≈1px).
static void draw_char(const Renderer *r, CharPair cp, int x, int y, bool flip) {
    if (flip) {
        // oglindit: a doua jumătate ajunge la stînga
        draw_slot(r, cp.a, x + 1, y, true);
        draw_slot(r, cp.b, x,     y, true);
    } else {
        draw_slot(r, cp.a, x,     y, false);
        draw_slot(r, cp.b, x + 1, y, false);
    }
}

void render_init(Renderer *r) {
    // Build tileset: toate 32 tile-uri din TILE_GFX (pixel-perfect din binar).
    // slot index == tile index (0-15 hartă, 16-31 caracter half-plane).
    Image img = GenImageColor(TILE_PX * TILESET_SLOTS, TILE_PX, (Color){0,0,0,0});

    // LEVEL_RENDER (004776): tile_addr = idx*16 + 017450; TILE_GFX[idx] = paletă 0-3
    for (int t = 0; t < TILESET_SLOTS; t++)
        for (int row = 0; row < TILE_PX; row++)
            for (int col = 0; col < TILE_PX; col++) {
                uint8_t p = TILE_GFX[t][row][col];
                if (p) ImageDrawPixel(&img, t * TILE_PX + col, row, PAL[p]);
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
            draw_slot(r, idx, col * TILE_PX, row * TILE_PX, false);
        }
}

// SPRITE_DRAW (014030) + SPRITE_HELPERS (013216): alege perechea de tile-uri
// caracter în funcție de starea animației (climb/walk/stand) și o desenează
// ca figură 2-tile suprapusă.
void render_player(Renderer *r, const Player *p, GameState gs, float state_timer) {
    // Blink în GS_DEAD (8 Hz)
    bool show = (gs == GS_PLAYING || gs == GS_LEVEL_WIN || gs == GS_ALL_WIN)
             || (gs == GS_DEAD && (int)(state_timer * 8) % 2 == 0);
    if (!show) return;

    CharPair cp;
    switch (p->anim) {
        case PA_CLIMB: cp = CHAR_CLIMB; break;
        case PA_WALK:  cp = (p->anim_frame & 1) ? CHAR_WALK1 : CHAR_WALK0; break;
        case PA_STAND:
        default:       cp = CHAR_STAND; break;
    }
    draw_char(r, cp, (int)p->px, (int)p->py, p->facing < 0);
}

// Inamic: aceeași logică de blit 2-tile, cadru alternant din throttle animație
void render_enemy(Renderer *r, const Enemy *e) {
    if (!e->active) return;
    int frame = (int)(e->anim_t / ENEMY_ANIM_DT) % ENEMY_ANIM_HORIZ;
    CharPair cp = (frame & 1) ? ENEMY_WALK1 : ENEMY_WALK0;
    draw_char(r, cp, e->col * TILE_PX, e->row * TILE_PX, e->dir < 0);
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
