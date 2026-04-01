#!/usr/bin/env bash
set -euo pipefail

ROOT="/home/davey/Documents/powerslide"
CAPTURE_SCRIPT="$ROOT/dolphin_fault_capture.sh"

cd "$ROOT"

make -j4

cp "$ROOT/wii_build/powerslide.elf" "$ROOT/wii_build/powerslide_full_latest.elf"
cp "$ROOT/wii_build/powerslide.dol" "$ROOT/wii_build/powerslide_full_latest.dol"
cp "$ROOT/wii_build/powerslide.map" "$ROOT/wii_build/powerslide_full_latest.map"

"$CAPTURE_SCRIPT" "$ROOT/wii_build/powerslide_full_latest.dol" 20

LATEST_FAULTS="$(ls -1t "$ROOT"/wii_build/dolphin_faults/run_*.faults.log | head -n1)"
echo "Latest fault summary: $LATEST_FAULTS"

PCS=$(grep -Eo '0x[0-9a-fA-F]+' "$LATEST_FAULTS" | sort -u || true)
if [[ -n "$PCS" ]]; then
  echo "addr2line for captured PCs:"
  /opt/devkitpro/devkitPPC/bin/powerpc-eabi-addr2line -Cfipe "$ROOT/wii_build/powerslide_full_latest.elf" $PCS || true
fi
