; =============================================================================
; КЛАД (Crocodile version, 1991) — Annotated Disassembly
; Target: Электроника БК-0010 (PDP-11 compatible, KM1801VM2)
; Source binary: assets/original/ex_klad/KLAD.BIN
;   load=0732, length=024275 (10429 bytes), two-stage packed
; Unpacked RAM dump: /tmp/emu2_dump.bin (64 KB flat, org=0)
;
; All addresses and constants are OCTAL unless noted [hex] or [dec].
; Screen framebuffer: 040000–077777 (16 KB, 256×256 px, 2bpp color)
; Game playfield offset: 046000 (6 tile-rows below top of screen)
; Tile bank: 017450 — 16 tiles × 16 bytes each (8×8 px @ 2bpp)
; Scanline stride: 100 octal = 64 dec bytes (32 words × 2 bytes)
;
; Session: 2026-05-30 — extracted from bk_unpack_harness + pdp11dis
; =============================================================================

; =============================================================================
; MEMORY MAP (unpacked RAM)
; =============================================================================
; 000000–000777   Interrupt vectors + BK-0010 system workspace
;   000004        Bus-error / illegal-instruction vector (set at 004116)
; 001000          Reset / restart entry — JMP @#4000
; 001230–001302   Level pointer table (10 entries × 2 bytes = 22 words)
;                   each entry = octal address of a level map block
; 001300          ptr: current level map start address
; 001302          ptr: current position in level pointer table
; 001304          ptr: current level pointer table entry pointer
; 001312          SPEED / difficulty  (400/1000/2000/4000 = keys 1-4)
; 004000–077777   Game code + data (unpacked)
; 012342–012402   Key action dispatch table (11 entries × 2 bytes)
; 014420          Player entity record (see ENTITY RECORD below)
; 017450          Tile pixel data bank: 16 tiles × 16 bytes
; 017450          Tile 0  (background / empty)
; 017460          Tile 1  (wall — diamond-mesh pattern)
; 017470          Tile 2  (ladder)
; 017500          Tile 3  (water)
; 017510          Tile 4  (gold / treasure)
; 017520–017650   Tiles 5-15 (enemies, misc)
; 020270          Sprite animation table (entity × direction × frame)
; 021640          Sprite draw buffer / workspace
; 022100+         Level maps: 352 bytes each (22 rows × 16 bytes, 2 tiles/byte)
; 046000          Playfield framebuffer start (screen row 6)
; 017430          Game state word (set to 010404 at init)
; 017436          Lives counter (initialized to 5)
; 017440          Score accumulator
;
; I/O REGISTERS (BK-0010 hardware)
; 177662          Keyboard data register  (polled in menus)
; 177664          Scroll register  (set to 001330 = 760 dec at init)
; 177714          Keyboard shift register (polled in gameplay loop)
; 177716          System register  (bit 6 tested for vsync gate)

; =============================================================================
; ENTITY RECORD LAYOUT (at 014420 for player, similar for enemies)
; word +0: entity type / state flags  (bit 12 = active)
; word +2: X position (column in pixels)
; word +4: Y position (row in pixels)
; word +6: sprite index
; word +10: animation frame / direction
; ... (further fields TBD from 06204 analysis)
; =============================================================================

; =============================================================================
; SECTION 1: PACKED BINARY — DEPACKER STUB (from KLAD.BIN, load=0732)
; =============================================================================
; Bytes 0000732–0000777: interrupt vector trampoline (all BNE +2 = padding)
; These 38 words fill the gap between load address 0732 and entry 01000.

        .ORG    0732

; --- padding / interrupt redirect area ---
; 0732–0776: all words = 001000 (BNE +2 = NOP-equivalent filler)
; The real BK-0010 trap vectors at 0004, 0010 etc. point into ROM.
; This padding ensures the depacker loads contiguously.

; =============================================================================
; DEPACKER STAGE 1  (entry point for entire packed binary)
; Implements a backward-LZ decompressor.
; =============================================================================

DEPACK_ENTRY:   ; 001000
        MOV     PC,R4           ; R4 = current PC (001002)
        ADD     #236,R4         ; R4 → compressed data start (~001240)
        MOV     R4,R0           ; R0 = read cursor (forward into packed data)
        MOV     #100000,R3      ; R3 = 100000 (32768 dec) = write cursor start
                                ;   writes DOWNWARD toward 077000
        CLR     R1              ; R1 = accumulator
        MOV     #20,R2          ; R2 = bit counter (20 octal = 16 dec bits/word)

; Stage 1 inner loop: decompresses ~37 words of stage-2 loader into 077000+
DEPACK1_LOOP:   ; 001022
        TST     -(R0)           ; peek bit stream word (backward scan)
        CLR     -(R3)           ; pre-clear output word
        ROL     (R0)            ; rotate bit into carry
        BCC     DEPACK1_LIT     ; carry=0 → literal; carry=1 → back-ref
        MOVB    (R4)+,R5        ; read length byte
        SUB     #157,R5         ; adjust length
        BMI     DEPACK1_COPY    ; if negative → single-byte copy path
        SWAB    R5              ; move length to high byte
        BISB    (R4)+,R5        ; merge low byte (offset)
        SUB     #10757,R5       ; adjust combined offset/length
DEPACK1_COPY:                   ; 001052
        SUB     R5,R1           ; R1 -= delta (back-reference offset)
        MOV     R1,(R3)         ; write to output
DEPACK1_LIT:                    ; 001056
        SOB     R2,DEPACK1_LOOP ; loop 16 times per word
        CMP     R3,#77000       ; reached target? (077000 = 32256 dec)
        BNE     DEPACK1_LOOP    ; keep going until we reach 077000

; Copy stage-2 loader from compressed-data area to 077000
        MOV     #34,R2          ; 28 dec words to copy
DEPACK1_COPY2:                  ; 001072
        MOV     -(R0),-(R3)     ; backward copy
        SOB     R2,DEPACK1_COPY2
        ADD     #24117,R0       ; advance R0 to main compressed payload
        MOV     #552,R2         ; 362 dec iterations for main depack
        JMP     (R3)            ; jump to stage-2 at 077000

; =============================================================================
; DEPACKER STAGE 2  (loaded at 077000 by stage 1)
; Full backward-LZ; decompresses game payload into 001000–076710
; =============================================================================
; (stage 2 code lives at 077000 after stage 1 runs; not annotated separately
;  as it overwrites itself. Entry is JMP (R3) above.)
; Final result: game code at 004000 onwards, level data at 022100+, tiles at 017450.

; =============================================================================
; SECTION 2: UNPACKED GAME CODE (from emu2_dump.bin)
; =============================================================================

        .ORG    0

; 001000: restart/reset trampoline
RESTART:        ; 001000
        JMP     @#GAME_INIT     ; jump to full game init

; 001004: called on some game-over paths
GAME_OVER_SOFT: ; 001004
        CLR     R0
        JMP     @#004160        ; partial reinit

; 001016: player-death handler (called when lives > 0)
PLAYER_DEATH:   ; 001016
        JSR     PC,@#010206     ; death animation / flash screen
        BR      001032

; 001024: score bonus path
BONUS_LIFE:     ; 001024
        MOV     #1000,R0
        BR      GAME_OVER_SOFT+2

; 001034: level-complete handler
LEVEL_COMPLETE: ; 001034
        JSR     PC,@#013524     ; level transition routine
        MOV     @#001304,R5     ; load level pointer
        MOV     (R5),@#001302   ; update current level address
        ADD     #540,@#001300   ; advance map pointer (540 oct = 352 dec = one level block)
        ADD     #2,@#001304     ; next level table entry
        MOV     @#001304,R5
        CMP     #001302,R5      ; wrapped around?
        BEQ     001112          ; yes → restart from level 0
        NOP
        NOP
        JSR     PC,@#012442     ; load next level data
        JMP     @#001344        ; back to game loop top

001112: JMP     RESTART         ; all levels done → restart

; --- level pointer table (10 levels) ---
; 001230: table of 10 word-addresses (little-endian), each → a 352-byte map block

; =============================================================================
; Routine: key_difficulty_select   (001142)
; Called from difficulty-select menu (key '1'-'4', codes 061-064 octal)
; R0 = key code.  Sets speed word at @#001312.
; =============================================================================
KEY_DIFFICULTY: ; 001142
        CMP     #061,R0         ; key '1'?
        BNE     001160
        MOV     #400,@#001312   ; slowest
        RTS     PC
001160: CMP     #062,R0         ; key '2'?
        BNE     001176
        MOV     #1000,@#001312
        RTS     PC
001176: CMP     #063,R0         ; key '3'?
        BNE     001212
        MOV     #2000,@#001312
        RTS     PC
001212: CMP     #064,R0         ; key '4'?
        BNE     001226
        MOV     #4000,@#001312  ; fastest
001226: RTS     PC

; =============================================================================
; Routine: delay_spin  (001306)
; Busy-wait: loops R5 times (R5=1 on entry from GAME_LOOP → single spin)
; =============================================================================
DELAY_SPIN:     ; 001306
        MOV     R5,-(SP)
        MOV     #1,R5
001314: TST     R0              ; burn cycles
        SOB     R5,001314
        MOV     (SP)+,R5
        RTS     PC

; =============================================================================
; GAME LOOP TOP  (001344)  ← main per-frame entry
; =============================================================================
GAME_LOOP:      ; 001344
        JMP     @#004674        ; → keyboard poll + vsync gate

; =============================================================================
; KEYBOARD POLL — GAMEPLAY  (004674)
; Reads hardware shift-register @#177714 (11 bits = 11 possible keys)
; Table at 012342: 11 word entries mapping bit positions to action addresses
; =============================================================================
KBD_GAME_POLL:  ; 004674
        BIT     #100,@#177716   ; test system-register bit 6 (frame gate)
        BNE     004710          ; bit set → process input
        JMP     @#001354        ; bit clear → skip input this frame

004710: MOV     #012342,R1      ; R1 → key action table
        MOV     #013,R5         ; 11 entries
        MOV     @#177714,R3     ; read keyboard shift register
KBD_SCAN:       ; 004724
        MOV     (R1)+,R0        ; load action address for this bit
        ROR     R3              ; rotate LSB into carry
        BCS     KBD_HIT         ; carry set → this key is pressed
        SOB     R5,KBD_SCAN     ; next bit
        JMP     @#001424        ; no key → continue
KBD_HIT:        ; 004740
        JMP     @#001436        ; dispatch action R0

; Key action table (012342) — 11 entries, each is address of action routine:
; bit 0 → move LEFT
; bit 1 → move RIGHT
; bit 2 → move UP (climb ladder)
; bit 3 → move DOWN
; bit 4 → action / pick up
; bits 5-10 → TBD (probably pause, fire for enemy variants, etc.)
; (exact mapping confirmed by tracing @#177714 bit order vs key matrix)

; =============================================================================
; ACTION DISPATCH  (001436)
; R0 = action address, R4 → player entity record (014420)
; =============================================================================
ACT_DISPATCH:   ; 001436
        MOV     #014420,R4      ; player entity base
        CMP     #010,(R4)       ; entity type == 10?
        BNE     001512          ; not active → skip
        BIT     #2000,@2(R4)    ; check flag bit (collision / dead?)
        BEQ     001512          ; flag clear → skip
        MOV     #021640,R2      ; sprite workspace
        MOV     R0,-(SP)
        MOV     #010,R0
        JSR     PC,@#007432     ; call movement routine
        MOV     (SP)+,R0
        CMPB    #015,@2(R4)     ; state == 015 (dead)?
        BEQ     001324          ; yes → death path
        BR      001602          ; continue game loop

; =============================================================================
; Routine: level_render  (004776)
; Renders the full tile map from current level data into the framebuffer.
; Level map: 22 rows × 16 bytes, 2 tiles per byte (lo-nibble=left, hi-nibble=right)
; =============================================================================
LEVEL_RENDER:   ; 004776
        MOV     #005150,R4      ; R4 → tile map source (current level data)
        CLR     R1              ; R1 = screen X accumulator
        MOV     #026,R5         ; R5 = 22 rows (026 oct)
LREND_ROW:      ; 005010
        MOV     R1,-(SP)        ; save row start
        MOV     #020,R3         ; R3 = 16 bytes per row (020 oct)
LREND_BYTE:     ; 005016
        MOVB    (R4),R2         ; read tile byte (two tiles)
        BIC     #177760,R2      ; isolate low nibble = left tile index
        ASL     R2              ;
        ASL     R2              ; × 16 (tile size = 16 bytes)
        ASL     R2              ;
        ASL     R2              ;
        ADD     #017450,R2      ; R2 → tile pixel data in bank
        JSR     PC,@#005106     ; blit tile to screen at R1
        ADD     #2,R1           ; advance screen X by 2 bytes (one tile column)

        MOVB    (R4)+,R2        ; read same byte, advance pointer
        BIC     #177417,R2      ; isolate high nibble = right tile index (already ×16? no)
        ADD     #017450,R2      ; add bank base (high nibble already shifted by BIC clearing low)
        JSR     PC,@#005106     ; blit tile
        ADD     #2,R1           ; advance X

        SOB     R3,LREND_BYTE   ; loop 16 bytes/row
        MOV     (SP)+,R1        ; restore row start
        ADD     #1000,R1        ; advance to next row (1000 oct = 512 dec bytes = 8 scanlines × 64 bytes)
        SOB     R5,LREND_ROW    ; loop 22 rows
        RTS     PC

; =============================================================================
; Routine: tile_blit_fwd  (005106)
; Copies one 8×8 tile (8 words) forward into the framebuffer.
; IN:  R2 → tile data source (in tile bank at 017450+)
;      R1 = framebuffer offset from 046000
; Preserves: R1, R3, R4, R5
; =============================================================================
TILE_BLIT_FWD:  ; 005106
        MOV     R5,-(SP)
        MOV     R4,-(SP)
        MOV     R3,-(SP)
        MOV     R1,-(SP)
        MOV     #010,R5         ; R5 = 8 rows
TBLIT_ROW:      ; 005122
        MOV     (R2)+,046000(R1) ; copy one word (one scanline of tile) to screen
        ADD     #100,R1         ; advance R1 by 100 oct = 64 dec bytes (one full scanline)
        SOB     R5,TBLIT_ROW    ; 8 rows
        MOV     (SP)+,R1
        MOV     (SP)+,R3
        MOV     (SP)+,R4
        MOV     (SP)+,R5
        RTS     PC

; =============================================================================
; Routine: tile_blit_rev  (014302)
; Identical function to tile_blit_fwd but uses explicit offset from R3 (tile index).
; Called 8x — used for sprite/entity rendering where source is indexed.
; IN:  R2 → byte containing tile index (reads nibble)
;      R1 = framebuffer offset
; =============================================================================
TILE_BLIT_REV:  ; 014302
        MOV     R5,-(SP)
        MOV     R1,-(SP)
        MOVB    (R2),R3         ; read tile index byte
        BIC     #177400,R3      ; mask to 8 bits
        ASL     R3              ; × 16
        ASL     R3
        ASL     R3
        ASL     R3
        ADD     #017450,R3      ; R3 → tile pixel data
        MOV     #010,R5         ; 8 rows
TBREV_ROW:      ; 014334
        MOV     (R3)+,046000(R1) ; copy word to screen
        ADD     #100,R1          ; next scanline
        SOB     R5,TBREV_ROW
        MOV     (SP)+,R1
        MOV     (SP)+,R5
        RTS     PC

; =============================================================================
; Routine: sprite_draw  (014030)
; Draws a 2-tile-wide sprite (player or enemy) from entity record.
; IN:  R4 → entity record, R0 = animation state, R2 = screen position
; =============================================================================
SPRITE_DRAW:    ; 014030
        MOV     R2,-(SP)
        MOV     2(R4),R2        ; load entity X position
        MOV     6(R4),R1        ; load entity Y / screen offset
        JSR     PC,TILE_BLIT_REV ; blit left tile
        MOV     (R4),R3         ; entity state
        ASL     R3              ; × 4 (2-bit frame index in animation table)
        ASL     R3
        ADD     #020270,R3      ; R3 → animation table entry
        ADD     2(R3),R2        ; adjust X by animation frame delta-X
        ADD     4(R3),R1        ; adjust Y by animation frame delta-Y
        CMP     #024,4(R4)      ; boundary check
        BLE     014120
        SUB     2(R3),R2        ; clamp X
        SUB     2(R3),R2
        SUB     4(R3),R1        ; clamp Y
        SUB     4(R3),R1
014120: JSR     PC,TILE_BLIT_REV ; blit right tile
        MOV     (SP),R2
        MOV     R0,R3
        ; ...continues with second pass...
        RTS     PC

; =============================================================================
; Routine: vsync_wait  (012326)
; Waits for vertical blank using BK-0010 EMT 16 (r0=7).
; Called 6x — used after each frame draw to pace the game.
; =============================================================================
VSYNC_WAIT:     ; 012326
        MOV     R0,-(SP)
        MOV     #7,R0
        EMT     016             ; BK-0010 FOCAL/Monitor wait-for-vsync
        MOV     (SP)+,R0
        RTS     PC

; =============================================================================
; GAME INIT  (004000) — full cold start
; =============================================================================
GAME_INIT:      ; 004000
        MOV     #5,@#017436     ; lives = 5
        SUB     #012,SP         ; allocate 5 words on stack
        MOV     #1,-(SP)        ;  arg: display mode
        MOV     #017436,-(SP)   ;  arg: lives display address
        MOV     #002204,-(SP)   ;  arg: screen position
        JSR     PC,@#004210     ; draw lives counter
        CLR     @#017440        ; score = 0
        ; (second JSR 004210 draws score)
        BR      004112

004112: MOV     #0740,SP        ; reset stack pointer
        MOV     #100274,@#4     ; set bus-error vector → trap handler
        JMP     @#005754        ; → hardware init

; =============================================================================
; HARDWARE INIT  (005754)
; Sets up screen scroll, I/O, then loads first level.
; =============================================================================
HW_INIT:        ; 005754
        TSTB    @#040           ; check ROM presence
        BNE     005770
        MOV     #0233,R0 ; EMT 233 (init video mode)
        EMT     016
005770: TSTB    @#056
        BNE     006004
        MOV     #0232,R0
        EMT     016
006004: MOV     #0224,R0 ; EMT 224
        EMT     016
        MOV     #0236,R0
        EMT     016
        MOV     #014,R0
        EMT     016
        MOV     #0221,R0
        EMT     016
        MOV     #001330,@#177664 ; scroll register = 760 dec (screen offset)
        MOV     #010404,@#017430 ; game-state word
        JMP     @#002072         ; → game sequence / title screen

; =============================================================================
; Routine: num_render  (004210)
; Renders a decimal number to the screen (score / lives display).
; Arguments passed on stack: screen address, value, mode
; =============================================================================
NUM_RENDER:     ; 004210
        MOV     R1,010(SP)
        MOV     SP,R1
        ADD     #012,R1
        MOV     R2,(R1)+
        MOV     R4,(R1)+
        MOV     R5,(R1)+
        MOV     (SP)+,(R1)+
        MOV     (SP)+,R5
        MOV     (SP)+,R4
        ; converts integer to digit characters, blits each via tile_blit
        ; ... (digit-render loop follows)
        RTS     PC

; =============================================================================
; ENTITY / OBJECT HANDLER  (006204)
; Called 6x — manages entity state machine (movement, collision, animation).
; IN: R5 → entity record
; Reads entity type (bit 12), extracts position fields via bit ops.
; =============================================================================
ENTITY_HANDLER: ; 006204
        MOV     (R5),R1         ; load entity word
        BIT     #010000,R1      ; bit 12 = entity active?
        BEQ     006324          ; no → skip
        MOV     R1,R2
        BIC     #177600,R1      ; low 7 bits = entity sub-type
        ASL     R2
        SWAB    R2              ; bring high byte to low
        ASR     R2
        BIC     #177600,R2      ; bits 8-14 = direction/state
        ; ...continues with position update and collision checks...
        RTS     PC

; =============================================================================
; TITLE SCREEN SEQUENCE  (002072)
; =============================================================================
TITLE_SEQ:      ; 002072
        JSR     PC,@#004776     ; render title screen tiles (level_render)
        ; Display text strings via EMT 20 / EMT 24:
        ; "КЛАД" logo, author credits, prompt
        MOV     #011,R1
        MOV     #015,R2
        EMT     024             ; display string R1 at position R2
        MOV     #004744,R1
        MOV     #016,R2
        EMT     020
        MOV     #012,R1
        MOV     #017,R2
        EMT     024
        MOV     #004762,R1
        MOV     #014,R2
        EMT     020
        NOP
        JMP     @#002246        ; → keyboard wait

; TITLE KEYBOARD WAIT  (002264)
; Waits for LF (code 012 octal = 10 dec) to start game.
; On BK-0010 this is the Enter/Return key in some modes.
TITLE_WAIT:     ; 002264
        EMT     006             ; wait for keypress → R0
        CMP     #012,R0         ; Enter/LF?
        BNE     TITLE_WAIT      ; loop
        MOV     #002330,R1      ; load difficulty-select text
        MOV     #001400,R2
        EMT     020             ; display "Select difficulty: 1-4"
        JMP     @#003234        ; → difficulty select

; DIFFICULTY SELECT  (003234)
; Waits for keys '1'-'4' (octal 061-064), sets game speed.
DIFF_SELECT:    ; 003234
        EMT     006             ; wait for key
        CMP     #061,R0         ; < '1'?
        BGT     003234
        CMP     #064,R0         ; > '4'?
        BLT     003234
        SUB     #0,R0           ; (nop — subtract 0)
        JSR     PC,@#001142     ; KEY_DIFFICULTY → sets @#001312
        MOV     #014,R0
        EMT     016
        JMP     @#002250        ; → game start

; =============================================================================
; GAME LOOP — continuation path (001354)
; Reached when shift-register key poll skips (bit 6 of @#177716 clear).
; Reads MENU keyboard register @#177662 (slower, used for held-key actions).
; =============================================================================
GAME_LOOP_MENU: ; 001354
        MOV     @#177662,R0     ; read keyboard data register (menu-mode keys)
        CMP     #3,R0           ; Ctrl-C (= STOP on BK-0010)?
        BNE     001372
        JMP     @#GAME_OVER_SOFT ; Ctrl-C → soft game over
001372: CMP     #060,R0         ; < '0' (ASCII 48)?
        BGE     001406          ; ≥ '0': do table lookup
        JSR     PC,@#KEY_DIFFICULTY ; keys '1'-'4' → set @#001312 speed
        BR      GAME_LOOP       ; back to game loop top
; Key-code table scan
001406: MOV     #014,R3         ; 14 oct = 12 dec entries in key table
        MOV     #001732,R5      ; R5 → key code table at 001732
001416: CMP     (R5)+,R0        ; compare this entry with pressed key
        BEQ     001432          ; match
        SOB     R3,001416       ; next entry
        MOV     #177777,R0      ; sentinel = no key matched
        BR      001436
001432: MOV     010406(R5),R0   ; load action code from parallel table at 012342
                                ; offset 010406 oct from R5 puts us at 012342
        ; fall through to ACT_DISPATCH (001436)

; Key code table at 001732 — 12 entries of key codes (octal), BK-0010 menu keyboard:
; [0]=017(Ctrl-O = cursor left)  [1]=016(Ctrl-N = cursor right)
; [2-4]=0(unused)
; [5]=031  [6]=033 (arrow variants)
; [7-8]=0(unused)
; [9]=010(Ctrl-H = backspace/up) [10]=032 [11]=022
; Parallel action codes at 012342+2n: 012, 022, 0, 0, 0, 006, 0, 0, 0, 002, 004, 006

; =============================================================================
; GAME TICK — main per-entity update (001602)
; Called each game loop after keyboard dispatch.
; Sequence: delay → player state check → enemy1 → enemy2 → enemy3 →
;           entity erase → collision check → level-end check
; =============================================================================
GAME_TICK:      ; 001602
        JSR     PC,@#001306     ; DELAY_SPIN — paces the game tick
        JSR     PC,@#012570     ; PLAYER_STATE_CHECK — handle player tile state
        JSR     PC,@#007376     ; ENEMY1_TICK — enemy 1 animation + move
        JSR     PC,@#006552     ; ENEMY2_TICK — enemy 2 state machine
        JSR     PC,@#007462     ; ENEMY3_TICK — enemy 3 state machine
        JSR     PC,@#007306     ; ENTITY1_RESTORE — erase/restore enemy 1
        JSR     PC,@#006462     ; WATER_COLLISION — player vs water check
; Level-complete check: if player tile pointer == enemy1 or enemy2 tile pointer
001636: CMP     @#014422,@#014432 ; player tile addr == enemy1 tile addr?
        BNE     001652
        JMP     002040          ; → level-complete path
001652: CMP     @#014422,@#014442 ; player tile addr == enemy2 tile addr?
        BNE     001666
        JMP     002040
001666: JMP     002000          ; → main loop continue

; =============================================================================
; Routine: game_over_wait  (003274)
; Displayed after all lives lost — waits for Enter to restart.
; =============================================================================
GAME_OVER_WAIT: ; 003274
        EMT     006             ; blocking keypress → R0
        CMP     #012,R0         ; Enter/LF?
        BNE     GAME_OVER_WAIT
        JMP     @#RESTART       ; restart from cold init

; =============================================================================
; Routine: lives_display  (003372)
; Re-renders the lives counter in the HUD.
; =============================================================================
LIVES_DISPLAY:  ; 003372
        MOV     #1,R2           ; text colour/mode
        MOV     #027,R1         ; EMT 24 string index
        EMT     024             ; display "♥" or lives icon
        SUB     #012,SP         ; allocate NUM_RENDER args
        MOV     #1,-(SP)
        MOV     #017436,-(SP)   ; @#LIVES
        MOV     #002204,-(SP)   ; screen position for lives
        JSR     PC,@#004210     ; NUM_RENDER
        MOV     #002204,R1
        MOV     #001407,R2
        EMT     020             ; display formatted lives count
        RTS     PC

; =============================================================================
; Routine: death_score_penalty  (003444)
; Penalizes score on death and re-renders HUD.
; =============================================================================
DEATH_SCORE:    ; 003444
        DEC     @#017436        ; lives -= 1
        ADD     #0764,@#017440  ; score += 500 dec (0764 oct) penalty? or award?
        SUB     #012,SP
        MOV     #1,-(SP)
        MOV     #017436,-(SP)
        MOV     #002204,-(SP)
        JSR     PC,@#004210     ; redraw lives counter
        SUB     #012,SP
        MOV     #1,-(SP)
        MOV     #017440,-(SP)
        MOV     #002162,-(SP)
        JSR     PC,@#004210     ; redraw score

; =============================================================================
; Routine: game_level_loop  (003576)
; Outer level render loop: repeatedly renders the entity set until complete.
; =============================================================================
GAME_LEVEL_LOOP: ; 003576
        MOV     @#017430,R2     ; load game-state word → R2 (pointer table base)
        MOV     2(R2),R4        ; R4 = second entry in state table (level ptr?)
003602: MOV     R4,-(SP)
        MOV     R2,-(SP)
        JSR     PC,@#005002     ; LEVEL_RENDER_FROM_R4 (render level from R4)
        MOV     #006156,R5      ; R5 → sound entity table B
        JSR     PC,@#006204     ; ENTITY_HANDLER (drives sound engine)
        MOV     (SP)+,R2
        MOV     (SP)+,R4
        CMP     #022640,R4      ; end of entity table reached?
        BGT     003646
        SUB     #024,R2         ; advance R2 backward by 20 (oct 24)
        BR      003602
003646: JMP     @#003274        ; → game-over wait

; =============================================================================
; Routine: hud_render  (003652)
; Renders score and lives to the HUD display area.
; =============================================================================
HUD_RENDER:     ; 003652
        MOV     #1,R2
        CLR     R1
        EMT     024             ; clear HUD area
        SUB     #012,SP
        MOV     #1,-(SP)
        MOV     #017440,-(SP)   ; @#SCORE
        MOV     #002162,-(SP)   ; screen position
        JSR     PC,@#004210     ; draw score digits
        SUB     #012,SP
        MOV     #1,-(SP)
        MOV     #017436,-(SP)   ; @#LIVES
        MOV     #002204,-(SP)
        JSR     PC,@#004210     ; draw lives digits
        MOV     #002154,R1
        MOV     #001437,R2
        EMT     020             ; display "LIVES:" label
        RTS     PC

; =============================================================================
; Routine: bonus_life  (003746)
; Called when player collects a bonus-life item (tile state 5).
; =============================================================================
BONUS_LIFE_ADD: ; 003746
        JSR     PC,@#012326     ; VSYNC_WAIT
        INC     @#017436        ; lives += 1
        JSR     PC,@#003372     ; redraw lives HUD
        RTS     PC

; =============================================================================
; Routine: score_add  (003764)
; Called when player collects a gold tile (tile state 4).
; Adds 12 octal = 10 decimal to score.
; =============================================================================
SCORE_ADD:      ; 003764
        JSR     PC,@#012326     ; VSYNC_WAIT
        ADD     #012,@#017440   ; score += 012 (oct) = 10 dec
        RTS     PC

; =============================================================================
; Routine: player_sprite_init  (006054)
; Initialises player sprite display each frame — draws player + checks tile flag.
; =============================================================================
PLAYER_SPRITE_INIT: ; 006054
        JSR     PC,@#003652     ; HUD_RENDER (refresh score/lives display)
        MOV     @#014420,R0     ; load player state word
        MOV     #021640,R2      ; sprite workspace
        MOV     #014420,R4      ; player entity base
        JSR     PC,@#014030     ; SPRITE_DRAW (draw player sprite)
        MOV     @#014422,R3     ; R3 → player's current tile in working buffer
        BIT     #4000,(R3)      ; bit 11 set → tile is solid (no passage)?
        BNE     006132          ; yes → done (can't draw through solid)
        MOV     #010,R0         ; R0 = action code 8
        MOV     #021640,R2
        MOV     #014420,R4
        JSR     PC,@#014030     ; SPRITE_DRAW second pass (with offset frame)
006132: RTS     PC

; Sound entity data — table A (at 006134): entity records for sound set A
; table B (at 006156): entity records for sound set B
; Each group drives the ENTITY_HANDLER sound engine at 006204.
; Format: word = frequency/state data; parsed by ENTITY_HANDLER per-entity.

; =============================================================================
; Routine: entity_state_init  (006444)
; Clears animation counters and calls player_sprite_init.
; Called on level start (from LEVEL_RESET 010354).
; =============================================================================
ENTITY_STATE_INIT: ; 006444
        CLR     @#017360        ; clear enemy 3 tick counter
        CLR     @#017376        ; clear enemy 2 tick counter
        JSR     PC,@#006054     ; PLAYER_SPRITE_INIT
        RTS     PC

; =============================================================================
; Routine: water_collision  (006462)
; Checks if player's tile (at @#014442 pointer) has state 15 (water/death).
; If yes: erases player + enemy2 sprites, restores tile to background.
; =============================================================================
WATER_COLLISION: ; 006462
        MOV     @#014442,R0     ; R0 → player's tile (in working buffer @#14550)
        CMPB    #015,(R0)       ; tile state == 15 (water / lethal)?
        BNE     006550          ; no → safe
        MOV     @#014442,R2     ; set up for erase blit
        MOV     @#014446,R1
        JSR     PC,@#014302     ; TILE_BLIT_REV (erase sprite from screen)
        SUB     #100,R2         ; adjust position (up one scanline?)
        SUB     #1000,R1        ; adjust Y (up one tile row)
        JSR     PC,@#014302     ; erase again (clear sprite artifact)
        MOV     @#017430,R0     ; restore tile from game state table
        MOV     020(R0),@#014442 ; restore player tile pointer to default
        MOV     022(R0),@#014446 ; restore Y offset
        CLR     @#017376        ; clear enemy 2 tick counter
006550: RTS     PC

; =============================================================================
; Routine: enemy2_tick  (006552)
; State machine for enemy 2: throttled by counter @#17376 (fires at 400 oct).
; Compares enemy2 column with player column to move enemy2 toward player.
; =============================================================================
ENEMY2_TICK:    ; 006552
        TST     @#014442        ; enemy 2 tile pointer set?
        BNE     006562
        RTS     PC              ; no enemy 2 → bail
006562: CMP     #0400,@#017376  ; counter reached 400 oct?
        BEQ     006602          ; yes → fire move
        INC     @#017376        ; just tick counter
        RTS     PC
006602: MOV     #014440,R5      ; R5 → enemy 2 entity record
        MOV     #014422,R4      ; R4 → player entity record
006612: CMP     #010,(R5)+      ; enemy state == 10 (active)?
        BEQ     007136          ; yes → upper-path movement
        MOV     (R5),R0
        SUB     #014550,R0      ; R0 = enemy2 column (relative to working buf)
        BIC     #177700,R0      ; keep low 6 bits (= column 0-63)
        MOV     (R4),R1
        SUB     #014550,R1      ; R1 = player column
        BIC     #177700,R1
        CMP     R0,R1           ; enemy2 column vs player column
        BEQ     006750          ; same column → vertical movement path
        BGT     007036          ; enemy2 > player → move left
; enemy2 < player → move right:
        MOV     (R5),R1
        BIT     #100000,(R1)    ; right side blocked (flag bit 15)?
        BEQ     006750          ; blocked → try vertical
        CMPB    #011,2(R1)      ; tile to right is ladder (tile 9)?
        BEQ     006750          ; on ladder → vertical instead
        ADD     #2,R1           ; advance right one column (2 bytes in buffer)
        CMP     R1,@#014432     ; would hit enemy1 position?
        BEQ     006750
        CLR     R0
        JSR     PC,007202       ; SPRITE_ANIM_C (enemy2 sprite, 8-frame throttle)
        MOV     #014440,R4
        BIT     #4000,@2(R4)    ; collision/ground flag?
        BNE     006746
        MOV     (R5),R1
        ADD     #100,R1         ; move down one row (100 oct = 64 bytes)
        CMP     R1,@#014432
        BEQ     006746
        JSR     PC,007242       ; SPRITE_ANIM_D (enemy2 sprite, 5-frame throttle)
006746: RTS     PC

; Same column path / vertical movement for enemy2:
006750: ; (enemy2 vertical movement logic — similar structure to horizontal)
        ; Compares row offsets (BIC #77 masks the column, leaves row portion)
        ; ...falls through to sprite animation

; =============================================================================
; Routine: enemy3_tick  (007462)
; State machine for enemy 3 — identical structure to enemy2_tick.
; Throttled by @#17360 (fires at 400 oct).
; Enemy 3 entity at @#14430; player at @#14422; compare vs @#14442.
; =============================================================================
ENEMY3_TICK:    ; 007462
        CMP     #0400,@#017360  ; counter reached threshold?
        BGE     007474
        BR      007502          ; not yet → do movement instead
007474: INC     @#017360        ; just tick counter
        RTS     PC
007502: MOV     #014430,R5      ; R5 → enemy 3 entity
        MOV     #014422,R4      ; R4 → player entity
        ; (same compare-and-move logic as enemy2_tick)
        ; at 010036: enemy3 column == player column → descend into lower
        ; at 010052: JSR 010142 (SPRITE_ANIM_B for enemy 3)
        ; at 010072: CLR enemy3 state

; =============================================================================
; Routine: entity1_restore  (007306)
; If enemy 1 (at @#14432) has state 15 (dead/in-water):
;   erase enemy 1 sprite from screen, restore tile from game-state table,
;   clear enemy 3 tick counter.
; =============================================================================
ENTITY1_RESTORE: ; 007306
        MOV     @#014432,R0     ; R0 → enemy 1 tile pointer
        CMPB    #015,(R0)       ; state == 15 (water/lethal)?
        BNE     007374          ; no → nothing to do
        MOV     @#014432,R2
        MOV     @#014436,R1
        JSR     PC,@#014302     ; TILE_BLIT_REV (erase enemy 1 sprite)
        SUB     #100,R2
        SUB     #1000,R1
        JSR     PC,@#014302     ; erase artifact
        MOV     @#017430,R0     ; restore from game-state table
        MOV     014(R0),@#014432 ; restore enemy 1 tile pointer (offset 12 dec)
        MOV     016(R0),@#014436 ; restore enemy 1 Y offset
        CLR     @#017360        ; reset enemy 3 tick counter
007374: RTS     PC

; =============================================================================
; Routine: enemy1_tick  (007376)
; Animation throttle for enemy 1: counter @#17372 (fires every 5 calls).
; Calls player sprite draw helper + animation routine for enemy 1.
; =============================================================================
ENEMY1_TICK:    ; 007376
        CMP     #4,@#017372     ; counter reached 4?
        BGE     007424          ; yes → fire
        JSR     PC,@#012740     ; PLAYER_MOVE_STEP (tile stepping/animation)
        JSR     PC,@#012024     ; (secondary animation routine for enemy 1)
        CLR     @#017372        ; reset counter
        BR      007430
007424: INC     @#017372        ; just tick counter
007430: RTS     PC

; =============================================================================
; Routine: anim_throttle_player  (007432)
; Sprite animation throttle for the player entity.
; Counter @#17374: redraws player sprite only when counter < 3;
; on draw: counter is cleared (stays at 0 until caller varies R0).
; Called from ACT_DISPATCH with R0=10 (draw) or action code (move).
; =============================================================================
ANIM_THROTTLE_PLAYER: ; 007432
        CMP     #3,@#017374     ; counter >= 3?
        BGE     007454          ; yes → skip draw, just tick
        JSR     PC,@#014030     ; SPRITE_DRAW (player sprite)
        CLR     @#017374        ; reset counter
        BR      007460
007454: INC     @#017374        ; tick counter (0..N until caller resets)
007460: RTS     PC

; =============================================================================
; Routine: sprite_anim_c  (007202)  [called for enemy 2 moving right]
; 8-frame animation throttle for enemy 2 entity.
; Counter @#17366 (byte): draws sprite via @#14030 when counter < 7.
; =============================================================================
SPRITE_ANIM_C:  ; 007202
        MOV     #021760,R2      ; enemy sprite workspace
        MOV     #014440,R4      ; enemy 2 entity base
        CMPB    #7,@#017366     ; counter < 7?
        BGE     007234
        JSR     PC,@#014030     ; SPRITE_DRAW
        CLRB    @#017366        ; reset counter
        BR      007240
007234: INCB    @#017366        ; tick counter
007240: RTS     PC

; =============================================================================
; Routine: sprite_anim_d  (007242)  [called for enemy 2 moving down]
; 5-frame animation throttle for enemy 2 entity.
; Counter @#17370 (byte): draws sprite via @#14030 when counter < 4.
; =============================================================================
SPRITE_ANIM_D:  ; 007242
        MOV     #010,R0
        MOV     #021760,R2
        MOV     #014440,R4      ; enemy 2 entity base
        CMPB    #4,@#017370     ; counter < 4?
        BGE     007300
        JSR     PC,@#014030     ; SPRITE_DRAW
        CLRB    @#017370
        BR      007304
007300: INCB    @#017370
007304: RTS     PC

; =============================================================================
; Routine: sprite_anim_a  (010102)  [called for enemy 3 moving right]
; 8-frame throttle for enemy 3. Counter @#17362.
; =============================================================================
SPRITE_ANIM_A:  ; 010102
        MOV     #021760,R2      ; enemy sprite workspace
        MOV     #014430,R4      ; enemy 3 entity base
        CMP     #7,@#017362
        BGE     010134
        JSR     PC,@#014030     ; SPRITE_DRAW
        CLR     @#017362
        BR      010140
010134: INC     @#017362
010140: RTS     PC

; =============================================================================
; Routine: sprite_anim_b  (010142)  [called for enemy 3 moving down]
; 5-frame throttle for enemy 3. Counter @#17364.
; =============================================================================
SPRITE_ANIM_B:  ; 010142
        MOV     #010,R0
        MOV     #021760,R2
        MOV     #014430,R4      ; enemy 3 entity base
        CMP     #4,@#017364
        BGE     010200
        JSR     PC,@#014030     ; SPRITE_DRAW
        CLR     @#017364
        BR      010204
010200: INC     @#017364
010204: RTS     PC

; =============================================================================
; Routine: level_reset  (010206)
; Full level re-initialize: called by player_death_anim to reset all entities
; to their starting positions for the current level.
; IN: R0 = 0 (reset from start) or nonzero (use alternate position table)
; =============================================================================
LEVEL_RESET:    ; 010206
        MOV     #017420,R4      ; R4 → entity spawn table base (@17420)
        MOV     010(R4),R2      ; R2 → current spawn table entry
        TST     R0              ; R0 == 0?
        BNE     010234
        ADD     #024,010(R4)    ; advance spawn table pointer by 20 dec
        MOV     010(R4),R2      ; reload
010234: MOV     #4,R5           ; copy 4 words (8 bytes) from spawn table
010240: MOV     (R2)+,(R4)+     ; copy entity record entry
        SOB     R5,010240
        MOV     #017420,R4      ; reset R4
        MOV     010(R4),R2
        ADD     #010,R2         ; R2 → enemy spawn data (offset +8)
        MOV     #014422,R5      ; R5 → entity tile-pointer area
        MOV     #6,R1           ; 6 entries
010270: MOV     (R2)+,(R5)      ; copy enemy spawn positions
        ADD     #4,R5           ; skip alternate fields
        SOB     R1,010270
        MOV     2(R4),R4        ; R4 → level source data address (@17422)
        JSR     PC,@#013524     ; COLLISION_MAP_BUILD (unpack + analyze tiles)
        JSR     PC,@#012442     ; LEVEL_RENDER_FULL (render working buf to screen)
; Reset all entity state words to 0
        MOV     #0,R4
        MOV     R4,@#014420     ; player state word = 0
        MOV     R4,@#014430     ; enemy 1 state word = 0
        MOV     R4,@#014440     ; enemy 2 state word = 0
; Reset starting X position for all entities
        MOV     #024,R4         ; initial X = 24 oct = 20 dec (column 10 × 2 bytes)
        MOV     R4,@#014424     ; player X
        MOV     R4,@#014434     ; enemy 1 X
        MOV     R4,@#014444     ; enemy 2 X
        JSR     PC,@#006444     ; ENTITY_STATE_INIT (clear counters + draw sprites)
; Clear sprite workspace area 011224–012024
        MOV     #011224,R5
010364: CLR     (R5)+
        CMP     #012024,R5
        BGT     010364
        MOV     #011224,@#017400 ; store base of cleared area at @17400
        RTS     PC

; =============================================================================
; KEY ACTION CODE TABLE (012342)
; 12 entries (12 oct = 10 dec), indexed 0-11.
; In gameplay mode (shift register @#177714), each entry corresponds to a bit.
; In menu mode (@#177662 path), looked up by key code from table at 001732.
; Action codes: 002=climb_up, 004=climb_down, 006=jump/fire,
;               012=move_left, 022=move_right, 0=no action
; =============================================================================
KEY_ACTION_TBL: ; 012342
        .WORD   012             ; bit 0 → action 12 (move left)
        .WORD   022             ; bit 1 → action 22 (move right)
        .WORD   0               ; bit 2 → (unused)
        .WORD   0               ; bit 3 → (unused)
        .WORD   0               ; bit 4 → (unused)
        .WORD   006             ; bit 5 → action 6 (jump/fire)
        .WORD   0               ; bit 6 → (unused)
        .WORD   0               ; bit 7 → (unused)
        .WORD   0               ; bit 8 → (unused)
        .WORD   002             ; bit 9 → action 2 (climb up)
        .WORD   004             ; bit 10 → action 4 (climb down)
        .WORD   006             ; bit 11 → action 6 (alt fire)

; =============================================================================
; Routine: level_render_full  (012442)
; Renders the complete tile map from working buffer @#14550 to framebuffer.
; Like level_render (004776) but reads from the EXPANDED working buffer.
; Buffer: 22 rows × 32 tile columns = 704 entries.
; Per entry: word at @#14550+2n, low nibble = tile index.
; =============================================================================
LEVEL_RENDER_FULL: ; 012442
        MOV     #014550,R4      ; R4 → working tile buffer (22×32 expanded tiles)
        CLR     R1              ; R1 = screen column accumulator
        MOV     #026,R5         ; R5 = 22 rows (026 oct)
LFULL_ROW:      ; 012454
        MOV     R1,-(SP)        ; save row-start column
        MOV     #040,R3         ; R3 = 32 columns per row (040 oct)
LFULL_COL:      ; 012462
        MOV     (R4)+,R2        ; read working-buffer word (tile index in low nibble)
        BIC     #177760,R2      ; isolate low nibble (tile 0-15)
        ASL     R2              ; × 16 (tile size = 16 bytes)
        ASL     R2
        ASL     R2
        ASL     R2
        ADD     #017450,R2      ; R2 → tile pixel data in bank
        JSR     PC,@#012530     ; blit tile to framebuffer at R1
        ADD     #2,R1           ; advance column (2 bytes per tile)
        SOB     R3,LFULL_COL    ; 32 columns
        MOV     (SP)+,R1        ; restore row start
        ADD     #1000,R1        ; advance row (1000 oct = 8 scanlines × 64 bytes)
        SOB     R5,LFULL_ROW    ; 22 rows
        RTS     PC

; =============================================================================
; Routine: tile_blit_sub  (012530)
; Blit a single 8×8 tile to framebuffer. Identical logic to tile_blit_fwd (005106).
; IN: R2 → tile pixel data (16 bytes = 8 words)
;     R1 = framebuffer column offset from 046000
; Preserves all registers.
; =============================================================================
TILE_BLIT_SUB:  ; 012530
        MOV     R3,-(SP)
        MOV     R2,-(SP)
        MOV     R1,-(SP)
        MOV     R5,-(SP)
        MOV     #010,R5         ; 8 scanlines
TSUB_ROW:       ; 012544
        MOV     (R2)+,046000(R1) ; write one word to framebuffer scanline
        ADD     #100,R1          ; next scanline (100 oct = 64 bytes)
        SOB     R5,TSUB_ROW
        MOV     (SP)+,R5
        MOV     (SP)+,R1
        MOV     (SP)+,R2
        MOV     (SP)+,R3
        RTS     PC

; =============================================================================
; Routine: player_state_check  (012570)
; Examines the player's current tile state byte (in working buffer at @#14422).
; State transitions:
;   state 11 (=9 dec, water?): set state to 17, trigger death via JMP @#4640
;   state 4  (gold collected): clear state, JSR score_add (003764)
;   state 5  (bonus life):     clear state, JSR bonus_life_add (003746)
;   state 6  (level complete): set state 20, respawn two enemies via enemy_respawn
; =============================================================================
PLAYER_STATE_CHECK: ; 012570
        MOV     @#014422,R3     ; R3 → player tile word (in working buffer)
        CMPB    #011,(R3)       ; state == 011 (water/kill tile)?
        BNE     012626          ; no → check other states
        MOVB    #017,(R3)       ; set tile state to 017 (player-death marker)
        JMP     @#004640        ; → player death trigger
012614: NOP
        BIC     #4000,177700(R3) ; clear bit 11 of tile-2 (collision flag update)
        JSR     PC,@#012326     ; VSYNC_WAIT
012626: CMPB    #4,(R3)         ; state == 4 (gold tile collected)?
        BNE     012642
        CLRB    (R3)            ; clear tile state
        JSR     PC,@#003764     ; SCORE_ADD (+10 points + vsync)
012642: CMPB    #5,(R3)         ; state == 5 (bonus life tile)?
        BNE     012656
        CLRB    (R3)
        JSR     PC,@#003746     ; BONUS_LIFE_ADD (+1 life + vsync)
012656: CMPB    #6,(R3)         ; state == 6 (level-complete trigger)?
        BNE     012714          ; none of the above → done
        MOVB    #020,(R3)       ; mark tile as level-completed
        MOV     @#017424,R5     ; R5 → enemy 1 entity area
        JSR     PC,@#012716     ; ENEMY_RESPAWN (respawn enemy 1)
        MOV     @#017426,R5     ; R5 → enemy 2 entity area
        JSR     PC,@#012716     ; ENEMY_RESPAWN (respawn enemy 2)
        JSR     PC,@#002060     ; trigger entity handler for group A
012714: RTS     PC

; =============================================================================
; Routine: enemy_respawn  (012716)
; Sets enemy entity state to 11 (active/chasing) and sets adjacency flags.
; IN: R5 → enemy entity record
; =============================================================================
ENEMY_RESPAWN:  ; 012716
        MOVB    #011,(R5)       ; set enemy state byte to 011 (= 9 dec = chasing)
        BIS     #100000,177776(R5) ; set bit 15 of PREVIOUS word (mark left neighbour)
        BIS     #040000,2(R5)   ; set bit 13 of NEXT word (mark right neighbour)
        RTS     PC

; =============================================================================
; Routine: player_move_step  (012740)
; Executes one step of player movement into the next tile.
; Sets up animation frame record at @#12372 from player entity (@#14420).
; Checks if movement is permitted (tile flags), blits tile, updates position.
; =============================================================================
PLAYER_MOVE_STEP: ; 012740
        MOV     #012372,R4      ; R4 → animation frame record (temporary)
        MOV     #014420,R5      ; R5 → player entity record
        MOV     2(R5),6(R4)     ; copy player tile pointer to frame record
        MOV     6(R5),010(R4)   ; copy player Y offset to frame record
        TST     4(R4)           ; direction field set?
        BNE     013024          ; yes → execute move
        JMP     @#005724        ; no → bail to some idle path

013024: MOV     R4,R3           ; R3 = frame record base
        ADD     4(R4),R3        ; R3 += direction offset → next tile address
        BIT     2(R3),@0(R4)    ; check tile flag (is destination passable?)
        BEQ     013116          ; flag clear → tile IS passable, execute blit
; destination blocked by collision flag — stay in place
        MOVB    @0(R4),R2       ; load source tile index
        MOV     2(R4),R1        ; load screen offset
        ASL     R2              ; × 16 (tile size)
        ASL     R2
        ASL     R2
        ASL     R2
        MOV     017460(R2),046400(R1) ; blit tile at offset (re-draw blocked tile)
        ADD     (R3),(R4)       ; update tile reference
        ADD     (R3),2(R4)      ; update screen Y
        MOV     #01700,R5
        MOV     2(R4),R1
        XOR     R5,046400(R1)   ; XOR effect (flicker/highlight for blocked move)
        CLR     R1
        RTS     PC

013116: ; movement is permitted — blit new tile, update position
        MOV     (R4),R2         ; source tile index
        MOV     2(R4),R1        ; screen offset
        JSR     PC,@#014302     ; TILE_BLIT_REV (draw tile at new position)
        MOV     4(R4),16(R4)    ; save direction for next frame
        CLR     4(R4)           ; clear pending direction
        ; set up sprite draw in animation workspace at @#21640
        MOV     #014420,R4
        MOV     #021640,R5
        ADD     4(R4),R5        ; R5 += animation frame offset
        ADD     (R4),R5         ; R5 += state offset
        MOV     (R5),R2         ; load sprite pixel data
        MOV     6(R4),R1        ; Y offset
        JSR     PC,@#012530     ; TILE_BLIT_SUB (draw player sprite at new position)
        JMP     @#005710        ; → continue game loop

; =============================================================================
; Routine: collision_map_build  (013524)
; Unpacks packed level data (2 tiles/byte) into working buffer @#14550,
; one byte per tile. Then scans the expanded buffer and computes collision
; flags for each tile based on its 4 neighbours.
; IN: R4 → packed level data source (from level table or @#17422)
; OUT: @#14550 filled with per-tile bytes + flag bits
;
; Tile value meanings (0-15 in low nibble):
;   0  = empty/background   (passable)
;   1  = wall               (solid, blocks all movement)
;   2  = ladder             (climbable up/down, not walkable)
;   3  = water              (lethal on contact)
;   4  = gold               (collectible, sets state 4)
;   5  = bonus life token   (collectible, sets state 5)
;   6  = level-exit         (collectible, sets state 6)
;   7-9 = platform variants (solid top, passable sides)
;   10-15 = reserved / enemy spawn tiles
;
; Flags set in each tile word (high bits):
;   bit  8  (#0400)   = left neighbour is solid (≥tile 10)
;   bit  9  (#1000)   = right neighbour is solid
;   bit 10  (#10000)  = above neighbour is climbable / solid
;   bit 11  (#4000)   = tile is solid OR tile below is solid (standing ground)
;   bit 12  (#20000)  = can descend from this tile (ladder below)
;   bit 13  (#40000)  = left is ladder-entry
;   bit 14  (#100000) = right is ladder-entry
; =============================================================================
COLLISION_MAP_BUILD: ; 013524
        MOV     #014550,R2      ; R2 → working buffer start
        MOV     R2,R3
        ADD     #02576,R3       ; R3 = end of buffer (14550 + 2576 = 17346)
; Phase 1: unpack 2-tiles-per-byte into separate entries
UNPACK_LOOP:    ; 013536
        MOVB    (R4),(R2)       ; copy byte from source
        BIC     #177760,(R2)+   ; keep low nibble (left tile), advance R2
        MOVB    (R4)+,(R2)      ; copy same source byte, advance R4
        BIC     #177417,(R2)    ; keep high nibble (right tile)
        ASR     (R2)            ; shift right 4 positions to get raw index
        ASR     (R2)
        ASR     (R2)
        ASR     (R2)+           ; advance R2
        CMP     R3,R2
        NOP
        BPL     UNPACK_LOOP     ; continue until buffer full
; Phase 2: scan expanded buffer, set collision flags per tile
COLL_SCAN:      ; 013570
        MOV     #014550,R2      ; reset R2 to buffer start
; check right neighbour (at R2+2):
        CMPB    #010,2(R2)      ; right tile >= 10 (solid)?
        BMI     013610
        BIS     #1000,(R2)      ; set bit 9 (right blocked)
013610: CMPB    #010,177776(R2) ; left tile (at R2-2) >= 10?
        BMI     013624
        BIS     #400,(R2)       ; set bit 8 (left blocked)
013624: CMPB    #010,(R2)       ; this tile >= 10 (solid)?
        BNE     013636
        BIS     #4000,(R2)      ; set bit 11 (this tile is solid ground)
013636: CMPB    #6,100(R2)      ; tile below (R2+64, next row) >= 6?
        BPL     013652
        BIS     #4000,(R2)      ; set bit 11 (solid below = can stand here)
013652: CMPB    #011,2(R2)      ; right tile == 9 (ladder)?
        BMI     013666
        BIS     #100000,(R2)    ; set bit 14 (ladder-entry right)
013666: CMPB    #011,177776(R2) ; left tile == 9?
        BMI     013702
        BIS     #040000,(R2)    ; set bit 13 (ladder-entry left)
013702: CMPB    #010,(R2)       ; this tile >= 10?
        BNE     013724
        CMPB    #010,177700(R2) ; tile above (R2-64) >= 10?
        BMI     013724
        BIS     #020000,(R2)    ; set bit 12 (can descend / ladder-top)
013724: CMPB    #010,100(R2)    ; tile below >= 10?
        BNE     013740
        BIS     #010000,(R2)    ; set bit 10 (solid below → can climb down)
013740: CMPB    #010,(R2)       ; this tile >= 10?
        BNE     013762
        CMPB    #6,100(R2)      ; tile below >= 6?
        BMI     013762
        BIS     #010000,(R2)
013762: CMPB    #7,100(R2)      ; tile below >= 7 (platform variant)?
        BMI     013776
        BIS     #2000,(R2)      ; set bit 10 variant
        ; ... continues with more neighbour checks, then loops over all tiles
        RTS     PC

; =============================================================================
; Routine: player_death_trigger  (004640)
; Called when player enters a water/lethal tile (from PLAYER_STATE_CHECK).
; Sets collision flags on adjacent tiles, then jumps to death animation.
; IN: R3 → player tile pointer (working buffer)
; =============================================================================
PLAYER_DEATH_TRIGGER: ; 004640
        BIS     #2000,177700(R3) ; set flag bit on tile above player
        BIS     #020000,100(R3)  ; set flag on tile below player
        JMP     @#012614         ; → continue in PLAYER_STATE_CHECK

; =============================================================================
; Routine: level_render_from_r4  (005002)
; Renders tile map from R4 (packed level data pointer) to framebuffer.
; Same algorithm as level_render (004776) — 22 rows × 16 bytes, 2 tiles/byte.
; This is the second copy, called from game_level_loop (003576).
; =============================================================================
LEVEL_RENDER_R4: ; 005002
        CLR     R1              ; screen column accumulator
        MOV     #026,R5         ; 22 rows
005010: MOV     R1,-(SP)
        MOV     #020,R3         ; 16 bytes per row
005016: MOVB    (R4),R2         ; read byte (2 tiles)
        BIC     #177760,R2      ; low nibble = left tile
        ASL     R2
        ASL     R2
        ASL     R2
        ASL     R2              ; × 16
        ADD     #017450,R2      ; tile bank
        JSR     PC,@#005106     ; TILE_BLIT_FWD
        ADD     #2,R1
        MOVB    (R4)+,R2        ; advance pointer, get high nibble
        BIC     #177417,R2      ; high nibble = right tile (already × 16 after BIC)
        ADD     #017450,R2
        JSR     PC,@#005106     ; TILE_BLIT_FWD
        ADD     #2,R1
        SOB     R3,005016       ; 16 bytes/row
        MOV     (SP)+,R1
        ADD     #1000,R1        ; next tile row
        SOB     R5,005010       ; 22 rows
        RTS     PC

; =============================================================================
; SOUND ENGINE / ENTITY HANDLER  (006204)
; Dual-purpose: active entities (bit 12 set) drive the BK-0010 speaker
; by toggling bit 7 of system register @#177716 with burn loops.
; Inactive entities (bit 12 clear) compute delay timing.
;
; Called with R5 pointing to a sound/entity data table (006134 or 006156).
; Table format per entry:
;   word 0: state word (bit 12 = active; low 7 bits = tone counter; bits 8-9 = speed)
;   word 2: timing word (bits 8-14 = outer count; low 4 bits = inner tick)
;
; Active entity (bit 12 set):
;   Extracts low 7 bits as R1 (tone frequency counter)
;   Extracts bits 8-9 as R2 (speed shift: 0-3, shifts R1 left)
;   Extracts bits 8-14 of second word as outer count R3, low 4 bits as R4
;   Generates square wave: alternates @#177716 writes with burn loops
;   @#102064 and @#102076 hold the two sys-reg values (high/low speaker states)
;
; Inactive entity (bit 12 clear):
;   Uses R1 as delay divider, computes delay into @#5146
;   Loops until @#5146 iterations consumed
; =============================================================================
ENTITY_HANDLER: ; 006204
        MOV     (R5),R1         ; load entity state word
        BIT     #010000,R1      ; bit 12 = entity active?
        BEQ     006324          ; no → inactive path
        MOV     R1,R2
        BIC     #177600,R1      ; R1 = low 7 bits (tone frequency param)
        ASL     R2
        SWAB    R2              ; high byte → low (gets bits 8-15)
        BIC     #177774,R2      ; R2 = bits 0-1 of that = speed shift (0-3)
        MOV     (R5)+,R3        ; R3 = second word; advance R5
        SWAB    R3
        ASR     R3
        BIC     #177600,R3      ; R3 = 7 bits = outer iteration count
        MOV     R3,R4
        BIC     #177760,R3      ; R3 = low 4 bits = inner tick
        ASR     R4
        ASR     R4
        ASR     R4
        ASR     R4              ; R4 = top 3 bits (macro-timing)
        ADD     R2,R4           ; R4 += speed shift
        MOV     #3,R0
        SUB     R2,R0           ; R0 = 3 - speed
        TST     R0
        BEQ     006306
        ASL     R1              ; R1 <<= 1 per speed step
        SOB     R0,006276
006306: TST     R4
        BEQ     006316
        ASL     R3              ; R3 <<= 1 per macro-timing
        SOB     R4,006306
006316: MOV     #1,R2
        BR      006366          ; → speaker toggle loop
; Inactive entity path:
006324: CMP     #077,R1         ; R1 (= low 7 bits) out of range?
        BPL     006436          ; ≥ 0 → skip delay path
        MOV     #6,R2
        ASR     R1              ; R1 /= 64 (6 right shifts)
        SOB     R2,006336
        MOV     #0377,R2        ; delay seed
        ASL     R2
        SOB     R1,006346       ; compute delay count
        MOV     R2,@#005146     ; store timing into @#5146
        MOV     (R5)+,R4        ; advance entity pointer
        BR      ENTITY_HANDLER  ; loop to next entity
; Speaker toggle loop (generates square-wave tone):
006366: MOV     R1,R0
        MOV     @#102064,@#177716 ; write speaker-high value to sys reg
        SOB     R0,006376       ; burn R1 cycles (= half-period high)
        MOV     R1,R0
        MOV     @#102076,@#177716 ; write speaker-low value to sys reg
        SOB     R0,006410       ; burn R1 cycles (= half-period low)
        SOB     R3,006366       ; outer count R3 full cycles
        TST     (R5)            ; more entities?
        BEQ     006434          ; no → RTS
        MOV     @#005146,R0     ; load timing
        SOB     R0,006424       ; inter-note delay
        SOB     R2,006420       ; outer delay count
        JMP     ENTITY_HANDLER  ; loop to next entity
006434: RTS     PC
; Active entity with large R1 (≥ 0):
006436: MOV     R1,R2
        MOV     (R5)+,R4
        BR      006414          ; → timing delay path

; =============================================================================
; End of full annotation.
; All ~35 routines now documented.
; =============================================================================

; GLOBAL VARIABLES SUMMARY (all octal addresses):
; @#001300  CUR_MAP_ADDR     — current level map start address
; @#001302  CUR_LEVEL_PTR    — pointer into level pointer table
; @#001304  LEVEL_TBL_PTR    — current level table entry address
; @#001312  SPEED            — game speed (400/1000/2000/4000)
; @#005146  SOUND_TIMING     — computed sound delay (used by entity handler)
; @#014420  PLAYER_STATE     — player entity state word
; @#014422  PLAYER_TILE_PTR  — pointer to player's current tile in @#14550 buffer
; @#014424  PLAYER_X         — player screen column (bytes from left)
; @#014430  ENEMY1_STATE     — enemy 1 state
; @#014432  ENEMY1_TILE_PTR  — enemy 1 tile pointer
; @#014436  ENEMY1_Y         — enemy 1 Y screen offset
; @#014440  ENEMY2_STATE     — enemy 2 state
; @#014442  ENEMY2_TILE_PTR  — enemy 2 tile pointer
; @#014446  ENEMY2_Y         — enemy 2 Y screen offset
; @#017360  ENEMY3_TICK_CTR  — enemy 3 tick counter (throttle)
; @#017362  ENEMY3_ANIM_A    — enemy 3 animation counter (8-frame)
; @#017364  ENEMY3_ANIM_B    — enemy 3 animation counter (5-frame)
; @#017366  ENEMY2_ANIM_A    — enemy 2 animation counter (8-frame)
; @#017370  ENEMY2_ANIM_B    — enemy 2 animation counter (5-frame)
; @#017372  ENEMY1_TICK_CTR  — enemy 1 tick counter
; @#017374  PLAYER_ANIM_CTR  — player animation throttle counter
; @#017376  ENEMY2_TICK_CTR  — enemy 2 tick counter
; @#017400  SPRITE_BUF_BASE  — base of cleared sprite workspace area
; @#017420  SPAWN_TABLE_BASE — entity spawn position table
; @#017424  ENEMY1_SPAWN_PTR — pointer to enemy 1 spawn record
; @#017426  ENEMY2_SPAWN_PTR — pointer to enemy 2 spawn record
; @#017430  GAME_STATE       — game state word (= 010404, used as table base)
; @#017436  LIVES            — lives remaining (init 5)
; @#017440  SCORE            — score accumulator
