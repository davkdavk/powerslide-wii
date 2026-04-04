# Decisions Log

## 2026-04-04 (confirmed native GX texture contract fix)
- Confirmed by real Wii result that the remaining desert track mapping bug was not DE2 UV decoding.
- Root cause: native direct GX real-texture path uploaded terrain/world-object textures as row-major `RGB565`, while GX expects tiled memory layout for `GX_TF_RGB565`.
- Decision: keep DE2 `uv/uw` as the active direct-path UV pair for now and fix the renderer contract first.
- Implemented tiled `RGB565` terrain texture cache/build path in `wii_stubs/OGRE/WiiGXRenderer.cpp`.
- Keep earlier per-batch sampler-state propagation (wrap/clamp and texture scale) as part of the baseline.

## 2026-04-04 (Emergency restore assessment)

### `git status`

```text
On branch main
Your branch is up to date with 'origin/wii-port'.

Changes not staged for commit:
  (use "git add <file>..." to update what will be committed)
  (use "git restore <file>..." to discard changes in working directory)
	modified:   DECISIONS.md
	modified:   ISSUES.md
	modified:   PROGRESS.md
	modified:   STATUS.md
	modified:   orig_src/BaseApp.cpp
	modified:   orig_src/mesh/MshData.h
	modified:   rules.md
	modified:   wii_stubs/OGRE/Ogre.h
	modified:   wii_stubs/OGRE/WiiGXRenderer.cpp

Untracked files:
  (use "git add <file>..." to include in what will be committed)
	dolphin_latest.log
	orig_src/TerrainDebug.cpp
	orig_src/TerrainDebug.h

no changes added to commit (use "git add" and/or "git commit -a")
```

### `git diff --stat HEAD`

```text
 DECISIONS.md                     |  21 +++
 ISSUES.md                        |  25 +++
 PROGRESS.md                      |  25 +++
 STATUS.md                        |  36 ++++-
 orig_src/BaseApp.cpp             |  31 ++++
 orig_src/mesh/MshData.h          |   2 +-
 rules.md                         |   9 ++
 wii_stubs/OGRE/Ogre.h            |   6 +-
 wii_stubs/OGRE/WiiGXRenderer.cpp | 319 ++++++++++++++++++++++++++++-----------
 9 files changed, 386 insertions(+), 88 deletions(-)
```

### `git log --oneline -10`

```text
676149c Fix Wii terrain texture assignment and world-space UVs
5f1a5a1 Stabilize Wii terrain texture routing and harden crash paths
d525a3f Improve Wii terrain texture mapping diagnostics and batch UV path
9d81e4a Stabilize Wii terrain rendering with centered transform baseline
004812e Initialize Wii native terrain debug baseline
```

## 2026-04-03 (current direct-GX UV policy and object-path decisions)
- Reclassified the church and water tank issue: they are not going through the regular Wii mesh renderer. They are part of the track DE2 loaded with `isTerrain=true` and are rendered through the direct GX indexed terrain-style path.
- Decision: stop spending time on the generic model/submesh renderer as the primary fix path for church-like world objects. Keep those fixes, but treat them as secondary until the direct GX path matches PC behavior.
- Confirmed with user visual evidence that the pink debug-color batch behaves correctly while other colored batches do not. Decision: treat this as a per-batch uploaded-UV acceptance/handling issue, not a whole-object classification issue.
- Decision: normalize all textured direct GX chunks to the same batching behavior as the good pink chunks:
  - textured chunks always use the per-texture sub-batch upload branch when texture names exist,
  - missing triangle-slot data falls back to texture slot `0` instead of taking a separate single-upload path.
- Decision: remove silent world-planar fallback for chunks that already have uploaded UV data. If a batch has uploaded UVs, use them; only use planar mapping when no uploaded UV stream exists.
- Decision: remove the Wii-side UV magnitude gate for direct GX batches and accept any finite uploaded UV values. Rationale: pink likely survived because its UV magnitudes happened to fit the old cutoff while other valid batches did not.
- Decision: use PC OGRE only as the reference for expected UV behavior, not as the active implementation target. Confirmed behavior: mountains/buildings may legitimately be separate UV islands as long as each island is internally coherent.
- Operational decision: continue using real Wii output as source of truth. Dolphin is not a decision-maker for final visual parity in the current phase.

## 2026-04-03 (evidence-driven blocker reclassification from SD log)
- Read latest hardware log before further blind refactors; used diagnostic summaries as source of truth.
- Reclassified blocker after confirming in a single run:
  - `[TEX_FRAME_SUMMARY] requestedUnique=53 boundUnique=53`
  - `[UV_SUMMARY] batches=229 noUV=72 scale=10.00`
- Decision: stop treating current issue as a pure single-texture-bind failure.
- Decision: treat current issue as mapping-fidelity/visual-quality problem under mixed UV availability and current world-planar UV policy.
- Operational decision: keep strong diagnostics (`[TEX_BATCH]`, `[TEX_BIND]`, `[TEX_FRAME_SUMMARY]`, `[UV_SUMMARY]`) active until visual output matches expected terrain surface variation.

## 2026-04-02 (UV anchoring correction: world-space only fallback)
- Confirmed terrain UV fallback in stable Wii render path must never depend on camera-space transformed coordinates.
- Standardized fallback UV generation for batches with missing inline UVs (`hasUV=0`) to world-space planar mapping only:
  - `u = worldX / 10.0`
  - `v = worldZ / 10.0`
- Removed fallback UV clamping that could mask movement-related drift symptoms; keep raw world-anchored tiling for stability and debuggability.
- Added frame summary logging for UV coverage to measure remaining missing-UV batches:
  - `[UV_SUMMARY] batches=<N> noUV=<M> scale=10.00`.

## 2026-04-02 (stability-first crash triage and texture-routing proof strategy)
- Prioritized hard crash elimination before further texture-correctness tuning; adopted addr2line-driven fix loop on real Wii crash dumps.
- Kept fixes narrowly scoped and reversible:
  - null guards in `CloneMaterial(...)`,
  - safe override-material-name fallback in mesh creation,
  - defensive early-outs in `AddjustNormals(...)`.
- Added reset/power callback behavior to return to loader/menu as an operator-efficiency decision for rapid hardware iteration.
- Chose an explicit proof step for terrain texture routing:
  - temporary per-batch forced texture cycling + `[TEX_BATCH]` logging,
  - use visual output + bind logs to distinguish routing failure vs texture-source failure.
- After white-terrain result, accepted log evidence (`source='Loaders/Texture1'`) as root signal and moved to texture-name resolution hardening instead of changing UV math again.
- Decision: default forced batch-cycling debug OFF after the experiment; keep diagnostics available but return to normal mapping path for real validation.

## 2026-04-02 (texture/UV correction strategy after successful texture load)
- Accepted that current priority is no longer "can textures load" but "can textures map correctly per geometry batch".
- Chose a guarded UV reintroduction approach in the stable stage-4 Wii renderer:
  - use uploaded per-index UVs when finite/in-range,
  - otherwise fall back to prior planar UV mapping.
- Chose to preserve chunk-level boundaries in renderer-owned terrain buffers so rendering can bind per-batch texture names instead of one global texture assumption.
- Temporarily forced clamp wrap mode in guarded texture pass to reduce repeated-pattern masking during correctness diagnostics; defer final wrap policy until UV/routing is validated.
- Added mandatory texture/UV diagnostics (`[TEX_BIND]`, `[UVDBG]`) as required evidence for next iteration decisions, rather than relying on visual guesswork alone.

## 2026-04-02 (terrain stabilization decisions)
- Accepted centered camera-space terrain transform as the first stable Wii baseline after repeated world-space path failures.
- Chose pragmatic staged validation over large rewrites: fullscreen present -> camera-space primitives -> terrain points/subset -> full indexed terrain.
- Deferred wireframe as a primary diagnostic because Wii line-emulation path was unstable and misleading under current GX constraints.
- Standardized on safe depth cueing via height-based vertex coloring while textured pipeline is brought up incrementally.
- Kept this baseline as the safety anchor; future gameplay/world-camera and texture work should be introduced behind toggles/fallbacks to avoid regressions.

## 2026-04-02 (milestone execution policy)
- Adopted explicit staged milestones (M1-M5) with a single active milestone focus at a time.
- Preserve a runnable baseline at every step; no milestone work should remove the ability to render the map stably.
- Prefer additive toggles/fallbacks when introducing new camera or texture behavior, then remove dead paths only after milestone validation.

## 2026-04-01 (current diagnostic decisions)
- Keep full historical records; add top-of-file current-state entries instead of rewriting prior logs.
- Treat real Wii output/logs as primary truth for render bring-up; Dolphin remains useful for crash symbolization and quick smoke checks but not final visual parity.
- Maintain strict systematic iteration: one controlled probe change per step, then redeploy and validate.
- Do not force physics stepping in the `playerPhysics == null` path during native bring-up; previous attempt produced reproducible crash path in physics/vector math and was reverted.
- Continue with render-path isolation probes inside `renderTerrainBuffer()` (known on-screen test primitive + limited terrain subset + explicit draw-range logs) before further architecture changes.
- Preserve indexed topology as the authoritative terrain draw mode for current data; prior sequential-topology experiment produced center-spike artifacts and was discarded.

## 2026-03-28
- Kept the existing OGRE compatibility-layer strategy instead of attempting a direct large-scale source rewrite; this is the fastest path to reach Milestone 1 (all translation units compiling).
- Prioritized API-surface shims in this order: overlay/UI and material/font APIs first, then matrix/scene gaps, then DataStream parity. This follows the current highest-frequency compile blockers.
- Decided to maintain stubs as no-op-safe where behavior is unclear, and to prefer returning defaults over throwing so runtime bring-up can continue.
- Introduced an OGRE-like smart pointer wrapper (`SharedPtr`) in Wii stubs to preserve `isNull`, `setNull`, and `getPointer` usage patterns in original source without invasive rewrites.
- Preserved `-fno-exceptions` build mode; for exception paths hit in game code (e.g., movable text font lookup), prefer safe early-return behavior instead of enabling exceptions globally.
- Continued pragmatic parity policy: implement narrow API surface required by current compile errors first (overlay/font/text/particle/camera methods), postpone deeper behavior fidelity until all TUs compile.

## 2026-03-28 (compile closure)
- Kept `-fno-exceptions` and `-fno-rtti` as hard constraints for Wii build parity; resolved remaining blockers by removing try/catch and RTTI casts in affected game files rather than relaxing toolchain flags.
- Unblocked Lua manager compilation by dropping dependency on Lua internal headers and extending local `wii_stubs/lauxlib.h` with minimal compatibility entry points (`luaL_getn`, `luaL_setn`, `luaL_findtable`).
- Preferred additive shim extensions in OGRE/SFML stubs over invasive gameplay rewrites for scene/material/mesh/audio parity, which accelerated closure to full TU compile.
- Marked Milestone 1 compile goal complete after a clean full scan reached 97/97 translation units compiling.

## 2026-03-28 (link milestone)
- Chose to link directly from the validated TU scan object set (`wii_build/scan_now5_*.o`) to avoid partial/stale object selection and preserve exact compile baseline during link bring-up.
- Kept the existing devkitPPC/libogc link stack (`-lfat -lwiiuse -lbte -logc -lm`, `rvl.ld`) and only validated symbol closure before attempting deeper runtime behavior work.

## 2026-03-28 (runtime boot fallback)
- For Milestone 2 stability, chose a fail-open boot strategy in `BaseApp::go`: when `setup()` fails, continue into a minimal render loop instead of exiting, so Dolphin/Wii still boots to a controlled blank-screen state.
- Added null-safe frame and resize guards around `mWindow`, `mGameModeSwitcher`, and `mInputHandler` to avoid shutdown-time or partial-init null dereferences during early runtime bring-up.

## 2026-03-28 (Wii runtime defaults)
- For Wii builds, decided to default runtime data directory to `sd:` and initialize FAT at process startup (`fatInitDefault`) to align with `rules.md` requirement that packed assets come from `sd:/powerslide/`.
- Retained a local fallback (`mDataDir = ""`) only as a secondary recovery path when SD init fails, to keep runtime investigation unblocked while preserving SD-first behavior.
- Updated OGRE root stub render loop to run until frame callbacks stop it (instead of hard-capping at 1000 frames), matching expected game-loop behavior for Dolphin/Wii runtime testing.

## 2026-03-28 (render-path verification)
- Chose to add a temporary always-visible debug ground quad in `RenderWindow::update()` for Wii builds to quickly verify whether frame presentation and primitive submission are functioning, independent of higher-level OGRE scene/material conversion status.

## 2026-03-28 (GX cache/state correctness)
- Because forced debug geometry still did not appear, prioritized low-level GX correctness fixes before more gameplay-side work: flush CPU-written vertex arrays (`DCFlushRange`), increase draw buffer capacity, and explicitly set current position matrix (`GX_SetCurrentMtx`) in the Wii renderer.

## 2026-03-28 (GX direct submit fallback)
- Since indexed array path still produced blue-only output, switched Wii renderer to direct vertex submission (`GX_DIRECT` + `GX_Position3f32`/`GX_Color4u8`) as a robustness-first fallback for runtime bring-up.

## 2026-03-28 (hard overlay probe)
- Added a final low-level diagnostic draw in `Renderer::endFrame()` (orthographic red quad right before `GX_CopyDisp`) to isolate framebuffer presentation issues from all higher-level game/render abstractions.

## 2026-03-28 (overlay state hardening)
- Extended the diagnostic quad path to override GX state explicitly (disable Z, force color/alpha update, disable blending, use strip) to rule out prior pipeline state blocking visibility.

## 2026-03-28 (devkitPro parity reset)
- With repeated blue-only results, reset Wii GX initialization and draw flow to match the official devkitPro `graphics/gx/triangle` sample (copy callback, GX_INDEX8 arrays, camera/view setup, and copy timing) to establish a known-good baseline.

## 2026-03-28 (ManualObject bridge)
- Since the GX baseline now renders, chose to route OGRE `ManualObject` geometry directly into the Wii GX renderer as the simplest path to get early in-game debug and track visuals before full mesh/material conversion is complete.

## 2026-03-28 (force LLT debug)
- Enabled LLT debug render path unconditionally during race scene init to verify that `ManualObject`-based track lines/markers actually emit into the new GX bridge.

## 2026-03-28 (boot into race mode on Wii)
- For Wii bring-up, forced the game to switch directly into `ModeRaceSingle` after menu initialization so the race scene (LLT, exclusions, AI debug) is constructed without relying on menu interaction.

## 2026-03-28 (LLT path probe)
- Added an unconditional `ManualObject` cross at the start of `LLTLoader::load` to verify that the LLT loader path is actually executed on Wii builds.

## 2026-03-28 (ManualObject persistence)
- Since one-shot `ManualObject` draws still weren't visible, switched to caching and drawing all ManualObjects each frame via a small registry to keep debug geometry on screen across frames.

## 2026-03-28 (race init probe)
- Added a forced debug cross directly in `BaseRaceMode::initScene` to confirm whether the race scene init path runs at all in current Wii boot flow.

## 2026-03-28 (BaseApp probe)
- Added an early `ManualObject` probe in `BaseApp::setup` (post-GameModeSwitcher creation) to ensure manual geometry creation is viable even if race scene never initializes.

## 2026-03-28 (GX cache flush for manual geometry)
- Added `DCFlushRange` and explicit `GX_SetCurrentMtx` in WiiGX draw paths to ensure CPU-written vertex/colour arrays are visible to GX when drawing ManualObject geometry.

## 2026-03-28 (triangle sanity pass)
- Switched the per-frame debug geometry to a simple RGB triangle (matching devkitPro triangle sample) to verify the non-indexed draw path after recent GX flush changes.

## 2026-03-28 (raw GX triangle reset)
- As requested, stripped `RenderWindow::update()` to a verbatim devkitPro triangle draw sequence using raw GX calls only, moving all GX init into `RenderWindow` construction and bypassing the WiiGX abstraction entirely.

## 2026-03-28 (restore fix10 pipeline)
- Per user instruction, reverted `WiiGXRenderer.cpp` and `RenderWindow::update()` back to the known-working fix10 GX pipeline (GX_INDEX8 arrays + post-retrace copy) before attempting any new ManualObject integration.

## 2026-03-28 (ManualObject on fix10 pipeline)
- Kept the confirmed GX_INDEX8 + post-retrace pipeline intact and routed ManualObject drawing through it, removing the hardcoded debug triangle and validating via a temporary cross probe.

## 2026-03-29 (Makefile as build source of truth)
- Promoted the Wii Makefile to compile all `orig_src/**/*.cpp` and all `wii_stubs/**/*.cpp` so runtime builds reflect actual game code; `wii_build/build_full.sh` is now considered retired.

## 2026-03-29 (entry-gap isolation between BaseApp ctor and go)
- Enumerated all calls that existed in `orig_src/Main.cpp` between `BaseApp base;` and `base.go(isSafeRun);` on Wii:
  1) `WiiGX::Renderer::getInstance()`
  2) `WiiGX::Renderer::presentProbeColor(0, 255, 255, 255)`
  3) `WiiGX::Renderer::presentProbeColor(255, 0, 255, 255)`
- Disabled all of the above gap calls to isolate whether the crash is before `BaseApp::go()` or inside `BaseApp::go()`.

## 2026-03-29 (WPAD crash containment)
- Used `powerpc-eabi-addr2line` on crash PCs and confirmed faults resolve to libogc WPAD callback code (`__wpad_read_remote_name_finished` / `__wpad_register_new`) rather than game code.
- Decided to remove `WPAD_Init()` from renderer initialization (`WiiGXRenderer::init`) so graphics startup does not implicitly spawn WPAD callback state during setup bring-up.

## 2026-03-29 (requested single addr2line evidence)
- Command run: `/opt/devkitpro/devkitPPC/bin/powerpc-eabi-addr2line -e wii_build/powerslide_full_fix69.elf -f 0x80174194`
- Output:
  - `__wpad_read_remote_name_finished`
  - `/home/davem/projects/devkitpro/pacman-packages/libogc/src/libogc-3.0.4/wiiuse/wpad.c:926 (discriminator 1)`

## 2026-03-29 (new black-screen crash addr2line)
- Crash address from Dolphin log: `PC = 0x80206518` (invalid read from `0x00000005` while running `fix70`).
- Command run: `powerpc-eabi-addr2line -e wii_build/powerslide_full_fix70.elf -f 0x80206518`
- Output:
  - `build_argv`
  - `??:?`
- Decision: treat this as startup argument/path corruption around `main` setup; switch Wii forced path call from `app.go(false)` to `app.go(true)` before building `fix71`.

## 2026-03-29 (repeated crash-site verification)
- `fix71` crash address from Dolphin log: `PC = 0x80206518` (invalid read `0x00000005`), addr2line still resolves to:
  - `build_argv`
  - `??:?`
- `fix72` crash address from Dolphin log: `PC = 0x801ff6d4` (invalid read `0x00000005`), addr2line still resolves to:
  - `build_argv`
  - `??:?`
- Decision: the persistent pre-source symbol `build_argv` indicates startup argv parsing remains corrupted before game code line mapping; keep changes focused on avoiding early WPAD and launcher-side startup perturbations while preserving known-good GX red baseline behavior.

## 2026-03-29 (fix74 crash mapping)
- Crash address from Dolphin log after `build_argv` stub (`fix74`): `PC = 0x80216fdc`, invalid read `0x00000001`.
- Command run: `powerpc-eabi-addr2line -e wii_build/powerslide_full_fix74.elf -f 0x80216fdc`
- Output:
  - `strnlen`
  - `??:?`
- Decision: add a defensive Wii-side `strnlen` override that returns `0` for null/low invalid pointers and otherwise performs bounded scanning, to prevent startup libc crashes from malformed argv-derived pointers.

## 2026-03-29 (fix75 crash mapping)
- Crash address from Dolphin log after `strnlen` guard (`fix75`): `PC = 0x80208ba8`, invalid read `0x00000001`.
- Command run: `powerpc-eabi-addr2line -e wii_build/powerslide_full_fix75.elf -f 0x80208ba8`
- Output:
  - `strchr`
  - `??:?`
- Decision: add a defensive Wii-side `strchr` override with the same null/low-address guard used for `strnlen`.

## 2026-03-29 (fix76 crash mapping)
- Crash address from Dolphin log after `strchr` guard (`fix76`): `PC = 0x801ffa90`, invalid read `0x00000001`.
- Command run: `powerpc-eabi-addr2line -e wii_build/powerslide_full_fix76.elf -f 0x801ffa90`
- Output:
  - `_concatenate_path`
  - `??:?`
- Decision: override `_concatenate_path` to return a stable static path rooted at `sd:/powerslide/`, bypassing malformed path concatenation input during startup bring-up.

## 2026-03-29 (rollback unsafe libc/startup overrides)
- Black-screen branch with libc/startup symbol overrides (`build_argv`, `strnlen`, `strchr`, `_concatenate_path`, `__CheckARGV`) diverged from known-good red baseline and prevented reliable crash telemetry.
- Decision: remove those overrides and return to the last stable startup model (fix67-style red screen path), then resume forward-only crash isolation with addr2line from that baseline.

## 2026-03-29 (fix80 current crash)
- After rollback rebuild (`fix80`), Dolphin crash returns to:
  - `Invalid read from 0x00000005, PC = 0x801ff6d4`
- Command run: `powerpc-eabi-addr2line -e wii_build/powerslide_full_fix80.elf -f 0x801ff6d4`
- Output:
  - `build_argv`
  - `??:?`
- Decision: avoid broad libc overrides; keep startup close to baseline and target game-side progression (go/setup/render loop path) only after preserving stable red-screen GX init.

## 2026-03-29 (requested Wii main signature hardening)
- Applied requested Wii entry-point shape: `orig_src/Main.cpp` uses `int main()` with no `argc`/`argv` references.
- Verified `orig_src/Main.cpp` contains no `argc`, `argv`, or `argv[0]` path logic.
- Updated non-Wii-full fallback mains in `wii_stubs/stubs.cpp` and `wii_stubs/GameMain.cpp` to `int main()` as well so no local alternate entrypoints declare argv on Wii codepaths.

## 2026-03-29 (single-entrypoint refactor fix82)
- Inspected `wii_stubs/stubs.cpp` and `wii_stubs/GameMain.cpp`; both contain standalone hardware-init demo code guarded by `#if !defined(WII_FULL_BUILD)` and are not intended as full-build entrypoints.
- Kept only one active main for Wii full build by adding `wii_stubs/WiiEntry.cpp` with:
  - `int main()`
  - `fatInitDefault()`
  - call to `powerslide_main()`
- Renamed `orig_src/Main.cpp` entry function from `main` to `powerslide_main`.
- Renamed non-full-build demo mains to avoid accidental collision:
  - `wii_stubs/stubs.cpp`: `main` -> `wii_stub_entry`
  - `wii_stubs/GameMain.cpp`: `main` -> `wii_game_stub_entry`
- Built `fix82` and captured screenshot.
- Post-fix82 crash telemetry:
  - Dolphin: `Invalid read from 0x00000005, PC = 0x801ff724`
  - addr2line: `build_argv` / `??:?`

## 2026-03-29 (fix83 __system_argv attempt and fallback)
- Attempted requested CRT override in `wii_stubs/WiiEntry.cpp` by defining `__system_argv` directly; linker failed with multiple-definition against libogc `common_crt0.o` and SDA section mismatch.
- Reverted direct `__system_argv` definition in source to keep link stable.
- Applied requested linker fallback in `Makefile`:
  - `LDFLAGS += -Wl,--undefined=__system_argv`
- Built and captured `fix83`; crash remains unchanged:
  - Dolphin: `Invalid read from 0x00000005, PC = 0x801ff724`
  - addr2line: `build_argv` / `??:?`

## 2026-03-29 (fix90 assert-path correction)
- Crash attribution corrected from disassembly: `PC=0x8020026c` lands in `__assert_func`, while `build_argv` starts at `0x80200284` and was not the faulting instruction.
- Decision: treat prior `addr2line` symbol attribution around this site as unreliable due to symbol proximity; for future crashes in dense symbol regions, verify with `objdump` disassembly before acting on `addr2line` names.
- Applied Wii build policy change to bypass assertion aborts during bring-up by adding `-DNDEBUG` to `CFLAGS` in `Makefile`.

## 2026-03-29 (fix91/fix92 null-crash triage with addr2line + objdump)
- Ran requested symbolization first on `fix91`: `/opt/devkitpro/devkitPPC/bin/powerpc-eabi-addr2line -e wii_build/powerslide_full_fix91.elf -f -C 0x8015772c`.
- addr2line result at `0x8015772c` resolves into inlined `std::vector<std::string>::push_back` from `RacingGridGeneration::generate(...)` at `orig_src/gamelogic/RacingGridGeneration.cpp:42`.
- Verified with disassembly: `/opt/devkitpro/devkitPPC/bin/powerpc-eabi-objdump -dSl --start-address=0x801576a0 --stop-address=0x80157770 wii_build/powerslide_full_fix91.elf`.
  - Source-annotated objdump shows the faulting block corresponds to `res.push_back(availableCharacters[...])` at line 42.
  - This confirms crash attribution to game code path, not startup argv path.
- Decision: harden `RacingGridGeneration::generate` against empty/short character tables and index mismatch by adding bounded index resolution and fallback character (`"frantic"`) instead of raw chained indexing.
- Rationale: in Wii bring-up, upstream data loaders/stubs can yield empty/incomplete `availableCharacters`, and unchecked indexing feeds invalid references into `std::vector<std::string>::push_back`.

## 2026-03-29 (post-fix92 setup-depth localization)
- Continued forward probe localization from `fix90` baseline using additional `presentProbeColor(...)` markers inside `BaseApp::setup`.
- Observed progression at runtime:
  - reaches probe immediately after `mTrayMgr` creation,
  - reaches increasingly later probes immediately before `mGameModeSwitcher` construction,
  - does not yet reach constructor probes inside `GameModeSwitcher` (cyan) or race-init probes (purple/orange).
- Decision: treat current blocker as being on the `mGameModeSwitcher = std::make_shared<GameModeSwitcher>(createModeContext())` path (constructor entry or earliest sub-init), not in earlier setup/resource/bootstrap code.
- Added fallback behavior in `orig_src/loaders/TextureLoader.cpp` for missing textures (`load` and `loadChroma`) to return generated placeholder textures instead of relying on assert-only failure paths; this is aimed at preventing null texture/material cascades in early menu/game-mode initialization.

## 2026-03-29 (Wii asset path order: sd then local Dolphin)
- Updated Wii setup path selection in `BaseApp::setup` to always try `sd:/powerslide/` first by setting `mGameState.setDataDir("sd:")` before `initOriginalData()`.
- If SD path load fails, now explicitly fall back to working-directory relative `./powerslide/` by setting `mGameState.setDataDir(".")` and retrying `initOriginalData()`.
- Rationale: Dolphin runs `.dol` with `wii_build/` as cwd in this workflow, so `./powerslide/data.pf` resolves to `wii_build/powerslide/data.pf` while preserving SD-first behavior for real Wii hardware.

## 2026-03-29 (fix113 crash stack triage + fix114 null-material hardening)
- Ran requested full stack symbolization against `wii_build/powerslide_full_fix113.elf`:
  - Command: `/opt/devkitpro/devkitPPC/bin/powerpc-eabi-addr2line -e wii_build/powerslide_full_fix113.elf -f 0x800bc64c 0x800bbe88 0x800bc910 0x8014e034 0x80039c8c 0x8003a0fc 0x8015bc44 0x8000b3b8`
  - Results:
    - `0x800bc64c` -> `std::vector<Ogre::Technique>::size()` (`stl_vector.h:1119`)
    - `0x800bbe88` -> `_Rb_tree::_M_begin()` (`stl_tree.h:1378`)

## 2026-03-31 (native terrain pass correction)
- Chose to keep the native direct terrain draw path and fix its concrete data-flow/render issues rather than abandoning it back to OGRE mesh-only debugging.
- Decision rationale from latest Wii evidence:
  - terrain creation and GX submission are already proven alive,
  - the framebuffer symptom now matches presentation bugs better than loader bugs,
  - the direct path is the fastest place to remove chunk loss and transform mismatch.
- Applied policy for this pass:
  - accumulate all terrain uploads for the frame instead of replacing prior chunks,
  - raise direct terrain storage to a dedicated `65535`-vertex cap,
  - align the terrain pass camera/model-view convention with the known-working mesh renderer,
  - reset the accumulated terrain buffer only once per terrain rebuild (`StaticMeshProcesser::initParts`), not per chunk.
    - `0x800bc910` -> `BaseRaceMode::initData` (`orig_src/gamemodes/BaseRaceMode.cpp:87`)
    - `0x8014e034` -> `GameModeSwitcher::frameEnded` (`orig_src/gamelogic/GameModeSwitcher.cpp:359`)
    - `0x80039c8c` -> `std::swap<GameModeSwitcher*>` (`bits/move.h:237`)
    - `0x8003a0fc` -> `std::__shared_ptr<Ogre::Root>::get` (`shared_ptr_base.h:1673`)
    - `0x8015bc44` -> `powerslide_main` (`orig_src/Main.cpp:202`)
    - `0x8000b3b8` -> `exit` (`??:?`)
- Verified with `objdump -dSl` around all addresses.
  - Critical confirmation for top PC `0x800bc64c`: disassembly maps into `BaseRaceMode::initTerrain` around `BaseRaceMode.cpp:504-507`, specifically the chain:
    - `particleMat = MaterialManager::getByName("Test/Particle")`
    - `particleMat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(particle)`
  - The trap sequence (`li r9,0; lwz r9,56(r9); trap`) indicates a null/invalid object use during that material setup path.
- Fix applied first at the top PC site (as requested):
  - In `orig_src/gamemodes/BaseRaceMode.cpp`, added guarded material acquisition in `initTerrain`:
    - helper lambda `getOrCreateMaterial(...)` that uses `MaterialManager::getByName(...)`,
    - if missing, logs and creates fallback material via `MaterialManager::create(..., TEMP_RESOURCE_GROUP_NAME)`.
  - Replaced direct `getByName` calls for:
    - `Test/Particle`
    - `Test/ParticleAlpha`
    - `Test/ParticleFog`
    - `Test/ParticleFogAlpha`
  - This prevents null-material dereference in the confirmed crash block.

## 2026-03-29 (0x800c7df4 repeated crash - different fix strategy)
- User reported repeated crash signature across later builds: `Invalid read from 0x00000000, PC = 0x800c7df4`.
- Address triage on latest build in sequence (`fix118`) per request:
  - `addr2line`: `BaseMenuMode::clearData()` at `orig_src/gamemodes/BaseMenuMode.cpp:141`.
  - `objdump -dSl` context around `0x800c7df4` shows:
    - virtual `doClearData()` call,
    - then immediate dereference of `mSceneMgr` (`lwz r10,76(r31)` then `lwz r8,0(r10)`),
    - fault occurs because `mSceneMgr == NULL` while `clearData()` still executes.
- Why prior fixes were insufficient:
  - Previous work focused on race/material initialization (`BaseRaceMode::initTerrain`) and menu-loading bypass logic.
  - That did not address lifecycle ordering where menu teardown can run before `BaseMenuMode::initCamera()` created a scene manager.
  - In this path, null is still dereferenced in `BaseMenuMode::clearData()` regardless of material fixes.
- Different approach selected (as requested):
  - Instead of adding more nested checks around one dereference, stubbed the problematic teardown path for this state by turning `BaseMenuMode::clearData()` into an early no-op when `mSceneMgr` is null on Wii.
  - This treats the whole crashing teardown section as non-critical in bring-up when menu scene was never instantiated.
- Additional progression work to validate forward movement after this no-op:
  - Forced Wii constructor race transition state in `GameModeSwitcher` (`mGameMode=ModeMenu`, `mGameModeNext=ModeRaceSingle`, `mIsSwitchMode=true`) before `frameEnded()`.
  - Added null guards around `mUILoader/mUILoaderChampionship/mUIUnloader` usage in `GameModeSwitcher::frameEnded` and `loadState`.
  - Added deeper `BaseRaceMode::initData` probes to localize race startup stage.
- Result after rebuild/run (`fix119`): runtime reaches stable race-init probe color `(192,64,0)` and no longer dies immediately at the previous `0x800c7df4` menu-clear null dereference site.

## 2026-03-29 (same-folder Dolphin PF layout)
- Switched local PF path semantics from `<dataDir>/powerslide/<file>.pf` to `<dataDir>/<file>.pf` for Dolphin testing parity when assets sit next to `.dol`.
  - `PFLoader` path construction now omits the fixed `powerslide/` subfolder.
  - `OgreTools` (`getReadableFile` / `getWritibleFile`) updated to the same path convention so writable/readable sidecar files remain consistent.
- Retained path priority logic in setup:
  1. `sd:/powerslide/*.pf` on real Wii (`mDataDir = "sd:"`)
  2. `./*.pf` fallback for Dolphin (`mDataDir = "."`)
- Rationale: user moved PF payloads beside `.dol` under `wii_build/`, so requiring `powerslide/` subfolder prevents fallback resolution.

## 2026-03-29 (0x8005c0fc triage and stream-path hardening)
- Ran required crash triage for `PC=0x8005c0fc` on `fix124`:
  - `addr2line`: `std::string::_M_is_local` (inlined),
  - `objdump -dSl` maps context to `PSBaseCar::initModel` around repeated `STRSettings::parse(...)` calls and temporary string cleanup (`PSBaseCar.cpp` lines ~74-81).
- Interpretation: crash is likely secondary memory corruption/invalid stack/object state exposed during heavy string parse/setup in car init, not necessarily a standalone `std::string` bug.
- Chosen fix direction:
  - eliminate risky local-file stream ownership handoff patterns in PF access paths,
  - fail fast in car init if core car params are unavailable.
- Implemented changes:
  - `PFLoader` local file open paths now use `Ogre::FileStreamDataStream(fullPath, writable)` directly for both `init()` and `getFile()`.
  - `OgreTools` local `getReadableFile/getWritibleFile` updated to the same direct constructor path.
  - `PSBaseCar::initModel` Wii guard added: if `mCarSettings` did not load (`!mCarSettings.isLoaded()`), log and return early.
- Post-change behavior (`fix125`): no immediate transition to a new crash signature in short/medium runs; runtime remains stably at race-init probe color `(192,64,0)` pending next deeper progression step.

## 2026-03-29 (0x8005c320 repeated crash - full function stub strategy)
- User reported repeated crash signature: invalid write to `0x00000d30`, `PC=0x8005c320` (3 builds in a row), and requested full-function no-op approach.
- Triage against `wii_build/powerslide_full_fix130.elf`:
  - `addr2line -f -C 0x8005c320` => `PSBaseCar::initModel(...)` at `orig_src/pscar/PSBaseCar.cpp:85`.
  - `objdump -dSl` around `0x8005c320` confirms store to `initialVehicleSetup` member (`stfs ... 316(r29)`) immediately after `mCarSettings` path; with null base this matches the reported `0x00000d30`-class member-offset write.
- Different approach applied (requested):
  - Stubbed entire `PSBaseCar::initModel(...)` to early return under `#if defined(WII) || defined(__wii__)`.
  - This avoids all writes in the crashing function instead of trying more local null checks.
- Built requested numbered artifact:
  - `wii_build/powerslide_full_fix131.elf/.dol/.map`
- Screenshot result after stub:
  - `/tmp/wii_test_fix131_rerun.png`
  - `/tmp/wii_test_fix131_rerun_16.png`
  - dominant color `(0,24,255)` (in `BaseRaceMode::initModel` entry probe), indicating flow progresses into model stage without immediate recurrence of the `0x8005c320` write site.

## 2026-03-29 (policy-driven stubbing expansion for milestone-3 focus)
- Applied user policy to freely stub non-critical systems while preserving terrain/LLT/render/PF code paths.
- Added Wii no-op in `PSPlayerCar::initModel` to eliminate remaining player car model-init coupling.
- In `BaseRaceMode::initModel`, when minimal-runtime mode is active, create a simple manual quad (`wii_geometry_probe`) attached to race root and return immediately.
  - Purpose: verify that geometry submission path can proceed without full car/AI/physics runtime stack.
- Observed progression from probe colors:
  - `(0,24,255)` -> `(0,2,255)` -> `(0,10,255)` in later builds,
  - indicates forward execution into deeper post-terrain model path with stubs active.
- Decision: continue with this reduced runtime profile and next isolate why probe geometry is not visually dominant despite reaching geometry-probe insertion point.

## 2026-03-30 (real Wii progression + initModel crash isolation)
- Confirmed on real Wii from SD logs that FAT/PF, LLT, AI, terrain, and particles complete successfully; active blocker moved to early `BaseRaceMode::initModel`.
- Chose to continue the established `addr2line + objdump` workflow for every new crash instead of broad behavior changes, to keep each fix tied to a verified PC site.
- Added line-level Wii OSReport probes inside `initModel` before key operations (arrow texture load, lap-controller registration, models-pool init) to localize the exact failing line.
- Temporary stubs are allowed for arrow/vehicle visuals and model-copy code on Wii if needed to pass `initModel`; track geometry, terrain, PF loading, and renderer paths remain non-stubbed.

## 2026-03-31 (native loose-file pipeline + mesh visibility isolation)
- Kept `WII_NATIVE_ASSET_PIPELINE` as the active direction and treated loose-file runtime (`sd:/powerslide/wii_data/...`) as source of truth over PF-archive translation.
- Continued to use pre-baked big-endian assets and `.tpl` texture remap for Wii-native load compatibility.
- Kept non-critical systems stubbed/guarded (HUD/start sequence/vehicle internals) so rendering bring-up can proceed without unrelated runtime churn.
- Prioritized real mesh rendering over LLT debug visuals; LLT overlay draw is disabled in native mode during mesh validation.
- Added and kept OGRE mesh-to-GX draw path with 16-bit indices and batched submission as the primary terrain/track visibility path.
- Forced `Ogre::drawMeshEntities()` from `BaseRaceMode::frameStarted` in native mode to remove dependency on uncertain `RenderWindow::update()` execution ordering.
- Adopted binary render-path isolation: draw a guaranteed in-pass sanity primitive first, then diagnose mesh data flow.
- Current branch decision: because the sanity triangle is visible, focus next on mesh content/population/registration (not low-level GX visibility).

## 2026-03-31 (absolute mesh visibility + cull/material bypass pass)
- Added brute-force visibility path decisions for milestone-3 unblock:
  - camera-side visibility in stubbed camera returns true for `isVisible(AxisAlignedBox)` to avoid accidental frustum rejection from bad bounds;
  - mesh pass enforces opaque fixed-function TEV (`GX_PASSCLR`) and explicit GX cull/depth defaults for visibility diagnostics.
- Added deferred mesh-name binding in scene manager (`createEntity(name, meshName, group)` now resolves via `MeshManager`) to ensure terrain entities created by mesh name can attach to actual mesh objects in stubs.
- Added vertex-buffer truth probes in `drawMeshEntities`:
  - reports null/valid vertex buffer state,
  - reports first vertex position when available,
  - reports submitted terrain index counts per diagnostic interval.
- Added terrain attach confirmation in `StaticMeshProcesser::initPart` to verify terrain entities are attached to scene nodes during native bring-up.
- Added native-mode camera tether in `BaseRaceMode::frameStarted` (`(0,500,0)` look-at `(0,0,0)`) to remove dynamic camera uncertainty during geometry visibility diagnostics.
- Resulting decision branch: if framebuffer still shows only sanity triangle, treat OGRE-entity terrain path as non-authoritative for milestone 3 and move to direct terrain upload/draw from `MSHData::plainBuffer` in the Wii pass.

## 2026-03-31 (direct plainBuffer-to-GX pass decision outcome)
- Implemented direct upload path from `MSHData::plainBuffer` in `StaticMeshProcesser::createMesh(...)` into Wii renderer-owned terrain buffer (`uploadTerrainBufferF32(...)`) and explicit native render call (`renderTerrainBuffer()`) from frame loop.
- Kept this path texture/material independent (flat opaque color, no OGRE material dependency) to isolate geometry visibility only.
- Framebuffer result after direct path: still sanity triangle only (mustard background), no terrain footprint.
- Decision update: issue is likely before/at terrain mesh generation timing or runtime branch execution (not only entity registration/culling/material path). Next work must verify whether `createMesh(...)` is actually reached with non-zero terrain triangles in native mode at runtime.

## 2026-04-01 (init-loop stabilization under native loose-file pipeline)
- Root cause identified from Wii `debug.log`: repeated full race initialization was being re-triggered while `WII_NATIVE_ASSET_PIPELINE` kept vehicle/physics setup stubbed, so `player physics is null` persisted and the app never stabilized in steady race mode.
- Chosen fix: in `GameModeSwitcher::frameEnded`, when `WII_NATIVE_ASSET_PIPELINE` is active and current mode is any race mode, force mode lock (`mGameModeNext = mGameMode`, `mIsSwitchMode = false`) to prevent accidental mode-switch churn.
- Also set `mGameModeNext = ModeRaceSingle` after constructor-time Wii race bootstrap to keep mode state internally consistent.
- Rationale: milestone-3 terrain rendering does not require live vehicle physics; race-mode state must stay stable even with physics stubs so terrain render path can run deterministically.

## 2026-04-01 (disable rear-view mirror path in native terrain bring-up)
- Observed that mirror RTT/viewports are unnecessary for milestone-3 terrain diagnostics and can add extra viewport/state churn during Wii native bring-up.
- Decision: under `WII_NATIVE_ASSET_PIPELINE`, disable rear-camera creation and mirror render-target setup in `BaseRaceMode::initCamera` and force mirror panel hidden in `BaseRaceMode::initMisc`.
- Rationale: keep a single stable main-camera render path while isolating terrain visibility; mirror can be restored after terrain path is verified stable.

## 2026-04-01 (switch terrain debug camera from birdseye to start-line chase)
- Reference DM999 screenshot confirms expected start framing: low chase camera over car, looking down track, with canyon walls and road/sand foreground visible.
- Decision: in Wii terrain direct renderer (`WiiGXRenderer::renderTerrainBuffer`), replace birdseye look-at (`+500/+500`) with deterministic chase-style camera derived from PHY start position + normalized car forward vector.
- New debug camera model: place camera behind start point along forward vector (`cameraBackDist`), slightly above track (`cameraHeight`), and look ahead down the track (`lookAheadDist`).
- Rationale: align milestone-3 terrain visibility diagnostics with real gameplay framing, reducing false negatives caused by top-down diagnostic view mismatch.

## 2026-04-01 (suppress probe-color full-screen clears in native runtime)
- Symptom from hardware run: terrain appears briefly, then frame stream reverts to flashing flat rectangles/colors.
- Root cause candidate: numerous `presentProbeColor(...)` calls across init/frame paths perform immediate full-screen clear+present and can overwrite scene output during steady runtime.
- Decision: under `WII_NATIVE_ASSET_PIPELINE`, make `WiiGXRenderer::presentProbeColor(...)` a no-op.
- Rationale: keep debug logging but eliminate framebuffer-clobber side effects so terrain render visibility is not masked by probe clears.

## 2026-04-01 (disable legacy OGRE draw passes in native terrain path)
- Hardware symptom persisted as flashing rectangle/background even after probe-color suppression, indicating another renderer path was still overwriting or conflicting with direct terrain output.
- Root cause candidate: `RenderWindow::update()` still executed legacy `drawMeshEntities()` and `drawManualObjects()` every frame, using a separate fixed camera/GX setup that can replace/obscure the direct terrain pass.
- Decision: under `WII_NATIVE_ASSET_PIPELINE`, skip legacy OGRE mesh/manual draw passes inside `RenderWindow::update()` and keep only `beginFrame()/endFrame()` wrapper.
- Rationale: milestone-3 explicitly uses direct terrain upload+draw; mixing legacy and direct renderers in the same frame creates unstable, non-deterministic output.

## 2026-04-01 (systematic pipeline mapping and blocker classification)
- Per user request, mapped and verified full runtime order and dependencies rather than continuing ad hoc render tweaks:
  - `Main` -> `BaseApp::go/setup` -> `GameModeSwitcher` transition -> `BaseRaceMode::initData` staged pipeline.
  - Stages validated in order: `initScene` -> `loadResources` -> `initTerrain` -> `initModel` -> `initMisc` -> `initLightLists`.
- Kept existing probes and used real Wii `debug.log` to classify failure at dependency level.
- Classification result:
  - mode-switch/init-loop regression is not current blocker,
  - asset/loader path reaches terrain chunk creation with non-zero `triCount`,
  - direct terrain render fails because upload precondition `triIndexes.size() >= triCount` is false for all merged chunks,
  - renderer consequently early-returns with `dataValid=0`.
- Decision: treat this as a mesh-data linkage bug (index propagation into direct upload path), not camera or frame-loop mystery.

## 2026-04-01 (surgical merged-index reconstruction for native direct upload)
- Implemented minimal fix in `StaticMeshProcesser::mergeMSH(...)`: while appending merged triangle vertices in linear order, reconstruct per-triangle indices into `mergedData.triIndexes` using the current merged vertex base (`base, base+1, base+2`).
- Forced explicit parity after each merge bucket update: `mergedData.triCount = mergedData.triIndexes.size()`.
- Left DE2 parsing untouched; this change only fills index data omitted by merge stage so native direct indexed upload has valid payload.
- Added upload confirmation probe in `createMesh(...)` native branch:
  - `[GXDIR] uploading chunk: triIndexes size X == triCount X verts=Y indices=Z`.
- Rationale: unblock terrain direct render path without broad loader refactors; preserves existing merged triangle/vertex ordering semantics.
