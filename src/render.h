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

// LEVEL_RENDER (004776): ASL×4 + ADD #17450 → tile_addr = idx*16 + 0o17450
// Tileset texture: un strip orizontal de slots 8×8 px
// Slot 0-15   : tile-uri hartă (TILE_GFX[0..15], din binar)
// Slot 16-19  : player walk frames (SPRITE_GFX[0..3])
// Slot 20     : player climb        (SPRITE_GFX[8])
// Slot 21     : player death        (SPRITE_GFX[12])
// Slot 22-25  : enemy walk frames   (SPRITE_GFX[16..19])
enum {
    RS_MAP     =  0,  // tile-uri hartă (índicele = TileIdx direct)
    RS_PLW     = 16,  // player walk
    RS_PLC     = 20,  // player climb
    RS_PLD     = 21,  // player death
    RS_ENW     = 22,  // enemy walk
    RS_COUNT   = 26,
};

typedef struct {
    Texture2D       tileset;
    RenderTexture2D target;   // 256×192 render target (upscalat la fereastră)
} Renderer;

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

// Composite: upscale nearest-neighbour target → fereastră
void render_present(Renderer *r);
