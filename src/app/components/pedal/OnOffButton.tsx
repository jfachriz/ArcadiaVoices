import React from 'react';

interface OnOffButtonProps {
  type: "white" | "black";
  label: string;
  size?: "sm" | "md" | "lg";
  hasBrackets?: boolean;
  isActive: boolean;
  onToggle: () => void;
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

export default function OnOffButton({
  type, label, size = "md", hasBrackets = false, isActive, onToggle
}: OnOffButtonProps) {
  return (
    <div className="flex flex-col items-center gap-4 relative">
      {hasBrackets && (
        <>
          <div className="absolute left-[-20px] top-1/2 -translate-y-[20px] w-[2px] h-[40px] bg-gray-300 rounded-full opacity-50" style={{ transform: 'translateY(-50%) scaleX(0.5) rotate(15deg)'}}></div>
          <div className="absolute right-[-20px] top-1/2 -translate-y-[20px] w-[2px] h-[40px] bg-gray-300 rounded-full opacity-50" style={{ transform: 'translateY(-50%) scaleX(0.5) rotate(-15deg)'}}></div>
        </>
      )}

      <div
        className={`relative ${dimensions[size]} rounded-full cursor-pointer transition-transform duration-100 ${isActive ? 'scale-95' : 'scale-100'}`}
        onClick={onToggle}
        style={{
          boxShadow: isActive
            ? '0 2px 4px rgba(0,0,0,0.2), 0 1px 2px rgba(0,0,0,0.1)'
            : '0 10px 20px rgba(0,0,0,0.2), 0 4px 8px rgba(0,0,0,0.1)',
        }}
      >
        <div
          className="absolute inset-0 rounded-full border-[1.5px] border-white/20 transition-all duration-100"
          style={{
            background: isActive
              ? "radial-gradient(circle, #ccc 0%, #777 70%, #444 100%)"
              : "radial-gradient(circle, #eee 0%, #aaa 70%, #666 100%)",
            boxShadow: isActive
              ? "inset 0 4px 8px rgba(0,0,0,0.7), 0 2px 4px rgba(0,0,0,0.8)"
              : "inset 0 2px 4px rgba(255,255,255,0.8), inset 0 -4px 6px rgba(0,0,0,0.4), 0 6px 12px rgba(0,0,0,0.7)",
            transform: isActive ? "translateY(2px)" : "translateY(0)",
          }}
        >
          <div
            className="w-4/5 h-4/5 absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 rounded-full border border-black/30"
            style={{
              boxShadow: "inset 0 3px 5px rgba(255,255,255,0.5), inset 0 -3px 5px rgba(0,0,0,0.3)"
            }}
          />
        </div>
      </div>

      <span className={`${labelSizeMap[size]} text-[#94a0b5] tracking-widest uppercase font-semibold`}>{label}</span>
    </div>
  );
}
