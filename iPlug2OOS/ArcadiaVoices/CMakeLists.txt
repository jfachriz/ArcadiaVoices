cmake_minimum_required(VERSION 3.25)
project("Arcadia Voices" VERSION 1.0.0)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../iPlug2" CACHE PATH "iPlug2 root directory")
endif()

include(${IPLUG2_DIR}/iPlug2.cmake)

find_package(iPlug2 REQUIRED)

# WEB_RESOURCES must live under resources/web/ (not dist/ directly) - iPlug2's
# _iplug_add_web_resources() CMake helper only preserves subdirectory structure
# (e.g. assets/) when the resource path contains "/resources/web/". Globbing
# dist/ directly flattens dist/assets/* into a flat Resources/web/, which
# breaks index.html's relative "./assets/index.js" and "./assets/index.css"
# references (blank/non-rendering WebView UI). Keep resources/web/ in sync
# with the Vite build output by running:
#   npm run build && rm -rf iPlug2OOS/ArcadiaVoices/resources/web && \
#     cp -R dist iPlug2OOS/ArcadiaVoices/resources/web
# before configuring/building with CMake.
file(GLOB_RECURSE WEBVIEW_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/resources/web/*")

iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    ArcadiaVoices.cpp
    ArcadiaVoices.h
    resources/resource.h
  RESOURCES
    resources/fonts/Roboto-Regular.ttf
  WEB_RESOURCES
    ${WEBVIEW_RESOURCES}
  FORMATS
    VST3 AU
  UI WEBVIEW
  DEFINES
    NO_IGRAPHICS
    WEBVIEW_EDITOR_DELEGATE
)
