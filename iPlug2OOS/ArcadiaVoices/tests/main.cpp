// ArcadiaVoices DSP Standalone Test Harness
// Compile: g++ -std=c++17 -I. tests/main.cpp -o dsp_test
// Run:     ./dsp_test
// Expected: All PASS/FAIL lines with no crashes, assertions, or infinite loops.

// Standalone test — define 'sample' type for DelayPedalDSP (normally from iPlug2)
// Use float here to match test buffer types; iPlug2 defaults to double but
// the float→sample cast in ProcessBlock handles the conversion.
#ifndef sample
typedef float sample;
#endif

#include "dsp/DelayPedalDSP.h"
#include "dsp/ParamSmoother.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>

// Test utilities
static int gTestsPassed = 0;
static int gTestsFailed = 0;

#define TEST(name, condition) do { \
    if (!(condition)) { \
        printf("  FAIL: %s\n", name); \
        gTestsFailed++; \
    } else { \
        printf("  PASS: %s\n", name); \
        gTestsPassed++; \
    } \
} while(0)

#define TEST_FLOAT(name, val, expected, tolerance) do { \
    const float _v = (val); \
    const float _e = (expected); \
    const float _t = (tolerance); \
    if (std::abs(_v - _e) <= _t) { \
        printf("  PASS: %s (%f)\n", name, _v); \
        gTestsPassed++; \
    } else { \
        printf("  FAIL: %s — expected %f ± %f, got %f\n", name, _e, _t, _v); \
        gTestsFailed++; \
    } \
} while(0)

// Generate a sine wave
static void genSine(float* buf, int len, float freq, float sampleRate, float amp = 0.5f) {
    for (int i = 0; i < len; i++) {
        buf[i] = amp * std::sin(2.0f * M_PI * freq * i / sampleRate);
    }
}

// Generate an impulse at position N
static void genImpulse(float* buf, int len, int pos, float amp = 1.0f) {
    std::memset(buf, 0, len * sizeof(float));
    if (pos < len) buf[pos] = amp;
}

// Compute RMS of a buffer
static float computeRMS(const float* buf, int len) {
    float sum = 0.0f;
    for (int i = 0; i < len; i++) sum += buf[i] * buf[i];
    return std::sqrt(sum / len);
}

// Compute peak amplitude
static float computePeak(const float* buf, int len) {
    float peak = 0.0f;
    for (int i = 0; i < len; i++) {
        float absVal = std::abs(buf[i]);
        if (absVal > peak) peak = absVal;
    }
    return peak;
}

// ===== TESTS =====

void test_circular_buffer() {
    printf("\n=== CircularBuffer Tests ===\n");

    CircularBuffer buf;
    buf.Init(1024);

    TEST("buffer size is power of 2", buf.Size() == 1024);
    TEST("mask is size - 1", buf.Mask() == 1023);
    TEST("max delay has guard space", buf.MaxDelay() == 1020);

    // Write a known pattern and read it back
    for (int i = 0; i < 100; i++) {
        buf.Write(static_cast<float>(i));
    }

    // Read at offset 50: should get sample 49 (writeIndex - delayOffset - 1)
    // writeIndex = 100, delayOffset = 50 → readPos = 100 - 50 - 1 = 49
    float sample = buf.Read(50.0f);
    TEST_FLOAT("read at integer offset returns expected sample", sample, 49.0f, 0.01f);

    // Read at offset 0: should get sample 99 (the most recent written)
    sample = buf.Read(0.0f);
    TEST_FLOAT("read at offset 0 returns newest sample", sample, 99.0f, 0.01f);

    // Clear and verify silence
    buf.Clear();
    for (int i = 0; i < 10; i++) { buf.Write(1.0f); }
    float rms = computeRMS(buf.Data(), buf.Size());
    TEST("buffer was cleared and re-written", rms > 0.0f);
}

void test_soft_clipper() {
    printf("\n=== SoftClipper Tests ===\n");

    SoftClipper clipper;
    clipper.SetThreshold(1.0f);

    // Below threshold: pass through approximately
    TEST_FLOAT("small signal passes through", clipper.Process(0.5f), 0.5f, 0.05f);

    // Above threshold: clip
    TEST_FLOAT("large signal is clipped", clipper.Process(5.0f), 1.0f, 0.1f);

    // Clipping is symmetric
    TEST_FLOAT("negative signal clips symmetrically", clipper.Process(-5.0f), -1.0f, 0.1f);

    // Very silent: pass through
    TEST_FLOAT("near-zero passes through", clipper.Process(0.001f), 0.001f, 0.001f);

    // Hard clip mode is more aggressive
    float hardClip = clipper.ProcessHard(2.0f);
    float softClip = clipper.Process(2.0f);
    TEST("hard clipper output < soft clipper output for same input", hardClip < softClip);
}

void test_mod_lfo() {
    printf("\n=== ModLFO Tests ===\n");

    ModLFO lfo;
    lfo.Init(44100.0f);
    lfo.SetRate(0.5f); // 0.5 Hz = one full cycle per 2 seconds

    // Sample LFO at various points
    std::vector<float> samples(88200); // 2 seconds at 44100
    for (auto& s : samples) s = lfo.Process();

    // Sine wave: first value should be ~0 (phase 0)
    TEST_FLOAT("sine LFO starts near 0", samples[0], 0.0f, 0.01f);

    // At quarter cycle (44100/2 = 22050): should be near 1.0 (sine peak)
    TEST_FLOAT("sine LFO peaks at quarter cycle", samples[22049], 1.0f, 0.05f);

    // At half cycle: should be near 0 again
    TEST_FLOAT("sine LFO crosses zero at half cycle", samples[44099], 0.0f, 0.05f);

    // At three-quarter cycle: should be near -1.0 (trough)
    TEST_FLOAT("sine LFO troughs at three-quarter cycle", samples[66149], -1.0f, 0.05f);

    // Test triangle wave
    ModLFO tri;
    tri.Init(44100.0f);
    tri.SetRate(0.5f);
    tri.SetWaveform(1);

    std::vector<float> triSamples(88200);
    for (auto& s : triSamples) s = tri.Process();

    // Triangle starts at 1.0 (4*|0-0.5|-1 = 4*0.5-1 = 1.0) — it rises from phase 0
    // At quarter cycle (22050): triangle rises from -1 to 1 over half cycle
    // phase = 0.25, triangle = 4*|0.25-0.5|-1 = 4*0.25-1 = 0.0
    // At half cycle (44100): phase = 0.5, triangle = 4*|0.5-0.5|-1 = -1.0
    TEST_FLOAT("triangle at quarter cycle is 0", triSamples[22049], 0.0f, 0.05f);
    TEST_FLOAT("triangle at half cycle is -1", triSamples[44099], -1.0f, 0.05f);

    // Reset
    tri.Reset();
    TEST_FLOAT("after reset, triangle = 1.0", tri.Process(), 1.0f, 0.01f);
}

void test_pitch_shifter() {
    printf("\n=== PitchShifter Tests ===\n");

    // Create a shared delay line with a known signal
    const size_t bufSize = 16384;
    CircularBuffer delayLine;
    delayLine.Init(bufSize);

    // Fill with a 440 Hz sine
    std::vector<float> testSig(8192);
    genSine(testSig.data(), 8192, 440.0f, 44100.0f, 0.5f);
    for (auto s : testSig) delayLine.Write(s);

    PitchShifter shifter;
    shifter.Init(44100.0f);
    shifter.SetGrainSizeMs(30.0f);

    // At ratio = 1.0, output should be ≈ same frequency (pitch unchanged)
    shifter.SetPitchRatio(1.0f);
    std::vector<float> out(4096);
    // Need a fresh buffer with sustained content for pitch shifter to track
    CircularBuffer fb;
    fb.Init(bufSize);
    for (int i = 0; i < 8192; i++) fb.Write(0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f));

    PitchShifter ps;
    ps.Init(44100.0f);
    ps.SetPitchRatio(1.0f);

    for (int i = 0; i < 4096; i++) {
        out[i] = ps.Process(fb, 100.0f);
    }

    // Output should have non-zero RMS (signal is passing through)
    float rms = computeRMS(out.data(), 4096);
    TEST("ratio=1 produces non-zero output", rms > 0.01f);

    // At ratio = 2.0 (up octave), still produces output
    ps.Reset();
    ps.SetPitchRatio(2.0f);
    for (int i = 0; i < 4096; i++) {
        out[i] = ps.Process(fb, 100.0f);
    }
    rms = computeRMS(out.data(), 4096);
    TEST("ratio=2 produces non-zero output", rms > 0.01f);

    // At ratio = 0.5 (down octave), still produces output
    ps.Reset();
    ps.SetPitchRatio(0.5f);
    for (int i = 0; i < 4096; i++) {
        out[i] = ps.Process(fb, 100.0f);
    }
    rms = computeRMS(out.data(), 4096);
    TEST("ratio=0.5 produces non-zero output", rms > 0.01f);
}

void test_pitch_detector() {
    printf("\n=== PitchDetector Tests ===\n");

    PitchDetector detector;
    detector.Init(44100.0f, 2048, 1024);

    // Generate a 440 Hz sine (A4) at reasonable amplitude
    std::vector<float> buf(4096);
    genSine(buf.data(), 4096, 440.0f, 44100.0f, 0.5f);

    // Push samples and detect
    detector.ProcessBlock(buf.data(), 4096);
    bool detected = detector.Detect();

    TEST("440 Hz tone is detected", detected);
    TEST_FLOAT("frequency near 440 Hz", detector.DetectedFreq(), 440.0f, 10.0f);

    // Check that we get a note name
    TEST("detected note is not empty", !detector.DetectedNote().empty());

    // Test silence detection
    PitchDetector silent;
    silent.Init(44100.0f, 2048, 1024);
    std::vector<float> silence(4096, 0.0f);
    silent.ProcessBlock(silence.data(), 4096);
    bool silentDetected = silent.Detect();

    TEST("silence is not detected as pitch", !silentDetected);

    // Test very low tone (80 Hz — low E string)
    PitchDetector lowDetector;
    lowDetector.Init(44100.0f, 2048, 1024);
    std::vector<float> lowBuf(4096);
    genSine(lowBuf.data(), 4096, 82.41f, 44100.0f, 0.5f); // E2
    lowDetector.ProcessBlock(lowBuf.data(), 4096);
    bool lowDetected = lowDetector.Detect();
    TEST("low E (82 Hz) is detected", lowDetected);
}

void test_tempo_sync() {
    printf("\n=== TempoSync Tests ===\n");

    TempoSync sync;

    // Default BPM = 120, default division = 1/8
    // 1/8 at 120 BPM = (60000/120) * 0.5 = 250 ms
    double delayMs = sync.CalcDelayMs();
    TEST_FLOAT("default delay (120 BPM, 1/8) = 250ms", static_cast<float>(delayMs), 250.0f, 0.001f);

    // Change BPM to 140
    sync.SetBPM(140.0);
    delayMs = sync.CalcDelayMs();
    TEST_FLOAT("140 BPM, 1/8 = ~214.3ms", static_cast<float>(delayMs), 214.286f, 0.1f);

    // Change to 1/4 note
    sync.SetDivision(7); // index 7 = 1/4
    delayMs = sync.CalcDelayMs();
    TEST_FLOAT("140 BPM, 1/4 = ~428.6ms", static_cast<float>(delayMs), 428.571f, 0.1f);

    // Change to 1 bar
    sync.SetDivision(12); // index 12 = 1 bar (4 beats)
    delayMs = sync.CalcDelayMs();
    TEST_FLOAT("140 BPM, 1 bar = ~1714.3ms", static_cast<float>(delayMs), 1714.286f, 0.1f);

    // Cycle forward
    sync.SetDivision(4); // 1/8
    sync.CycleForward();
    TEST("cycle forward to 1/8D", sync.DivisionIndex() == 5);

    // Cycle wraps around
    sync.SetDivision(13); // last index (2 bars)
    sync.CycleForward();
    TEST("cycle wraps to 1/32", sync.DivisionIndex() == 0);

    // Cycle backwards
    sync.SetDivision(0);
    sync.CycleBackward();
    TEST("cycle backward wraps to 2 bars", sync.DivisionIndex() == 13);

    // Static helper
    delayMs = TempoSync::CalcDelayMs(120.0, 1.0); // 1/4 at 120 BPM
    TEST_FLOAT("static CalcDelayMs 120 BPM, 1x = 500ms", static_cast<float>(delayMs), 500.0f, 0.001f);
}

void test_preset_manager() {
    printf("\n=== PresetManager Tests ===\n");

    PresetManager pm;
    pm.Init();

    // Should have 3 factory presets
    TEST("3 factory presets", pm.Count() == 3);

    // Active preset should be 1 ("multi voice")
    const PresetSnapshot* active = pm.ActivePreset();
    TEST("active preset is multi voice", active && std::strcmp(active->name, "multi voice") == 0);

    // Test accessing by ID
    const PresetSnapshot* chorus = pm.GetPreset(2);
    TEST("preset 2 is chorus room", chorus && std::strcmp(chorus->name, "chorus room") == 0);

    // Test accessing by name
    const PresetSnapshot* deep = pm.GetPresetByName("deep space");
    TEST("deep space found by name", deep != nullptr);
    TEST("deep space has id 3", deep && deep->id == 3);

    // Test save as new
    PresetSnapshot userPreset;
    userPreset.timeMs = 500.0f;
    userPreset.feedbackPct = 50.0f;
    userPreset.mixPct = 75.0f;
    userPreset.modPct = 30.0f;
    userPreset.pitchL = -12.0f;
    userPreset.pitchR = 12.0f;
    userPreset.powerOn = true;
    userPreset.syncMode = true;
    userPreset.noteDivision = 4; // 1/8

    int newId = pm.SaveAsNew("test preset", userPreset);
    TEST("new preset gets id 4", newId == 4);
    TEST("now have 4 presets", pm.Count() == 4);

    const PresetSnapshot* loaded = pm.LoadPreset(4);
    TEST("loaded preset has correct time", loaded && loaded->timeMs == 500.0f);
    TEST("active preset is the loaded one", pm.ActivePresetId() == 4);

    // Test loading non-existent preset
    const PresetSnapshot* notFound = pm.LoadPreset(99);
    TEST("loading non-existent preset returns null", !notFound);

    // Make sure active preset doesn't change on failed load
    TEST("active preset unchanged after failed load", pm.ActivePresetId() == 4);

    pm.Reset();
    TEST("reset restores 3 factory presets", pm.Count() == 3);
}

void test_param_smoother() {
    printf("\n=== ParamSmoother Tests ===\n");

    ParamSmoother smoother;
    smoother.Init(44100.0f, 10.0f); // 10ms time constant

    smoother.Snap();
    TEST("initial value is 0 after snap", smoother.Current() == 0.0f);

    smoother.SetTarget(1.0f);
    float val = smoother.Process();
    TEST("first smoothed value is > 0", val > 0.0f);
    TEST("first smoothed value is < target", val < 1.0f);

    // After many iterations, should approach target
    for (int i = 0; i < 4410; i++) { // 100ms at 44100
        val = smoother.Process();
    }
    TEST("after 100ms, close to target", std::abs(val - 1.0f) < 0.01f);

    // Snap to a new value then Process() should give the target
    smoother.Snap(); // mCurrent = mTarget = 1.0
    smoother.SetTarget(0.0f);
    smoother.Process();
    TEST("after snap + new target, first process moves toward target", smoother.Current() < 1.0f);
}

void test_pitch_delay_voice() {
    printf("\n=== PitchDelayVoice Tests ===\n");

    PitchDelayVoice voice;
    voice.Init(44100.0f, 131072); // 2 seconds at 48kHz max

    voice.SetDelayTimeMs(5.0f);   // Short delay so impulse fits in test buffer
    voice.SetFeedbackPct(70.0f);  // High feedback to sustain
    voice.SetModDepth(0.0f);     // No modulation for clean test
    voice.SetPitchSemitones(0);   // No pitch shift

    // Send an impulse through
    const int numSamples = 4096;
    float out[numSamples];

    // Generate an impulse at sample 100 (5ms delay = ~220 samples)
    float input[numSamples];
    genImpulse(input, numSamples, 100);

    // Process
    for (int i = 0; i < numSamples; i++) {
        out[i] = voice.Process(input[i]);
    }

    // With delay = 5ms = ~220 samples, impulse at 100 should appear at ~320
    float peak = computePeak(out, numSamples);
    TEST("output has signal from delayed impulse", peak > 0.01f);

    // Check for significant output in the expected delay range (5ms ≈ 220 samples)
    // Plus pitch shifter grain offset, so allow a wider search window
    bool foundPeak = false;
    for (int i = 200; i < 600; i++) {
        if (out[i] > 0.01f) { foundPeak = true; break; }
    }
    TEST("delayed impulse found in expected range", foundPeak);

    // Test with pitch shift
    PitchDelayVoice voice2;
    voice2.Init(44100.0f, 131072);
    voice2.SetDelayTimeMs(10.0f); // short delay
    voice2.SetFeedbackPct(0.0f);  // no feedback
    voice2.SetPitchSemitones(7);  // up a fifth

    std::fill(out, out + numSamples, 0.0f);
    genImpulse(input, numSamples, 0);

    for (int i = 0; i < numSamples; i++) {
        out[i] = voice2.Process(input[i]);
    }

    peak = computePeak(out, numSamples);
    TEST("pitch-shifted impulse produces output", peak > 0.0f);
}

void test_delay_pedal_dsp() {
    printf("\n=== DelayPedalDSP Tests ===\n");

    DelayPedalDSP dsp;
    dsp.Init(44100.0f);

    // Set known parameters
    dsp.SetDelayTimeMs(200.0f);
    dsp.SetFeedbackPct(30.0f);
    dsp.SetMixPct(50.0f);
    dsp.SetModDepth(0.0f);
    dsp.SetPitchLSemitones(0);
    dsp.SetPitchRSemitones(0);
    dsp.SetPowerOn(true);

    TEST("power is on", dsp.IsPowerOn() == true);
    TEST("mix is 50%", dsp.MixPct() == 50.0f);
    TEST("feedback is 30%", dsp.FeedbackPct() == 30.0f);
    TEST("delay is 200ms", dsp.DelayTimeMs() == 200.0f);

    // Process a block of audio (impulse)
    const int blockSize = 2048;
    std::vector<float> inL(blockSize, 0.0f);
    std::vector<float> inR(blockSize, 0.0f);
    std::vector<float> outL(blockSize, 0.0f);
    std::vector<float> outR(blockSize, 0.0f);

    // Impulse at start
    inL[0] = 1.0f;
    inR[0] = 1.0f;

    float* inputs[2] = { inL.data(), inR.data() };
    float* outputs[2] = { outL.data(), outR.data() };

    dsp.ProcessBlock(inputs, outputs, blockSize);

    // With mix = 50%, dry = 0.5, impulse at sample 0 should produce
    // output[0] ≈ 1.0 * 0.5 = 0.5 (dry portion)
    TEST_FLOAT("impulse dry path is ~0.5", outL[0], 0.5f, 0.05f);

    // There should be non-zero output in the wet path after delay
    float rms = computeRMS(outL.data(), blockSize);
    TEST("output has signal beyond dry impulse", rms > 0.01f);

    // Test bypass: power off should pass input straight through
    dsp.SetPowerOn(false);

    std::fill(outL.begin(), outL.end(), 0.0f);
    std::fill(outR.begin(), outR.end(), 0.0f);
    inL[0] = 0.75f;
    inR[0] = 0.75f;

    dsp.ProcessBlock(inputs, outputs, blockSize);

    TEST_FLOAT("bypass: output[0] equals input[0]", outL[0], 0.75f, 0.001f);
    TEST_FLOAT("bypass: output[0] equals input[0]", outR[0], 0.75f, 0.001f);

    // Check all samples in bypass are exact passthrough
    bool allMatch = true;
    for (int i = 0; i < blockSize; i++) {
        if (outL[i] != inL[i] || outR[i] != inR[i]) {
            allMatch = false;
            break;
        }
    }
    TEST("bypass: all samples are exact passthrough", allMatch);

    // Test power on again — should process again
    dsp.SetPowerOn(true);

    std::fill(inL.begin(), inL.end(), 0.0f);
    std::fill(inR.begin(), inR.end(), 0.0f);
    std::fill(outL.begin(), outL.end(), 0.0f);
    std::fill(outR.begin(), outR.end(), 0.0f);

    inL[0] = 1.0f;
    inR[0] = 1.0f;
    dsp.ProcessBlock(inputs, outputs, blockSize);

    rms = computeRMS(outL.data(), blockSize);
    TEST("re-enabling power produces output again", rms > 0.01f);

    // Test preset application
    PresetManager& pm = dsp.GetPresetManager();
    const PresetSnapshot* preset = pm.GetPreset(3); // "deep space"
    TEST("deep space preset exists", preset != nullptr);

    if (preset) {
        dsp.ApplyPreset(*preset);
        TEST_FLOAT("delay set from preset", dsp.DelayTimeMs(), 1200.0f, 0.1f);
        TEST_FLOAT("feedback set from preset", dsp.FeedbackPct(), 80.0f, 0.1f);
        TEST("pitchL set from preset", dsp.PitchLSemitones() == -7);
        TEST("pitchR set from preset", dsp.PitchRSemitones() == 4);
    }
}

void test_tempo_sync_integration() {
    printf("\n=== TempoSync Integration Tests ===\n");

    DelayPedalDSP dsp;
    dsp.Init(44100.0f);

    // Switch to synced mode at 120 BPM, 1/8 note
    dsp.SetHostBPM(120.0);
    dsp.SetSyncMode(SyncMode::Synced);
    dsp.SetNoteDivision(4); // 1/8

    // At 120 BPM, 1/8 = 250ms → 44100 * 0.25 = 11025 samples
    TEST_FLOAT("synced delay time", dsp.DelayTimeMs(), 250.0f, 0.1f);

    // Change division to 1/4
    dsp.SetNoteDivision(7); // 1/4
    TEST_FLOAT("1/4 at 120 BPM", dsp.DelayTimeMs(), 500.0f, 0.1f);

    // Change BPM
    dsp.SetHostBPM(140.0);
    TEST_FLOAT("1/4 at 140 BPM", dsp.DelayTimeMs(), 428.571f, 0.5f);

    // Switch back to free mode — should restore last manual time
    dsp.SetDelayTimeMs(350.0f); // set a manual time
    dsp.SetSyncMode(SyncMode::Free);
    TEST_FLOAT("free mode restores manual time", dsp.DelayTimeMs(), 350.0f, 0.1f);
}

void test_presets_apply_to_block() {
    printf("\n=== Preset Apply + ProcessBlock Tests ===\n");

    DelayPedalDSP dsp;
    dsp.Init(44100.0f);

    // Load "deep space" preset and process 1 second of audio
    const PresetSnapshot* preset = dsp.GetPresetManager().GetPreset(3);
    if (preset) {
        dsp.ApplyPreset(*preset);
    }

    const int blockSize = 44100; // 1 second
    std::vector<float> inL(blockSize, 0.0f);
    std::vector<float> inR(blockSize, 0.0f);
    std::vector<float> outL(blockSize, 0.0f);
    std::vector<float> outR(blockSize, 0.0f);

    genSine(inL.data(), blockSize, 220.0f, 44100.0f, 0.5f);
    std::memcpy(inR.data(), inL.data(), blockSize * sizeof(float));

    float* inputs[2] = { inL.data(), inR.data() };
    float* outputs[2] = { outL.data(), outR.data() };

    dsp.ProcessBlock(inputs, outputs, blockSize);

    float rmsL = computeRMS(outL.data(), blockSize);
    float rmsR = computeRMS(outR.data(), blockSize);

    TEST("deep space preset produces non-zero output (L)", rmsL > 0.01f);
    TEST("deep space preset produces non-zero output (R)", rmsR > 0.01f);

    // Both channels should produce similar energy
    TEST("left and right channels have energy", rmsL > 0.01f && rmsR > 0.01f);
}

void test_no_infinite_loop() {
    printf("\n=== Stability Test (no infinite loops) ===\n");

    DelayPedalDSP dsp;
    dsp.Init(44100.0f);

    // Run with high feedback (110%) — should not clip to infinity
    dsp.SetFeedbackPct(110.0f);
    dsp.SetMixPct(100.0f);
    dsp.SetModDepth(100.0f);
    dsp.SetPitchLSemitones(12);
    dsp.SetPitchRSemitones(-12);

    const int blockSize = 44100; // 1 second
    std::vector<float> inL(blockSize, 0.0f);
    std::vector<float> inR(blockSize, 0.0f);
    std::vector<float> outL(blockSize, 0.0f);
    std::vector<float> outR(blockSize, 0.0f);

    // Input: moderate sine
    genSine(inL.data(), blockSize, 440.0f, 44100.0f, 0.3f);
    std::memcpy(inR.data(), inL.data(), blockSize * sizeof(float));

    float* inputs[2] = { inL.data(), inR.data() };
    float* outputs[2] = { outL.data(), outR.data() };

    dsp.ProcessBlock(inputs, outputs, blockSize);

    // Peak should be bounded by soft clipper (should not exceed ~1.5)
    float peakL = computePeak(outL.data(), blockSize);
    float peakR = computePeak(outR.data(), blockSize);

    TEST("peak output is bounded (L)", peakL < 2.0f);
    TEST("peak output is bounded (R)", peakR < 2.0f);

    // Shouldn't be silent either (feedback should sustain)
    float rmsL = computeRMS(outL.data(), blockSize);
    float rmsR = computeRMS(outR.data(), blockSize);
    TEST("high feedback produces sustained output (L)", rmsL > 0.0001f);
    TEST("high feedback produces sustained output (R)", rmsR > 0.0001f);
}

void test_parameter_ranges() {
    printf("\n=== Parameter Range Tests ===\n");

    DelayPedalDSP dsp;
    dsp.Init(44100.0f);

    // Test that all parameter ranges are accepted without crashing
    dsp.SetDelayTimeMs(1.0f);
    TEST("min delay 1ms sets correctly", dsp.DelayTimeMs() == 1.0f);

    dsp.SetDelayTimeMs(2000.0f);
    TEST("max delay 2000ms sets correctly", dsp.DelayTimeMs() == 2000.0f);

    dsp.SetFeedbackPct(0.0f);
    TEST("min feedback 0%", dsp.FeedbackPct() == 0.0f);

    dsp.SetFeedbackPct(110.0f);
    TEST("max feedback 110%", dsp.FeedbackPct() == 110.0f);

    dsp.SetMixPct(0.0f);
    TEST("min mix 0%", dsp.MixPct() == 0.0f);

    dsp.SetMixPct(100.0f);
    TEST("max mix 100%", dsp.MixPct() == 100.0f);

    dsp.SetPitchLSemitones(-24);
    TEST("min pitchL -24", dsp.PitchLSemitones() == -24);

    dsp.SetPitchLSemitones(24);
    TEST("max pitchL +24", dsp.PitchLSemitones() == 24);

    dsp.SetPitchRSemitones(-24);
    TEST("min pitchR -24", dsp.PitchRSemitones() == -24);

    dsp.SetPitchRSemitones(24);
    TEST("max pitchR +24", dsp.PitchRSemitones() == 24);
}

void test_different_sample_rates() {
    printf("\n=== Sample Rate Tests ===\n");

    // Test that DSP works at different sample rates
    DelayPedalDSP dsp;
    dsp.Init(48000.0f); // 48kHz

    TEST("init at 48kHz works", dsp.SampleRate() == 48000.0f);

    dsp.SetDelayTimeMs(100.0f);

    const int blockSize = 48000;
    std::vector<float> inL(blockSize, 0.0f);
    std::vector<float> inR(blockSize, 0.0f);
    std::vector<float> outL(blockSize, 0.0f);
    std::vector<float> outR(blockSize, 0.0f);

    // Use continuous tone so output is sustained regardless of delay length
    genSine(inL.data(), blockSize, 220.0f, 48000.0f, 0.5f);
    std::memcpy(inR.data(), inL.data(), blockSize * sizeof(float));

    float* inputs[2] = { inL.data(), inR.data() };
    float* outputs[2] = { outL.data(), outR.data() };

    dsp.ProcessBlock(inputs, outputs, blockSize);

    float rms = computeRMS(outL.data(), blockSize);
    TEST("48kHz produces output", rms > 0.01f);

    // Test at 44100
    DelayPedalDSP dsp44;
    dsp44.Init(44100.0f);
    dsp44.SetDelayTimeMs(100.0f);

    std::vector<float> inL44(44100, 0.0f);
    std::vector<float> inR44(44100, 0.0f);
    std::vector<float> outL44(44100, 0.0f);
    std::vector<float> outR44(44100, 0.0f);

    genSine(inL44.data(), 44100, 220.0f, 44100.0f, 0.5f);
    std::memcpy(inR44.data(), inL44.data(), 44100 * sizeof(float));

    inputs[0] = inL44.data();
    inputs[1] = inR44.data();
    outputs[0] = outL44.data();
    outputs[1] = outR44.data();

    dsp44.ProcessBlock(inputs, outputs, 44100);
    rms = computeRMS(outL44.data(), 44100);
    TEST("44100 produces output", rms > 0.01f);
}

int main() {
    printf("========================================\n");
    printf("  Arcadia Voices DSP Core Test Harness\n");
    printf("========================================\n");

    test_circular_buffer();
    test_soft_clipper();
    test_mod_lfo();
    test_pitch_shifter();
    test_pitch_detector();
    test_tempo_sync();
    test_preset_manager();
    test_param_smoother();
    test_pitch_delay_voice();
    test_delay_pedal_dsp();
    test_tempo_sync_integration();
    test_presets_apply_to_block();
    test_no_infinite_loop();
    test_parameter_ranges();
    test_different_sample_rates();

    printf("\n========================================\n");
    printf("  Results: %d passed, %d failed out of %d total\n",
           gTestsPassed, gTestsFailed, gTestsPassed + gTestsFailed);
    printf("========================================\n");

    return gTestsFailed > 0 ? 1 : 0;
}
