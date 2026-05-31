#!/usr/bin/env python3
"""Headless УКНЦ КЛАД logic tracer — TOTAL programmatic control, no GUI/clicks/keys.

Reuses the PDP-11 CPU core in archive/bk0010/tools/bk_emu.py (KM1801VM2 is identical
on BK-0010 and УКНЦ). Loads the real target binary KLAD_1987_Baranov.SAV into RAM at
001000 and lets us:
  * call any routine and run until it returns (RTS to a sentinel) or HALTs,
  * read/write any memory cell or register,
  * dump regions of the 64K space before/after an action.

I/O-page reads (>=0177600) return 0; the УКНЦ video/keyboard ports live BELOW that
(0176640, 040546) so they behave as plain RAM here — harmless for LOGIC tracing
(we only care about game state, not pixels). For pixels, the Qt emulator stays the
visual ground truth; for mechanics, THIS is the tool.

Usage:  python3 tools/uknc_trace.py            # self-test (GAME_INIT → lives)
        from tools.uknc_trace import Machine    # as a library
"""
import sys, os
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "archive", "bk0010", "tools"))
from bk_emu import CPU, Trap   # noqa: E402

SAV = os.path.join(HERE, "..", "assets", "uknc", "KLAD_1987_Baranov.SAV")
LOAD = 0o1000          # program load address (after 512-byte RT-11 header)
SENTINEL = 0o000004    # an address the program never executes; we detect PC==here

class Machine:
    def __init__(self, sav=SAV):
        body = open(sav, "rb").read()[512:]    # strip RT-11 .SAV header
        self.cpu = CPU()
        self.cpu.mem[LOAD:LOAD+len(body)] = body
        self.cpu.r[6] = 0o160000               # stack pointer
        self.size = len(body)

    # --- memory / register access -------------------------------------------------
    def rb(self, a):   return self.cpu.rdb(a)
    def rw(self, a):   return self.cpu.rdw(a)
    def wb(self, a, v): self.cpu.wrb(a, v)
    def ww(self, a, v): self.cpu.wrw(a, v)
    def reg(self, i):  return self.cpu.r[i]
    def setreg(self, i, v): self.cpu.r[i] = v & 0xffff

    def dump(self, a, n):
        return bytes(self.cpu.mem[a:a+n])

    # --- execution ----------------------------------------------------------------
    def call(self, addr, maxins=5_000_000, regs=None):
        """Call a subroutine: push SENTINEL as return addr, set PC=addr, run until
        the routine RTSes back to SENTINEL (or HALT / instruction limit)."""
        if regs:
            for i, v in regs.items():
                self.cpu.r[i] = v & 0xffff
        c = self.cpu
        c.r[6] = (c.r[6] - 2) & 0xffff
        c.wrw(c.r[6], SENTINEL)     # fake return address
        c.r[7] = addr
        n = 0
        try:
            while n < maxins:
                if c.r[7] == SENTINEL:
                    return ("returned", n)
                c.step(); n += 1
        except Trap as t:
            return ("trap:%s" % t, n)
        return ("limit", n)

    def run(self, addr, stop_pc=None, maxins=5_000_000):
        """Run from addr until PC hits stop_pc (or HALT / limit). No sentinel push."""
        c = self.cpu; c.r[7] = addr; n = 0
        try:
            while n < maxins:
                if stop_pc is not None and c.r[7] == stop_pc:
                    return ("stop_pc", n)
                c.step(); n += 1
        except Trap as t:
            return ("trap:%s" % t, n)
        return ("limit", n)


def _selftest():
    m = Machine()
    print("loaded %d bytes at %06o" % (m.size, LOAD))
    print("lives @017436 before GAME_INIT: %03o" % m.rw(0o17436))
    status, n = m.call(0o4000)            # GAME_INIT
    print("GAME_INIT -> %s after %d instr" % (status, n))
    lives = m.rw(0o17436)
    print("lives @017436 after GAME_INIT: %03o (%d)" % (lives, lives))
    ok = (lives == 0o333)
    print("SELF-TEST %s — expected 0o333 (219) per CLAUDE.md ASM note"
          % ("PASS ✅" if ok else "FAIL ❌ (got %o)" % lives))
    return ok


if __name__ == "__main__":
    _selftest()
