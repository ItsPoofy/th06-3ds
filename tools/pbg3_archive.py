"""Read TH06 archives without confusing saves and configuration files for assets."""
from pathlib import Path


class Bits:
    def __init__(self, data, offset=0, end=None):
        self.data, self.pos = data, offset * 8
        self.end = len(data) if end is None else end

    def read(self, count):
        if self.pos + count > self.end * 8:
            raise ValueError('Unexpected end of archive.')
        value = 0
        for _ in range(count):
            value = (value << 1) | ((self.data[self.pos // 8] >> (7 - self.pos % 8)) & 1)
            self.pos += 1
        return value

    def integer(self):
        return self.read((self.read(2) + 1) * 8)


def archives(source):
    for path in sorted(Path(source).iterdir()):
        if not path.is_file() or path.suffix.lower() != '.dat':
            continue
        with path.open('rb') as file:
            if file.read(4) == b'PBG3':
                yield path


def unpack(path):
    path = Path(path)
    data = path.read_bytes()
    member = None
    try:
        if data[:4] != b'PBG3':
            raise ValueError('Not a TH06 PBG3 archive.')
        bits = Bits(data, 4)
        count, table = bits.integer(), bits.integer()
        header_end = (bits.pos + 7) // 8
        if not count or count > len(data) or not header_end <= table < len(data):
            raise ValueError('Invalid archive directory.')
        bits = Bits(data, table)
        entries = []
        for _ in range(count):
            fields = [bits.integer() for _ in range(5)]
            name = bytearray()
            while (byte := bits.read(8)):
                name.append(byte)
            entries.append((name.decode('ascii'), fields))
        result = {}
        for index, (member, (_, _, checksum, offset, size)) in enumerate(entries):
            end = entries[index + 1][1][3] if index + 1 < len(entries) else table
            if not header_end <= offset < end <= table:
                raise ValueError('Invalid compressed-data offsets.')
            bits = Bits(data, offset, end)
            window = bytearray(8192)
            head, output = 1, bytearray()
            # Replacement archives may end at their declared size without a
            # zero-distance terminator, just like the original reader permits.
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
            if len(output) != size:
                raise ValueError(f'Incomplete data: expected {size} bytes, decoded {len(output)}.')
            if sum(data[offset:end]) & 0xffffffff != checksum:
                raise ValueError('Archive checksum does not match; the file may be damaged.')
            result[member] = bytes(output)
        return result
    except ValueError as error:
        location = f'{path.name} / {member}' if member else path.name
        raise ValueError(f'{location}: {error}') from error
