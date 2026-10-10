"""Package the asset-free ELF for a homebrew-enabled Nintendo 3DS."""
import argparse
from pathlib import Path
import subprocess
import tempfile
import wave

parser = argparse.ArgumentParser()
parser.add_argument('build', type=Path)
parser.add_argument('--makerom', default='makerom')
parser.add_argument('--bannertool', default='bannertool')
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
build = args.build.resolve()
with tempfile.TemporaryDirectory(prefix='th06-banner-') as temporary:
    silence = Path(temporary)/'silence.wav'
    with wave.open(str(silence), 'wb') as wav:
        wav.setparams((1, 2, 32000, 3200, 'NONE', 'not compressed'))
        wav.writeframes(bytes(6400))
    subprocess.run([args.bannertool, 'makebanner', '-i', str(root/'packaging/banner.png'),
                    '-a', str(silence), '-o', str(build/'th06.bnr')], check=True)
subprocess.run([args.makerom, '-f', 'cia', '-o', str(build/'th06.cia'), '-rsf', str(root/'packaging/th06.rsf'),
                '-target', 't', '-elf', str(build/'th06.elf'), '-icon', str(build/'th06.smdh'),
                '-banner', str(build/'th06.bnr'), '-desc', 'app:4', '-major', '1', '-minor', '0', '-micro', '0'], check=True)
print(build/'th06.cia')
