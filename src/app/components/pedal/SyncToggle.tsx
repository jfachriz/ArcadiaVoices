import { motion } from 'motion/react';

interface SyncToggleProps {
  isSynced: boolean;
  onToggle: () => void;
}

export default function SyncToggle({ isSynced, onToggle }: SyncToggleProps) {
  return (
    <div
      onClick={onToggle}
      className="relative w-12 h-6 rounded-full cursor-pointer flex items-center px-[2px] transition-colors duration-200"
      style={{
        backgroundColor: isSynced ? '#0a0b0e' : '#1a1c24',
        border: '1px solid rgba(255,255,255,0.1)',
        boxShadow: 'inset 0 2px 6px rgba(0,0,0,0.7)'
      }}
    >
      <motion.div
        className="w-[18px] h-[18px] rounded-full bg-gradient-to-b from-[#e0e0e0] to-[#888] flex items-center justify-center text-[7px] font-bold text-black"
        animate={{ x: isSynced ? 26 : 0 }}
        transition={{ type: 'spring', stiffness: 500, damping: 30 }}
        style={{ boxShadow: '0 2px 4px rgba(0,0,0,0.6), inset 0 1px 1px rgba(255,255,255,0.8)' }}
      >
        {isSynced ? '♪' : 'ms'}
      </motion.div>
      <span className="absolute left-[-24px] text-[8px] text-[#94a0b5] tracking-wide uppercase select-none font-semibold">free</span>
      <span className="absolute right-[-28px] text-[8px] text-[#94a0b5] tracking-wide uppercase select-none font-semibold">sync</span>
    </div>
  );
}
