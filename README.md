# Arcadia Voices (AV) - Multi-Voice Pitch Delay Plugin

![Arcadia Voices](Assets/Delay_UI.png)

[![Sponsor](https://img.shields.io/badge/Sponsor-%E2%9D%A4-pink?logo=github)](https://github.com/sponsors/jfachriz)

**A professional stereo pitch-shifting delay plugin with tempo sync, modulation, and live pitch detection.**

## 🎯 What is Arcadia Voices?

Arcadia Voices is a **stereo delay plugin** where each channel (left/right) is independently pitch-shifted, creating rich harmonic textures from your audio. Unlike traditional delays that repeat at the same pitch, AV transforms your sound into cascading musical intervals.

### Key Features

- **Dual Pitch-Shifting Delay** - Independent pitch control for left (-24 to +24 semitones) and right (-24 to +24 semitones) channels
- **Tempo Sync** - Sync delay time to your DAW's tempo with multiple note divisions (1/32 to 2 bars)
- **Modulation** - Add chorus/wobble effects to your repeats with adjustable depth
- **Live Pitch Detection** - Visual feedback showing detected input pitch on dot-matrix displays
- **Preset System** - Save and recall your favorite settings with an intuitive on-screen menu
- **True Bypass** - Zero processing when powered off for pristine signal path
- **Professional UI** - Guitar pedal-style interface with responsive knobs and LED indicators

## 🎛️ Controls & Parameters

### Main Controls

| Control | Range | Description |
|---------|-------|-------------|
| **TIME** | 1ms - 500ms | Base delay time (logarithmic scale) |
| **FEEDBACK** | 0% - 100% | Repeat regeneration (over 100% for runaway effects) |
| **MIX** | 0% - 100% | Dry/wet balance |
| **MOD** | 0% - 100% | Modulation depth for chorus/wobble effects |
| **PITCH L** | -24 to +24 semitones | Left channel pitch shifting |
| **PITCH R** | -24 to +24 semitones | Right channel pitch shifting |
| **POWER** | ON/OFF | True bypass switch |
| **SYNC** | Free/Synced | Toggle tempo synchronization |
| **DIVISION** | 1/32 to 2 bars | Note division when synced (appears when SYNC=ON) |

### Factory Presets

- **Multi Voice** - Classic interval delay with left channel pitched down, right channel pitched up
- **Chorus Room** - Short delay with modulation for lush spatial effects
- **Deep Space** - Long, atmospheric delays with extreme pitch shifting

## 🚀 Quick Start

### Basic Setup

1. **Insert** Arcadia Voices on your audio track
2. **Turn on** the power switch (green LED indicates active)
3. **Set TIME** to your desired delay length (try 350ms for a good starting point)
4. **Adjust PITCH L/R** to create your desired interval (e.g., -2 and +1 semitones)
5. **Dial in FEEDBACK** to control how many repeats you hear
6. **Set MIX** to balance dry and wet signals

### Pro Tips

- **For shimmer effects**: Set PITCH R to +12 or +24 semitones with moderate feedback
- **For wide stereo**: Set PITCH L to -12 and PITCH R to +12 semitones
- **For rhythmic patterns**: Enable SYNC and choose a note division that matches your tempo
- **For modulation**: Increase MOD for chorus-like effects on repeats
- **For infinite repeats**: Set FEEDBACK to 100% (use with caution!)

## 🎵 Use Cases

### Guitar & Bass
- **Ambient swells** - Long delays with pitch shifting create atmospheric textures
- **Harmonic layers** - Add octave shifts to create fuller sound
- **Rhythmic patterns** - Sync to tempo for locked-in delay patterns

### Vocals
- **Doubling effect** - Short delays with slight pitch variation
- **Harmony generator** - Create automatic harmonies from single vocal lines
- **Space creator** - Add depth and dimension to lead vocals

### Synths & Keys
- **Arpeggiator-like effects** - Short synced delays with pitch shifts
- **Pad creator** - Long delays with modulation for lush backgrounds
- **Glitch effects** - Extreme pitch shifts with high feedback

### Drums & Percussion
- **Rhythmic echoes** - Synced delays that lock to your groove
- **Pitch-shifted tails** - Add musical character to drum hits
- **Spatial enhancement** - Create width in your drum mix

## 🔧 System Requirements

### macOS
- **OS Version**: macOS 10.15 (Catalina) or later
- **CPU**: Intel processor (Apple Silicon supported via Rosetta)
- **RAM**: 4GB minimum (8GB recommended)
- **Plugin Formats**: VST3, AU
- **Sample Rates**: Up to 48kHz
- **DAW Compatibility**: All major DAWs (Logic, Ableton, Reaper, Pro Tools, etc.)

## 📚 Preset Management

### Saving Presets

1. Click the preset number display (e.g., "01") to open the preset menu
2. Adjust your settings to the desired sound
3. Click "Save as New" to store your preset
4. Your new preset will appear in the list with an auto-generated name

### Loading Presets

1. Click the preset number display to open the menu
2. Browse the preset list
3. Click any preset to load it immediately
4. Click "Cancel" to close without changing presets

### Factory Presets

The plugin comes with three factory presets:
- **01: Multi Voice** - Classic interval delay
- **02: Chorus Room** - Short modulated delay
- **03: Deep Space** - Long atmospheric delay

## 🎛️ Advanced Features

### Tempo Sync

Enable tempo synchronization for rhythmic delay patterns:

1. Click the **SYNC** toggle above the TIME knob
2. The **DIVISION** selector will appear
3. Click the division button to cycle through options (1/32, 1/16, 1/8, 1/4, 1/2, 1 bar, 2 bars)
4. The delay time will automatically adjust to match your DAW's tempo

### Live Pitch Detection

The dot-matrix displays show real-time pitch detection:
- Left display: Detected pitch for left channel input
- Right display: Detected pitch for right channel input
- Format shows note name + cents offset (e.g., "C# +12")
- Detection stops when plugin is bypassed

### True Bypass

- **ON**: Full audio processing with pitch-shifting delay
- **OFF**: Complete bypass - your signal passes through untouched
- LED indicator shows power state (green = on, gray = off)
- Screen content fades in/out with power toggle

## 🐞 Troubleshooting

### Plugin Doesn't Appear in DAW

- **Check installation location**: Ensure files are in correct plugin folders
- **Rescan plugins**: Most DAWs have a "Rescan Plugins" option
- **Restart DAW**: Sometimes a simple restart is needed
- **Check format**: Ensure your DAW supports VST3 or AU formats

### No Sound When Plugin is Active

- **Check power**: Make sure the power switch is ON (green LED)
- **Check mix**: Ensure MIX knob is not at 0%
- **Check bypass**: Some DAWs have their own bypass - make sure it's not engaged
- **Check input**: Verify your track has audio input

### GUI Doesn't Load

- **WebView requirement**: The plugin uses WebView for its interface
- **macOS version**: Ensure you're running macOS 10.15 or later
- **DAW compatibility**: Try in a different DAW to isolate the issue

### Controls Don't Respond

- **Click and drag**: All knobs use drag interactions
- **Pointer capture**: If knobs stop responding, click again to recapture
- **DAW automation**: Ensure your DAW isn't overriding plugin controls

## 📜 License

Arcadia Voices is proprietary software developed by Archangel DSP. All rights reserved.

For license terms and usage rights, please refer to the included LICENSE file or contact:

**Archangel DSP**
Support: archangeldsp@gmail.com
Website: https://archangeldsp.sbs

## 🙏 Support & Feedback

### ❤️ Support the Project

If you find Arcadia Voices useful, please consider supporting its development!
[![Sponsor](https://img.shields.io/badge/Sponsor-%E2%9D%A4-pink?logo=github)](https://github.com/sponsors/jfachriz)

### Getting Help

- **Documentation**: This README and GUIDE.md
- **Presets**: Explore factory presets for inspiration

### Reporting Issues

Found a bug or have a feature request? Please include:
- Your macOS version
- DAW name and version
- Steps to reproduce the issue
- Expected vs. actual behavior

### Feature Requests

We love hearing your ideas! While we can't promise every feature will be implemented, we carefully consider all suggestions that align with our vision for Arcadia Voices.

## 🎉 What's Next?

Arcadia Voices is just the beginning! Stay tuned for:
- **Windows version** - Coming soon
- **Additional presets** - Expanding our factory library
- **Advanced features** - Based on user feedback

Follow us on social media for updates and tutorials:
- Twitter: @ArchangelDSP
- Instagram: @ArchangelDSP
- YouTube: Archangel DSP

## 🔒 Privacy

Arcadia Voices does not collect or transmit any personal data. All preset storage and plugin state is local to your computer.

## 📝 Changelog

### Version 1.1 (Current)
- Initial release
- VST3 and AU formats for macOS
- Full feature set including pitch-shifting, tempo sync, and preset system
- Three factory presets
- Complete documentation

## 🤝 Credits

**Developed by**: Archangel DSP
**DSP Engineering**: The Arcadia Voices Team
**UI/UX Design**: [Design Team]
**Testing**: Our amazing beta testers

Special thanks to our users and community for inspiration and feedback!

---

**Arcadia Voices** - Transform your sound with musical delays
**© 2026 Archangel DSP** | All rights reserved

[Download Now](https://archangeldsp.sbs/arcadia-voices) | [User Guide](https://github.com/jfachriz/ArcadiaVoices/blob/main/guidelines/GUIDE.md) | [Support](mailto:archangeldsp@gmail.com)
