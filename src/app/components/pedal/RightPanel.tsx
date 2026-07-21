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
    <Panel className="w-1/4 flex flex-col items-center justify-around py-8">
      <div className="flex flex-col items-center gap-2">
        <img src={textImg} alt="Branding Text" className="h-6 object-contain filter brightness-110 opacity-90 mb-1" />
        <Knob type="white" label="mix" value={mixValue} onChange={onMixChange} />
      </div>
      <Knob type="black" label="mod" value={modValue} onChange={onModChange} />
    </Panel>
  );
}
