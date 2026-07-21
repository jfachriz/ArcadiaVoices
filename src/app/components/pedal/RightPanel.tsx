import Knob from './Knob';
import Panel from './Panel';
import textImg from '../../../assets/text.png';

interface RightPanelProps {
  mixValue: number;
  onMixChange: (v: number) => void;
  modValue: number;
  onModChange: (v: number) => void;
}

export default function RightPanel({ mixValue, onMixChange, modValue, onModChange }: RightPanelProps) {
  return (
    <Panel className="w-1/4 flex flex-col items-center justify-around py-8 relative">
      <img src={textImg} alt="Branding Text" className="absolute top-2 left-1/2 -translate-x-1/2 h-10 object-contain filter brightness-110 opacity-90" />
      <Knob type="white" label="mix" value={mixValue} onChange={onMixChange} />
      <Knob type="black" label="mod" value={modValue} onChange={onModChange} />
    </Panel>
  );
}
