#!/bin/bash
set -euo pipefail

# =====================================================================
# EDIT THESE VALUES
# =====================================================================
PRODUCT_NAME="Arcadia Voices"                          # Used in titles / dmg name
VERSION="1.0"
IDENTIFIER_PREFIX="com.ArchangelDSP.ArcadiaVoices"     # Reverse-DNS prefix

# Pre-requisite rule from AGENTS.md: Always build web resources first
echo "==> Building web UI resources..."
./copy_web_resources.sh

SRC_AU="$(pwd)/iPlug2OOS/build/macos-xcode/out/Release/Arcadia Voices.component"
SRC_VST3="$(pwd)/iPlug2OOS/build/macos-xcode/out/Release/Arcadia Voices.vst3"

# Default install locations are RELATIVE (no leading slash).
# This lets Installer.app's native "Install for all users / just me" choice
# resolve them to /Library/... (system, default) or ~/Library/... (per-user).
AU_INSTALL_LOCATION="Library/Audio/Plug-Ins/Components"
VST3_INSTALL_LOCATION="Library/Audio/Plug-Ins/VST3"

# =====================================================================
# Do not need to edit below this line
# =====================================================================
WORKDIR="$(pwd)/build_tmp"
OUTDIR="$(pwd)/Installer"
rm -rf "$WORKDIR" && mkdir -p "$WORKDIR" "$OUTDIR"

AU_ROOT="$WORKDIR/root_au"
VST3_ROOT="$WORKDIR/root_vst3"
mkdir -p "$AU_ROOT" "$VST3_ROOT"

echo "==> Staging payloads"
cp -R "$SRC_AU"   "$AU_ROOT/"
cp -R "$SRC_VST3" "$VST3_ROOT/"

AU_PKG_ID="${IDENTIFIER_PREFIX}.au"
VST3_PKG_ID="${IDENTIFIER_PREFIX}.vst3"

echo "==> Building component package: AU"
pkgbuild \
  --root "$AU_ROOT" \
  --identifier "$AU_PKG_ID" \
  --version "$VERSION" \
  --install-location "$AU_INSTALL_LOCATION" \
  "$WORKDIR/au.pkg"

echo "==> Building component package: VST3"
pkgbuild \
  --root "$VST3_ROOT" \
  --identifier "$VST3_PKG_ID" \
  --version "$VERSION" \
  --install-location "$VST3_INSTALL_LOCATION" \
  "$WORKDIR/vst3.pkg"

echo "==> Writing distribution.xml"
cat > "$WORKDIR/distribution.xml" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>${PRODUCT_NAME} Installer</title>
    <organization>${IDENTIFIER_PREFIX}</organization>

    <!-- Enables the native "Install for all users of this computer" /
         "Install for me only" location choice. System (all users) is
         offered first / is the default. -->
    <domains enable_localSystem="true" enable_currentUserHome="true" enable_anywhere="false"/>

    <options customize="always" require-scripts="false" rootVolumeOnly="false"/>

    <!-- Top-level checkboxes: user can pick which plugin(s) to install -->
    <choices-outline>
        <line choice="choiceAU"/>
        <line choice="choiceVST3"/>
    </choices-outline>

    <choice id="choiceAU"
            title="Audio Unit (AU) Component"
            description="Installs the AU plugin to ${AU_INSTALL_LOCATION}"
            start_selected="true">
        <pkg-ref id="${AU_PKG_ID}"/>
    </choice>

    <choice id="choiceVST3"
            title="VST3 Plugin"
            description="Installs the VST3 plugin to ${VST3_INSTALL_LOCATION}"
            start_selected="true">
        <pkg-ref id="${VST3_PKG_ID}"/>
    </choice>

    <pkg-ref id="${AU_PKG_ID}"   version="${VERSION}" onConclusion="none">au.pkg</pkg-ref>
    <pkg-ref id="${VST3_PKG_ID}" version="${VERSION}" onConclusion="none">vst3.pkg</pkg-ref>
</installer-gui-script>
EOF

echo "==> Building product installer (.pkg)"
FINAL_PKG="$OUTDIR/${PRODUCT_NAME}-${VERSION}.pkg"
productbuild \
  --distribution "$WORKDIR/distribution.xml" \
  --package-path "$WORKDIR" \
  "$FINAL_PKG"

echo "==> Generating rounded icon from Assets/1.png"
if [ -f "Assets/1.png" ]; then
  swift make_rounded_icon.swift Assets/1.png "$WORKDIR"
  iconutil -c icns "$WORKDIR/icon.iconset" -o "$WORKDIR/VolumeIcon.icns"
fi

echo "==> Wrapping into .dmg"
DMG_ROOT="$WORKDIR/dmg_root"
mkdir -p "$DMG_ROOT"
cp "$FINAL_PKG" "$DMG_ROOT/"

if [ -f "$WORKDIR/VolumeIcon.icns" ]; then
  cp "$WORKDIR/VolumeIcon.icns" "$DMG_ROOT/.VolumeIcon.icns"
  SetFile -a C "$DMG_ROOT" || true
fi

FINAL_DMG="$OUTDIR/${PRODUCT_NAME}-${VERSION}.dmg"
rm -f "$FINAL_DMG"
hdiutil create \
  -volname "${PRODUCT_NAME} ${VERSION}" \
  -srcfolder "$DMG_ROOT" \
  -ov -format UDZO \
  "$FINAL_DMG"

echo ""
echo "==================================================================="
echo "Done."
echo "Installer package: $FINAL_PKG"
echo "Disk image:        $FINAL_DMG"
echo "==================================================================="
