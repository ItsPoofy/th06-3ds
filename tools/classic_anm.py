"""Keep Steam's changed sprite atlases and scripts compatible with the port.

The engine and bundled cosmetic sheets use the original ANM slot/UV layout.
Steam moved menu slots, padded the bullet atlas, and inserted a credits row.
Retarget those sheets by sprite ID while retaining the user's image pixels.
"""
import io
import struct

from PIL import Image


def sprites(data):
    count, = struct.unpack_from('<I', data)
    return {
        sprite[0]: sprite[1:]
        for index in range(count)
        for offset in [struct.unpack_from('<I', data, 64 + index * 4)[0]]
        for sprite in [struct.unpack_from('<I4f', data, offset)]
    }


def normalize(assets, resources):
    for template in sorted((resources/'classic-compatibility').glob('*.anm')):
        name = template.name
        if name not in assets:
            raise ValueError(f'Steam Classic installation is missing {name}.')
        original = bytearray(template.read_bytes())
        steam = assets[name]
        target_sprites, source_sprites = sprites(original), sprites(steam)
        width, height = struct.unpack_from('<II', original, 12)
        sheet_name = template.stem + '.png'
        sheet = Image.open(io.BytesIO(assets[sheet_name])).convert('RGBA')
        result = Image.new('RGBA', (width, height))
        # Removed legacy PC settings are drawn by the port's native menu.
        # Keep their old pixels where available; all visible Steam sprites are
        # copied to their original slots below before localization is applied.
        result.paste(sheet.crop((0, 0, width, height)), (0, 0))
        for sprite_id, (x, y, w, h) in target_sprites.items():
            source_id = sprite_id
            if template.stem.startswith('title01'):
                if sprite_id in (6, 21, 22):
                    continue  # Digit 6 stayed put; Graphic/32Bits were removed.
                if sprite_id >= 23:
                    source_id -= 2
            elif template.stem == 'staff01' and sprite_id == 6:
                source_id = 7  # Program support follows Steam's new credits row.
            source = source_sprites.get(source_id)
            if source is None:
                raise ValueError(f'{name}: Missing Steam sprite {source_id}.')
            sx, sy, sw, sh = source
            tile = sheet.crop((round(sx), round(sy), round(sx + sw), round(sy + sh)))
            result.paste(tile.resize((round(w), round(h)), Image.Resampling.NEAREST), (round(x), round(y)))
        output = io.BytesIO()
        result.save(output, format='PNG')
        assets[sheet_name] = output.getvalue()
        # DDS supplied alpha in the color sheet, rather than a second image.
        struct.pack_into('<I', original, 36, 0)
        assets[name] = bytes(original)
