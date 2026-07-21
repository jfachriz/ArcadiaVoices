import React from 'react';

interface PanelProps {
  children: React.ReactNode;
  className?: string;
}

export default function Panel({ children, className = "" }: PanelProps) {
  return (
    <div
      className={`bg-[#12141c] rounded-[16px] border border-white/10 relative overflow-hidden ${className}`}
      style={{
        boxShadow: '0 4px 12px rgba(0,0,0,0.6), inset 0 2px 4px rgba(255,255,255,0.05), inset 0 -4px 10px rgba(0,0,0,0.7)'
      }}
    >
      {children}
    </div>
  );
}
