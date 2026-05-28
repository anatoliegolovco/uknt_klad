#!/usr/bin/env python3
"""
extract_data.py -- dump & visualise candidate data blocks from a КЛАД image.

This is a STUDY tool. It locates the 20-entry record table found at 010406 in
KLAD3.BIN (first field steps by 0540 octal = 352 bytes) and dumps each 352-byte
data block both raw and as a 1-bpp PNG (BK framebuffer convention: bit set =
pixel on, LSB first), so the layouts can be eyeballed and correlated with the
emulator. Output goes to assets/original/extracted/ which is git-ignored: these
are derivatives of the copyrighted binary and stay local.

No external deps -- includes a tiny zlib-based PNG writer.

Usage:
    python3 tools/extract_data.py assets/original/ex_klad3/KLAD3.BIN
    python3 tools/extract_data.py <bin> --table 010406 --stride 0540 \
            --count 20 --width 256 --montage out.png
"""
import sys, argparse, zlib, struct, os

def load_bin(path):
    b = open(path, "rb").read()
    load = b[0] | b[1] << 8
    length = b[2] | b[3] << 8
    return b[4:4 + length], load

def png_gray(path, w, h, pixels):
    """Write an 8-bit grayscale PNG. `pixels` is a bytes object of len w*h."""
    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xffffffff)
    raw = bytearray()
    for y in range(h):
        raw.append(0)                       # filter: none
        raw += pixels[y * w:(y + 1) * w]
    out = b"\x89PNG\r\n\x1a\n"
    out += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 0, 0, 0, 0))
    out += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
    out += chunk(b"IEND", b"")
    open(path, "wb").write(out)

def block_to_pixels(blk, width_px):
    """1-bpp, LSB-first, `width_px` pixels per row -> grayscale bytes."""
    bits = []
    for byte in blk:
        for bit in range(8):
            bits.append(0 if (byte >> bit) & 1 else 255)  # set bit = black
    rows = (len(bits) + width_px - 1) // width_px
    bits += [255] * (rows * width_px - len(bits))
    return bytes(bits), width_px, rows

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("file")
    ap.add_argument("--table", type=lambda x: int(x, 8), default=0o10406)
    ap.add_argument("--stride", type=lambda x: int(x, 8), default=0o540)
    ap.add_argument("--count", type=int, default=20)
    ap.add_argument("--width", type=int, default=256, help="bitmap width in px")
    ap.add_argument("--first", type=lambda x: int(x, 8), default=0o22100,
                    help="address of first data block")
    ap.add_argument("--out", default="assets/original/extracted")
    ap.add_argument("--montage", default=None)
    args = ap.parse_args()

    code, load = load_bin(args.file)
    os.makedirs(args.out, exist_ok=True)
    tiles = []
    for i in range(args.count):
        addr = args.first + i * args.stride
        off = addr - load
        blk = code[off:off + args.stride]
        if len(blk) < args.stride:
            break
        open(f"{args.out}/block_{i:02d}_{addr:06o}.bin", "wb").write(blk)
        px, w, h = block_to_pixels(blk, args.width)
        png_gray(f"{args.out}/block_{i:02d}_{addr:06o}.png", w, h, px)
        tiles.append((px, w, h))
        print(f"block {i:2d} @0{addr:o}: {len(blk)} bytes -> "
              f"{args.out}/block_{i:02d}_{addr:06o}.png ({w}x{h})")

    if args.montage and tiles:
        w = tiles[0][1]; gap = 4
        total_h = sum(h + gap for _, _, h in tiles)
        canvas = bytearray([200]) * (w * total_h)
        y = 0
        for px, _, h in tiles:
            for r in range(h):
                canvas[(y + r) * w:(y + r) * w + w] = px[r * w:(r + 1) * w]
            y += h + gap
        png_gray(args.montage, w, total_h, bytes(canvas))
        print(f"montage -> {args.montage} ({w}x{total_h})")

if __name__ == "__main__":
    main()
