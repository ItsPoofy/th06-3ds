"""Resolve terrain cutouts against their surface instead of the distant sky."""
from PIL import Image


def complete_surface(name, image):
    if name != 'stg1bg.anm':
        return image
    image = image.copy()
    # The first sixteen tiles and four shoreline tiles share the ground plane.
    # Their transparent portions must contain river at that same depth. Letting
    # them reveal the distant, fogged water layer produces dark triangular gaps.
    # Use this sheet's own river texture in both Original and Retexture modes.
    # Sprite 19 contains water; sprite 18 is its bright reflection overlay.
    river = image.crop((176, 48, 192, 64))
    river.putalpha(255)
    tiles = [(1 + column * 16, 1 + row * 16, 16)
             for column in range(8) for row in range(2)]
    tiles += [(129 + column * 16, 1, 15) for column in range(4)]
    for x, y, size in tiles:
        tile = image.crop((x, y, x + size, y + size))
        beneath = river.crop((0, 0, size, size))
        image.paste(Image.alpha_composite(beneath, tile), (x, y))
    return image
