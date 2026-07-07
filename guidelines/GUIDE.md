# Arcadia Voices - Complete Technical Guide

This document provides a comprehensive technical overview of the Arcadia Voices plugin, covering architecture, implementation details, and development workflow.

## 📋 Table of Contents

1. [Project Overview](#project-overview)
2. [Technical Architecture](#technical-architecture)
3. [DSP Implementation](#dsp-implementation)
4. [UI Implementation](#ui-implementation)
5. [Build System](#build-system)
6. [Installation & Usage](#installation--usage)
7. [Development Workflow](#development-workflow)
8. [Troubleshooting](#troubleshooting)

## Project Overview

**Plugin Name:** Arcadia Voices (AV)
**Developer:** Archangel DSP
**Type:** Multi-Voice Pitch Delay (VST3/AU)
**Platform:** macOS (Intel), up to 48kHz sample rate
**Status:** Complete (as of 2026-07-08)

### Technology Stack

- **DSP Engine:** C++ (iPlug2 framework, header-only)
- **UI:** React 19 + TypeScript + Vite
- **Styling:** Tailwind CSS v4 + Shadcn Theme + Radix UI Primitives
- **Bridge:** WebView with custom message protocol (SPVFUI/SAMFUI)
- **Build System:** CMake + npm
- **Package Manager:** npm (frontend) + Vendored C++ SDKs (backend)

## Technical Architecture

### System Diagram

```
Host DAW (VST3/AU)
  │
  ├─ Audio Processing (C++/iPlug2)
  │     └─ ArcadiaVoices Plugin Bundle
  │           ├─ DSP Core
  │           │     ├─ DelayPedalDSP (main processor)
  │           │     ├─ PitchDelayVoice (L/R channels)
  │           │     ├─ CircularBuffer (131072 samples)
  │           │     ├─ PitchShifter (dual read pointers)
  │           │     ├─ ModLFO (sine/triangle)
  │           │     ├─ SoftClipper (feedback limiter)
  │           │     ├─ PitchDetector (YIN algorithm)
  │           │     ├─ TempoSync (BPM → ms conversion)
  │           │     └─ PresetManager
  │           │
  │           └─ iPlug2 Host Glue
  │                 ├─ Parameter registration (9 params)
  │                 ├─ ProcessBlock wiring
  │                 ├─ State serialization
  │                 └─ WebView hosting
  │
  └─ User Interface (WebView)
        └─ React Frontend
              ├─ Entry: src/main.tsx
              ├─ Root: src/app/App.tsx
              ├─ Components: src/app/components/
              │     ├─ pedal/ (pedal-specific)
              │     └─ ui/ (shadcn/Radix primitives)
              ├─ Bridge: src/app/iplugBridge.ts
              ├─ Hook: src/app/useBridge.ts
              └─ Styles: src/styles/ (Tailwind + theme.css)
```

### Key Components

#### DSP Core
- **CircularBuffer**: 131072 samples (2^17), bitmask wrapping, 4-sample guard
- **PitchShifter**: Dual read pointers with crossfade, fractional advancement
- **ModLFO**: Sine/triangle waveform for modulation effects
- **PitchDetector**: YIN algorithm, 20-40ms window, per-channel detection
- **TempoSync**: Converts BPM + division → delay time, smooth transitions
- **PresetManager**: In-memory storage with iPlug2 serialization

#### UI Components
- **LeftPanel**: TIME knob, FEEDBACK knob, SYNC toggle, DIVISION selector
- **CenterPanel**: OLED screen, POWER button, PITCH L/R knobs, LED indicator
- **RightPanel**: MIX knob, MOD knob
- **OLEDScreen**: Preset display, pitch readouts, preset menu overlay

## DSP Implementation

### Core Algorithm

**Pitch-Shifting Method:** Delay-line with variable-rate read pointers + crossfaded overlapping taps

**Signal Flow:**
```
Input → [Bypass Check]
      → [Circular Buffer (131072 samples)]
             │
             ├─ Read pointer A (pitch ratio) ┐
             ├─ Read pointer B (pitch ratio, offset) ├─ Crossfade → Pitched Repeat
             │
             ├─ Modulation LFO (mod parameter)
             │
             └─ Feedback path → Soft Clipper → Back to buffer

Dry signal → Mix (dry/wet) → Output
```

### Parameters & Ranges

| Control | Range | Default | Curve | Notes |
|---------|-------|---------|-------|-------|
| **time** | 1ms - 2000ms | 350ms | Logarithmic | Base delay time |
| **feedback** | 0% - 110% | 35% | Linear | >100% allows oscillation |
| **mix** | 0% - 100% | 50% | Linear | Dry/wet balance |
| **mod** | 0% - 100% | 20% | Linear | Modulation depth |
| **pitchL** | -24 to +24 semitones | ~-2 | Stepped | Left channel pitch shift |
| **pitchR** | -24 to +24 semitones | ~+1 | Stepped | Right channel pitch shift |
| **power** | ON/OFF | ON | Toggle | True bypass switch |
| **syncMode** | Free/Synced | Free | Toggle | Tempo synchronization |
| **noteDivision** | 1/32 to 2 bars | 1/8 | Stepped | Note division (synced mode) |

### Parameter Mapping

```typescript
// UI (0-100) → DSP (real units)
time:      knobValue → 1ms-2000ms (log curve)
feedback:  knobValue → 0%-110% (linear)
mix:       knobValue → 0%-100% (linear)
mod:       knobValue → 0%-100% (linear)
pitchL/R:  knobValue → -24..+24 semitones (linear)
           // knob 50 = 0 semitones, knob 0 = -24, knob 100 = +24
```

### Bypass Behavior

- **Power OFF**: True bypass (early-out in ProcessBlock, no processing)
- **Buffer State**: Preserved during bypass (repeats resume when powered on)
- **Pitch Detection**: Stops when bypassed
- **LED Sequence**: Green LED first, then screen fade-in animation

## UI Implementation

### Component Hierarchy

```
PedalShell
├── LeftPanel
│   ├── SyncToggle (Free/Synced)
│   ├── NoteDivisionSelector (visible when synced)
│   ├── Knob(time, white)
│   └── Knob(feedback, black)
│
├── CenterPanel
│   ├── OLEDScreen
│   │   ├── TopIcon
│   │   ├── TitleArea (preset name or param value)
│   │   ├── PresetNumberDisplay (clickable → opens menu)
│   │   └── PitchReadoutRow
│   │       ├── DotMatrixIndicator + PitchLabel (L)
│   │       └── DotMatrixIndicator + PitchLabel (R)
│   │
│   ├── LEDIndicator (power state)
│   ├── Knob(pitchL, small black)
│   ├── OnOffButton(power, large black)
│   └── Knob(pitchR, small black)
│
└── RightPanel
    ├── Knob(mix, white)
    └── Knob(mod, black)
```

### Critical UI Features

1. **Power Button**
   - Replaces old preset knob
   - True bypass (not just UI toggle)
   - LED sequencing: green LED → screen fade-in

2. **Preset System**
   - On-screen menu (click preset number display)
   - Factory presets: "multi voice", "chorus room", "deep space"
   - Save as New functionality
   - In-memory storage with iPlug2 serialization

3. **Live Pitch Detection**
   - Dot-matrix indicators show detected pitch
   - Format: note name + cents offset (e.g., "C# +12")
   - Continuous updates from DSP

4. **Tempo Sync Controls**
   - SyncToggle above time knob
   - NoteDivisionSelector (appears when synced)
   - Cycles through division values

### Pointer Capture Requirement

All drag-based interactions must use `setPointerCapture` for WKWebView compatibility:

```typescript
const handlePointerDown = (e: React.PointerEvent) => {
    e.preventDefault();
    setIsDragging(true);
    startY.current = e.clientY;
    startVal.current = value;
    pointerIdRef.current = e.pointerId;
    knobRef.current = e.currentTarget as HTMLDivElement;
    knobRef.current.setPointerCapture(e.pointerId);  // CRITICAL
};
```

## Build System

### CMake Configuration

**Core Files:**
- `iPlug2OOS/CMakeLists.txt` - Top-level workspace
- `iPlug2OOS/iPlug2/iPlug2.cmake` - Shared configuration
- `iPlug2OOS/ArcadiaVoices/CMakeLists.txt` - Plugin declaration
- `iPlug2OOS/CMakePresets.json` - Canonical presets

**Supported Presets:**
- macOS: `macos-xcode`, `macos-ninja`, `macos-make`, `macos-xcode-universal`
- iOS: `ios-sim-xcode`, `visionos-*`
- Windows: `windows-vs2022`, `windows-vs2022-arm64ec`
- Web: `web` (Emscripten)

**Build Commands:**
```bash
# Configure
cmake --preset=macos-xcode

# Build
cmake --build --preset=macos-xcode
```

### Distribution Scripts

**macOS:**
- `scripts/makedist-mac.sh` - Builds all formats, strips symbols, codesigns, creates installer
- Output: `build-mac/out/`
- Version parsed from `config.h` PLUG_VERSION_HEX

**Windows:**
- `scripts/makedist-win.bat` - Python resource updates, MSBuild, InnoSetup installer
- Supports x64 and ARM64EC

**WebAssembly:**
- `scripts/makedist-wasm.sh` - Emscripten pipeline with file_packager.py
- Generates AudioWorklet processor JS and HTML

### CI Pipelines

- `.github/workflows/build-cmake.yml` - Comprehensive matrix for all presets
- `.github/workflows/build-mac.yml` / `build-win.yml` - Legacy workflows
- Validates bundles with pluginval/VST3 validator/CLAP validator

## Installation & Usage

### macOS Installation

1. **Download** the installer from [Releases](https://github.com/your-repo/releases)
2. **Run** the `.pkg` installer package
3. **Restart** your DAW
4. **Scan** for new plugins (VST3 and AU formats)

### Manual Installation

1. Copy files to plugin folders:
   - VST3: `~/Library/Audio/Plug-Ins/VST3/`
   - AU: `~/Library/Audio/Plug-Ins/Components/`
2. Restart DAW and rescan plugins

### Quick Start Guide

1. **Insert** Arcadia Voices on your audio track
2. **Turn on** power switch (green LED = active)
3. **Set TIME** to desired delay (350ms = good starting point)
4. **Adjust PITCH L/R** for your interval (-2/+1 = classic setting)
5. **Dial FEEDBACK** for repeat length
6. **Set MIX** for dry/wet balance

### Pro Tips

- **Shimmer effects**: PITCH R = +12/+24 with moderate feedback
- **Wide stereo**: PITCH L = -12, PITCH R = +12
- **Rhythmic patterns**: Enable SYNC with matching note division
- **Modulation**: Increase MOD for chorus-like repeats
- **Infinite repeats**: FEEDBACK > 100% (use cautiously!)

## Development Workflow

### Frontend Development

```bash
# Install dependencies
npm install

# Start dev server
npm run dev

# Build for production
npm run build

# Copy to iPlug2 resources
./copy_web_resources.sh
```

### C++ Development

```bash
# Configure (Xcode example)
cmake --preset=macos-xcode

# Build
cmake --build --preset=macos-xcode

# Run tests
./iPlug2OOS/ArcadiaVoices/tests/main
```

### Full Build Pipeline

```bash
# Build frontend
npm run build

# Copy resources
./copy_web_resources.sh

# Configure CMake
cmake --preset=macos-xcode

# Build plugin
cmake --build --preset=macos-xcode

# Create installer
./build_installer.sh
```

### Message Bridge Wiring

**Critical Integration Points:**

1. **UI → DSP Messages:**
   - Knob values (time, feedback, mix, mod, pitchL, pitchR)
   - Power toggle, sync mode, preset commands

2. **DSP → UI Messages:**
   - Live pitch detection values
   - Current power state
   - Preset list + active preset ID
   - All parameter values (WebView hydration)

3. **App.tsx Integration:**
   - Import and use `useBridge` hook
   - All change handlers must call `sendKnobValue()`

4. **WebView Hydration:**
   - `OnIdle()` sends parameter values every ~10Hz
   - UI subscribes via `subscribeToMessageId()`

### Serialization

**Critical for State Preservation:**

```cpp
// CORRECT pattern - must call base class
bool MyPlugin::SerializeState(IByteChunk& chunk) const {
    if (!Plugin::SerializeState(chunk)) return false;  // saves all params
    // then write custom data...
}

int MyPlugin::UnserializeState(const IByteChunk& chunk, int startPos) {
    int pos = Plugin::UnserializeState(chunk, startPos);  // restores all params
    if (pos < 0) return startPos;
    // then read custom data from pos...
    return pos;
}
```

## Troubleshooting

### Common Issues & Solutions

**Knob Drag Not Working**
- Ensure `setPointerCapture` is implemented in knob components
- Test in both dev server and compiled plugin
- Check WKWebView pointer event handling

**Blank GUI**
- Verify `copy_web_resources.sh` ran successfully
- Check `Contents/Resources/web/` in built plugin
- Ensure WebView resources are properly bundled

**Controls Have No Effect**
- Wire `useBridge` in App.tsx
- Call `sendKnobValue()` from all change handlers
- Verify message bridge is connected

**Parameters Reset on Reload**
- Call base class in `SerializeState`/`UnserializeState`
- Verify serialization includes all parameters
- Check DAW project save/load cycle

**Preset System Issues**
- Move `PresetManager.Init()` to constructor (not `Init()`)
- Store complete state (all parameters + sync/power state)
- Verify preset serialization in `UnserializeState`

**UI Shows Wrong Values**
- Implement `OnIdle()` hydration messages
- Subscribe to messages via `subscribeToMessageId()`
- Check WebView reload behavior

### Debugging Tools

- **pluginval**: VST3 validation tool
- **auval**: Audio Unit validation tool
- **DAW console**: Check for error messages
- **Browser dev tools**: For WebView debugging (if available)

## System Requirements

### macOS
- **OS Version**: macOS 10.15 (Catalina) or later
- **CPU**: Intel processor (Apple Silicon via Rosetta)
- **RAM**: 4GB minimum (8GB recommended)
- **Plugin Formats**: VST3, AU
- **Sample Rates**: Up to 48kHz
- **DAW Compatibility**: Logic, Ableton, Reaper, Pro Tools, etc.

## Best Practices

### Parameter Management
- Map UI values to real units in TypeScript
- DSP works only with real units (ms, %, semitones)
- Centralize mapping logic in `useBridge` hook

### State Preservation
- Call base class serialization methods
- Store complete state (all parameters + presets)
- Test DAW project save/load cycles

### Performance
- Use bitmask wrapping for circular buffers
- Smooth parameter changes to avoid clicks
- Stop pitch detection when bypassed
- Optimize WebView message frequency

### UI Responsiveness
- Implement pointer capture for all drag interactions
- Hydrate UI state on WebView reload
- Animate transitions (LED → screen fade-in)
- Use requestAnimationFrame for smooth animations

### Testing
- Verify serialization by saving/reloading DAW projects
- Test WebView reload behavior
- Check bypass functionality
- Validate preset save/load cycles
- Test across multiple DAWs

## Current Status

**All phases completed (as of 2026-07-08):**
- ✅ DSP core with all features
- ✅ iPlug2 integration
- ✅ WebView UI implementation
- ✅ Message bridge (verified end-to-end)
- ✅ Packaging (installer created)
- ✅ Documentation (this guide)

The project is in a finished, production-ready state with all major components implemented and integration bugs resolved.

## File Structure

### Essential Files & Directories

```
ArcadiaVoices/
├── README.md                          # User documentation
├── ARCADIA_VOICES_COMPLETE_GUIDE.md   # This technical guide
├── AV.vst3                            # VST3 plugin bundle
├── AV.component                       # Audio Unit bundle
├── guidelines/                        # Development guidelines
├── src/                              # React frontend source
├── iPlug2OOS/                         # C++ plugin implementation
├── Installer/                         # Built installers
├── build_installer.sh                 # Packaging script
├── copy_web_resources.sh              # Resource bundler
└── package.json                       # Frontend dependencies
```

### Build Artifacts

- `AV.vst3` - VST3 plugin (1.3MB)
- `AV.component` - Audio Unit (1.0MB)
- `Installer/AV-1.0.pkg` - macOS installer
- `Installer/AV-1.0.dmg` - Disk image

## Future Enhancements

### Potential Features
- Windows version (VST3 support)
- Additional factory presets
- MIDI learn/automation
- Advanced modulation shapes
- Preset import/export
- A/B comparison
- Undo/redo functionality

### Technical Improvements
- WebAssembly version for browser use
- iOS/AUV3 support
- CLAP format support
- Improved pitch detection algorithms
- Granular synthesis options
- Multi-channel processing

---

**Arcadia Voices** - Professional Pitch-Shifting Delay Plugin
**© 2026 Archangel DSP** | All rights reserved

For support: support@archangeldsp.com
For documentation: https://docs.archangeldsp.com/arcadia-voices
