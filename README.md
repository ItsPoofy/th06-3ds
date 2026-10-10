# Touhou 6 for the 3DS

A Nintendo 3DS port of **Touhou Koumakyou ~ the Embodiment of Scarlet Devil**, maintained by **ItsPoofy** and based on [GensokyoClub/th06](https://github.com/GensokyoClub/th06).

The release includes an asset converter, a Homebrew Launcher build (`.3dsx`), and a HOME Menu build (`.cia`). Original game archives, stage data, sprites, and audio are not included. The converter uses your PC installation to create the assets required by both builds. The port's expanded backgrounds and UI lettering are included.

## Requirements

- A homebrew-enabled Nintendo 3DS or 2DS. For console setup, follow [3DS Hacks Guide](https://3ds.hacks.guide/).
- A complete original Japanese TH06 installation with its DAT archives and original WAV files in `bgm`.
- A Windows 10 or Windows 11 PC for the converter.
- DSP firmware on your SD card. Follow the [DSP setup instructions](https://3ds.hacks.guide/finalizing-setup.html#section-iii-rtc-and-dsp-setup) if you haven't dumped it.

## Prepare the assets

1. Download `TH06-Asset-Converter.exe` from [Releases](https://github.com/ItsPoofy/th06-3ds/releases/latest).
2. Move the executable to a writable folder on your PC.
3. Open `TH06-Asset-Converter.exe`.
4. Select the TH06 folder containing the original DAT archives and the `bgm` folder.
5. Wait for **Assets ready**. Music conversion can take a few minutes.
6. Optional: Select **Add retexture pack…** and choose the EoSD Retexture Pack v0.3 folder to import retextures, Old/New player sprites, and Original/PCB/IN/SA sound sets.
7. Optional: Select **Add TH20 graphics…** and choose your Touhou 20 folder to import its focus indicator and circular boss-bar artwork.
8. Copy the generated `assets` folder to `/3ds/th06/` on your SD card.

The converter creates `assets` beside its executable and leaves the original game folder unchanged. If `assets` already exists beside the converter, move or rename it before converting again.

Both builds read `/3ds/th06/assets/`. Keep the folder name and its contents together. Without optional TH20 artwork, the game uses the original boss bar; the TH20 focus graphic is unavailable. Missing optional retextures or sound sets use the original assets.

## Install the Homebrew Launcher build

1. Download `th06.3dsx` from [Releases](https://github.com/ItsPoofy/th06-3ds/releases/latest).
2. Copy `th06.3dsx` to `/3ds/th06/` on your SD card.
3. Reinsert the SD card into your console.
4. Open **Touhou 6** in Homebrew Launcher.

```text
SD card/
└── 3ds/
    └── th06/
        ├── th06.3dsx
        └── assets/
            ├── asset-version.txt
            ├── audio/
            ├── data/
            ├── textures/
            ├── ui/
            └── variants/
```

## Install the HOME Menu build

Prepare and copy the assets first. Installing the CIA does not install game assets.

1. Download `th06.cia` from [Releases](https://github.com/ItsPoofy/th06-3ds/releases/latest).
2. Copy `th06.cia` to `/cias/` on your SD card.
3. Reinsert the SD card into your console.
4. Open **FBI**.
5. Select **SD**, then open **cias**.
6. Select `th06.cia`.
7. Select **Install and delete CIA**.
8. Open **Touhou 6** from the HOME Menu.

The CIA and 3DSX share settings, records, and replays in `/3ds/th06/`. The default screen mode is **Top screen only**. Change it in **Options > Display**.

## Update

Replace the 3DSX or install the new CIA. If the release notes require asset conversion, generate a new `assets` folder and replace only `/3ds/th06/assets/`. Keep the other files in `/3ds/th06/` to preserve settings and records.

## Troubleshooting

| Problem | Action |
| --- | --- |
| The game says assets are missing | Copy the entire generated `assets` folder to `/3ds/th06/`. Check for `/3ds/th06/assets/asset-version.txt`; avoid an extra nested `assets` folder. |
| The converter reports missing WAV music | Select an installation with the original WAV files in `bgm`. An OGG-only installation isn't supported. |
| Music and effects are silent | Dump DSP firmware using the linked console setup guide, then restart the game. |
| An optional setting uses original artwork or audio | Import the corresponding optional content in the converter and copy the updated assets to the SD card. Restart the game. |
| The game crashes | Open a GitHub issue with the console model, 3DSX/CIA version, stage or menu, character, shot type, and settings. Attach `/3ds/th06/startup.log` and the Luma crash dump if available. |

## Development and credits

See [Build from source](docs/build.md) for the game and converter build instructions. The game source retains the upstream [GPL-3.0 license](LICENSE).

- Original game: ZUN / Team Shanghai Alice.
- Reimplementation: [GensokyoClub/th06](https://github.com/GensokyoClub/th06) and its contributors.
- 3DS port, expanded backgrounds, UI, and converter: ItsPoofy.
- English dialogue and endings: [Touhou Patch Center](https://www.thpatch.net/wiki/Touhou_Patch_Center:Main_page), `lang_en` patches.
- Native platform libraries: [devkitPro](https://devkitpro.org/), libctru, Citro3D, Citro2D, and SDL.
- DSP-ADPCM encoder: [VGAudio](https://github.com/Thealexbarney/VGAudio).
- Optional TH20 extraction: [Touhou Toolkit](https://github.com/thpatch/thtk).
- Font fallback: [Noto fonts](https://github.com/notofonts), under the SIL Open Font License.

Bundled converter dependency notices are in [resources/licenses](tools/converter/resources/licenses). The installation guide follows the [Google developer documentation style guide](https://developers.google.com/style).
