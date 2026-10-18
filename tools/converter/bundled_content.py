"""Install bundled cosmetic assets while preserving the port's UI and backgrounds."""
import shutil

UI_SHEETS = ('ascii', 'asciis', 'title01', 'title01s', 'title02', 'title03',
             'title04', 'title04s', 'select01', 'select02', 'select03', 'select04',
             'select05', 'result00', 'result01', 'result02', 'result03', 'replay00', 'music00',
             'staff01', 'staff02', 'staff03', 'front', 'slpl00a', 'slpl00b', 'slpl01a', 'slpl01b')


def install(assets, resources):
    shutil.copytree(resources/'bundled-assets', assets, dirs_exist_ok=True)
    variants = assets/'variants'
    # Old player sprites always come from the selected TH06 installation.
    for character in ('reimu', 'marisa'):
        original = variants/(character+'-old')/'textures'
        original.mkdir(parents=True, exist_ok=True)
        for new in (variants/(character+'-new')/'textures').iterdir():
            source = assets/'textures'/new.name
            if source.is_file():
                shutil.copy2(source, original/new.name)
    # Retexture changes gameplay art; the port's fitted and translated UI stays.
    for lang in ('jp', 'en'):
        output = variants/('retexture-'+lang)/'textures'
        output.mkdir(parents=True, exist_ok=True)
        for stem in UI_SHEETS:
            name = stem+'.anm.bin'
            localized = variants/'en/textures'/name
            source = localized if lang == 'en' and localized.is_file() else assets/'textures'/name
            if source.is_file():
                shutil.copy2(source, output/name)
