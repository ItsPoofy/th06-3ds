"""Generate the English shot type titles (select04) in the original text style.

Usage:
    python tools/prepare_shot_titles.py <popw12.ttf>

DF Popcorn W12 is the font of the original menu text and is not part of this
repository. Like the original titles, the letters are white with an outside
glow in each shot type's color. The glow reaches a little further.
"""
from pathlib import Path
import sys
import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

sys.path.insert(0, str(Path(__file__).resolve().parent/'converter'))
import menu_text_art as art

TITLES = [('Spirit Amulet "Homing Amulet"', '"Fantasy Seal"'),
          ('Dream Amulet "Persuasion Needle"', '"Evil-Sealing Circle"'),
          ('Magic Amulet "Magic Missile"', '"Stardust Reverie"'),
          ('Love Amulet "Illusion Laser"', '"Master Spark"')]
BLOCK, WIDTH = 48, 256
# Both lines use the largest size at which the longest line fits.
MAX_SIZE, MARGIN = 17.0, 8
LINE_TOPS = (5, 22)
# The glow's alpha by distance in pixels outside the white letters, and each
# title's glow color, as in the English patch's select04.
GLOW_DISTANCE = np.array([0, 1, 2, 3, 4, 5])
GLOW_ALPHA = np.array([255, 171, 67, 30, 8.5, 0])/255
GLOW_COLORS = np.array([[57, 82, 140], [93, 28, 67], [0, 74, 16], [93, 28, 67]])
# The glow reaches this much further than the patch's.
GLOW_SPREAD = 1.3


def style(glyph, color):
    """White letters with an outside glow in color. Returns an RGBA image."""
    height, width = glyph.shape[0]//art.SS, glyph.shape[1]//art.SS
    fill = art.coverage(glyph)
    distance = (ndimage.distance_transform_edt(~glyph)/art.SS)[art.SS//2::art.SS, art.SS//2::art.SS]/GLOW_SPREAD
    glow = np.interp(distance, GLOW_DISTANCE, GLOW_ALPHA)*(1 - fill)
    alpha = fill + glow
    rgb = (255*fill[..., None] + color*glow[..., None])/np.maximum(alpha[..., None], 1e-4)
    def channel(v): return np.clip(np.round(v), 0, 255).astype(np.uint8)
    return Image.fromarray(np.dstack([channel(rgb), channel(alpha*255)]), 'RGBA')


def prepare(resources, pop):
    resources = Path(resources)
    size = MAX_SIZE
    while max(art.ink_width(line, pop, size)[0] for pair in TITLES for line in pair) > WIDTH - 2*MARGIN:
        size -= .5
    overlay = Image.new('RGBA', (256, 256))
    mask = Image.new('L', overlay.size)
    for index, lines in enumerate(TITLES):
        top = index*BLOCK
        glyph = np.zeros((BLOCK*art.SS, WIDTH*art.SS), bool)
        for line, y in zip(lines, LINE_TOPS):
            ink, left = art.ink_width(line, pop, size)
            glyph |= art.glyphs(line, pop, size, (WIDTH - ink)/2 - left, y, WIDTH, BLOCK)
        overlay.paste(style(glyph, GLOW_COLORS[index]), (0, top))
        ImageDraw.Draw(mask).rectangle((0, top, WIDTH - 1, top + BLOCK - 1), fill=255)
    overlay.save(resources/'select04.anm.bin.overlay.png')
    mask.save(resources/'select04.anm.bin.mask.png')
    print('size', size)


if __name__ == '__main__':
    prepare(Path(__file__).resolve().parent/'converter/resources', sys.argv[1])
