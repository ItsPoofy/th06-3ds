"""Render the modern spell HUD in the port's fonts, without reference-game art."""
from PIL import Image, ImageDraw, ImageFont
from prepare_assets import write_texture
from spell_title_art import prepare as prepare_spell_title_art


def atlas(font_path):
    image = Image.new('RGBA', (512, 512))
    draw = ImageDraw.Draw(image)

    def text(value, rect, size, centered=True, stroke=3):
        x, y, w, h = rect
        font = ImageFont.truetype(str(font_path), size)
        box = font.getbbox(value)
        if box[2]-box[0]+2*stroke > w or box[3]-box[1]+2*stroke > h:
            raise ValueError(f'Boss HUD glyph does not fit: {value}')
        left=x+(w-box[2]+box[0])/2-box[0] if centered else x+stroke-box[0]
        draw.text((left, y+stroke-box[1]), value,
                  font=font, fill='white', stroke_width=stroke, stroke_fill='black')

    for i, value in enumerate('0123456789'):
        text(value, (i*32, 0, 32, 52), 40, stroke=2)
    for i, value in enumerate('0123456789./'):
        # Proportional glyphs share a left bearing, including the narrow slash.
        text(value, (i*22, 56, 22, 34), 24, False)
    for value, rect in [('Bonus', (0, 96, 96, 32)), ('History', (112, 96, 128, 32)),
                        ('Failed', (256, 96, 96, 32))]:
        text(value, rect, 24, False)
    # Names stay in the same Latin form on both language HUDs, as in modern Touhou.
    names = ['Reimu Hakurei', 'Marisa Kirisame', 'Rumia', 'Daiyousei', 'Cirno',
             'Hong Meiling', 'Koakuma', 'Patchouli Knowledge', 'Sakuya Izayoi',
             'Remilia Scarlet', 'Flandre Scarlet']
    for i, value in enumerate(names):
        text(value, ((i%2)*256, 128+(i//2)*32, 256, 32), 24, False)
    return image


def prepare(assets, resources):
    # Match the DFPOP HUD/shot-type labels; English MS Gothic text uses the
    # separate Biolinum runtime path.
    write_texture(assets/'textures'/'boss-spell-hud.bin', atlas(resources/'dfpop1std-w5.otf'))
    prepare_spell_title_art(assets,resources)
