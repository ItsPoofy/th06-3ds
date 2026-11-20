"""Prepare the proportional menu text atlases used by native practice menus.

Usage:
    python tools/prepare_menu_typeface.py <popw12.ttf> <title01.png>

popw12.ttf is DFPPOPCorn-W12, the font of the original menu text, and
title01.png is the original title sheet from a converted assets folder; neither
is part of this repository. Everything is drawn with render_ui_label, the
renderer of the menu labels and difficulty labels:
  - Glyphs in the menu label style at 18 px (2 px black stroke, the title's
    white-to-grey face and soft shadow). The face shading runs from the top of
    the capitals to the baseline for every glyph, so separately drawn letters
    shade as one line. Each glyph has two tiles, its shadow and stroke, and its
    face: a string draws every stroke first and then every face, so no letter's
    stroke covers its neighbor, as in a label.
  - The practice type labels as the difficulty labels are drawn (32 px, 1 px
    stroke) with Normal's blue face.
menu-typeface.png holds printable ASCII and the English type labels.
menu-typeface-ja.png holds every other character the practice sources use
(PracticeMenu.cpp and the practice catalog's Japanese names) and the Japanese
type labels. Both are described in assets/ui-font/menu_typeface.hpp.
"""
from pathlib import Path
import sys
from PIL import Image, ImageFont
import render_ui_label

ROOT = Path(__file__).resolve().parent.parent
MODE_WIDTH, MODE_HEIGHT = 400, 50
# Normal's face rows, sampled from its label in selection-en.png. Rendering
# "Normal" with them reproduces that label (mean difference 0.2 of 255).
NORMAL_PROFILE = [(255, 255, 255), (255, 255, 255), (250, 250, 250), (253, 253, 253), (255, 255, 255),
                  (252, 252, 252), (248, 249, 251), (239, 243, 251), (227, 237, 251), (214, 229, 251),
                  (202, 222, 252), (193, 219, 254), (169, 202, 251), (152, 192, 250), (138, 185, 254),
                  (120, 172, 251), (104, 163, 251), (89, 154, 250), (76, 147, 251), (64, 140, 251),
                  (54, 134, 251), (48, 130, 251), (48, 130, 250), (49, 132, 254), (47, 128, 247), (47, 128, 247)]
GLYPH_STYLE = {'font_size': 18, 'stroke_width': 2, 'padding': 2}
SOURCES = ['src/platform/3ds/PracticeMenu.cpp', 'src/practice/Th06.cpp', 'src/practice/Th06CatalogJa.hpp']
SHEETS = {
    'en': {'size': (512, 512), 'modes': ('Regular Practice', 'Custom Practice'), 'name': 'menu-typeface.png'},
    'ja': {'size': (1024, 512), 'modes': ('通常練習', 'カスタム練習'), 'name': 'menu-typeface-ja.png'},
}


def build(chars, font_path, title01, sheet_size, modes):
    style = GLYPH_STYLE
    font = ImageFont.truetype(str(font_path), style['font_size'], layout_engine=ImageFont.Layout.BASIC)
    _, cap_top, _, baseline = font.getbbox('H')
    margin = style['padding']+3
    sheet = Image.new('RGBA', sheet_size)
    metrics = []
    x = y = row_height = 0
    for char in chars:
        advance = round(font.getlength(char)*64)
        if char == ' ':
            metrics.append((0, 0, 0, 0, 0, 0, advance, 0, 0))
            continue
        back, face = render_ui_label.render_label(char, font_path, title01, **style,
                                                  face_rows=(cap_top, baseline-1), layers=True)
        # Both layers keep the same crop: the shadow-and-stroke tile's ink.
        crop = back.getbbox()
        back, face = back.crop(crop), face.crop(crop)
        box = font.getbbox(char, stroke_width=style['stroke_width'])
        places = []
        for tile in (back, face):
            if x+tile.width+1 > sheet.width:
                x, y, row_height = 0, y+row_height+1, 0
            places.append((x, y))
            sheet.alpha_composite(tile, (x, y))
            x += tile.width+1
            row_height = max(row_height, tile.height)
        metrics.append((*places[0], back.width, back.height, *places[1], advance,
                        box[0]-margin+crop[0], box[1]-margin+crop[1]))
    mode_top = y+row_height+2
    if mode_top+2*(MODE_HEIGHT+2) > sheet.height:
        raise ValueError('menu glyphs leave no room for the practice type labels')
    mode_font = ImageFont.truetype(str(font_path), 32*3, layout_engine=ImageFont.Layout.BASIC)
    reference_box = mode_font.getbbox('Normal', stroke_width=3)
    reference_face = render_ui_label.mask_for('Normal', mode_font, 3)[1].getbbox()
    # Share Normal's gradient range. Descenders retain its final blue row.
    mode_face_rows = ((reference_box[1]+reference_face[1])/3,
                      (reference_box[1]+reference_face[3]-1)/3)
    for index, text in enumerate(modes):
        tile, _, _ = render_ui_label.render_label(text, font_path, title01, 32, 1, 2,
                                                 profile=NORMAL_PROFILE, face_rows=mode_face_rows)
        if tile.width > MODE_WIDTH:
            raise ValueError(f'{text} is wider than its cell')
        # As in the difficulty labels' cells: centered, the ink 6 px lower than drawn.
        sheet.alpha_composite(tile, ((MODE_WIDTH-tile.width)//2, mode_top+(MODE_HEIGHT+2)*index+6))
    return sheet, metrics, mode_top


def prepare(font_path, title01):
    japanese = sorted({c for source in SOURCES for c in (ROOT/source).read_text(encoding='utf-8') if ord(c) > 126})
    lines = ['#pragma once', '// Generated by tools/prepare_menu_typeface.py.', 'namespace CtrMenuTypefaceAssets {',
             f'inline constexpr float size={GLYPH_STYLE["font_size"]};',
             f'inline constexpr int modeWidth={MODE_WIDTH},modeHeight={MODE_HEIGHT};',
             '// The shadow-and-stroke tile at x,y and the face tile at faceX,faceY, both w by h.',
             'struct Glyph {int x,y,w,h,faceX,faceY,advance;float left,top;};']
    for language, chars in (('en', [chr(c) for c in range(32, 127)]), ('ja', japanese)):
        settings = SHEETS[language]
        sheet, metrics, mode_top = build(chars, font_path, title01, settings['size'], settings['modes'])
        sheet.save(ROOT/'tools/converter/resources'/settings['name'])
        prefix = '' if language == 'en' else 'ja'
        lines.append(f'inline constexpr int {prefix}{"modeTop" if not prefix else "ModeTop"}={mode_top};')
        if prefix:
            lines.append('// Code points of the Japanese glyphs, ascending, and their glyphs.')
            lines.append('inline constexpr unsigned jaCodes[]={' + ','.join(str(ord(c)) for c in chars) + '};')
        lines.append(f'inline constexpr Glyph {prefix + "Glyphs" if prefix else "glyphs"}[]={{')
        lines += ['    {%d,%d,%d,%d,%d,%d,%d,%.2ff,%.2ff},' % metric for metric in metrics]
        lines.append('};')
    lines.append('}')
    (ROOT/'assets/ui-font/menu_typeface.hpp').write_text('\n'.join(lines)+'\n', encoding='utf-8')
    print(len(japanese), 'Japanese glyphs')


if __name__ == '__main__':
    prepare(Path(sys.argv[1]), Path(sys.argv[2]))
