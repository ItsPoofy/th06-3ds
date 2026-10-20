"""Converter regressions: saves beside archives and actionable failure reports."""
from contextlib import redirect_stdout
import io
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

TOOLS = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(TOOLS), str(TOOLS/'converter')]
from pbg3_archive import archives, unpack
from prepare_assets import prepare
from diagnostics import error_message, write_error_log
from pipeline import convert


def archive_bytes(name='test.bin', payload=b'game data', bad_checksum=False):
    def pack(bits):
        bits += '0' * (-len(bits) % 8)
        return bytes(int(bits[i:i+8], 2) for i in range(0, len(bits), 8))

    def integer(value):
        return '11' + f'{value:032b}'

    compressed = pack(''.join('1'+f'{byte:08b}' for byte in payload) + '0'*14)
    header_length = 13
    table = header_length + len(compressed)
    header = b'PBG3' + pack(integer(1) + integer(table))
    checksum = (sum(compressed) + bad_checksum) & 0xffffffff
    fields = (0, 0, checksum, header_length, len(payload))
    directory = pack(''.join(integer(value) for value in fields)
                     + ''.join(f'{byte:08b}' for byte in name.encode('ascii')+b'\0'))
    return header + compressed + directory


class ConverterErrors(unittest.TestCase):
    def test_used_installation_ignores_saves_and_config_without_modifying_them(self):
        with tempfile.TemporaryDirectory() as temporary:
            source = Path(temporary)/'kouma'
            source.mkdir()
            expected = {
                'score.dat': b'TH6K'+bytes(32),
                'config.DAT': b'configuration',
                'unrelated.dat': b'',
                '\u7d05\u9b54\u90f7ST.DAT': archive_bytes(),
            }
            for name, data in expected.items():
                (source/name).write_bytes(data)
            (source/'folder.dat').mkdir()
            self.assertEqual([p.name for p in archives(source)], ['\u7d05\u9b54\u90f7ST.DAT'])
            output = Path(temporary)/'converted'
            with redirect_stdout(io.StringIO()):
                prepare(source, output)
            self.assertEqual((output/'data/test.bin').read_bytes(), b'game data')
            for name, data in expected.items():
                self.assertEqual((source/name).read_bytes(), data)

    def test_save_only_folder_explains_what_to_select(self):
        with tempfile.TemporaryDirectory() as temporary:
            source = Path(temporary)/'kouma'
            source.mkdir()
            (source/'score.dat').write_bytes(b'TH6K')
            with self.assertRaisesRegex(ValueError, 'No TH06 PBG3 game archives'):
                prepare(source, Path(temporary)/'converted')

    def test_damaged_archive_identifies_its_filename(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)/'damaged.DAT'
            for data in (b'PBG3', b'not an archive'):
                path.write_bytes(data)
                with self.assertRaisesRegex(ValueError, 'damaged.DAT: .+'):
                    unpack(path)

    def test_incomplete_installation_lists_the_missing_resources(self):
        with tempfile.TemporaryDirectory() as temporary:
            source = Path(temporary)/'kouma'
            (source/'bgm').mkdir(parents=True)
            (source/'bgm/music.wav').touch()
            (source/'game.DAT').write_bytes(archive_bytes())
            with redirect_stdout(io.StringIO()), self.assertRaisesRegex(
                    ValueError, 'Missing game resources: title01.anm, stage1.std'):
                convert(source, Path(temporary)/'assets', Path(temporary), lambda text: None)
            self.assertFalse((Path(temporary)/'assets').exists())

    def test_checksum_failure_identifies_archive_and_member(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)/'damaged.DAT'
            path.write_bytes(archive_bytes(bad_checksum=True))
            with self.assertRaisesRegex(ValueError, 'damaged.DAT / test.bin: Archive checksum'):
                unpack(path)

    def test_empty_exception_still_has_readable_details_and_a_log(self):
        with tempfile.TemporaryDirectory() as temporary:
            try:
                raise AssertionError()
            except Exception as error:
                path = write_error_log(error, 'Extracting game archives', 'kouma', 'assets', temporary)
                message = error_message(error, 'Extracting game archives', path)
            self.assertIn('AssertionError:', message)
            self.assertIn('Extracting game archives', message)
            self.assertIn(str(path), message)
            report = path.read_text(encoding='utf-8')
            self.assertIn('Traceback', report)
            self.assertIn('AssertionError', report)
            self.assertIn('Source: kouma', report)

    def test_log_falls_back_when_converter_folder_is_not_writable(self):
        with patch('diagnostics.Path.write_text', side_effect=PermissionError()):
            path = write_error_log(ValueError('failed'), 'Preparing', 'kouma', 'assets', '.')
        self.assertIsNotNone(path)
        try:
            self.assertIn('ValueError: failed', path.read_text(encoding='utf-8'))
        finally:
            path.unlink()

    def test_error_remains_readable_even_if_no_log_can_be_saved(self):
        with patch('diagnostics.Path.write_text', side_effect=PermissionError()), \
                patch('diagnostics.tempfile.NamedTemporaryFile', side_effect=PermissionError()):
            path = write_error_log(EOFError(), 'Preparing', 'kouma', 'assets', '.')
        self.assertIsNone(path)
        self.assertIn('EOFError:', error_message(EOFError(), 'Preparing', path))


if __name__ == '__main__':
    unittest.main()
