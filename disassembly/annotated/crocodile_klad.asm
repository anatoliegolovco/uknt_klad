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
; End of annotated section.
; Unannotated regions (to be completed — Objective 2 in progress):
;   007432  — movement routine (called from ACT_DISPATCH)
;   010206  — player death animation
;   012442  — load next level data
;   013524  — level-complete transition
;   014354+ — more sprite draw helpers
; =============================================================================
