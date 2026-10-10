# Touhou 6 for the 3DS

A Nintendo 3DS port of **Touhou Koumakyou ~ the Embodiment of Scarlet Devil**, maintained by **ItsPoofy** and based on [GensokyoClub/th06](https://github.com/GensokyoClub/th06).

The release includes an asset converter, a Homebrew Launcher build (`.3dsx`), and a HOME Menu build (`.cia`). Original game archives, stage data, sprites, and audio are not included. The converter uses your PC installation to create the assets required by both builds. The port's expanded backgrounds and UI lettering are included.

## Requirements

- A homebrew-enabled Nintendo 3DS or 2DS. For console setup, follow [3DS Hacks Guide](https://3ds.hacks.guide/).
- A complete original Japanese TH06 installation with its DAT archives and original WAV files in `bgm`.
- A Windows 10 or Windows 11 PC for the converter.
- DSP firmware on your SD card. Follow the [DSP setup instructions](https://3ds.hacks.guide/finalizing-setup.html#section-iii-rtc-and-dsp-setup) if you haven't dumped it.

## Instructions

1. Download `th06.3dsx` from [Releases](https://github.com/ItsPoofy/th06-3ds/releases/latest).
2. Download `TH06-Asset-Converter.exe` from [Releases](https://github.com/ItsPoofy/th06-3ds/releases/latest).
3. Move the executable to a writable folder on your PC.
4. Open `TH06-Asset-Converter.exe`.
5. Select the TH06 folder containing the original DAT archives and the `bgm` folder.
6. Wait for **Assets ready**. Music conversion can take a few minutes.
7. Copy the generated `assets` folder to `/3ds/th06/` on your SD card.

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

See [Build from source](docs/build.md) for the game and converter build instructions.

- Original game: ZUN / Team Shanghai Alice.
- Reimplementation: [GensokyoClub/th06](https://github.com/GensokyoClub/th06)
