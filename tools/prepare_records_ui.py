"""Generate the records menu's English overlay in the original text style.

Usage:
    python tools/prepare_records_ui.py <converted assets> <popw12.ttf>

<converted assets> is a converter output folder with the original textures,
such as the one Build.ps1 uses. DF Popcorn W12 is the font of the original
menu text and is not part of this repository.

result01 (Easy, Normal, Hard, Lunatic, Extra, Spell Card) is English in the
original, so English uses that sheet unchanged.
"""
from pathlib import Path
import sys
import math
import numpy as np
from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parent/'converter'))
import menu_text_art as art
from graphics import decode_texture
import render_ui_label

ROWS = ['View Easy Records', 'View Normal Records', 'View Hard Records',
        'View Lunatic Records', 'View Extra Records']
ROW_SIZES = np.arange(18, 26, .5)


def sprite(sheet, x, y, width, height):
    return sheet.crop((x, y, x + width, y + height))


def heading(original, pop):
    """Draw "Dreamy Records" and its subtitle in the style of 夢の記録 and its subtitle."""
    width, height = original.size
    rows, cols = np.mgrid[:height, :width]
    # The subtitle covers the lower right. The rest is 夢の記録.
    subtitle_area = (rows >= 30) & (cols >= 26)
    title_style = art.measure(original, ignore=subtitle_area)
    geometry = art.fit(sprite(original, 0, 0, 160, 30), '夢の記録', pop, np.arange(30, 44, 1.0))
    # Shrink the size until the line fits the sprite.
    size = geometry['size']
    while art.ink_width('Dreamy Records', pop, size)[0] > width - 2*geometry['x'] - 2*geometry['outline']:
        size -= .5
    # The fill follows the sprite rows, so align the ink top with 夢の記録.
    heading_rows = np.nonzero(((np.asarray(original)[..., 3] >= art.SOLID) & ~subtitle_area).any(1))[0]
    probe = art.glyphs('Dreamy Records', pop, size, 0, 0, 512, 96)
    top = heading_rows.min() - np.nonzero(probe.any(1))[0].min()/art.SS
    glyph = art.glyphs('Dreamy Records', pop, size, geometry['x'], top, width, height)
    # 夢の記録 has heavier strokes than DF Popcorn's Latin. Thicken the glyphs
    # until their mean stroke width (twice area over edge length) matches.
    def stroke(mask):
        edge = mask & ~art.ndimage.binary_erosion(mask)
        return 2*mask.sum()/max(edge.sum(), 1)/art.SS
    heading_ink = np.kron((np.asarray(original)[..., 3] >= art.SOLID) & ~subtitle_area,
                          np.ones((art.SS, art.SS))).astype(bool)
    target = stroke(heading_ink)
    weight = 0.0
    while stroke(art.solid(glyph, weight + geometry['outline'] + .25)) <= target and weight < 3:
        weight += .25
    glyph = art.solid(glyph, weight)
    image = art.render(glyph, geometry['outline'], title_style)

    # The subtitle is English in the original and is drawn over 夢の記録.
    # Fit it on "Spell Card", which is clear of the heading, then copy the
    # original pixels wherever the fitted line fully covers them. Only the
    # glow around it is redrawn.
    text, tail = 'Result of Score and Spell Card', 'Spell Card'
    # A gap at x = 171-174 separates "and" from "Spell".
    spell = 173
    tail_geometry = art.fit(sprite(original, spell, 26, width - spell, height - 26), tail, pop,
                            np.arange(13, 19, .5), outlines=(.5, 1), tracks=(0, .5, 1, 1.5, 2))
    font = art.ImageFont.truetype(str(pop), round(tail_geometry['size']*art.SS))
    prefix = text[:-len(tail)]
    advance = (font.getlength(prefix) + len(prefix)*tail_geometry['track']*art.SS)/art.SS
    glyph = art.glyphs(text, pop, tail_geometry['size'], spell + tail_geometry['x'] - advance,
                       26 + tail_geometry['y'], width, height, track=tail_geometry['track'])
    subtitle_style = art.measure(original, ignore=~subtitle_area | (cols < 150))
    subtitle = art.render(glyph, tail_geometry['outline'], subtitle_style)
    covered = art.coverage(art.solid(glyph, tail_geometry['outline'])) >= 1
    pixels = np.asarray(subtitle).copy()
    pixels[covered] = np.asarray(original)[covered]
    image.alpha_composite(Image.fromarray(pixels, 'RGBA'))
    return image


# Records board labels: (english, japanese, hue). A hue of None keeps the blue
# style of the original rows. Hues match the difficulty select cards, red for
# Reimu and gold for Marisa.
REIMU, MARISA = 0, 48
BOARD_LABELS = {
    'difficulty0': ('Easy', 'Easy', 129), 'difficulty1': ('Normal', 'Normal', 217),
    'difficulty2': ('Hard', 'Hard', 300), 'difficulty3': ('Lunatic', 'Lunatic', 12),
    'difficulty4': ('Extra', 'Extra', 44),
    'character0': ('Reimu Hakurei', '博麗 霊夢', REIMU), 'character1': ('Marisa Kirisame', '霧雨 魔理沙', MARISA),
    'shot0': ('Spirit Sign', '霊符', REIMU), 'shot1': ('Dream Sign', '夢符', REIMU),
    'shot2': ('Magic Sign', '魔符', MARISA), 'shot3': ('Love Sign', '恋符', MARISA),
    'spellcards': ('Spell Cards', '御札戦歴', None), 'quit': ('Quit', 'Quit', None),
    'play': ('Play', '再生', None), 'back': ('Back', '戻る', None),
    'header0': ('Reimu - Spirit Sign', '霊夢 - 霊符', REIMU), 'header1': ('Reimu - Dream Sign', '霊夢 - 夢符', REIMU),
    'header2': ('Marisa - Magic Sign', '魔理沙 - 魔符', MARISA), 'header3': ('Marisa - Love Sign', '魔理沙 - 恋符', MARISA),
}
# Controls share one enlarged size. Header labels use a smaller common size.
BOTTOM_SCALE = 1.5
HEADER_SCALE = 1.25
ATLAS_WIDTH = 512


def board_label(text, pop, geometry, colors, title01, hue, scale):
    probe = np.nonzero(art.glyphs('EHN', pop, geometry['size'], 4, geometry['y'], 64, 32).any(1))[0]
    ink_top, ink_bottom = probe.min()/art.SS, (probe.max() + 1)/art.SS
    style = colors if hue is None else art.tint(colors, hue, ink_top, ink_bottom)
    profile = []
    for y in range(math.floor(ink_top), math.ceil(ink_bottom)):
        rgba = art.lookup(style, y, len(art.DISTANCES)-1, 1)
        profile.append(tuple(round(float(c/max(rgba[3], 1e-6))) for c in rgba[:3]))
    if hue is None:
        for i, color in enumerate(profile):
            blend = float(np.clip((i/max(1, len(profile)-1)-1/3)*6, 0, 1))
            profile[i] = tuple(round(255*(1-blend)+channel*blend) for channel in color)
    size = int(geometry['size']*scale)
    image, face_top, _ = render_ui_label.render_label(text, pop, title01, size, 2, 0, profile=profile)
    font = ImageFont.truetype(str(pop), size, layout_engine=ImageFont.Layout.BASIC)
    top = math.ceil(geometry['y']*scale + font.getbbox(text)[1] - face_top)
    tile = Image.new('RGBA', (image.width, round(32*scale)))
    tile.alpha_composite(image, (0, top))
    return tile


def section_caption(text, pop, title01):
    image, _, _ = render_ui_label.render_label(text, pop, title01, 22, 1, 0, profile=[(255, 255, 255)])
    font = ImageFont.truetype(str(pop), 22, layout_engine=ImageFont.Layout.BASIC)
    offset = font.getbbox(text, stroke_width=1)[1] - font.getbbox('DIFFICULTY', stroke_width=1)[1]
    if offset:
        aligned = Image.new('RGBA', (image.width, image.height+abs(offset)))
        aligned.alpha_composite(image, (0, max(0, offset)))
        return aligned
    return image


def board_atlas(resources, pop, geometry, colors, title01):
    """Write records-en.png, records-jp.png and assets/records_labels.hpp."""
    def label(text, hue, scale):
        return board_label(text, pop, geometry, colors, title01, hue, scale)
    entries = []
    for name, (english, japanese, hue) in BOARD_LABELS.items():
        scale = HEADER_SCALE if name.startswith('header') else BOTTOM_SCALE
        entries.append((name, label(english, hue, scale), label(japanese, hue, scale)))
        if name.startswith('difficulty'):
            entries.append(('small-' + name, label(english, hue, HEADER_SCALE), label(japanese, hue, HEADER_SCALE)))
    for name, text in (('caption-difficulty', 'DIFFICULTY'), ('caption-character', 'CHARACTER'),
                       ('caption-shot', 'SHOT TYPE'), ('caption-stage', 'STAGE')):
        image = section_caption(text, pop, title01)
        entries.append((name, image, image.copy()))
    # Shelf packing. Both languages share each slot, sized to the wider label.
    slots, x, y, shelf = [], 1, 1, 0
    for name, english, japanese in entries:
        width, height = max(english.width, japanese.width), english.height
        if x + width + 1 > ATLAS_WIDTH:
            x, y, shelf = 1, y + shelf + 2, 0
        slots.append((name, x, y, english, japanese))
        x, shelf = x + width + 2, max(shelf, height)
    height = 1 << (y + shelf + 1 - 1).bit_length()
    for lang, index in (('en', 3), ('jp', 4)):
        sheet = Image.new('RGBA', (ATLAS_WIDTH, height))
        for slot in slots:
            sheet.paste(slot[index], (slot[1], slot[2]))
        sheet.save(resources/f'records-{lang}.png')
    lines = ['#pragma once',
             '// Generated by tools/prepare_records_ui.py. Each label sits at (x, y) in',
             '// ui/records-atlas.bin with the given height, and is english_w or japanese_w wide.',
             'namespace CtrRecordsAssets {',
             'struct Label {const char* name;int x,y,h,english_w,japanese_w;};',
             'inline constexpr Label labels[]={']
    for name, x, y, english, japanese in slots:
        lines.append('{"%s",%d,%d,%d,%d,%d},' % (name, x, y, english.height, english.width, japanese.width))
    lines += ['};', '}', '']
    (Path(__file__).resolve().parent.parent/'assets/records_labels.hpp').write_text('\n'.join(lines), encoding='utf-8')


def board_style(assets, pop):
    original = decode_texture(Path(assets)/'textures/result00.anm.bin')
    rows = [sprite(original, 0, 48 + 32*index, 256, 32) for index in range(5)]
    colors = sum(art.measure(row) for row in rows)
    normal = np.asarray(rows[1])[..., 3] >= art.SOLID
    end = next(c for c in range(256) if normal[:, :c].any() and not normal[:, c:c + 3].any())
    geometry = art.fit(rows[1].crop((0, 0, end, 32)), 'Normal', pop, ROW_SIZES)
    return geometry, colors


def prepare(resources, assets, pop):
    resources, assets = Path(resources), Path(assets)
    original = decode_texture(assets/'textures/result00.anm.bin')
    overlay = Image.new('RGBA', (256, 256))
    mask = Image.new('L', overlay.size)
    def place(image, x, y):
        overlay.paste(image, (x, y))
        ImageDraw.Draw(mask).rectangle((x, y, x + image.width - 1, y + image.height - 1), fill=255)
    place(heading(sprite(original, 0, 0, 256, 48), pop), 0, 0)
    # The rows share one style. Pool their colors, and take the size and
    # baseline from the Latin "Normal", since the English rows are all Latin.
    geometry, colors = board_style(assets, pop)
    board_atlas(resources, pop, geometry, colors, assets/'data/title01.png')
    def row(text, width):
        glyph = art.glyphs(text, pop, geometry['size'], geometry['x'], geometry['y'], width, 32)
        return art.render(glyph, geometry['outline'], colors)
    for index, text in enumerate(ROWS):
        place(row(text, 256), 0, 48 + 32*index)
    place(row('View Spell Cards', 192), 0, 208)
    # Quit at (192, 208) is English in the original and stays unchanged.
    overlay.save(resources/'result00.anm.bin.overlay.png')
    mask.save(resources/'result00.anm.bin.mask.png')
    for name in ('result01.anm.bin.overlay.png', 'result01.anm.bin.mask.png'):
        (resources/name).unlink(missing_ok=True)


if __name__ == '__main__':
    prepare(Path(__file__).resolve().parent/'converter/resources', *sys.argv[1:3])
