"""Render menu labels exactly as ui-font.png's were made, and add them to the sheet.

Usage:
    python tools/render_ui_label.py <DFPPOPCorn-W12 font> <title01.png> "<text>" <style> <x> <y> [--check "<existing label>"]

This is the label renderer the sheet was generated with (render_ui_font.py and
text_gradients.py from the original port work), restored here:
  - DFPPOPCorn-W12 (popw12.ttf), Pillow's basic layout.
  - Styles: menu and option are 22 px with a 2 px black stroke; row and button
    are 16 px with a 1 px stroke.
  - Drawn at 3x and downsampled once with Lanczos.
  - The face takes the original white-to-grey rows of "Start" in title01.png.
  - A soft black shadow, offset down and right by size/13 and blurred by
    size/26 at 65% opacity, sits outside the stroke with 2 + 3 px margins.
title01.png is the original title sheet in a converted assets folder
(Local Files/local/assets/pc/data/title01.png). --check renders an existing
label and reports how far it is from the sheet's pixels instead of adding one.
"""
from collections import Counter
from functools import lru_cache
from pathlib import Path
import argparse
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parent.parent
SHEET = ROOT/'tools/converter/resources/ui-font.png'
HEADER = ROOT/'assets/ui-font/ui_labels.hpp'
STYLES = {
    'menu': {'font_size': 22, 'stroke_width': 2, 'padding': 2},
    'option': {'font_size': 22, 'stroke_width': 2, 'padding': 2},
    'row': {'font_size': 16, 'stroke_width': 1, 'padding': 2},
    'button': {'font_size': 16, 'stroke_width': 1, 'padding': 2},
}


@lru_cache(maxsize=None)
def neutral_profile(title01):
    image = Image.open(title01).convert('RGB')
    values = []
    # Start's face occupies rows 5..22. Ignore its antialiased edge and shadow.
    for y in range(5, 23):
        colors = Counter(image.getpixel((x, y))[0] for x in range(72)
                         if len(set(image.getpixel((x, y)))) == 1 and image.getpixel((x, y))[0] >= 130)
        values.append(colors.most_common(1)[0][0])
    return [(255, 255, 255)] + [(255, 255, 255) if v >= 254 else (v, v, v) for v in values[1:]]


def apply_face_gradient(image, mask, profile, rows=None):
    """Color the face layer with the profile from its top row to its bottom row.

    rows gives other top and bottom rows, so separately drawn glyphs share one
    line's shading; rows outside them take the profile's end colors."""
    bounds = mask.getbbox()
    if not bounds:
        return image
    top, bottom = rows or (bounds[1], bounds[3] - 1)
    pixels = image.load()
    for y in range(bounds[1], bounds[3]):
        position = min(max((y-top)*(len(profile)-1)/max(1, bottom-top), 0), len(profile)-1)
        index = int(position)
        fraction = position-index
        color = tuple(round(profile[index][c]*(1-fraction)+profile[min(index+1, len(profile)-1)][c]*fraction)
                      for c in range(3))
        for x in range(bounds[0], bounds[2]):
            if mask.getpixel((x, y)):
                pixels[x, y] = color + (pixels[x, y][3],)
    return image


def mask_for(text, font, stroke):
    box = font.getbbox(text, stroke_width=stroke)
    width, height = max(1, box[2]-box[0]), max(1, box[3]-box[1])
    outer = Image.new('L', (width, height))
    inner = Image.new('L', (width, height))
    ImageDraw.Draw(outer).text((-box[0], -box[1]), text, font=font, fill=255, stroke_width=stroke, stroke_fill=255)
    ImageDraw.Draw(inner).text((-box[0], -box[1]), text, font=font, fill=255)
    return outer, inner


def render_label(text, font_path, title01, font_size, stroke_width, padding, profile=None, face_rows=None, layers=False):
    """The label image and its face's top and bottom rows. face_rows gives the
    shading's top and bottom in the font's own coordinates at native size.
    With layers, returns the shadow-and-stroke image and the face image instead."""
    font = ImageFont.truetype(str(font_path), font_size, layout_engine=ImageFont.Layout.BASIC)
    scale = 3
    native_box = font.getbbox(text, stroke_width=stroke_width)
    native_size = (max(1, native_box[2]-native_box[0]), max(1, native_box[3]-native_box[1]))
    face_bounds = mask_for(text, font, stroke_width)[1].getbbox()
    large = ImageFont.truetype(str(font_path), font_size*scale, layout_engine=ImageFont.Layout.BASIC)
    outer, inner = mask_for(text, large, stroke_width*scale)
    base = Image.new('RGBA', outer.size, (0, 0, 0, 0))
    base.putalpha(outer)
    gradient = Image.new('RGBA', inner.size)
    gradient.paste((255, 255, 255, 255), (0, 0, *gradient.size))
    gradient.putalpha(inner)
    rows = None
    if face_rows:
        large_box = large.getbbox(text, stroke_width=stroke_width*scale)
        rows = (face_rows[0]*scale-large_box[1], face_rows[1]*scale-large_box[1])
    gradient = apply_face_gradient(gradient, inner, profile or neutral_profile(str(title01)), rows)
    base = Image.alpha_composite(base, gradient)
    base = base.resize(native_size, Image.Resampling.LANCZOS)
    outer = outer.resize(native_size, Image.Resampling.LANCZOS)
    shadow_offset = max(1, round(font_size/13))
    shadow_blur = max(.5, font_size/26)
    margin = padding+3
    size = (base.width+margin*2+shadow_offset, base.height+margin*2+shadow_offset)
    mask = Image.new('L', size)
    mask.paste(outer, (margin+shadow_offset, margin+shadow_offset))
    mask = mask.filter(ImageFilter.GaussianBlur(shadow_blur)).point(lambda a: round(a*.65))
    image = Image.new('RGBA', size, (0, 0, 0, 0))
    image.putalpha(mask)
    if layers:
        # The shadow and black stroke, then the face alone, so a line drawn one
        # glyph at a time can put every stroke beneath every face as a label does.
        stroke = Image.new('RGBA', native_size, (0, 0, 0, 0))
        stroke.putalpha(outer)
        image.alpha_composite(stroke, (margin, margin))
        face = Image.new('RGBA', size, (0, 0, 0, 0))
        face.alpha_composite(gradient.resize(native_size, Image.Resampling.LANCZOS), (margin, margin))
        return image, face
    image.alpha_composite(base, (margin, margin))
    return image, margin+face_bounds[1], margin+face_bounds[3]-1


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('font', type=Path)
    parser.add_argument('title01', type=Path)
    parser.add_argument('text')
    parser.add_argument('style', choices=STYLES)
    parser.add_argument('x', type=int, nargs='?', default=0)
    parser.add_argument('y', type=int, nargs='?', default=0)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    image, face_top, face_bottom = render_label(args.text, args.font, args.title01, **STYLES[args.style])
    sheet = Image.open(SHEET).convert('RGBA')
    header = HEADER.read_text(encoding='utf-8')
    if args.check:
        import re
        x, y, w, h = map(int, re.search(r'\{"%s",(\d+),(\d+),(\d+),(\d+)' % re.escape(args.text), header).groups())
        original = np.asarray(sheet.crop((x, y, x+w, y+h))).astype(int)
        drawn = np.zeros_like(original)
        rendered = np.asarray(image).astype(int)[:h, :w]
        drawn[:rendered.shape[0], :rendered.shape[1]] = rendered
        print(f'size {image.size} vs sheet {(w, h)}; mean difference {np.abs(drawn-original).mean():.2f}, max {np.abs(drawn-original).max()}')
        return
    if sheet.crop((args.x, args.y, args.x+image.width, args.y+image.height)).getchannel('A').getbbox():
        raise ValueError(f'ui-font.png is not empty at {args.x},{args.y} {image.size}')
    sheet.alpha_composite(image, (args.x, args.y))
    sheet.save(SHEET)
    line = f'{{"{args.text}",{args.x},{args.y},{image.width},{image.height},{face_top},{face_bottom},"{args.style}"}},'
    anchor = '{"3DS Version 1.1"'
    HEADER.write_text(header.replace(anchor, line+'\n'+anchor, 1), encoding='utf-8')
    print(line)


if __name__ == '__main__':
    main()
