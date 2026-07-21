interface PowerLEDProps {
  powerOn: boolean;
}

export default function PowerLED({ powerOn }: PowerLEDProps) {
  return (
    <div
      className="absolute top-6 left-1/2 -translate-x-1/2 w-3.5 h-3.5 rounded-full transition-all duration-150"
      style={{
        backgroundColor: powerOn ? '#ff2020' : '#4a0000',
        boxShadow: powerOn
          ? '0 0 12px #ff4040, 0 0 24px rgba(255,40,40,0.8), inset 0 2px 2px rgba(255,255,255,0.6)'
          : 'inset 0 2px 4px rgba(0,0,0,0.8)',
        border: '1px solid rgba(0,0,0,0.6)'
      }}
    />
  );
}
