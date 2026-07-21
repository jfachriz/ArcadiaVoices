import { motion, AnimatePresence } from 'motion/react';
import { useState } from 'react';
import { NOTE_DIVISIONS, timeToMs, knobToSemitone, type SavedPreset, type KnobValues } from './types';
import logoImg from '../../../assets/logo.png';

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
    <div className="h-[55%] text-white flex flex-col items-center justify-center relative shadow-[inset_0_-10px_20px_rgba(0,0,0,0.5)] bg-[#000000] overflow-hidden rounded-t-[16px]">
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
            {/* Logo image */}
            <img src={logoImg} alt="Logo" className="absolute top-2 h-14 object-contain filter brightness-110 opacity-90" />

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
          </motion.div>
        )}
      </AnimatePresence>

      {/* Preset Menu Modal — rendered directly over the entire panel when open */}
      <AnimatePresence>
        {isPresetMenuOpen && (
          <PresetMenuOverlay
            presets={savedPresets}
            activeId={activePresetId}
            onLoad={onLoadPreset}
            onSave={onSaveCurrentPreset}
            onCancel={onCancelPresetMenu}
            onRename={onRenamePreset}
            onDelete={onDeletePreset}
          />
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

export function PresetMenuOverlay({
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
    <motion.div
      initial={{ opacity: 0, scale: 0.96 }}
      animate={{ opacity: 1, scale: 1 }}
      exit={{ opacity: 0, scale: 0.96 }}
      className="absolute inset-0 z-50 bg-[#090b10]/95 backdrop-blur-md flex flex-col items-center justify-center p-6 rounded-t-[16px] rounded-b-none border border-white/20 shadow-2xl"
    >
      <h3 className="text-white/80 text-sm mb-5 tracking-widest uppercase font-semibold">Select Preset</h3>
      <div className="flex flex-wrap justify-center gap-4 mb-6 max-w-[90%]">
        {presets.map(p => (
          <div key={p.id} className="relative group flex flex-col items-center">
            {renamingId === p.id ? (
              <input
                autoFocus
                value={renameInput}
                onChange={e => setRenameInput(e.target.value)}
                onKeyDown={e => { if (e.key === 'Enter') commitRename(); if (e.key === 'Escape') setRenamingId(null); }}
                onBlur={commitRename}
                className="w-28 px-2 py-1.5 text-xs rounded-lg bg-white/10 text-white border border-white/40 outline-none text-center"
              />
            ) : (
              <>
                <button
                  onClick={() => onLoad(p)}
                  className={`w-14 h-14 sm:w-16 sm:h-16 rounded-xl flex flex-col items-center justify-center text-2xl sm:text-3xl font-light transition-all ${
                    activeId === p.id
                      ? 'bg-white text-black shadow-[0_0_20px_rgba(255,255,255,0.6)] scale-105'
                      : 'bg-white/10 text-white/80 border border-white/20 hover:bg-white/25'
                  }`}
                >
                  {p.id.toString().padStart(2, '0')}
                </button>
                {/* Hover actions: rename + delete */}
                <div className="absolute -top-2 hidden group-hover:flex gap-1.5 z-10">
                  <button
                    onClick={() => startRename(p)}
                    className="w-5 h-5 rounded-full bg-white/30 text-white text-[9px] flex items-center justify-center hover:bg-white/50 transition-colors shadow"
                    title="Rename"
                  >✎</button>
                  <button
                    onClick={() => onDelete(p.id)}
                    className="w-5 h-5 rounded-full bg-white/30 text-white text-[9px] flex items-center justify-center hover:bg-red-500 hover:text-white transition-colors shadow"
                    title="Delete"
                    style={{ display: presets.length <= 1 ? 'none' : undefined }}
                  >✕</button>
                </div>
                <span className="text-[10px] text-white/60 mt-1.5 truncate max-w-20 text-center leading-tight font-medium">{p.name}</span>
              </>
            )}
          </div>
        ))}
      </div>
      <div className="flex gap-4">
        <button
          onClick={onSave}
          className="text-xs tracking-wider uppercase border border-white/40 px-5 py-2 rounded-full text-white hover:bg-white hover:text-black font-semibold transition-all shadow-md"
        >
          Save as New
        </button>
        <button
          onClick={onCancel}
          className="text-xs tracking-wider uppercase border border-white/40 px-5 py-2 rounded-full text-white/80 hover:bg-white/20 font-semibold transition-all"
        >
          Cancel
        </button>
      </div>
    </motion.div>
  );
}
