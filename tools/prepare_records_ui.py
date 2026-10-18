"""Generate the records menu's clean, aligned English overlays."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


def label(text, width, height, size, font_path, cyan=False):
    scale = 3
    font = ImageFont.truetype(str(font_path), size*scale)
    image = Image.new('RGBA', (width*scale, height*scale))
    draw = ImageDraw.Draw(image)
    box = draw.textbbox((0, 0), text, font=font)
    x, y = 8*scale-box[0], (height*scale-(box[3]-box[1]))//2-box[1]
    draw.text((x, y), text, font=font, fill='white', stroke_width=scale, stroke_fill='black')
    if cyan:
        # Match the native colored headings while keeping our UI typeface.
        face = Image.new('L', image.size)
        ImageDraw.Draw(face).text((x, y), text, font=font, fill=255)
        gradient = Image.new('RGBA', image.size)
        pixels = gradient.load()
        for row in range(image.height):
            fraction = row/max(1, image.height-1)
            color = (round(255-127*fraction), 255, 255, 255)
            for col in range(image.width):
                pixels[col, row] = color
        image.paste(gradient, (0, 0), face)
    return image.resize((width, height), Image.Resampling.LANCZOS)


def prepare(resources):
    resources = Path(resources)
    font_path = Path('C:/Windows/Fonts/arial.ttf')
    if not font_path.exists():
        font_path = resources/'latin-font.ttf'
    overlay = Image.open(resources/'result00.anm.bin.overlay.png').convert('RGBA')
    mask = Image.open(resources/'result00.anm.bin.mask.png').convert('L')
    rows = ['View Easy Mode Records', 'View Normal Mode Records',
            'View Hard Mode Records', 'View Lunatic Mode Records', 'View Extra Mode Records', 'Spell Cards']
    for index, text in enumerate(rows):
        y, width = 48+32*index, 192 if index == 5 else 256
        overlay.paste(label(text, width, 32, 19, font_path), (0, y))
        ImageDraw.Draw(mask).rectangle((0, y, width-1, y+31), fill=255)
    overlay.paste(label('Quit', 64, 32, 19, font_path), (192, 208))
    ImageDraw.Draw(mask).rectangle((192, 208, 255, 239), fill=255)
    overlay.save(resources/'result00.anm.bin.overlay.png')
    mask.save(resources/'result00.anm.bin.mask.png')
    overlay = Image.new('RGBA', (256, 256))
    mask = Image.new('L', overlay.size)
    for text, x, y, width in [('Easy', 0, 0, 112), ('Normal', 112, 0, 144),
                              ('Hard', 0, 48, 112), ('Lunatic', 112, 48, 144),
                              ('Extra', 0, 96, 112), ('Spell Cards', 0, 144, 256)]:
        overlay.paste(label(text, width, 48, 24, font_path, True), (x, y))
        ImageDraw.Draw(mask).rectangle((x, y, x+width-1, y+47), fill=255)
    overlay.save(resources/'result01.anm.bin.overlay.png')
    mask.save(resources/'result01.anm.bin.mask.png')


if __name__ == '__main__':
    prepare(Path(__file__).resolve().parent/'converter/resources')
