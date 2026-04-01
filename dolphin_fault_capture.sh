#!/usr/bin/env bash
set -euo pipefail

APPIMAGE="/home/davey/Dolphin_Emulator-2603a-anylinux-x86_64.AppImage"
DEFAULT_DOL="/home/davey/Documents/powerslide/wii_build/powerslide_full_latest.dol"
OUTDIR="/home/davey/Documents/powerslide/wii_build/dolphin_faults"
DOLPHIN_LOG="/home/davey/.local/share/dolphin-emu/Logs/dolphin.log"

DOL_PATH="${1:-$DEFAULT_DOL}"
RUN_SECONDS="${2:-20}"
BATCH_MODE="${DOLPHIN_BATCH:-1}"
AUTO_DISMISS="${DOLPHIN_AUTO_DISMISS:-$BATCH_MODE}"
PANIC_MAX_CAPTURES="${DOLPHIN_PANIC_MAX_CAPTURES:-200}"

HAVE_IMPORT=0
if command -v import >/dev/null 2>&1; then
  HAVE_IMPORT=1
fi

PANIC_CAPTURE_COUNT=0

if [[ "$AUTO_DISMISS" == "1" ]] && ! command -v xdotool >/dev/null 2>&1; then
  echo "Missing dependency: xdotool" >&2
  exit 1
fi

if [[ ! -x "$APPIMAGE" ]]; then
  echo "Missing Dolphin AppImage: $APPIMAGE" >&2
  exit 1
fi

if [[ ! -f "$DOL_PATH" ]]; then
  echo "Missing DOL: $DOL_PATH" >&2
  exit 1
fi

mkdir -p "$OUTDIR"

TS="$(date +%Y%m%d_%H%M%S)"
RAW_LOG="$OUTDIR/run_${TS}.raw.log"
FAULT_LOG="$OUTDIR/run_${TS}.faults.log"
DOLPHIN_COPY="$OUTDIR/run_${TS}.dolphin.log"

mkdir -p "$(dirname "$DOLPHIN_LOG")"
: >"$DOLPHIN_LOG"

pkill -9 Dolphin_Emulator-2603a-anylinux-x86_64.AppImage 2>/dev/null || true
pkill -9 dolphin-emu 2>/dev/null || true
sleep 1

if [[ "$BATCH_MODE" == "1" ]]; then
  BATCH_FLAG="-b"
else
  BATCH_FLAG=""
fi

"$APPIMAGE" $BATCH_FLAG -e "$DOL_PATH" \
  -C "Dolphin.Core.WiiSDCard=True" \
  -C "Dolphin.Core.WiiSDCardEnableFolderSync=True" \
  -C "Dolphin.Core.WiiSDCardSyncFolder=/home/davey/Documents/powerslide/wii_build" \
  >"$RAW_LOG" 2>&1 &
DPID=$!

sleep 2

END_TIME=$((SECONDS + RUN_SECONDS))
while (( SECONDS < END_TIME )); do
  for pattern in "Panic Alert" "Question" "Error" "Assertion"; do
    while IFS= read -r wid; do
      [[ -n "$wid" ]] || continue

      if [[ "$HAVE_IMPORT" == "1" ]]; then
        if [[ "$PANIC_CAPTURE_COUNT" -lt "$PANIC_MAX_CAPTURES" ]]; then
          PANIC_CAPTURE_COUNT=$((PANIC_CAPTURE_COUNT + 1))
          PANIC_IMG="$OUTDIR/run_${TS}.panic_$(printf '%04d' "$PANIC_CAPTURE_COUNT")_window_${wid}.png"
          import -window "$wid" "$PANIC_IMG" >/dev/null 2>&1 || true
        fi
      fi

      if [[ "$AUTO_DISMISS" == "1" ]]; then
        xdotool windowactivate "$wid" key --window "$wid" Return >/dev/null 2>&1 || true
        xdotool windowactivate "$wid" key --window "$wid" KP_Enter >/dev/null 2>&1 || true
        xdotool windowactivate "$wid" key --window "$wid" i >/dev/null 2>&1 || true
      fi
    done < <(xdotool search --name "$pattern" 2>/dev/null || true)
  done
  if ! kill -0 "$DPID" 2>/dev/null; then
    break
  fi
  sleep 1
done

kill "$DPID" 2>/dev/null || true
pkill -9 Dolphin_Emulator-2603a-anylinux-x86_64.AppImage 2>/dev/null || true
pkill -9 dolphin-emu 2>/dev/null || true
sleep 1

if [[ -f "$DOLPHIN_LOG" ]]; then
  cp "$DOLPHIN_LOG" "$DOLPHIN_COPY" || true
fi

COMBINED="$OUTDIR/run_${TS}.combined.log"
cat "$RAW_LOG" >"$COMBINED"
if [[ -f "$DOLPHIN_COPY" ]]; then
  printf "\n--- dolphin.log ---\n" >>"$COMBINED"
  cat "$DOLPHIN_COPY" >>"$COMBINED"
fi

{
  echo "DOL: $DOL_PATH"
  echo "Run seconds: $RUN_SECONDS"
  echo "Raw log: $RAW_LOG"
  echo "Dolphin log copy: $DOLPHIN_COPY"
  echo
  echo "Fault lines:"
  grep -E "Invalid (read|write) to .* PC = 0x[0-9a-fA-F]+|PC[: =]+0x[0-9a-fA-F]+|Panic Alert|DSI|ISI|Program Exception|Exception" "$COMBINED" || true
  echo
  echo "Unique PCs:"
  grep -Eo "PC[: =]+0x[0-9a-fA-F]+" "$COMBINED" | sed -E 's/.*(0x[0-9a-fA-F]+)/\1/' | sort | uniq -c | sort -nr || true
} >"$FAULT_LOG"

echo "Saved raw log: $RAW_LOG"
echo "Saved fault summary: $FAULT_LOG"
if [[ "$PANIC_CAPTURE_COUNT" -gt 0 ]]; then
  echo "Saved panic screenshots: $PANIC_CAPTURE_COUNT (prefix run_${TS}.panic_*)"
fi
