#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Building React UI..."
npm run build

echo "Copying web resources to iPlug2..."
rm -rf "$SCRIPT_DIR/iPlug2OOS/ArcadiaVoices/resources/web"
cp -R "$SCRIPT_DIR/dist" "$SCRIPT_DIR/iPlug2OOS/ArcadiaVoices/resources/web"

echo "Web resources copied successfully!"