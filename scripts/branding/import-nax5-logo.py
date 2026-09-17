#!/usr/bin/env python3
"""Build launcher branding assets from the NAX5 brand sheet or logo source."""

from __future__ import annotations

import shutil
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
BRANDING = ROOT / "assets" / "branding"
RES = ROOT / "gui" / "res"
GUI = ROOT / "gui"

DEFAULT_SHEET = BRANDING / "nax5-brand-sheet.jpg"
DEFAULT_SOURCE = BRANDING / "nax5-logo-source.jpg"
ICO_SIZES = (16, 24, 32, 48, 64, 128, 256)


def _content_mask(img: Image.Image, threshold: int = 20) -> np.ndarray:
    arr = np.array(img.convert("RGB"))
    return arr.max(axis=2) > threshold


def _bbox_from_mask(mask: np.ndarray, pad: int = 0) -> tuple[int, int, int, int] | None:
    rows = np.where(mask.any(axis=1))[0]
    cols = np.where(mask.any(axis=0))[0]
    if len(rows) == 0 or len(cols) == 0:
        return None
    left = max(0, int(cols[0]) - pad)
    top = max(0, int(rows[0]) - pad)
    right = min(mask.shape[1], int(cols[-1]) + 1 + pad)
    bottom = min(mask.shape[0], int(rows[-1]) + 1 + pad)
    return left, top, right, bottom


def is_brand_sheet(img: Image.Image) -> bool:
    mask = _content_mask(img)
    h, w = mask.shape
    top = mask[: h // 2, :]
    bottom = mask[h // 2 :, :]
    top_left = top[:, : w // 2]
    bottom_band = bottom[:, int(w * 0.1) : int(w * 0.9)]
    return bool(top_left.any()) and bool(bottom_band.any())


def extract_app_icon(img: Image.Image) -> Image.Image:
    mask = _content_mask(img)
    h, w = mask.shape
    top = mask[: int(h * 0.55), :]
    left = top[:, : w // 2]
    bbox = _bbox_from_mask(left, pad=4)
    if bbox is None:
        raise RuntimeError("Could not find app icon region in brand sheet")
    left_x, top_y, right_x, bottom_y = bbox
    icon = img.crop((left_x, top_y, right_x, bottom_y)).convert("RGBA")

    side = max(icon.width, icon.height)
    square = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    offset = ((side - icon.width) // 2, (side - icon.height) // 2)
    square.alpha_composite(icon, offset)
    return square


def extract_wordmark(img: Image.Image) -> Image.Image:
    mask = _content_mask(img)
    h, w = mask.shape
    bottom = mask[int(h * 0.52) :, :]
    bbox = _bbox_from_mask(bottom, pad=8)
    if bbox is None:
        top = mask[: int(h * 0.55), :]
        right = top[:, w // 2 :]
        bbox = _bbox_from_mask(right, pad=8)
        if bbox is None:
            raise RuntimeError("Could not find wordmark region in brand sheet")
        left_x, top_y, right_x, bottom_y = bbox
        return remove_dark_background(img.crop((left_x + w // 2, top_y, right_x + w // 2, bottom_y)))

    left_x, top_y, right_x, bottom_y = bbox
    top_y += int(h * 0.52)
    bottom_y += int(h * 0.52)
    return remove_dark_background(img.crop((left_x, top_y, right_x, bottom_y)))


def remove_dark_background(img: Image.Image) -> Image.Image:
    rgba = img.convert("RGBA")
    pixels = rgba.load()
    width, height = rgba.size

    samples = [
        rgba.getpixel((2, 2)),
        rgba.getpixel((width - 3, 2)),
        rgba.getpixel((2, height - 3)),
        rgba.getpixel((width - 3, height - 3)),
    ]
    bg_r = sum(s[0] for s in samples) // len(samples)
    bg_g = sum(s[1] for s in samples) // len(samples)
    bg_b = sum(s[2] for s in samples) // len(samples)

    for y in range(height):
        for x in range(width):
            r, g, b, _ = rgba.getpixel((x, y))
            dr = r - bg_r
            dg = g - bg_g
            db = b - bg_b
            dist = (dr * dr + dg * dg + db * db) ** 0.5
            peak = max(r, g, b)

            if dist < 16 and peak < 24:
                alpha = 0
            elif dist < 38:
                alpha = int(min(255, max(0, (dist - 16) * 255 / 22)))
            else:
                alpha = 255

            if peak > 16 and b >= max(r, g) + 5:
                alpha = max(alpha, min(255, int((peak - 10) * 5)))

            pixels[x, y] = (r, g, b, alpha)

    return trim_transparent(rgba, pad=6)


def trim_transparent(img: Image.Image, pad: int = 8) -> Image.Image:
    bbox = img.getbbox()
    if not bbox:
        return img
    left = max(0, bbox[0] - pad)
    top = max(0, bbox[1] - pad)
    right = min(img.width, bbox[2] + pad)
    bottom = min(img.height, bbox[3] + pad)
    return img.crop((left, top, right, bottom))


def make_white_variant(img: Image.Image) -> Image.Image:
    rgba = img.convert("RGBA")
    pixels = rgba.load()
    for y in range(rgba.height):
        for x in range(rgba.width):
            r, g, b, a = pixels[x, y]
            if a == 0:
                continue
            if b >= max(r, g) + 8:
                pixels[x, y] = (170, 220, 255, a)
            else:
                pixels[x, y] = (255, 255, 255, a)
    return rgba


def resize_square_icon(icon: Image.Image, size: int) -> Image.Image:
    return icon.resize((size, size), Image.Resampling.LANCZOS)


def save_ico(icon_master: Image.Image, path: Path) -> None:
    # Pillow 12: pass one 256px master and `sizes=...` to embed all layers.
    # Do not combine `sizes` with `append_images` — that writes only 16x16.
    master = resize_square_icon(icon_master, 256).convert("RGBA")
    master.save(path, format="ICO", sizes=[(size, size) for size in ICO_SIZES])


def build_from_sheet(img: Image.Image) -> tuple[Image.Image, Image.Image]:
    return extract_app_icon(img), extract_wordmark(img)


def build_from_single_logo(img: Image.Image) -> tuple[Image.Image, Image.Image]:
    wordmark = trim_transparent(remove_dark_background(img))
    side = max(wordmark.width, wordmark.height)
    square = Image.new("RGBA", (side, side), (7, 11, 24, 255))
    scale = min(0.82 * side / wordmark.width, 0.82 * side / wordmark.height)
    target = (max(1, round(wordmark.width * scale)), max(1, round(wordmark.height * scale)))
    scaled = wordmark.resize(target, Image.Resampling.LANCZOS)
    offset = ((side - scaled.width) // 2, (side - scaled.height) // 2)
    square.alpha_composite(scaled, offset)
    return square, wordmark


def main() -> int:
    source = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_SHEET
    if not source.exists() and DEFAULT_SOURCE.exists():
        source = DEFAULT_SOURCE
    if not source.exists():
        print(f"Source image not found: {source}", file=sys.stderr)
        return 1

    BRANDING.mkdir(parents=True, exist_ok=True)
    if source.resolve() != DEFAULT_SHEET.resolve():
        shutil.copy2(source, DEFAULT_SHEET)

    raw = Image.open(source)
    if is_brand_sheet(raw):
        app_icon, wordmark = build_from_sheet(raw)
    else:
        app_icon, wordmark = build_from_single_logo(raw)

    icon_master = resize_square_icon(app_icon, 256)
    wordmark_white = make_white_variant(wordmark)

    logo_path = RES / "nax5-logo.png"
    logo_white_path = RES / "nax5-logo-white.png"
    icon_path = RES / "nax5.png"
    ico_path = GUI / "nax5.ico"

    wordmark.save(logo_path, optimize=True)
    wordmark_white.save(logo_white_path, optimize=True)
    icon_master.save(icon_path, optimize=True)
    save_ico(icon_master, ico_path)

    print(f"Wrote {logo_path} ({wordmark.size[0]}x{wordmark.size[1]})")
    print(f"Wrote {logo_white_path}")
    print(f"Wrote {icon_path}")
    print(f"Wrote {ico_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
