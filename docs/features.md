# New features

## Custom Practice

**Practice Start** now has two options:

- Regular Practice: the original practice mode. Play one full stage from the
  start, with scores recorded.
- Custom Practice: Based on thprac. Choose what to practice. You can configure everything from where to start, like during a specific stage section or spell card to your resources or scores.

### Setup

Press Left or Right to change a value, or press A to open a list.

- Stage: what stage to play. Stages will appear once you reach it on that
  difficulty with that shot type.
- Start: where the run begins. See [Start points](#start-points).
- Portion, Section, or Frame: the exact starting point. Which row appears
  depends on Start.
- Phase: the phase of the selected attack.
- Options: opens the start values and run rules.
- Start Practice: begins the run.

The top screen shows the stage, start point, and section. For spell cards it
also shows your history from normal play as captures and attempts. Note that attempts done in Custom Practice mode will not be counted towards your history.

### Start points

| Start | What you choose |
| --- | --- |
| Beginning | Start at the beginning of the stage. |
| Stage Portion | A part of the stage before or after the midboss, such as `1st Half 2`. |
| Mid Boss | A midboss attack. |
| End Boss | A boss attack. |
| Nonspell | A nonspell attack. |
| Spellcard | A spell card. |
| Frame | A stage frame. Left and Right move by 60 frames (one second). |

Spell cards you have not seen yet show as `????????` and cannot be selected.
They unlock the first time you meet them in play.

### Options

| Option | Range | Default | Notes |
| --- | --- | --- | --- |
| Lives | 0 to 8 | 8 | |
| Bombs | 0 to 8 | 8 | |
| Power | 0 to 128 | 128 | |
| Score | 0 to 999,999,990 | 0 | Left and Right move by 1,000,000. |
| Graze | 0 to 99,999 | 0 | Left and Right move by 100. |
| Point items | 0 to 9,999 | 0 | Left and Right move by 10. |
| Rank | 0 to 32 | 32 | Up to 99 with Rank Lock on. |
| Rank Lock | On / Off | Off | Holds rank at the chosen value. |
| Repeat Until Stopped | On / Off | Off | Automatically restarts the section when it ends or you run out of lives. |
| Stop After Section | On / Off | On | Ends the run when the selected section ends. |
| Skip Dialogue | On / Off | On | Available only for sections with dialogue. |
| Fake Shot | Selected, Reimu A, Reimu B, Marisa A, Marisa B | Selected | Stage 4 only. Plays the boss attacks for another shot type. |

## Options

Options are split into five pages: Gameplay, Controls, Display, Audio, and
Content. The new settings are:

### Gameplay

| Setting | Values | Default |
| --- | --- | --- |
| PoC | Max Power Only, Always | Max Power Only |

PoC sets when moving to the top of the playfield collects every item on
screen. `Always` collects at any power level.

### Controls

| Setting | Values | Default |
| --- | --- | --- |
| Toggle Fire | On, Off | Off |
| Toggle Focus | On, Off | Off |

With Toggle Fire on, press Shoot once to start firing and again to stop.
Toggle Focus works the same way for Focus.

### Display

| Setting | Values | Default |
| --- | --- | --- |
| Screen Mode | Top Only, Split | Top Only |
| Alignment | Top, Middle, Bottom | Bottom |
| Focus Hitbox | On, Off | On |
| Quick Info | On, Off | On |
| Boss Bar | OG, Classic, Modern | Modern |
| FPS Counter | On, Off | On |

- Screen Mode: `Top Only` draws the playfield on the top screen. `Split`
  stretches it across both screens.
- Alignment: the playfield position in Split mode. It has no effect in Top
  Only mode.
- Focus Hitbox: shows your hitbox while you focus.
- Quick Info: shows your lives and bombs beside the character name on the
  sidebar.
- Boss Bar:`OG` is the original straight bar, `Modern` is a circular bar
  around the boss, and `Classic` is the original-style boss HUD with the
  modern spell card decorations.
- FPS Counter: shows the frame rate.

### Audio

| Setting | Values | Default |
| --- | --- | --- |
| Sound Set | Original, PCB, IN, SA | Original |

Sound Set replaces the sound effects with the sets from Perfect Cherry
Blossom, Imperishable Night, or Subterranean Animism.

### Content

| Setting | Values | Default |
| --- | --- | --- |
| Graphic | OG, Retexture | OG |
| Player Sprites | Old, New | Old |

- Language: changes menus, dialogue, and endings.
- Graphic: `Retexture` uses the EoSD Retexture Pack v0.3.
- Player Sprites: picks the Old or New Reimu and Marisa sprites.

Replays play back with the Language, Graphic, and Player Sprites settings they
were recorded with.

## Music Room

Tap the controls on the bottom screen to play or pause, skip to the previous or
next track, or seek within the track. Seeking also works while the track is
paused. Press Left or Right to page through a track's description.
