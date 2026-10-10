"""Prepare original PBG3 assets and tiled PICA textures before packaging."""
from pathlib import Path
import io
import struct
import numpy as np
from PIL import Image
from stage_backgrounds import complete_surface
from score_digits import regular_score_digits


class Bits:
    def __init__(self, data, offset=0):
        self.data, self.pos = data, offset * 8

    def read(self, count):
        value = 0
        for _ in range(count):
            value = (value << 1) | ((self.data[self.pos // 8] >> (7 - self.pos % 8)) & 1)
            self.pos += 1
        return value

    def integer(self):
        return self.read((self.read(2) + 1) * 8)


def unpack(path):
    data = path.read_bytes()
    assert data[:4] == b'PBG3'
    bits = Bits(data, 4)
    count, table = bits.integer(), bits.integer()
    bits = Bits(data, table)
    entries = []
    for _ in range(count):
        fields = [bits.integer() for _ in range(5)]
        name = bytearray()
        while (byte := bits.read(8)):
            name.append(byte)
        entries.append((name.decode('ascii'), fields))
    result = {}
    for index, (name, (_, _, checksum, offset, size)) in enumerate(entries):
        bits = Bits(data, offset)
        window = bytearray(8192)
        head, output = 1, bytearray()
        # Some replacement archives end at the declared size without the
        # original encoder's zero-distance terminator.
        while len(output) < size:
            if bits.read(1):
                values = [bits.read(8)]
            else:
                start = bits.read(13)
                if not start:
                    break
                length = bits.read(4) + 3
                values = None
            if values is not None:
                for value in values:
                    output.append(value)
                    window[head] = value
                    head = (head + 1) & 8191
            else:
                for i in range(length):
                    value = window[(start + i) & 8191]
                    output.append(value)
                    window[head] = value
                    head = (head + 1) & 8191
        assert len(output) == size, name
        end = entries[index + 1][1][3] if index + 1 < len(entries) else table
        assert sum(data[offset:end]) & 0xffffffff == checksum, name
        result[name] = bytes(output)
    return result


def write_texture(path, image):
    w, h = image.size
    pw, ph = max(8, 1 << (w - 1).bit_length()), max(8, 1 << (h - 1).bit_length())
    pixels = np.zeros((ph, pw, 4), dtype=np.uint8)
    pixels[:h, :w] = np.asarray(image.convert('RGBA'))[:, :, ::-1]
    inverse = np.empty(64, dtype=np.int32)
    for y in range(8):
        for x in range(8):
            m = sum(((x >> b) & 1) << (2 * b) | ((y >> b) & 1) << (2 * b + 1) for b in range(3))
            inverse[m] = y * 8 + x
    tiled = pixels.reshape(ph // 8, 8, pw // 8, 8, 4).transpose(0, 2, 1, 3, 4).reshape(-1, 64, 4)[:, inverse]
    path.write_bytes(struct.pack('<4s4I', b'TX3D', w, h, pw, ph) + tiled.tobytes())


def prepare(source, root=None):
    root = root or source
    root.mkdir(parents=True, exist_ok=True)
    assets = {}
    for archive in sorted(p for p in source.iterdir() if p.suffix.lower() == '.dat'):
        assets.update(unpack(archive))
    if not assets:
        raise ValueError('The selected folder contains no TH06 PBG3 archives.')
    raw, textures = root / 'data', root / 'textures'
    raw.mkdir(exist_ok=True)
    textures.mkdir(exist_ok=True)
    images = {}
    for name, data in assets.items():
        (raw / name).write_bytes(data)
        if name.lower().endswith(('.bmp', '.png', '.jpg')):
            images[name] = Image.open(io.BytesIO(data)).convert('RGBA')
            write_texture(textures / (name + '.bin'), images[name])
    count = 0
    for name, data in assets.items():
        if not name.endswith('.anm'):
            continue
        fields = struct.unpack_from('<16I', data)
        w, h, fmt, name_offset, alpha_offset = fields[3], fields[4], fields[5], fields[7], fields[9]
        image_name = data[name_offset:].split(b'\0', 1)[0].decode('ascii')
        if image_name.startswith('@'):
            print(name, 'dynamic', w, h)
            continue
        image_name = image_name.replace('\\', '/').rsplit('/', 1)[-1]
        image = images[image_name].resize((w, h), Image.Resampling.NEAREST)
        # Preserve the original texture format's quantization and opacity.
        a = np.array(image)
        if fmt in (3, 4):
            a[:, :, 3] = 255
        if fmt in (2, 3):
            for channel, levels in enumerate((31, 63 if fmt == 3 else 31, 31)):
                a[:, :, channel] = ((a[:, :, channel].astype(np.uint16) >> (2 if levels == 63 else 3)) * 255 // levels)
            if fmt == 2:
                a[:, :, 3] = (a[:, :, 3] >= 128) * 255
        if fmt == 5:
            a = (a >> 4) * 17
        if alpha_offset:
            alpha_name = data[alpha_offset:].split(b'\0', 1)[0].decode('ascii')
            alpha_name = alpha_name.replace('\\', '/').rsplit('/', 1)[-1]
            alpha = np.asarray(images[alpha_name].resize((w, h), Image.Resampling.NEAREST))[:, :, 0]
            if fmt == 2:
                alpha = (alpha >= 128) * 255
            elif fmt == 5:
                alpha = (alpha >> 4) * 17
            a[:, :, 3] = alpha
        write_texture(textures / (name + '.bin'), regular_score_digits(name, complete_surface(name, Image.fromarray(a))))
        count += 1
    print(f'Prepared {len(assets)} files, {len(images)} surfaces, {count} animation textures')
    for name, data in assets.items():
        if name.endswith('.std'):
            paths = [data[656 + i * 128:784 + i * 128].split(b'\0', 1)[0] for i in range(4)]
            print(name, paths)


if __name__ == '__main__':
    import sys
    prepare(Path(sys.argv[1]))
