#!/usr/bin/env python3
"""
csg_extract.py — extract sprites from RCT1's CSG1I.DAT + csg1.1 pair
                 into palette PNGs (index 0 = transparent).

The RCT1 CSG sprite archive is two files:
  csg1i.dat  — 69,917 × 16-byte StoredG1Element records
  csg1.1     — concatenated pixel data referenced by records' offsets

StoredG1Element layout (little endian):
  uint32 offset       byte offset into csg1.1 where this sprite's data begins
  int16  width
  int16  height
  int16  x_offset     screen-space x where pixel (0,0) of the sprite sits
  int16  y_offset     screen-space y where pixel (0,0) of the sprite sits
  uint16 flags        bit0 = BMP, bit2 = RLE-compressed
  uint16 zoomed_offset

Pixel format (flags 0x05 = BMP + RLE):
  data[0 .. 2*height-1]  uint16 row offsets (relative to sprite data start)
  per row, starting at its row offset, RLE chunks until end-of-row:
    byte code       bit7 = end-of-row, bits 0..6 = run length
    byte x_start    pixel x where this run begins in the row
    code&0x7F bytes of palette indices
  Pixels not covered by any run = palette 0 (transparent).
"""

import argparse
import os
import struct
from pathlib import Path

from PIL import Image

from rct_palette import RCT_PALETTE_BYTES as _PALETTE


def load_index(idx_path: Path):
    """Return list of (offset, width, height, x_off, y_off, flags, zoom_off)."""
    raw = idx_path.read_bytes()
    if len(raw) % 16 != 0:
        raise ValueError(f"{idx_path} size {len(raw)} not a multiple of 16")
    n = len(raw) // 16
    records = []
    for i in range(n):
        records.append(struct.unpack_from('<IhhhhHH', raw, i * 16))
    return records


def decode_rle(data: bytes, off: int, w: int, h: int) -> bytes:
    """Decode one RLE-encoded sprite into a w*h palette buffer."""
    pixels = bytearray(w * h)  # initialised to 0 = transparent
    # Row offset table starts at `off`; 2 bytes per row, total 2*h.
    for row in range(h):
        row_off = struct.unpack_from('<H', data, off + 2 * row)[0]
        cur = off + row_off
        while True:
            if cur >= len(data):
                return bytes(pixels)
            code = data[cur]; cur += 1
            run_len = code & 0x7F
            last_in_row = code & 0x80
            x_start = data[cur]; cur += 1
            run = data[cur:cur + run_len]; cur += run_len
            # Clamp draw window to the sprite's row.
            for i, p in enumerate(run):
                x = x_start + i
                if 0 <= x < w:
                    pixels[row * w + x] = p
            if last_in_row:
                break
    return bytes(pixels)


def decode_raw(data: bytes, off: int, w: int, h: int) -> bytes:
    return data[off:off + w * h]


# Palette indices RCT uses for ground shadows. These overlap with chassis /
# wheel colours visually, so we identify shadow pixels SPATIALLY: any pixel
# in this range that's reachable via flood-fill (through other shadow pixels
# AND transparent pixels) from outside the sprite is treated as shadow.
SHADOW_PALETTE_RANGE = range(10, 14)


def mask_shadows(buf: bytearray, w: int, h: int) -> bytearray:
    """Clear shadow pixels by flood-filling from the sprite border through
    transparent (palette 0) and shadow-palette pixels. Anything reached and
    still in the shadow palette range is set to 0. Interior chassis/wheel
    pixels of the same palette indices are NOT reached because they're
    bounded by non-shadow body colours."""
    shadow_set = set(SHADOW_PALETTE_RANGE)
    visited = bytearray(w * h)  # 0 = unvisited, 1 = exterior
    stack: list[tuple[int, int]] = []

    # Seed with border pixels that are themselves transparent or shadow-palette.
    def seed(x: int, y: int) -> None:
        if 0 <= x < w and 0 <= y < h:
            p = buf[y * w + x]
            if (p == 0 or p in shadow_set) and not visited[y * w + x]:
                visited[y * w + x] = 1
                stack.append((x, y))

    for x in range(w):
        seed(x, 0); seed(x, h - 1)
    for y in range(h):
        seed(0, y); seed(w - 1, y)

    while stack:
        x, y = stack.pop()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if not (0 <= nx < w and 0 <= ny < h): continue
            if visited[ny * w + nx]: continue
            p = buf[ny * w + nx]
            if p == 0 or p in shadow_set:
                visited[ny * w + nx] = 1
                stack.append((nx, ny))

    out = bytearray(buf)
    for i, v in enumerate(visited):
        if v and out[i] in shadow_set:
            out[i] = 0
    return out


def extract_sprite(records, data: bytes, idx: int,
                   mask_shadow: bool = False) -> Image.Image | None:
    rec = records[idx]
    off, w, h, x_off, y_off, flags, _ = rec
    if w <= 0 or h <= 0:
        return None
    if flags & 0x04:           # RLE
        buf = decode_rle(data, off, w, h)
    elif flags & 0x01:         # BMP raw
        buf = decode_raw(data, off, w, h)
    else:
        # Unknown / palette-only — return None so caller can skip.
        return None
    if mask_shadow:
        buf = bytes(mask_shadows(bytearray(buf), w, h))
    img = Image.frombytes('P', (w, h), buf)
    img.putpalette(_PALETTE)
    img.info['transparency'] = 0
    return img


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--idx', required=True, help='path to csg1i.dat')
    ap.add_argument('--data', required=True, help='path to csg1.1 (or CSG1.DAT)')
    ap.add_argument('--out', required=True, help='output directory')
    ap.add_argument('--indices', nargs='*', type=int,
                    help='specific sprite indices to extract; if omitted, extract all')
    ap.add_argument('--manifest', action='store_true',
                    help='emit manifest.tsv with width/height/x/y per index')
    ap.add_argument('--mask-shadows', action='store_true',
                    help='flood-fill shadow pixels (palette 10-13 reachable '
                         'from sprite border via shadow-or-transparent) to '
                         'transparent. Wheels/chassis interior to body remain.')
    args = ap.parse_args()

    records = load_index(Path(args.idx))
    data = Path(args.data).read_bytes()
    out_dir = Path(args.out); out_dir.mkdir(parents=True, exist_ok=True)

    indices = args.indices if args.indices else range(len(records))

    manifest = []
    written = skipped = 0
    for i in indices:
        if i < 0 or i >= len(records):
            print(f'  [{i}] out of range'); skipped += 1; continue
        rec = records[i]
        off, w, h, x_off, y_off, flags, zoom = rec
        img = extract_sprite(records, data, i, mask_shadow=args.mask_shadows)
        if img is None:
            skipped += 1
            manifest.append((i, w, h, x_off, y_off, flags, 'skipped'))
            continue
        path = out_dir / f'{i}.png'
        img.save(path)
        written += 1
        manifest.append((i, w, h, x_off, y_off, flags, path.name))

    if args.manifest:
        with (out_dir / 'manifest.tsv').open('w') as f:
            f.write('index\twidth\theight\tx_offset\ty_offset\tflags\tfile\n')
            for row in manifest:
                f.write('\t'.join(str(x) for x in row) + '\n')

    print(f'wrote {written}  skipped {skipped}  → {out_dir}')


if __name__ == '__main__':
    main()
