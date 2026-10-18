"""Rebuild English character titles without reducing their font size."""
import argparse
import json
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


def title(lines, width, size, color, font_path, rows=None):
    scale = 3
    font = ImageFont.truetype(str(font_path), size * scale)
    height = size * (rows or len(lines)) + 4
    image = Image.new('RGBA', (width * scale, height * scale))
    draw = ImageDraw.Draw(image)
    for row, line in enumerate(lines):
        box = font.getbbox(line)
        if box[2] - box[0] > image.width - 12:
            raise ValueError(f'Title line is too wide: {line}')
        x = (image.width - box[2] + box[0]) // 2 - box[0]
        y = (row * size + 2 + (height - 4 - size * len(lines)) / 2) * scale - box[1]
        draw.text((x, y), line, font=font, fill='white', stroke_width=5, stroke_fill=color)
    return image.resize((width, height), Image.Resampling.LANCZOS)


def center_statistics(atlas, positions):
    for character in range(2):
        x, y, w, h = positions[f'character-top-specs{character}']
        tile = atlas.crop((x, y, x+w, y+h))
        bounds = tile.getbbox()
        nx, ny, nw, nh = positions[f'character-top-name{character}']
        name_bounds = atlas.crop((nx, ny, nx+nw, ny+nh)).getbbox()
        if bounds and name_bounds:
            offset = round((name_bounds[0] + name_bounds[2] - nw - bounds[0] - bounds[2] + w) / 2)
            centered = Image.new('RGBA', tile.size)
            centered.alpha_composite(tile, (offset, 0))
            atlas.paste(centered, (x, y))


def prepare(repo, font_path):
    resources = repo / 'tools/converter/resources'
    atlas = Image.open(resources / 'selection-en.png').convert('RGBA')
    positions = json.loads((resources / 'selection-en.json').read_text())
    center_statistics(atlas, positions)
    japanese = Image.open(resources / 'selection-jp.png').convert('RGBA')
    center_statistics(japanese, json.loads((resources / 'selection-jp.json').read_text()))
    japanese.save(resources / 'selection-jp.png')
    tiles = []
    for name, (x, y, w, h) in positions.items():
        tile = atlas.crop((x, y, x+w, y+h))
        if name.startswith('character-') and '-title' in name:
            character = int(name[-1])
            lines = ['Shrine Maiden of', 'Hakurei Shrine'] if character == 0 else ['Western Magician', 'of the East']
            tile = title(lines, w, 14 if 'bottom' in name else 16,
                         (157, 6, 155) if character == 0 else (157, 6, 6), font_path,
                         rows=2)
        tiles.append((name, tile))
    # Shelf-pack the existing labels along with the taller title canvases.
    output = Image.new('RGBA', (512, 1024))
    # Native HUD slots are intentionally blank until conversion inserts the
    # user's original sprites. Identical pixels do not make slots interchangeable.
    metadata = {}; shelves = []
    for name, tile in sorted(tiles, key=lambda item: (-item[1].height, -item[1].width)):
        shelf = next((s for s in shelves if tile.height <= s[2] and s[0] + tile.width + 1 <= output.width), None)
        if shelf is None:
            shelf = [1, 1 if not shelves else shelves[-1][1] + shelves[-1][2] + 1, tile.height]
            shelves.append(shelf)
        x, y = shelf[:2]
        if y + tile.height + 1 > output.height:
            raise ValueError('Selection atlas exceeds GPU dimensions')
        output.alpha_composite(tile, (x, y))
        metadata[name] = [x, y, tile.width, tile.height]
        shelf[0] += tile.width + 1
    output.save(resources / 'selection-en.png')
    (resources / 'selection-en.json').write_text(json.dumps(metadata), encoding='utf-8')
    header = ['#pragma once', '#include "selection_labels.hpp"',
              'namespace CtrSelectionEnglishAssets {', 'inline constexpr CtrSelectionAssets::Label labels[]={']
    header += ['{' + json.dumps(name) + ',' + ','.join(map(str, rect)) + '},' for name, rect in metadata.items()]
    (repo / 'assets/selection_en_labels.hpp').write_text('\n'.join(header + ['};', '}']) + '\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--font', type=Path, required=True)
    args = parser.parse_args()
    prepare(Path(__file__).resolve().parents[1], args.font)
