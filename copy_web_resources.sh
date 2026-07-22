#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Building React UI for Arcadia Voices..."
npm run build

echo "Copying web resources to iPlug2..."
rm -rf "$SCRIPT_DIR/iPlug2OOS/ArcadiaVoices/resources/web"
cp -R "$SCRIPT_DIR/dist" "$SCRIPT_DIR/iPlug2OOS/ArcadiaVoices/resources/web"

# Update build output and user plugin bundles
for path in \
  "$SCRIPT_DIR/build/out/Arcadia Voices.vst3/Contents/Resources/web" \
  "$SCRIPT_DIR/build/out/Arcadia Voices.component/Contents/Resources/web" \
  "$SCRIPT_DIR/build/out/AV.vst3/Contents/Resources/web" \
  "$SCRIPT_DIR/build/out/AV.component/Contents/Resources/web" \
  "$HOME/Library/Audio/Plug-Ins/VST3/Arcadia Voices.vst3/Contents/Resources/web" \
  "$HOME/Library/Audio/Plug-Ins/Components/Arcadia Voices.component/Contents/Resources/web" \
  "$HOME/Library/Audio/Plug-Ins/VST3/AV.vst3/Contents/Resources/web" \
  "$HOME/Library/Audio/Plug-Ins/Components/AV.component/Contents/Resources/web"; do
  if [ -d "$path" ]; then
    rm -rf "$path"
    cp -R "$SCRIPT_DIR/dist" "$path"
    echo "Updated bundle resource: $path"
  fi
done

# System-level paths (attempt update if writable)
for path in \
  "/Library/Audio/Plug-Ins/VST3/Arcadia Voices.vst3/Contents/Resources/web" \
  "/Library/Audio/Plug-Ins/Components/Arcadia Voices.component/Contents/Resources/web" \
  "/Library/Audio/Plug-Ins/VST3/AV.vst3/Contents/Resources/web" \
  "/Library/Audio/Plug-Ins/Components/AV.component/Contents/Resources/web"; do
  if [ -d "$path" ] && [ -w "$path" ]; then
    rm -rf "$path"
    cp -R "$SCRIPT_DIR/dist" "$path"
    echo "Updated system bundle resource: $path"
  fi
done

# Sync dark UI screenshot to web-archangeldsp website assets if present
WEB_DSP_DIR="$SCRIPT_DIR/../web-archangeldsp"
if [ -d "$WEB_DSP_DIR/public/plugins" ] && [ -f "$SCRIPT_DIR/Assets/Delay_UI.png" ]; then
  cp "$SCRIPT_DIR/Assets/Delay_UI.png" "$WEB_DSP_DIR/public/plugins/arcadia-voices-transparent.png"
  cp "$SCRIPT_DIR/Assets/Delay_UI.png" "$WEB_DSP_DIR/public/plugins/AV.png"
  cp "$SCRIPT_DIR/Assets/Delay_UI.png" "$WEB_DSP_DIR/public/plugins/arcadiavoices.png"
  echo "Synced dark UI screenshot to web-archangeldsp website!"
fi

echo "Arcadia Voices web resources packaged successfully!"