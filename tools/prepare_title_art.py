"""Generate English title overlays in the style of the original menu titles.

The original titles use DF Popcorn W12 (DFPPOPCorn-W12). The font is not part
of this repository. Pass the path to popw12.ttf:

    python tools/prepare_title_art.py path/to/popw12.ttf

The style values below were measured from the 恋々音樂館 sprite in music00.
Rendered in this style at 48 px, that title matches the original glyph
positions and colors.
"""
from pathlib import Path
import sys
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy import ndimage

SS = 4
SIZE = 48
TOP = 9.5
OUTLINE = 1.25
OUTLINE_COLOR = np.array([3, 7, 7])
# Red keeps 255. Green and blue rise from this value at the top of the ink to
# 255 at GRADIENT of the ink height, and the rest of the glyph is white.
RED_GB = 51
GRADIENT = .4
# Teal glow by distance in pixels outside the outline.
GLOW_DISTANCE = np.array([0, .5, 1, 2, 3, 4, 4.6])
GLOW_ALPHA = np.array([235, 215, 155, 76, 40, 14, 0]) / 255
GLOW_RGB = np.array([[34, 85, 85], [34, 85, 85], [34, 85, 85], [34, 51, 68],
                     [17, 34, 34], [0, 17, 17], [0, 17, 17]])


def glyphs(text, font_path, width, height, max_width, left=None):
    """Return a supersampled glyph mask in width x height.

    The line is centered, or starts at left when given. Text wider than
    max_width is condensed horizontally to fit.
    """
    font = ImageFont.truetype(str(font_path), SIZE*SS)
    line = Image.new('L', (int(font.getlength(text)) + 32*SS, height*SS))
    # The original title draws the 48 px line at y = 9.5 in its 64 px sprite.
    ImageDraw.Draw(line).text((8*SS, round(TOP*SS)), text, font=font, fill=255)
    box = line.getbbox()
    line = line.crop((box[0], 0, box[2], line.height))
    if line.width > max_width*SS:
        line = line.resize((max_width*SS, line.height), Image.Resampling.LANCZOS)
    mask = Image.new('L', (width*SS, height*SS))
    x = (width*SS - line.width)//2 if left is None else round(left*SS)
    mask.paste(line, (x, 0))
    return np.asarray(mask) > 127


def styled(glyph):
    """Apply the fill gradient, outline and glow, then reduce to 4-bit channels."""
    height, width = glyph.shape[0]//SS, glyph.shape[1]//SS
    radius = round(OUTLINE*SS)
    y, x = np.mgrid[-radius:radius + 1, -radius:radius + 1]
    outer = ndimage.binary_dilation(glyph, structure=x*x + y*y <= radius*radius)
    def down(mask): return mask.reshape(height, SS, width, SS).mean((1, 3))
    glyph_cover, outer_cover = down(glyph), down(outer)
    rows = np.nonzero(glyph.any(1))[0]
    top, bottom = rows.min()/SS, (rows.max() + 1)/SS
    t = np.clip((np.arange(height) + .5 - top)/(GRADIENT*(bottom - top)), 0, 1)
    gb = RED_GB + (255 - RED_GB)*t
    fill = np.stack([np.full(height, 255.), gb, gb], -1)[:, None, :]
    distance = (ndimage.distance_transform_edt(~outer)/SS)[SS//2::SS, SS//2::SS]
    glow_alpha = np.interp(distance, GLOW_DISTANCE, GLOW_ALPHA)
    glow_rgb = np.stack([np.interp(distance, GLOW_DISTANCE, GLOW_RGB[:, c]) for c in range(3)], -1)
    halo = (1 - outer_cover)*glow_alpha
    premultiplied = (glyph_cover[..., None]*fill + (outer_cover - glyph_cover)[..., None]*OUTLINE_COLOR
                     + halo[..., None]*glow_rgb)
    alpha = outer_cover + halo
    rgb = np.where(alpha[..., None] > 1e-4, premultiplied/np.maximum(alpha[..., None], 1e-4), 0)
    def quantize(v): return (np.clip(np.round(v/17), 0, 15)*17).astype(np.uint8)
    return Image.fromarray(np.dstack([quantize(rgb), quantize(alpha*255)]), 'RGBA')


def title(text, font_path, width, height, max_width, left=None):
    return styled(glyphs(text, font_path, width, height, max_width, left))


def half_width(image):
    """Average column pairs. The game draws these sprites at twice their width."""
    pixels = np.asarray(image).astype(float)
    alpha = pixels[..., 3:]/255
    premultiplied = (pixels[..., :3]*alpha).reshape(pixels.shape[0], -1, 2, 3).mean(2)
    alpha = alpha.reshape(pixels.shape[0], -1, 2, 1).mean(2)
    rgb = np.where(alpha > 1e-4, premultiplied/np.maximum(alpha, 1e-4), 0)
    def quantize(v): return (np.clip(np.round(v/17), 0, 15)*17).astype(np.uint8)
    return Image.fromarray(np.dstack([quantize(rgb), quantize(alpha*255)]), 'RGBA')


def prepare(resources, font_path):
    resources = Path(resources)
    overlay = Image.new('RGBA', (256, 256))
    mask = Image.new('L', overlay.size)
    # MusicRoom draws the English title at twice its width. The text starts 7 px
    # in, like 恋々音樂館.
    overlay.paste(half_width(title('Lovely Music Room', font_path, 512, 64, 500, left=7)), (0, 0))
    ImageDraw.Draw(mask).rectangle((0, 0, 255, 63), fill=255)
    overlay.save(resources/'music00.anm.bin.overlay.png')
    mask.save(resources/'music00.anm.bin.mask.png')


if __name__ == '__main__':
    prepare(Path(__file__).resolve().parent/'converter/resources', sys.argv[1])
