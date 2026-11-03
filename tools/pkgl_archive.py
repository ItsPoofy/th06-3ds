"""Read the Steam Classic PKGL container (index and payload XOR, then Zstd).

Format reference: https://github.com/Renko6626/thdat-nc/blob/main/src/pkgl.h
"""
from pathlib import Path, PureWindowsPath
import struct
import zlib

import zstandard


def key_from_seed(seed):
    mask = (1 << 64) - 1

    def mix(value):
        value &= mask
        value = ((value ^ (value >> 30)) * 0xbf58476d1ce4e5b9) & mask
        value = ((value ^ (value >> 27)) * 0x94d049bb133111eb) & mask
        return value ^ (value >> 31)

    state = (seed * 0x9e3779b1 + 1) ^ (seed << 32)
    golden = 0x9e3779b97f4a7c15
    return struct.pack('<QQ', mix(state + golden), mix(state + 2 * golden))


def xor(data, seed):
    key = key_from_seed(seed)
    return bytes(value ^ key[index % 16] for index, value in enumerate(data))


def unpack(path):
    path = Path(path)
    data = path.read_bytes()
    member = None
    try:
        if len(data) < 8 or data[:4] != b'PKGL':
            raise ValueError('Not a Steam TH06 PKGL archive.')
        table_size, = struct.unpack_from('<I', data, 4)
        table_end = 8 + table_size
        if not table_size or table_end > len(data):
            raise ValueError('Invalid archive directory size.')
        table = xor(data[8:table_end], zlib.crc32(path.stem.encode('utf-8')))
        cursor, result = 0, {}
        decoder = zstandard.ZstdDecompressor()
        while cursor < len(table):
            member = None
            if len(table) - cursor < 32:
                raise ValueError('Truncated archive directory record; keep the original DAT filename.')
            flags, seed, size, stored_size, offset, name_size = struct.unpack_from('<HIQQQH', table, cursor)
            cursor += 32
            if not 0 < name_size <= 255 or cursor + name_size > len(table):
                raise ValueError('Invalid archive member name; keep the original DAT filename.')
            member = table[cursor:cursor + name_size].decode('utf-8')
            cursor += name_size
            name = PureWindowsPath(member)
            if name.drive or name.root or '..' in name.parts or '\0' in member or ':' in member:
                raise ValueError('Invalid archive member path.')
            if flags & ~1:
                raise ValueError(f'Unsupported archive member flags: {flags:#x}.')
            if offset < table_end or offset + stored_size > len(data):
                raise ValueError('Truncated archive member data.')
            payload = xor(data[offset:offset + stored_size], seed)
            if flags & 1:
                payload = decoder.decompress(payload, max_output_size=max(1, size), allow_extra_data=False)
            if len(payload) != size:
                raise ValueError(f'Incomplete data: expected {size} bytes, decoded {len(payload)}.')
            normalized = name.as_posix()
            if normalized in result:
                raise ValueError('Duplicate archive member name.')
            result[normalized] = payload
        return result
    except (ValueError, zstandard.ZstdError) as error:
        location = f'{path.name} / {member}' if member else path.name
        raise ValueError(f'{location}: {error}') from error
