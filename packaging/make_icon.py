#!/usr/bin/env python3
"""
Build a macOS .icns from packaging/icon_source.png.
Applies the standard macOS rounded-square ("squircle") mask and
generates every size required by Apple's icon set.
"""

import os
import sys
import subprocess
from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(ROOT, "icon_source.png")
ICONSET = os.path.join(ROOT, "MathEngine.iconset")
ICNS = os.path.join(ROOT, "MathEngine.icns")

# macOS icons fit content inside a squircle with ~18% padding
# Final output is 1024x1024 base; content area is ~824x824
BASE = 1024
PAD = int(BASE * 0.10)
CONTENT = BASE - PAD * 2
CORNER_RADIUS = int(BASE * 0.225)  # macOS squircle radius

def squircle_mask(size, radius):
    """Generate a rounded-rect mask at high resolution."""
    mask = Image.new("L", (size * 4, size * 4), 0)
    d = ImageDraw.Draw(mask)
    d.rounded_rectangle(
        [(0, 0), (size * 4 - 1, size * 4 - 1)],
        radius=radius * 4,
        fill=255,
    )
    return mask.resize((size, size), Image.LANCZOS)

def build_base_icon():
    """Create the 1024x1024 master icon."""
    src = Image.open(SRC).convert("RGBA")
    sw, sh = src.size

    # Squircle background — gradient blue
    base = Image.new("RGBA", (BASE, BASE), (0, 0, 0, 0))
    bg = Image.new("RGBA", (BASE, BASE), (0, 0, 0, 0))
    bd = ImageDraw.Draw(bg)
    # Soft white background to match source style
    bd.rectangle([(0, 0), (BASE, BASE)], fill=(252, 253, 255, 255))
    mask = squircle_mask(BASE, CORNER_RADIUS)
    base.paste(bg, (0, 0), mask)

    # Resize source to fit content area, keeping aspect
    scale = CONTENT / max(sw, sh)
    new_w = int(sw * scale)
    new_h = int(sh * scale)
    src_resized = src.resize((new_w, new_h), Image.LANCZOS)

    # Paste centered
    x = (BASE - new_w) // 2
    y = (BASE - new_h) // 2
    fg_canvas = Image.new("RGBA", (BASE, BASE), (0, 0, 0, 0))
    fg_canvas.paste(src_resized, (x, y), src_resized)

    # Apply squircle mask to combined result so corners are clipped
    base = Image.alpha_composite(base, fg_canvas)
    masked = Image.new("RGBA", (BASE, BASE), (0, 0, 0, 0))
    masked.paste(base, (0, 0), mask)

    return masked

def main():
    if not os.path.exists(SRC):
        print(f"Missing {SRC}", file=sys.stderr)
        sys.exit(1)

    print("Building master icon...")
    master = build_base_icon()

    # Save master so we can inspect it
    master.save(os.path.join(ROOT, "icon_master_1024.png"))

    # Apple's required sizes for .iconset
    sizes = [
        (16,   "icon_16x16.png"),
        (32,   "icon_16x16@2x.png"),
        (32,   "icon_32x32.png"),
        (64,   "icon_32x32@2x.png"),
        (128,  "icon_128x128.png"),
        (256,  "icon_128x128@2x.png"),
        (256,  "icon_256x256.png"),
        (512,  "icon_256x256@2x.png"),
        (512,  "icon_512x512.png"),
        (1024, "icon_512x512@2x.png"),
    ]

    # Clean iconset dir
    if os.path.exists(ICONSET):
        import shutil; shutil.rmtree(ICONSET)
    os.makedirs(ICONSET)

    for size, name in sizes:
        img = master.resize((size, size), Image.LANCZOS)
        img.save(os.path.join(ICONSET, name), optimize=True)
        print(f"  {name} ({size}x{size})")

    print(f"Running iconutil...")
    subprocess.run(
        ["iconutil", "-c", "icns", "-o", ICNS, ICONSET],
        check=True,
    )
    print(f"Wrote {ICNS}")

if __name__ == "__main__":
    main()
