#!/usr/bin/env python3
"""
DMG background, rendered with SF Pro at 2x retina resolution.
We render natively at 2x (1800x1000) and save that as the primary background.
Logical window size is 900x500.
"""

import os
from PIL import Image, ImageDraw, ImageFont

# Logical (1x) window content size
W, H = 900, 500

# Render at 2x for retina sharpness
SCALE = 2

# Layout — icon centers
LEFT_X = 230
RIGHT_X = 670
CENTER_X = (LEFT_X + RIGHT_X) // 2  # 450
ROW_Y = 290

SF_BOLD = "/Library/Fonts/SF-Pro-Display-Bold.otf"
SF_REG = "/Library/Fonts/SF-Pro-Display-Regular.otf"
SF_TEXT_REG = "/Library/Fonts/SF-Pro-Text-Regular.otf"

def font(path, size):
    if os.path.exists(path):
        return ImageFont.truetype(path, size)
    for fp in ("/System/Library/Fonts/SFNS.ttf",
               "/System/Library/Fonts/Helvetica.ttc"):
        if os.path.exists(fp):
            return ImageFont.truetype(fp, size)
    return ImageFont.load_default()

def draw_image(scale):
    w, h = W * scale, H * scale
    img = Image.new("RGBA", (w, h), (255, 255, 255, 255))
    d = ImageDraw.Draw(img)

    arrow_y = int(ROW_Y * scale)

    # ---- "Drag to Applications folder" label ----
    label_font = font(SF_BOLD, int(24 * scale))
    label = "Drag to Applications folder"
    lb = d.textbbox((0, 0), label, font=label_font)
    lw = lb[2] - lb[0]
    label_y = arrow_y - int(60 * scale)
    d.text(
        (int(CENTER_X * scale) - lw // 2, label_y),
        label,
        font=label_font,
        fill=(30, 30, 40, 255),
    )

    # ---- Arrow ----
    arrow_x1 = int((LEFT_X + 70) * scale)
    arrow_x2 = int((RIGHT_X - 70) * scale)
    arrow_color = (40, 40, 50, 255)

    # Dashed shaft
    seg_len = int(22 * scale)
    gap = int(12 * scale)
    head_size = int(32 * scale)
    line_w = int(6 * scale)

    shaft_end = arrow_x2 - head_size - int(2 * scale)
    x = arrow_x1
    while x + seg_len <= shaft_end:
        d.line(
            [(x, arrow_y), (x + seg_len, arrow_y)],
            fill=arrow_color,
            width=line_w,
        )
        x += seg_len + gap

    # Arrow head
    head_pts = [
        (arrow_x2, arrow_y),
        (arrow_x2 - head_size, arrow_y - head_size // 2),
        (arrow_x2 - head_size, arrow_y + head_size // 2),
    ]
    d.polygon(head_pts, fill=arrow_color)

    # ---- Install help link at the bottom ----
    link_font = font(SF_TEXT_REG, int(15 * scale))
    link_text = "Install help: github.com/theomirzakhanian/mathengine/blob/main/INSTALL_MAC.md"
    lb = d.textbbox((0, 0), link_text, font=link_font)
    tw = lb[2] - lb[0]
    d.text(
        (int(CENTER_X * scale) - tw // 2, h - int(40 * scale)),
        link_text,
        font=link_font,
        fill=(70, 110, 200, 255),
    )

    return img

if __name__ == "__main__":
    out_dir = os.path.dirname(os.path.abspath(__file__))

    # @1x (logical resolution): 900x500
    img1 = draw_image(scale=1)
    img1.save(os.path.join(out_dir, "dmg_background.png"), optimize=True)

    # @2x retina: 1800x1000 — auto-picked by Finder on HiDPI
    img2 = draw_image(scale=2)
    img2.save(os.path.join(out_dir, "dmg_background@2x.png"), optimize=True)

    print(f"Wrote dmg_background.png ({W}x{H})")
    print(f"Wrote dmg_background@2x.png ({W*2}x{H*2})")
