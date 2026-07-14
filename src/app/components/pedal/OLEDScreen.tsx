import { motion, AnimatePresence } from 'motion/react';
import { useState } from 'react';
import { NOTE_DIVISIONS, timeToMs, knobToSemitone, type SavedPreset, type KnobValues } from './types';

interface OLEDScreenProps {
  powerOn: boolean;
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
}

export default function OLEDScreen({
  powerOn, activeKnob, isPresetMenuOpen, savedPresets, activePresetId,
  knobValues, syncMode, noteDivisionIndex,
  pitchLValue, pitchRValue,
  onOpenPresetMenu, onLoadPreset, onSaveCurrentPreset, onCancelPresetMenu,
  onRenamePreset, onDeletePreset
}: OLEDScreenProps) {
  return (
    <div className="h-[55%] text-white flex flex-col items-center justify-center relative shadow-[inset_0_-10px_20px_rgba(0,0,0,0.5)] bg-[#000000] overflow-hidden">
      <AnimatePresence>
        {powerOn && (
          <motion.div
            key="screen-content"
            initial={{ opacity: 0, scale: 0.95, filter: 'brightness(0)' }}
            animate={{
              opacity: 1,
              scale: activeKnob ? 1.02 : 1,
              filter: activeKnob ? 'brightness(1.2)' : 'brightness(1)'
            }}
            exit={{ opacity: 0, scale: 0.95, filter: 'brightness(0)' }}
            transition={{ type: "spring", stiffness: 300, damping: 20 }}
            className="w-full h-full flex flex-col items-center justify-center relative"
          >
            {isPresetMenuOpen ? (
              <PresetMenuOverlay
                presets={savedPresets}
                activeId={activePresetId}
                onLoad={onLoadPreset}
                onSave={onSaveCurrentPreset}
                onCancel={onCancelPresetMenu}
                onRename={onRenamePreset}
                onDelete={onDeletePreset}
              />
            ) : (
              <>
                {/* Logo icon */}
                <div className="absolute top-6 flex space-x-[2px] items-center justify-center w-8 h-8 border border-white/20 rounded-md">
                  <div className="w-[3px] h-[10px] bg-white rounded-sm"></div>
                  <div className="w-[3px] h-[14px] bg-white rounded-sm"></div>
                  <div className="w-[3px] h-[10px] bg-white rounded-sm"></div>
                </div>

                {/* Title area — shows knob value or preset name */}
                <AnimatePresence mode="wait">
                  <motion.div
                    key={activeKnob && activeKnob !== 'preset' && activeKnob !== 'power' ? 'value' : 'title'}
                    initial={{ opacity: 0, y: 10 }}
                    animate={{ opacity: 1, y: 0 }}
                    exit={{ opacity: 0, y: -10 }}
                    className="mt-8 text-xl tracking-wide font-light h-8 flex items-center"
                  >
                    {activeKnob && activeKnob !== 'preset' && activeKnob !== 'power' ? (
                      <span className="text-white">
                        {activeKnob === 'time' && !syncMode
                          ? `time: ${timeToMs(knobValues.time as number)} ms`
                          : activeKnob === 'time' && syncMode
                            ? `time: ${NOTE_DIVISIONS[noteDivisionIndex]}`
                            : activeKnob === 'left' || activeKnob === 'right'
                              ? `${activeKnob}: ${knobToSemitone(knobValues[activeKnob] as number)} st`
                              : `${activeKnob}: ${Math.round(knobValues[activeKnob] as number)}`
                        }
                      </span>
                    ) : (
                      savedPresets.find(p => p.id === activePresetId)?.name || "multi voice"
                    )}
                  </motion.div>
                </AnimatePresence>

                {/* Preset number display */}
                <motion.div
                  key={activePresetId}
                  initial={{ scale: 0.8, opacity: 0 }}
                  animate={{ scale: 1, opacity: 1 }}
                  onClick={onOpenPresetMenu}
                  className="mt-3 border-[1.5px] border-white/80 rounded-lg px-4 py-1 text-4xl font-light tracking-widest cursor-pointer hover:bg-white/10 transition-colors"
                >
                  {activePresetId.toString().padStart(2, '0')}
                </motion.div>

                {/* Left pitch indicator */}
                <PitchIndicator side="left" value={pitchLValue} />

                {/* Right pitch indicator */}
                <PitchIndicator side="right" value={pitchRValue} />
              </>
            )}
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
}

function PitchIndicator({ side, value }: { side: 'left' | 'right'; value: number }) {
  const dotCount = Math.floor((value / 100) * 60);
  const isLeft = side === 'left';

  return (
    <div className={`absolute ${isLeft ? 'left-6' : 'right-6'} top-8 bottom-8 w-16 flex flex-col ${isLeft ? '' : 'text-right'} justify-between`}>
      <div className={`flex-1 flex items-end opacity-60 ${isLeft ? '' : 'justify-end'}`}>
        <div className={`w-full flex flex-wrap gap-[2px] content-end ${isLeft ? '' : 'justify-end'}`}>
          {Array.from({ length: dotCount }).map((_, i) => (
            <motion.div
              key={`${side}-${i}`}
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              className="w-[3px] h-[3px] bg-white/80 rounded-full"
            />
          ))}
        </div>
      </div>
    </div>
  );
}

function PresetMenuOverlay({
  presets, activeId, onLoad, onSave, onCancel, onRename, onDelete
}: {
  presets: SavedPreset[];
  activeId: number;
  onLoad: (p: SavedPreset) => void;
  onSave: () => void;
  onCancel: () => void;
  onRename: (id: number, newName: string) => void;
  onDelete: (id: number) => void;
}) {
  const [renamingId, setRenamingId] = useState<number | null>(null);
  const [renameInput, setRenameInput] = useState('');

  const startRename = (p: SavedPreset) => {
    setRenamingId(p.id);
    setRenameInput(p.name);
  };

  const commitRename = () => {
    if (renamingId !== null && renameInput.trim()) {
      onRename(renamingId, renameInput.trim());
    }
    setRenamingId(null);
  };

  return (
    <div className="absolute inset-0 z-50 bg-black/95 flex flex-col items-center justify-center p-4">
      <h3 className="text-white/60 text-[10px] mb-4 tracking-widest uppercase">Select Preset</h3>
      <div className="flex flex-wrap justify-center gap-2 mb-6 max-w-[85%]">
        {presets.map(p => (
          <div key={p.id} className="relative group flex flex-col items-center">
            {renamingId === p.id ? (
              <input
                autoFocus
                value={renameInput}
                onChange={e => setRenameInput(e.target.value)}
                onKeyDown={e => { if (e.key === 'Enter') commitRename(); if (e.key === 'Escape') setRenamingId(null); }}
                onBlur={commitRename}
                className="w-24 px-2 py-1 text-xs rounded bg-white/10 text-white border border-white/30 outline-none text-center"
              />
            ) : (
              <>
                <button
                  onClick={() => onLoad(p)}
                  className={`w-10 h-10 rounded flex flex-col items-center justify-center text-lg font-light transition-all ${
                    activeId === p.id
                      ? 'bg-white text-black shadow-[0_0_15px_rgba(255,255,255,0.4)]'
                      : 'bg-white/5 text-white/70 border border-white/20 hover:bg-white/20'
                  }`}
                >
                  {p.id.toString().padStart(2, '0')}
                </button>
                {/* Hover actions: rename + delete */}
                <div className="absolute -top-3 hidden group-hover:flex gap-1">
                  <button
                    onClick={() => startRename(p)}
                    className="w-4 h-4 rounded-full bg-white/20 text-white/80 text-[7px] flex items-center justify-center hover:bg-white/40 transition-colors"
                    title="Rename"
                  >✎</button>
                  <button
                    onClick={() => onDelete(p.id)}
                    className="w-4 h-4 rounded-full bg-white/20 text-white/80 text-[7px] flex items-center justify-center hover:bg-red-400/60 hover:text-white transition-colors"
                    title="Delete"
                    style={{ display: presets.length <= 1 ? 'none' : undefined }}
                  >✕</button>
                </div>
                <span className="text-[6px] text-white/40 mt-1 truncate max-w-16 text-center leading-tight">{p.name}</span>
              </>
            )}
          </div>
        ))}
      </div>
      <div className="flex gap-4">
        <button
          onClick={onSave}
          className="text-[10px] tracking-wider uppercase border border-white/30 px-3 py-1.5 rounded-full text-white/70 hover:bg-white hover:text-black transition-colors"
        >
          Save as New
        </button>
        <button
          onClick={onCancel}
          className="text-[10px] tracking-wider uppercase border border-white/30 px-3 py-1.5 rounded-full text-white/70 hover:bg-white hover:text-black transition-colors"
        >
          Cancel
        </button>
      </div>
    </div>
  );
}
