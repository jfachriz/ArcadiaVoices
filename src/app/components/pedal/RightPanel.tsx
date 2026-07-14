import Knob from './Knob';
import Panel from './Panel';

interface RightPanelProps {
  mixValue: number;
  onMixChange: (v: number) => void;
  modValue: number;
  onModChange: (v: number) => void;
}

export default function RightPanel({ mixValue, onMixChange, modValue, onModChange }: RightPanelProps) {
  return (
    <Panel className="w-1/4 flex flex-col items-center justify-around py-8">
      <Knob type="white" label="mix" value={mixValue} onChange={onMixChange} />
      <Knob type="black" label="mod" value={modValue} onChange={onModChange} />
    </Panel>
  );
}
