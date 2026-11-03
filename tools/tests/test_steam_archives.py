"""Synthetic Steam archives: both encodings, dispatch, and damaged inputs."""
from contextlib import redirect_stdout
import io
from pathlib import Path
import struct
import sys
import tempfile
import unittest

import zstandard

TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS))
from game_archives import archives, unpack
from pkgl_archive import key_from_seed
from prepare_assets import prepare
from test_converter_errors import archive_bytes


# Known key vectors independently checked against the reference C reader.
TABLE_KEY = bytes.fromhex('4f7cf19ca84b554b5afc59b4ded70b63')  # CRC32(th06ST)
ENTRY_KEY = bytes.fromhex('2347b1418ee07a3507c08ac68bbaa29a')  # seed 0x12345678


def encrypted(data, key):
    return bytes(value ^ key[index % 16] for index, value in enumerate(data))


def fixture(name='steam.bin', payload=b'Steam Classic game data', compressed=False,
            flags=None, size=None, offset=None):
    name = name.encode('utf-8')
    stored = zstandard.ZstdCompressor().compress(payload) if compressed else payload
    table_size = 32 + len(name)
    record = struct.pack('<HIQQQH', int(compressed) if flags is None else flags,
                         0x12345678, len(payload) if size is None else size,
                         len(stored), 8 + table_size if offset is None else offset, len(name))
    return (b'PKGL' + struct.pack('<I', table_size)
            + encrypted(record + name, TABLE_KEY) + encrypted(stored, ENTRY_KEY))


class SteamArchives(unittest.TestCase):
    def test_key_schedule_matches_reference_vectors(self):
        self.assertEqual(key_from_seed(0xa56e2801), TABLE_KEY)
        self.assertEqual(key_from_seed(0x12345678), ENTRY_KEY)
        self.assertEqual(key_from_seed(0), bytes.fromhex('c15c0289ec2d0a9167ec8e65a18debbe'))

    def test_extracts_plain_and_zstd_members_including_unicode(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)/'th06ST.dat'
            for compressed in (False, True):
                path.write_bytes(fixture('folder/\u5b57.bin', b'game bytes'*100, compressed))
                self.assertEqual(unpack(path), {'folder/\u5b57.bin': b'game bytes'*100})

    def test_both_archive_formats_prepare_together_while_saves_are_ignored(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root/'source'
            source.mkdir()
            (source/'th06ST.dat').write_bytes(fixture())
            (source/'old.DAT').write_bytes(archive_bytes('old.bin', b'original data'))
            save = b'TH6K saved game'
            (source/'score.dat').write_bytes(save)
            self.assertEqual({p.name for p in archives(source)}, {'th06ST.dat', 'old.DAT'})
            with redirect_stdout(io.StringIO()):
                prepare(source, root/'assets')
            self.assertEqual((root/'assets/data/steam.bin').read_bytes(), b'Steam Classic game data')
            self.assertEqual((root/'assets/data/old.bin').read_bytes(), b'original data')
            self.assertEqual((source/'score.dat').read_bytes(), save)

    def test_invalid_directories_and_payloads_name_the_archive(self):
        invalid = [b'PKGL', fixture()[:12], fixture()[:-1],
                   fixture(offset=0), fixture(flags=2), fixture(size=999),
                   fixture(compressed=True, size=999)]
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)/'th06ST.dat'
            for data in invalid:
                with self.subTest(data=data[:12]):
                    path.write_bytes(data)
                    with self.assertRaisesRegex(ValueError, 'th06ST.dat.*: .+'):
                        unpack(path)

    def test_renaming_an_archive_explains_filename_dependency(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)/'renamed.dat'
            path.write_bytes(fixture())
            with self.assertRaisesRegex(ValueError, 'keep the original DAT filename'):
                unpack(path)

    def test_member_cannot_escape_destination(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)/'th06ST.dat'
            for name in ('../bad.bin', r'..\bad.bin', '/bad.bin', 'C:/bad.bin'):
                path.write_bytes(fixture(name))
                with self.subTest(name=name), self.assertRaisesRegex(ValueError, 'Invalid archive member path'):
                    unpack(path)


if __name__ == '__main__':
    unittest.main()
