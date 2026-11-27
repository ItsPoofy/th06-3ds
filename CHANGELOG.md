# Changelog

## 1.1 (unreleased)

Since [v1.0.4](https://github.com/ItsPoofy/th06-3ds/releases/tag/v1.0.4).
Regenerate your assets with the updated converter.

- Added Custom Practice, based on [thprac](https://github.com/touhouworldcup/thprac), with section, spell card, and phase selection, adjustable starting values, and automatic retries. Available stages and spells depend on your progress.
- Redesigned the replay menu with character portraits, stage scores, and slowdown rate. Choose a recorded stage to watch and hold R to fast-forward.
- Added playback controls to Music Room, including pause and seeking, with song descriptions on the lower screen.
- Added a Classic boss bar layout based on Touhou 7 and an option to display rank below Graze.
- Slowed the focus rings to match New Classic.
- Improved rendering, audio, and memory performance, including faster practice retries.

### Bug fixes

- Fixed replays losing sync during dialogue.
- Fixed missing title-screen artwork and menus after replay playback or saving.
- Fixed the black-screen hang when turning off the console while the game is running.
- Fixed an SDL port bug of unwanted color tints on spell card backgrounds.
- Fixed lost texture detail and visible seams in stage backgrounds, including Stage 1's grass.
- Fixed dialogue text shifting as it appears.
- Fixed Sakuya's damaged portrait reverting after her final Stage 6 spell card and portraits carrying over into the next boss's dialogue.