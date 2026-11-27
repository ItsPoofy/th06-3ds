"""Render records and replay labels with the shared menu label renderer."""
from pathlib import Path
import sys
from PIL import Image, ImageFont
from render_ui_label import render_label, mask_for

ROOT = Path(__file__).resolve().parent.parent
TITLE_SIZE = 19
SUBTITLE_SIZE = 8
RED = [(255, 255, 255), (255, 242, 242), (255, 205, 205), (255, 170, 170)]
LABELS = [
    ('records', 'Dreamy Records', '夢の記録', TITLE_SIZE, RED),
    ('subtitle', 'Result of Score and Spell Card', 'Result of Score and Spell Card', SUBTITLE_SIZE, [(255, 255, 255)]),
    ('replay-title', 'Replays', 'リプレイ', TITLE_SIZE, RED),
    ('replay-subtitle', 'Look at these elegant replays!', '瀟洒なるリプレイを見よう！', SUBTITLE_SIZE, [(255, 255, 255)]),
    ('play-replay', 'Play this replay?', 'リプレイを再生しますか？', 18, None),
    ('save', 'Do you want to save this replay?', 'リプレイを保存しますか？', 15, None),
    ('overwrite', 'Is it alright to overwrite?', '上書きしてもよろしいですか？', 15, None),
    ('continue-error1', 'Replay cannot be saved', 'コンティニューしたため', 15, RED),
    ('continue-error2', 'as a result of continuing.', 'リプレイを保存できません', 15, RED),
    ('rate-error1', 'Replay cannot be saved', 'リフレッシュレート設定により', 15, RED),
    ('rate-error2', 'with this refresh rate.', 'リプレイを保存できません', 15, RED),
    ('save-error', 'This replay cannot be saved.', 'リプレイを保存できません', 15, RED),
    ('choose-slot', 'Select a replay slot', '保存先を選択', 15, None),
    ('replay-name', 'Replay name', 'リプレイ名', 15, None),
    ('yes', 'Yes', 'はい', 15, None), ('no', 'No', 'いいえ', 15, None),
    ('empty', 'No replays', 'リプレイがありません', 18, None),
]
REPLAY_LABELS = [
    ('replay-number', 'No.', 'No.', 12, None),
    ('replay-name-header', 'Name', 'Name', 12, None),
    ('replay-player', 'Player', 'Player', 12, None),
    ('replay-rank', 'Rank', 'Rank', 12, None),
    ('replay-score', 'Score', 'Score', 12, None),
    ('replay-stage', 'STAGE', 'STAGE', 11, 'copy:caption-stage'),
    ('replay-slowdown', 'SLOW DOWN', 'SLOW DOWN', 11, 'caption'),
    ('replay-play', 'Play', '再生', 15, 'copy:play'),
    ('replay-back', 'Back', '戻る', 15, 'copy:back'),
    ('replay-confirm', 'Play this replay?', 'リプレイを再生しますか？', 22, None),
    ('replay-yes', 'Yes', 'はい', 15, None),
    ('replay-no', 'No', 'いいえ', 15, None),
]
for index, (english, japanese) in enumerate((('Reimu - Spirit Sign', '霊夢 - 霊符'),
                                           ('Reimu - Dream Sign', '霊夢 - 夢符'),
                                           ('Marisa - Magic Sign', '魔理沙 - 魔符'),
                                           ('Marisa - Love Sign', '魔理沙 - 恋符'))):
    REPLAY_LABELS.append((f'replay-header{index}', english, japanese, 15, f'summary:header{index}'))
for index, text in enumerate(('Easy', 'Normal', 'Hard', 'Lunatic', 'Extra')):
    REPLAY_LABELS.append((f'replay-list-difficulty{index}', text, text, 12, f'small-difficulty{index}'))
    REPLAY_LABELS.append((f'replay-difficulty{index}', text, text, 15, f'copy:difficulty{index}'))
REPLAY_LABELS.append(('replay-caption', 'REPLAY No.', 'REPLAY No.', 11, 'caption'))
for slot in range(16):
    REPLAY_LABELS.append((f'replay-slot{slot:02}', f'{slot:02}', f'{slot:02}', 11, 'caption'))
LABELS += REPLAY_LABELS


def label_profile(name):
    import re
    import numpy as np
    header = (ROOT/'assets/records_labels.hpp').read_text(encoding='utf-8')
    x, y, height, width, _ = map(int, re.search(r'\{"'+name+r'",(\d+),(\d+),(\d+),(\d+),(\d+)\}', header).groups())
    pixels = np.asarray(Image.open(ROOT/'tools/converter/resources/records-en.png').convert('RGBA'))[y:y+height, x:x+width]
    profile = []
    for row in pixels:
        face = row[(row[:, 3] > 240) & (row[:, :3].max(1) > 200)]
        if len(face):
            profile.append(tuple(np.median(face[:, :3], axis=0).astype(int)))
    return profile



def records_profile(title01):
    import numpy as np
    sys.path.insert(0, str(ROOT/'tools/converter'))
    from graphics import decode_texture
    original = np.asarray(decode_texture(title01.parent.parent/'textures/result00.anm.bin'))
    profile = []
    for y in range(9, 37):
        # The subtitle begins at x=26, y=30; sample the heading alone.
        row = original[y, :26 if y >= 30 else 160]
        face = row[(row[:, 3] > 240) & (row[:, :3].max(1) > 180)]
        if len(face):
            profile.append(tuple(np.median(face[:, :3], axis=0).astype(int)))
    return profile


def prepare(font, title01):
    entries = []
    heading_profile = records_profile(title01)
    sheets = {language: Image.new('RGBA', (512, 512)) for language in ('en', 'jp')}
    rendered = []
    for name, english, japanese, size, profile in LABELS:
        stroke, padding = (2, 2) if name == 'replay-confirm' else (1, 0)
        if name in ("records", "replay-title"):
            profile = heading_profile
        divisor = 1
        if isinstance(profile, str) and profile.startswith('copy:'):
            import re
            source = profile.removeprefix('copy:')
            header = (ROOT/'assets/records_labels.hpp').read_text(encoding='utf-8')
            gx, gy, gh, ew, jw = map(int, re.search(r'\{"'+source+r'",(\d+),(\d+),(\d+),(\d+),(\d+)\}', header).groups())
            images = [Image.open(ROOT/f'tools/converter/resources/records-{language}.png').convert('RGBA').crop((gx, gy, gx+width, gy+gh))
                      for language, width in (('en', ew), ('jp', jw))]
            divisor = 2
        elif isinstance(profile, str) and profile.startswith('summary:'):
            from prepare_records_ui import board_style, board_label, BOARD_LABELS, BOTTOM_SCALE
            geometry, colors = board_style(title01.parent.parent, font)
            hue = BOARD_LABELS[profile.removeprefix('summary:')][2]
            images = [board_label(text, font, geometry, colors, title01, hue, BOTTOM_SCALE) for text in (english, japanese)]
            divisor = 2
        elif profile == 'caption':
            from prepare_records_ui import section_caption
            images = [section_caption(text, font, title01) for text in (english, japanese)]
            divisor = 2
        else:
            if isinstance(profile, str):
                profile = label_profile(profile)
            images = [render_label(text, font, title01, size, stroke, padding, profile=profile)[0] for text in (english, japanese)]
        ink = []
        ink_heights = []
        if divisor == 1:
            font_at_size = ImageFont.truetype(str(font), size, layout_engine=ImageFont.Layout.BASIC)
            for text in (english, japanese):
                box = mask_for(text, font_at_size, stroke)[1].getbbox()
                ink.extend((padding+3+box[0], padding+3+box[1], box[2]-box[0]))
                ink_heights.append(box[3]-box[1])
        else:
            import numpy as np
            for image in images:
                pixels = np.asarray(image)
                mask = (pixels[:, :, 3] > 200) & (pixels[:, :, :3].max(2) > 127)
                rows, columns = np.where(mask)
                ink.extend((int(columns.min()), int(rows.min()), int(columns.max()-columns.min()+1)))
                ink_heights.append(int(rows.max()-rows.min()+1))
        advance_font = ImageFont.truetype(str(font), size*divisor, layout_engine=ImageFont.Layout.BASIC)
        advances = [round(advance_font.getlength(text)*64) for text in (english, japanese)]
        rendered.append((name, images, ink, divisor, advances, ink_heights))
    shelves = []
    for name, images, ink, divisor, advances, ink_heights in sorted(rendered, key=lambda item: (-max(i.height for i in item[1]), -max(i.width for i in item[1]))):
        width, height = max(i.width for i in images), max(i.height for i in images)
        shelf = next((s for s in shelves if height <= s[2] and s[0]+width+1 <= 512), None)
        if shelf is None:
            shelf = [1, 1 if not shelves else shelves[-1][1]+shelves[-1][2]+2, height]
            shelves.append(shelf)
        x, y = shelf[:2]
        if y+height+1 > 512:
            raise ValueError('Records heading atlas is full')
        for language, image in zip(('en', 'jp'), images):
            sheets[language].paste(image, (x, y))
        entries.append((name, x, y, height, images[0].width, images[1].width, *ink, divisor, *advances, *ink_heights))
        shelf[0] += width+2
    for language, sheet in sheets.items():
        sheet.save(ROOT/f'tools/converter/resources/record-headings-{language}.png')
    lines = ['#pragma once', 'namespace CtrRecordHeadingAssets {',
             'struct Label {const char* name;int x,y,h,en,jp,enLeft,enTop,enInkWidth,jpLeft,jpTop,jpInkWidth,divisor,enAdvance,jpAdvance,enInkHeight,jpInkHeight;};', 'inline constexpr Label labels[]={']
    lines += ['{"%s",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d},' % entry for entry in entries]
    lines += ['};', '}', '']
    (ROOT/'assets/record_headings.hpp').write_text('\n'.join(lines), encoding='utf-8')


if __name__ == '__main__':
    prepare(Path(sys.argv[1]), Path(sys.argv[2]))
