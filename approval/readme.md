# macless approval submission

candidate: 1.0.1, for minecraft java 1.16.1 and 26.3. this package is ready for technical review. it is not an approval notice or a claim that runs using it are legal.

rules reviewed: [minecraft java edition speedrunning rules v8](https://mc.sr/rules/rules_v8.pdf), with the matching [public rules source](https://github.com/minecraft%2djava%2dedition%2dspeedrunning/rules/blob/main/rules.typ). section numbers below refer to v8. moderator decisions can change the conditions for this candidate.

## interface update

this candidate includes direct numeric input, explicit imgui text focus in the 26.3 sdl input manager, fonts baked at their displayed pixel sizes, and corrected wrapping and spacing. the profiler uses smooth text without a heavy shadow, a solid charcoal background, and subtle row separators. the feature restrictions below are retained.

## what the tool does

macless is a client only fabric mod with a bundled jni library. it changes the game's render target size and composites the result into the existing window. dear imgui provides its settings interface. it also redraws the currently selected vanilla profiler section, the displayed entity counter, and the block entity percentage when that entry is in the selected section. eyezoom magnifies the rendered game image and draws its original measuring grid, with labels 1 through 9. there is no automatic eye detection, angle measurement, stronghold calculation, world seed inspection, entity position lookup, or profiler branch search.

the gui and overlay styles are retained. overlay content, fonts, position, colors, padding, corners, and size are configurable. obs output and the ninjabrain in game mirror are absent. optional images, browser sources, and desktop/window captures are composition layers; they do not analyze their pixels. their content and external programs still need to meet the rules.

## changes made for this candidate

1. removed automatic traversal of profiler branches. both adapters publish only the section selected through vanilla numeric navigation. mapless disappears when that section has no block entity row; selecting the block entity section itself also works. this is a direct internal data adapter, so its approval is still requested below.
1. capped profiler and mapless percentage text at two decimal places, matching both vanilla profiler displays. chart geometry retains the vanilla percentages rather than inventing new values.
1. restricted optional key remaps to disjoint, reversible key swaps. assigning an overlapping swap removes the earlier pair. this also normalizes imported profiles. remaps and mode bindings are inactive in chat and normal menus.
1. capped either source axis at 16,384 pixels and retained the 16 megapixel budget and gpu limits. after entering a world, the first source size exceeding the visible framebuffer locks the only oversized pair for the entire process. another oversized pair is rejected. resetting settings, undo, changing worlds, and returning to the menu do not clear it. eyezoom and preemptive share the same pair by default. smaller sources remain available. shrinking the window after a lock can make a previously small mode count as oversized, so that switch may be refused.
1. clamped effective mouse turn scaling to the range achievable by vanilla sensitivity settings. the requested normal and eyezoom multipliers remain configurable, but the resulting cubic gain cannot go below vanilla sensitivity 0 or above vanilla sensitivity 1. at vanilla sensitivity 0.5, the effective multiplier is limited to 0.064 through 4.096. switching automatically with a mode still needs a ruling.
1. removed locally calculated ninjabrain prediction bearings, correction angles, and nether coordinates. the panel displays fields returned by the bot, plus its usual chunk to block coordinate formatting. measurement corrections supplied directly by the bot remain visible.
1. removed the embedded fabric api components. vanilla resource pack registration supplies the mod shaders. no fabric api jar is required or bundled.
1. separated qa builds from submission builds. release jars have verification disabled at compile time and omit the world, fixture, control, screenshot controller, and benchmark helpers. changing qa system properties does not enable those helpers in a release build.
1. log the bundled native library's platform, filename, sha256, and sha512 when loaded. these logs identify the library; they are not a replacement for an approved logging mod if moderators require one.
1. moved the default profiler above the hotbar. custom layouts still need to keep recording requirements visible.

## rulings requested

1. **profiler and counter access   a.11.9 and a.10.6.** may a reviewed fabric mod redraw the current vanilla profiler section and entity counter from the same client side data, including a selected block entity percentage? the generic new tool f3 data exception does not establish permission for this by itself. vanilla numeric selection is preserved, hidden branches are not traversed, and percentages use vanilla precision. this is not an ocr implementation. please rule explicitly on the adapters and rendered views.
2. **virtual resizing   a.10.4.a b.** does the window resolution exception cover resizing a render target inside an unchanged operating system window? our bound uses the visible framebuffer, which is stricter than the monitor's full dimensions, and our one oversized pair lock lasts until process exit. please confirm both the virtual rendering approach and the boundary/retina interpretation.
3. **mode sensitivity   a.1.4.** is automatic mode specific sensitivity acceptable when effective turn gain stays within the vanilla slider's attainable range? it multiplies mouse deltas before the vanilla player turn call rather than writing the options slider. smoothing and input granularity can differ; equivalent gain alone does not establish approval.
4. **native libraries and logging   a.8.5 and a.8.16.** please review each bundled dll, dylib, and so together with its fabric jar. these are extracted from the jar and loaded through system.load; they are not separately injected into an arbitrary process. please specify whether the injected library rule and liblogger requirement apply, and which approved logging arrangement is required on each platform. the self reported hashes are informational.
5. **ninjabrain api   a.11.7.a.i.** please confirm parity for the gui display and coordinate conversion `chunk * 16 + 4`. prediction distance and certainty, eye records, and blind/divine messages come from the bot. no new prediction bearing is calculated. the user's external bot version and settings remain subject to their own rules.
6. **versions and renderers   a.8.5 and the version specific lwjgl rules.** please approve exact builds separately for 1.16.1 and 26.3. support for opengl, vulkan, or an external metal backend does not approve that backend, mcopt, a launcher runtime, or another mod. the v8 text does not explicitly settle every 26.3 renderer/runtime combination.
7. **composition and recordings   a.2.9 and a.10.6.** please confirm permitted eyezoom crops, extracted overlays, and passive browser/capture layers. runners must keep the hotbar, required pause text, toasts, and timer visible in the recording, and must not use an external layer for prohibited screen reading. custom positioning can obscure required information even though the default profiler has been raised.

machine readable questions are in `rulings.json`; all are pending. if a feature needs a different restriction, a new candidate and new artifact hashes should be reviewed.

## reviewer entry points

1. `native/core.cpp`: settings migration, resolution lock, key swaps, and effective sensitivity.
1. `native/input.cpp`: gameplay input gating and physical key chords.
1. `native/features.cpp`: current section only mapless selection.
1. `native/ui.cpp` and `native/overlays.cpp`: gui, measuring grid, profiler geometry, rounding, and custom layout.
1. [src/main/java/dev/riye/toolscreen/mixin/profilermixin.java](../src/main/java/dev/riye/toolscreen/mixin/%50rofiler%4dixin.java): 26.3 selected section capture and vanilla pie suppression.
1. [legacy/src/main/java/dev/riye/toolscreen/mixin/legacyminecraftmixin.java](../legacy/src/main/java/dev/riye/toolscreen/mixin/%4cegacy%4dinecraft%4dixin.java): 1.16.1 profiler capture.
1. both mouse mixins: sensitivity integration.
1. [shared/java/dev/riye/toolscreen/nativebridge.java](../shared/java/dev/riye/toolscreen/%4eative%42ridge.java): bundled library extraction/loading, hash logging, and jni contract.
1. `native/ninja_macos.mm`, `native/ninja_desktop.cpp`, and [shared/java/dev/riye/toolscreen/ninjabrainclient.java](../shared/java/dev/riye/toolscreen/%4einjabrain%43lient.java): external bot transport and field parsing.
1. both gradle build files: release/qa split, dependencies, and resources. `.github/workflows/build.yml` provides platform builds; a workflow file is not evidence of a hosted successful run.

## files and review status

the release package contains both universal release jars, source, third party notices, checksums, a gallery, and verification results. `artifacts.json` records the unchanged binary identities, `build.json` provides build instructions and dependencies, and `evidence/verification.json` records the validation scope. `checklist.json` identifies the remaining review actions. each jar includes macos arm64/intel, windows x64, and linux x64/arm64 natives. `checksums.json` identifies jar, native, helper, and source hashes with sha256 and sha512. `verification.json` separates compiled targets from live tests and identifies qa versus release checks.

the public repository and versioned release provide the source, exact binaries, and permanent revision. publication status is recorded in `checklist.json`. no request has been sent to moderators. the release notes identify the reviewed source commit, both exact jar hashes, and native hashes. the release is marked as a candidate rather than an approval. the copied prism test instance is isolated; none of its other mods are endorsed by this package. macos 14 is the compilation target, but the apple silicon host's system built static dependencies emit macos 15 deployment warnings; macos 14 runtime compatibility has not been established.

## suggested submission text

requesting review of macless 1.0.1 for minecraft 1.16.1 and 26.3. it is a fabric/jni resizing and composition tool with a dear imgui settings interface, eyezoom, selected section profiler/mapless views, an entity counter, and a ninjabrain api panel. this candidate removes automatic profiler traversal, limits displayed precision to vanilla's two decimals, uses reversible key swaps, bounds effective sensitivity to vanilla's range, and locks one oversized source resolution per process. source, exact binaries, sha256/sha512 hashes, platform coverage, and pending rules questions are attached. please rule on the seven questions above and specify any platform/version or logging conditions. this request does not presume approval.
