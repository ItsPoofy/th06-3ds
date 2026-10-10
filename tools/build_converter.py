"""Build the Windows converter from the checked-in source and resources."""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
resources = root/'tools/converter/resources'
# Extraction helpers are only used by developers to regenerate the checked-in
# cosmetics. End users receive the converted sprites directly.
developer_tools = {'thdat.exe', 'thanm.exe', 'libpng16-16.dll', 'zlib1.dll'}
data = []
for resource in sorted(resources.iterdir()):
    if resource.name not in developer_tools:
        destination = 'resources/'+resource.name if resource.is_dir() else 'resources'
        data.extend(['--add-data', str(resource)+';'+destination])
subprocess.run([sys.executable, '-m', 'PyInstaller', '--noconfirm', '--clean',
                '--onefile', '--windowed', '--name', 'TH06-Asset-Converter',
                '--collect-submodules', 'fontTools',
                '--paths', str(root/'tools'), '--paths', str(root/'tools/converter'),
                *data,
                '--distpath', str(root/'dist'), '--workpath', str(root/'build/converter'),
                '--specpath', str(root/'build/converter'), str(root/'tools/converter/app.py')], check=True)
