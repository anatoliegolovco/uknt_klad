#!/usr/bin/env python3
"""
pdp11dis.py -- a small, self-contained PDP-11 (KM1801VM2 / УКНЦ / BK) disassembler.

Why this exists
---------------
The Ubuntu radare2 package ships without a PDP-11 plugin, and we want full
control over annotations and PC-relative target resolution anyway. The PDP-11
ISA is small and regular, so a from-scratch decoder is both practical and
useful for studying КЛАД. NO disassembled output is "borrowed" from anywhere;
this is a textbook implementation of the published instruction encoding.

Input format
------------
By default the input is a BK/УКНЦ raw memory-image ".BIN":
    word0 = load address (bytes), word1 = length (bytes), then `length` bytes.
Use --raw to treat the whole file as code with --org giving the load address.

All numbers are printed in OCTAL (the native PDP-11 convention).

Usage
-----
    python3 tools/pdp11dis.py KLAD.BIN
    python3 tools/pdp11dis.py --raw --org 01000 dump.bin
    python3 tools/pdp11dis.py KLAD.BIN --json     # machine-readable
"""
import sys, argparse, json

REG = {6: "SP", 7: "PC"}
def rname(r): return REG.get(r, f"R{r}")

# Double-operand: opcode in bits 15-12 (and bit 15 selects byte variant for 11..16)
DOUBLE = {
    0o01: "MOV", 0o02: "CMP", 0o03: "BIT", 0o04: "BIC", 0o05: "BIS", 0o06: "ADD",
    0o11: "MOVB", 0o12: "CMPB", 0o13: "BITB", 0o14: "BICB", 0o15: "BISB", 0o16: "SUB",
}

# Single-operand: full 10-bit opcode (bits 15-6).
SINGLE = {
    0o000300: "SWAB",
    0o005000: "CLR",  0o005100: "COM",  0o005200: "INC",  0o005300: "DEC",
    0o005400: "NEG",  0o005500: "ADC",  0o005600: "SBC",  0o005700: "TST",
    0o006000: "ROR",  0o006100: "ROL",  0o006200: "ASR",  0o006300: "ASL",
    0o006700: "SXT",  0o006500: "MFPI", 0o006600: "MTPI",
    0o105000: "CLRB", 0o105100: "COMB", 0o105200: "INCB", 0o105300: "DECB",
    0o105400: "NEGB", 0o105500: "ADCB", 0o105600: "SBCB", 0o105700: "TSTB",
    0o106000: "RORB", 0o106100: "ROLB", 0o106200: "ASRB", 0o106300: "ASLB",
    0o106700: "MFPS", 0o106500: "MFPD", 0o106600: "MTPD",
    0o000100: "JMP",
}

# Conditional branches: opcode in bits 15-8, signed byte offset in 7-0.
BRANCH = {
    0o000400: "BR",  0o001000: "BNE", 0o001400: "BEQ", 0o002000: "BGE",
    0o002400: "BLT", 0o003000: "BGT", 0o003400: "BLE", 0o100000: "BPL",
    0o100400: "BMI", 0o101000: "BHI", 0o101400: "BLOS",0o102000: "BVC",
    0o102400: "BVS", 0o103000: "BCC", 0o103400: "BCS",
}

# Register + operand (reg in bits 8-6): MUL/DIV/ASH/ASHC use src, XOR uses dst.
REG_OP = {0o070000: "MUL", 0o071000: "DIV", 0o072000: "ASH", 0o073000: "ASHC",
          0o074000: "XOR"}

# No-operand / special
NO_OPERAND = {
    0o000000: "HALT", 0o000001: "WAIT", 0o000002: "RTI", 0o000003: "BPT",
    0o000004: "IOT",  0o000005: "RESET",0o000006: "RTT",  0o000240: "NOP",
}
# Condition-code set/clear group 000240..000277
CCC = {0o000241: "CLC", 0o000242: "CLV", 0o000244: "CLZ", 0o000250: "CLN",
       0o000257: "CCC", 0o000261: "SEC", 0o000262: "SEV", 0o000264: "SEZ",
       0o000270: "SEN", 0o000277: "SCC"}


def signed8(b):
    return b - 256 if b & 0x80 else b


class Decoder:
    def __init__(self, data, org):
        self.data = data        # bytes of code only
        self.org = org          # load address of data[0]
        self.pos = 0            # byte offset into data

    def eof(self):
        return self.pos >= len(self.data)

    def word(self):
        """Fetch one little-endian word, advancing pos. Returns (value, addr)."""
        addr = self.org + self.pos
        if self.pos >= len(self.data):
            self.pos += 2
            return 0, addr
        if self.pos + 1 >= len(self.data):
            v = self.data[self.pos]
            self.pos += 1
            return v, addr
        v = self.data[self.pos] | (self.data[self.pos + 1] << 8)
        self.pos += 2
        return v, addr

    def operand(self, field):
        """Decode a 6-bit mode/reg field, consuming extra words as needed.
        Returns the assembler text. Handles PC-relative / immediate / absolute."""
        mode = (field >> 3) & 7
        reg = field & 7
        if reg == 7 and mode in (2, 3, 6, 7):
            ext, _ = self.word()
            if mode == 2:   # immediate  #n
                return f"#{ext:o}"
            if mode == 3:   # absolute   @#addr
                return f"@#{ext:o}"
            if mode == 6:   # relative   addr (PC of *next* word + ext)
                tgt = (self.org + self.pos + ext) & 0xFFFF
                return f"{tgt:o}"
            if mode == 7:   # relative deferred  @addr
                tgt = (self.org + self.pos + ext) & 0xFFFF
                return f"@{tgt:o}"
        r = rname(reg)
        if mode == 0: return r                  # Rn
        if mode == 1: return f"({r})"           # (Rn)
        if mode == 2: return f"({r})+"          # (Rn)+
        if mode == 3: return f"@({r})+"         # @(Rn)+
        if mode == 4: return f"-({r})"          # -(Rn)
        if mode == 5: return f"@-({r})"         # @-(Rn)
        if mode == 6:                            # X(Rn)
            ext, _ = self.word()
            return f"{ext:o}({r})"
        # mode 7                                 # @X(Rn)
        ext, _ = self.word()
        return f"@{ext:o}({r})"

    def decode_one(self):
        w, addr = self.word()
        op12 = (w >> 12) & 0o17
        op_single = w & 0o177700        # bits 15-6
        op_branch = w & 0o177400        # bits 15-8
        op_reg = w & 0o177000           # bits 15-9

        # No-operand & condition codes
        if w in NO_OPERAND:
            return addr, w, NO_OPERAND[w], ""
        if w in CCC:
            return addr, w, CCC[w], ""
        if (w & 0o177770) == 0o000200:  # RTS Rn
            return addr, w, "RTS", rname(w & 7)
        if (w & 0o177700) == 0o006400:  # MARK nn
            return addr, w, "MARK", f"{w & 0o77:o}"
        if (w & 0o177000) == 0o104000:  # EMT / TRAP
            mn = "EMT" if (w & 0o000400) == 0 else "TRAP"
            return addr, w, mn, f"{w & 0o377:o}"

        # JSR  0004RDD
        if (w & 0o177000) == 0o004000:
            reg = (w >> 6) & 7
            dst = self.operand(w & 0o77)
            return addr, w, "JSR", f"{rname(reg)},{dst}"

        # SOB  0077R NN  (subtract one and branch, backward)
        if (w & 0o177000) == 0o077000:
            reg = (w >> 6) & 7
            off = w & 0o77
            tgt = (self.org + self.pos - 2 * off) & 0xFFFF
            return addr, w, "SOB", f"{rname(reg)},{tgt:o}"

        # Register+operand group (MUL/DIV/ASH/ASHC/XOR)
        if op_reg in REG_OP:
            reg = (w >> 6) & 7
            ea = self.operand(w & 0o77)
            mn = REG_OP[op_reg]
            if mn == "XOR":
                return addr, w, mn, f"{rname(reg)},{ea}"
            return addr, w, mn, f"{ea},{rname(reg)}"

        # Branches
        if op_branch in BRANCH:
            off = signed8(w & 0o377)
            tgt = (self.org + self.pos + 2 * off) & 0xFFFF
            return addr, w, BRANCH[op_branch], f"{tgt:o}"

        # Single-operand
        if op_single in SINGLE:
            ea = self.operand(w & 0o77)
            return addr, w, SINGLE[op_single], ea

        # Double-operand
        if op12 in DOUBLE:
            src = self.operand((w >> 6) & 0o77)
            dst = self.operand(w & 0o77)
            return addr, w, DOUBLE[op12], f"{src},{dst}"

        # Unknown -> emit as a data word
        return addr, w, ".WORD", f"{w:o}"


def load_bin(path, raw, org):
    blob = open(path, "rb").read()
    if raw:
        return blob, org, None
    load = blob[0] | (blob[1] << 8)
    length = blob[2] | (blob[3] << 8)
    return blob[4:4 + length], load, (load, length)


def main():
    ap = argparse.ArgumentParser(description="PDP-11 disassembler")
    ap.add_argument("file")
    ap.add_argument("--raw", action="store_true", help="treat whole file as code")
    ap.add_argument("--org", type=lambda x: int(x, 0), default=0o1000,
                    help="load address for --raw (octal ok via 0o..., default 01000)")
    ap.add_argument("--json", action="store_true", help="emit JSON instead of text")
    args = ap.parse_args()

    code, org, hdr = load_bin(args.file, args.raw, args.org)
    dec = Decoder(code, org)
    rows = []
    while not dec.eof():
        rows.append(dec.decode_one())

    if args.json:
        out = [{"addr": f"{a:06o}", "word": f"{w:06o}", "mn": m, "ops": o}
               for (a, w, m, o) in rows]
        print(json.dumps({"org": f"{org:06o}", "rows": out}, ensure_ascii=False))
        return

    if hdr:
        print(f"; {args.file}")
        print(f"; load=0{hdr[0]:o}  length=0{hdr[1]:o} ({hdr[1]} bytes)")
    print(f"        .ORG    0{org:o}\n")
    for a, w, m, o in rows:
        line = f"{a:06o}: {w:06o}   {m:<6}"
        if o:
            line += " " + o
        print(line)


if __name__ == "__main__":
    main()
