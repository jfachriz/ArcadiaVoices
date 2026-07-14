import React from 'react';

interface PanelProps {
  children: React.ReactNode;
  className?: string;
}

export default function Panel({ children, className = "" }: PanelProps) {
  return (
    <div
      className={`bg-[#fdfdfd] rounded-[16px] border border-black/5 relative overflow-hidden ${className}`}
      style={{
        boxShadow: '0 4px 10px rgba(0,0,0,0.05), inset 0 2px 4px rgba(255,255,255,1), inset 0 -2px 6px rgba(0,0,0,0.03)'
      }}
    >
      {children}
    </div>
  );
}
