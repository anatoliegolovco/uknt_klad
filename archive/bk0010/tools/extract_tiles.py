#!/usr/bin/env python3
"""Extract КЛАД (KLAD3.BIN, Баранов line) tile bank + level maps.

Format cracked by tracing the blit routine (docs/reverse/08_level_data.md):

  * Tile bank @ octal 017450: 16 tiles, 16 bytes each = 8x8 pixels at 2bpp.
    Each row is one little-endian word; pixel p uses bits (2p, 2p+1), LSB=left.
  * Map nibble -> tile index (BIC #177760 keeps the low nibble; the high nibble
    is the next tile). Source addr = 017450 + index*16.
  * Level table @ octal 010406: 20 records, 10 words each; field 0 steps by
    0540 (=352) -> level maps at 022100, 022640, ... one 352-byte block each.
  * A level is 32x22 tiles packed two-per-byte (low nibble = left cell).
  * Playfield is blitted to screen base 046000 (040000 + 6 tile-rows).

Outputs PNGs + raw .bin under assets/original/extracted/klad3/.
This is for personal study of an abandonware title (see design/legal.md).
"""
import struct, os, sys, zlib

TILE_BANK = 0o17450
LEVEL_TABLE_FIRST = 0o22100
LEVEL_STRIDE = 0o540
N_TILES = 16
N_LEVELS = 20
COLS, ROWS = 32, 22

def load_bin(path):
    d = open(path, "rb").read()
    load, ln = struct.unpack("<HH", d[:4])
    return d[4:4 + ln], load

def png(path, w, h, rgb):  # rgb: bytes of length w*h*3
    raw = bytearray()
    for y in range(h):
        raw.append(0); raw += rgb[y*w*3:(y+1)*w*3]
    def chunk(t, d):
        c = t + d
        return struct.pack(">I", len(d)) + c + struct.pack(">I", zlib.crc32(c) & 0xffffffff)
    sig = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)  # 8-bit RGB
    open(path, "wb").write(sig + chunk(b"IHDR", ihdr) +
                           chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b""))

# 4-colour palette (approx; the BK 2bpp values 0..3). Tweak to taste.
PALETTE = [(0, 0, 0), (40, 200, 220), (60, 200, 70), (235, 235, 235)]

def tile_rows(tb):  # 16 bytes -> 8 rows x 8 values(0..3)
    return [[(tb[r*2] | (tb[r*2+1] << 8)) >> (2*p) & 3 for p in range(8)] for r in range(8)]

def main():
    src = sys.argv[1] if len(sys.argv) > 1 else "assets/original/ex_klad3/KLAD3.BIN"
    outdir = "assets/original/extracted/klad3"
    os.makedirs(outdir, exist_ok=True)
    d, load = load_bin(src)
    bank_off = TILE_BANK - load
    bank = [tile_rows(d[bank_off + t*16: bank_off + t*16 + 16]) for t in range(N_TILES)]
    open(f"{outdir}/tiles.bin", "wb").write(d[bank_off:bank_off + N_TILES*16])

    # tile sheet, 8 per row, scaled
    sc = 12; cols = 8; rws = 2; sep = 2
    W = cols*(8*sc+sep)+sep; H = rws*(8*sc+sep)+sep
    img = bytearray(W*H*3)
    for t in range(N_TILES):
        gx = (t % cols)*(8*sc+sep)+sep; gy = (t//cols)*(8*sc+sep)+sep
        for ry in range(8):
            for rx in range(8):
                r, g, b = PALETTE[bank[t][ry][rx]]
                for dy in range(sc):
                    for dx in range(sc):
                        i = ((gy+ry*sc+dy)*W + (gx+rx*sc+dx))*3
                        img[i:i+3] = bytes((r, g, b))
    png(f"{outdir}/tiles.png", W, H, bytes(img))

    # levels
    for lv in range(N_LEVELS):
        addr = LEVEL_TABLE_FIRST + lv*LEVEL_STRIDE
        off = addr - load
        block = d[off:off + 352]
        open(f"{outdir}/level_{lv:02d}.bin", "wb").write(block)
        cells = []
        for byte in block:
            cells += [byte & 15, (byte >> 4) & 15]
        sc = 4; W = COLS*8*sc; H = ROWS*8*sc
        img = bytearray(W*H*3)
        for i, idx in enumerate(cells[:COLS*ROWS]):
            cx = (i % COLS)*8; cy = (i//COLS)*8
            for ry in range(8):
                for rx in range(8):
                    r, g, b = PALETTE[bank[idx][ry][rx]]
                    for dy in range(sc):
                        for dx in range(sc):
                            p = ((cy*sc+ry*sc+dy)*W + (cx*sc+rx*sc+dx))*3
                            img[p:p+3] = bytes((r, g, b))
        png(f"{outdir}/level_{lv:02d}.png", W, H, bytes(img))
    print(f"extracted {N_TILES} tiles + {N_LEVELS} levels -> {outdir}/")

if __name__ == "__main__":
    main()
