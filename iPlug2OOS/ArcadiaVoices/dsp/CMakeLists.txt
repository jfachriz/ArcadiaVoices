# Test harness for ArcadiaVoices DSP core (standalone, no iPlug2)
cmake_minimum_required(VERSION 3.16)
project(ArcadiaVoices_DSP_Test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source: just the test main — all DSP classes are header-only
add_executable(dsp_test tests/main.cpp)
target_include_directories(dsp_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
