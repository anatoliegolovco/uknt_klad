#!/usr/bin/env python3
"""Compose two screenshots side by side (original | ours) with labels, for fidelity review.

Usage: python3 tools/compare_screens.py <original.png> <ours.png> <out.png> [label_l] [label_r]
The original (headless УКНЦ, 640x288) and ours (raylib window) are scaled to the same height.
"""
import sys
from PIL import Image, ImageDraw

def load_scaled(path, h):
    im = Image.open(path).convert('RGB')
    w = int(im.width * h / im.height)
    return im.resize((w, h), Image.NEAREST)

def main():
    a, b, out = sys.argv[1], sys.argv[2], sys.argv[3]
    la = sys.argv[4] if len(sys.argv) > 4 else "ORIGINAL (headless УКНЦ)"
    lb = sys.argv[5] if len(sys.argv) > 5 else "OURS (C23 + raylib)"
    H = 288
    ia, ib = load_scaled(a, H), load_scaled(b, H)
    pad, bar = 12, 22
    W = ia.width + ib.width + pad * 3
    img = Image.new('RGB', (W, H + bar + pad), (24, 24, 28))
    dr = ImageDraw.Draw(img)
    img.paste(ia, (pad, bar))
    img.paste(ib, (pad * 2 + ia.width, bar))
    dr.text((pad, 5), la, fill=(255, 230, 120))
    dr.text((pad * 2 + ia.width, 5), lb, fill=(120, 230, 255))
    img.save(out)
    print(f"side-by-side -> {out}")

if __name__ == "__main__":
    main()
