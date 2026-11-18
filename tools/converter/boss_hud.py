"""Render the modern spell HUD in the port's fonts, without reference-game art."""
from PIL import Image, ImageDraw, ImageFont
from prepare_assets import write_texture
from spell_title_art import prepare as prepare_spell_title_art


# TH20's Bonus and History face (ascii_1280.png), one row per pixel from the
# top of the capitals to the bottom of the descenders: white shading to blue.
LABEL_FACE = [(255, 255, 255), (245, 251, 255), (232, 246, 255), (216, 240, 255), (199, 233, 255),
              (179, 226, 255), (159, 218, 255), (138, 210, 255), (117, 201, 255), (96, 193, 255),
              (76, 185, 255), (56, 178, 255), (39, 171, 255), (23, 165, 255), (10, 160, 255)]


def atlas(font_path):
    image = Image.new('RGBA', (512, 512))
    draw = ImageDraw.Draw(image)

    def text(value, rect, size, centered=True, stroke=3, face=None):
        x, y, w, h = rect
        font = ImageFont.truetype(str(font_path), size)
        box = font.getbbox(value)
        if box[2]-box[0]+2*stroke > w or box[3]-box[1]+2*stroke > h:
            raise ValueError(f'Boss HUD glyph does not fit: {value}')
        left=x+(w-box[2]+box[0])/2-box[0] if centered else x+stroke-box[0]
        top=y+stroke-box[1]
        draw.text((left, top), value,
                  font=font, fill='white', stroke_width=stroke, stroke_fill='black')
        if face:
            # The face shades over the same rows for every label, as in TH20.
            mask = Image.new('L', image.size)
            ImageDraw.Draw(mask).text((left, top), value, font=font, fill=255)
            first, last = top+font.getbbox('H')[1], top+font.getbbox('y')[3]-1
            colors = Image.new('RGBA', image.size)
            for row in range(y, y+h):
                t = min(max((row-first)/max(1, last-first), 0), 1)*(len(face)-1)
                i = int(t)
                c = tuple(round(face[i][k]+(face[min(i+1, len(face)-1)][k]-face[i][k])*(t-i)) for k in range(3))
                colors.paste((*c, 255), (x, row, x+w, row+1))
            colors.putalpha(mask)
            image.alpha_composite(colors)

    for i, value in enumerate('0123456789'):
        text(value, (i*32, 0, 32, 52), 40, stroke=2)
    for i, value in enumerate('0123456789./'):
        # Proportional glyphs share a left bearing, including the narrow slash.
        text(value, (i*22, 56, 22, 34), 24, False)
    for value, rect, face in [('Bonus', (0, 96, 96, 32), LABEL_FACE), ('History', (112, 96, 128, 32), LABEL_FACE),
                              ('Failed', (256, 96, 96, 32), None)]:
        text(value, rect, 24, False, face=face)
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
