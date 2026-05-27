"""RCT colour palette extracted verbatim from RCTGen-wip/src/iso-render/Palette.cpp.

The on-disk palette has 255 entries; index 0 is transparent and indices 1-9
are reserved (all zero). Index 255 is unused (sentinel kFragmentUnused).
"""

from __future__ import annotations
import re
from pathlib import Path


def load_rct_palette(palette_cpp: Path) -> list[tuple[int, int, int]]:
    src = palette_cpp.read_text()
    trips = re.findall(r'\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}', src)
    # The main 256-entry palette ends with (92,255,92); the subsequent block
    # is the 12-entry transparent-grey palette which we don't need here.
    main_end = next(i + 1 for i in range(len(trips) - 1)
                    if trips[i] == ('92', '255', '92')
                    and trips[i + 1] == ('23', '35', '35'))
    pal = [tuple(int(c) for c in t) for t in trips[:main_end]]
    # Pad to 256 with a debug magenta so we never silently lose pixels.
    while len(pal) < 256:
        pal.append((255, 0, 255))
    return pal


def palette_to_bytes(pal: list[tuple[int, int, int]]) -> bytes:
    """Flat RGB byte string suitable for PIL.Image.putpalette()."""
    out = bytearray()
    for r, g, b in pal:
        out += bytes((r, g, b))
    return bytes(out)


# Default location relative to repo root.
DEFAULT_PALETTE_CPP = Path(__file__).resolve().parents[1] \
    / 'RCTGen-wip' / 'src' / 'iso-render' / 'Palette.cpp'

RCT_PALETTE: list[tuple[int, int, int]] = load_rct_palette(DEFAULT_PALETTE_CPP)
RCT_PALETTE_BYTES: bytes = palette_to_bytes(RCT_PALETTE)
