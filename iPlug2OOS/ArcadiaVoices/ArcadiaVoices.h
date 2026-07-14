#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "dsp/DelayPedalDSP.h"

// nlohmann/json from iPlug2/Dependencies for JSON bridge messages
#include "json.hpp"

const int kNumPresets = 1;

enum EParams
{
  kParamTime = 0,
  kParamFeedback,
  kParamMix,
  kParamMod,
  kParamPitchL,
  kParamPitchR,
  kParamPower,
  kParamSyncMode,
  kParamNoteDivision,
  kNumParams
};

// Message tags for DSP → UI communication (must match IPlugMsgTag in iplugBridge.ts)
enum class IPlugMsgTag
{
  PitchReadoutL = 0,
  PitchReadoutR = 1,
  PresetList = 2,
  PowerState = 3,
};

using namespace iplug;

class ArcadiaVoices final : public Plugin
{
public:
  ArcadiaVoices(const InstanceInfo& info);

#if IPLUG_EDITOR
  bool OnHostRequestingSupportedViewConfiguration(int width, int height) override { return true; }
#endif

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;
  void OnIdle() override;
#endif

  // Handle messages from the WebView UI
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;

  // Preset state serialization
  bool SerializeState(IByteChunk& chunk) const override;
  int UnserializeState(const IByteChunk& chunk, int startPos) override;

private:
  void SendJSONFromDelegate(const nlohmann::json& jsonMsg);

  DelayPedalDSP mDSP;
};
