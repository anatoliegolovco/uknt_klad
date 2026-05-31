#!/usr/bin/env python3
"""Minimal PDP-11 (BK-0010 subset) emulator.

Purpose: run the КЛАД packed binaries far enough for their built-in
decompressor to unpack the real program (with plain tile bitmaps) into RAM,
then dump memory so tiles can be extracted as for the uncompressed KLAD3.

Implements the integer instruction set (all addressing modes, double-operand,
single-operand incl. ROL/ROR/ASL/ASR/SWAB/SXT, branch group, SOB, JSR/RTS,
JMP). I/O-page reads return 0, writes are ignored. Stops on HALT / illegal op /
instruction-count limit / optional stop-PC.
"""
import struct, sys

class Trap(Exception): pass

class CPU:
    def __init__(self):
        self.mem = bytearray(0o200000)
        self.r = [0]*8
        self.N=self.Z=self.V=self.C=0
        self.jmp_log=[]

    def rdw(self,a):
        a&=0o177776
        if a>=0o177600: return 0
        return self.mem[a]|(self.mem[a+1]<<8)
    def wrw(self,a,v):
        a&=0o177776
        if a>=0o177600: return
        v&=0xffff; self.mem[a]=v&0xff; self.mem[a+1]=v>>8
    def rdb(self,a):
        a&=0o177777
        return 0 if a>=0o177600 else self.mem[a]
    def wrb(self,a,v):
        a&=0o177777
        if a<0o177600: self.mem[a]=v&0xff
    def fetch(self):
        w=self.rdw(self.r[7]); self.r[7]=(self.r[7]+2)&0xffff; return w

    def ea(self,mode,reg,byte):
        d = 1 if (byte and reg<6) else 2
        if mode==0: return ('r',reg)
        if mode==1: return ('m',self.r[reg])
        if mode==2:
            a=self.r[reg]; self.r[reg]=(a+(2 if reg>=6 else d))&0xffff; return ('m',a)
        if mode==3:
            a=self.r[reg]; self.r[reg]=(a+2)&0xffff; return ('m',self.rdw(a))
        if mode==4:
            self.r[reg]=(self.r[reg]-(2 if reg>=6 else d))&0xffff; return ('m',self.r[reg])
        if mode==5:
            self.r[reg]=(self.r[reg]-2)&0xffff; return ('m',self.rdw(self.r[reg]))
        if mode==6:
            x=self.fetch(); return ('m',(self.r[reg]+x)&0xffff)
        x=self.fetch(); return ('m',self.rdw((self.r[reg]+x)&0xffff))

    def ld(self,e,byte):
        t,a=e
        if t=='r': return self.r[a]&(0xff if byte else 0xffff)
        return self.rdb(a) if byte else self.rdw(a)
    def st(self,e,v,byte):
        t,a=e
        if t=='r':
            self.r[a]=(self.r[a]&0xff00)|(v&0xff) if byte else v&0xffff
        else:
            (self.wrb if byte else self.wrw)(a,v)

    def nz(self,v,byte):
        m=0x80 if byte else 0x8000; k=0xff if byte else 0xffff
        self.Z=1 if (v&k)==0 else 0; self.N=1 if (v&m) else 0

    def step(self):
        op=self.fetch()
        b=op&0o177400
        def br(c):
            o=op&0xff;  o-=0x100 if o>=0x80 else 0
            if c: self.r[7]=(self.r[7]+o*2)&0xffff
        # --- branch group ---
        BR={0o000400:True}
        if   b==0o000400: return br(True)
        elif b==0o001000: return br(self.Z==0)
        elif b==0o001400: return br(self.Z==1)
        elif b==0o002000: return br(self.N==self.V)
        elif b==0o002400: return br(self.N!=self.V)
        elif b==0o003000: return br(self.Z==0 and self.N==self.V)
        elif b==0o003400: return br(self.Z==1 or self.N!=self.V)
        elif b==0o100000: return br(self.N==0)
        elif b==0o100400: return br(self.N==1)
        elif b==0o101000: return br(self.C==0 and self.Z==0)
        elif b==0o101400: return br(self.C==1 or self.Z==1)
        elif b==0o102000: return br(self.V==0)
        elif b==0o102400: return br(self.V==1)
        elif b==0o103000: return br(self.C==0)
        elif b==0o103400: return br(self.C==1)
        # --- SOB ---
        if (op&0o177000)==0o077000:
            reg=(op>>6)&7; self.r[reg]=(self.r[reg]-1)&0xffff
            if self.r[reg]: self.r[7]=(self.r[7]-(op&0o77)*2)&0xffff
            return
        # --- JSR ---
        if (op&0o177000)==0o004000:
            reg=(op>>6)&7; _,a=self.ea((op>>3)&7,op&7,False)
            self.r[6]=(self.r[6]-2)&0xffff; self.wrw(self.r[6],self.r[reg])
            self.r[reg]=self.r[7]; self.r[7]=a&0xffff; return
        # --- RTS ---
        if (op&0o177770)==0o000200:
            reg=op&7; self.r[7]=self.r[reg]; self.r[reg]=self.rdw(self.r[6]); self.r[6]=(self.r[6]+2)&0xffff; return
        # --- JMP ---
        if (op&0o177700)==0o000100:
            _,a=self.ea((op>>3)&7,op&7,False); self.jmp_log.append(a); self.r[7]=a&0xffff; return
        # --- SWAB ---
        if (op&0o177700)==0o000300:
            e=self.ea((op>>3)&7,op&7,False); v=self.ld(e,False)
            r=((v<<8)|(v>>8))&0xffff; self.st(e,r,False); self.nz(r&0xff,True); self.V=0; self.C=0; return
        # --- misc no-operand ---
        if op==0: raise Trap("HALT")
        if op in (1,2,3,4,5,0o240,0o241,0o242,0o244,0o250,0o257,0o260,0o261,0o262,0o264,0o270,0o277):
            # RTI/RTT/BPT/IOT/RESET and condition-code ops: treat as nops/cc
            if 0o240<=op<=0o277:
                bitset=op&0o17; setf=(op&0o20)!=0
                for fl,mask in (('C',1),('V',2),('Z',4),('N',8)):
                    if bitset&mask: setattr(self,fl,1 if setf else 0)
            return
        # --- single-operand groups (word: 0050xx-0067xx ; byte: +0100000) ---
        key=op&0o007700; bflag=bool(op&0o100000); top=op&0o170000
        if (top==0 or top==0o100000) and (0o005000<=(op&0o007700)<=0o006700) and (op&0o000700)<=0o0700 and (op&0o007000)>= 0:
            pass
        single={0o005000:'CLR',0o005100:'COM',0o005200:'INC',0o005300:'DEC',
                0o005400:'NEG',0o005500:'ADC',0o005600:'SBC',0o005700:'TST',
                0o006000:'ROR',0o006100:'ROL',0o006200:'ASR',0o006300:'ASL',
                0o006700:'SXT'}
        if (top==0 or top==0o100000) and key in single:
            name=single[key]; byte=bflag and name not in ('SXT',)
            e=self.ea((op>>3)&7,op&7,byte); v=self.ld(e,byte)
            mask=0xff if byte else 0xffff; m=0x80 if byte else 0x8000
            if name=='CLR': self.st(e,0,byte); self.Z=1;self.N=0;self.V=0;self.C=0; return
            if name=='TST': self.nz(v,byte); self.V=0;self.C=0; return
            if name=='INC': r=(v+1)&mask; self.V=1 if (v&mask)==(0x7f if byte else 0x7fff) else 0; self.nz(r,byte); self.st(e,r,byte); return
            if name=='DEC': r=(v-1)&mask; self.V=1 if (v&mask)==m else 0; self.nz(r,byte); self.st(e,r,byte); return
            if name=='NEG': r=(-v)&mask; self.nz(r,byte); self.C=0 if r==0 else 1; self.V=1 if r==m else 0; self.st(e,r,byte); return
            if name=='COM': r=(~v)&mask; self.nz(r,byte); self.C=1;self.V=0; self.st(e,r,byte); return
            if name=='ROL': c=1 if (v&m) else 0; r=((v<<1)|self.C)&mask; self.C=c; self.nz(r,byte); self.V=self.N^self.C; self.st(e,r,byte); return
            if name=='ROR': c=v&1; r=((v>>1)|(self.C*m))&mask; self.C=c; self.nz(r,byte); self.V=self.N^self.C; self.st(e,r,byte); return
            if name=='ASL': c=1 if (v&m) else 0; r=(v<<1)&mask; self.C=c; self.nz(r,byte); self.V=self.N^self.C; self.st(e,r,byte); return
            if name=='ASR': c=v&1; r=((v>>1)|(v&m))&mask; self.C=c; self.nz(r,byte); self.V=self.N^self.C; self.st(e,r,byte); return
            if name=='SXT': r=mask if self.N else 0; self.Z=0 if self.N else 1; self.st(e,r,False); return
            if name=='ADC': a=self.C; r=(v+a)&mask; self.C=1 if (v==mask and a) else 0; self.V=1 if ((v&mask)==(0x7f if byte else 0x7fff) and a) else 0; self.nz(r,byte); self.st(e,r,byte); return
            if name=='SBC': a=self.C; r=(v-a)&mask; self.C=1 if (v==0 and a) else 0; self.nz(r,byte); self.st(e,r,byte); return
        # --- double-operand ---
        dop=op>>12
        if dop in (1,2,3,4,5,6,0o11,0o12,0o13,0o14,0o15,0o16):
            byte=dop>=0o10; o=dop&7
            se=self.ea((op>>9)&7,(op>>6)&7,byte); s=self.ld(se,byte)
            mask=0xff if byte else 0xffff; m=0x80 if byte else 0x8000
            if o==1:  # MOV/MOVB
                if byte:
                    de=self.ea((op>>3)&7,op&7,True)
                    if de[0]=='r': self.r[de[1]]=(0xff00|s) if (s&0x80) else (s&0xff)  # sign-extend to word
                    else: self.wrb(de[1],s)
                    self.nz(s,True); self.V=0; return
                de=self.ea((op>>3)&7,op&7,False); self.st(de,s,False); self.nz(s,False); self.V=0; return
            de=self.ea((op>>3)&7,op&7,byte); d=self.ld(de,byte)
            if o==2:  # CMP (src-dst)
                r=(s-d)&mask; self.nz(r,byte); self.C=1 if (s&mask)<(d&mask) else 0
                self.V=1 if (((s^d)&(s^r))&m) else 0; return
            if o==3:  # BIT
                r=d&s; self.nz(r,byte); self.V=0; return
            if o==4:  # BIC
                r=d&(~s&mask); self.st(de,r,byte); self.nz(r,byte); self.V=0; return
            if o==5:  # BIS
                r=d|s; self.st(de,r,byte); self.nz(r,byte); self.V=0; return
            if o==6:  # ADD(word)/SUB(byte-dop=16)
                if byte:
                    r=(d-s)&mask; self.C=1 if (d&mask)<(s&mask) else 0
                    self.V=1 if (((d^s)&(d^r))&m) else 0
                else:
                    r=(d+s)&mask; self.C=1 if (d+s)>0xffff else 0
                    self.V=1 if ((~(d^s))&(d^r)&m) else 0
                self.nz(r,byte); self.st(de,r,byte); return
        raise Trap("illegal op %06o at %06o"%(op,(self.r[7]-2)&0xffff))


def run(path, entry=None, maxins=2_000_000, trace_jmp=False):
    d=open(path,"rb").read(); load,ln=struct.unpack('<HH',d[:4]); body=d[4:4+ln]
    cpu=CPU(); cpu.mem[load:load+len(body)]=body
    cpu.r[7]= entry if entry is not None else load
    cpu.r[6]=0o160000
    n=0
    try:
        while n<maxins:
            cpu.step(); n+=1
    except Trap as t:
        print("stopped: %s after %d instr"%(t,n))
    else:
        print("limit %d reached, PC=%06o"%(n,cpu.r[7]))
    print("first JMP targets:", [oct(x) for x in cpu.jmp_log[:30]])
    return cpu

if __name__=="__main__":
    path=sys.argv[1]; entry=int(sys.argv[2],8) if len(sys.argv)>2 else None
    maxi=int(sys.argv[3]) if len(sys.argv)>3 else 2_000_000
    cpu=run(path,entry,maxi)
    open("/tmp/emu_dump.bin","wb").write(cpu.mem)
    print("dumped 64K -> /tmp/emu_dump.bin")
