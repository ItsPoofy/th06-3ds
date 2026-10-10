# Build from source

The game build does not require a PC game installation or a ROMFS folder. Game assets are generated separately by the converter and loaded from the SD card at runtime.

## Build the 3DSX

Install [devkitPro](https://devkitpro.org/wiki/Getting_Started), the `3ds-dev` group, and the 3DS SDL2, SDL2_image, and SDL2_ttf port libraries. CMake, Ninja, and Python 3 are also required. Configure `DEVKITPRO` and `DEVKITARM` for your installation.

Run these commands from the repository root in a devkitPro shell:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/3DS.cmake" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The output is `build/th06.3dsx`. The generated SMDH identifies the application as **Touhou 6**, describes it as **Touhou 6 for the 3DS**, and lists **ItsPoofy** as the author.

For a native Windows SDK layout with `.exe` tools, use `cmake/Windows3DS.cmake` as the toolchain file. It reads the same environment variables.

## Build the CIA

Install [makerom](https://github.com/3DSGuy/Project_CTR/releases) and [bannertool](https://github.com/Epicpkmn11/bannertool/releases). Build the game first, then run:

```sh
python tools/package_cia.py build --makerom /path/to/makerom --bannertool /path/to/bannertool
```

The output is `build/th06.cia`. Packaging uses the homebrew icon, banner, and permissions in `packaging/`. The CIA title ID is `000400000F706000`. Both console generations use the legacy application memory layout; New 3DS can use its faster CPU and L2 cache. No game ROMFS is embedded.

## Build the Windows converter

Use Python 3.12 on Windows with Tkinter installed.

```powershell
python -m pip install -r tools/converter/requirements.txt
python tools/build_converter.py
```

The output is `dist/TH06-Asset-Converter.exe`. It includes Python and its dependencies, so the end user doesn't need a Python installation. The ADPCM helper uses .NET Framework 4.5 or later, which is available on Windows 10 and Windows 11.

The converter modules separate the folder-picker interface, conversion pipeline, texture preparation, translation formats, and bundled content installation. Its checked-in resources contain the port's UI lettering, expanded backgrounds, translation patches, open font fallbacks, conversion tools, EoSD Retexture Pack v0.3 graphics and sound sets, and the TH20 focus and circular boss-bar sprites. Original TH06 archives, music, and stage scripts are generated from the user's installation.

The converted cosmetics are in `tools/converter/resources/bundled-assets/`. The converter installs them automatically. It creates the Old player sprites and the Japanese and English UI overrides from the user's assets, preserving the port's expanded backgrounds and lettering.

### Rebuild conversion helpers

`PrepareAudio.cs` is the source for the music helper. Download [VGAudio 2.2.1](https://www.nuget.org/packages/VGAudio/2.2.1) and use its `lib/net45/VGAudio.dll` with the .NET Framework C# compiler:

```powershell
csc /optimize+ /reference:VGAudio.dll /out:PrepareAudio.exe tools/PrepareAudio.cs
```

Place `PrepareAudio.exe` and `VGAudio.dll` in `tools/converter/resources/` before building the converter.

The developer utilities in `tools/converter/optional_content.py` prepare retexture and TH20 graphics. The `thdat.exe` and `thanm.exe` helpers are built from [Touhou Toolkit](https://github.com/thpatch/thtk), which supports TH20 extraction. Their PNG and zlib runtime libraries are included beside them in the source repository; the converter executable excludes these developer tools. The helper licenses, retexture credits, and font notices are retained in `tools/converter/resources/licenses/`.

## Asset and publication boundaries

Keep generated `assets`, local ROMFS data, PC game folders, settings, scores, and crash dumps outside the repository. The release artifacts are the converter executable, 3DSX, and CIA. The converter bundles the selected retexture graphics and sound sets and the two TH20 sprites, alongside the port's backgrounds and UI. Do not add complete game archives, original stage scripts, or original music to the bundle.
