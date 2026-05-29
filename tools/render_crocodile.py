#!/usr/bin/env python3
"""Render artifacts from a 64K BK memory dump produced by bk_unpack_harness.

Given the post-unpack RAM dump of a packed КЛАД (Crocodile) binary, emit:
  * title_screen.png  — the BK screen (040000, 256x256 @ 2bpp)
  * sprites.png       — the unpacked tile/sprite region as an 8x8 2bpp sheet

The 4-colour palette is the runtime guess (black / cyan / green / white);
adjust PALETTE to match a specific BK palette-register setting.
"""
import struct, zlib, sys

PALETTE = [(0, 0, 0), (40, 200, 220), (60, 200, 70), (235, 235, 235)]

def png_rgb(path, w, h, rgb):
    raw = bytearray()
    for y in range(h):
        raw.append(0); raw += rgb[y*w*3:(y+1)*w*3]
    def ch(t, d):
        c = t + d
        return struct.pack(">I", len(d)) + c + struct.pack(">I", zlib.crc32(c) & 0xffffffff)
    open(path, "wb").write(b"\x89PNG\r\n\x1a\n" +
        ch(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)) +
        ch(b"IDAT", zlib.compress(bytes(raw), 9)) + ch(b"IEND", b""))

def render_screen(mem, out, base=0o40000, sc=3):
    W = H = 256
    rgb = bytearray(W*sc*H*sc*3)
    for y in range(256):
        for bx in range(64):
            byte = mem[base + y*64 + bx]
            for pp in range(4):
                cr, cg, cb = PALETTE[(byte >> (2*pp)) & 3]
                px = bx*4 + pp
                for dy in range(sc):
                    for dx in range(sc):
                        i = ((y*sc+dy)*W*sc + (px*sc+dx))*3
                        rgb[i:i+3] = bytes((cr, cg, cb))
    png_rgb(out, W*sc, H*sc, bytes(rgb))

def render_sheet(mem, out, start, end, per_row=16, sc=11):
    n = (end - start)//16
    cols = per_row; rws = (n + cols - 1)//cols; sep = 2
    cw = 8*sc + sep; W = cols*cw + sep; H = rws*cw + sep
    rgb = bytearray([25]*W*H*3)
    for t in range(n):
        tb = mem[start + t*16: start + t*16 + 16]
        gx = (t % cols)*cw + sep; gy = (t//cols)*cw + sep
        for r in range(8):
            w = tb[r*2] | (tb[r*2+1] << 8)
            for p in range(8):
                cr, cg, cb = PALETTE[(w >> 2*p) & 3]
                for dy in range(sc):
                    for dx in range(sc):
                        i = ((gy+r*sc+dy)*W + (gx+p*sc+dx))*3
                        rgb[i:i+3] = bytes((cr, cg, cb))
    png_rgb(out, W, H, bytes(rgb))

if __name__ == "__main__":
    dump = sys.argv[1] if len(sys.argv) > 1 else "/tmp/emu2_dump.bin"
    outdir = sys.argv[2] if len(sys.argv) > 2 else "assets/original/extracted/crocodile"
    mem = open(dump, "rb").read()
    render_screen(mem, f"{outdir}/title_screen.png")
    render_sheet(mem, f"{outdir}/sprites.png", 0o20400, 0o22400)
    print(f"rendered title_screen.png + sprites.png -> {outdir}/")
