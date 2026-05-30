// uknc_defs.h — Constante exacte din КЛАД 1987 Баранов (УКНЦ МС-0511)
// Sursa: disassembly/annotated/uknc_klad_1987.asm
// NIMIC nu este inventat. Fiecare valoare are referință la adresa ASM.
#pragma once
#include <stdint.h>

// =============================================================================
// TILE INDICES (din harta de nivel, 0-15, nibble-encoded)
// Sursa: DAT_TILE_* symbols + COLLISION_MAP_BUILD unpacking (013524)
// =============================================================================

typedef uint8_t TileIdx;

// Fiecare valoare = exact indexul din tile bank @ 017450 (stride=16 bytes)
#define TIDX_AIR        0   // 017450: background / aer — never blitted
#define TIDX_LADDER     1   // 017470: scară tip 1 (3C 3C FF FF... cross pattern)
#define TIDX_EXIT       2   // 017510: ieșire nivel (same pixels as air, collision=exit)
#define TIDX_UNUSED3    3   // 017530: neutilizat
#define TIDX_GOLD_A     4   // 017550: aur frame A → la colectare: SCORE_ADD
#define TIDX_GOLD_B     5   // 017570: aur frame B → la colectare: BONUS_LIFE_ADD
#define TIDX_GOLD_C     6   // 017610: aur frame C → la colectare: LEVEL_COMPLETE
#define TIDX_WATER      7   // 017630: apă — letal (state=15/dead la contact)
#define TIDX_LADDER2    8   // 017650: scară tip 2 (aceleași pixels ca tip 1)
#define TIDX_WALL_A     9   // 017670: pereți tip A
#define TIDX_WALL_B    11   // 017730: pereți border stânga/dreapta
#define TIDX_WALL_C    12   // 017750: pereți interior (cel mai frecvent)
#define TIDX_WALL_D    13   // 017770: pereți border jos
#define TIDX_WATER2    14   // 020010: apă variantă animată — și ea letală

// Clasele logice derivate din tile index (din PLAYER_STATE_CHECK + WATER_COLLISION):
static inline int tidx_is_ladder(TileIdx t) { return t == TIDX_LADDER || t == TIDX_LADDER2; }
static inline int tidx_is_wall(TileIdx t)   { return t==9||t==10||t==11||t==12||t==13; }
static inline int tidx_is_water(TileIdx t)  { return t == TIDX_WATER || t == TIDX_WATER2; }
static inline int tidx_is_gold(TileIdx t)   { return t>=4 && t<=6; }
static inline int tidx_is_exit(TileIdx t)   { return t == TIDX_EXIT; }
static inline int tidx_is_solid(TileIdx t)  { return tidx_is_wall(t); }

// =============================================================================
// TILE STATE BYTES — valorile din BUF_TILE_WORK detectate de PLAYER_STATE_CHECK
// Sursa: PLAYER_STATE_CHECK (012570) + WATER_COLLISION (006462)
// BUF_TILE_WORK (014550): 22×32 octeți, fiecare = tile index + entity flags
// =============================================================================

#define TSTATE_GOLD_COLLECT  4   // 012626: CMPB #4,(R3) → SCORE_ADD (+10)
#define TSTATE_BONUS_LIFE    5   // 012642: CMPB #5,(R3) → BONUS_LIFE_ADD
#define TSTATE_LEVEL_WIN     6   // 012656: CMPB #6,(R3) → LEVEL_COMPLETE sequence
#define TSTATE_ENTITY_ACTIVE 0o11 // 011 octal = 9 decimal: enemy occupies tile → death
#define TSTATE_WATER_LETHAL  0o15 // 015 octal = 13: water contact → death sequence
#define TSTATE_DEAD          0o17 // 017 octal = 15: player dead state
#define TSTATE_LEVEL_NEXT    0o20 // 020 octal = 16: set la LEVEL_COMPLETE

// =============================================================================
// VARIABILE GLOBALE (RAM la runtime)
// Sursa: VAR_* symbols din symbol table
// =============================================================================

// Inițializate la GAME_INIT (004000):
#define VAR_LIVES_INIT  0333 // 0o333 = 219 decimal; afișat ca "9" (FN_LIVES_DISPLAY)
#define VAR_SCORE_INIT  0    // CLR @#017440 la GAME_INIT
#define VAR_SPEED_DEFAULT 1000 // viteza implicită; variante: 400/1000/2000/4000

// Dimensiuni hartă (confirmate din COLLISION_MAP_BUILD stride):
#define MAP_ROWS  22   // 22 rânduri per nivel
#define MAP_COLS  32   // 32 coloane per nivel (16 bytes × 2 nibble)
#define MAP_STRIDE 352 // 0o540 bytes per nivel (ADD #540 @ LEVEL_COMPLETE:001050)

// Număr niveluri:
#define NUM_LEVELS 10  // TBL_LEVEL_PTRS: 10 intrări × 2 bytes

// =============================================================================
// ENTITY RECORD LAYOUT
// Sursa: ACT_DISPATCH (001436), SPRITE_DRAW (014030), PLAYER_MOVE_STEP (012740)
// Player la 014420, Enemy1 la 014430, Enemy2 la 014440
// =============================================================================

// Offset-uri în entity record (în bytes, fiecare word = 2 bytes):
#define EREC_STATE    0  // word +0:  stare entitate (010=activ, 0=inactiv)
#define EREC_TILEPTR  2  // word +2:  pointer tile în BUF_TILE_WORK
#define EREC_X        4  // word +4:  coloana X (screen bytes)
#define EREC_Y        6  // word +6:  offset Y ecran
#define EREC_SPRFRAME 8  // word +8:  animation frame curent
#define EREC_DIR     10  // word +10: direcție / frame animație

#define EREC_STATE_ACTIVE  010  // 0o10 = 8: entitate activă
#define EREC_STATE_DEAD    0    // 0: inactivă/moartă

// =============================================================================
// SCORE
// Sursa: FN_SCORE_ADD (003764): ADD #12, @#017440 (octal 12 = decimal 10)
// =============================================================================

#define SCORE_PER_GOLD 10  // +10 la colectare aur (TIDX_GOLD_A)

// =============================================================================
// ANIMAȚIE — throttle counters
// Sursa: FN_SPRITE_ANIM_A/B/C/D (010102, 010142, 007202, 007242)
// "throttle 8-frame" = ciclează 8 frame-uri; "throttle 5-frame" = 5 frame-uri
// =============================================================================

#define ANIM_FRAMES_HORIZ   8   // frame-uri animație mișcare orizontală inamic
#define ANIM_FRAMES_VERT    5   // frame-uri animație mișcare verticală (scară)

// =============================================================================
// GRAFICE
// Sursa: DAT_TILE_PIXELS (017450) + sprite bank (031300)
// =============================================================================

#define TILE_W  8   // lățime tile pixels
#define TILE_H  8   // înălțime tile pixels
#define TILE_BYTES 16  // bytes per tile (8 pixel-plane + 8 color-plane)

#define TILE_BANK_ADDR  017450  // adresa tile bank (BK/УКНЦ address space)
#define TILE_BANK_COUNT 32      // 32 tile-uri (0-15 level tiles, 16-31 sprite extras)
#define SPRITE_BANK_ADDR 031300 // adresa sprite bank
#define SPRITE_FRAME_COUNT 208  // 208 frame-uri animație (031300–037677)

// Paleta УКНЦ 2bpp (pixel_plane, color_plane) → color index:
// 0 (0,0) = negru    1 (1,0) = verde    2 (0,1) = galben    3 (1,1) = alb
#define PAL_BLACK   0
#define PAL_GREEN   1
#define PAL_YELLOW  2
#define PAL_WHITE   3
