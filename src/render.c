// render.c — randare 2-culori (КЛАД 1987 e 1bpp), tile-uri 16×8.
// LEVEL_RENDER (004776) + DISP_SCANLINE_WRITE (040060): tile = 16px lat × 8 înalt, 1bpp.
#include "render.h"
#include "gfx_data.h"
#include "uknc_font.h"     // font REAL УКНЦ 8×8, extras pixel-exact (font/)
#include "i18n.h"
#include <stdio.h>
#include <string.h>

Lang g_lang = LANG_RU;     // implicit: rusă (fidel originalului); L comută la română

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
void  render_toggle_lang(void) { g_lang = (g_lang == LANG_RU) ? LANG_RO : LANG_RU; }
// Apa randată distinct (galben), ca în emulatorul УКНЦ (banda de jos = apă). În mono → alb.
static Color render_water(void) { return g_mono ? PAL_MONO.fg : (Color){236,204,64,255}; }

// Tile-uri caracter (16-31), 16×8 1bpp. Indicii vin din tabela de animație 012410
// (indexată (state-0o21)*2 + dir): climb=18 (verificat: table[10]=18), walk=20/21/22.
// SPRITE_HELPERS (013216) scrie indexul în celula player; SPRITE_DRAW (014030) îl blit-uie.
// Figura УКНЦ e crudă (16×8) — om alb pe albastru (красный/зелёный doar în mod RGB).
#define CHAR_STAND  20   // walk frame A (nu există tile "stand" separat în tabelă)
#define CHAR_WALK0  20
#define CHAR_WALK1  21
#define CHAR_CLIMB  18

// tileset: pixeli fg = alb opac, fond = transparent. La desenare aplicăm tenta fg.
static void draw_slot(const Renderer *r, int slot, int x, int y, bool flip, Color tint) {
    float w = flip ? -(float)TILE_W : (float)TILE_W;
    DrawTexturePro(r->tileset,
        (Rectangle){(float)(slot * TILE_W), 0, w, (float)TILE_H},
        (Rectangle){(float)x, (float)y, (float)TILE_W, (float)TILE_H},
        (Vector2){0, 0}, 0.0f, tint);
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

    // Atlas font REAL УКНЦ 8×8 (font/uknc_font.h) — glife albe opace pe transparent,
    // tentate la desenare. Înlocuiește DejaVu: text pixel-exact ca originalul, și merge
    // în WASM (date embedded, nu fișier TTF de pe disc).
    Image fimg = GenImageColor(UKNC_FONT_COUNT * 8, 8, (Color){0,0,0,0});
    for (int i = 0; i < UKNC_FONT_COUNT; i++)
        for (int row = 0; row < 8; row++)
            for (int col = 0; col < 8; col++)
                if ((UKNC_FONT[i].rows[row] >> (7 - col)) & 1)
                    ImageDrawPixel(&fimg, i * 8 + col, row, (Color){255,255,255,255});
    r->fonttex = LoadTextureFromImage(fimg);
    SetTextureFilter(r->fonttex, TEXTURE_FILTER_POINT);
    UnloadImage(fimg);
}

void render_shutdown(Renderer *r) {
    UnloadTexture(r->tileset);
    UnloadRenderTexture(r->target);
    UnloadTexture(r->fonttex);
}

// index glifei pentru un codepoint (UKNC_FONT e mic; scan liniar e suficient)
static int font_index(uint32_t cp) {
    for (int i = 0; i < UKNC_FONT_COUNT; i++)
        if (UKNC_FONT[i].cp == cp) return i;
    return -1;
}

// Text cu fontul REAL УКНЦ. `size` = înălțimea glifei în px (8 = nativ). Lățime fixă 8px.
void render_text(Renderer *r, const char *utf8, int x, int y, int size, Color c) {
    float s = (float)size / 8.0f;
    float adv = 8.0f * s;
    float fx = (float)x;
    for (int i = 0; utf8[i];) {
        int bytes = 0;
        int cp = GetCodepointNext(utf8 + i, &bytes);
        i += bytes;
        int gi = font_index((uint32_t)cp);
        if (gi >= 0)
            DrawTexturePro(r->fonttex,
                (Rectangle){(float)(gi * 8), 0, 8, 8},
                (Rectangle){fx, (float)y, adv, 8.0f * s},
                (Vector2){0, 0}, 0.0f, c);
        // Toate glifele (chirilic + latin acum în matrice) au aceeași celulă 8×8 → fără
        // font de rezervă (care trunchia), grilă uniformă. Glifă lipsă = avans gol.
        fx += adv;
    }
}

// LEVEL_RENDER (004776): 22 rânduri × 32 tile-uri. Tile 0 = aer.
// Tile 2 (exit) = invizibil în original; marcaj subtil.
void render_map(Renderer *r, const Map *m) {
    for (int row = 0; row < MAP_ROWS; row++)
        for (int col = 0; col < MAP_COLS; col++) {
            TileIdx idx = map_raw(m, col, row);
            if (idx == TIDX_AIR) continue;
            int dx = col*TILE_W, dy = row*TILE_H + PLAYFIELD_Y;
            if (idx == TIDX_EXIT) {
                // În original ieșirea e invizibilă (pixeli = aer). Design B: jucătorul TREBUIE
                // să o vadă → ușă vizibilă: cadru + săgeată-sus (▲) = „ieși pe aici".
                Color fg = render_fg();
                DrawRectangleLines(dx, dy, TILE_W, TILE_H, fg);                 // cadrul ușii
                DrawTriangle((Vector2){ dx + TILE_W/2.0f, dy + 1 },             // vârf sus
                             (Vector2){ dx + 3,           dy + TILE_H - 2 },    // bază stânga
                             (Vector2){ dx + TILE_W - 3,  dy + TILE_H - 2 },    // bază dreapta
                             fg);
                continue;
            }
            if (idx == 10) {   // KI-10: UȘA. Închisă = bloc plin; deschisă (cheia luată) = cadru gol.
                Color fg = render_fg();
                if (m->door_open) DrawRectangleLines(dx+1, dy, TILE_W-2, TILE_H, fg); // ușă deschisă
                else              DrawRectangle(dx, dy, TILE_W, TILE_H, fg);           // ușă închisă
                continue;
            }
            // apa (7 mică, 13/14 adâncă) galbenă ca în emulator; restul = alb (fg)
            Color tint = (idx == 7 || idx == 13 || idx == 14) ? render_water() : render_fg();
            draw_slot(r, idx, dx, dy, false, tint);
        }
}

// Figura player REALĂ — capturată din output-ul AFIȘAT al emulatorului УКНЦ (frame-diff:
// reference_emu/sprites/player_spawn.png). Datele brute ale tile-urilor-caracter sînt dither;
// figura curată apare doar prin transformarea de display УКНЦ (3 planuri + scale + paletă).
// Vezi docs/reverse/KNOWN_ISSUES.md KI-4. 12px lat × 8 înalt. '#' = pixel fg.
#define SPR_W 12
static const char *PLAYER_ART[8] = {        // STAND (= mers frame 0)
    "....####..##",
    "....####..##",
    "##....##..##",
    "##########..",
    "....####....",
    "....##..##..",
    "....##..##..",
    "....##......",
};
// KI-5: cadre de animație. Mers = 2 cadre (picioarele alternează — pas); urcat = 2 cadre
// (siluetă simetrică pe scară, brațe/picioare alternând). p->anim_frame comută 0/1.
static const char *PLAYER_WALK[2][8] = {
  { "....####..##","....####..##","##....##..##","##########..","....####....","....##..##..","....##..##..","....##......" },
  { "....####..##","....####..##","##....##..##","##########..","....####....","....##..##..","...##....##.","..##......##" },  // pas (picioare desfăcute)
};
static const char *PLAYER_CLIMB[2][8] = {
  { "....####....","....####....","..########..","....####....","..##.##.##..","....####....","..##....##..","..##....##.." },  // brațe sus
  { "....####....","....####....","....####....","..########..","....####....","..##.##.##..","...##..##...","..##....##.." },  // brațe jos / pas scară
};

// KI-7: inamicul era HAȘURAT și cu siluetă diferită de jucător (creatură cu „coarne"/picioare).
// Desenat cu hașură (checkerboard) → se distinge clar de jucătorul plin.
static const char *ENEMY_ART[8] = {
    "..##....##..",
    "..##....##..",
    "..########..",
    ".##########.",
    "###.####.###",
    ".##########.",
    "..##....##..",
    ".##......##.",
};

// Desenează o figură-sprite (block art) la (x,y), cu oglindire opțională. Dacă hatch=true,
// desenează doar pixelii pe „tabla de șah" → aspect hașurat (ca inamicul original).
static void draw_sprite_art(const char *art[8], int x, int y, bool flip, bool hatch) {
    Color fg = render_fg();
    for (int row = 0; row < 8; row++)
        for (int col = 0; col < SPR_W; col++)
            if (art[row][col] == '#') {
                int dx = flip ? (SPR_W - 1 - col) : col;
                if (hatch && (((x + dx) + (y + row)) & 1)) continue;   // hașură
                DrawRectangle(x + dx, y + row, 1, 1, fg);
            }
}

// SPRITE_DRAW (014030): figura player. NOTĂ: animația (walk/climb) folosește deocamdată
// același frame; frame-urile de mers/cățărat se capturează la fel (frame-diff la acele poze).
void render_player(Renderer *r, const Player *p, GameState gs, float state_timer) {
    (void)r;
    bool show = (gs == GS_PLAYING || gs == GS_LEVEL_WIN || gs == GS_ALL_WIN)
             || (gs == GS_DEAD && (int)(state_timer * 8) % 2 == 0);
    if (!show) return;
    int f = p->anim_frame & 1;
    const char **art = (p->anim == PA_CLIMB) ? PLAYER_CLIMB[f]
                     : (p->anim == PA_WALK)  ? PLAYER_WALK[f]
                     :                          PLAYER_ART;     // PA_STAND
    draw_sprite_art(art, (int)p->px + 2, (int)p->py + PLAYFIELD_Y, p->facing < 0, false);
}

void render_enemy(Renderer *r, const Enemy *e) {
    (void)r;
    if (!e->active) return;
    // KI-7: siluetă distinctă + hașurată (≠ jucătorul plin). În timpul warmup-ului clipește
    // ușor (semnal că încă nu s-a activat).
    bool blink = (e->warmup > 0.0f) && ((int)(e->warmup * 6) & 1);
    if (blink) return;
    draw_sprite_art(ENEMY_ART, e->col * TILE_W + 2, e->row * TILE_H + PLAYFIELD_Y, e->dir < 0, true);
}

// HUD_RENDER (003652): bara de sus — "Счет {scor}   Попытки {vieți}" (text rusesc, fidel).
void render_hud(Renderer *r, const Score *s) {
    char buf[32];
    Color fg = render_fg();
    render_text(r, T(STR_SCORE), 80, 3, 11, fg);
    snprintf(buf, sizeof buf, "%d", s->score);
    render_text(r, buf, 170, 3, 11, fg);
    render_text(r, T(STR_LIVES), 280, 3, 11, fg);              // "Попытки" / "Incercari"
    snprintf(buf, sizeof buf, "%d", s->lives);
    render_text(r, buf, 400, 3, 11, fg);
}

// "КЛАД" mare ca TILE-ART (din blocuri, ca originalul care încarcă un "title level").
// 7 rânduri; litere de 5 coloane + 1 gap.  '#' = bloc.   К  Л  А  Д
static const char *KLAD_ART[7] = {
    "#...# .#### .###. .####.",
    "#..#. .#..# #...# .#..#.",
    "#.#.. .#..# #...# .#..#.",
    "##... .#..# ##### .#..#.",
    "#.#.. .#..# #...# .#..#.",
    "#..#. .#..# #...# ######",
    "#...# #...# #...# #....#",
};

// TITLE_SEQ (002072): КЛАД (tile-art) + credite (font REAL УКНЦ). Originalul NU are
// prompt "нажмите клавишу" pe titlu — doar КЛАД + Николаев 1987 + Баранов.
void render_title(Renderer *r) {
    Color fg = render_fg();
    int bw = 8, bh = 8;                          // mărimea unui bloc (px)
    int cols = (int)strlen(KLAD_ART[0]);
    int x0 = (VW - cols * bw) / 2, y0 = 30;
    for (int row = 0; row < 7; row++)
        for (int col = 0; col < cols; col++)
            if (KLAD_ART[row][col] == '#')
                DrawRectangle(x0 + col*bw, y0 + row*bh, bw, bh, fg);
    render_text(r, "Николаев 1987",  VW/2 - 52, 120, 13, fg);   // autor — nu se traduce
    render_text(r, "Баранов",        VW/2 - 28, 142, 13, fg);
    render_text(r, T(STR_LANG_HINT), VW/2 - 52, 168, 10, fg);   // L = comută limba
}

// DIFF_SELECT (003234): alegere viteză 1-4 (1 = rapid, 4 = lent).
void render_speed_select(Renderer *r, int speed) {
    Color fg = render_fg();
    render_text(r, T(STR_SPEED), VW/2 - 84, 64, 16, fg);
    for (int i = 1; i <= 4; i++) {
        char b[4]; snprintf(b, sizeof b, "%d", i);
        Color c = (i == speed) ? (Color){255,230,120,255} : fg;
        render_text(r, b, VW/2 - 36 + (i-1)*24, 100, 18, c);
    }
    render_text(r, T(STR_SPEED_HINT), VW/2 - 110, 140, 11, fg);
}

// Upscale la fereastră. MĂSURAT din emulatorul original (02_gameplay.png): tile-urile
// se afișează 16px lat × 9.5px înalt — LATE, nu pătrate. Labirintul e centrat pe ecranul
// albastru cu margini (~65px laterale în 640). Deci: lățime plină (16px/tile), înălțime
// ×1.19 (8→9.5px/tile), fundal albastru ca УКНЦ, centrat cu margini (~0.9 din fereastră).
void render_present(Renderer *r) {
    BeginDrawing();
    ClearBackground(render_bg());                  // ecran УКНЦ albastru (nu negru)
    const float aspect_w = (float)VW;              // 512 — tile-uri 16px late (ca originalul)
    const float aspect_h = (float)VH * 1.19f;      // ~228 — tile-uri ~9.5px înalte
    float sx = (float)GetScreenWidth()  / aspect_w;
    float sy = (float)GetScreenHeight() / aspect_h;
    float s  = ((sx < sy) ? sx : sy) * 0.92f;      // 0.92 → margine albastră de jur împrejur
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
