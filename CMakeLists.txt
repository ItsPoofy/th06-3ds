cmake_minimum_required(VERSION 3.20)
project(th06_3ds LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
find_package(SDL2 REQUIRED CONFIG)
find_package(SDL2_image REQUIRED CONFIG)
find_package(SDL2_ttf REQUIRED CONFIG)
file(GLOB CORE_SOURCES CONFIGURE_DEPENDS src/*.cpp src/pbg3/*.cpp src/thirdparty/sjis_converter.cpp)
list(FILTER CORE_SOURCES EXCLUDE REGEX "/SoundPlayer\\.cpp$")
add_executable(th06 ${CORE_SOURCES}
    src/midi/MidiDefault.cpp
    src/platform/3ds/CtrRenderer.cpp
    src/platform/3ds/MenuLayer.cpp
    src/platform/3ds/GameplayHud.cpp
    src/platform/3ds/GameplayLogo.cpp
    src/platform/3ds/PauseMenu.cpp
    src/platform/3ds/ExtraRecordsCode.cpp
    src/platform/3ds/ResultKeyboard.cpp
    src/platform/3ds/Touch.cpp
    src/platform/3ds/Presentation.cpp
    src/platform/3ds/DisplaySettings.cpp
    src/platform/3ds/Settings.cpp
    src/platform/3ds/ContentAssets.cpp
    src/platform/3ds/LatchedInput.cpp
    src/platform/3ds/OptionsMenu.cpp
    src/platform/3ds/TitleLogo.cpp
    src/platform/3ds/BossHealthBar.cpp
    src/platform/3ds/MenuAnimation.cpp
    src/platform/3ds/SelectionArt.cpp
    src/platform/3ds/SelectionFlow.cpp
    src/platform/3ds/PracticeSelection.cpp
    src/platform/3ds/SelectionLayout.cpp
    src/platform/3ds/StageScores.cpp
    src/platform/3ds/SelectionReturn.cpp
    src/platform/3ds/GameDecorations.cpp
    src/platform/3ds/BossCounters.cpp
    src/platform/3ds/BossPresentation.cpp
    src/platform/3ds/DialogueReveal.cpp
    src/platform/3ds/PlayfieldText.cpp
    src/platform/3ds/FocusEffect.cpp
    src/platform/3ds/StageDepth.cpp
    src/platform/3ds/CharacterInfo.cpp
    src/platform/3ds/CharacterPortraits.cpp
    src/platform/3ds/Controls.cpp
    src/platform/3ds/GameLoop.cpp
    src/platform/3ds/Runtime.cpp
    src/platform/3ds/NdspAudio.cpp
    src/platform/3ds/SfxVoices.cpp
    src/platform/3ds/SfxMixer.cpp
    src/platform/3ds/SoundPlayerNdsp.cpp)
target_include_directories(th06 PRIVATE src "${CMAKE_CURRENT_BINARY_DIR}/generated" "$ENV{DEVKITPRO}/portlibs/3ds/include")
target_compile_options(th06 PRIVATE -O3 -ffunction-sections -fdata-sections -fno-strict-aliasing)
target_link_options(th06 PRIVATE -Wl,--gc-sections -Wl,-Map,th06.map)
target_link_libraries(th06 PRIVATE SDL2::SDL2main SDL2_image::SDL2_image-static
    SDL2_ttf::SDL2_ttf-static SDL2::SDL2-static citro2d citro3d freetype png bz2 z ctru m)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/scene_shader.hpp"
    COMMAND "${CTR_PICASSO_EXE}" -o "${CMAKE_CURRENT_BINARY_DIR}/scene.shbin"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/3ds/scene.v.pica"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_shader.py"
        "${CMAKE_CURRENT_BINARY_DIR}/scene.shbin" "${CMAKE_CURRENT_BINARY_DIR}/generated/scene_shader.hpp"
    DEPENDS src/platform/3ds/scene.v.pica tools/embed_shader.py VERBATIM)
target_sources(th06 PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated/scene_shader.hpp")
ctr_generate_smdh(OUTPUT th06.smdh NAME "Touhou 6" DESCRIPTION "Touhou 6 for the 3DS" AUTHOR "ItsPoofy" ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.png")
ctr_create_3dsx(th06 SMDH th06.smdh)
