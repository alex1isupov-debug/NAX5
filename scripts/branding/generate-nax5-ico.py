#!/usr/bin/env python3
"""Generate gui/nax5.ico from the NAX5 app icon design."""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "gui" / "nax5.ico"
SIZES = (16, 24, 32, 48, 64, 128, 256)


def lerp(a: int, b: int, t: float) -> int:
    return int(a + (b - a) * t)


def gradient_color(x: float, y: float, size: int) -> tuple[int, int, int, int]:
    t = (x + y) / (2 * size)
    r = lerp(0x19, 0x12, t)
    g = lerp(0xD4, 0x49, t)
    b = lerp(0xFF, 0xE8, t)
    return (r, g, b, 255)


def draw_icon(size: int) -> Image.Image:
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    radius = max(2, round(size * 0.234))
    draw.rounded_rectangle((0, 0, size - 1, size - 1), radius=radius, fill=(7, 11, 24, 255))

    scale = size / 64.0
    points = [
        (15, 14),
        (27, 14),
        (37, 25),
        (47, 14),
        (59, 14),
        (43, 32),
        (59, 50),
        (47, 50),
        (37, 39),
        (26, 50),
        (14, 50),
        (31, 32),
        (15, 14),
    ]
    scaled = [(round(x * scale), round(y * scale)) for x, y in points]

    xs = [p[0] for p in scaled]
    ys = [p[1] for p in scaled]
    min_x, max_x = min(xs), max(xs)
    min_y, max_y = min(ys), max(ys)
    center_x = (min_x + max_x) / 2
    center_y = (min_y + max_y) / 2

    mask = Image.new("L", (size, size), 0)
    mask_draw = ImageDraw.Draw(mask)
    mask_draw.polygon(scaled, fill=255)

    fill = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    fill_draw = ImageDraw.Draw(fill)
    for y in range(size):
        for x in range(size):
            if mask.getpixel((x, y)):
                fill_draw.point((x, y), gradient_color(x - center_x, y - center_y, size))

    highlight = [
        (round(21 * scale), round(14 * scale)),
        (round(27 * scale), round(14 * scale)),
        (round(37 * scale), round(25 * scale)),
        (round(47 * scale), round(14 * scale)),
        (round(53 * scale), round(14 * scale)),
        (round(37 * scale), round(31 * scale)),
        (round(21 * scale), round(14 * scale)),
    ]
    draw.polygon(highlight, fill=(124, 238, 255, 180))

    img.alpha_composite(fill)
    return img


def main() -> None:
    images = [draw_icon(size) for size in SIZES]
    images[0].save(
        OUT,
        format="ICO",
        sizes=[(size, size) for size in SIZES],
        append_images=images[1:],
    )
    print(f"Wrote {OUT}")


if __name__ == "__main__":
    main()
