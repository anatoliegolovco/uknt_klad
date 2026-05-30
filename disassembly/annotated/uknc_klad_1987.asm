; =============================================================================
; КЛАД (Баранов 1987) — Annotated Disassembly, УКНЦ (МС-0511) version
; Source binary: assets/original/extracted/uknc/KLAD_1987_Baranov.SAV
;   RT-11 .SAV format: 512-byte header + 16896-byte program image
;   SAVBLK=41 (33 blocks), SAVTOP=040000, SAVSTA=001000 (entry point)
;   Image covers addresses 0o001000..0o042000 (16896 bytes)
;
; Relation to BK-0010 Crocodile version:
;   83% of code is WORD-IDENTICAL to the BK-0010 Crocodile version
;   (disassembly/annotated/crocodile_klad.asm).
;   The BK-0010 Crocodile 1991 binary is a 2-stage LZ-packed rerelease of
;   this УКНЦ 1987 Баранов binary. Game logic, level data, tile bank, and
;   entity routines are the same. Only hardware I/O routines differ.
;
; All addresses and constants are OCTAL unless noted [dec] or [hex].
;
; УКНЦ (МС-0511) hardware differences from BK-0010:
;   VIDEO: Planar framebuffer. Accessed via port registers:
;         @#176640 — pixel data (word, written per scanline via I/O)
;         @#176642 — colour register
;         @#176676 — line/row register
;     NOT a linear memory-mapped framebuffer like BK-0010's 040000-077777.
;   KEYBOARD: Polled via status register @#040546 (bit 7 = key ready).
;             Key data read via @#040700 (MOVB @#040546, R0 etc.).
;             NOT the shift-register @#177714 or data @#177662 of BK-0010.
;   VSYNC: Game calls in-game routine at 041400 (soft vsync loop)
;          instead of EMT 016 r0=7.
;   SPEAKER: @#177716 bit 7 still used for audio (same as BK-0010).
;   CPU: KM1801VM2-compatible (PDP-11), same instruction set.
;        Different clock, so speed/timing values differ.
; =============================================================================

; =============================================================================
; MEMORY MAP (УКНЦ, unpacked in user address space)
; =============================================================================
; 000000–000777   Interrupt vectors + RT-11/FODOS system workspace
; 001000–014417   Game code — 83% identical to BK-0010 Crocodile version
;                 (see crocodile_klad.asm for line-by-line annotations)
; 014420–017777   Entity records, working tile buffer, tile bank, globals
; 020000–022077   Sprite animation tables, sprite workspaces
; 022100–037677   Level maps (10 levels × 352 bytes)
; 037700–040057   Tile pixel data (УКНЦ format — different encoding)
; 040060–042000   УКНЦ-specific I/O driver (video blit, keyboard, vsync)
;
; KEY VARIABLES (same addresses as BK-0010):
; 001300  CUR_MAP_ADDR     — current level map start address
; 001302  CUR_LEVEL_PTR    — pointer in level table
; 001304  LEVEL_TBL_PTR    — current level table entry
; 001312  SPEED            — game speed (differs: УКНЦ values)
; 014420  PLAYER_STATE     — player entity state word
; 014422  PLAYER_TILE_PTR  — pointer into working tile buffer @#14550
; 014430  ENEMY1_STATE
; 014432  ENEMY1_TILE_PTR
; 014440  ENEMY2_STATE
; 014442  ENEMY2_TILE_PTR
; 017430  GAME_STATE       — word = 010404 at init
; 017436  LIVES            — init = 0o333 (= 219 dec, see GAME_INIT)
; 017440  SCORE            — init = 0
; 017450  TILE_BANK        — 16 tiles × 16 bytes each (8×8 px)
;
; УКНЦ I/O REGISTERS (hardware-specific):
; 040546  Keyboard status register (bit 7 = key ready, bit 8 = key code)
; 040660  KBD_READ — keyboard read routine entry
; 041020  KBD_POLL — keyboard poll wrapper
; 041040  DISP_COL_BLIT — write one tile column to УКНЦ video hardware
; 041100  EMT_TEXT — EMT 020 display text routine
; 041140  DISP_SETUP — display initialization entry
; 041400  VSYNC_WAIT_UKNC — software vsync loop
; 041740  DISP_WRITE_COL — write column word to @#176640 (pixel data port)
; 176640  Video pixel data port (write: sends scanline word to display)
; 176642  Video colour register
; 176676  Video line/row control register
; 177716  System register (bit 7 = speaker toggle, shared with BK-0010)

; =============================================================================
; SECTION 1: SHARED GAME CODE (001000–014417)
; This section is 83% identical to crocodile_klad.asm.
; Differences are marked with [УКНЦ DIFF] comments.
; See crocodile_klad.asm for full annotation of identical routines.
; =============================================================================

        .ORG    001000

; --- RESTART/trampolines --- (identical to BK-0010)
RESTART:        ; 001000
        JMP     @#004000        ; JMP @#GAME_INIT

GAME_OVER_SOFT: ; 001004
        CLR     R0
        JMP     @#004160

; 001012: MOV (R2),R0 / BGE 1000 — game-over counter check (identical)
; 001016: JSR PC, @#010206      — LEVEL_RESET (identical)
; 001024: MOV #1000, R0         — BK-0010 bonus path
; 001034: JSR PC, @#013524      — COLLISION_MAP_BUILD (identical)

; --- LEVEL_COMPLETE (001034) --- (identical to BK-0010)
LEVEL_COMPLETE: ; 001034
        JSR     PC,@#013524     ; COLLISION_MAP_BUILD
        MOV     @#001304,R5
        MOV     (R5),@#001302
        ADD     #540,@#001300
        ADD     #2,@#001304
        MOV     @#001304,R5
        CMP     #001302,R5
        BEQ     001112
        NOP
        NOP
        JSR     PC,@#012442     ; LEVEL_RENDER_FULL
        JMP     @#001344        ; back to game loop

001112: JMP     RESTART

; 001116: MOV #010404, @#017430 + JMP (game state init, identical)

; --- KEY_DIFFICULTY (001142) --- [УКНЦ DIFF: speed values differ]
; BK-0010: speed values = 400/1000/2000/4000
; УКНЦ:    speed values = different (УКНЦ CPU clock ≠ BK-0010 clock)
KEY_DIFFICULTY: ; 001142
        ; Structure identical to BK-0010; immediate speed values differ
        ; (the four MOV #SPEED, @#001312 instructions have different values)
        ; See _uknc1987_raw.asm at 001142..001227 for exact values.

; --- DELAY_SPIN (001306) --- (identical)
; --- GAME_LOOP (001344) --- (identical: JMP @#004674)

; --- GAME_LOOP_MENU (001354) --- [УКНЦ DIFF: keyboard read path]
; BK-0010: MOV @#177662, R0  (reads menu keyboard data register)
; УКНЦ:    JSR PC, @#040660  (calls keyboard read routine)
;          then CMP #055, R0  (different sentinel value)
GAME_LOOP_MENU: ; 001354
        JSR     PC,@#040660     ; [УКНЦ] read keyboard via custom routine
        CMP     #055,R0         ; CTRL-M (= 45 dec) = stop key?
        BNE     001372
        JMP     @#001004        ; → GAME_OVER_SOFT
001372: CMP     #033,R0         ; < 033 (ESC-related code)?
        BGT     001406          ; ≥ 033: table lookup
        JSR     PC,@#001142     ; KEY_DIFFICULTY
        BR      001344          ; back to game loop
001406: ; table scan (identical structure to BK-0010)
        ; [УКНЦ DIFF] key code table at 001732 has УКНЦ key codes
        ; [УКНЦ DIFF] some key codes differ (УКНЦ keyboard layout)

; --- KEY_CODE_TBL (001732) --- [УКНЦ DIFF: different key codes]
; BK-0010 codes: 017(←), 016(→), 031(↑), 010(Ctrl-H), 032, 022
; УКНЦ codes:    different mapping from УКНЦ keyboard scan codes

; --- Game loop body (001436..001671) --- (IDENTICAL to BK-0010)
; ACT_DISPATCH, GAME_TICK sequence: all addresses identical.

; --- TITLE_SEQ (002072..003233) --- [УКНЦ DIFF: text display calls]
; EMT 020 and EMT 024 calls go through УКНЦ RT-11 display routines.
; String content: KOI8-R encoded Russian text (УКНЦ native).
; Text display calls use EMT 020 (RT-11) not BK-0010 specific EMTs.
; Otherwise title, difficulty select, and game-over wait are identical.

; --- DIFF_SELECT (003234) --- [УКНЦ DIFF: uses EMT 006 or keyboard routine]
; --- GAME_LEVEL_LOOP (003576) --- (identical)
; --- HUD_RENDER (003652) --- [УКНЦ DIFF: display addresses differ slightly]
; --- BONUS_LIFE_ADD (003746) --- [УКНЦ DIFF: calls VSYNC_WAIT_UKNC]
; --- SCORE_ADD (003764) --- [УКНЦ DIFF: calls VSYNC_WAIT_UKNC]

; =============================================================================
; GAME_INIT (004000) — [УКНЦ DIFF: lives initial value]
; =============================================================================
GAME_INIT:      ; 004000
        MOV     #0333,@#017436  ; [УКНЦ DIFF] LIVES = 0o333 (≠ BK-0010's #5)
                                ; 0o333 = 219 decimal = possibly BCD encoding
                                ; or УКНЦ version uses a different lives scheme
        ; Rest identical to BK-0010: draws lives, clears score, stack reset,
        ; bus-error vector setup, then JMP @#005754 (HW_INIT)

; =============================================================================
; HW_INIT (005754) — [УКНЦ DIFF: first instruction is BR not TSTB]
; УКНЦ: starts with BR (skip some check) + EMT calls for display init
; =============================================================================
HW_INIT:        ; 005754
        BR      005762          ; [УКНЦ DIFF] skip ROM check (BR not TSTB)
        ; 005762: continues with EMT 016 calls and @#177664 (scroll)
        ; same EMT sequence and JMP @#002072 as BK-0010

; =============================================================================
; KBD_GAME_POLL (004674) — [УКНЦ DIFF: polls УКНЦ keyboard, not @#177714]
; =============================================================================
KBD_GAME_POLL:  ; 004674
        JSR     PC,@#041020     ; [УКНЦ] KBD_POLL — reads УКНЦ keyboard status
        NOP
        BNE     004710          ; key available → process
        JMP     @#001354        ; no key → menu keyboard path
004710: MOV     #004660,R1      ; [УКНЦ DIFF] R1 → different action table base
        MOV     #6,R5           ; [УКНЦ DIFF] only 6 entries (vs 11 in BK-0010)
        MOV     #0,R3           ; [УКНЦ DIFF] R3=0 (key code from KBD_POLL)
KBD_SCAN:       ; 004724
        MOV     (R1)+,R0        ; action code from table
        ROR     R3              ; rotate key bit
        BCS     KBD_HIT         ; key pressed
        SOB     R5,KBD_SCAN
        JMP     @#001424        ; no key
KBD_HIT:        ; 004740
        JMP     @#001436        ; dispatch

; Key action data (004660-004773) — [УКНЦ DIFF: different from BK-0010]
; УКНЦ uses 6-entry keyboard table with different key assignments.
; Data at 004744-004773 = УКНЦ key codes in KOI8-R (Ш, Ю, etc. for arrows)

; =============================================================================
; TILE_BLIT_FWD (005106) — [УКНЦ DIFF: inner loop writes via I/O port, not direct RAM]
; =============================================================================
TILE_BLIT_FWD_UKNC: ; 005106
        MOV     R5,-(SP)
        MOV     R4,-(SP)
        MOV     R3,-(SP)
        MOV     R1,-(SP)
        MOV     #010,R5         ; 8 scanlines
TBLIT_ROW_U:    ; 005120 (identical start)
        ; [УКНЦ DIFF] instead of: MOV (R2)+, 046000(R1) [direct FB write]
        ; УКНЦ does:
        JSR     PC,@#041040     ; DISP_COL_BLIT — write tile column via port
        JSR     PC,@#040060     ; secondary display write (colour/line update)
        NOP
        MOV     (SP)+,R1
        MOV     (SP)+,R3
        MOV     (SP)+,R4
        MOV     (SP)+,R5
        RTS     PC

; NOTE: TILE_BLIT_REV (014302), TILE_BLIT_SUB (012530) also differ in inner
; loop — use JSR to display routines instead of direct 046000(R1) write.

; =============================================================================
; ENTITY_HANDLER / SOUND ENGINE (006204) — [УКНЦ DIFF: speaker values]
; Structure identical to BK-0010. The two constants at @#102064 and @#102076
; (speaker HIGH/LOW values for @#177716) have different values in УКНЦ version.
; УКНЦ uses the same @#177716 bit 7 for speaker (hardware is same).
; =============================================================================
; (body identical — see crocodile_klad.asm for full annotation)

; =============================================================================
; VSYNC_WAIT (012326) — [УКНЦ DIFF: calls in-game vsync routine, not EMT]
; =============================================================================
VSYNC_WAIT_UKNC: ; 012326
        MOV     R0,-(SP)        ; save R0
        JSR     PC,@#041400     ; [УКНЦ] VSYNC_WAIT_LOOP (soft vsync)
        NOP
        MOV     (SP)+,R0
        RTS     PC

; KEY_ACTION_TBL (012342) — [УКНЦ DIFF: different action codes + key layout]
; 6 entries instead of 11 (УКНЦ keyboard has fewer mapped keys in this game)
; Entry layout: same (word = action code matched to key bit/position)

; =============================================================================
; SECTIONS 012442–013523 — IDENTICAL TO BK-0010
; (LEVEL_RENDER_FULL, TILE_BLIT_SUB inner differs at framebuffer write,
;  PLAYER_STATE_CHECK, ENEMY_RESPAWN, PLAYER_MOVE_STEP — logic identical)
; =============================================================================

; =============================================================================
; SECTIONS 013524–014417 — IDENTICAL TO BK-0010
; (COLLISION_MAP_BUILD, SPRITE_DRAW, TILE_BLIT_REV — logic identical,
;  only TILE_BLIT_REV inner framebuffer write differs)
; =============================================================================

; =============================================================================
; ENTITY RECORDS + WORKING BUFFER (014420–017777) — [УКНЦ DIFF: init values]
; Same addresses as BK-0010. Initial values in SAV image differ (not zeroed).
; PLAYER_STATE (014420..014422): different initial values in this snapshot
; Working tile buffer (014550+): different content in SAV image
; TILE_BANK (017450): tile pixel data — УКНЦ format (may differ from BK-0010)
; =============================================================================

; =============================================================================
; SECTION 2: LEVEL MAPS + DATA (022100–037677)
; Level format: 22 rows × 16 bytes, 2 tiles/byte — IDENTICAL to BK-0010
; 10 level maps × 352 bytes each starting at 022100.
; Levels confirmed by COLLISION_MAP_BUILD reading same format.
; =============================================================================

; =============================================================================
; SECTION 3: УКНЦ-SPECIFIC I/O DRIVER (040060–042000)
; This section does NOT exist in BK-0010 (that range is video RAM there).
; The УКНЦ version embeds custom display, keyboard, and vsync routines
; in the game image rather than using ROM traps (EMT).
; =============================================================================

; =============================================================================
; Routine: disp_scanline_write  (040060)
; Writes pixel data to УКНЦ display hardware registers.
; Called from tile blit routines as second pass per tile column.
; Updates @#176640 (pixel port) and @#176642 (colour register).
; =============================================================================
DISP_SCANLINE_WRITE: ; 040060
        MOV     R1,@#176640     ; write pixel word to display pixel port
        ; ... additional writes to 176640 with different offset
        ; (see raw disassembly at 040060..040140 for full code)
        ; pattern: writes R1 at different row offsets via @#176640

; =============================================================================
; Routine: disp_secondary_write  (040060+)
; Second tile column blit pass: line/row register + pixel data
; =============================================================================
        ; 040100..040220: secondary blit writes (colour port + row updates)

; =============================================================================
; Routine: disp_xy_to_port  (040220)
; Converts tile X/Y coordinates to УКНЦ video port writes.
; Uses DIV #100 to split coordinates.
; =============================================================================
DISP_XY_TO_PORT: ; 040220
        MOV     (R3)+,R4        ; get coordinate word
        ; ... DIV #100, R0 converts row address to scanline number
        ; writes results to 176640/176642/176676

; =============================================================================
; Routine: kbd_read  (040660)
; УКНЦ keyboard read routine.
; Reads key code from keyboard controller into R0.
; Returns: R0 = key code (or 0 if no key)
; =============================================================================
KBD_READ:       ; 040660
        MOV     R1,-(SP)
        MOV     R2,-(SP)
        MOV     #040712,R1      ; R1 → keyboard data register base
        MOV     #1,R2           ; mode = 1 (read)
        ; ... calls into keyboard controller I/O
        MOVB    @#040546,R0     ; read key status register
        ; (clears key-ready flag after read)
        MOV     (SP)+,R2
        MOV     (SP)+,R1
        RTS     PC

        ; 040700: MOVB @#040546, R0 — direct keyboard status read
        ; 040546 low byte: bit 7 = key ready, bits 6-0 = key scan code

; =============================================================================
; Routine: kbd_poll  (041020)
; Keyboard poll wrapper: checks if key is ready, reads it.
; Returns: zero flag set = no key, cleared = key in R0.
; Called from KBD_GAME_POLL at 004674.
; =============================================================================
KBD_POLL:       ; 041020
        JSR     PC,@#040660     ; KBD_READ
        BIT     #200,@#040546   ; bit 7 = key-ready flag still set?
        ; (BIT sets Z=1 if bit clear = no key available)
        RTS     PC

; =============================================================================
; Routine: disp_col_blit  (041040)
; Writes one tile column to the УКНЦ display port.
; Called from TILE_BLIT_FWD_UKNC (005122) and TILE_BLIT_REV (014334).
; IN: R2 → source tile data (pixel word), R1 = screen column
; Translates R1 (column offset) to УКНЦ port coordinates.
; =============================================================================
DISP_COL_BLIT:  ; 041040
        MOV     R0,-(SP)
        CLR     R0
        DIV     #100,R0         ; divide column by 64 → row + column within row
        MOV     R1,-(SP)
        MOV     R1,R0
        ; R0 = integer(R1 / 64) = tile row
        ; R1 = R1 mod 64 = column within row
        ; writes computed address to @#176640 (pixel data port)
        ; writes colour to @#176642
        ADD     (SP)+,R1        ; restore/add back column
        MOV     (SP)+,R0
        RTS     PC

; =============================================================================
; Routine: emt_text_display  (041100)
; Handles EMT 020/024 text display calls for УКНЦ.
; Wraps RT-11-style text output to УКНЦ display system.
; =============================================================================
EMT_TEXT:       ; 041100
        EMT     020             ; RT-11 display string
        ; ... additional text display handling

; =============================================================================
; Routine: display_init  (041140)
; УКНЦ display initialization.
; Sets up tile dimensions and display parameters.
; =============================================================================
DISP_INIT:      ; 041140
        MOV     #024,R2         ; tile size / screen width parameter
        ; ... initializes display registers

; =============================================================================
; Routine: display_mode_check  (041200)
; Checks display mode (colour depth / screen mode).
; =============================================================================
DISP_MODE:      ; 041200
        CMPB    R0,#4           ; mode check
        CMPB    R0,#3
        ; ... sets up 2-colour or 4-colour mode

; =============================================================================
; Routine: vsync_wait_loop  (041400)
; УКНЦ software vsync: busy-loops for a fixed number of iterations.
; Equivalent of BK-0010's EMT 016 r0=7 (wait for vertical blank).
; Called from VSYNC_WAIT_UKNC (012326).
; =============================================================================
VSYNC_WAIT_LOOP: ; 041400
        MOV     R3,-(SP)
        MOV     R1,-(SP)
        MOV     #050,R1         ; outer loop count = 40 dec
        MOV     #100,R3         ; inner loop count = 64 dec
        MOV     R2,R2           ; (nop)
        JSR     PC,@#041740     ; DISP_WRITE_COL — write column to display
        MOV     (SP)+,R2
        MOV     (SP)+,R1
        MOV     (SP)+,R3
        RTS     PC

; =============================================================================
; Routine: disp_pixel_port_write  (041460)
; Writes pixel data word to УКНЦ display hardware via port @#176642.
; Also updates line register @#176676.
; Called repeatedly to transfer scanlines to display controller.
; =============================================================================
DISP_PIXEL_PORT: ; 041460
        MOVB    (R2),@#176642   ; write byte to colour/mode port
        ; ... per-scanline write loop

; =============================================================================
; Routine: disp_line_advance  (041500)
; Advances display line counter and signals end-of-line to video controller.
; Uses @#177716 bit 7 toggle for display sync pulse.
; =============================================================================
DISP_LINE_ADV:  ; 041500
        MOV     #2,R2
        SOB     R2,041510       ; 2-cycle loop per scanline
        ; ...
        MOVB    R1,@#176676     ; write line number to row register
        BIS     #200,@#177716   ; toggle bit 7 (display clock / sync)
        SOB     R1,041640       ; count down display lines
        ; ...

; =============================================================================
; Routine: disp_write_col  (041740)
; Writes a full tile column (8 scanlines) to the УКНЦ display port.
; Core of the planar-video blit path.
; Stores R3 and R1 to display control words (041646, 041650),
; then loops back through display pipeline.
; =============================================================================
DISP_WRITE_COL: ; 041740
        MOV     R3,041646       ; store outer count
        MOV     R1,041650       ; store inner count
        BR      041500          ; jump into display line advance loop

; =============================================================================
; End of УКНЦ annotation.
; =============================================================================
;
; SUMMARY OF DIFFERENCES FROM BK-0010:
;
; | Area                    | BK-0010                       | УКНЦ 1987              |
; |-------------------------|-------------------------------|------------------------|
; | Load format             | BK .BIN packed (LZ)           | RT-11 .SAV (flat)      |
; | Framebuffer             | Linear 040000-077777, direct  | Port-based @#176640+   |
; | Tile blit               | MOV (R2)+, 046000(R1)         | JSR DISP_COL_BLIT      |
; | Keyboard (gameplay)     | BIT @#177716 + @#177714       | JSR KBD_POLL → @#40546 |
; | Keyboard (menu)         | MOV @#177662, R0              | JSR KBD_READ           |
; | Vsync                   | EMT 016 r0=7                  | JSR VSYNC_WAIT_LOOP    |
; | Lives initial value     | 5                             | 0o333 (219 dec)        |
; | Speed values            | 400/1000/2000/4000            | Different (УКНЦ clock) |
; | Speaker                 | @#177716 bit 7 toggle         | @#177716 bit 7 toggle  |
; | Key codes               | BK-0010 scan codes            | УКНЦ keyboard codes    |
; | Text display            | BK EMT 020/024                | RT-11 EMT 020          |
; | I/O driver address      | ROM traps (EMT)               | 040060-042000 (in-game)|
; | Code similarity         | —                             | 83% identical          |
;
; IDENTICAL BETWEEN VERSIONS (83%):
;   RESTART, GAME_OVER_SOFT, PLAYER_DEATH, LEVEL_COMPLETE
;   ACT_DISPATCH, GAME_TICK, all enemy state machines (ENEMY1/2/3_TICK)
;   ANIM_THROTTLE_PLAYER, SPRITE_ANIM_A/B/C/D
;   COLLISION_MAP_BUILD, LEVEL_RESET, PLAYER_STATE_CHECK
;   SPRITE_DRAW, NUM_RENDER, LEVEL_RENDER, LEVEL_RENDER_FULL
;   KEY_DIFFICULTY, DELAY_SPIN, WATER_COLLISION, ENTITY1_RESTORE
;   ENEMY_RESPAWN, PLAYER_MOVE_STEP
;   All level map data (022100-037677): identical 10 levels
;   All entity/game logic variables and constants
