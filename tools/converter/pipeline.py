"""Convert a TH06 installation without changing the source folder."""
import io
import json
import re
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import wave
from fontTools import subset
from fontTools.ttLib import TTFont
import numpy as np
from prepare_assets import prepare as prepare_game
from game_formats import message, ending
from graphics import prepare as prepare_graphics
from bundled_content import install as install_bundled_content
from boss_hud import prepare as prepare_boss_hud
from sfx_pcm import encode as encode_sfx
from PIL import ImageFont


def sounds(dest):
    output = dest/'variants/sound-Original/audio'
    output.mkdir(parents=True)
    for source in (dest/'data').glob('*.wav'):
        with wave.open(str(source), 'rb') as wav:
            rate, channels, width, frames = wav.getframerate(), wav.getnchannels(), wav.getsampwidth(), wav.getnframes()
            pcm = wav.readframes(frames)
        if width == 1:
            pcm = ((np.frombuffer(pcm, dtype=np.uint8).astype(np.int16)-128)<<8).astype('<i2').tobytes()
        elif width != 2:
            raise ValueError(f'Unsupported sound precision: {source.name}')
        (output/(source.stem+'.pcm')).write_bytes(encode_sfx(pcm, rate, channels))
        (dest/'audio').mkdir(exist_ok=True)
        shutil.copy2(output/(source.stem+'.pcm'), dest/'audio'/(source.stem+'.pcm'))


def language(dest, resources):
    translated = dest/'variants/en/data'
    translated.mkdir(parents=True, exist_ok=True)
    english_font = ImageFont.truetype(str(resources/'THBiolinum.otf'), 44)
    for patch in (resources/'translation').glob('*.jdiff'):
        name = patch.name.removesuffix('.jdiff')
        source = dest/'data'/name
        if not source.exists():
            continue
        diff = json.loads(patch.read_text(encoding='utf-8-sig'))
        if name.endswith('.end'):
            data = ending(source.read_bytes(), diff, english_font)
        else:
            data = message(source.read_bytes(), diff, english_font)
        translated.joinpath(name).write_bytes(data)
    comments = json.loads((resources/'translation/musiccmt.js').read_text(encoding='utf-8-sig'))
    names = json.loads((resources/'english-map.json').read_text(encoding='utf-8'))
    original = (dest/'data/musiccmt.txt').read_bytes().decode('cp932')
    blocks = re.findall(r'@([^\r\n]+)\r?\n([^\r\n]+)', original)
    result = []
    for index, (path, name) in enumerate(blocks):
        paragraph = ' '.join(comments[str(index+1)][1:]).translate(str.maketrans({'’': "'", '‘': "'", '“': '"', '”': '"', '—': '-', '–': '-', '…': '...'}))
        lines = []
        for word in paragraph.split():
            if (lines and len((lines[-1]+' '+word).encode('utf-8')) <= 32
                    and english_font.getlength(lines[-1]+' '+word)<=460):
                lines[-1] += ' '+word
            else:
                lines.append(word)
        packed = [b''.join(s.encode('utf-8').ljust(32) for s in lines[n:n+2]).decode('utf-8') for n in range(0, len(lines), 2)]
        result.extend(['@'+path, names.get(name, name)]+packed+[''])
    (translated/'musiccmt.txt').write_bytes(('\r\n'.join(result)+'\r\n').encode('utf-8'))


def font(dest, resources):
    source = Path('C:/Windows/Fonts/msgothic.ttc')
    if source.exists():
        face = TTFont(source, fontNumber=0)
        # Complete Japanese repertoire also covers optional stage/translation packs.
        options = subset.Options(); options.recalc_timestamp = False
        worker = subset.Subsetter(options=options)
        worker.populate(unicodes=list(range(32, 0x250))+list(range(0x2000, 0x3100))+list(range(0x4e00, 0xa000))+list(range(0xff00, 0xfff0)))
        worker.subset(face)
        face.save(dest/'msgothic-3ds.ttf')
    else:
        shutil.copy2(resources/'japanese-font.otf', dest/'msgothic-3ds.ttf')
    shutil.copy2(resources/'latin-font.ttf', dest/'NotoSans-Regular.ttf')
    shutil.copy2(resources/'THBiolinum.otf', dest/'THBiolinum.otf')
    shutil.copy2(resources/'THBiolinum-scaled.otf', dest/'THBiolinum-scaled.otf')


def convert(source, output, resources, report):
    source, output = Path(source).resolve(), Path(output).resolve()
    if not source.is_dir():
        raise ValueError('Select the folder containing the original TH06 DAT files and bgm folder.')
    if source == output or source.is_relative_to(output):
        raise ValueError('The original game folder must be outside the output assets folder.')
    if output.exists():
        raise ValueError('An assets folder already exists beside the converter. Rename it or move it before converting again.')
    if not any((source/'bgm').glob('*.wav')):
        raise ValueError('Original WAV music is missing from the bgm folder. Select an original TH06 installation with WAV music.')
    with tempfile.TemporaryDirectory(prefix='th06-convert-', dir=output.parent) as temporary:
        root = Path(temporary); dest = root/'romfs'
        report('Extracting game data and preparing textures…')
        prepare_game(source, dest)
        required = ('title01.anm', 'stage1.std', 'stage6.std', 'msg1.dat', 'front.anm')
        if any(not (dest/'data'/name).is_file() for name in required):
            raise ValueError('This is not a complete TH06 installation. Select the folder containing all original DAT archives.')
        report('Building backgrounds and menu graphics…')
        prepare_graphics(root, resources)
        report('Preparing English dialogue and endings…')
        language(dest, resources)
        report('Converting sound effects…')
        sounds(dest)
        report('Adding retexture graphics, sound sets, and TH20 focus and boss-bar sprites…')
        install_bundled_content(dest, resources)
        report('Encoding music to DSP-ADPCM. This can take a few minutes…')
        (dest/'audio').mkdir(exist_ok=True)
        command = [str(resources/'PrepareAudio.exe'), str(source/'bgm'), str(dest/'data'), str(dest/'audio')]
        result = subprocess.run(command, capture_output=True, text=True, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        if result.returncode:
            raise RuntimeError('Music conversion failed:\n'+result.stdout+result.stderr)
        if len(list((dest/'audio').glob('*.ndsp'))) < 17:
            raise ValueError('Music conversion did not produce all 17 TH06 tracks. Check that the original WAV files are present.')
        report('Preparing Japanese font…')
        font(dest, resources)
        prepare_boss_hud(dest, resources)
        (dest/'asset-version.txt').write_text('TH06-3DS assets 1\n', encoding='ascii')
        dest.rename(output)
    report('Done. Copy assets to sd:/3ds/th06/assets/.')
