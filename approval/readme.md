# macless approval notes

macless 1.0.2 supports minecraft java 1.16.1 and 26.3. it is ready for review, but it is not approved for submitted runs. no request has been sent to moderators yet.

these notes use [rules v8](https://mc.sr/rules/rules_v8.pdf) and the matching [rules source](https://github.com/minecraft%2djava%2dedition%2dspeedrunning/rules/blob/main/rules.typ). the section numbers below refer to v8. moderators may set different conditions for this version.

## what it does

macless is a client only fabric mod with a bundled jni library. it resizes the game's render target and composites the image into the existing operating system window. dear imgui handles the settings interface.

it redraws the currently selected vanilla profiler section, the displayed entity counter, and the block entity percentage when that entry is visible in the selected section. eyezoom magnifies the rendered image and adds its original measuring grid, numbered 1 through 9. it does not detect eyes, measure angles, calculate strongholds, inspect world seeds, look up entity positions, or search profiler branches automatically.

overlay content, fonts, position, colors, padding, corners, and size are configurable. obs output and the ninjabrain in game mirror are removed. optional image, browser, desktop, and window layers only display their content; they do not analyze pixels. those sources and their external programs still need to follow the rules.

the current interface keeps the gui and overlay designs, with direct numeric input, imgui text focus connected to the 26.3 sdl input manager, fonts baked at their displayed pixel sizes, and fixes for wrapping and spacing. the profiler uses smooth text without a heavy shadow, a solid charcoal background, and subtle row separators.

## restrictions in this version

1. both adapters use the profiler section selected with vanilla number keys. mapless hides unless its block entity row is visible or the block entity section itself is selected.
2. profiler and mapless text shows at most two decimal places, matching both vanilla displays. pie geometry keeps the original vanilla percentages.
3. optional remaps are disjoint, reversible key swaps. overlapping assignments remove the old pair, and imported profiles are normalized too. resize shortcuts work in menus except while typing in chat, text fields, books, or signs. remaps remain inactive in menus.
4. sources keep the 16,384 pixel axis cap, 16 megapixel budget, and gpu limits. once in a world, the first pair exceeding the visible framebuffer locks the only oversized pair until process exit. settings resets, undo, world changes, and menus cannot clear it. eyezoom and preemptive share this pair by default; other oversized pairs are rejected. smaller sources are allowed, but shrinking the window may make a previously small mode oversized and block the switch.
5. the normal multiplier is configurable, but effective cubic mouse gain stays between vanilla sensitivity 0 and 1. boat eye uses 0.02291165 in the vanilla camera sensitivity calculation. whole mouse counts preserve its angle grid; regular input compensates for the configured sensitivity, and eyezoom uses a fixed 0.15 count multiplier. cinematic smoothing is bypassed while boat eye is enabled. the saved minecraft sensitivity is unchanged. at vanilla sensitivity 0.5, the effective multiplier is limited to 0.064 through 4.096. automatic switching with a mode still needs a ruling.
6. locally calculated ninjabrain prediction bearings, correction angles, and nether coordinates are removed. the panel shows bot fields, including measurement corrections, with the usual chunk to block coordinate formatting.
7. fabric api is not required or bundled. vanilla resource pack registration supplies the shaders.
8. release jars disable verification at compile time and omit world, fixture, control, screenshot controller, and benchmark helpers. qa system properties cannot enable them.
9. loading a native library logs its platform, filename, sha256, and sha512. required logging mods still need a ruling.
10. the default profiler sits above the hotbar. custom layouts must keep required recording information visible.

## questions for moderators

1. **profiler and counters (a.11.9 and a.10.6).** can a reviewed fabric mod read and redraw the current vanilla profiler section and entity counter, including the selected block entity percentage? the new tool f3 data exception alone does not permit this adapter. it uses internal data rather than ocr, keeps vanilla numeric selection, does not traverse hidden branches, and uses vanilla percentage precision. please review both the data access and the rendered overlays.
2. **resizing (a.10.4.a and a.10.4.b).** does the window resolution exception also cover resizing a render target inside an unchanged operating system window? the limit uses the visible framebuffer, which is smaller than or equal to the monitor's full dimensions, and only one oversized pair is allowed until the process exits. does this approach meet the rule, including its bounds and retina interpretation?
3. **sensitivity (a.1.4).** is automatic mode specific sensitivity allowed when effective gain stays within the vanilla slider's range? it scales accumulated mouse counts before vanilla sensitivity is applied and leaves the options slider alone. boat eye uses a fixed godsens camera curve with integer counts, compensates for the normal sensitivity outside eyezoom, and bypasses cinematic smoothing. the quantization and mode switching still need review.
4. **native libraries and logging (a.8.5 and a.8.16).** can each bundled dll, dylib, and so be approved with its fabric jar? the libraries are extracted from the jar and loaded through `system.load` inside minecraft, rather than injected into an arbitrary process. do the injected library rule and liblogger requirement apply? which approved logging setup is needed on each platform? the mod's own hash logs only report the loaded file.
5. **ninjabrain (a.11.7.a.i).** is the gui display and `chunk * 16 + 4` coordinate conversion allowed? prediction distance and certainty, eye records, and blind/divine messages come from the bot. the mod does not calculate a new prediction bearing. the external bot version and settings still need to meet their own rules.
6. **versions and renderers (a.8.5 and the version specific lwjgl rules).** can the exact 1.16.1 and 26.3 builds be approved separately, and under which renderer and runtime conditions? support for opengl, vulkan, and external metal does not approve mcopt, the backend, the launcher runtime, or other mods. v8 does not settle every 26.3 renderer and runtime combination.
7. **composition and recordings (a.2.9 and a.10.6).** which eyezoom crops, extracted overlays, and passive browser or capture layers are allowed? recordings must keep the hotbar, required pause text, toasts, and timer visible. external layers cannot be used for prohibited screen reading. custom layouts can cover required information even though the default profiler has been raised.

all seven questions remain pending in `rulings.json`. any required feature changes need a new build and new hashes for review.

## where to look in the source

1. `native/core.cpp`: settings migration, the resolution lock, key swaps, and effective sensitivity.
2. `native/input.cpp`: gameplay input gating and physical key chords.
3. `native/features.cpp`: mapless selection from the current section only.
4. `native/ui.cpp` and `native/overlays.cpp`: the gui, measuring grid, pie geometry, rounding, and layouts.
5. [26.3 profiler mixin](../src/main/java/dev/riye/toolscreen/mixin/%50rofiler%4dixin.java): selected section capture and hiding the vanilla pie.
6. [1.16.1 minecraft mixin](../legacy/src/main/java/dev/riye/toolscreen/mixin/%4cegacy%4dinecraft%4dixin.java): profiler capture.
7. both mouse mixins: sensitivity integration.
8. [native bridge](../shared/java/dev/riye/toolscreen/%4eative%42ridge.java): library extraction and loading, hash logs, and the jni interface.
9. `native/ninja_macos.mm`, `native/ninja_desktop.cpp`, and the [ninjabrain client](../shared/java/dev/riye/toolscreen/%4einjabrain%43lient.java): bot transport and field parsing.
10. both gradle build files: release and qa settings, dependencies, and resources. `.github/workflows/build.yml` builds the platform versions; its results are linked below.

## files and checks

the public repository and versioned release include both universal jars, source, third party notices, and sha256 and sha512 hashes. each jar bundles macos arm64 and intel, windows x64, and linux x64 and arm64 libraries.

`artifacts.json` identifies the current jars and native libraries. `build.json` has the build commands and dependencies. the release checksums identify its uploaded files. the [current check record](evidence/release%2d1.0.2.json) lists code checks and packaging checks for 1.0.2. no final 1.0.2 jar was launched in minecraft.

`checklist.json` tracks publication and review. the older [local verification record](evidence/verification.json), [source comparison](evidence/source%2dcomparison.json), and [hosted build record](evidence/hosted%2dbuild.json) describe 1.0.1. they are historical evidence, not runtime verification of 1.0.2. native and java input and resizing code changed in this update.

full minecraft sessions on windows, linux, and intel macos remain unverified. the prism test instance was an isolated copy, and this review does not approve its other mods. macos 14 is the compilation target, and freetype is built from pinned source. macos 14 runtime compatibility remains unverified.

## submission draft

please review macless 1.0.2 for minecraft 1.16.1 and 26.3. the source, release jars, sha256 and sha512 hashes, platform checks, and seven questions are included above. please review the resizing, composition, eyezoom, profiler, mapless, counter, ninjabrain, and input features, and confirm the version, platform, and logging requirements. approval is still pending.
