"""Import optional graphics and sound sets from the user's own copies."""
import io
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import wave
import numpy as np
from PIL import Image
from prepare_assets import unpack, write_texture
from game_formats import image_texture

UI_SHEETS = ('ascii', 'asciis', 'title01', 'title01s', 'title02', 'title03',
             'title04', 'title04s', 'select01', 'select02', 'select03', 'select04',
             'select05', 'result00', 'result02', 'result03', 'replay00', 'music00',
             'staff01', 'staff02', 'staff03', 'front', 'slpl00a', 'slpl00b', 'slpl01a', 'slpl01b')


def retexture(source, assets, resources, report):
    source, assets = Path(source), Path(assets)
    archives = [source/'Inivisible Hitbox/th06e_CM.DAT', source/'Inivisible Hitbox/th06e_TL.DAT',
                source/'th06e_ST.DAT', source/'Sound/Original/th06e_IN.DAT']
    if any(not p.is_file() for p in archives):
        raise ValueError('Select the EoSD Retexture Pack v0.3 folder containing Inivisible Hitbox, Sound, and th06e_ST.DAT.')
    report('Preparing optional retexture and sound sets…')
    base = {p.name: p.read_bytes() for p in (assets/'data').iterdir() if p.is_file()}
    graphics = dict(base)
    for archive in archives:
        graphics.update(unpack(archive))
    variants = assets/'variants'
    for name in graphics:
        if not name.endswith('.anm'):
            continue
        image = image_texture(graphics, name)
        if image is None:
            continue
        player = 'reimu' if name.startswith(('player00.', 'face00')) else 'marisa' if name.startswith(('player01.', 'face01')) else None
        variant = player+'-new' if player else 'retexture'
        target = variants/variant/'textures'
        target.mkdir(parents=True, exist_ok=True)
        write_texture(target/(name+'.bin'), image)
        if player:
            original = variants/(player+'-old')/'textures'
            original.mkdir(parents=True, exist_ok=True)
            shutil.copy2(assets/'textures'/(name+'.bin'), original/(name+'.bin'))
    for stem in UI_SHEETS:
        name = stem+'.anm.bin'
        for lang in ('jp', 'en'):
            output = variants/('retexture-'+lang)/'textures'
            output.mkdir(parents=True, exist_ok=True)
            localized = variants/'en/textures'/name
            source_texture = localized if lang == 'en' and localized.exists() else assets/'textures'/name
            if source_texture.exists():
                shutil.copy2(source_texture, output/name)
    for pack in ('Original', 'PCB', 'IN', 'SA'):
        output = variants/('sound-'+pack)/'audio'
        output.mkdir(parents=True, exist_ok=True)
        for name, data in unpack(source/f'Sound/{pack}/th06e_IN.DAT').items():
            if not name.endswith('.wav'):
                continue
            with wave.open(io.BytesIO(data), 'rb') as wav:
                rate, channels, width, frames = wav.getframerate(), wav.getnchannels(), wav.getsampwidth(), wav.getnframes()
                pcm = wav.readframes(frames)
            if width == 1:
                pcm = ((np.frombuffer(pcm, dtype=np.uint8).astype(np.int16)-128)<<8).astype('<i2').tobytes()
            elif width != 2:
                raise ValueError(f'Unsupported SFX precision: {pack}/{name}')
            (output/(Path(name).stem+'.pcm')).write_bytes(struct.pack('<4sIII', b'PCM3', rate, channels, frames)+pcm)


def th20(source, assets, resources, report):
    archive = Path(source)/'th20.dat'
    if not archive.is_file():
        raise ValueError('Select the Touhou 20 folder containing th20.dat.')
    report('Extracting optional TH20 focus and boss-bar artwork…')
    with tempfile.TemporaryDirectory(prefix='th06-th20-') as temporary:
        root = Path(temporary)
        def run(arguments):
            result = subprocess.run(arguments, cwd=root, capture_output=True, text=True,
                                    creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
            if result.returncode:
                raise RuntimeError(result.stdout+result.stderr)
        run([str(resources/'thdat.exe'), '-x20', str(archive), 'effect.anm', 'front.anm'])
        run([str(resources/'thanm.exe'), '-x20', str(root/'effect.anm'), 'effect/eff_sloweffect.png'])
        run([str(resources/'thanm.exe'), '-x20', str(root/'front.anm'), 'front/lifebar.png'])
        write_texture(Path(assets)/'ui/focus-th20.bin', Image.open(root/'effect/eff_sloweffect.png').crop((0, 0, 64, 64)))
        write_texture(Path(assets)/'textures/boss-lifebar.bin', Image.open(root/'front/lifebar.png'))
