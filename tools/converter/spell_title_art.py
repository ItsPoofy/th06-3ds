"""Prepare the player underline and the original TH20 boss butterfly band."""
from PIL import Image
from prepare_assets import write_texture


def prepare(assets, resources):
    line = Image.new('RGBA', (512, 8))
    pixels = line.load()
    for x in range(512):
        for y, glow in enumerate((0, 30, 90, 255, 255, 90, 30, 0)):
            pixels[x, y] = (255, 243, 205, round(min(1, x/64)*glow))
    write_texture(assets/'textures/spell-title-underline.bin', line)
    # TH20 ascii_1280.anm sprite285: x=1, y=385, w=510, h=78.
    # Retain its original pixels; GPU padding is handled by write_texture.
    band = Image.open(resources/'spell-title-butterflies.png').convert('RGBA')
    write_texture(assets/'textures/spell-title-butterflies.bin', band)
