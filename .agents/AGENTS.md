# Arcadia Voices Project Rules

## Build Workflow
- **Pre-requisite for C++ Plugin Builds**: Always run `./copy_web_resources.sh` prior to building with CMake (`cmake --build build ...`). This ensures the Vite React UI bundle is freshly compiled and placed into `iPlug2OOS/ArcadiaVoices/resources/web` before the plugin binaries are packaged and deployed.

## Web UI Bundling & WKWebView Best Practices
- **Single-File HTML Bundling**: Always use `vite-plugin-singlefile` in `vite.config.ts` so Vite inlines JS, CSS, and image assets into a single `dist/index.html`. This avoids WebKit CORS restrictions on local `file://` URLs.
- **Space-Safe `NSURL` Creation**: In Objective-C `IPlugWebView_mac.mm`, use `[NSURL fileURLWithPath:]` rather than `[NSURL URLWithString:]` to ensure paths containing spaces (e.g. `Arcadia Voices.vst3`) resolve correctly.
- **View Autoresizing Masks**: Ensure `WKWebView` and helper container views have `[view setAutoresizingMask: NSViewWidthSizable | NSViewHeightSizable]` set so the web interface resizes dynamically with host DAW windows.
- **Enable DevTools in C++**: Keep `SetEnableDevTools(true)` enabled during development to allow right-clicking and inspecting the Web UI via Safari Web Inspector.

