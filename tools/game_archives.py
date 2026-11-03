"""Detect supported game archives by signature, ignoring saves and config."""
from pathlib import Path

import pbg3_archive
import pkgl_archive

READERS = {b'PBG3': pbg3_archive.unpack, b'PKGL': pkgl_archive.unpack}


def archives(source):
    for path in sorted(Path(source).iterdir()):
        if path.is_file() and path.suffix.lower() == '.dat':
            with path.open('rb') as file:
                if file.read(4) in READERS:
                    yield path


def unpack(path):
    path = Path(path)
    with path.open('rb') as file:
        reader = READERS.get(file.read(4))
    if reader is None:
        raise ValueError(f'{path.name}: Not a supported TH06 game archive.')
    return reader(path)
