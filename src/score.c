// score.c — scor, vieți, nivel
#include "score.h"

// GAME_INIT (004000): MOV #0333, @#LIVES; CLR @#SCORE
void score_init(Score *s) {
    s->score = 0;
    s->lives = 9;   // 0o333 în original, afișat "9"
    s->level = 0;
}

// SCORE_ADD (003764): JSR VSYNC_WAIT; ADD #012, @#017440
// Octal 012 = decimal 10 — confirmat din instrucțiunea ADD directă
void score_add_gold(Score *s) {
    s->score += 10;
}

// BONUS_LIFE_ADD (003746): JSR VSYNC_WAIT; INC @#017436; JSR LIVES_DISPLAY
void score_add_life(Score *s) {
    s->lives++;
}

// LEVEL_COMPLETE (001034):
//   ADD #540, @#CUR_MAP_ADDR  → stride 352 bytes per nivel
//   ADD #2,   @#LEVEL_TBL_PTR → avansează pointer tabelă
//   CMP #1302, R5             → dacă am trecut de ultimul nivel → ALL WIN
bool score_level_advance(Score *s) {
    s->level++;
    return s->level >= NUM_LEVELS;
}

// PLAYER_DEATH (001016): JSR LEVEL_RESET; lives--
bool score_lose_life(Score *s) {
    s->lives--;
    return s->lives <= 0;
}
