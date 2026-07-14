#include "ArcadiaVoices.h"
#include "IPlug_include_in_plug_src.h"
#include "json.hpp"

#include <algorithm>
#include <cmath>

ArcadiaVoices::ArcadiaVoices(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  // ---- Parameter registration (matches GUIDELINE Section 2) ----
  // time: 1ms–2000ms, default 350ms, log curve. UI knob 0-100 maps in WebView.
  GetParam(kParamTime)->InitDouble("Time", 350.0, 1.0, 2000.0, 0.01, "ms");

  // feedback: 0–110%, default 35%
  GetParam(kParamFeedback)->InitDouble("Feedback", 35.0, 0.0, 110.0, 0.1, "%");

  // mix: 0–100%, default 50%
  GetParam(kParamMix)->InitDouble("Mix", 50.0, 0.0, 100.0, 0.1, "%");

  // mod: 0–100%, default 20%
  GetParam(kParamMod)->InitDouble("Mod", 20.0, 0.0, 100.0, 0.1, "%");

  // pitchL: -24–+24 semitones, default -2, stepped to integer semitone
  GetParam(kParamPitchL)->InitInt("PitchL", -2, -24, 24, "st");

  // pitchR: -24–+24 semitones, default +1, stepped to integer semitone
  GetParam(kParamPitchR)->InitInt("PitchR", 1, -24, 24, "st");

  // power: ON/OFF — bypass
  GetParam(kParamPower)->InitBool("Power", true);

  // syncMode: Free/Synced
  GetParam(kParamSyncMode)->InitBool("SyncMode", false);

  // noteDivision: 0–13 matching the division list
  GetParam(kParamNoteDivision)->InitInt("NoteDivision", 4, 0, 13, "");

  // ---- WebView UI setup (matches ArcadiaTS9 pattern) ----
#ifdef WEBVIEW_EDITOR_DELEGATE
#ifdef DEBUG
  SetEnableDevTools(true);
#endif

  mEditorInitFunc = [&]() {
    LoadIndexHtml(__FILE__, GetBundleID());
    EnableScroll(false);
  };
#endif
}

#if IPLUG_DSP
void ArcadiaVoices::OnReset()
{
  const double sr = GetSampleRate();
  mDSP.Init(static_cast<float>(sr));
}

void ArcadiaVoices::OnIdle()
{
#ifdef WEBVIEW_EDITOR_DELEGATE
  // Throttle: ~10Hz (every ~20 idle calls at ~200Hz)
  static int idleCounter = 0;
  if (++idleCounter % 20 != 0) return;

  // === Always-sent messages (independent of power state) ===

  // Send all current parameter values (so UI can rehydrate after WebView
  // reload — without this the UI renders hardcoded defaults until the user
  // touches a knob, even though the DSP has the correct values).
  {
    nlohmann::json paramMsg;
    paramMsg["id"] = "paramValues";
    for (int i = 0; i < kNumParams; i++) {
      paramMsg[std::to_string(i)] = GetParam(i)->GetNormalized();
    }
    SendJSONFromDelegate(paramMsg);
  }

  // Send power state (needed for LED even when DSP is bypassed)
  {
    nlohmann::json msg;
    msg["id"] = "power";
    msg["on"] = mDSP.IsPowerOn();
    SendJSONFromDelegate(msg);
  }

  // Send full preset list + active id (needed so UI can repopulate
  // after WebView reload without losing user-created presets)
  {
    nlohmann::json presetMsg;
    presetMsg["id"] = "presetList";
    presetMsg["msgTag"] = static_cast<int>(IPlugMsgTag::PresetList);
    presetMsg["activeId"] = mDSP.GetPresetManager().ActivePresetId();

    nlohmann::json presets = nlohmann::json::array();
    const int count = mDSP.GetPresetManager().Count();
    for (int i = 0; i < count; i++) {
      const PresetSnapshot* p = mDSP.GetPresetManager().GetPresetByIndex(i);
      if (!p) continue;
      nlohmann::json pj;
      pj["id"] = p->id;
      pj["name"] = std::string(p->name);
      pj["time"] = p->timeMs;
      pj["feedback"] = p->feedbackPct;
      pj["mix"] = p->mixPct;
      pj["mod"] = p->modPct;
      pj["pitchL"] = p->pitchL;
      pj["pitchR"] = p->pitchR;
      pj["powerOn"] = p->powerOn;
      pj["syncMode"] = p->syncMode;
      pj["noteDivision"] = p->noteDivision;
      presets.push_back(pj);
    }
    presetMsg["presets"] = presets;
    SendJSONFromDelegate(presetMsg);
  }

  if (mDSP.IsPowerOn()) {
    // Send pitch readout for each channel
    for (int ch = 0; ch < 2; ch++) {
      const PitchDetector& detector = mDSP.Detector(ch);
      nlohmann::json pitchMsg;
      pitchMsg["id"] = "pitch";
      pitchMsg["channel"] = ch;
      pitchMsg["note"] = detector.DetectedNote();
      pitchMsg["confidence"] = detector.Confidence();
      SendJSONFromDelegate(pitchMsg);
    }
  }
#endif
}

void ArcadiaVoices::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  // Sync iPlug2 parameters → DSP parameters
  mDSP.SetPowerOn(GetParam(kParamPower)->Bool());
  mDSP.SetDelayTimeMs(static_cast<float>(GetParam(kParamTime)->Value()));
  mDSP.SetFeedbackPct(static_cast<float>(GetParam(kParamFeedback)->Value()));
  mDSP.SetMixPct(static_cast<float>(GetParam(kParamMix)->Value()));
  mDSP.SetModDepth(static_cast<float>(GetParam(kParamMod)->Value()));
  mDSP.SetPitchLSemitones(static_cast<int>(GetParam(kParamPitchL)->Value()));
  mDSP.SetPitchRSemitones(static_cast<int>(GetParam(kParamPitchR)->Value()));

  const bool syncMode = GetParam(kParamSyncMode)->Bool();
  if (syncMode) {
    mDSP.SetHostBPM(GetTempo());
    mDSP.SetNoteDivision(static_cast<int>(GetParam(kParamNoteDivision)->Value()));
  }
  mDSP.SetSyncMode(syncMode ? SyncMode::Synced : SyncMode::Free);

  // Let the DSP process the block
  mDSP.ProcessBlock(inputs, outputs, nFrames);
}
#endif

// ---- Preset conversion helpers ----
// UI sends knob 0-100 values; DSP stores real units. iPlug2 parameter system
// uses normalized 0-1 internally for all GetParam()->Set() calls.

// Convert knob 0-100 → ms (matches iPlug2 InitDouble linear range 1-2000ms)
static double KnobToTimeMs(double knob) {
  const double t = std::max(0.0, std::min(1.0, knob / 100.0));
  return 1.0 + t * (2000.0 - 1.0);
}
// Convert knob 0-100 → percent 0-110
static double KnobToFeedbackPct(double knob) {
  return (std::max(0.0, std::min(100.0, knob)) / 100.0) * 110.0;
}
// Convert knob 0-100 → percent 0-100
static double KnobToMixPct(double knob) {
  return std::max(0.0, std::min(100.0, knob));
}
static double KnobToModPct(double knob) {
  return std::max(0.0, std::min(100.0, knob));
}
// Convert knob 0-100 → semitones -24..+24
static double KnobToSemitones(double knob) {
  const double t = std::max(0.0, std::min(1.0, knob / 100.0));
  return -24.0 + t * 48.0;
}

// ---- UI ← DSP: Send JSON message (wraps SendArbitraryMsgFromDelegate with base64) ----
void ArcadiaVoices::SendJSONFromDelegate(const nlohmann::json& jsonMsg)
{
  const std::string jsonStr = jsonMsg.dump();
  SendArbitraryMsgFromDelegate(-1, static_cast<int>(jsonStr.size()), jsonStr.c_str());
}

// ---- UI → DSP: Handle incoming messages from the WebView ----
bool ArcadiaVoices::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  if (dataSize <= 0 || !pData) return false;

  // Messages from the WebView arrive as JSON via SAMFUI
  // The WebViewEditorDelegate already parses SAMFUI and calls OnMessage
  // with the decoded data. We receive it as a blob; try to parse as JSON.
  try {
    const char* dataStr = static_cast<const char*>(pData);
    const auto json = nlohmann::json::parse(dataStr, dataStr + dataSize, nullptr, false);
    if (json.is_discarded()) return false;

    // Handle parameter changes from UI (knob drags, power toggle, preset select)
    if (json.contains("id")) {
      const std::string id = json["id"];

      if (id == "paramChange" && json.contains("paramIdx") && json.contains("value")) {
        const int paramIdx = json["paramIdx"];
        const double value = json["value"];
        if (paramIdx >= 0 && paramIdx < kNumParams) {
          GetParam(paramIdx)->Set(value);
          return true;
        }
      }

      // Handle "savePreset" — user saved a new preset from the UI
      // Receives knob 0-100 values; converts to real DSP units for storage.
      if (id == "savePreset" && json.contains("name") && json.contains("knobs")) {
        const auto& knobs = json["knobs"];
        PresetSnapshot snapshot;
        std::strncpy(snapshot.name, json["name"].get<std::string>().c_str(), sizeof(snapshot.name) - 1);
        snapshot.name[sizeof(snapshot.name) - 1] = '\0';
        // Convert 0-100 knob values → real DSP units
        snapshot.timeMs = KnobToTimeMs(knobs.value("time", 50.0));
        snapshot.feedbackPct = KnobToFeedbackPct(knobs.value("feedback", 30.0));
        snapshot.mixPct = KnobToMixPct(knobs.value("mix", 70.0));
        snapshot.modPct = KnobToModPct(knobs.value("mod", 20.0));
        snapshot.pitchL = KnobToSemitones(knobs.value("left", 45.0));
        snapshot.pitchR = KnobToSemitones(knobs.value("right", 55.0));
        snapshot.powerOn = knobs.value("powerOn", true);
        snapshot.syncMode = knobs.value("syncMode", false);
        snapshot.noteDivision = static_cast<int>(knobs.value("noteDivision", 4.0));
        const int newId = mDSP.GetPresetManager().SaveAsNew(snapshot.name, snapshot);
        // Respond with updated preset list
        nlohmann::json response;
        response["id"] = "presetSaved";
        response["newId"] = newId;
        SendJSONFromDelegate(response);
        return true;
      }

      // Handle "loadPreset" — user selected a preset from the UI
      // Individual knob values are sent as SPVFUI by the JS side separately;
      // this just updates the PresetManager's active preset ID for serialization.
      if (id == "loadPreset" && json.contains("presetId")) {
        const int presetId = json["presetId"];
        mDSP.GetPresetManager().LoadPreset(presetId);
        return true;
      }

      // Handle "renamePreset" — user renamed a preset from the UI
      if (id == "renamePreset" && json.contains("presetId") && json.contains("name")) {
        const int presetId = json["presetId"];
        const std::string newName = json["name"];
        if (mDSP.GetPresetManager().RenamePreset(presetId, newName.c_str())) {
          return true;
        }
      }

      // Handle "deletePreset" — user deleted a preset from the UI
      if (id == "deletePreset" && json.contains("presetId")) {
        const int presetId = json["presetId"];
        if (mDSP.GetPresetManager().DeletePreset(presetId)) {
          return true;
        }
      }
    }

    // Check for SPVFUI-style messages sent via IPlugSendMsg from the WebView
    if (json.contains("msg") && json["msg"] == "SPVFUI") {
      const int paramIdx = json["paramIdx"];
      const double value = json["value"];
      if (paramIdx >= 0 && paramIdx < kNumParams) {
        SendParameterValueFromUI(paramIdx, value);
        return true;
      }
    }
  } catch (...) {
    // Not JSON — ignore
  }

  return false;
}

// ---- State serialization (preserves user-created presets across DAW sessions) ----
// NOTE: Must call Plugin::SerializeState first to persist standard registered
// parameters (time, feedback, mix, mod, pitchL, pitchR, power, syncMode,
// noteDivision). Overriding without the base call silently drops them.
bool ArcadiaVoices::SerializeState(IByteChunk& chunk) const
{
  // 1. Base class serializes all registered parameters
  if (!Plugin::SerializeState(chunk)) return false;

  // 2. Custom data: preset list + active preset id
  const PresetManager& pm = mDSP.GetPresetManager();
  const int numPresets = pm.Count();
  const int activeId = pm.ActivePresetId();

  // Write header: preset count + active preset id
  chunk.Put(&numPresets);
  chunk.Put(&activeId);

  // Write each preset
  for (int i = 0; i < numPresets; i++) {
    const PresetSnapshot* preset = pm.GetPresetByIndex(i);
    if (!preset) continue;

    const int id = preset->id;
    chunk.Put(&id);
    chunk.PutStr(preset->name);
    chunk.Put(&preset->timeMs);
    chunk.Put(&preset->feedbackPct);
    chunk.Put(&preset->mixPct);
    chunk.Put(&preset->modPct);
    chunk.Put(&preset->pitchL);
    chunk.Put(&preset->pitchR);
    chunk.Put(&preset->powerOn);
    chunk.Put(&preset->syncMode);
    chunk.Put(&preset->noteDivision);
  }

  return true;
}

// NOTE: Must call Plugin::UnserializeState first to restore standard registered
// parameters before reading custom preset data.
int ArcadiaVoices::UnserializeState(const IByteChunk& chunk, int startPos)
{
  // 1. Base class restores all registered parameters
  int pos = Plugin::UnserializeState(chunk, startPos);
  if (pos < 0) return startPos; // fallback: don't corrupt if format mismatch

  // 2. Custom data: preset list + active preset id
  int numPresets = 0;
  int activeId = 1;

  // Read header
  chunk.Get(&numPresets, pos);
  pos += sizeof(int);
  chunk.Get(&activeId, pos);
  pos += sizeof(int);

  // Clear and restore ALL presets from the chunk (not just extras — the
  // saved chunk includes all presets including modified factory ones, so
  // we must NOT call Init() which would recreate factory defaults and
  // discard user modifications to factory presets).
  mDSP.GetPresetManager().Clear();

  for (int i = 0; i < numPresets; i++) {
    PresetSnapshot preset;
    int id = 0;
    chunk.Get(&id, pos); pos += sizeof(int);
    WDL_String nameStr;
    pos = chunk.GetStr(nameStr, pos);
    std::strncpy(preset.name, nameStr.Get(), sizeof(preset.name) - 1);
    preset.name[sizeof(preset.name) - 1] = '\0';
    chunk.Get(&preset.timeMs, pos); pos += sizeof(float);
    chunk.Get(&preset.feedbackPct, pos); pos += sizeof(float);
    chunk.Get(&preset.mixPct, pos); pos += sizeof(float);
    chunk.Get(&preset.modPct, pos); pos += sizeof(float);
    chunk.Get(&preset.pitchL, pos); pos += sizeof(float);
    chunk.Get(&preset.pitchR, pos); pos += sizeof(float);
    chunk.Get(&preset.powerOn, pos); pos += sizeof(bool);
    chunk.Get(&preset.syncMode, pos); pos += sizeof(bool);
    chunk.Get(&preset.noteDivision, pos); pos += sizeof(int);
    preset.id = id;
    mDSP.GetPresetManager().RestorePreset(preset);
  }

  // Update ActivePresetId so serialization and the UI know which preset
  // is currently active. Do NOT overwrite iPlug2 parameter values with
  // preset snapshot data — Plugin::UnserializeState() at the top already
  // restored the correct saved parameter values. Overwriting them here
  // would discard changes made after the preset was saved (e.g. switching
  // to sync mode after selecting a preset). ProcessBlock reads from
  // GetParam() on every callback and will sync the DSP's internal cached
  // values from the correctly-restored iPlug2 parameters automatically.
  mDSP.GetPresetManager().LoadPreset(activeId);
  if (const PresetSnapshot* preset = mDSP.GetPresetManager().GetPreset(activeId)) {
    mDSP.ApplyPreset(*preset);
  }

  return pos;
}
