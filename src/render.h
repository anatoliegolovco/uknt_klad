// render.h — randare tile + sprite + HUD
// Sursa ASM: LEVEL_RENDER (004776), SPRITE_DRAW (014030), HUD_RENDER (003652)
#pragma once
#include "raylib.h"
#include "klad.h"
#include "map.h"
#include "player.h"
#include "enemy.h"
#include "score.h"
#include "i18n.h"

// Tileset = 32 sloturi × 16×8 px, 1bpp (slot index = tile index).
//   slots 0-15  : tile-uri hartă (TILE_GFX[0..15])
//   slots 16-31 : tile-uri caracter (TILE_GFX[16..31])
enum { TILESET_SLOTS = 32 };

typedef struct {
    Texture2D       tileset;
    RenderTexture2D target;   // 512×192 render target (upscalat la fereastră)
} Renderer;

// Paletă 2-culori cu comutare color (УКНЦ) / mono (monitor școlar).
Color render_bg(void);
Color render_fg(void);
void  render_toggle_mono(void);

void render_init(Renderer *r);
void render_shutdown(Renderer *r);

// LEVEL_RENDER (004776): pentru fiecare tile != 0 → draw_slot la (col*8, row*8)
void render_map(Renderer *r, const Map *m);

// SPRITE_DRAW (014030): player la (px, py) cu frame-ul animației curente
void render_player(Renderer *r, const Player *p, GameState gs, float state_timer);

// Enemy sprite cu frame animație orizontal
void render_enemy(Renderer *r, const Enemy *e);

// HUD_RENDER (003652): scor + nivel + vieți în bara de jos
void render_hud(Renderer *r, const Score *s);
void render_debug_player(const Player *p);

// Composite: upscale nearest-neighbour target → fereastră
void render_present(Renderer *r);
