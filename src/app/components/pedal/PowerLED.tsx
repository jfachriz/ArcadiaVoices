interface PowerLEDProps {
  powerOn: boolean;
}

export default function PowerLED({ powerOn }: PowerLEDProps) {
  return (
    <div
      className="absolute top-6 left-1/2 -translate-x-1/2 w-3 h-3 rounded-full transition-colors duration-200"
      style={{
        backgroundColor: powerOn ? '#10b981' : '#888',
        boxShadow: powerOn
          ? '0 0 10px 2px #10b981, inset 0 1px 2px rgba(255,255,255,0.8)'
          : 'inset 0 1px 2px rgba(0,0,0,0.5)',
        border: '1px solid rgba(0,0,0,0.2)'
      }}
    />
  );
}
