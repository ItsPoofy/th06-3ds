"""Shared TH06 translation and image-format conversion.

Inputs are supplied by the user; no game resources are included here.
"""
import io
import re
import struct
import textwrap
import numpy as np
from PIL import Image

def plain(text):
    return re.sub(r'<c\$([^$]*)\$[^>]*>', r'\1', text).replace('　', ' ').strip()

def message(data, patch):
    count = struct.unpack_from('<I', data)[0]
    scripts = []
    for index in range(count):
        pos = struct.unpack_from('<I', data, 4 + index * 4)[0]
        instructions = []
        while pos + 4 <= len(data):
            time, op, size = struct.unpack_from('<HBB', data, pos)
            args = data[pos + 4:pos + 4 + size]
            instructions.append((time, op, args))
            pos += 4 + size
            if op == 0:
                break
        translations = patch.get(str(index), {})
        counters = {}
        skip_continuation = False
        result = bytearray()

        def emit(time, op, args):
            if len(args) > 255:
                raise ValueError('Message instruction exceeds its byte-sized payload')
            result.extend(struct.pack('<HBB', time, op, len(args)) + args)

        for time, op, args in instructions:
            if op in (3, 8):
                color, line = struct.unpack_from('<hh', args)
                kind = '' if op == 3 else f'h{color}_'
                counter_key = (time, kind)
                number = counters.get(counter_key, 0)
                counters[counter_key] = number + 1
                key = f'{time}_{kind}{number}'
                translated = translations.get(key)
                if translated:
                    lines = [plain(s) for s in translated['lines']]
                    if op == 3:
                        # Keep readable type. Long English paragraphs get extra
                        # two-line pages using the same immediate confirm logic.
                        lines = textwrap.wrap(' '.join(lines), width=44,
                                              break_long_words=False, break_on_hyphens=False) or [' ']
                        for n, text in enumerate(lines):
                            if n and n % 2 == 0:
                                emit(time, 4, struct.pack('<i', 300))
                            emit(time, op, struct.pack('<hh', color, n % 2) + text.encode('utf-8') + b'\0')
                        skip_continuation = True
                    else:
                        for n, text in enumerate(lines):
                            emit(time, op, struct.pack('<hh', color, n) + text.encode('utf-8') + b'\0')
                        skip_continuation = True
                    continue
                if skip_continuation and line == 1:
                    continue
                skip_continuation = False
            if op == 4:
                skip_continuation = False
            emit(time, op, args)
        scripts.append(result)
    offset = 4 + count * 4
    offsets = []
    for script in scripts:
        offsets.append(offset)
        offset += len(script)
    return struct.pack('<I', count) + struct.pack(f'<{count}I', *offsets) + b''.join(scripts)

def ending(data, patch):
    lines = data.splitlines(keepends=True)
    result = []
    index = 0
    while index < len(lines):
        original = lines[index]
        entry = patch.get(str(index))
        if entry and not original.startswith(b'@'):
            text = ' '.join(plain(s) for s in entry['lines'])
            # The original parser displays two lines per page. Reflow all
            # words into those pages, leaving its control instructions intact.
            wrapped = textwrap.wrap(text, width=62, break_long_words=False,
                                    break_on_hyphens=False) or [' ']
            for n, line in enumerate(wrapped):
                if n and n % 2 == 0:
                    result.append(b'@w1200\0' + b'4\0\n')
                result.append(line.encode('utf-8') + b'\0\n')
            index += 1
            while index < len(lines) and not lines[index].startswith(b'@') and str(index) not in patch:
                index += 1
        else:
            result.append(original)
            index += 1
    return b''.join(result)

def image_texture(assets, name):
    from stage_backgrounds import complete_surface
    from score_digits import regular_score_digits
    data = assets[name]
    fields = struct.unpack_from('<16I', data)
    width, height, fmt, offset, alpha_offset = fields[3], fields[4], fields[5], fields[7], fields[9]
    source = data[offset:].split(b'\0', 1)[0].decode('ascii').replace('\\', '/').rsplit('/', 1)[-1]
    if source.startswith('@'):
        return None
    image = Image.open(io.BytesIO(assets[source])).convert('RGBA').resize((width, height), Image.Resampling.NEAREST)
    if alpha_offset:
        alpha_name = data[alpha_offset:].split(b'\0', 1)[0].decode('ascii').replace('\\', '/').rsplit('/', 1)[-1]
        image.putalpha(Image.open(io.BytesIO(assets[alpha_name])).convert('RGB').getchannel('R').resize(image.size, Image.Resampling.NEAREST))
    elif fmt in (3, 4):
        image.putalpha(255)
    if fmt == 5:
        image = Image.fromarray((np.asarray(image) >> 4) * 17)
    return regular_score_digits(name, complete_surface(name, image))
