# Wii Build - Powerslide Remake Stub

## Requirements

1. **devkitPro** with devkitPPC installed
   - Download from: https://devkitpro.org/
   - Install devkitPPC and libogc

2. **fat** library (included with devkitPro)

## Building

```bash
cd wii
make
```

This will create:
- `wii_build/powerslide.elf` - ELF binary
- `wii_build/powerslide.dol` - DOL executable

## Running

Copy `powerslide.dol` to your SD card:
```
/apps/powerslide/boot.dol
```

Or use a USB loader / Homebrew Channel.

## What This Does

This stub version:
- Initializes Wii video mode
- Initializes libfat for SD card access
- Displays a test pattern showing stub status
- Boots without crashing

## Stub Status

| Component | Status |
|-----------|--------|
| OGRE | STUBBED |
| SFML | STUBBED |
| OpenAL | STUBBED |
| OIS | STUBBED |
| libfat | WORKING |

## Files Created

- `wii/Makefile` - Main build file
- `wii_stubs/` - Stub headers and linker script
  - `OGRE/Ogre.h` - OGRE stub
  - `SFML/Audio.hpp` - SFML Audio stub
  - `OIS/OISInputManager.h` - OIS stub
  - `AL/al.h` - OpenAL stub
  - `stubs.cpp` - Wii entry point
  - `wii.ld` - Linker script

## Next Steps

To continue development:
1. Replace stub functions with actual Wii implementations
2. Add Wii controller support using libogc
3. Implement graphics using Wii GX or a Wii-specific renderer
4. Add audio using Wii AX (Wii Sound System)
