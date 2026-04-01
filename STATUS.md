# STATUS REPORT - 2026-04-01

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
