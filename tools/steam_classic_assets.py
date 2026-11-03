"""Normalize Classic's DDS sheets to the image paths used by the port.

DDS already contains alpha, so its ANMs have no separate alpha-image reference.
PNG preserves that alpha without introducing another lossy image conversion.
"""
import io
from pathlib import PurePosixPath
import re

from PIL import Image
from classic_anm import normalize as normalize_anm


def normalize(assets, resources):
    surfaces = {'title00', 'select00', 'music', 'result', 'th06logo', 'staff00'}
    renamed = {}
    result = {}
    for name, data in assets.items():
        # Steam's pre-rendered font caches are not used by the port's text renderer.
        if re.fullmatch(r'(?:advance_)?\d+_0x[\dA-Fa-f]+_0x[\dA-Fa-f]+\.(?:dds|json|txt)', name):
            continue
        path = PurePosixPath(name)
        if path.suffix.lower() == '.dds':
            surface = path.stem in surfaces or re.fullmatch(r'end\d\d[b]?', path.stem)
            target = str(path.with_suffix('.jpg' if surface else '.png'))
            image = Image.open(io.BytesIO(data)).convert('RGBA')
            output = io.BytesIO()
            # The logical JPG paths are legacy game identifiers. Store lossless
            # PNG bytes; both the converter and SDL detect the image signature.
            image.save(output, format='PNG')
            result[target] = output.getvalue()
            renamed[name.encode('ascii')] = target.encode('ascii')
        else:
            result[name] = data
    if renamed:
        for name, data in result.items():
            if name.endswith(('.anm', '.end')):
                # Extensions have the same byte length, preserving ANM offsets.
                for old, new in renamed.items():
                    data = data.replace(old + b'\0', new + b'\0')
                result[name] = data
        if not (resources/'classic-compatibility/etama3.anm').is_file():
            raise ValueError('Steam Classic compatibility definitions are missing from the converter.')
        normalize_anm(result, resources)
    return result
