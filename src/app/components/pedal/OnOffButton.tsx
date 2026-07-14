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
          className={`absolute inset-0 rounded-full border-[1.5px] transition-all duration-100 ${
            type === 'white'
              ? 'bg-gradient-to-br from-white to-[#e0e0e0] border-[#c0c0c0]'
              : 'bg-gradient-to-b from-[#333] via-[#111] to-[#000] border-[#222]'
          }`}
          style={{
            boxShadow: type === 'white'
              ? (isActive ? 'inset 0 4px 8px rgba(0,0,0,0.2)' : 'inset 0 2px 4px rgba(255,255,255,0.8), inset 0 -4px 6px rgba(0,0,0,0.1)')
              : (isActive ? 'inset 0 6px 12px rgba(0,0,0,0.6)' : 'inset 0 4px 6px rgba(255,255,255,0.15), inset 0 -4px 6px rgba(0,0,0,0.4)')
          }}
        >
        </div>
      </div>

      <span className={`${labelSizeMap[size]} text-gray-700 tracking-wide font-medium`}>{label}</span>
    </div>
  );
}
