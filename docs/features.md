# New features

## Custom Practice

Practice Start offers Regular Practice for full-stage runs with recorded
scores, and Custom Practice for choosing a starting section, spell card, or
attack phase with your own resources and score. Custom Practice is based on
[thprac](https://github.com/touhouworldcup/thprac).

Stages become available as you reach them on the selected difficulty, and spell
cards unlock when you encounter them. Undiscovered cards appear as `????????`.
The top screen shows your normal-play captures and attempts, with Custom
Practice attempts excluded from that history.

### Start points

| Start | What you choose |
| --- | --- |
| Beginning | Start at the beginning of the stage. |
| Stage Portion | A part of the stage before or after the midboss, such as `1st Half 2`. |
| Mid Boss | A midboss attack. |
| End Boss | A boss attack. |
| Nonspell | A nonspell attack. |
| Spellcard | A spell card. |
| Frame | A specific frame in the stage. |

### Starting values

| Option | Range |
| --- | --- |
| Lives | 0 to 8 |
| Bombs | 0 to 8 |
| Power | 0 to 128 |
| Score | 0 to 999,999,990 |
| Graze | 0 to 99,999 |
| Point items | 0 to 9,999 |
| Rank | 0 to 32, or up to 99 with Rank Lock |

### Run options

| Option | What it does |
| --- | --- |
| Rank Lock | Holds rank at the chosen value. |
| Repeat Until Stopped | Restarts the section when it ends or you run out of lives. |
| Stop After Section | Ends the run after the selected section. Turn it off to continue through the stage. |
| Skip Dialogue | Skips dialogue during the run. |
| Fake Shot | Uses another shot type's Patchouli attacks in Stage 4. |

## Options

### Gameplay

| Setting | Values |
| --- | --- |
| PoC | Max Power Only, Always |

PoC sets when moving to the top of the playfield collects every item on
screen. `Always` collects at any power level.

### Controls

| Setting | Values |
| --- | --- |
| Toggle Fire | On, Off |
| Toggle Focus | On, Off |

Toggle Fire and Toggle Focus let you press the corresponding button once to
enable it and again to stop, so you can play without holding the button down.

### Display

| Setting | Values |
| --- | --- |
| Screen Mode | Top Only, Split |
| Alignment | Top, Middle, Bottom |
| Focus Hitbox | On, Off |
| Quick Info | On, Off |
| Boss Bar | OG, Classic, Modern |
| FPS Counter | On, Off |

Screen Mode keeps the playfield on the top screen or stretches it across both
screens, with Alignment controlling its position in Split mode. Focus Hitbox
shows your hitbox while focusing, and Quick Info puts your lives and bombs
beside the character name on the sidebar.

Boss Bar offers the original TH06 layout (`OG`), a Touhou 7-style horizontal
bar with modern spell card decorations (`Classic`), or a circular health bar
around the boss (`Modern`).

### Audio

| Setting | Values |
| --- | --- |
| Sound Set | Original, PCB, IN, SA |

Sound Set replaces the sound effects with the sets from Perfect Cherry
Blossom, Imperishable Night, or Subterranean Animism.

### Content

| Setting | Values |
| --- | --- |
| Language | English, Japanese |
| Graphic | OG, Retexture |
| Player Sprites | Old, New |

Language changes menus, dialogue, and endings, while Graphic lets you switch
between the original artwork and EoSD Retexture Pack v0.3. Player Sprites
selects the Old or New Reimu and Marisa sprites.

Replays play back with the Language, Graphic, and Player Sprites settings they
were recorded with.

## Music Room

The bottom screen has track descriptions and playback controls for pausing,
switching tracks, and seeking, including while a track is paused.
