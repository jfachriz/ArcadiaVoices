// Shared types and constants for the Arcadia Voices UI

export interface KnobValues {
  power: boolean;
  time: number;
  feedback: number;
  mix: number;
  mod: number;
  left: number;
  right: number;
  preset: number;
}

export interface SavedPreset {
  id: number;
  name: string;
  knobs: {
    time: number;
    feedback: number;
    mix: number;
    mod: number;
    left: number;
    right: number;
  };
  powerOn: boolean;
  syncMode: boolean;
  noteDivisionIndex: number;
}

export const NOTE_DIVISIONS = [
  '1/32', '1/16T', '1/16', '1/8T', '1/8', '1/8D',
  '1/4T', '1/4', '1/4D', '1/2T', '1/2', '1/2D',
  '1 bar', '2 bars'
];

// Convert time knob value (0–100) to milliseconds
export function timeToMs(value: number): number {
  return Math.round(5 * Math.pow(10, 2 * value / 100));
}

// Convert knob value (0–100) to semitones (-24 to +24)
export function knobToSemitone(value: number): number {
  return Math.round((value / 100) * 48 - 24);
}
