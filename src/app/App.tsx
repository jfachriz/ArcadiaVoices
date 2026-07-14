import React, { useState, useRef, useEffect } from 'react';
import LeftPanel from './components/pedal/LeftPanel';
import CenterPanel from './components/pedal/CenterPanel';
import RightPanel from './components/pedal/RightPanel';
import { NOTE_DIVISIONS, type KnobValues, type SavedPreset } from './components/pedal/types';
import useBridge from './useBridge';
import { subscribeToMessageId, sendArbitraryMsg } from './iplugBridge';

// Convert real DSP units back to UI knob 0-100 space
function msToKnob(ms: number): number {
  if (ms <= 0) return 0;
  // timeToMs: Math.round(5 * Math.pow(10, 2 * knob / 100))
  // reverse: knob = 50 * log10(ms / 5)
  const knob = 50 * Math.log10(ms / 5);
  return Math.round(Math.max(0, Math.min(100, knob)));
}
function pctToKnob(pct: number, max: number): number {
  return Math.round(Math.max(0, Math.min(100, (pct / max) * 100)));
}
function semitonesToKnob(st: number): number {
  return Math.round(Math.max(0, Math.min(100, ((st + 24) / 48) * 100)));
}

const FACTORY_PRESETS: SavedPreset[] = [
  { id: 1, name: "multi voice", knobs: { time: 50, feedback: 30, mix: 70, mod: 20, left: 45, right: 55 }, powerOn: true, syncMode: false, noteDivisionIndex: 4 },
  { id: 2, name: "chorus room", knobs: { time: 20, feedback: 40, mix: 50, mod: 80, left: 50, right: 50 }, powerOn: true, syncMode: false, noteDivisionIndex: 4 },
  { id: 3, name: "deep space", knobs: { time: 90, feedback: 80, mix: 60, mod: 40, left: 30, right: 70 }, powerOn: true, syncMode: false, noteDivisionIndex: 4 },
];

export default function App() {
  const [knobs, setKnobs] = useState<KnobValues>({
    power: true, time: 50, feedback: 30, mix: 70, mod: 20, left: 45, right: 55, preset: 1
  });

  const [syncMode, setSyncMode] = useState(false);
  const [noteDivisionIndex, setNoteDivisionIndex] = useState(4);
  const [activeKnob, setActiveKnob] = useState<string | null>(null);
  const [isPresetMenuOpen, setIsPresetMenuOpen] = useState(false);
  const [savedPresets, setSavedPresets] = useState<SavedPreset[]>(FACTORY_PRESETS);
  const syncKnobPosRef = useRef(0);
  const { sendKnobValue } = useBridge();

  const handleKnobChange = (key: keyof KnobValues, value: number | boolean) => {
    if (key === 'time' && syncMode) {
      const divisionStep = 100 / (NOTE_DIVISIONS.length - 1);
      const snappedIndex = Math.round((value as number) / divisionStep);
      const clampedIndex = Math.max(0, Math.min(NOTE_DIVISIONS.length - 1, snappedIndex));
      syncKnobPosRef.current = clampedIndex * divisionStep;
      setNoteDivisionIndex(clampedIndex);
      sendKnobValue('noteDivision', clampedIndex);
      setActiveKnob('time');
      return;
    }
    setKnobs(prev => ({ ...prev, [key]: value }));
    if (key !== 'power' && key !== 'preset') setActiveKnob(key);
    // Send value to DSP via bridge (skip 'preset' — it's UI-only)
    if (key !== 'preset') {
      sendKnobValue(key as any, value);
    }
  };

  const handleToggleSync = () => {
    const newSyncMode = !syncMode;
    setSyncMode(newSyncMode);
    sendKnobValue('syncMode', newSyncMode);
  };

  const handleNoteDivisionChange = (index: number) => {
    setNoteDivisionIndex(index);
    sendKnobValue('noteDivision', index);
  };

  const loadPreset = (preset: SavedPreset) => {
    setKnobs(prev => ({ ...prev, ...preset.knobs, preset: preset.id, power: preset.powerOn }));
    setSyncMode(preset.syncMode);
    setNoteDivisionIndex(preset.noteDivisionIndex);
    // Send all preset knob values to DSP
    const kv = preset.knobs;
    sendKnobValue('time', kv.time);
    sendKnobValue('feedback', kv.feedback);
    sendKnobValue('mix', kv.mix);
    sendKnobValue('mod', kv.mod);
    sendKnobValue('left', kv.left);
    sendKnobValue('right', kv.right);
    sendKnobValue('power', preset.powerOn);
    sendKnobValue('syncMode', preset.syncMode);
    sendKnobValue('noteDivision', preset.noteDivisionIndex);
    // Notify C++ of the active preset id for serialization
    sendArbitraryMsg(0, JSON.stringify({ id: "loadPreset", presetId: preset.id }));
    setIsPresetMenuOpen(false);
  };

  const saveCurrentPreset = () => {
    const newId = savedPresets.length + 1;
    const presetName = "user preset " + newId;
    const powerVal = knobs.power as boolean;
    setSavedPresets([...savedPresets, {
      id: newId,
      name: presetName,
      knobs: {
        time: knobs.time as number,
        feedback: knobs.feedback as number,
        mix: knobs.mix as number,
        mod: knobs.mod as number,
        left: knobs.left as number,
        right: knobs.right as number,
      },
      powerOn: powerVal,
      syncMode,
      noteDivisionIndex,
    }]);
    // Notify C++ of the new preset (sends knob 0-100 values, C++ converts to real units)
    sendArbitraryMsg(0, JSON.stringify({
      id: "savePreset",
      name: presetName,
      knobs: {
        time: knobs.time,
        feedback: knobs.feedback,
        mix: knobs.mix,
        mod: knobs.mod,
        left: knobs.left,
        right: knobs.right,
        powerOn: powerVal,
        syncMode,
        noteDivision: noteDivisionIndex,
      }
    }));
    handleKnobChange('preset', newId);
    setIsPresetMenuOpen(false);
  };

  // Receive parameter values from C++ (rehydrates UI after WebView reload,
  // so the on-screen knob positions match the actual DSP state instead of
  // showing the hardcoded useState defaults until the first user interaction).
  useEffect(() => {
    const unsub = subscribeToMessageId('paramValues', (_tag, data) => {
      try {
        const msg = JSON.parse(data);
        // Convert normalized 0-1 values back to UI knob 0-100 space
        const getKnob = (idx: number): number =>
          Math.round(Math.max(0, Math.min(100, (msg[String(idx)] ?? 0.5) * 100)));

        setKnobs(prev => ({
          ...prev,
          time: getKnob(0),
          feedback: getKnob(1),
          mix: getKnob(2),
          mod: getKnob(3),
          left: getKnob(4),
          right: getKnob(5),
          power: (msg["6"] ?? 1) > 0.5,
        }));
        setSyncMode((msg["7"] ?? 0) > 0.5);
        setNoteDivisionIndex(Math.round((msg["8"] ?? 0.3077) * 13));
      } catch {}
    });
    return unsub;
  }, []);

  // Receive preset list from C++ (repopulates after WebView reload / DAW project load)
  useEffect(() => {
    const unsub = subscribeToMessageId('presetList', (_tag, data) => {
      try {
        const msg = JSON.parse(data);
        if (msg.presets && Array.isArray(msg.presets)) {
          const presets: SavedPreset[] = msg.presets.map((p: any) => ({
            id: p.id,
            name: p.name,
            knobs: {
              time: msToKnob(p.time ?? 350),
              feedback: pctToKnob(p.feedback ?? 35, 110),
              mix: pctToKnob(p.mix ?? 50, 100),
              mod: pctToKnob(p.mod ?? 20, 100),
              left: semitonesToKnob(p.pitchL ?? -2),
              right: semitonesToKnob(p.pitchR ?? 1),
            },
            powerOn: p.powerOn ?? true,
            syncMode: p.syncMode ?? false,
            noteDivisionIndex: p.noteDivision ?? 4,
          }));
          setSavedPresets(presets);
          if (msg.activeId) {
            setKnobs(prev => ({ ...prev, preset: msg.activeId }));
          }
        }
      } catch {}
    });
    return unsub;
  }, []);

  const handleRenamePreset = (id: number, newName: string) => {
    setSavedPresets(prev => prev.map(p => p.id === id ? { ...p, name: newName } : p));
    sendArbitraryMsg(0, JSON.stringify({ id: "renamePreset", presetId: id, name: newName }));
  };

  const handleDeletePreset = (id: number) => {
    if (savedPresets.length <= 1) return; // never delete last preset
    setSavedPresets(prev => {
      const next = prev.filter(p => p.id !== id);
      return next.length > 0 ? next : prev;
    });
    sendArbitraryMsg(0, JSON.stringify({ id: "deletePreset", presetId: id }));
  };

  useEffect(() => {
    if (activeKnob) {
      const timer = setTimeout(() => setActiveKnob(null), 1000);
      return () => clearTimeout(timer);
    }
  }, [activeKnob, knobs]);

  return (
    <div className="min-h-screen bg-[#f3f4f6] flex items-center justify-center p-8 font-sans">
      <div
        className="relative w-full max-w-[1000px] aspect-[4/3] sm:aspect-[1.4] bg-[#d3d5d7] rounded-[32px] p-6 shadow-2xl overflow-hidden select-none"
        style={{
          boxShadow: '0 30px 60px rgba(0,0,0,0.15), inset 0 2px 4px rgba(255,255,255,0.7), inset 0 -6px 12px rgba(0,0,0,0.1)'
        }}
      >
        <div className="w-full h-full flex flex-col gap-4">
          <div className="flex-1 flex gap-4">
            <LeftPanel
              syncMode={syncMode}
              onToggleSync={handleToggleSync}
              timeValue={knobs.time as number}
              noteDivisionIndex={noteDivisionIndex}
              onTimeChange={(v) => handleKnobChange('time', v)}
              feedbackValue={knobs.feedback as number}
              onFeedbackChange={(v) => handleKnobChange('feedback', v)}
            />

            <CenterPanel
              powerOn={knobs.power as boolean}
              onPowerToggle={() => handleKnobChange('power', !knobs.power)}
              activeKnob={activeKnob}
              isPresetMenuOpen={isPresetMenuOpen}
              savedPresets={savedPresets}
              activePresetId={knobs.preset as number}
              knobValues={knobs}
              syncMode={syncMode}
              noteDivisionIndex={noteDivisionIndex}
              pitchLValue={knobs.left as number}
              pitchRValue={knobs.right as number}
              onOpenPresetMenu={() => setIsPresetMenuOpen(true)}
              onLoadPreset={loadPreset}
              onSaveCurrentPreset={saveCurrentPreset}
              onCancelPresetMenu={() => setIsPresetMenuOpen(false)}
              onRenamePreset={handleRenamePreset}
              onDeletePreset={handleDeletePreset}
              onPitchLChange={(v) => handleKnobChange('left', v)}
              onPitchRChange={(v) => handleKnobChange('right', v)}
            />

            <RightPanel
              mixValue={knobs.mix as number}
              onMixChange={(v) => handleKnobChange('mix', v)}
              modValue={knobs.mod as number}
              onModChange={(v) => handleKnobChange('mod', v)}
            />
          </div>
        </div>
      </div>
    </div>
  );
}
