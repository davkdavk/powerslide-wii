# Powerslide Wii Port — OpenCode Rules

## Goal
Port the Powerslide remake (dm999) to the Nintendo Wii as a homebrew .dol
application using devkitPro, devkitPPC, and libogc. Work autonomously and
continuously. Do not stop and ask for input. Make reasonable decisions and
document them in DECISIONS.md.

## Non-negotiable Requirements
- Target 60fps on real Wii hardware
- Total memory usage must stay under 80MB
- Must boot and run on Dolphin emulator
- Asset files data.pf, gameshell.pf, store.pf are read from sd:/powerslide/
- Final output is a working .dol file

## Replacements
- Replace OGRE with Wii GX. Use Gouraud shading. No shaders, no soft shadows,
  no per-pixel lighting.
- Replace OpenAL and SFML audio with libogc ASND
- Replace all keyboard and gamepad input with Classic Controller via libogc WPAD
  - Left stick = steer
  - A = accelerate
  - B = brake
  - Plus = pause
- Remove neural network AI entirely. Replace with simple waypoint/centerline
  following AI for all bot cars.

## Coding Rules
- Use C++17
- Do not use dynamic memory allocation in the game loop. Allocate everything
  upfront at load time.
- Keep all platform-specific code isolated in a platform/ directory so the
  core game logic stays clean
- If a feature cannot be implemented on Wii hardware, stub it out and log a
  warning. Do not block progress.
- Prefer simple and stable over clever and fragile

## Decision Making
- If you encounter a problem with more than one solution, pick the simplest one
  and document it in DECISIONS.md
- If a dependency cannot be ported, remove it and note what functionality was
  lost in DECISIONS.md
- Never stop working to ask a question. Make a call and move on.

## Milestones (work through these in order)
1. Codebase compiles for devkitPPC with all unportable code stubbed out
2. Blank screen .dol boots in Dolphin without crashing
3. Track renders on screen
4. Player car renders and moves with input
5. Bot cars render and follow waypoints
6. Audio plays
7. Menus work
8. Full race is completable start to finish
9. Memory usage confirmed under 80MB
10. Framerate confirmed at 60fps on Dolphin

## Output
- Keep a file called PROGRESS.md updated after each milestone is reached
- Keep a file called DECISIONS.md with every significant technical decision made
- Keep a file called ISSUES.md with any known bugs or limitations

## Automated Visual Test Loop
- Use this loop for all future Wii/Dolphin visual checks. Decide results yourself via imagemagick sampling.
- Prereqs (install once if missing):
  - `sudo apt-get install -y xdotool imagemagick`
- Crash loop (every build):
  1. Build: `make -j4` (or `make clean && make` when needed)
  2. Run: `./dolphin_fault_capture.sh wii_build/powerslide_full_fixNN.dol 20`
  3. Read latest `wii_build/dolphin_faults/run_*.faults.log`
  4. If PCs exist, resolve all with `powerpc-eabi-addr2line -Cfipe wii_build/powerslide_full_fixNN.elf <pcs>`
  5. Verify crash site with `powerpc-eabi-objdump -d --start-address=... --stop-address=...`
  6. Fix code, rebuild, repeat until no new crash PCs
  7. Only then produce Wii candidate build for hardware test

## Build Command
- Makefile is the single source of truth for Wii builds.
- Use `./build_full.sh` for autonomous local loop (build + Dolphin fault capture + addr2line summary).
- Full clean build: `make clean && make`
- Incremental build: `make`
