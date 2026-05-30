// score.h — scor, vieți, nivel
// Sursa ASM: SCORE_ADD (003764), BONUS_LIFE_ADD (003746),
//            LEVEL_COMPLETE (001034), GAME_INIT (004000)
#pragma once
#include "klad.h"

// GAME_INIT (004000): MOV #0333, @#LIVES; CLR @#SCORE
// Număr niveluri: 10 (TBL_LEVEL_PTRS: 10 × 2 bytes la 001230)
#define NUM_LEVELS  10

typedef struct {
    int score;  // VAR_SCORE (017440): ADD #012 per aur = +10 decimal
    int lives;  // VAR_LIVES (017436): init 0o333 → afișat "9"
    int level;  // 0-based (0–9)
} Score;

// GAME_INIT (004000): MOV #0333 → 9 vieți; CLR score; nivel = 0
void score_init(Score *s);

// SCORE_ADD (003764): ADD #012, @#017440 — octal 012 = decimal 10
void score_add_gold(Score *s);

// BONUS_LIFE_ADD (003746): INC @#017436
void score_add_life(Score *s);

// LEVEL_COMPLETE (001034): avansează pointer nivel, verifică wrap
// Returnează true dacă au fost completate toate nivelurile
bool score_level_advance(Score *s);

// PLAYER_DEATH (001016): lives-- → dacă 0 → GAME_OVER
bool score_lose_life(Score *s);
