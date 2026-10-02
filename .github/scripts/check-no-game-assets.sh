#!/usr/bin/env bash
# Stop the build if the APK contains game assets. The APK must contain only soh.o2r.
# Usage: check-no-game-assets.sh <apk>
set -euo pipefail

APK="$1"
# List first, so that a missing or broken APK stops the build instead of passing the check.
LISTING="$(unzip -l "$APK")"
if grep -E '\.(z64|n64|v64)$|oot(-mq)?\.o2r$' <<< "$LISTING"; then
  echo "The APK contains game assets. Stop." >&2
  exit 1
fi
echo "The APK has no game assets."
