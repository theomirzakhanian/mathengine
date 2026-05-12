#!/usr/bin/env bash
# Build a plain DMG: just the app + Applications symlink. No custom background.
set -euo pipefail

APP_PATH="${1:-build/app/mathapp.app}"
if [ ! -d "$APP_PATH" ]; then
    echo "App bundle not found: $APP_PATH"
    exit 1
fi

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
STAGE="$ROOT/dmg_stage"
FINAL_DMG="$ROOT/MathEngine-macOS.dmg"
VOL_NAME="MathEngine"

echo "Staging..."
rm -rf "$STAGE" "$FINAL_DMG"
mkdir -p "$STAGE"
cp -R "$APP_PATH" "$STAGE/MathEngine.app"
ln -s /Applications "$STAGE/Applications"

echo "Ad-hoc signing..."
xattr -cr "$STAGE/MathEngine.app"
codesign --force --deep --sign - "$STAGE/MathEngine.app"

echo "Creating DMG..."
hdiutil create -volname "$VOL_NAME" -srcfolder "$STAGE" -ov \
    -format UDZO -fs HFS+ "$FINAL_DMG" >/dev/null

rm -rf "$STAGE"

echo "Done: $FINAL_DMG"
ls -lh "$FINAL_DMG"
