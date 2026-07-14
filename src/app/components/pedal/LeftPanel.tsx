import SyncToggle from './SyncToggle';
import Knob from './Knob';
import Panel from './Panel';
import { NOTE_DIVISIONS } from './types';

interface LeftPanelProps {
  syncMode: boolean;
  onToggleSync: () => void;
  timeValue: number;
  noteDivisionIndex: number;
  onTimeChange: (v: number) => void;
  feedbackValue: number;
  onFeedbackChange: (v: number) => void;
}

export default function LeftPanel({
  syncMode, onToggleSync,
  timeValue, noteDivisionIndex, onTimeChange,
  feedbackValue, onFeedbackChange
}: LeftPanelProps) {
  const knobValue = syncMode
    ? (noteDivisionIndex / (NOTE_DIVISIONS.length - 1)) * 100
    : timeValue;

  return (
    <Panel className="w-1/4 flex flex-col items-center justify-around py-8">
      <div className="flex flex-col items-center gap-3">
        <SyncToggle isSynced={syncMode} onToggle={onToggleSync} />
        <Knob
          type="white"
          label="time"
          value={knobValue}
          onChange={onTimeChange}
          isSynced={syncMode}
          noteDivisionLabel={NOTE_DIVISIONS[noteDivisionIndex]}
        />
      </div>
      <Knob
        type="black"
        label="feedback"
        value={feedbackValue}
        onChange={onFeedbackChange}
      />
    </Panel>
  );
}
