"""Build the Windows converter from the checked-in source and resources."""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
subprocess.run([sys.executable, '-m', 'PyInstaller', '--noconfirm', '--clean',
                '--onefile', '--windowed', '--name', 'TH06-Asset-Converter',
                '--collect-submodules', 'fontTools',
                '--paths', str(root/'tools'), '--paths', str(root/'tools/converter'),
                '--add-data', str(root/'tools/converter/resources')+';resources',
                '--distpath', str(root/'dist'), '--workpath', str(root/'build/converter'),
                '--specpath', str(root/'build/converter'), str(root/'tools/converter/app.py')], check=True)
