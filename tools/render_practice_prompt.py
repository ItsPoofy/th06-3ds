"""Render selection prompts as the existing ones in practice-font.png were made.

Usage:
    python tools/render_practice_prompt.py <popw12.ttf> <key> "<English>" "<Japanese>" <r> <g> <b> [--check]

The prompts above the selection cards ("Choose your shot type" and the rest)
were drawn by the port's original asset scripts, restored here:
  - English: caption() at 18 px in a 290x24 cell, white letters with a 3 px
    stroke in the prompt's color at 3x, downsampled with Lanczos.
  - Japanese: white letters with a 1 px stroke in the prompt's color over a
    blurred glow of the same color, at 3x, downsampled and cropped to the ink.
<key> names the pair; the English label is <key>-en. --check renders the
difficulty prompt's two labels and reports their difference from the sheet.
Otherwise the two labels are added to practice-font.png's free space and to
assets/ui-font/practice_labels.hpp.
"""
from pathlib import Path
import argparse
import re
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parent.parent
SHEET = ROOT/'tools/converter/resources/practice-font.png'
HEADER = ROOT/'assets/ui-font/practice_labels.hpp'


def wrap(text, font, width, max_lines=2):
    while True:
        lines = []
        for paragraph in text.split('\n'):
            words = paragraph.split()
            while words:
                line = words.pop(0)
                while words and font.getlength(line + ' ' + words[0]) <= width:
                    line += ' ' + words.pop(0)
                lines.append(line)
        if len(lines) <= max_lines and all(font.getlength(line) <= width for line in lines):
            return font, lines
        if font.size <= 1:
            raise ValueError('Caption cannot fit its rectangle')
        font = ImageFont.truetype(font.path, font.size-1)


def caption(text, size, dimensions, outline, path, max_lines=1):
    scale = 3
    image = Image.new('RGBA', (dimensions[0]*scale, dimensions[1]*scale))
    font = ImageFont.truetype(str(path), size*scale)
    font, lines = wrap(text, font, image.width-18, max_lines)
    while sum(font.getbbox(line)[3]-font.getbbox(line)[1] for line in lines) + (len(lines)-1)*3 > image.height-9:
        font, lines = wrap(text, ImageFont.truetype(str(path), font.size-1), image.width-18, max_lines)
    heights = [font.getbbox(line)[3]-font.getbbox(line)[1] for line in lines]
    y = (image.height - (sum(heights) + (len(lines)-1)*3))//2
    draw = ImageDraw.Draw(image)
    for line, height in zip(lines, heights):
        box = font.getbbox(line)
        x = (image.width-(box[2]-box[0]))//2 - box[0]
        draw.text((x, y-box[1]), line, font=font, fill='white', stroke_width=5, stroke_fill=outline)
        y += height+3
    return image.resize(dimensions, Image.Resampling.LANCZOS)


def mask_for(text, font, stroke):
    box = font.getbbox(text, stroke_width=stroke)
    width, height = max(1, box[2]-box[0]), max(1, box[3]-box[1])
    outer = Image.new('L', (width, height))
    inner = Image.new('L', (width, height))
    ImageDraw.Draw(outer).text((-box[0], -box[1]), text, font=font, fill=255, stroke_width=stroke, stroke_fill=255)
    ImageDraw.Draw(inner).text((-box[0], -box[1]), text, font=font, fill=255)
    return outer, inner


def japanese(text, color, path):
    font = ImageFont.truetype(str(path), 18*3, layout_engine=ImageFont.Layout.BASIC)
    outer, inner = mask_for(text, font, 3)
    image = Image.new('RGBA', (outer.width+12, outer.height+12))
    glow = Image.new('L', image.size)
    glow.paste(outer, (6, 6))
    glow = glow.filter(ImageFilter.GaussianBlur(3)).point(lambda a: round(a*.7))
    halo = Image.new('RGBA', image.size, (*color, 0))
    halo.putalpha(glow)
    image = Image.alpha_composite(image, halo)
    stroke = Image.new('RGBA', outer.size, (*color, 0))
    stroke.putalpha(outer)
    face = Image.new('RGBA', inner.size, (255, 255, 255, 0))
    face.putalpha(inner)
    stroke = Image.alpha_composite(stroke, face)
    image.alpha_composite(stroke, (6, 6))
    image = image.resize((round(image.width/3), round(image.height/3)), Image.Resampling.LANCZOS)
    return image.crop(image.getbbox())


def entries():
    return {m.group(1): tuple(map(int, m.groups()[1:5])) for m in
            re.finditer(r'\{"([^"]+)",(\d+),(\d+),(\d+),(\d+)', HEADER.read_text(encoding='utf-8'))}


def free_spot(sheet, width, height, taken):
    """A spot clear of every existing label's cell, not only of its ink."""
    alpha = (np.asarray(sheet.getchannel('A')) > 0).copy()
    for x, y, w, h in taken:
        alpha[y:y+h, x:x+w] = True
    for y in range(1, sheet.height-height):
        for x in range(1, sheet.width-width):
            if not alpha[y-1:y+height+1, x-1:x+width+1].any():
                return x, y
    raise ValueError('practice-font.png has no room')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('font', type=Path)
    parser.add_argument('key')
    parser.add_argument('english')
    parser.add_argument('japanese')
    parser.add_argument('color', type=int, nargs=3)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    color = tuple(args.color)
    labels = {f'{args.key}-en': caption(args.english, 18, (290, 24), color, args.font),
              args.key: japanese(args.japanese, color, args.font)}
    sheet = Image.open(SHEET).convert('RGBA')
    if args.check:
        known = entries()
        for name, image in labels.items():
            x, y, w, h = known[name]
            original = np.asarray(sheet.crop((x, y, x+w, y+h))).astype(int)
            drawn = np.asarray(image).astype(int)
            if drawn.shape != original.shape:
                print(name, 'size', image.size, 'vs sheet', (w, h))
                continue
            print(name, f'mean difference {np.abs(drawn-original).mean():.2f}')
        return
    header = HEADER.read_text(encoding='utf-8')
    lines = []
    taken = list(entries().values())
    for name, image in labels.items():
        x, y = free_spot(sheet, image.width, image.height, taken)
        taken.append((x, y, image.width, image.height))
        sheet.alpha_composite(image, (x, y))
        # Prompts keep the whole label in its face rows, as the existing ones.
        lines.append(f'{{"{name}",{x},{y},{image.width},{image.height},0,{image.height-1}}},')
    sheet.save(SHEET)
    end = header.index('};', header.index('labels[]'))
    HEADER.write_text(header[:end]+'\n'.join(lines)+'\n'+header[end:], encoding='utf-8')
    print('\n'.join(lines))


if __name__ == '__main__':
    main()
