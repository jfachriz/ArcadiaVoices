#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Building React UI..."
npm run build

echo "Copying web resources to iPlug2..."
rm -rf "$SCRIPT_DIR/iPlug2OOS/ArcadiaVoices/resources/web"
cp -R "$SCRIPT_DIR/dist" "$SCRIPT_DIR/iPlug2OOS/ArcadiaVoices/resources/web"

# Also update existing build artifacts and deployed plugin bundles directly to prevent CMake caching stale assets
if [ -d "$SCRIPT_DIR/build/out/AV.vst3/Contents/Resources/web" ]; then
  rm -rf "$SCRIPT_DIR/build/out/AV.vst3/Contents/Resources/web"
  cp -R "$SCRIPT_DIR/dist" "$SCRIPT_DIR/build/out/AV.vst3/Contents/Resources/web"
fi

if [ -d "$SCRIPT_DIR/build/out/AV.component/Contents/Resources/web" ]; then
  rm -rf "$SCRIPT_DIR/build/out/AV.component/Contents/Resources/web"
  cp -R "$SCRIPT_DIR/dist" "$SCRIPT_DIR/build/out/AV.component/Contents/Resources/web"
fi

if [ -d "$HOME/Library/Audio/Plug-Ins/VST3/AV.vst3/Contents/Resources/web" ]; then
  rm -rf "$HOME/Library/Audio/Plug-Ins/VST3/AV.vst3/Contents/Resources/web"
  cp -R "$SCRIPT_DIR/dist" "$HOME/Library/Audio/Plug-Ins/VST3/AV.vst3/Contents/Resources/web"
fi

if [ -d "$HOME/Library/Audio/Plug-Ins/Components/AV.component/Contents/Resources/web" ]; then
  rm -rf "$HOME/Library/Audio/Plug-Ins/Components/AV.component/Contents/Resources/web"
  cp -R "$SCRIPT_DIR/dist" "$HOME/Library/Audio/Plug-Ins/Components/AV.component/Contents/Resources/web"
fi

echo "Web resources copied successfully to all build and deployed bundles!"