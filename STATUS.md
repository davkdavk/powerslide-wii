# STATUS REPORT - 2026-04-02

## 2026-04-02 latest status (current)

- Runtime stability improved versus prior hour's crashes: previously repeated null-deref faults in material/normal paths were patched and latest runs proceed to terrain rendering.
- New QoL behavior is active on Wii: hardware RESET/POWER now returns to loader/menu (no power-cycle required).
- Current visual blocker is now strictly textured-terrain correctness:
  - textured mode can still appear globally wrong (sand-everywhere baseline),
  - debug forced per-batch texture mode produced all-white terrain due to placeholder texture source usage.
- Log-backed diagnosis from latest run:
  - `[TEX_BATCH]` confirms per-batch texture routing code executes,
  - `[TEX_BIND] ... source='Loaders/Texture1'` in white run confirmed placeholder source was being sampled,
  - mixed `hasUV=1` and `hasUV=0` batches are present; UV path is partially valid but not final-correct.
- Latest code change (now deployed): terrain texture-name resolution now tries chunk-name variants (`_m_1/_m_2/_m_3/.tga/.png`) and only accepts populated texture payloads.
- Current immediate verification target on hardware:
  - with forced debug cycling disabled, confirm whether textured mode returns to non-white output and whether mapping improves beyond sand-dominant appearance.

- Real Wii texture path now binds loaded terrain textures in stable mode; user confirmed visible texture content (no longer flat shading only).
- Current active blocker changed from texture-load failure to mapping correctness:
  - texture pattern repeats broadly across geometry when texture mode is enabled.
- Per-batch terrain metadata and UV path were reintroduced in Wii direct renderer:
  - upload path now records chunk batch ranges and texture names,
  - render path iterates chunk batches and binds textures per batch.
- Native mesh upload now sends explicit per-index UVs through `uploadTerrainIndexedDataWithUV(...)`.
- UV safety guardrails are active in stable path to prevent prior white-screen regressions:
  - sanity/range checks,
  - clamped UV output,
  - planar fallback for invalid UV samples.
- Added new runtime diagnostics in `debug.log` for source-of-truth validation:
  - `[TEX_BIND] requested=... source=...`
  - `[UVDBG] batch=... rangeU/rangeV ... invalidUV=...`
- Build status: `make -j2` passes and generates current `wii_build/powerslide.dol`.

- Real Wii milestone reached: full track mesh is now visible and recognizable on hardware.
- Stable render baseline is active in `wii_stubs/OGRE/WiiGXRenderer.cpp` using centered camera-space terrain transform (`v - center`, fixed forward offset) with indexed triangle draw.
- Full draw budget is enabled (`drawCountStable = mTerrainProbeDrawCount`), and user confirmed stability at full map scale.
- Depth readability improved without wireframe by applying height-based vertex color gradient (safe path; no wireframe line-emulation).
- Camera movement controls are re-enabled in stable mode (D-pad X/Z, `A/B` Y).
- Runtime state toggles were added (`+/-/1/2`) for camera/texture diagnostics; mode logging is present in `debug.log`.
- Data contracts remain healthy in latest runs: valid payload, finite vertices, index range in bounds; blocker is no longer geometry corruption.
- Next focus: transition from centered debug camera toward gameplay/world camera while preserving this stable baseline, then texture path bring-up.

## Milestone tracker (next)

- M1 - Stable visual baseline: complete.
- M2 - Camera parity: in progress.
  - Goal: gameplay/world camera mode behaves correctly while centered fallback remains available.
- M3 - Real terrain textures: pending.
  - Goal: replace procedural/checker terrain sampling with real track texture mapping.
- M4 - Race runtime re-entry: pending.
  - Goal: re-enable car/model/physics flow on top of stable terrain renderer without regressions.
- M5 - Performance and cleanup: pending.
  - Goal: reduce debug-only paths, keep one known-good fallback mode, and validate Wii performance envelope.

## 2026-04-01 latest status (current)

- Terrain data upload path is now active in native mode on real Wii logs:
  - `[GXDIR] uploading chunk...` present.
  - `[TERRAIN_RENDER] entry dataValid=1 verts=61179 indices=61179` present.
  - `[TERRAIN_RENDER] indexed topology ... invalid=0` present.
  - `[TERRAIN_RENDER] draw complete` present.
- Current real Wii visual symptom: geometry is not stable terrain; user reports moving/stretched/random triangles.
- Current Dolphin symptom: usually clear blue/red with no persistent terrain polys; one run hit a crash screen during an experimental null-physics recovery change.
- Crash root cause from that experiment was symbolized and confirmed:
  - `PC=0x80066bd8` -> `Ogre::Vector3::operator-` in `wii_stubs/OGRE/OgreSceneManager.h:82`, called from `Physics::timeStep` (`orig_src/physics/Physics.cpp`).
  - The risky null-physics `timeStep` recovery in `BaseRaceMode::frameStarted` was reverted.
- Current active test build (not yet confirmed on Wii due SD read-only remount churn):
  - adds a fixed yellow on-screen test triangle in `renderTerrainBuffer()`,
  - limits terrain draw to first 900 indices (300 triangles),
  - logs `maxDrawnIndex` for the drawn subset.
  - local hash: `971a195fac0247c1532b675b9295c85abe4d6ba3ccb1030a272ea0f069007966`.
- Current blocker is no longer missing indices; it is final render correctness on real hardware (likely transform/state/space mismatch under Wii GX runtime conditions).

## 0. Canonical Runtime Order (Systematic)

This is the normal in-engine order for a race boot in this branch:

1. `Main` -> `BaseApp::go()` (`orig_src/Main.cpp`, `orig_src/BaseApp.cpp:338`)
2. `BaseApp::setup()` builds Root/window/input/resources and creates `GameModeSwitcher` (`orig_src/BaseApp.cpp:380`)
3. `GameModeSwitcher` transitions to race mode on Wii native (`orig_src/gamelogic/GameModeSwitcher.cpp`)
4. `BaseRaceMode::initData()` executes strict stage order (`orig_src/gamemodes/BaseRaceMode.cpp:67`):
   - `initScene()`
   - `loadResources()`
   - `initTerrain()`
   - `initModel()`
   - `initMisc()`
   - `initLightLists()`
5. `BaseRaceMode::initCamera()` sets viewport/camera (`orig_src/gamemodes/BaseRaceMode.cpp:194`)
6. Per-frame loop: `BaseApp::{frameStarted,frameRenderingQueued,frameEnded}` -> `GameModeSwitcher` -> active mode frame methods (`orig_src/BaseApp.cpp:502`, `519`, `536`)

## 0.1 Stage Linkage And Dependencies

- `initScene()` produces scene manager/world + LLT + exclusions + PHY start transforms + AI + particles; all later stages depend on this.
- `initTerrain()` builds DE2 terrain parts via `StaticMeshProcesser::initParts`, then terrain maps/terrain metadata.
- `initModel()` initializes player/AI model+physics path (currently partially stubbed in native bring-up).
- `initMisc()` and `initCamera()` finalize race presentation runtime.
- Native direct terrain renderer depends on `StaticMeshProcesser::createMesh(...)` successfully calling `uploadTerrainIndexedData(...)`.

## 0.2 Current Failure Link (From Real Wii Log)

- `init` loop is no longer repeating (`[MESH_PROBE] initParts entry` appears once).
- `initTerrain` and DE2 parsing complete successfully with non-zero chunk triangle counts.
- Direct upload gate fails for every merged chunk:
  - `[GXDIR] skipping chunk: triIndexes size 0 < triCount ...` (53 times)
- Therefore renderer receives no direct terrain payload:
  - `[TERRAIN_RENDER] entry dataValid=0 verts=0 indices=0`
  - `[TERRAIN_RENDER] EARLY RETURN...`

Conclusion: current blocker is not mode switching and not asset loading; it is the index dependency mismatch in direct terrain upload path.

## 1. CURRENT STATE

**On screen:** Flashing yellow rectangle. No terrain geometry visible.

**Latest build:** `boot.dol` size `2673484` — may not be deployed (SD was read-only during last build).

**What the last confirmed working build showed:**
- Terrain geometry flashed briefly (colored triangles visible)
- Camera position was wrong — geometry appeared then disappeared
- Log confirmed chunks uploading: `uploaded terrain vertices=102` through `26073`
- `processPart` was populating data: `part[0] processed verts=139 tris=186`

**Current log shows:** Every chunk skipped with `triIndexes size 0 < triCount X` — meaning the build running on Wii is an older one where `processPart()` didn't populate triangle indices.

---

## 2. DE2 INDEX FORMAT FINDINGS

**Triangle structure in DE2 file (per-triangle, 6× u16 fields = 12 bytes):**
- `v0, v1, v2` — vertex indices into global vertex pool
- `t0, t1, t2` — texcoord indices
- `hz0, hz1` — material/decal flags + texture path index

**What we found:**
- The DE2 file stores **global vertex indices** (referencing the full 15616-vertex pool)
- `processPart()` deduplicates vertices into a **local per-chunk vertex array** via `FindVertex()`
- The local indices from `FindVertex()` are stored in `TriTex[q].v0/v1/v2`
- **Bug was:** `mshData.triIndexes` was being populated with GLOBAL indices (v0,v1,v2 from the file) instead of LOCAL indices (TriTex[q].v0 etc.)
- **Fix applied:** Changed line 842-844 to use `TriTex[q].v0/v1/v2` (local deduplicated indices)

**Stride confirmed:** Each triangle = 12 bytes (6× u16). Vertex = 12 bytes (3× f32).

**Endianness:** File is little-endian. Wii is big-endian. `__builtin_bswap16/32` used for LE reads.

---

## 3. WHAT WAS JUST CHANGED (Last 5 Builds)

| Build | Files Changed | Reason |
|-------|--------------|--------|
| Fix 1 | `DE2Loader.cpp` | Changed triangle indices from global to local (`TriTex[q].v0` instead of raw `v0`) |
| Fix 2 | `DE2Loader.cpp` | Removed `assert(texCount != 0)` that aborted on missing textures; added `numTriangles` logging |
| Fix 3 | `DE2Loader.cpp` | Bypassed LOD validation check that was aborting with "invalid part/LOD state" |
| Fix 4 | `WiiGXRenderer.cpp` | Far clipping plane 5000→20000; camera distance 3000→800→500 units |
| Fix 5 | `WiiGXRenderer.cpp` | Added comprehensive `[TERRAIN_RENDER]` debug logging; disabled Z-test and culling |

**Also changed earlier but stable:**
- `WiiGXRenderer.h/cpp` — Indexed rendering path (`uploadTerrainIndexedData`)
- `WiiGXRenderer.h/cpp` — Car start position camera (`setCarStartPositionF32`)
- `WiiGXRenderer.h/cpp` — D-pad camera controls (`updateCameraFromInput`)
- `StaticMeshProcesser.cpp` — Native terrain upload path using `vertexes` + `triIndexes`
- `BaseRaceMode.cpp` — Extract car start position from PHYLoader after track load

---

## 4. CURRENT BLOCKER

**The build running on the Wii has `triIndexes` empty for all 53 chunks** — `processPart()` either returns early or the deployed build predates the index fix, so no vertex/index data reaches the GPU.

---

## 5. NEXT STEP

1. **Deploy the latest build** (`boot.dol` size `2673484`) to SD card — the one with comprehensive `[TERRAIN_RENDER]` logging
2. **Run on Wii and read the log** — the new logs will show exactly where the pipeline breaks:
   - `[TERRAIN_RENDER] entry dataValid=X verts=Y indices=Z` — confirms data reached renderer
   - `[TERRAIN_RENDER] EARLY RETURN` — shows why render was skipped
   - `[TERRAIN_RENDER] drawing N triangles` — confirms draw call executed
   - `[TERRAIN_RENDER] camera at (x,y,z) looking at (x,y,z)` — shows exact camera position
3. **Based on log output:**
   - If `dataValid=0` or `indices=0`: `processPart()` still not populating data → check if latest build is actually running
   - If `drawing N triangles` but nothing visible: camera position is wrong → use logged camera coords to adjust
   - If `EARLY RETURN`: trace which condition fails
