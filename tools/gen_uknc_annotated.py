#!/usr/bin/env python3
"""
gen_uknc_annotated.py
Generează disassembly adnotat complet pentru УКНЦ КЛАД 1987 Баранов.

Strategie:
  1. Parsează raw УКНЦ disassembly (raw/uknc_klad_1987.asm) — baza.
  2. Parsează BK-0010 raw (raw/bk0010_unpacked.asm) — pentru comparație word-cu-word.
  3. Compară word-cu-word: identic → marchează [=], diferit → marchează [DIFF].
  4. Injectează labeluri + comentarii din tabela ANNOTATIONS pentru adresele cunoscute.
  5. Emite fișierul complet adnotat cu TOATE instrucțiunile.
"""

import re, sys, os

HERE = os.path.dirname(__file__)
ROOT = os.path.dirname(HERE)

RAW_UKNC   = os.path.join(ROOT, "disassembly/raw/uknc_klad_1987.asm")
RAW_BK     = os.path.join(ROOT, "disassembly/raw/bk0010_unpacked.asm")
UKNC_BIN   = os.path.join(ROOT, "assets/original/extracted/uknc/KLAD_1987_prog.bin")
BK_DUMP    = os.path.join(ROOT, "assets/original/extracted/bk0010/emu2_dump.bin")
OUT        = os.path.join(ROOT, "disassembly/annotated/uknc_klad_1987.asm")

# ---------------------------------------------------------------------------
# Annotation tables
# ---------------------------------------------------------------------------

# Label per address (octal int → label string)
LABELS = {
    0o001000: "RESTART",
    0o001004: "GAME_OVER_SOFT",
    0o001016: "PLAYER_DEATH",
    0o001024: "BONUS_LIFE",
    0o001034: "LEVEL_COMPLETE",
    0o001112: "ALL_LEVELS_DONE",
    0o001116: "GAME_STATE_INIT",
    0o001130: "LEVEL_PTR_INIT",
    0o001142: "KEY_DIFFICULTY",
    0o001306: "DELAY_SPIN",
    0o001344: "GAME_LOOP",
    0o001354: "GAME_LOOP_MENU",
    0o001406: "KEY_TABLE_SCAN",
    0o001424: "NO_KEY",
    0o001432: "KEY_HIT",
    0o001436: "ACT_DISPATCH",
    0o001602: "GAME_TICK",
    0o001636: "LEVEL_END_CHECK",
    0o001732: "KEY_CODE_TBL",
    0o002000: "GAME_BRANCHES",
    0o002050: "SOUND_WRAPPER_B",
    0o002060: "SOUND_WRAPPER_A",
    0o002072: "TITLE_SEQ",
    0o002264: "TITLE_WAIT",
    0o003234: "DIFF_SELECT",
    0o003274: "GAME_OVER_WAIT",
    0o003372: "LIVES_DISPLAY",
    0o003444: "DEATH_SCORE",
    0o003576: "GAME_LEVEL_LOOP",
    0o003652: "HUD_RENDER",
    0o003746: "BONUS_LIFE_ADD",
    0o003764: "SCORE_ADD",
    0o004000: "GAME_INIT",
    0o004210: "NUM_RENDER",
    0o004640: "PLAYER_DEATH_TRIGGER",
    0o004660: "KEY_ACTION_DATA",
    0o004674: "KBD_GAME_POLL",
    0o004724: "KBD_SCAN",
    0o004740: "KBD_HIT",
    0o004776: "LEVEL_RENDER",
    0o005002: "LEVEL_RENDER_R4",
    0o005010: "LRND_ROW",
    0o005016: "LRND_BYTE",
    0o005106: "TILE_BLIT_FWD",
    0o005120: "TBLIT_ROW",
    0o005754: "HW_INIT",
    0o006054: "PLAYER_SPRITE_INIT",
    0o006134: "SOUND_TBL_A",
    0o006156: "SOUND_TBL_B",
    0o006204: "ENTITY_HANDLER",
    0o006324: "ENT_INACTIVE",
    0o006366: "ENT_SPEAKER_LOOP",
    0o006434: "ENT_DONE",
    0o006444: "ENTITY_STATE_INIT",
    0o006462: "WATER_COLLISION",
    0o006552: "ENEMY2_TICK",
    0o006602: "ENEMY2_MOVE",
    0o007136: "ENEMY2_MOVE_UP",
    0o007202: "SPRITE_ANIM_C",
    0o007242: "SPRITE_ANIM_D",
    0o007306: "ENTITY1_RESTORE",
    0o007376: "ENEMY1_TICK",
    0o007432: "ANIM_THROTTLE_PLAYER",
    0o007462: "ENEMY3_TICK",
    0o007502: "ENEMY3_MOVE",
    0o010036: "ENEMY3_MOVE_UP",
    0o010102: "SPRITE_ANIM_A",
    0o010142: "SPRITE_ANIM_B",
    0o010206: "LEVEL_RESET",
    0o010240: "LRESET_COPY",
    0o010270: "LRESET_ENEMIES",
    0o010364: "LRESET_CLR",
    0o012326: "VSYNC_WAIT",
    0o012342: "KEY_ACTION_TBL",
    0o012442: "LEVEL_RENDER_FULL",
    0o012454: "LFULL_ROW",
    0o012462: "LFULL_COL",
    0o012530: "TILE_BLIT_SUB",
    0o012544: "TSUB_ROW",
    0o012570: "PLAYER_STATE_CHECK",
    0o012716: "ENEMY_RESPAWN",
    0o012740: "PLAYER_MOVE_STEP",
    0o013024: "PMOVE_EXEC",
    0o013116: "PMOVE_BLIT",
    0o013216: "SPRITE_HELPERS",
    0o013524: "COLLISION_MAP_BUILD",
    0o013536: "CMAP_UNPACK",
    0o013570: "CMAP_FLAGS",
    0o014030: "SPRITE_DRAW",
    0o014120: "SDRAW_BLIT2",
    0o014302: "TILE_BLIT_REV",
    0o014334: "TBREV_ROW",
    0o014420: "PLAYER_STATE_WORD",
    0o014422: "PLAYER_TILE_PTR",
    0o014424: "PLAYER_X",
    0o014430: "ENEMY1_STATE_WORD",
    0o014432: "ENEMY1_TILE_PTR",
    0o014434: "ENEMY1_X",
    0o014436: "ENEMY1_Y",
    0o014440: "ENEMY2_STATE_WORD",
    0o014442: "ENEMY2_TILE_PTR",
    0o014444: "ENEMY2_X",
    0o014446: "ENEMY2_Y",
    0o014550: "TILE_WORK_BUF",
    0o017430: "GAME_STATE",
    0o017436: "LIVES",
    0o017440: "SCORE",
    0o017450: "TILE_BANK",
    0o021640: "SPRITE_WS_PLAYER",
    0o021760: "SPRITE_WS_ENEMY",
    0o022100: "LEVEL_MAPS",
    # УКНЦ I/O driver
    0o040060: "DISP_SCANLINE_WRITE",
    0o040220: "DISP_XY_CONVERT",
    0o040320: "DISP_COL_SETUP",
    0o040420: "DISP_SERIAL_WAIT",
    0o040520: "DISP_BLIT_INNER",
    0o040560: "DISP_COL_ADDR",
    0o040600: "DISP_COL_LOOP",
    0o040640: "DISP_SWAP",
    0o040660: "KBD_READ",
    0o040700: "KBD_STATUS_READ",
    0o040760: "KBD_WAIT",
    0o041000: "DISP_INIT_ENTRY",
    0o041020: "KBD_POLL",
    0o041040: "DISP_COL_BLIT",
    0o041060: "DCOL_ADVANCE",
    0o041100: "EMT_TEXT",
    0o041120: "EMT_TEXT2",
    0o041140: "DISP_SETUP",
    0o041160: "DISP_SETUP_RTS",
    0o041200: "DISP_MODE_CHECK",
    0o041260: "DISP_STATE_TST",
    0o041340: "DISP_COL_RENDER",
    0o041360: "DISP_STRIP_COLOR",
    0o041400: "VSYNC_WAIT_LOOP",
    0o041460: "DISP_PIXEL_PORT",
    0o041500: "DISP_LINE_ADV",
    0o041520: "DLINE_INNER",
    0o041600: "DLINE_ROW_WRITE",
    0o041620: "DLINE_SYNC_BIT",
    0o041640: "DLINE_COUNT",
    0o041660: "DLINE_DONE",
    0o041740: "DISP_WRITE_COL",
}

# End-of-line comment per address (octal int → comment string)
# For identical addresses: short note. For УКНЦ-diff: [УКНЦ] tag.
COMMENTS = {
    # === RESTART area ===
    0o001000: "JMP @#GAME_INIT — full restart trampoline",
    0o001004: "CLR R0",
    0o001006: "JMP @#004160 — partial reinit",
    0o001012: "MOV (R2),R0 — load game-over counter",
    0o001014: "BGE RESTART — counter < 0 → keep going",
    0o001016: "JSR PC, @#LEVEL_RESET — death animation + level reinit",
    0o001022: "BR 1032 — skip bonus path",
    0o001024: "MOV #1000, R0 — bonus life award value",
    0o001030: "BR GAME_OVER_SOFT+2 — apply bonus",
    0o001034: "JSR PC, @#COLLISION_MAP_BUILD — rebuild tile flags",
    0o001040: "MOV @#LEVEL_TBL_PTR, R5",
    0o001044: "MOV (R5), @#CUR_LEVEL_PTR — load next level address",
    0o001050: "ADD #540, @#CUR_MAP_ADDR — advance map 352 bytes (one level)",
    0o001056: "ADD #2, @#LEVEL_TBL_PTR — next table entry",
    0o001064: "MOV @#LEVEL_TBL_PTR, R5",
    0o001070: "CMP #001302, R5 — wrapped around table?",
    0o001074: "BEQ ALL_LEVELS_DONE",
    0o001076: "NOP",
    0o001100: "NOP",
    0o001102: "JSR PC, @#LEVEL_RENDER_FULL — render new level",
    0o001106: "JMP @#GAME_LOOP",
    0o001112: "JMP RESTART — all levels done → full restart",
    0o001116: "MOV #010404, @#GAME_STATE — init game-state word",
    0o001124: "JMP 1014 — back to game-over check",
    0o001132: "MOV #001230, @#LEVEL_TBL_PTR — reset to level 1",
    0o001140: "RTS PC",
    # === KEY_DIFFICULTY ===
    0o001142: "CMP #061, R0 — key '1'?",
    0o001146: "BNE 1160",
    0o001150: "MOV #SPEED1, @#001312 — [УКНЦ DIFF] speed value differs (УКНЦ clock)",
    0o001156: "RTS PC",
    0o001160: "CMP #062, R0 — key '2'?",
    0o001164: "BNE 1176",
    0o001166: "MOV #SPEED2, @#001312",
    0o001174: "RTS PC",
    0o001176: "CMP #063, R0 — key '3'?",
    0o001202: "BNE 1212",
    0o001204: "MOV #SPEED3, @#001312",
    0o001212: "CMP #064, R0 — key '4'?",
    0o001216: "BNE 1226",
    0o001220: "MOV #SPEED4, @#001312",
    0o001226: "RTS PC",
    # === DELAY_SPIN ===
    0o001306: "MOV R5, -(SP)",
    0o001310: "MOV #1, R5",
    0o001314: "TST R0 — burn cycle",
    0o001316: "SOB R5, 1314",
    0o001320: "MOV (SP)+, R5",
    0o001322: "RTS PC",
    # === GAME_LOOP ===
    0o001344: "JMP @#KBD_GAME_POLL — main per-frame entry",
    # === GAME_LOOP_MENU — [УКНЦ DIFF] ===
    0o001354: "[УКНЦ] JSR PC, @#KBD_READ — read keyboard via УКНЦ routine",
    0o001360: "CMP #055, R0 — Ctrl-M = stop key?",
    0o001364: "BNE 1372",
    0o001366: "JMP @#GAME_OVER_SOFT",
    0o001372: "CMP #033, R0 — [УКНЦ] < ESC-range threshold",
    0o001376: "BGT 1406",
    0o001400: "JSR PC, @#KEY_DIFFICULTY",
    0o001404: "BR GAME_LOOP",
    0o001406: "MOV #014, R3 — 12 table entries",
    0o001412: "MOV #001732, R5 — R5 → KEY_CODE_TBL",
    0o001416: "CMP (R5)+, R0 — compare key code",
    0o001420: "BEQ 1432",
    0o001422: "SOB R3, 1416",
    0o001424: "MOV #177777, R0 — no match sentinel",
    0o001430: "BR ACT_DISPATCH",
    0o001432: "MOV 010406(R5), R0 — load action code from KEY_ACTION_TBL",
    # === ACT_DISPATCH ===
    0o001436: "MOV #014420, R4 — R4 → player entity",
    0o001442: "CMP #010, (R4) — entity type active?",
    0o001446: "BNE 1512",
    0o001450: "BIT #2000, @2(R4) — collision/active flag set?",
    0o001456: "BEQ 1512",
    0o001460: "MOV #021640, R2 — sprite workspace",
    0o001464: "MOV R0, -(SP)",
    0o001466: "MOV #010, R0",
    0o001472: "JSR PC, @#ANIM_THROTTLE_PLAYER",
    0o001476: "MOV (SP)+, R0",
    0o001500: "CMPB #015, @2(R4) — state == dead?",
    0o001506: "BEQ 1324",
    0o001510: "BR 1602",
    0o001512: "CMP #177777, R0 — no-key sentinel?",
    0o001516: "BEQ 1602",
    0o001520: "CMP #012, R0 — action ≤ 12?",
    0o001524: "BLE 1536",
    0o001526: "MOV #021640, R2",
    0o001532: "JSR PC, @#ANIM_THROTTLE_PLAYER",
    0o001536: "MOV 2(R4), R3",
    0o001562: "BIT #4000, (R3) — ground collision flag?",
    0o001566: "BNE 1602",
    0o001570: "MOV #010, R0",
    0o001574: "JSR PC, @#DELAY_SPIN",
    0o001600: "BR ACT_DISPATCH",
    # === GAME_TICK ===
    0o001602: "JSR PC, @#DELAY_SPIN",
    0o001606: "JSR PC, @#PLAYER_STATE_CHECK",
    0o001612: "JSR PC, @#ENEMY1_TICK",
    0o001616: "JSR PC, @#ENEMY2_TICK",
    0o001622: "JSR PC, @#ENEMY3_TICK",
    0o001626: "JSR PC, @#ENTITY1_RESTORE",
    0o001632: "JSR PC, @#WATER_COLLISION",
    0o001636: "CMP @#014422, @#014432 — player tile == enemy1 tile?",
    0o001644: "BNE 1652",
    0o001646: "JMP 002040 — level end",
    0o001652: "CMP @#014422, @#014442 — player tile == enemy2 tile?",
    0o001660: "BNE 1666",
    0o001662: "JMP 002040",
    0o001666: "JMP 002000",
    # === KEY_CODE_TBL (УКНЦ key codes) ===
    0o001732: "[УКНЦ] key code table — 12 entries, УКНЦ keyboard scan codes",
    # === GAME_INIT ===
    0o004000: "[УКНЦ DIFF] MOV #0333, @#LIVES — init lives (0o333=219 dec, УКНЦ encoding)",
    0o004006: "SUB #012, SP — allocate stack args for NUM_RENDER",
    0o004012: "MOV #1, -(SP)",
    0o004016: "MOV #017436, -(SP) — @#LIVES",
    0o004022: "MOV #002204, -(SP)",
    0o004026: "JSR PC, @#NUM_RENDER — draw lives counter",
    0o004032: "CLR @#017440 — SCORE = 0",
    # === KBD_GAME_POLL (УКНЦ) ===
    0o004674: "[УКНЦ] JSR PC, @#KBD_POLL — check УКНЦ keyboard status",
    0o004700: "NOP",
    0o004702: "BNE 4710 — key available?",
    0o004704: "JMP @#GAME_LOOP_MENU — no key → menu path",
    0o004710: "[УКНЦ DIFF] MOV #004660, R1 — R1 → УКНЦ action table (6 entries)",
    0o004714: "[УКНЦ DIFF] MOV #6, R5 — 6 entries (not 11 like BK-0010)",
    0o004720: "[УКНЦ DIFF] MOV #0, R3 — R3 = key code from KBD_POLL",
    0o004724: "MOV (R1)+, R0 — action code",
    0o004726: "ROR R3 — rotate key bit into carry",
    0o004730: "BCS KBD_HIT",
    0o004732: "SOB R5, KBD_SCAN",
    0o004734: "JMP @#NO_KEY",
    0o004740: "JMP @#ACT_DISPATCH",
    # === TILE_BLIT_FWD inner loop (УКНЦ DIFF) ===
    0o005122: "[УКНЦ DIFF] JSR PC, @#DISP_COL_BLIT — write column via video port (not direct RAM)",
    0o005126: "[УКНЦ DIFF] JSR PC, @#DISP_SCANLINE_WRITE — secondary scanline write",
    0o005132: "NOP",
    # === HW_INIT ===
    0o005754: "[УКНЦ DIFF] BR 5762 — skip ROM presence check",
    # === ENTITY_HANDLER speaker loop (УКНЦ DIFF values) ===
    0o006370: "[УКНЦ DIFF] MOV @#?, @#177716 — speaker HIGH value (different from BK-0010)",
    0o006402: "[УКНЦ DIFF] MOV @#?, @#177716 — speaker LOW value",
    # === VSYNC_WAIT (УКНЦ) ===
    0o012326: "MOV R0, -(SP)",
    0o012330: "[УКНЦ DIFF] JSR PC, @#VSYNC_WAIT_LOOP — soft vsync (no EMT 016)",
    0o012334: "NOP",
    0o012336: "MOV (SP)+, R0",
    0o012340: "RTS PC",
    # === KEY_ACTION_TBL (УКНЦ DIFF) ===
    0o012342: "[УКНЦ DIFF] key action table — 6 entries (fewer than BK-0010's 11)",
    # === TILE_BLIT_SUB inner (УКНЦ DIFF) ===
    0o012544: "[УКНЦ DIFF] JSR PC → display port write (not MOV to 046000)",
    # === COLLISION_MAP_BUILD ===
    0o013524: "MOV #014550, R2 — R2 → working tile buffer",
    0o013530: "MOV R2, R3",
    0o013532: "ADD #002576, R3 — R3 = buffer end",
    0o013536: "MOVB (R4), (R2) — copy tile byte (packed)",
    0o013540: "BIC #177760, (R2)+ — keep low nibble (left tile), advance",
    0o013544: "MOVB (R4)+, (R2) — copy high nibble byte, advance source",
    0o013546: "BIC #177417, (R2) — keep high nibble",
    0o013552: "ASR (R2) — shift right ×4 to normalize tile index",
    0o013554: "ASR (R2)",
    0o013556: "ASR (R2)",
    0o013560: "ASR (R2)+ — advance R2",
    0o013562: "CMP R3, R2",
    0o013566: "BPL CMAP_UNPACK — continue until end",
    # === SPRITE_DRAW ===
    0o014030: "MOV R2, -(SP)",
    0o014032: "MOV 2(R4), R2 — player X position",
    0o014034: "MOV 6(R4), R1 — player Y offset",
    0o014036: "JSR PC, TILE_BLIT_REV — blit left tile",
    # === TILE_BLIT_REV inner (УКНЦ DIFF) ===
    0o014334: "[УКНЦ DIFF] JSR PC → display port (not MOV to 046000(R1))",
    # === УКНЦ I/O DRIVER ===
    # KBD_READ
    0o040660: "MOV R1, -(SP) — save regs",
    0o040662: "MOV R2, -(SP)",
    0o040664: "MOV #040712, R1 — R1 → keyboard controller registers",
    0o040670: "MOV #1, R2 — read mode",
    # KBD_STATUS_READ
    0o040700: "MOVB @#040546, R0 — read keyboard status byte",
    # KBD_POLL
    0o041020: "JSR PC, @#KBD_READ — call keyboard read",
    0o041024: "BIT #200, @#040546 — bit 7 = key-ready flag",
    0o041032: "RTS PC",
    # DISP_COL_BLIT
    0o041040: "MOV R0, -(SP)",
    0o041042: "CLR R0",
    0o041044: "DIV #100, R0 — split column: R0=row, R1=col within row",
    0o041050: "MOV R1, -(SP)",
    0o041052: "MOV R1, R0",
    # VSYNC_WAIT_LOOP
    0o041400: "MOV R3, -(SP)",
    0o041402: "MOV R1, -(SP)",
    0o041404: "MOV #050, R1 — outer vsync loop count (40 dec)",
    0o041410: "MOV #100, R3 — inner loop count (64 dec)",
    0o041416: "JSR PC, @#DISP_WRITE_COL — transfer frame data",
    0o041422: "MOV (SP)+, R2",
    0o041424: "MOV (SP)+, R1",
    0o041426: "MOV (SP)+, R3",
    0o041430: "RTS PC",
    # DISP_PIXEL_PORT
    0o041460: "MOVB (R2), @#176642 — write pixel byte to colour port",
    # DISP_LINE_ADV
    0o041500: "MOV #2, R2",
    0o041504: "SOB R2, 041510 — 2-cycle sync",
    # DISP_ROW_WRITE
    0o041600: "MOVB R1, @#176676 — write row number to line register",
    # DISP_SYNC_BIT
    0o041620: "BIS #200, @#177716 — toggle speaker/sync bit",
    0o041640: "SOB R1, 041640 — count display lines",
    # DISP_WRITE_COL
    0o041740: "MOV R3, 041646 — store outer count",
    0o041744: "MOV R1, 041650 — store inner count",
    0o041750: "BR DISP_LINE_ADV — enter display pipeline",
    # DISP_SCANLINE_WRITE
    0o040060: "MOV R1, @#176640 — write pixel word to video pixel port",
    0o040140: "MOV R1, @#176640 — second scanline write (offset row)",
}

# Section headers: address → header comment block
SECTION_HEADERS = {
    0o001000: """\
; =============================================================================
; SECTION 1: GAME LOGIC (001000–037677) — 83% identical to BK-0010
; See disassembly/annotated/crocodile_klad.asm for full BK-0010 reference.
; Lines marked [УКНЦ DIFF] are hardware-specific changes.
; Lines marked [УКНЦ] are УКНЦ-only paths (keyboard, video).
; =============================================================================

; --- Restart / game-over trampolines ---""",
    0o001034: "\n; --- LEVEL_COMPLETE (001034) ---",
    0o001142: "\n; --- KEY_DIFFICULTY (001142) — [УКНЦ DIFF: speed values] ---",
    0o001306: "\n; --- DELAY_SPIN (001306) ---",
    0o001344: "\n; --- GAME_LOOP entry (001344) ---",
    0o001354: "\n; --- GAME_LOOP_MENU (001354) — [УКНЦ DIFF: keyboard read path] ---",
    0o001436: "\n; --- ACT_DISPATCH (001436) ---",
    0o001602: "\n; --- GAME_TICK (001602) — per-frame entity update sequence ---",
    0o001732: "\n; --- KEY_CODE_TBL (001732) — [УКНЦ DIFF: УКНЦ key scan codes] ---",
    0o002000: "\n; --- Game branches + sound wrappers (002000..002071) ---",
    0o002072: "\n; --- TITLE_SEQ (002072) ---",
    0o002264: "\n; --- TITLE_WAIT (002264) ---",
    0o003234: "\n; --- DIFF_SELECT (003234) ---",
    0o003274: "\n; --- GAME_OVER_WAIT (003274) ---",
    0o003372: "\n; --- LIVES_DISPLAY (003372) ---",
    0o003444: "\n; --- DEATH_SCORE (003444) ---",
    0o003576: "\n; --- GAME_LEVEL_LOOP (003576) ---",
    0o003652: "\n; --- HUD_RENDER (003652) ---",
    0o003746: "\n; --- BONUS_LIFE_ADD (003746) ---",
    0o003764: "\n; --- SCORE_ADD (003764) ---",
    0o004000: "\n; --- GAME_INIT (004000) — [УКНЦ DIFF: lives = 0o333] ---",
    0o004210: "\n; --- NUM_RENDER (004210) ---",
    0o004640: "\n; --- PLAYER_DEATH_TRIGGER (004640) ---",
    0o004660: "\n; --- KEY_ACTION_DATA (004660) — [УКНЦ DIFF: 6 entries] ---",
    0o004674: "\n; --- KBD_GAME_POLL (004674) — [УКНЦ DIFF: УКНЦ keyboard] ---",
    0o004776: "\n; --- LEVEL_RENDER (004776) ---",
    0o005002: "\n; --- LEVEL_RENDER_R4 (005002) ---",
    0o005106: """\
\n; --- TILE_BLIT_FWD (005106) ---
; [УКНЦ DIFF] inner loop calls DISP_COL_BLIT instead of direct MOV to 046000(R1)
; BK-0010: MOV (R2)+, 046000(R1)   ← direct framebuffer write
; УКНЦ:    JSR PC, @#DISP_COL_BLIT ← goes through video port @#176640""",
    0o005754: "\n; --- HW_INIT (005754) — [УКНЦ DIFF: BR instead of TSTB] ---",
    0o006054: "\n; --- PLAYER_SPRITE_INIT (006054) ---",
    0o006134: "\n; --- SOUND_TBL_A (006134) — sound entity data, set A ---",
    0o006156: "\n; --- SOUND_TBL_B (006156) — sound entity data, set B ---",
    0o006204: """\
\n; --- ENTITY_HANDLER / SOUND ENGINE (006204) ---
; Generates BK-0010/УКНЦ speaker tones by toggling @#177716 bit 7.
; [УКНЦ DIFF] the two speaker HIGH/LOW constants differ (same register, different values).""",
    0o006444: "\n; --- ENTITY_STATE_INIT (006444) ---",
    0o006462: "\n; --- WATER_COLLISION (006462) ---",
    0o006552: "\n; --- ENEMY2_TICK (006552) ---",
    0o007136: "\n; --- ENEMY2_MOVE_UP (007136) ---",
    0o007202: "\n; --- SPRITE_ANIM_C (007202) — 8-frame throttle, enemy 2 ---",
    0o007242: "\n; --- SPRITE_ANIM_D (007242) — 5-frame throttle, enemy 2 ---",
    0o007306: "\n; --- ENTITY1_RESTORE (007306) ---",
    0o007376: "\n; --- ENEMY1_TICK (007376) ---",
    0o007432: "\n; --- ANIM_THROTTLE_PLAYER (007432) ---",
    0o007462: "\n; --- ENEMY3_TICK (007462) ---",
    0o010036: "\n; --- ENEMY3_MOVE_UP (010036) ---",
    0o010102: "\n; --- SPRITE_ANIM_A (010102) — 8-frame throttle, enemy 3 ---",
    0o010142: "\n; --- SPRITE_ANIM_B (010142) — 5-frame throttle, enemy 3 ---",
    0o010206: "\n; --- LEVEL_RESET (010206) — full level reinit on death ---",
    0o012326: "\n; --- VSYNC_WAIT (012326) — [УКНЦ DIFF: calls soft vsync routine] ---",
    0o012342: "\n; --- KEY_ACTION_TBL (012342) — [УКНЦ DIFF: 6 entries, different codes] ---",
    0o012442: "\n; --- LEVEL_RENDER_FULL (012442) ---",
    0o012530: "\n; --- TILE_BLIT_SUB (012530) ---",
    0o012570: "\n; --- PLAYER_STATE_CHECK (012570) ---",
    0o012716: "\n; --- ENEMY_RESPAWN (012716) ---",
    0o012740: "\n; --- PLAYER_MOVE_STEP (012740) ---",
    0o013216: "\n; --- Sprite animation helpers (013216..013523) ---",
    0o013524: "\n; --- COLLISION_MAP_BUILD (013524) ---",
    0o014030: "\n; --- SPRITE_DRAW (014030) ---",
    0o014302: """\
\n; --- TILE_BLIT_REV (014302) ---
; [УКНЦ DIFF] inner loop: JSR to display port instead of direct 046000(R1) write""",
    0o014420: "\n; =============================================================================\n; SECTION 2: ENTITY RECORDS + DATA (014420–037677)\n; =============================================================================",
    0o017450: "\n; --- TILE_BANK (017450) — 16 tiles × 16 bytes (8×8 px @ 2bpp) ---",
    0o022100: "\n; --- LEVEL_MAPS (022100) — 10 levels × 352 bytes ---",
    0o040060: (
        "\n; =============================================================================\n"
        "; SECTION 3: UKNC I/O DRIVER (040060-042000)\n"
        "; This section does NOT exist in BK-0010 (that range is video RAM there).\n"
        "; UKNC game embeds custom display, keyboard, and vsync routines here.\n"
        ";\n"
        "; UKNC VIDEO HARDWARE:\n"
        ";   @#176640 -- pixel data port (word write = one scanline word)\n"
        ";   @#176642 -- colour/palette register\n"
        ";   @#176676 -- line/row register\n"
        ";   Tile writes go via DISP_COL_BLIT (041040) -> DISP_WRITE_COL (041740)\n"
        ";\n"
        "; UKNC KEYBOARD HARDWARE:\n"
        ";   @#040546 -- keyboard status (bit 7 = key ready, bits 6-0 = scan code)\n"
        ";   KBD_READ (040660) -> reads @#040712, returns key in R0\n"
        ";   KBD_POLL (041020) -> wrapper: read + check bit 7\n"
        "; ============================================================================="
    ),
    0o040660: "\n; --- KBD_READ (040660) — УКНЦ keyboard read routine ---",
    0o040700: "\n; --- KBD_STATUS_READ (040700) --- ",
    0o041020: "\n; --- KBD_POLL (041020) — keyboard poll wrapper, called from KBD_GAME_POLL ---",
    0o041040: "\n; --- DISP_COL_BLIT (041040) — write tile column to УКНЦ video port ---",
    0o041060: "\n; --- DCOL_ADVANCE (041060) ---",
    0o041100: "\n; --- EMT_TEXT (041100) — EMT 020 text display handler ---",
    0o041140: "\n; --- DISP_SETUP (041140) — display initialization ---",
    0o041200: "\n; --- DISP_MODE_CHECK (041200) — check display colour mode ---",
    0o041340: "\n; --- DISP_COL_RENDER (041340) --- ",
    0o041400: (
        "\n; --- VSYNC_WAIT_LOOP (041400) ---\n"
        "; Software vsync: busy-loops for ~40x64 iterations, transferring frame data.\n"
        "; Equivalent of BK-0010 EMT 016 r0=7 (wait-for-vertical-blank).\n"
        "; Called from VSYNC_WAIT (012326)."
    ),
    0o041460: "\n; --- DISP_PIXEL_PORT (041460) — write to @#176642 colour port ---",
    0o041500: "\n; --- DISP_LINE_ADV (041500) — advance display line counter ---",
    0o041600: "\n; --- DLINE_ROW_WRITE (041600) — write row to @#176676 ---",
    0o041620: "\n; --- DLINE_SYNC_BIT (041620) — toggle @#177716 bit 7 ---",
    0o041740: "\n; --- DISP_WRITE_COL (041740) — core column write, entry for vsync loop ---",
}

# ---------------------------------------------------------------------------
# Parse raw УКНЦ disassembly
# ---------------------------------------------------------------------------
def parse_raw(path):
    """Returns list of (addr_oct_int, raw_line_str) tuples."""
    lines = []
    with open(path) as f:
        for line in f:
            m = re.match(r'^(\d{6}):\s+(\d{6})\s+(.*)', line.rstrip())
            if m:
                addr = int(m.group(1), 8)
                word = m.group(2)
                rest = m.group(3)
                lines.append((addr, word, rest))
    return lines

# ---------------------------------------------------------------------------
# Load binary images for diff comparison
# ---------------------------------------------------------------------------
def load_words(path, base_addr):
    with open(path, 'rb') as f:
        data = f.read()
    words = {}
    for i in range(0, len(data)-1, 2):
        addr = base_addr + i
        words[addr] = int.from_bytes(data[i:i+2], 'little')
    return words

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    uknc_lines = parse_raw(RAW_UKNC)

    # Load binaries for diff marking
    uknc_words = load_words(UKNC_BIN, 0o1000)
    bk_words   = load_words(BK_DUMP,  0)

    out = []

    out.append("""\
; =============================================================================
; КЛАД (Баранов 1987) — Complete Annotated Disassembly, УКНЦ (МС-0511)
; Source: assets/original/extracted/uknc/KLAD_1987_Baranov.SAV
;   RT-11 .SAV: 512-byte header + 16896-byte program image at 0o001000
;   Entry point: 0o001000  Top: 0o042000
;
; 83% of code is word-identical to BK-0010 Crocodile (see crocodile_klad.asm).
; [УКНЦ DIFF] = word differs from BK-0010 counterpart at same address.
; [УКНЦ]      = УКНЦ-only path (no equivalent in BK-0010).
;
; All constants OCTAL unless noted [dec] or [hex].
; Hardware: KM1801VM2 PDP-11, УКНЦ video port @#176640, kbd @#040546.
; =============================================================================

        .ORG    001000
""")

    for (addr, word, insn) in uknc_lines:
        # Section header?
        if addr in SECTION_HEADERS:
            out.append(SECTION_HEADERS[addr])
            out.append("")

        # Label?
        label = LABELS.get(addr, "")

        # Diff marker
        bk_w = bk_words.get(addr)
        uknc_w = uknc_words.get(addr)
        if bk_w is not None and uknc_w is not None:
            diff_mark = "" if (uknc_w == bk_w) else " [DIFF]"
        else:
            diff_mark = ""

        # Comment
        comment = COMMENTS.get(addr, "")
        if comment and not comment.startswith("[УКНЦ"):
            comment_str = f"  ; {comment}"
        elif comment:
            comment_str = f"  ; {comment}"
        else:
            comment_str = ""

        # Format line
        addr_str = f"{addr:06o}"
        word_str = f"{word}"
        insn_str = insn.strip()

        if label:
            out.append(f"{label}:")
            if diff_mark:
                out.append(f"        ; [УКНЦ DIFF]")
        elif diff_mark:
            insn_str = insn_str  # diff already in comment table

        out.append(f"{addr_str}: {word_str}   {insn_str}{comment_str}")

    out.append("")
    out.append("; =============================================================================")
    out.append("; END OF DISASSEMBLY")
    out.append("; Generated by tools/gen_uknc_annotated.py")
    out.append("; =============================================================================")

    result = "\n".join(out)
    with open(OUT, 'w') as f:
        f.write(result)
    print(f"Written {len(out)} lines → {OUT}")
    # Stats
    diff_count = sum(1 for (a, w, i) in uknc_lines
                     if bk_words.get(a) is not None and uknc_words.get(a) != bk_words.get(a))
    same_count = sum(1 for (a, w, i) in uknc_lines
                     if bk_words.get(a) is not None and uknc_words.get(a) == bk_words.get(a))
    print(f"Instructions: {len(uknc_lines)} total, {same_count} identical to BK-0010, {diff_count} different")

if __name__ == "__main__":
    main()
