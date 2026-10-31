# Macless

Resizing, EyeZoom, and useful F3 overlays for Minecraft speedrunning. Built for macOS, Windows, and Linux.

## Get it

[Download version 1.0.2](https://github.com/riye%2dprog/macless/releases/tag/v1.0.2). Choose the JAR matching Minecraft 1.16.1 or 26.3, put it in your mods folder, and restart. Fabric API is not required.

Minecraft 26.3 needs Java 25. Minecraft 1.16.1 needs Java 17 or newer.

Each JAR includes native libraries for Apple Silicon, Intel Macs, Windows x64, and Linux x64 and arm64. macOS targets version 14, although compatibility with macOS 14 still needs runtime verification. Linux requires glibc 2.28 or newer. Additional capture dependencies and exact build settings are listed in [build details](approval/build.json).

26.3 supports OpenGL, Vulkan, and Metal through a compatible external mcopt backend. 1.16.1 uses OpenGL.

## Use it

Ctrl+I or F8 opens the interface. F6 toggles Thin, F7 Wide, F9 EyeZoom, F5 Preemptive, and F10 Fullscreen. Shortcuts can be changed, combined into chords, or set to hold. Resize shortcuts also work in menus, except while typing in chat, text fields, books, or signs. Gameplay remaps and camera scaling stay inactive in menus.

Resizing changes the actual game render target inside the existing window. The operating system window keeps its size. EyeZoom magnifies that image beside the game viewport, with measuring labels from 1 to 9.

Normal mouse scaling applies to both axes. Boat Eye uses godsens (0.02291165) in the vanilla camera input, preserves your normal turn rate with whole mouse counts, and slows EyeZoom independently. Set Ninjabrain’s 1.13+ sensitivity to 0.02291165 and its tall resolution to match EyeZoom. Your saved Minecraft sensitivity stays unchanged. Values go up to 20x, while effective turn gain stays within vanilla sensitivity limits.

The profiler redraws the section selected through vanilla number keys. Mapless appears when the selected section contains the block entity entry. Neither searches hidden profiler branches, and the entity counter uses the displayed F3 counter.

Overlay size, position, font, colors, opacity, padding, and visible content are editable under Layout. Click a numeric field to type a value, then press Enter or leave the field to apply it. Settings save automatically in `config/macless/`.

Ninjabrain integration displays the bot's local API output. Enable the API in the bot first. There is no Ninjabrain overlay and no OBS output route.

## Status

This release is not approved for submitted runs.

[Binary hashes](approval/artifacts.json) and [test results](approval/evidence/release%2d1.0.2.json) are available.

## Build

Use Java 25, Python 3, CMake 3.24 or newer, and a C++20 compiler. FreeType is built from pinned source on every platform. The Gradle wrapper verifies its distribution checksum.

```sh
./gradlew assemble :legacy:assemble check
```

The platform workflow builds five native targets and assembles both universal JARs. [Hosted checks](https://github.com/riye%2dprog/macless/actions) build each update. Detailed release and QA commands, dependency versions, and local verification setup are in [build details](approval/build.json). QA controllers and fixtures are excluded from release JARs.

## License

MIT.
