import React, { useState, useRef, useEffect } from 'react';
import { motion, AnimatePresence } from 'motion/react';
import { timeToMs, knobToSemitone } from './types';

interface KnobProps {
  type: "white" | "black";
  label: string;
  size?: "sm" | "md" | "lg";
  hasBrackets?: boolean;
  value?: number;
  min?: number;
  max?: number;
  step?: number;
  onChange?: (val: number) => void;
  isSynced?: boolean;
  noteDivisionLabel?: string;
}

const dimensions = {
  sm: "w-12 h-12",
  md: "w-20 h-20",
  lg: "w-24 h-24"
};

const labelSizeMap = {
  sm: 'text-[11px]',
  md: 'text-xs',
  lg: 'text-xs'
} as const;

export default function Knob({
  type, label, size = "md", hasBrackets = false,
  value = 50, min = 0, max = 100, step = 1, onChange,
  isSynced = false, noteDivisionLabel = ""
}: KnobProps) {
  const [isDragging, setIsDragging] = useState(false);
  const startY = useRef(0);
  const startVal = useRef(value);
  const pointerIdRef = useRef<number | null>(null);
  const knobRef = useRef<HTMLDivElement | null>(null);

  const angleRange = 270;
  const angle = -135 + ((value - min) / (max - min)) * angleRange;

  const updateValueFromPointer = (clientY: number) => {
    const deltaY = startY.current - clientY;
    const sensitivity = (max - min) / 200;

    let newVal = startVal.current + (deltaY * sensitivity);
    if (step > 0) newVal = Math.round(newVal / step) * step;
    newVal = Math.max(min, Math.min(max, newVal));

    if (onChange) onChange(newVal);
  };

  const handlePointerDown = (e: React.PointerEvent) => {
    e.preventDefault();
    setIsDragging(true);
    startY.current = e.clientY;
    startVal.current = value;
    pointerIdRef.current = e.pointerId;
    knobRef.current = e.currentTarget as HTMLDivElement;
    knobRef.current.setPointerCapture(e.pointerId);
    document.body.style.cursor = 'ns-resize';
  };

  const handlePointerMove = (e: React.PointerEvent) => {
    if (!isDragging) return;
    e.preventDefault();
    updateValueFromPointer(e.clientY);
  };

  const handlePointerUp = (e: React.PointerEvent) => {
    if (!isDragging) return;
    if (pointerIdRef.current !== null && knobRef.current) {
      knobRef.current.releasePointerCapture(pointerIdRef.current);
    }
    pointerIdRef.current = null;
    setIsDragging(false);
    document.body.style.cursor = '';
  };

  useEffect(() => {
    if (!isDragging) return;

    const handleWindowPointerMove = (e: PointerEvent) => {
      e.preventDefault();
      updateValueFromPointer(e.clientY);
    };

    const handleWindowPointerUp = () => {
      if (pointerIdRef.current !== null && knobRef.current) {
        knobRef.current.releasePointerCapture(pointerIdRef.current);
      }
      pointerIdRef.current = null;
      setIsDragging(false);
      document.body.style.cursor = '';
    };

    window.addEventListener('pointermove', handleWindowPointerMove);
    window.addEventListener('pointerup', handleWindowPointerUp);

    return () => {
      window.removeEventListener('pointermove', handleWindowPointerMove);
      window.removeEventListener('pointerup', handleWindowPointerUp);
    };
  }, [isDragging, min, max, step, onChange]);

  return (
    <div className="flex flex-col items-center gap-4 relative">
      {hasBrackets && (
        <>
          <div className="absolute left-[-20px] top-1/2 -translate-y-[20px] w-[2px] h-[40px] bg-gray-300 rounded-full opacity-50" style={{ transform: 'translateY(-50%) scaleX(0.5) rotate(15deg)'}}></div>
          <div className="absolute right-[-20px] top-1/2 -translate-y-[20px] w-[2px] h-[40px] bg-gray-300 rounded-full opacity-50" style={{ transform: 'translateY(-50%) scaleX(0.5) rotate(-15deg)'}}></div>
        </>
      )}

      <AnimatePresence>
        {isDragging && (
          <motion.div
            initial={{ opacity: 0, y: 10, scale: 0.8 }}
            animate={{ opacity: 1, y: 0, scale: 1 }}
            exit={{ opacity: 0, scale: 0.8 }}
            className="absolute -top-10 bg-black/80 text-white text-xs px-2 py-1 rounded shadow-lg z-50 pointer-events-none whitespace-nowrap"
          >
            {label === 'time' && !isSynced ? `${timeToMs(value)} ms` : label === 'time' && isSynced ? noteDivisionLabel : label === 'left' || label === 'right' ? `${knobToSemitone(value)} st` : Math.round(value)}
          </motion.div>
        )}
      </AnimatePresence>

      <div
        className={`relative ${dimensions[size]} rounded-full cursor-ns-resize`}
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
        style={{
          boxShadow: '0 10px 20px rgba(0,0,0,0.2), 0 4px 8px rgba(0,0,0,0.1)',
          touchAction: 'none'
        }}
      >
        <div
          className="absolute inset-0 rounded-full border-[1.5px] border-white/10 transition-transform duration-75 bg-gradient-to-b from-[#3a3d45] via-[#1c1e24] to-[#0d0e12]"
          style={{
            transform: `rotate(${angle}deg)`,
            boxShadow: 'inset 0 3px 5px rgba(255,255,255,0.15), inset 0 -4px 6px rgba(0,0,0,0.6), 0 4px 8px rgba(0,0,0,0.5)'
          }}
        >
          <div
            className="absolute top-0 left-1/2 -translate-x-1/2 w-[2px] h-[35%] rounded-full bg-[#f8f8ff]"
            style={{ marginTop: '8%' }}
          ></div>
        </div>
      </div>

      <span className={`${labelSizeMap[size]} text-[#94a0b5] tracking-widest uppercase font-semibold`}>{label}</span>
    </div>
  );
}
