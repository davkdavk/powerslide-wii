# Progress

## 2026-04-01 - current pass update (keep history, refresh latest)

- Kept full historical entries intact; added this top entry as the current state snapshot.
- Confirmed pipeline progression moved past prior index-empty blocker:
  - direct chunk uploads now occur (`[GXDIR] uploading chunk...`),
  - terrain renderer receives valid payload (`dataValid=1`, `verts=61179`, `indices=61179`),
  - indexed draw executes with `invalid=0`.
- Applied render-path hardening in `wii_stubs/OGRE/WiiGXRenderer.cpp`:
  - removed mixed immediate/indexed vertex submission,
  - re-applied vertex descriptor/format state per terrain draw,
  - kept FIFO-safe index emission behavior.
- Tested a temporary sequential-topology draw experiment; result worsened into center-spike/starburst behavior, then reverted back to indexed topology.
- Attempted null-physics recovery by stepping world in null branch; Dolphin crash confirmed this path is unsafe in current init state.
  - Symbolized crash PC: `0x80066bd8` -> vector math in `OgreSceneManager.h` via `Physics::timeStep`.
  - Reverted that recovery attempt.
- Added current Wii-only visibility probe build:
  - fixed yellow test triangle in terrain pass,
  - terrain draw capped to 900 indices,
  - added `maxDrawnIndex` logging.
  - build hash: `971a195fac0247c1532b675b9295c85abe4d6ba3ccb1030a272ea0f069007966`.
- Repository management completed:
  - repo renamed to `davkdavk/powerslide-wii`,
  - recreated as a true fork of `dm999/powerslideremake`,
  - active work branch pushed as `wii-port`.

## 2026-04-01 - systematic runtime pipeline and dependency audit

- Documented canonical race boot/load/render order across `Main` -> `BaseApp` -> `GameModeSwitcher` -> `BaseRaceMode` staged init.
- Confirmed strict `BaseRaceMode::initData` stage order:
  - `initScene` -> `loadResources` -> `initTerrain` -> `initModel` -> `initMisc` -> `initLightLists`.
- Mapped key dependencies:
  - `initTerrain` depends on scene/world and track loaders from `initScene`.
  - native direct terrain draw depends on successful index+vertex upload from `StaticMeshProcesser::createMesh`.
- Real Wii `debug.log` verification against this model:
  - one-time terrain init (`[MESH_PROBE] initParts entry` count = 1),
  - no repeated mode-switch churn,
  - DE2 and chunk creation with non-zero `triCount`,
  - all chunks rejected for direct upload due to `triIndexes size 0 < triCount`,
  - renderer early-returns with `dataValid=0`.
- Updated status focus: active blocker is now isolated to terrain index propagation into native direct upload path, not high-level mode state transitions.

## 2026-04-01 - terrain rendering regression, comprehensive debug logging added

- **Current status: REGRESSED** - User sees only a flashing yellow rectangle, no terrain geometry visible.
- Prior builds showed terrain rendering briefly (geometry flashed, chunks uploaded successfully), but current build shows nothing usable.
- Root cause unknown - possible camera position issue, possible data pipeline regression, or both.
- Added comprehensive debug logging to `renderTerrainBuffer()` to trace the full render path:
  - `[TERRAIN_RENDER] entry` - logs data validity, vertex count, index count
  - `[TERRAIN_RENDER] EARLY RETURN` - logs why render was skipped
  - `[TERRAIN_RENDER] camera at ... looking at ...` - logs exact camera position
  - `[TERRAIN_RENDER] drawing N triangles, first vert=(x,y,z)` - logs draw call details
  - `[TERRAIN_RENDER] draw complete` - confirms draw finished
- Disabled Z-test and back-face culling temporarily to eliminate depth/culling issues.
- Camera uses car start position from PHYLoader as focus point.
- Far clipping plane set to 20000 units.
- Next step: run on Wii, read `[TERRAIN_RENDER]` logs to identify exact failure point.

## 2026-04-01 - indexed terrain rendering pipeline built

- Replaced interleaved buffer approach with direct indexed rendering.
- `uploadTerrainIndexedData()` accepts separate vertex and index arrays.
- Terrain chunks accumulate across all 53 mesh entities.
- Log confirmed chunks uploading: 102 to 26073 vertices per chunk.
- `processPart()` populates `triIndexes` correctly using `TriTex[q].v0/v1/v2` (local deduplicated indices).
- Removed asserts that aborted on zero texCount.
- Added triangle count logging to `processPart()`.
- LOD validation bypassed to continue despite part/LOD mismatch.

## 2026-03-31 - terrain direct-pass accumulation and camera alignment

- Read the latest real Wii `debug.log` after the local-space terrain-focus experiment; framebuffer result was reported as visually unchanged.
- Confirmed from log tail that the direct terrain path is stable but not moving:
  - repeated `WiiGX: [DATA] Vertex0: -343.682831, 1.375675, -133.109741`
  - repeated `WiiGX: Submitted 7341480 indices for Terrain`
- Isolated the active failure domain to the native direct terrain pass in `wii_stubs/OGRE/WiiGXRenderer.cpp`, not DE2 parsing or terrain mesh creation.
- Found three concrete issues in the direct terrain renderer:
  - `uploadTerrainBufferF32(...)` overwrote the terrain vertex buffer on every `createMesh(...)` call, so only the final merged terrain chunk could appear.
  - the direct terrain buffer was capped to `WIIGX_MAX_VERTICES` (`10000`), truncating much of the native terrain payload.
  - `renderTerrainBuffer()` used a separate hardcoded camera path (`0,500,900 -> 0,0,0`) instead of the matrix convention used by the working mesh draw path.
- Applied a minimal renderer fix set:
  - added `clearTerrainBuffer()` and call it once at the start of native `StaticMeshProcesser::initParts(...)`,
  - changed terrain uploads to append into a larger dedicated terrain buffer (`WIIGX_MAX_TERRAIN_VERTICES = 65535`) instead of replacing previous chunks,
  - aligned `renderTerrainBuffer()` to use the same `guLookAt + translate(-50)` model-view convention used by the mesh renderer,
  - preserved a single terrain focus point per full terrain build so all accumulated chunks share one camera target.
- User reported terrain now renders as a lime-green screen (previously a single green fragment), confirming all chunks are visible but camera is too close to the terrain surface.
- Applied camera fix:
  - added `setCarStartPositionF32(...)` to Wii renderer,
  - extracted car start position from `InitialVehicleSetup::mTrackPosition` (Matrix4 column 3) in `BaseRaceMode::initScene` after `PHYLoader`,
  - terrain camera now looks at car start position from `(carX, carY+200, carZ+200)` instead of the terrain centroid.
- Next verification step on real Wii: check whether the display changes from lime-green screen to a visible terrain scene with the camera near the car start line.

## 2026-03-28 - Milestone 1 bring-up (in progress)

- Ran full translation-unit scan for `orig_src/**/*.cpp` with devkitPPC flags.
- Current compile status: **39 / 97 translation units compile**.
- Core real-game TUs still compile: `Main.cpp`, `BaseApp.cpp`, `GameState.cpp`, `InputHandler.cpp`.
- Identified current top blocker clusters:
  - OGRE overlay/UI material/font API gaps (`DisplayString`, `TextAreaOverlayElement::setColour`, `Material` completeness, font manager).
  - Matrix3/scene gaps (`FromAxes`, `SetColumn`, physics/camera math helpers).
  - Remaining DataStream and resource-group behavior parity.

### This milestone step completed
- Added project tracking files: `DECISIONS.md`, `PROGRESS.md`, `ISSUES.md`.
- Re-established compile baseline and blocker priority from current workspace state.

### 2026-03-28 - Overlay/font/movable-text shim pass

- Expanded OGRE compatibility shims across `wii_stubs/OGRE/Ogre.h`, `wii_stubs/OGRE/OgreSceneManager.h`, and `wii_stubs/OGRE/SdkTrays.h`.
- Added/extended compatibility for:
  - `DisplayString` (`asWStr`, `asUTF8`, wide-string assignment support).
  - Shared pointer behavior needed by OGRE-style APIs (`isNull`, `setNull`, `getPointer`).
  - Overlay/UI elements (`TextAreaOverlayElement` alignment/colour/space width/getters/update; panel UV/material getters).
  - Font stack (`ResourceManager`, `ManualResourceLoader`, `Font`, `FontManager`, custom font manager compatibility).
  - Movable text/render plumbing (`Renderable`, `RenderQueue`, vertex declarations/buffers, `RenderOperation` fields).
  - Scene/math helpers (ray type, matrix/vector operators, additional camera/light/scene-node methods).
  - Particle and texture-unit APIs used by car/cheat code (`setEmitting`, emitter/affector APIs, `setTextureScroll`).
- Adjusted `orig_src/customs/MovableText.cpp` to avoid throw with `-fno-exceptions` by returning early if font lookup fails.

### Compile status update

- New full translation-unit scan result: **50 / 97 compile**, **47 / 97 fail**.
- Verified now-compiling previously blocked units include:
  - `orig_src/ui/elements/UIEditBox.cpp`
  - `orig_src/customs/CustomFont.cpp`
  - `orig_src/customs/CustomFontManager.cpp`
  - `orig_src/customs/CustomOverlaySystem.cpp`
  - `orig_src/customs/MovableText.cpp`
  - `orig_src/pscar/PSControllableCar.cpp`
  - `orig_src/ui/UIMainMenuLabels.cpp`

### 2026-03-28 - compile closure pass (97/97)

- Fixed `PixelBox` ordering/type breakage in `wii_stubs/OGRE/Ogre.h` by introducing namespace-level `Ogre::PixelBox` and reusing it from texture pixel buffers.
- Added a broad OGRE compatibility sweep across `wii_stubs/OGRE/Ogre.h` and `wii_stubs/OGRE/OgreSceneManager.h` to satisfy remaining high-frequency API gaps:
  - camera/view/scene helpers (`ProjectionType`, orthographic setup, ambient light, scene clear/destroy camera, viewport sky/clear/background);
  - mesh/material/render plumbing (`SubMesh` alias + `createSubMesh`, index data/material names, pass texture-unit creation, polygon mode, tangent hooks);
  - math helpers (`Vector2/Vector3` operators, `AxisAlignedBox::merge(Vector3)`, quaternion angle-axis constructor, `Vector3::getRotationTo`);
  - render/light/resource event parity (`RenderTargetEvent.source`, `RenderTargetViewportEvent.source`, resource-group listener add/remove);
  - shadow/fog/program enums and pointers (`ShadowCameraSetupPtr` acceptance, fog aliases, scene blend enums, GPU program manager shim);
  - overlay and tray interoperability (`OverlayElement::addChild/removeChild`, `CustomTrayManager` tray-layer compatibility helpers).
- Expanded SFML audio stubs in `wii_stubs/SFML/Audio.hpp` for missing volume APIs (`sf::Sound::getVolume`, `sf::Listener::getGlobalVolume`).
- Added Lua auxlib compatibility shims in `wii_stubs/lauxlib.h` (`luaL_getn`, `luaL_setn`, `luaL_findtable`) and removed Lua internal-header dependency in `orig_src/lua/DMLuaManager.cpp`.
- Eliminated remaining `-fno-exceptions` compile blockers in hot paths by removing try/catch wrappers from affected files:
  - `orig_src/loaders/PFLoader.cpp`
  - `orig_src/loaders/TEXLoader.cpp`
  - `orig_src/tools/OgreTools.cpp`
  - `orig_src/multiplayer/MultiplayerController.cpp`
  - `orig_src/multiplayer/MultiplayerControllerMaster.cpp`
  - `orig_src/multiplayer/MultiplayerControllerSlave.cpp`
  - `orig_src/multiplayer/MultiplayerRoomInfo.cpp`
  - `orig_src/ui/UIMainMenu.cpp`
- Replaced remaining RTTI-dependent `dynamic_cast` usage in scene listeners with `static_cast` in:
  - `orig_src/listeners/PlayerVehicleSceneObjectListener.cpp`
  - `orig_src/listeners/TerrainSceneObjectListener.cpp`
- Added missing OGRE include dependencies in headers that used OGRE types without including OGRE:
  - `orig_src/physics/InitialVehicleSetup.h`
  - `orig_src/gamelogic/Championship.h`
  - `orig_src/loaders/TRALoader.h`
  - `orig_src/sound/MusicProcessor.h`
- Updated PCH include portability for Wii pathing in `orig_src/pcheader.h`.

### Final compile status

- Final full translation-unit scan result: **97 / 97 compile**, **0 / 97 fail**.
- Scan artifact: `wii_build/scan_current5.txt`.

### 2026-03-28 - full link + DOL generation

- Performed first full link using all 97 compiled objects from `wii_build/scan_now5_*.o`.
- Link result: **success** (`wii_build/powerslide_full.elf`).
- Generated Wii DOL from linked ELF: `wii_build/powerslide_full.dol`.
- Verified no unresolved externals in linked ELF (`powerpc-eabi-nm -u` produced no undefined symbols).
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full.elf`):
  - text: `2791512`
  - data: `522909`
  - bss: `188640`
  - dec: `3503061`
  - hex: `3573d5`
- Ran a brief Dolphin headless smoke launch (`dolphin-emu -b -e wii_build/powerslide_full.dol` with timeout); process started and returned without immediate CLI error output.

### 2026-03-28 - runtime black-screen stabilization pass

- Hardened runtime frame lifecycle in `orig_src/BaseApp.cpp` for partial-init scenarios:
  - `go(bool)` now enters a fallback blank-screen render loop when `setup()` fails instead of returning immediately.
  - fallback path creates root/window if needed, registers frame listener, and starts rendering.
  - `frameStarted`, `frameRenderingQueued`, and `frameEnded` now guard null `mWindow` and null `mGameModeSwitcher`.
  - `windowResized` now guards null `mInputHandler` before mouse-state access.
- This is aimed at Milestone 2 behavior (boot + stable blank screen) even when resource/script init fails early.
- Verification is pending in this environment because `powerpc-eabi-g++` is currently unavailable in PATH.

### 2026-03-28 - runtime bring-up after toolchain install

- Confirmed devkitPPC toolchain availability in this environment (`powerpc-eabi-g++ (devkitPPC) 15.2.0`).
- Kept the runtime safety guards/fallback loop and made two additional Wii bring-up improvements:
  - `wii_stubs/OGRE/Ogre.h`: `Root::startRendering()` now runs continuously until frame callbacks request stop (removed fixed 1000-frame cap).
  - `orig_src/Main.cpp`: initialize FAT on Wii at startup (`fatInitDefault`) so `sd:/` asset access is available.
  - `orig_src/BaseApp.cpp` + `orig_src/GameState.h`: on Wii, default `GameState` data dir to `sd:` and retry local fallback only if SD asset init fails.
- Recompiled updated translation units into the active scan object set:
  - `wii_build/scan_now5_orig_src_BaseApp.o`
  - `wii_build/scan_now5_orig_src_Main.o`
- Linked fresh runtime builds with Wii GX renderer object included:
  - `wii_build/powerslide_full_fix2.elf/.dol`
  - `wii_build/powerslide_full_fix3.elf/.dol`
  - `wii_build/powerslide_full_fix4.elf/.dol`
- Latest size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix4.elf`):
  - text: `2370959`
  - data: `387196`
  - bss: `550128`
  - dec: `3308283`
  - hex: `327afb`
- Verified symbol closure on latest runtime artifact (`powerpc-eabi-nm -u wii_build/powerslide_full_fix4.elf` => no unresolved symbols).
- Dolphin headless smoke run for `powerslide_full_fix4.dol` sustained execution until timeout without immediate invalid read/write error output.

### 2026-03-28 - milestone 3 probe (track render visibility)

- Added a Wii GX debug ground draw directly in `Ogre::RenderWindow::update()` (`wii_stubs/OGRE/OgreSceneManager.h`) to validate that visible geometry can be emitted each frame, independent of OGRE scene conversion completeness.
  - Draws a simple green quad (two triangles) with `WiiGX::Renderer::drawIndexedPrimitives` before `endFrame()`.
- Recompiled affected TU and relinked new runtime artifact:
  - `wii_build/powerslide_full_fix5.elf`
  - `wii_build/powerslide_full_fix5.dol`
  - map: `wii_build/powerslide_full_fix5.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix5.elf`):
  - text: `2374679`
  - data: `387452`
  - bss: `550256`
  - dec: `3312387`
  - hex: `328b03`
- This build is intended to differentiate “render loop alive but scene empty” from “no geometry path at all” on real display output.

### 2026-03-28 - GX draw path correction

- Observation from real run: `fix5` still displayed light-blue clear only, meaning forced debug quad was not visible.
- Updated low-level Wii GX renderer (`wii_stubs/OGRE/WiiGXRenderer.cpp`) to improve primitive visibility:
  - expanded aligned vertex/color buffers to `WIIGX_MAX_VERTICES` capacity,
  - removed overly small hard cap (`vcount 100`, `icount 300`) in favor of full configured limits,
  - added `DCFlushRange` for CPU-written vertex/color arrays before drawing,
  - explicitly set active matrix with `GX_SetCurrentMtx(GX_PNMTX0)` after load.
- Recompiled `wii_build/WiiGXRenderer.o` and linked new artifact:
  - `wii_build/powerslide_full_fix6.elf`
  - `wii_build/powerslide_full_fix6.dol`
  - map: `wii_build/powerslide_full_fix6.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix6.elf`):
  - text: `2374899`
  - data: `387500`
  - bss: `649904`
  - dec: `3412303`
  - hex: `34114f`

### 2026-03-28 - GX submission rewrite (indexed array -> direct vertices)

- After user validation that `fix6` remained blue-only, rewrote Wii GX primitive submission path to remove indexed-array dependency and submit direct per-vertex data:
  - `GX_VA_POS`/`GX_VA_CLR0` switched to `GX_DIRECT`.
  - Position attribute format changed to `GX_F32` and emitted with `GX_Position3f32`.
  - Color emitted with `GX_Color4u8` per vertex.
  - `drawIndexedPrimitives` now resolves index -> source vertex and submits directly.
  - strengthened channel/projection state (`GX_SetChanCtrl`, wider near/far clip, dynamic Z mode in `beginFrame`).
- Recompiled renderer object and linked new artifact:
  - `wii_build/powerslide_full_fix7.elf`
  - `wii_build/powerslide_full_fix7.dol`
  - map: `wii_build/powerslide_full_fix7.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix7.elf`):
  - text: `2374743`
  - data: `387468`
  - bss: `549904`
  - dec: `3312115`
  - hex: `3289f3`

### 2026-03-28 - endFrame overlay diagnostic

- User reported `fix7` still showed only light-blue clear.
- Added a hard diagnostic primitive directly in `Renderer::endFrame()` (`wii_stubs/OGRE/WiiGXRenderer.cpp`):
  - switches to orthographic projection,
  - submits a red on-screen quad with direct GX commands immediately before copy/flush.
- This bypasses higher-level scene and draw helper paths to prove whether any non-clear geometry can reach the framebuffer.
- Recompiled and linked artifact:
  - `wii_build/powerslide_full_fix8.elf`
  - `wii_build/powerslide_full_fix8.dol`
  - map: `wii_build/powerslide_full_fix8.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix8.elf`):
  - text: `2375607`
  - data: `387528`
  - bss: `549904`
  - dec: `3313039`
  - hex: `328d8f`

### 2026-03-28 - endFrame overlay (state override)

- Since `fix8` still showed only blue, added explicit GX state overrides in the end-of-frame diagnostic path (disable Z, force color/alpha update, disable blend) and simplified to a triangle strip quad.
- Rebuilt artifact:
  - `wii_build/powerslide_full_fix9.elf`
  - `wii_build/powerslide_full_fix9.dol`
  - map: `wii_build/powerslide_full_fix9.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix9.elf`):
  - text: `2375659`
  - data: `387548`
  - bss: `549904`
  - dec: `3313111`
  - hex: `328dd7`

### 2026-03-28 - devkitPro triangle pipeline parity

- After `fix9` still displayed only blue, rewired Wii GX renderer to match the official devkitPro `gx/triangle` example:
  - uses `GX_INDEX8` arrays (s16 positions, u8 colors),
  - uses post-retrace copy callback (`VIDEO_SetPostRetraceCallback`) and `sReadyForCopy` flag,
  - uses `guLookAt` + translate `-50.0f` to place geometry in front of camera,
  - restores copy/viewport/scissor settings to example defaults.
- Rebuilt artifact:
  - `wii_build/powerslide_full_fix10.elf`
  - `wii_build/powerslide_full_fix10.dol`
  - map: `wii_build/powerslide_full_fix10.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix10.elf`):
  - text: `2445387`
  - data: `401048`
  - bss: `987032`
  - dec: `3833467`
  - hex: `3a7e7b`

### 2026-03-28 - ManualObject -> GX bridge

- User reported `fix10` shows a green band at bottom with black above, confirming GX pipeline now displays submitted geometry.
- Implemented a minimal `ManualObject` draw path for Wii builds:
  - `wii_stubs/OGRE/OgreSceneManager.h` now stores positions/colours during `begin/position/colour/index` and submits them to `WiiGX::Renderer` in `end()`.
  - This enables LLT and other debug/manual geometry to be visible while full OGRE mesh conversion is still pending.
- Tweaked the debug quad in `RenderWindow::update()` to be centered and closer to camera space for quick visibility checks.
- Recompiled and linked artifact:
  - `wii_build/powerslide_full_fix11.elf`
  - `wii_build/powerslide_full_fix11.dol`
  - map: `wii_build/powerslide_full_fix11.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix11.elf`):
  - text: `2445447`
  - data: `401064`
  - bss: `987032`
  - dec: `3833543`
  - hex: `3a7ec7`

### 2026-03-28 - LLT debug visibility push

- User reported `fix11` shows only the centered green quad (no LLT text/debug geometry yet).
- Improvements to surface in-game debug geometry:
  - `wii_stubs/OGRE/OgreSceneManager.h`: `ManualObject` now emits GX line lists/triangle strips based on the requested OGRE operation type.
  - `orig_src/gamemodes/BaseRaceMode.cpp`: forced LLT debug rendering on Wii (bypass Lua flag) to ensure LLT lines/markers are created.
  - `wii_stubs/OGRE/WiiGXRenderer.cpp`: restored clear color in `beginFrame()` for consistent background after render pipeline reset.
- Cleaned duplicate object in build set and relinked:
  - removed `wii_build/scan_now5_orig_src_BaseRaceMode.o` (duplicate object from legacy path).
  - new artifact: `wii_build/powerslide_full_fix12.elf/.dol` (map: `wii_build/powerslide_full_fix12.map`).

### 2026-03-28 - force race mode boot

- To get LLT debug geometry to render, forced Wii builds to switch directly into `ModeRaceSingle` after menu init (`orig_src/gamelogic/GameModeSwitcher.cpp`).
- Recompiled and re-integrated the updated `GameModeSwitcher` object into the scan set and relinked:
  - `wii_build/powerslide_full_fix13.elf`
  - `wii_build/powerslide_full_fix13.dol`
  - map: `wii_build/powerslide_full_fix13.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix13.elf`):
  - text: `2444927`
  - data: `400928`
  - bss: `987032`
  - dec: `3832887`
  - hex: `3a7c37`

### 2026-03-28 - LLT forced debug cross

- Since `fix13` still shows only the debug quad, injected a guaranteed `ManualObject` cross into `LLTLoader::load` before any PF reads.
  - This ensures we can tell whether the LLT loader path is reached at all.
- Rebuilt artifact:
  - `wii_build/powerslide_full_fix14.elf`
  - `wii_build/powerslide_full_fix14.dol`
  - map: `wii_build/powerslide_full_fix14.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix14.elf`):
  - text: `2451663`
  - data: `401028`
  - bss: `1087032`
  - dec: `3939723`
  - hex: `3c1d8b`

### 2026-03-28 - persistent ManualObject draw list

- The forced LLT cross still did not appear; to ensure ManualObject geometry persists beyond creation, added a simple global registry in `wii_stubs/OGRE/OgreSceneManager.h` and render it each frame.
- `ManualObject::end()` now caches converted GX vertices/indices and registers itself; `RenderWindow::update()` calls `drawManualObjects()` every frame.
- Rebuilt artifact:
  - `wii_build/powerslide_full_fix15.elf`
  - `wii_build/powerslide_full_fix15.dol`
  - map: `wii_build/powerslide_full_fix15.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix15.elf`):
  - text: `2452667`
  - data: `400984`
  - bss: `1087064`
  - dec: `3940715`
  - hex: `3c216b`

### 2026-03-28 - race init debug cross

- Since the LLT forced cross still didn't appear, injected a `ManualObject` cross directly inside `BaseRaceMode::initScene` before any loaders to verify whether race scene init is executing.
- Rebuilt artifact:
  - `wii_build/powerslide_full_fix16.elf`
  - `wii_build/powerslide_full_fix16.dol`
  - map: `wii_build/powerslide_full_fix16.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix16.elf`):
  - text: `2452939`
  - data: `400976`
  - bss: `1087064`
  - dec: `3940979`
  - hex: `3c2273`

### 2026-03-28 - BaseApp ManualObject probe

- Race init cross still not visible; added a ManualObject probe immediately after `GameModeSwitcher` creation in `BaseApp::setup` to prove that manual geometry creation works even before any scene setup.
- Rebuilt artifact:
  - `wii_build/powerslide_full_fix17.elf`
  - `wii_build/powerslide_full_fix17.dol`
  - map: `wii_build/powerslide_full_fix17.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix17.elf`):
  - text: `2455559`
  - data: `401068`
  - bss: `1087064`
  - dec: `3943691`
  - hex: `3c2d0b`

### 2026-03-28 - ManualObject cache flush

- ManualObject geometry still did not appear; added explicit cache flush + current matrix setup in `WiiGXRenderer` for both indexed and non-indexed draws.
- Rebuilt artifact:
  - `wii_build/powerslide_full_fix18.elf`
  - `wii_build/powerslide_full_fix18.dol`
  - map: `wii_build/powerslide_full_fix18.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix18.elf`):
  - text: `2455915`
  - data: `401088`
  - bss: `1087064`
  - dec: `3944067`
  - hex: `3c2e83`

### 2026-03-28 - debug triangle sanity check

- ManualObject output still not visible; replaced the per-frame debug quad with a devkitPro-style RGB triangle to confirm the drawPrimitives path is still viable after recent GX changes.
- Rebuilt artifact:
  - `wii_build/powerslide_full_fix19.elf`
  - `wii_build/powerslide_full_fix19.dol`
  - map: `wii_build/powerslide_full_fix19.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix19.elf`):
  - text: `2455819`
  - data: `401092`
  - bss: `1087032`
  - dec: `3943943`
  - hex: `3c2e07`

### 2026-03-28 - raw GX triangle reset

- Stripped `RenderWindow::update()` to the minimal devkitPro triangle draw (raw GX calls only) and moved GX init into `RenderWindow` construction to match sample ordering.
- Rebuilt artifact:
  - `wii_build/powerslide_full_fix20.elf`
  - `wii_build/powerslide_full_fix20.dol`
  - map: `wii_build/powerslide_full_fix20.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix20.elf`):
  - text: `2452883`
  - data: `400712`
  - bss: `526616`
  - dec: `3380211`
  - hex: `3393f3`

### 2026-03-28 - fix10 pipeline restore

- Reverted `WiiGXRenderer.cpp` and `RenderWindow::update()` to the known-working fix10 GX pipeline (GX_INDEX8 arrays + post-retrace copy).
- Rebuilt artifact:
  - `wii_build/powerslide_full_fix10r.elf`
  - `wii_build/powerslide_full_fix10r.dol`
  - map: `wii_build/powerslide_full_fix10r.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix10r.elf`):
  - text: `2452883`
  - data: `400712`
  - bss: `526616`
  - dec: `3380211`
  - hex: `3393f3`

### 2026-03-28 - ManualObject draw path wiring

- Step 1-3 implemented on confirmed fix10 pipeline:
  - `ManualObject::end()` already stages GX_INDEX8-friendly vertex/color arrays; kept that path and left GX init untouched.
  - `RenderWindow::update()` now calls `drawManualObjects()` each frame instead of the hardcoded triangle.
  - Added a test ManualObject cross in `BaseApp::setup()` to validate visibility.
- Rebuilt artifact:
  - `wii_build/powerslide_full_fix21.elf`
  - `wii_build/powerslide_full_fix21.dol`
  - map: `wii_build/powerslide_full_fix21.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix21.elf`):
  - text: `2455095`
  - data: `401060`
  - bss: `1086904`
  - dec: `3943059`
  - hex: `3c2a93`

### 2026-03-28 - RGB triangle restore

- Per instruction, restored the RGB triangle in `RenderWindow::update()` to re-confirm the known-good GX pipeline before adding any ManualObject drawing.
- Removed the test ManualObject cross from `BaseApp::setup()` for this validation phase.
- Rebuilt artifact:
  - `wii_build/powerslide_full_fix22.elf`
  - `wii_build/powerslide_full_fix22.dol`
  - map: `wii_build/powerslide_full_fix22.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix22.elf`):
  - text: `2452007`
  - data: `400924`
  - bss: `987000`
  - dec: `3839931`
  - hex: `3a97bb`

### 2026-03-28 - ManualObject draw loop added (triangle preserved)

- Added a global `gManualObjects` list and a new `drawManualObjects()` that uses the exact same GX_INDEX8 array calls and model/view setup as the RGB triangle.
- `RenderWindow::update()` still draws the RGB triangle first, then calls `drawManualObjects()`.
- Added a test ManualObject white cross in `BaseApp::setup()`.
- Rebuilt artifact:
  - `wii_build/powerslide_full_fix23.elf`
  - `wii_build/powerslide_full_fix23.dol`
  - map: `wii_build/powerslide_full_fix23.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix23.elf`):
  - text: `2456711`
  - data: `401312`
  - bss: `987032`
  - dec: `3845055`
  - hex: `3aabbf`

### 2026-03-28 - ManualObject registration probe

- Added per-second log in `drawManualObjects()` to report `gManualObjects` size.
- Updated test ManualObject to a white triangle with the exact same coordinates as the RGB triangle.
- Rebuilt artifact:
  - `wii_build/powerslide_full_fix24.elf`
  - `wii_build/powerslide_full_fix24.dol`
  - map: `wii_build/powerslide_full_fix24.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix24.elf`):
  - text: `2456915`
  - data: `401336`
  - bss: `987032`
  - dec: `3845283`
  - hex: `3aaca3`

### 2026-03-28 - OSReport diagnostics

- Switched ManualObject diagnostics to OSReport-compatible logging and added explicit probe points:
  - `BaseApp::setup()` logs when called.
  - `ManualObject::end()` logs each registration with total count.
  - `drawManualObjects()` logs current `gManualObjects` size once per second.
- Rebuilt artifact:
  - `wii_build/powerslide_full_fix25.elf`
  - `wii_build/powerslide_full_fix25.dol`
  - map: `wii_build/powerslide_full_fix25.map`
- Size snapshot (`powerpc-eabi-size wii_build/powerslide_full_fix25.elf`):
  - text: `2457695`
  - data: `401528`
  - bss: `987032`
  - dec: `3846255`
  - hex: `3ab06f`

### 2026-03-28 - BaseApp setup probe (fix36)

- Moved the Wii ManualObject probe to immediately after `configure()` in `BaseApp::setup()` to continue the crash hunt.
- Added `wii_build/build_full.sh` to compile+link the full 97-TU build and document it in `rules.md`.
- Rebuilt artifact:
  - `wii_build/powerslide_full_fix36.elf`
  - `wii_build/powerslide_full_fix36.dol`
  - map: `wii_build/powerslide_full_fix36.map`
- Screenshot capture pending (previous pixel checks deprecated).

### 2026-03-29 - BaseApp setup probe (fix37)

- Moved the Wii ManualObject probe to immediately after `registerLuaFunctions()` in `BaseApp::setup()`.
- Rebuilt artifact:
  - `wii_build/powerslide_full_fix37.elf`
  - `wii_build/powerslide_full_fix37.dol`
  - map: `wii_build/powerslide_full_fix37.map`
- Screenshot capture via window crop: `/tmp/wii_test_fix37.png` (triangle visible).

### 2026-03-29 - entry-gap isolation pass (fix59)

- Added explicit probe presentation helper `WiiGX::Renderer::presentProbeColor(...)` and instrumented Wii startup path probes.
- Followed entry-gap isolation request in `orig_src/Main.cpp`: removed all calls between `BaseApp base;` and `base.go(isSafeRun);` except `go()` itself.
- Built and captured:
  - `wii_build/powerslide_full_fix59.elf`
  - `wii_build/powerslide_full_fix59.dol`
  - screenshot: `/tmp/wii_test_fix59.png`
- Observed output remains cyan, indicating execution is reaching `go()`/`setup()` and current stall is not caused by the constructor-to-go entry gap calls.

### 2026-03-29 - WPAD disable + new startup crash loop (fix70-fix72)

- Disabled WPAD startup calls for Dolphin stability:
  - `wii_stubs/OGRE/WiiGXRenderer.cpp`: removed `WPAD_Init()` call path.
  - `wii_stubs/WiiInput.cpp`: disabled `WPAD_Init()` / `WPAD_ScanPads()` runtime path for Wii bring-up.
- Added issue log note that WPAD/Classic Controller input is deferred until after rendering milestones.
- Built and captured:
  - `wii_build/powerslide_full_fix70.elf/.dol` -> screenshot `/tmp/wii_test_fix70.png` (black)
  - `wii_build/powerslide_full_fix71.elf/.dol` -> screenshot `/tmp/wii_test_fix71.png` (black)
  - `wii_build/powerslide_full_fix72.elf/.dol` -> screenshot `/tmp/wii_test_fix72.png` (black)
- Captured new crash PCs from Dolphin log and resolved with addr2line each iteration:
  - fix70: `PC=0x80206518` -> `build_argv` (`??:?`)
  - fix71: `PC=0x80206518` -> `build_argv` (`??:?`)
  - fix72: `PC=0x801ff6d4` -> `build_argv` (`??:?`)
- Current blocker: startup invalid-read crash in `build_argv` before source-mapped game code.

### 2026-03-29 - rollback to stable startup model (fix80)

- Removed experimental libc/startup override files from `wii_stubs` (`argv_stub`, `strnlen_stub`, `strchr_stub`, `concatenate_path_stub`, `checkargv_stub`) because that branch remained black and obscured telemetry.
- Clean rebuilt and archived:
  - `wii_build/powerslide_full_fix80.elf`
  - `wii_build/powerslide_full_fix80.dol`
  - `wii_build/powerslide_full_fix80.map`
- Screenshot: `/tmp/wii_test_fix80.png` (black).
- Crash re-captured and resolved:
  - Dolphin: `Invalid read from 0x00000005, PC = 0x801ff6d4`
  - addr2line: `build_argv` / `??:?`
- Current status: startup crash loop is back to original `build_argv` signature, with GX red baseline still available in earlier stable branch (`fix67`) and no further backward test runs planned.

### 2026-03-29 - NDEBUG assert bypass test (fix90)

- Updated Wii build flags in `Makefile` to include `-DNDEBUG` so `assert()` checks are compiled out for Wii bring-up builds.
- Performed clean rebuild (`make clean && make`) and archived outputs:
  - `wii_build/powerslide_full_fix90.elf`
  - `wii_build/powerslide_full_fix90.dol`
  - `wii_build/powerslide_full_fix90.map`
- Captured screenshot via batch Dolphin loop:
  - `/tmp/wii_test_fix90.png`
- Visual result remains solid green in the current probe pipeline (dominant color `(0,255,0)`).

### 2026-03-29 - deeper setup probes + null crash hardening (fix91-fix92)

- Added deeper Wii probe colors through `BaseApp::setup`/`configure` path in `orig_src/BaseApp.cpp` to identify exact progression beyond the initial green setup probe.
- Built and archived `fix91`:
  - `wii_build/powerslide_full_fix91.elf`
  - `wii_build/powerslide_full_fix91.dol`
  - `wii_build/powerslide_full_fix91.map`
- Screenshot captures for `fix91`:
  - `/tmp/wii_test_fix91.png`
  - `/tmp/wii_test_fix91_12.png`
- `fix91` dominant color was `(0,128,255)`, matching the probe placed immediately after `mInputHandler` + scene-manager-factory setup in `BaseApp::configure`.
- Applied requested crash triage workflow on reported `PC=0x8015772c` using both addr2line and objdump on `fix91`:
  - addr2line mapped into `RacingGridGeneration::generate(...)` line 42 (`res.push_back(availableCharacters[...])` inlined into vector push_back)
  - source-annotated objdump around `0x8015772c` confirmed the same call-site block.
- Implemented bounds/fallback guards in `orig_src/gamelogic/RacingGridGeneration.cpp` to avoid invalid chained indexing when AI/character arrays are empty or shorter than expected.
- Built and archived `fix92`:
  - `wii_build/powerslide_full_fix92.elf`
  - `wii_build/powerslide_full_fix92.dol`
  - `wii_build/powerslide_full_fix92.map`
- Screenshot captures for `fix92`:
  - `/tmp/wii_test_fix92.png`
  - `/tmp/wii_test_fix92_12.png`
- `fix92` dominant color shifted to neutral runtime clear `(80,80,120)`, indicating execution moved past the prior `fix91` stall point and into the active render loop path without the immediate null/index crash.

### 2026-03-29 - asset fallback order update (fix112)

- Updated Wii asset-path selection in `orig_src/BaseApp.cpp`:
  - first attempt: `mGameState.setDataDir("sd:")` (real Wii `sd:/powerslide/`)
  - fallback attempt: `mGameState.setDataDir(".")` (Dolphin cwd-relative `./powerslide/`)
- Verified local `.pf` payloads are present for Dolphin fallback:
  - `wii_build/powerslide/data.pf`
  - `wii_build/powerslide/gameshell.pf`
  - `wii_build/powerslide/store.pf`
- Built and archived:
  - `wii_build/powerslide_full_fix112.elf`
  - `wii_build/powerslide_full_fix112.dol`
  - `wii_build/powerslide_full_fix112.map`
- Captured screenshot:
  - `/tmp/wii_test_fix112.png`
- Screenshot sampling for `fix112`:
  - center `(16,16,136)`
  - top-right `(16,16,136)`
  - bottom-right `(16,16,136)`
  - left-side samples black due to current viewport/crop layout
- Result: execution still stalls in the same early menu/UI load stage (probe `(16,16,136)`), so this path-order fix alone did not yet surface track geometry.

### 2026-03-29 - crash stack triage + top-PC fix (fix113-fix114)

- Added requested asset-open indicator for diagnostics:
  - `Ogre::gAssetsLoaded` set in `PFLoader::init` when `data.pf` stream opens successfully.
  - `RenderWindow::update()` now paints orange `(255,165,0)` when `gAssetsLoaded == false`, otherwise default blue/grey.
- Built and archived diagnostic build:
  - `wii_build/powerslide_full_fix113.elf/.dol/.map`
  - Screenshot: `/tmp/wii_test_fix113.png`
- Received Dolphin exception stack and ran full addr2line/objdump triage against fix113.
- Top crash PC `0x800bc64c` was verified in objdump to sit inside `BaseRaceMode::initTerrain` at particle-material setup (`BaseRaceMode.cpp` lines ~504-507), indicating null/invalid material object path.
- Applied top-PC fix:
  - added get-or-create fallback for `Test/Particle*` materials in `BaseRaceMode::initTerrain` before `getTechnique/getPass/getTextureUnitState` chain.
- Built and archived:
  - `wii_build/powerslide_full_fix114.elf`
  - `wii_build/powerslide_full_fix114.dol`
  - `wii_build/powerslide_full_fix114.map`
- Captured screenshots:
  - `/tmp/wii_test_fix114.png`
  - `/tmp/wii_test_fix114_20.png`
- Current visual status remains non-geometry probe/crash colors (no track geometry visible yet), but top crash site hardening is now in place and no longer relies on missing material assumptions.

### 2026-03-29 - repeated 0x800c7df4 crash handled with lifecycle no-op (fix119)

- Investigated repeated crash `PC=0x800c7df4` with addr2line+objdump on latest pre-fix build (`fix118`):
  - `addr2line` => `BaseMenuMode::clearData` (`orig_src/gamemodes/BaseMenuMode.cpp:141`)
  - `objdump` confirms null dereference of `mSceneMgr` inside menu teardown.
- Applied different fix strategy (not another material check):
  - in `BaseMenuMode::clearData`, early-return no-op on Wii when `mSceneMgr == NULL`.
  - this stubs non-critical menu teardown path during bring-up when menu camera/scene was never created.
- Added supporting transition hardening:
  - forced race switch-state fields before constructor-time `frameEnded()` call in `GameModeSwitcher`.
  - null-guarded loader/unloader pointer uses in `GameModeSwitcher::frameEnded` and `loadState`.
  - inserted deeper progress probes in `BaseRaceMode::initData`.
- Built and archived:
  - `wii_build/powerslide_full_fix119.elf`
  - `wii_build/powerslide_full_fix119.dol`
  - `wii_build/powerslide_full_fix119.map`
- Screenshots captured:
  - `/tmp/wii_test_fix119_t3.png`
  - `/tmp/wii_test_fix119_t6.png`
  - `/tmp/wii_test_fix119_t9.png`
  - `/tmp/wii_test_fix119_t12.png`
  - `/tmp/wii_test_fix119_t20.png`
- Dominant color for all samples: `(192,64,0)` (new race-init probe), indicating progression beyond previous repeated crash point.

### 2026-03-29 - path logic switched to same-folder PF layout (fix124)

- Updated asset path logic to remove mandatory `powerslide/` subfolder for local data lookups:
  - `PFLoader` now resolves local files as `<dataDir>/<file>.pf` (no extra subdirectory).
  - `OgreTools` writable/readable helper paths updated to match the same no-subfolder convention.
- Updated Wii setup path priority in `BaseApp`:
  - first try `sd:/powerslide/` via `mDataDir = "sd:"`,
  - fallback to local same-folder path via `mDataDir = "."` so files can be read as `./data.pf`, `./gameshell.pf`, `./store.pf`.
- Copied `.pf` files into `wii_build/` alongside `.dol` for Dolphin test layout:
  - `wii_build/data.pf`
  - `wii_build/gameshell.pf`
  - `wii_build/store.pf`
- Built and archived:
  - `wii_build/powerslide_full_fix124.elf`
  - `wii_build/powerslide_full_fix124.dol`
  - `wii_build/powerslide_full_fix124.map`
- Captured screenshots:
  - `/tmp/wii_test_fix124.png`
  - `/tmp/wii_test_fix124_14.png`
- Current observed probe remains `(192,64,0)` (race-init progression marker).

### 2026-03-29 - stocktake + 0x8005c0fc triage (fix125)

- Stocktake status:
  - Assets are now treated as same-folder with `.dol` for Dolphin (`./data.pf`, `./gameshell.pf`, `./store.pf`).
  - Asset files are present in `wii_build/` and setup path order remains SD first, local fallback second.
  - Runtime has progressed out of earlier menu teardown crash loop and is consistently in race init path.
- Crash triage performed for reported PC `0x8005c0fc`:
  - `addr2line` on `fix124` maps to `std::string::_M_is_local` inlined during `PSBaseCar::initModel`.
  - `objdump -dSl` context places the PC in `PSBaseCar.cpp` around repeated `STRSettings::parse(...)` string temp cleanup.
- Hardening applied after triage:
  - `PFLoader` local file opens switched to `Ogre::FileStreamDataStream(path, writable)` direct path constructor instead of `std::ifstream*` handoff path.
  - `OgreTools` read/write helpers updated to same direct `FileStreamDataStream(path, writable)` pattern.
  - Added Wii guard in `PSBaseCar::initModel`: if car params fail to load, skip car init instead of proceeding with invalid state.
- Built and archived:
  - `wii_build/powerslide_full_fix125.elf`
  - `wii_build/powerslide_full_fix125.dol`
  - `wii_build/powerslide_full_fix125.map`
- Screenshots captured:
  - `/tmp/wii_test_fix125.png`
  - `/tmp/wii_test_fix125_14.png`
  - `/tmp/wii_test_fix125_22.png`
  - `/tmp/wii_test_fix125_t3.png`
  - `/tmp/wii_test_fix125_t6.png`
  - `/tmp/wii_test_fix125_t9.png`
  - `/tmp/wii_test_fix125_t12.png`
  - `/tmp/wii_test_fix125_t15.png`
  - `/tmp/wii_test_fix125_t18.png`
  - `/tmp/wii_test_fix125_t21.png`
  - `/tmp/wii_test_fix125_t24.png`
- Current dominant color remains `(192,64,0)` through 24s, indicating stable race-init entry but stall before later probes (`192,96+`).

### 2026-03-29 - initScene/initTerrain stall localization (fix127)

- Reviewed `BaseRaceMode::initScene` and `initTerrain` for blocking constructs:
  - no `while` loops,
  - no `sleep/usleep/std::this_thread::sleep_for` calls,
  - no explicit wait-condition loops in these two functions.
- Added requested probe markers to localize stall region:
  - magenta `(255,0,255)` at `initScene` entry (before scene setup)
  - yellow `(255,255,0)` after `PHYLoader` stage in `initScene`
  - cyan `(0,255,255)` at end of `initScene` (before return)
  - white `(255,255,255)` at `initTerrain` entry
- Built and archived:
  - `wii_build/powerslide_full_fix127.elf`
  - `wii_build/powerslide_full_fix127.dol`
  - `wii_build/powerslide_full_fix127.map`
- Screenshots:
  - `/tmp/wii_test_fix127.png`
  - `/tmp/wii_test_fix127_16.png`
- Observed color: `(192,96,0)` (existing `BaseRaceMode::initData` post-`loadResources` probe), indicating execution has progressed past the prior `(192,64,0)` point and is now stalling after `loadResources` and before next later-stage race init probes complete.

### 2026-03-29 - deep drill into initTerrain and 0x8005c320 full-function stub (fix128-fix131)

- Added fine-grained probes inside `BaseRaceMode::initTerrain` and stage boundary before `initTerrain` call in `initData`.
- Localization result:
  - `fix128` reached `(0,60,255)` consistently (inside particle sub-block in `initTerrain`).
  - This isolated stall/crash zone to post-`TEXLoader().load(...)` + subsequent car/model path transition.
- Applied temporary safe stub in `initTerrain` for Wii to bypass particle material-binding path and force continuation signal.
- User then reported repeated hard crash signature at a later stage:
  - invalid write to `0x00000d30`, `PC=0x8005c320`.
- Per user instruction, performed addr2line+objdump on `fix130` and then took full no-op approach:
  - Function containing crash PC: `PSBaseCar::initModel(...)`.
  - Stubbed entire function to early return on Wii builds.
- Built and archived requested build:
  - `wii_build/powerslide_full_fix131.elf`
  - `wii_build/powerslide_full_fix131.dol`
  - `wii_build/powerslide_full_fix131.map`
- Captured screenshots for requested build:
  - `/tmp/wii_test_fix131_rerun.png`
  - `/tmp/wii_test_fix131_rerun_16.png`
- Current dominant probe color after full-function stub: `(0,24,255)` (entered `BaseRaceMode::initModel` without immediate `0x8005c320` write crash).

### 2026-03-29 - stub policy alignment + minimal runtime path (fix133)

- Applied user-directed stubbing policy:
  - freely stubbed non-milestone systems (car model/physics runtime path, AI runtime, UI/audio runtime updates),
  - preserved milestone-critical systems (PF loading, LLT/terrain loading, renderer/GX code).
- Implemented Wii minimal-runtime gates in `BaseRaceMode`:
  - `mWiiMinimalRuntime` set in `initTerrain` after core terrain stages,
  - early returns in `initMisc`, `frameStarted`, `frameRenderingQueued`, `timeStepBefore`, `timeStepAfter` when minimal mode enabled,
  - `AICountInRace` forced to zero in minimal mode to avoid AI path entry.
- Built and archived:
  - `wii_build/powerslide_full_fix133.elf`
  - `wii_build/powerslide_full_fix133.dol`
  - `wii_build/powerslide_full_fix133.map`
- Screenshots:
  - `/tmp/wii_test_fix133.png`
  - `/tmp/wii_test_fix133_16.png`
  - `/tmp/wii_test_fix133_24.png`
- Current color remains `(0,24,255)` (initModel entry probe), indicating progression survives prior crash site while non-critical runtime systems are now suppressed.

### 2026-03-29 - forward progression with aggressive non-critical stubs (fix134-fix136)

- Continued under explicit stub policy:
  - kept track/terrain/LLT/PF/GX paths intact,
  - stubbed non-critical vehicle/runtime branches for Wii bring-up.
- Added geometry probe object creation in `BaseRaceMode::initModel` when minimal mode is active (large white quad attached to race root node) and returned early to bypass remaining car-model path.
- Added Wii no-op for `PSPlayerCar::initModel` (alongside existing `PSBaseCar::initModel` stub) to remove remaining vehicle-model init dependency.
- Built and archived:
  - `wii_build/powerslide_full_fix134.elf/.dol/.map`
  - `wii_build/powerslide_full_fix135.elf/.dol/.map`
  - `wii_build/powerslide_full_fix136.elf/.dol/.map`
- Screenshots captured:
  - `/tmp/wii_test_fix134.png`, `/tmp/wii_test_fix134_16.png`, `/tmp/wii_test_fix134_24.png`
  - `/tmp/wii_test_fix135.png`, `/tmp/wii_test_fix135_16.png`, `/tmp/wii_test_fix135_24.png`
  - `/tmp/wii_test_fix136.png`, `/tmp/wii_test_fix136_16.png`, `/tmp/wii_test_fix136_24.png`
- Observed probe progression:
  - fix134 remained at `(0,24,255)`
  - fix135 reached `(0,2,255)`
  - fix136 reached `(0,10,255)`
- Interpretation: execution now advances deeper into post-terrain minimal path and reaches geometry-probe placement point, but rendered output remains probe-color-dominant and no stable visible track geometry frame yet.

### 2026-03-30 - real Wii milestone progression and current blocker

- Real Wii logs now confirm the following pipeline completes in order:
  - FAT + SD access,
  - PF mounts (`data.pf`, `gameshell.pf`, `store.pf`),
  - LLT load/parse (including first-vertex output),
  - AI load (`aiCount=3`),
  - terrain + particles stages.
- Current blocker moved to early `BaseRaceMode::initModel` (crash immediately after `[RACE] initModel enter`).
- Added line-level Wii probe logs inside `initModel` before:
  - arrow texture load,
  - player lap-controller registration,
  - models-pool initialization.
- Built latest diagnostic artifact for this path isolation:
  - `wii_build/powerslide_full_fix173.elf`
  - `wii_build/powerslide_full_fix173.dol`
  - `wii_build/powerslide_full_fix173.map`

### 2026-03-31 - native loose-file pipeline + mesh render isolation

- Shifted active runtime path to native loose files (`WII_NATIVE_ASSET_PIPELINE`) and validated repeated loading from `sd:/powerslide/wii_data/...`.
- Mirrored SD data layout into Dolphin sync folder (`wii_build/powerslide/wii_data`) to align emulator pathing with Wii.
- Added/iterated real mesh draw path in `wii_stubs/OGRE/OgreSceneManager.cpp`:
  - entity registry draw (`Entity` -> `Mesh` -> `SubMesh` -> index buffer),
  - 16-bit GX index submission,
  - batching for large index counts,
  - world-position offset application.
- Updated `wii_stubs/OGRE/WiiGXRenderer.cpp` to use `GX_INDEX16` submission for mesh-oriented paths.
- Disabled LLT overlay rendering in native mode (to prevent centerline/debug visuals from hiding mesh diagnosis).
- Forced mesh draw execution from runtime loop in `orig_src/gamemodes/BaseRaceMode.cpp` (`Ogre::drawMeshEntities()` in native mode) to decouple from uncertain window-update timing.
- Added mesh-path diagnostics and a guaranteed sanity primitive in mesh pass:
  - per-pass counters (`entities`, `drawn`, `skipped`, missing mesh/VD/POS/VB),
  - bright sanity triangle rendered in the same pass.
- User validation result: **colored sanity triangle is visible**.
  - This confirms the current GX pass/camera can display primitives.
  - Current blocker therefore narrowed to terrain mesh content flow (entity/data population/registration), not base GX visibility.

### Latest build/runtime snapshot

- Current built artifact:
  - `wii_build/powerslide.dol` (latest size observed: `2553772` bytes, Mar 31 08:12).
- Latest Dolphin fault-capture runs remain crash-clean:
  - e.g. `wii_build/dolphin_faults/run_20260331_081212.faults.log` has no fault lines.

### 2026-03-31 - absolute mesh visibility/cull bypass diagnostic pass

- Implemented brute-force mesh visibility diagnostics in Wii stubs:
  - Camera stub now returns visible for AABB checks (`Camera::isVisible(...)`),
  - Mesh pass enforces opaque TEV color pass and explicit GX state setup,
  - Added mesh-name -> mesh binding support for `createEntity(name, meshName, group)` via deferred resolution,
  - Added vertex-buffer probe path (`NULL` vs first-vertex data) and submitted-index accounting logs,
  - Added terrain attach log in `StaticMeshProcesser::initPart`,
  - Added native-mode camera tether in `BaseRaceMode::frameStarted`.
- Built and validated crash stability after these changes:
  - `wii_build/powerslide.dol` (`2562636` bytes, Mar 31 10:01),
  - Dolphin fault run `wii_build/dolphin_faults/run_20260331_100153.faults.log` remained clean.
- Framebuffer captures after this pass still show only sanity triangle on mustard background (no terrain footprint), confirming mesh visibility is still blocked upstream of final triangle submission for terrain.
