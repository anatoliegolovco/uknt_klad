#!/usr/bin/env bash
# dev_watch.sh — rebuild + restart КЛАД on any source/font change (polling; no inotify).
# Watches src/ and font/. On change: kill running klad → make → relaunch on DISPLAY :0.
# Usage:  bash tools/dev_watch.sh        (run in background; Ctrl-C / kill to stop)
set -u
cd "$(dirname "$0")/.."
export DISPLAY="${DISPLAY:-:0}"
WATCH="src font"

sig() { find $WATCH -type f \( -name '*.c' -o -name '*.h' -o -name 'Makefile' \) \
          -printf '%T@ %p\n' 2>/dev/null | sort | md5sum | cut -d' ' -f1; }

relaunch() {
    pkill -f build/klad 2>/dev/null; sleep 0.3
    if make -C src >/tmp/klad_build.log 2>&1; then
        setsid ./build/klad >/tmp/klad_run.log 2>&1 </dev/null & disown
        echo "[$(date +%H:%M:%S)] rebuilt + relaunched ✓"
    else
        echo "[$(date +%H:%M:%S)] BUILD FAILED — see /tmp/klad_build.log:"
        grep -iE 'error' /tmp/klad_build.log | head -5
    fi
}

echo "dev_watch: building + launching, then watching $WATCH ..."
relaunch
last="$(sig)"
while true; do
    sleep 1
    now="$(sig)"
    if [ "$now" != "$last" ]; then last="$now"; relaunch; fi
done
