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
        backgroundColor: isSynced ? '#111' : '#d5d5d5',
        boxShadow: 'inset 0 1px 3px rgba(0,0,0,0.3), 0 1px 1px rgba(255,255,255,0.5)'
      }}
    >
      <motion.div
        className="w-[18px] h-[18px] rounded-full bg-white flex items-center justify-center text-[7px] font-medium text-gray-600"
        animate={{ x: isSynced ? 26 : 0 }}
        transition={{ type: 'spring', stiffness: 500, damping: 30 }}
        style={{ boxShadow: '0 1px 3px rgba(0,0,0,0.3)' }}
      >
        {isSynced ? '♪' : 'ms'}
      </motion.div>
      <span className="absolute left-[-24px] text-[8px] text-gray-500 tracking-wide uppercase select-none">free</span>
      <span className="absolute right-[-28px] text-[8px] text-gray-500 tracking-wide uppercase select-none">sync</span>
    </div>
  );
}
