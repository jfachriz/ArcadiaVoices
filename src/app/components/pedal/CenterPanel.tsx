import OLEDScreen from './OLEDScreen';
import PowerLED from './PowerLED';
import Knob from './Knob';
import OnOffButton from './OnOffButton';
import { type SavedPreset, type KnobValues } from './types';

interface CenterPanelProps {
  powerOn: boolean;
  onPowerToggle: () => void;
  activeKnob: string | null;
  isPresetMenuOpen: boolean;
  savedPresets: SavedPreset[];
  activePresetId: number;
  knobValues: KnobValues;
  syncMode: boolean;
  noteDivisionIndex: number;
  pitchLValue: number;
  pitchRValue: number;
  onOpenPresetMenu: () => void;
  onLoadPreset: (preset: SavedPreset) => void;
  onSaveCurrentPreset: () => void;
  onCancelPresetMenu: () => void;
  onRenamePreset: (id: number, newName: string) => void;
  onDeletePreset: (id: number) => void;
  onPitchLChange: (v: number) => void;
  onPitchRChange: (v: number) => void;
}

export default function CenterPanel({
  powerOn, onPowerToggle, activeKnob,
  isPresetMenuOpen, savedPresets, activePresetId,
  knobValues, syncMode, noteDivisionIndex,
  pitchLValue, pitchRValue,
  onOpenPresetMenu, onLoadPreset, onSaveCurrentPreset, onCancelPresetMenu,
  onRenamePreset, onDeletePreset,
  onPitchLChange, onPitchRChange
}: CenterPanelProps) {
  return (
    <div className="flex-1 flex flex-col gap-0 rounded-[16px] overflow-hidden bg-[#12141c] border border-white/10 shadow-lg relative">
      <OLEDScreen
        powerOn={powerOn}
        activeKnob={activeKnob}
        isPresetMenuOpen={isPresetMenuOpen}
        savedPresets={savedPresets}
        activePresetId={activePresetId}
        knobValues={knobValues}
        syncMode={syncMode}
        noteDivisionIndex={noteDivisionIndex}
        pitchLValue={pitchLValue}
        pitchRValue={pitchRValue}
        onOpenPresetMenu={onOpenPresetMenu}
        onLoadPreset={onLoadPreset}
        onSaveCurrentPreset={onSaveCurrentPreset}
        onCancelPresetMenu={onCancelPresetMenu}
        onRenamePreset={onRenamePreset}
        onDeletePreset={onDeletePreset}
      />

      <div className="h-[45%] flex items-center justify-center gap-12 rounded-b-[16px] relative bg-[#0c0d12] shadow-[inset_0_4px_10px_rgba(0,0,0,0.6)]">
        <PowerLED powerOn={powerOn} />
        <Knob type="black" label="left" size="sm" value={pitchLValue} onChange={onPitchLChange} />
        <OnOffButton
          type="black"
          label="power"
          size="lg"
          isActive={powerOn}
          onToggle={onPowerToggle}
          hasBrackets
        />
        <Knob type="black" label="right" size="sm" value={pitchRValue} onChange={onPitchRChange} />
      </div>
    </div>
  );
}
