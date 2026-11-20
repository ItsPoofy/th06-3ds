"""Render the practice types' descriptions as the difficulty descriptions are drawn.

Usage:
    python tools/render_practice_description.py <popw12.ttf>

The difficulty descriptions ("For those who struggle with shooters." and the
rest) were drawn by caption() at 20 px in a 400x84 cell, white with a black
outline. This draws the two practice types' descriptions the same way in
English and Japanese, trims each to its ink, and adds them to practice-font.png
(widened to 512x512 if needed) and assets/ui-font/practice_labels.hpp as
practice-description<type>-native and -native-en. Existing labels are replaced.
"""
from pathlib import Path
import re
import sys
from PIL import Image
from render_practice_prompt import caption, entries, free_spot, HEADER, SHEET

DESCRIPTIONS = [
    ("That stage won't clear itself!\n(Scores recorded)", 'ステージを通して練習する方向けです\n（スコア記録あり）'),
    ('Got a grudge against a certain spell?\n(Scores not recorded)', '苦手な場面を練習する方向けです\n（スコア記録なし）'),
]


def main(font):
    sheet = Image.open(SHEET).convert('RGBA')
    if sheet.height < 512:
        grown = Image.new('RGBA', (sheet.width, 512))
        grown.alpha_composite(sheet)
        sheet = grown
    header = HEADER.read_text(encoding='utf-8')
    header = re.sub(r'\{"practice-description[^\n]*\n', '', header)
    known = {k: v for k, v in entries().items() if not k.startswith('practice-description')}
    for name, (x, y, w, h) in entries().items():
        if name.startswith('practice-description'):
            sheet.paste((0, 0, 0, 0), (x, y, x+w, y+h))
    taken = list(known.values())
    lines = []
    for index, (english, japanese) in enumerate(DESCRIPTIONS):
        for name, text in ((f'practice-description{index}-native-en', english), (f'practice-description{index}-native', japanese)):
            image = caption(text, 20, (400, 84), 'black', font, max_lines=2)
            image = image.crop(image.getbbox())
            x, y = free_spot(sheet, image.width, image.height, taken)
            taken.append((x, y, image.width, image.height))
            sheet.alpha_composite(image, (x, y))
            lines.append(f'{{"{name}",{x},{y},{image.width},{image.height},0,{image.height-1}}},')
    sheet.save(SHEET)
    end = header.index('};', header.index('labels[]'))
    HEADER.write_text(header[:end]+'\n'.join(lines)+'\n'+header[end:], encoding='utf-8')
    print('\n'.join(lines))


if __name__ == '__main__':
    main(Path(sys.argv[1]))
