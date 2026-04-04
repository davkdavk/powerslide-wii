# STATUS REPORT - 2026-04-04

## Current Baseline

- Real Wii desert track texture mapping is back to the expected baseline for the church/roof/road/sand world-object path.
- Confirmed fix landed in the native direct GX terrain/world-object renderer.
- Current active build root cause is no longer DE2 UV decoding.

## Confirmed Root Cause

- The native GX real-texture path was building `GX_TF_RGB565` terrain textures from row-major pixel data.
- GX `RGB565` textures require tiled memory layout.
- The checker debug texture path already used tiled layout, but the real terrain texture path did not.
- Result: hardware sampled incorrect texels even when texture identity and UVs were otherwise reasonable.

## Confirmed Fix

- Added tiled `RGB565` texture construction for the native terrain/world-object texture cache in `wii_stubs/OGRE/WiiGXRenderer.cpp`.
- Kept per-batch sampler state propagation already added earlier:
  - texture wrap/clamp
  - texture scale

## Current Build

- `wii_build/powerslide.dol`
- Size: `2695708`
- Deployed to: `/media/davey/DCDA-0BC2/apps/powerslide/boot.dol`

## Notes

- Terrain/world-object direct GX path remains the active Wii-native baseline.
- Temporary bring-up logging is being reduced now that the renderer-side root cause is confirmed.
