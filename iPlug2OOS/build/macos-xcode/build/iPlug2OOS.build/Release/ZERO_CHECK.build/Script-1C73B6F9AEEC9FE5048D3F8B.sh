#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Users/jfachriz/Documents/VST_Plugins/ArcadiaVoices/iPlug2OOS/build/macos-xcode
  make -f /Users/jfachriz/Documents/VST_Plugins/ArcadiaVoices/iPlug2OOS/build/macos-xcode/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Users/jfachriz/Documents/VST_Plugins/ArcadiaVoices/iPlug2OOS/build/macos-xcode
  make -f /Users/jfachriz/Documents/VST_Plugins/ArcadiaVoices/iPlug2OOS/build/macos-xcode/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Users/jfachriz/Documents/VST_Plugins/ArcadiaVoices/iPlug2OOS/build/macos-xcode
  make -f /Users/jfachriz/Documents/VST_Plugins/ArcadiaVoices/iPlug2OOS/build/macos-xcode/CMakeScripts/ReRunCMake.make
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Users/jfachriz/Documents/VST_Plugins/ArcadiaVoices/iPlug2OOS/build/macos-xcode
  make -f /Users/jfachriz/Documents/VST_Plugins/ArcadiaVoices/iPlug2OOS/build/macos-xcode/CMakeScripts/ReRunCMake.make
fi

