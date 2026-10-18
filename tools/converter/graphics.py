"""Build native overlays from user-supplied game art and port UI resources."""
import json
from pathlib import Path
import struct
import numpy as np
from PIL import Image, ImageOps
from prepare_assets import write_texture
from score_digits import regular_score_digits
from ui_animation_assets import animation


def decode_texture(path):
    raw = path.read_bytes()
    _, w, h, pw, ph = struct.unpack_from('<4s4I', raw)
    tiled = np.frombuffer(raw, dtype=np.uint8, offset=20).reshape(ph//8, pw//8, 64, 4)
    indices = [sum(((x>>b)&1)<<(2*b) | ((y>>b)&1)<<(2*b+1) for b in range(3))
               for y in range(8) for x in range(8)]
    pixels = tiled[:, :, indices].reshape(ph//8, pw//8, 8, 8, 4)
    pixels = pixels.transpose(0, 2, 1, 3, 4).reshape(ph, pw, 4)
    return Image.fromarray(pixels[:h, :w, ::-1])


def sprite(root, name, index):
    _, entries, sheet = animation(root, name)
    _, x, y, w, h = next(s for s in entries if s[0] == index)
    return sheet.crop((round(x), round(y), round(x+w), round(y+h)))


def card(root):
    original = sprite(root, 'select01.anm', 0)
    out = Image.new('RGBA', (280, 48), original.getpixel((128, 24)))
    sx = (0, 12, original.width-12, original.width)
    sy = (0, 12, original.height-12, original.height)
    tx, ty = (0, 12, 268, 280), (0, 12, 36, 48)
    for y in range(3):
        for x in range(3):
            if x == y == 1:
                continue
            piece = original.crop((sx[x], sy[y], sx[x+1], sy[y+1]))
            out.paste(piece.resize((tx[x+1]-tx[x], ty[y+1]-ty[y]), Image.Resampling.NEAREST), (tx[x], ty[y]))
    return out


def gameplay_material(sheet):
    material = sheet.convert('RGBA').crop((96, 80, 256, 96))
    material.putalpha(255)
    canvas = Image.new('RGBA', (400, 480))
    for y in range(0, 480, 16):
        for x in range(0, 400, 160):
            tile = ImageOps.mirror(material) if x//160 % 2 else material
            if y//16 % 2:
                tile = ImageOps.flip(tile)
            canvas.paste(tile, (x, y))
    return canvas


def selection_atlas(root, resources, lang):
    original = {'difficulty-card': card(root)}
    mapping = {'name-bar': 22, 'player-name-bar': 21, 'hud-high': 9,
               'hud-score': 8, 'hud-player': 10, 'hud-bomb': 11,
               'hud-power': 15, 'hud-graze': 16, 'hud-points': 20,
               'hud-life-star': 17, 'hud-bomb-star': 18, 'hud-max': 19}
    for name, index in mapping.items():
        original[name] = sprite(root, 'front.anm', index)
    for i in range(10):
        original['hud-digit'+str(i)] = sprite(root, 'ascii.anm', ord(str(i))-0x15)
    atlas = Image.open(resources/f'selection-{lang}.png').convert('RGBA')
    positions = json.loads((resources/f'selection-{lang}.json').read_text())
    for name, tile in original.items():
        if lang == 'en' and name.startswith('hud-') and name not in ('hud-life-star', 'hud-bomb-star') and not name.startswith('hud-digit'):
            continue
        x, y, w, h = positions[name]
        atlas.paste(tile.resize((w, h), Image.Resampling.NEAREST), (x, y))
    return atlas


def prepare(root, resources):
    dest = root / 'romfs'
    ui = dest / 'ui'
    ui.mkdir()
    for name in ('ui-font', 'practice-font', 'content-font'):
        write_texture(ui / (name+'.bin'), Image.open(resources/(name+'.png')))
    for lang in ('jp', 'en'):
        path = ui if lang == 'jp' else dest/'variants/en/ui'
        path.mkdir(parents=True, exist_ok=True)
        write_texture(path/'selection-atlas.bin', selection_atlas(root, resources, lang))
    for source in (resources/'backgrounds').glob('*.png'):
        image = Image.open(source).convert('RGBA')
        write_texture(ui/(source.stem+'.bin'), image)
        for suffix in ('.jpg', '.bmp'):
            raw = dest/'data'/(source.stem+suffix)
            if raw.exists():
                size = Image.open(raw).size
                center = image.crop((40, 0, 360, 240)).resize(size, Image.Resampling.NEAREST)
                write_texture(dest/'textures'/(raw.name+'.bin'), center)
    write_texture(ui/'gameplay-material.bin', gameplay_material(Image.open(dest/'data/front.png')))
    write_texture(ui/'gameplay-logo-atlas.bin', animation(root, 'front.anm')[2])
    # bundled_content installs fitted panels and clean native lower sheets;
    # the original sheets still have captions where we draw separate labels.
    localized = dest/'variants/en/textures'
    localized.mkdir(parents=True, exist_ok=True)
    for overlay in resources.glob('*.overlay.png'):
        name = overlay.name.removesuffix('.overlay.png')
        base = decode_texture(dest/'textures'/name)
        replacement = Image.open(overlay).convert('RGBA')
        mask = Image.open(resources/(name+'.mask.png'))
        base.paste(replacement, (0, 0), mask)
        write_texture(localized/name, regular_score_digits(name.removesuffix('.bin'), base))
