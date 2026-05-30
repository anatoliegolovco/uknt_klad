// i18n.h — șiruri de text (română implicit)
#pragma once
typedef enum {
    STR_SCORE, STR_LEVEL, STR_LIVES,
    STR_LEVEL_CLEAR, STR_GAME_OVER,
    STR_COUNT
} StrId;
static const char *const STRINGS[][STR_COUNT] = {
    {"Scor", "Nivel", "Vieti", "Nivel complet!", "Joc terminat"},
};
#define LANG 0
#define T(id) STRINGS[LANG][id]
