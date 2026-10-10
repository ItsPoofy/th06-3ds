"""Use the regular numeral glyphs for the small floating score atlas."""
from PIL import Image


def regular_score_digits(name, image):
    if name not in ('ascii.anm', 'asciis.anm'):
        return image
    image = image.copy()
    for digit in range(10):
        glyph = image.crop((digit * 16, 48, digit * 16 + 16, 64))
        image.paste(glyph.resize((8, 8), Image.Resampling.LANCZOS), (digit * 8, 0))
    return image
