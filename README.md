# Touhou 6 for the 3DS

A Nintendo 3DS port of **Touhou Koumakyou ~ the Embodiment of Scarlet Devil** with additional quality-of-life features, based on [GensokyoClub/th06](https://github.com/GensokyoClub/th06).

Compatible with both **Old and New Nintendo 3DS and 2DS systems**.

**This port is a work in progress.** Some screens and features aren't finished yet, and bugs remain.

You'll need the original PC game files. The included Windows converter prepares them for the 3DS.

## Features

- Integrated **EoSD Retexture Pack v0.3**, with selectable original/retextured graphics and Old/New player sprites.
- Original, PCB, IN, and SA sound sets.
- Toggle shooting and focus, plus remappable controls.
- Adjustable **Point of Collection** behavior: collect at max power only, or at any power.
- Optional circular boss health bars, focus hitbox indicator, and sidebar life/bomb counters.
- English/Japanese language selection, including translated dialogue and endings.

## Screenshots

<p align="center">
  <img src="docs/screenshots/main-menu.png" width="260" alt="English main menu" />
  <img src="docs/screenshots/stage-1.png" width="260" alt="Stage 1 gameplay on Normal difficulty" />
  <img src="docs/screenshots/stage-4.png" width="260" alt="Stage 4 gameplay on Lunatic difficulty" />
</p>

Screenshots taken in Azahar.

## Known unfinished parts

- Music Room currently uses only one screen.
- Some score displays are still too small on the console.
- Menu layouts and text sizing still need work.

## Requirements

- A homebrew-enabled 3DS or 2DS.
- The original Japanese TH06 game files, including the DAT archives and WAV music in `bgm`.
- A Windows 10 or 11 PC to run the converter.
- DSP firmware dumped on your console for audio. See [3DS Hacks Guide](https://3ds.hacks.guide/finalizing-setup.html).

## Installation

Download `TH06-Asset-Converter.exe` and either `th06.3dsx` or `th06.cia` from the [latest release](https://github.com/ItsPoofy/th06-3ds/releases/latest).

1. Put the converter in a writable folder on your PC, then run it.
2. Select your TH06 game folder—the folder containing the DAT archives and `bgm`.
3. Wait for **Assets ready**. The converter creates an `assets` folder beside the executable. Music conversion can take a few minutes.
4. Copy that `assets` folder to `/3ds/th06/` on your SD card.

Then install your chosen build:

- **3DSX:** Copy `th06.3dsx` to `/3ds/th06/`, then launch **Touhou 6** from Homebrew Launcher.
- **CIA:** Copy `th06.cia` to your SD card, install it with FBI, then launch **Touhou 6** from the HOME Menu.

Both builds need `/3ds/th06/assets/`. Installing the CIA does not copy the assets.

```text
SD card/
└── 3ds/
    └── th06/
        ├── th06.3dsx   ← only needed for Homebrew Launcher
        └── assets/
            ├── asset-version.txt
            ├── audio/
            ├── data/
            ├── textures/
            ├── ui/
            └── variants/
```

The converter requires WAV music; OGG-only installations aren't supported.

## Controls

| Input | Default action |
| --- | --- |
| D-pad / Circle Pad | Move |
| A | Shoot / confirm |
| B | Bomb / back |
| L | Focus |
| X | Skip dialogue |
| START | Pause |

Change bindings in **Options > Controls > Key Config**. Shooting and focus can also be set to toggle.

## Bugs

Report problems in [Issues](https://github.com/ItsPoofy/th06-3ds/issues). Include your console model, release version, whether you use CIA or 3DSX, and what happened. For crashes, attach `/3ds/th06/startup.log` and a Luma crash dump if available.

## Building

See [Build from source](docs/build.md).

## Credits

- **ZUN / Team Shanghai Alice** — original game.
- **[GensokyoClub/th06](https://github.com/GensokyoClub/th06)** — original reimplementation.
- **ItsPoofy** — 3DS port.
- **Emarrel** — EoSD Retexture Pack v0.3.
- **[Touhou Patch Center](https://www.thpatch.net/wiki/Touhou_Patch_Center:Main_page)** — English translation.
