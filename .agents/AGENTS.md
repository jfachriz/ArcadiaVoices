# Arcadia Voices Project Rules

## Build Workflow
- **Pre-requisite for C++ Plugin Builds**: Always run `./copy_web_resources.sh` prior to building with CMake (`cmake --build build ...`). This ensures the Vite React UI bundle is freshly compiled and placed into `iPlug2OOS/ArcadiaVoices/resources/web` before the plugin binaries are packaged and deployed.
