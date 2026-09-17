"""Crisp vector-style NAX5 launcher icon tiles."""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw

ICO_SIZES = (16, 24, 32, 48, 64, 128, 256)

X_POINTS_64 = [
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
X_HIGHLIGHT_64 = [
    (21, 14),
    (27, 14),
    (37, 25),
    (47, 14),
    (53, 14),
    (37, 31),
    (21, 14),
]
WIFI_CENTER_64 = (46, 17)
WIFI_RADII_64 = (5, 8, 11)


def _scale_points(points: list[tuple[int, int]], size: int) -> list[tuple[int, int]]:
    scale = size / 64.0
    return [(round(x * scale), round(y * scale)) for x, y in points]


def _draw_wifi_arcs(draw: ImageDraw.ImageDraw, size: int) -> None:
    if size < 24:
        return

    scale = size / 64.0
    cx = round(WIFI_CENTER_64[0] * scale)
    cy = round(WIFI_CENTER_64[1] * scale)
    stroke = max(1, round(2.2 * scale))
    color = (25, 212, 255, 255)

    for radius in WIFI_RADII_64:
        r = max(2, round(radius * scale))
        bbox = (cx - r, cy - r, cx + r, cy + r)
        draw.arc(bbox, 205, 300, fill=color, width=stroke)


def draw_crisp_icon(size: int) -> Image.Image:
    """Draw a native-resolution launcher icon (no blurry downscale)."""
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    radius = max(2, round(size * 0.22))
    draw.rounded_rectangle((0, 0, size - 1, size - 1), radius=radius, fill=(7, 11, 24, 255))

    x_points = _scale_points(X_POINTS_64, size)
    draw.polygon(x_points, fill=(25, 212, 255, 255))

    if size >= 20:
        highlight = _scale_points(X_HIGHLIGHT_64, size)
        draw.polygon(highlight, fill=(124, 238, 255, 200))

    _draw_wifi_arcs(draw, size)
    return img


def save_ico(path: Path) -> None:
    images = [draw_crisp_icon(size) for size in ICO_SIZES]
    images[0].save(
        path,
        format="ICO",
        sizes=[(size, size) for size in ICO_SIZES],
        append_images=images[1:],
    )
