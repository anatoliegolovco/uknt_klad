// uknc_emu.c — headless KM1801VM2 (PDP-11) emulator for tracing КЛАД 1987 logic.
//
// Purpose: run the real target binary assets/uknc/KLAD_1987_Baranov.SAV with TOTAL
// programmatic control — no GUI, no mouse, no keyboard injection. We call any routine,
// read/write any cell or register, and diff memory before/after an action. This is the
// LOGIC ground truth (what the code computes); the Qt УКНЦ emulator stays the VISUAL
// ground truth (pixels). BK-0010 and УКНЦ differ as MACHINES (video, PPU) but share the
// KM1801VM2 instruction set, so for CPU+RAM logic this faithful core is enough.
//
// Instruction set: full PDP-11 integer set + EIS (MUL/DIV/ASH/ASHC) + XOR, all addressing
// modes, byte/word, branch group, SOB, JSR/RTS/JMP/SWAB/MARK, condition codes. I/O page
// (>=0177600) reads 0 / ignores writes; the УКНЦ video/keyboard ports (0176640, 040546)
// sit below that and behave as plain RAM here (harmless for logic).
//
// Build:  cc -std=c23 -O2 -Wall -Wextra tools/uknc_emu.c -o /tmp/uknc_emu
// Use:    /tmp/uknc_emu selftest                 # GAME_INIT must set lives = 0o333
//         /tmp/uknc_emu call <octal_addr> [max]  # call routine, print changed words
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── machine state ─────────────────────────────────────────────────────────────
#define MEMSZ   0200000u          // 64 KiB, octal
#define IOPAGE  0177600u          // >= this → I/O page (reads 0, writes ignored)
#define LOAD    01000u            // program load address (after 512-byte RT-11 header)
#define SENTINEL 04u              // return address we plant so we can detect routine exit

typedef struct {
    uint8_t  mem[MEMSZ];
    uint16_t r[8];
    int N, Z, V, C;
    bool trapped;
    char trapmsg[64];
} CPU;

// ── memory access ─────────────────────────────────────────────────────────────
static uint16_t rdw(CPU *c, uint16_t a){ a&=0177776; if(a>=IOPAGE) return 0;
    return (uint16_t)(c->mem[a] | (c->mem[a+1]<<8)); }
static void wrw(CPU *c, uint16_t a, uint16_t v){ a&=0177776; if(a>=IOPAGE) return;
    c->mem[a]=v&0xff; c->mem[a+1]=v>>8; }
static uint8_t rdb(CPU *c, uint16_t a){ return a>=IOPAGE ? 0 : c->mem[a]; }
static void wrb(CPU *c, uint16_t a, uint8_t v){ if(a<IOPAGE) c->mem[a]=v; }
static uint16_t fetch(CPU *c){ uint16_t w=rdw(c,c->r[7]); c->r[7]+=2; return w; }

static void trap(CPU *c, const char *what, uint16_t op, uint16_t pc){
    c->trapped=true; snprintf(c->trapmsg,sizeof c->trapmsg,"%s %06o @ %06o",what,op,pc); }

// ── operand decode (effective address) ────────────────────────────────────────
// is_reg=true → operand is register `reg`; else operand is memory at `addr`.
typedef struct { bool is_reg; int reg; uint16_t addr; } Opnd;

static Opnd ea(CPU *c, int mode, int reg, bool byte){
    int d = (byte && reg<6) ? 1 : 2;            // autoinc/dec step (reg 6,7 always 2)
    Opnd o = {0};
    switch(mode){
        case 0: o.is_reg=true; o.reg=reg; return o;
        case 1: o.addr=c->r[reg]; return o;
        case 2: o.addr=c->r[reg]; c->r[reg]+=(reg>=6)?2:d; return o;          // (Rn)+
        case 3: o.addr=rdw(c,c->r[reg]); c->r[reg]+=2; return o;              // @(Rn)+
        case 4: c->r[reg]-=(reg>=6)?2:d; o.addr=c->r[reg]; return o;          // -(Rn)
        case 5: c->r[reg]-=2; o.addr=rdw(c,c->r[reg]); return o;              // @-(Rn)
        case 6: { uint16_t x=fetch(c); o.addr=c->r[reg]+x; return o; }        // X(Rn)
        default:{ uint16_t x=fetch(c); o.addr=rdw(c,(uint16_t)(c->r[reg]+x)); return o; } // @X(Rn)
    }
}
static uint16_t ld(CPU *c, Opnd o, bool byte){
    if(o.is_reg) return c->r[o.reg] & (byte?0xff:0xffff);
    return byte ? rdb(c,o.addr) : rdw(c,o.addr);
}
static void st(CPU *c, Opnd o, uint16_t v, bool byte){
    if(o.is_reg){ if(byte) c->r[o.reg]=(c->r[o.reg]&0xff00)|(v&0xff); else c->r[o.reg]=v; }
    else { if(byte) wrb(c,o.addr,v&0xff); else wrw(c,o.addr,v); }
}
static void setnz(CPU *c, uint16_t v, bool byte){
    uint16_t m=byte?0x80:0x8000, k=byte?0xff:0xffff;
    c->Z=((v&k)==0); c->N=((v&m)!=0);
}

// ── one instruction ───────────────────────────────────────────────────────────
static void step(CPU *c){
    uint16_t pc=c->r[7];
    uint16_t op=fetch(c);
    uint16_t b=op&0177400;

    // branch group (offset is signed byte × 2)
    int off=(int8_t)(op&0xff);
    #define BR(cond) do{ if(cond) c->r[7]+=off*2; return; }while(0)
    switch(b){
        case 0000400: BR(true);
        case 0001000: BR(c->Z==0);
        case 0001400: BR(c->Z==1);
        case 0002000: BR(c->N==c->V);
        case 0002400: BR(c->N!=c->V);
        case 0003000: BR(c->Z==0 && c->N==c->V);
        case 0003400: BR(c->Z==1 || c->N!=c->V);
        case 0100000: BR(c->N==0);
        case 0100400: BR(c->N==1);
        case 0101000: BR(c->C==0 && c->Z==0);
        case 0101400: BR(c->C==1 || c->Z==1);
        case 0102000: BR(c->V==0);
        case 0102400: BR(c->V==1);
        case 0103000: BR(c->C==0);
        case 0103400: BR(c->C==1);
    }

    // SOB Rn,offset
    if((op&0177000)==0077000){
        int reg=(op>>6)&7; c->r[reg]--;
        if(c->r[reg]) c->r[7]-=(op&077)*2;
        return;
    }
    // JSR reg,dst
    if((op&0177000)==0004000){
        int reg=(op>>6)&7; Opnd o=ea(c,(op>>3)&7,op&7,false);
        c->r[6]-=2; wrw(c,c->r[6],c->r[reg]);
        c->r[reg]=c->r[7]; c->r[7]=o.addr; return;
    }
    // RTS reg
    if((op&0177770)==0000200){
        int reg=op&7; c->r[7]=c->r[reg]; c->r[reg]=rdw(c,c->r[6]); c->r[6]+=2; return;
    }
    // JMP dst
    if((op&0177700)==0000100){
        Opnd o=ea(c,(op>>3)&7,op&7,false); c->r[7]=o.addr; return;
    }
    // SWAB dst
    if((op&0177700)==0000300){
        Opnd o=ea(c,(op>>3)&7,op&7,false); uint16_t v=ld(c,o,false);
        uint16_t r=(uint16_t)((v<<8)|(v>>8)); st(c,o,r,false);
        setnz(c,r&0xff,true); c->V=0; c->C=0; return;
    }
    // MARK n  (0064nn) — used by some return sequences
    if((op&0177700)==0006400){
        c->r[6]=c->r[7]+(op&077)*2; c->r[7]=c->r[5]; c->r[5]=rdw(c,c->r[6]); c->r[6]+=2; return;
    }

    // no-operand / condition-code ops
    if(op==0){ trap(c,"HALT",op,pc); return; }
    if(op==1||op==2||op==3||op==4||op==5) return;          // WAIT/RTI/BPT/IOT/RESET → nop
    if(op>=0104000 && op<=0104777) return;                 // EMT/TRAP (ФОДОС syscall) → nop
    if(op>=0240 && op<=0277){                               // condition-code set/clear
        int bits=op&017, setf=(op&020)!=0;
        if(bits&1) c->C=setf;
        if(bits&2) c->V=setf;
        if(bits&4) c->Z=setf;
        if(bits&8) c->N=setf;
        return;
    }

    // single-operand group (word 0050xx-0067xx; byte +0100000)
    uint16_t key=op&0007700; bool bflag=(op&0100000)!=0; uint16_t top=op&0170000;
    if(top==0 || top==0100000){
        const char *nm=NULL;
        switch(key){
            case 0005000: nm="CLR"; break; case 0005100: nm="COM"; break;
            case 0005200: nm="INC"; break; case 0005300: nm="DEC"; break;
            case 0005400: nm="NEG"; break; case 0005500: nm="ADC"; break;
            case 0005600: nm="SBC"; break; case 0005700: nm="TST"; break;
            case 0006000: nm="ROR"; break; case 0006100: nm="ROL"; break;
            case 0006200: nm="ASR"; break; case 0006300: nm="ASL"; break;
            case 0006700: nm="SXT"; break;
        }
        if(nm){
            bool byte = bflag && key!=0006700;        // SXT has no byte form
            Opnd o=ea(c,(op>>3)&7,op&7,byte); uint16_t v=ld(c,o,byte);
            uint16_t mask=byte?0xff:0xffff, m=byte?0x80:0x8000;
            if(!strcmp(nm,"CLR")){ st(c,o,0,byte); c->Z=1;c->N=0;c->V=0;c->C=0; return; }
            if(!strcmp(nm,"TST")){ setnz(c,v,byte); c->V=0;c->C=0; return; }
            if(!strcmp(nm,"INC")){ uint16_t r=(v+1)&mask; c->V=((v&mask)==(byte?0x7f:0x7fff)); setnz(c,r,byte); st(c,o,r,byte); return; }
            if(!strcmp(nm,"DEC")){ uint16_t r=(v-1)&mask; c->V=((v&mask)==m); setnz(c,r,byte); st(c,o,r,byte); return; }
            if(!strcmp(nm,"NEG")){ uint16_t r=(-v)&mask; setnz(c,r,byte); c->C=(r!=0); c->V=(r==m); st(c,o,r,byte); return; }
            if(!strcmp(nm,"COM")){ uint16_t r=(~v)&mask; setnz(c,r,byte); c->C=1;c->V=0; st(c,o,r,byte); return; }
            if(!strcmp(nm,"ROL")){ int cc=(v&m)!=0; uint16_t r=((v<<1)|c->C)&mask; c->C=cc; setnz(c,r,byte); c->V=c->N^c->C; st(c,o,r,byte); return; }
            if(!strcmp(nm,"ROR")){ int cc=v&1; uint16_t r=((v>>1)|(c->C?m:0))&mask; c->C=cc; setnz(c,r,byte); c->V=c->N^c->C; st(c,o,r,byte); return; }
            if(!strcmp(nm,"ASL")){ int cc=(v&m)!=0; uint16_t r=(v<<1)&mask; c->C=cc; setnz(c,r,byte); c->V=c->N^c->C; st(c,o,r,byte); return; }
            if(!strcmp(nm,"ASR")){ int cc=v&1; uint16_t r=((v>>1)|(v&m))&mask; c->C=cc; setnz(c,r,byte); c->V=c->N^c->C; st(c,o,r,byte); return; }
            if(!strcmp(nm,"SXT")){ uint16_t r=c->N?mask:0; c->Z=!c->N; st(c,o,r,false); return; }
            if(!strcmp(nm,"ADC")){ int a=c->C; uint16_t r=(v+a)&mask; c->C=(v==mask&&a); c->V=((v&mask)==(byte?0x7f:0x7fff)&&a); setnz(c,r,byte); st(c,o,r,byte); return; }
            if(!strcmp(nm,"SBC")){ int a=c->C; uint16_t r=(v-a)&mask; c->C=(v==0&&a); c->V=((v&mask)==m&&a); setnz(c,r,byte); st(c,o,r,byte); return; }
        }
    }

    // EIS + XOR group (op bits 15-9 = 0070xx..0074xx)
    if((op&0177000)==0070000){ // MUL Rn,src
        int reg=(op>>6)&7; Opnd s=ea(c,(op>>3)&7,op&7,false);
        int32_t a=(int16_t)c->r[reg], bb=(int16_t)ld(c,s,false); int32_t p=a*bb;
        c->r[reg]=(p>>16)&0xffff; c->r[reg|1]=p&0xffff;
        c->N=(p<0); c->Z=(p==0); c->V=0; c->C=(p< -32768 || p>32767); return;
    }
    if((op&0177000)==0071000){ // DIV Rn,src
        int reg=(op>>6)&7; Opnd s=ea(c,(op>>3)&7,op&7,false);
        int32_t dv=(int16_t)ld(c,s,false);
        int32_t num=(int32_t)((c->r[reg]<<16)|c->r[reg|1]);
        if(dv==0){ c->V=1; c->C=1; return; }
        int32_t q=num/dv, rem=num%dv;
        if(q>32767||q< -32768){ c->V=1; return; }
        c->r[reg]=q&0xffff; c->r[reg|1]=rem&0xffff; c->N=(q<0); c->Z=(q==0); c->V=0;c->C=0; return;
    }
    if((op&0177000)==0072000){ // ASH Rn,src (shift R by signed count)
        int reg=(op>>6)&7; Opnd s=ea(c,(op>>3)&7,op&7,false);
        int sh=ld(c,s,false)&077; if(sh&040) sh-=64;
        int32_t v=(int16_t)c->r[reg], r;
        if(sh>=0) r=v<<sh; else r=v>>(-sh);
        c->r[reg]=r&0xffff; setnz(c,c->r[reg],false); c->V=0; return;
    }
    if((op&0177000)==0073000){ // ASHC Rn,src (shift R:R|1 by signed count)
        int reg=(op>>6)&7; Opnd s=ea(c,(op>>3)&7,op&7,false);
        int sh=ld(c,s,false)&077; if(sh&040) sh-=64;
        int64_t v=(int32_t)((c->r[reg]<<16)|c->r[reg|1]), r;
        if(sh>=0) r=v<<sh; else r=v>>(-sh);
        c->r[reg]=(r>>16)&0xffff; c->r[reg|1]=r&0xffff;
        c->N=(r<0); c->Z=(r==0); c->V=0; return;
    }
    if((op&0177000)==0074000){ // XOR Rn,dst
        int reg=(op>>6)&7; Opnd d=ea(c,(op>>3)&7,op&7,false);
        uint16_t r=ld(c,d,false)^c->r[reg]; st(c,d,r,false); setnz(c,r,false); c->V=0; return;
    }

    // double-operand
    int dop=op>>12;
    if((dop>=1&&dop<=6) || (dop>=011&&dop<=016)){
        bool byte=dop>=010; int o=dop&7;
        Opnd se=ea(c,(op>>9)&7,(op>>6)&7,byte); uint16_t s=ld(c,se,byte);
        uint16_t mask=byte?0xff:0xffff, m=byte?0x80:0x8000;
        if(o==1){ // MOV/MOVB
            if(byte){ Opnd de=ea(c,(op>>3)&7,op&7,true);
                if(de.is_reg) c->r[de.reg]=(s&0x80)?(0xff00|s):(s&0xff); // MOVB to reg sign-extends
                else wrb(c,de.addr,s);
                setnz(c,s,true); c->V=0; return; }
            Opnd de=ea(c,(op>>3)&7,op&7,false); st(c,de,s,false); setnz(c,s,false); c->V=0; return;
        }
        Opnd de=ea(c,(op>>3)&7,op&7,byte); uint16_t d=ld(c,de,byte); uint16_t r;
        switch(o){
            case 2: r=(s-d)&mask; setnz(c,r,byte); c->C=((s&mask)<(d&mask)); c->V=(((s^d)&(s^r)&m)!=0); return; // CMP
            case 3: r=d&s; setnz(c,r,byte); c->V=0; return;                                                     // BIT
            case 4: r=d&(~s&mask); st(c,de,r,byte); setnz(c,r,byte); c->V=0; return;                            // BIC
            case 5: r=d|s; st(c,de,r,byte); setnz(c,r,byte); c->V=0; return;                                    // BIS
            case 6: if(byte){ r=(d-s)&mask; c->C=((d&mask)<(s&mask)); c->V=(((d^s)&(d^r)&m)!=0); }              // SUB(byte)
                    else { uint32_t sum=d+s; r=sum&mask; c->C=(sum>0xffff); c->V=((~(d^s)&(d^r)&m)!=0); }       // ADD(word)
                    setnz(c,r,byte); st(c,de,r,byte); return;
        }
    }
    trap(c,"illegal op",op,pc);
}

// ── harness ───────────────────────────────────────────────────────────────────
static CPU C;

static int load_sav(const char *path){
    FILE *f=fopen(path,"rb"); if(!f){ perror(path); return -1; }
    fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,512,SEEK_SET);   // skip RT-11 header
    long body=n-512; if(body<=0||LOAD+body>MEMSZ){ fclose(f); return -1; }
    size_t got=fread(C.mem+LOAD,1,body,f); fclose(f);
    if((long)got!=body) return -1;
    C.r[6]=0160000;
    return (int)body;
}

// Call a subroutine: plant SENTINEL return addr, run until it RTSes back (or HALT/limit).
static long call(uint16_t addr, long maxins){
    C.trapped=false;
    C.r[6]-=2; wrw(&C,C.r[6],SENTINEL);
    C.r[7]=addr;
    long n=0;
    while(n<maxins){
        if(C.r[7]==SENTINEL) return n;
        step(&C); n++;
        if(C.trapped) return -n;       // negative = trapped (still ran |n| instr)
    }
    return n;                          // hit limit
}

int main(int argc, char **argv){
    const char *sav="assets/uknc/KLAD_1987_Baranov.SAV";
    const char *cmd = argc>1 ? argv[1] : "selftest";

    if(!strcmp(cmd,"selftest")){
        if(load_sav(sav)<0) return 1;
        printf("lives @017436 before GAME_INIT: %03o\n", rdw(&C,017436));
        long n=call(04000, 5000000);
        printf("GAME_INIT -> %s after %ld instr%s\n",
               C.trapped?"TRAP":(n>=0?"returned/limit":"?"), n<0?-n:n,
               C.trapped?C.trapmsg:"");
        if(C.trapped) printf("  (trap detail: %s — expected: video-setup EIS, not logic)\n",C.trapmsg);
        uint16_t lives=rdw(&C,017436);
        printf("lives @017436 after GAME_INIT: %03o (%d)\n", lives, lives);
        bool ok=(lives==0333);
        printf("SELF-TEST %s — expected 0o333 (219)\n", ok?"PASS ✅":"FAIL ❌");
        return ok?0:1;
    }

    if(!strcmp(cmd,"call") && argc>=3){
        if(load_sav(sav)<0) return 1;
        uint16_t addr=(uint16_t)strtol(argv[2],NULL,8);
        long maxins = argc>=4 ? strtol(argv[3],NULL,10) : 5000000;
        static uint8_t before[MEMSZ]; memcpy(before,C.mem,MEMSZ);
        long n=call(addr,maxins);
        printf("call %06o -> %s after %ld instr%s\n", addr,
               C.trapped?"TRAP":"done", n<0?-n:n, C.trapped?C.trapmsg:"");
        int changes=0;
        for(uint16_t a=0; a<IOPAGE; a+=2){
            uint16_t o=before[a]|(before[a+1]<<8), nw=rdw(&C,a);
            if(o!=nw){ if(changes<60) printf("  %06o: %06o -> %06o\n",a,o,nw); changes++; }
        }
        printf("%d word(s) changed%s\n", changes, changes>60?" (first 60 shown)":"");
        return 0;
    }

    fprintf(stderr,"usage: %s selftest | call <octal_addr> [maxins]\n", argv[0]);
    return 2;
}
