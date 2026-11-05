# Build from source

## Build the 3DSX

Install [devkitPro](https://devkitpro.org/wiki/Getting_Started), the `3ds-dev` group, and the 3DS libraries for SDL2, SDL2_image, and SDL2_ttf. Install CMake, Ninja, and Python 3. Set `DEVKITPRO` and `DEVKITARM` to your SDK folders.

Clone the repository:

```sh
git clone https://github.com/ItsPoofy/th06-3ds.git
cd th06-3ds
```

In a devkitPro shell, configure and build the game:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/3DS.cmake" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The output is `build/th06.3dsx`. For a Windows SDK with `.exe` tools, set `CMAKE_TOOLCHAIN_FILE` to `cmake/Windows3DS.cmake`.

To run the game, generate assets with the converter and copy them to `/3ds/th06/assets/` on your SD card. See [Installation](../README.md#installation).

## Package the CIA

Install [makerom v0.18.4](https://github.com/3DSGuy/Project_CTR/releases/tag/makerom-v0.18.4) and [bannertool](https://github.com/Epicpkmn11/bannertool/releases). After building the game, run:

```sh
python tools/package_cia.py build \
  --makerom /path/to/makerom \
  --bannertool /path/to/bannertool
```

The output is `build/th06.cia`. Its title ID is `000400000F706000`. The icon, banner, and permissions come from `packaging/`.

## Build the Windows converter

Install Python 3.12 with Tkinter on Windows. From the repository root, run:

```powershell
python -m pip install -r tools/converter/requirements.txt
python tools/build_converter.py
```

The output is `dist/TH06-Asset-Converter.exe`. It includes Python and its dependencies. The music helper requires .NET Framework 4.5 or later.

The converter reads the user's PC game files, prepares textures and audio, applies translations, and installs the bundled graphics and sound sets. Its resources are in `tools/converter/resources/`. Converted cosmetics are in `bundled-assets/`, and notices are in `licenses/`.

### Rebuild the music helper

Download [VGAudio 2.2.1](https://www.nuget.org/packages/VGAudio/2.2.1). With its `lib/net45/VGAudio.dll` and the .NET Framework C# compiler available, run:

```powershell
csc /optimize+ /reference:VGAudio.dll /out:PrepareAudio.exe tools/PrepareAudio.cs
```

Copy `PrepareAudio.exe` and `VGAudio.dll` to `tools/converter/resources/` before building the converter.

Keep generated assets, PC game files, saves, settings, and crash dumps outside the repository. Publish `TH06-Asset-Converter.exe`, `th06.3dsx`, and `th06.cia` as release downloads.
