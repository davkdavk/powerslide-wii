# Known Issues / Limitations

## 2026-04-02 current blocker update (post white-texture debug run)
- Textured terrain can still regress to visually incorrect output even when batch routing executes.
- In the recent forced per-batch debug run, terrain turned white because binds resolved to placeholder `Loaders/Texture1` instead of populated track textures.
- Root cause class: terrain texture name mismatch/aliasing between chunk material names (e.g., `*.tex`) and actual loaded Wii texture asset variants (`*_m_1.tex`, etc.).
- Mitigation now landed: Wii renderer resolves multiple name variants and only accepts textures with valid pixel payload.
- Validation gap remains until next hardware run confirms placeholder-source binds are eliminated in textured mode.

## 2026-04-02 crash history (now mitigated in current build)
- Repeated DSI-class null-deref crashes were observed and symbolized in:
  - `CloneMaterial(...)` (`orig_src/tools/OgreTools.cpp`),
  - null material-name deref path in `StaticMeshProcesser::createMesh(...)`,
  - `AddjustNormals(...)` (`orig_src/tools/OgreTools.cpp`).
- Guard hardening is now applied, but these sites remain sensitive and should be kept under regression watch in future refactors.

## 2026-04-02 operational risk note
- SD mount/remount churn remains frequent during rapid Wii test loops and can interrupt deploy/log cycles.

## 2026-04-02 current texture-mapping blocker
- Terrain textures are now loading and binding in Wii stable mode, but mapping remains incorrect: one texture pattern repeats across wide geometry regions.
- Per-batch texture selection and UV submission are now wired, but correctness still depends on validating UV quality and expected per-material chunk boundaries in live runs.
- UV safety mode currently clamps UVs and falls back to planar UV on invalid samples; this avoids catastrophic regressions but can still hide fine-grained material mapping errors.
- Temporary clamp-wrap policy reduces full-scene tiling noise for validation but may differ from final intended material wrap behavior.
- Immediate validation dependency: collect real Wii `debug.log` with `[TEX_BIND]` and `[UVDBG]` lines to verify source texture usage and UV ranges per batch.

## 2026-04-02 current blockers after terrain breakthrough
- Primary corruption blocker is resolved: random/stretched triangle chaos is replaced by stable full-map terrain rendering on real Wii.
- Current render path is still a centered debug-space baseline, not yet full gameplay/world-camera parity.
- Depth readability remains approximate (height-color shading); textured terrain/material pipeline is not complete yet.
- Runtime button toggles for camera/texture diagnostics exist, but user-facing behavior still needs refinement/verification in live runs.
- SD mount churn (read-only/unmount) remains an operational risk for reliable deploy/test iteration.

## Milestone risk notes
- M2 camera parity risk: reintroducing world-camera math can regress to non-visible/unstable terrain if done as a hard switch.
- M3 real texture risk: full material path reintroduction may destabilize rendering; maintain procedural/vertex-color fallback until validated.
- M4 runtime re-entry risk: physics/model bring-up can reintroduce null-path crashes if enabled without staged guards.

## 2026-04-01 current blocker snapshot
- Historical blocker `triIndexes size 0 < triCount` is no longer the active front-line issue in latest runs.
- Latest real Wii logs show valid terrain payload and indexed draw execution, but on-screen output is still unstable (moving/stretched/random triangles).
- Latest Dolphin runs diverge from Wii (mostly clear blue/red, no persistent terrain), so Dolphin is currently secondary for final render correctness and Wii remains source of truth.
- Experimental null-physics world-step recovery caused a crash path through `Physics::timeStep`; this path was reverted and should remain disabled until full vehicle/physics init is intentionally re-enabled.
- Active deployment friction: SD card repeatedly remounts read-only, causing intermittent inability to deploy newest build and increasing risk of testing stale binaries.

## Build
- Milestone 1 compile target reached: 97/97 translation units compile.
- Full link now succeeds and a Wii DOL is generated (`wii_build/powerslide_full.elf`, `wii_build/powerslide_full.dol`).

## Runtime expectations (current)
- Even with additional compilation progress, many APIs currently return defaults/no-op behavior and may not reflect OGRE semantics.
- Wii target constraints from `rules.md` (60fps, <80MB, ASND/WPAD integration) are not yet validated.
- Dolphin smoke launch was only a short headless boot check; no gameplay/runtime verification is complete yet.

## Immediate technical blockers
- No remaining TU compile or link blockers in current workspace snapshot.
- Next blocker tier is runtime correctness and performance validation on Wii/Dolphin (input, rendering parity, asset IO, gameplay loop stability).

## Current validation gap
- Runtime fallback/guard changes are now rebuilt and relinked; latest artifact `wii_build/powerslide_full_fix4.dol` runs in Dolphin headless until timeout without immediate invalid memory errors.
- Remaining blocker is visual/runtime fidelity: current result is still a light-blue/blank screen, so scene/resource/gameplay pipeline is not yet rendering visible content.

## New blocker (fix59)
- After disabling all entry-point calls between `BaseApp` construction and `BaseApp::go()`, fix59 still shows cyan (probe from inside `BaseApp::setup`), which means execution reaches `go()` and enters `setup()` but stalls before later setup milestones and before the render loop becomes visible.
- Current suspected stall region is early-mid `BaseApp::setup` (resource/lua/init path) rather than the constructor-to-go gap.

## Crash location (addr2line)
- Crash PCs `0x80174194`, `0x80174198`, `0x8017419c`, `0x801741a0`, `0x801741a4` resolve to libogc WPAD internals in current builds, e.g. `__wpad_read_remote_name_finished` in `wpad.c` (line ~930 in fix63).
- Symptom is invalid writes around `0xffffff8c..0xffffff9c`, consistent with null/invalid pointer state inside asynchronous WPAD callback path.

## Active investigation
- Added diagnostic build `wii_build/powerslide_full_fix5.dol` with a forced debug ground quad in the per-frame Wii render path. Reported result: still light-blue only.
- Built `wii_build/powerslide_full_fix6.dol` with GX cache/state corrections (`DCFlushRange`, larger draw buffers, explicit current matrix) to validate whether primitive submission becomes visible.
- User reported `fix6` still blue-only. Built `wii_build/powerslide_full_fix7.dol` using direct GX vertex submission path to isolate indexed-array pipeline issues.
- User reported `fix7` still blue-only. Built `wii_build/powerslide_full_fix8.dol` with a hardcoded end-of-frame orthographic red quad diagnostic to test whether any submitted geometry reaches the framebuffer before copy/flush.
- Built `wii_build/powerslide_full_fix9.dol` that hard-overrides GX state during end-of-frame quad draw to rule out stale pipeline state blocking visibility.
- Built `wii_build/powerslide_full_fix10.dol` that replaces the renderer setup and draw path with the official devkitPro `gx/triangle` pipeline to establish a known-good GX baseline.
- User confirmed `fix10` shows visible geometry (green band + black). Next blocker: OGRE mesh/material conversion. Added a minimal `ManualObject` -> GX path in `fix11` to surface any manual debug geometry in-engine.
- User reported `fix11` only shows the centered green quad. For `fix12`, forced LLT debug render on Wii and mapped OGRE manual op types to GX line/strip primitives to expose track debug geometry.
- Built `wii_build/powerslide_full_fix13.dol` with Wii auto-switch into `ModeRaceSingle` so LLT debug content is actually created during boot.
- Built `wii_build/powerslide_full_fix14.dol` with a forced `ManualObject` cross inside `LLTLoader::load` to confirm whether the LLT loader path executes.
- Built `wii_build/powerslide_full_fix15.dol` that registers ManualObjects and draws them every frame; this should surface LLT and other debug geometry if created at all.
- Built `wii_build/powerslide_full_fix16.dol` with a forced `ManualObject` cross in `BaseRaceMode::initScene` to confirm race scene initialization is reached.
- Built `wii_build/powerslide_full_fix17.dol` with a `ManualObject` probe in `BaseApp::setup` (post-GameModeSwitcher creation) to verify manual geometry creation independent of race init.
- Built `wii_build/powerslide_full_fix18.dol` with cache flush + current matrix updates in WiiGX draw calls to ensure ManualObject vertex arrays are visible to GX.
- Built `wii_build/powerslide_full_fix19.dol` replacing the debug quad with a devkitPro-style RGB triangle to verify the drawPrimitives path after GX changes.
- Built `wii_build/powerslide_full_fix20.dol` with raw GX triangle draw (verbatim devkitPro sample) to isolate any remaining GX init issues.
- Built `wii_build/powerslide_full_fix10r.dol` after restoring the previously working fix10 pipeline; need to confirm the green band is visible again before proceeding.
- Built `wii_build/powerslide_full_fix21.dol` with ManualObject rendering wired into the fix10 pipeline and a test cross probe; awaiting confirmation of visibility before proceeding to LLT track geometry.
- Built `wii_build/powerslide_full_fix22.dol` restoring the RGB triangle as the only draw call to reconfirm the baseline pipeline.

## Input status (temporary)
- WPAD input is disabled for Dolphin stability pending a real hardware test pass; Classic Controller input wiring will be resumed after rendering milestones are complete.

## Current startup crash (post-WPAD disable)
- Latest black-screen builds (`fix71`, `fix72`) report invalid read `0x00000005` at startup PCs resolving to `build_argv` via addr2line (`??:?`), i.e. crash is occurring in startup argument handling before source-level mapping.
- This is separate from the earlier WPAD callback crash and currently blocks progression past black in these variants.
- `fix74` changed startup symptom to invalid read `0x00000001` at `strnlen` (`??:?`), indicating libc string handling is still receiving malformed startup pointers after argv stubbing.
- `fix75` moved crash to `strchr` (`??:?`) with the same low-address invalid read, indicating the malformed startup-pointer chain is progressing through libc string helpers.
- `fix76` moved crash to `_concatenate_path` (`??:?`) with low-address invalid read, indicating path-assembly startup helpers are still fed malformed pointers in this environment.
- `fix80` rollback (removing libc/startup overrides) returns to original startup crash signature: `build_argv` invalid read `0x00000005` at `PC=0x801ff6d4`.

## Current runtime blockers (fix91+)
- Reported game-loop null crash PC (`0x8015772c`) maps via addr2line+objdump to `orig_src/gamelogic/RacingGridGeneration.cpp:42` (`availableCharacters[...]` chained indexing in `RacingGridGeneration::generate`).
- Root cause class: invalid/short character table data during Wii bring-up causing out-of-range vector access and crash inside inlined `std::vector<std::string>::push_back`.
- Mitigation now in place (`fix92`): guarded index resolution and fallback character path in `RacingGridGeneration::generate`; no immediate recurrence at this site in current short run.
- Post-fix92 visual state is still a neutral clear `(80,80,120)` with no track geometry visible yet; progression through race-scene init/render content remains incomplete.

## Asset-path status (fix112)
- Wii asset lookup order is now explicit in setup:
  1. `sd:/powerslide/` via `mDataDir = "sd:"`
  2. `./powerslide/` via `mDataDir = "."` (Dolphin cwd fallback)
- Local Dolphin fallback payloads exist at `wii_build/powerslide/{data.pf,gameshell.pf,store.pf}`.
- Despite path-order update, runtime probe remains stuck at menu/UI-load stage color `(16,16,136)` in `fix112`, so additional null/loader hardening is still required before race-track geometry appears.

## Runtime crash site (fix113 stack)
- Crash PC `0x800bc64c` (verified by objdump) sits in `BaseRaceMode::initTerrain` particle material setup (`BaseRaceMode.cpp` around lines 504-507), in the chain using `MaterialManager::getByName("Test/Particle*")` followed by `getTechnique()->getPass()->getTextureUnitState()->setTexture(...)`.
- Root cause class: missing material entries returning null/invalid `MaterialPtr` in Wii stub environment, then dereferenced through technique/pass/texture state chain.
- Mitigation in `fix114`: get-or-create fallback material path for `Test/Particle`, `Test/ParticleAlpha`, `Test/ParticleFog`, `Test/ParticleFogAlpha` before dereference.
- Remaining blocker: after this hardening, geometry still not visible; further crash/loader progression remains to be traced in subsequent PCs.

## Current blocker (fix125)
- Assets are confirmed loading and runtime consistently reaches race-init probe `(192,64,0)`.
- Reported new crash PC `0x8005c0fc` maps via addr2line/objdump to inlined `std::string` cleanup inside `PSBaseCar::initModel` around repeated `STRSettings::parse` calls.
- Applied hardening:
  - local PF open path now uses direct `FileStreamDataStream(path, writable)` constructors in `PFLoader` and `OgreTools` (removed `std::ifstream*` handoff pattern),
  - `PSBaseCar::initModel` now bails early on Wii when car params fail to load.
- Current behavior: no new visible crash signature in short/medium runs, but execution stalls at `(192,64,0)` and does not advance to later race-init probes/geometry yet.

## Temporary no-op stubs (Wii bring-up)
- `PSBaseCar::initModel` is currently stubbed as a full no-op on Wii builds to bypass repeated null-member writes at `PC=0x8005c320` (invalid write to offset `0x00000d30` on null `InitialVehicleSetup` base).
- Lost functionality from this stub:
  - no player/AI car physics parameter setup,
  - no wheel/suspension/chassis setup from STR data,
  - no car model initialization from this path,
  - race can continue only in reduced bring-up mode for scene/terrain progression diagnostics.

- `BaseRaceMode` minimal-runtime Wii stubs enabled when terrain init reaches particle stage:
  - in `initTerrain`, once terrain and map loading complete, sets `mWiiMinimalRuntime = true`, forces AI count to zero, and returns early before particle/material binding tail.
  - in `initMisc`, `frameStarted`, `frameRenderingQueued`, `timeStepBefore`, and `timeStepAfter`, returns early when `mWiiMinimalRuntime` is active.
- Lost functionality from these stubs:
  - no race HUD/misc UI runtime updates,
  - no per-frame input/UI/audio race loop work,
  - no car/world timestep integration in minimal mode.
- Preserved functionality (milestone 3 critical path):
  - PF/asset loading,
  - LLT/track terrain scene loading,
  - renderer/GX path,
  - terrain/map pipeline up to early terrain stage probes.

- Additional non-critical stubs now active:
  - `PSPlayerCar::initModel` no-op on Wii.
  - `BaseRaceMode::initModel` early return in minimal-runtime mode after attaching simple geometry probe quad.
- Additional lost functionality:
  - no player car model setup path execution,
  - no normal vehicle visual/physics integration from player-car branch,
  - race presentation remains diagnostic/minimal and not gameplay-complete.

## 2026-03-30 real Wii status
- Confirmed working on real Wii: FAT init, PF loading (`data.pf`, `store.pf`, `gameshell.pf`), LLT parsing (with endian fixes), AI loading (`aiCount=3`), and terrain/particles stages.
- Current crash: immediately after `[RACE] initModel enter` in `BaseRaceMode::initModel`, before model stage completion.
- Active isolation method: line-by-line Wii OSReport probes in `initModel`, then crash-PC resolution with `addr2line` and verification with `objdump` against the matching `fixNN.elf`.
- Constraint retained: stubs may be used for car/arrow/model visuals only; do not stub track geometry, terrain loading, PF loader, or renderer pipeline.

## 2026-03-31 current blocker state (native mesh path)
- Runtime stability is now good in both Wii and Dolphin smoke runs (no recurring panic/fault signature in latest captures), but scene output is still mostly mustard/yellow without real terrain.
- Native loose-file path is active (`sd:/powerslide/wii_data/...`) with baked big-endian assets; this is no longer the primary blocker.
- A real mesh draw path exists and executes from frame loop, but expected terrain/track geometry is still not visibly present.
- LLT debug overlay was intentionally disabled in native mode to avoid masking mesh visibility diagnostics.
- Important new signal: a forced in-pass sanity triangle is visible, which proves GX submission, camera visibility, and this draw pass are fundamentally working.
- Therefore the active blocker has shifted to mesh data flow:
  - entity registration/population,
  - mesh vertex/index availability at draw time,
  - or content not being generated/attached as expected from terrain init.

## 2026-03-31 absolute visibility pass findings
- Brute-force visibility pass (cull/material/camera overrides) still renders only the sanity triangle in framebuffer captures.
- This strongly indicates the terrain triangles are not reaching the final GX submission path as usable mesh batches, despite renderer health.
- Current high-confidence failure domain is now narrowed to:
  - mesh binding in stub scene-manager entity path,
  - terrain mesh population from DE2/MSH into renderable buffers,
  - or draw-loop feeding non-empty entities while terrain remains absent.
- Dolphin OSREPORT output remains mostly empty-line spam, limiting text-based verification of custom probe messages; framebuffer evidence remains the trusted signal.

## 2026-03-31 direct plainBuffer-to-GX findings
- Added direct terrain draw path bypassing OGRE mesh/entity plumbing:
  - `MSHData::plainBuffer` -> `uploadTerrainBufferF32(...)` in mesh creation,
  - `renderTerrainBuffer()` called each native-mode frame.
- Expected result (any grey terrain mass) did not appear; framebuffer remained unchanged (sanity triangle only).
- This narrows active blocker further to one of:
  - `createMesh(...)` not executing in the runtime branch that reaches frame rendering,
  - `mshData.triCount`/`plainBuffer` not populated at execution time,
  - upload path invoked with zero/invalid vertex count due to earlier terrain build conditions.

## 2026-04-01 current blocker (systematic dependency audit)
- Canonical race-mode boot and init sequence is now verified and documented:
  - `BaseApp::setup` -> `GameModeSwitcher` race transition -> `BaseRaceMode::initData`
  - staged order: `initScene` -> `loadResources` -> `initTerrain` -> `initModel` -> `initMisc` -> `initLightLists`.
- Latest real Wii log confirms this order executes and does not loop during init.
- `initScene`/`initTerrain` data-loading path is operational (LLT/PHY/AI/DE2/terrain maps load and non-zero terrain chunk `triCount` is logged).
- Direct terrain render blocker is now precise:
  - each merged chunk is skipped in native upload gate with `triIndexes size 0 < triCount`;
  - renderer logs `dataValid=0` and early returns, so no direct terrain draw occurs.
- This is a data-linkage mismatch between merged terrain chunk representation and direct GX upload expectations (indices absent where upload expects them).

## 2026-03-31 updated direct-terrain blocker state
- Latest real Wii screenshot after the local-space focus experiment still looks essentially unchanged.
- Latest log evidence shows the direct terrain pass is active and stable rather than failing intermittently:
  - `WiiGX: [DATA] Vertex0: -343.682831, 1.375675, -133.109741` repeats frame after frame,
  - `WiiGX: Submitted 7341480 indices for Terrain` repeats on later frame summaries.
- Root-cause findings from code inspection:
  - `uploadTerrainBufferF32(...)` previously replaced the terrain buffer on each upload, so the direct path could only render the last merged terrain chunk.
  - the same path capped terrain storage at `10000` vertices, truncating native terrain data.
  - `renderTerrainBuffer()` used a different ad hoc camera/model-view setup from the working mesh path, so transform parity was poor even when geometry existed.
- Current active risk after the latest fix is no longer DE2 loading or terrain build, but whether the corrected accumulated terrain pass now presents in the expected world location on real Wii.

## Active limitations during this phase
- UI/HUD/start-sequence and parts of vehicle runtime remain stubbed/guarded; current builds are render-bring-up diagnostics, not gameplay-complete.
- Dolphin OSREPORT logging remains noisy and omits useful structured lines at times, so some validation still relies on on-screen probes plus targeted Wii logs.
- SD card mount churn between PC and Wii continues to slow deploy cadence.
