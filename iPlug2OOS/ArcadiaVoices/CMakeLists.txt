cmake_minimum_required(VERSION 3.25)
project(ArcadiaVoices VERSION 1.0.0)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../iPlug2" CACHE PATH "iPlug2 root directory")
endif()

include(${IPLUG2_DIR}/iPlug2.cmake)

find_package(iPlug2 REQUIRED)

# WEB_RESOURCES must live under resources/web/
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

if(TARGET ArcadiaVoices-vst3)
  set_target_properties(ArcadiaVoices-vst3 PROPERTIES
    OUTPUT_NAME "Arcadia Voices"
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
  )
endif()
if(TARGET ArcadiaVoices-au)
  set_target_properties(ArcadiaVoices-au PROPERTIES
    OUTPUT_NAME "Arcadia Voices"
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
  )
endif()
