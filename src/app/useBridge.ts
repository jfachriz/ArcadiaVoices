import { useEffect, useRef, useCallback } from 'react';
import {
  installIPlugBridge,
  subscribeToParams,
  sendParamChange,
  sendParamBegin,
  sendParamEnd,
  isIPlugWebView,
  IPlugParam,
  timeToNormalized,
  feedbackToNormalized,
  mixToNormalized,
  modToNormalized,
  pitchToNormalized,
  powerToNormalized,
  syncToNormalized,
  divisionToNormalized,
} from './iplugBridge';

// Maps knob parameter values (0-100 UI range) to normalized 0-1 and sends to DSP
type KnobParam =
  | 'time' | 'feedback' | 'mix' | 'mod'
  | 'left' | 'right' | 'power' | 'syncMode' | 'noteDivision';

function knobToIPlugParam(key: KnobParam): IPlugParam | null {
  switch (key) {
    case 'time': return IPlugParam.Time;
    case 'feedback': return IPlugParam.Feedback;
    case 'mix': return IPlugParam.Mix;
    case 'mod': return IPlugParam.Mod;
    case 'left': return IPlugParam.PitchL;
    case 'right': return IPlugParam.PitchR;
    case 'power': return IPlugParam.Power;
    case 'syncMode': return IPlugParam.SyncMode;
    case 'noteDivision': return IPlugParam.NoteDivision;
    default: return null;
  }
}

function knobValueToNormalized(key: KnobParam, value: number | boolean): number {
  switch (key) {
    case 'time': return timeToNormalized(value as number);
    case 'feedback': return feedbackToNormalized(value as number);
    case 'mix': return mixToNormalized(value as number);
    case 'mod': return modToNormalized(value as number);
    case 'left':
    case 'right': return pitchToNormalized(value as number);
    case 'power': return powerToNormalized(value as boolean);
    case 'syncMode': return syncToNormalized(value as boolean);
    case 'noteDivision': return divisionToNormalized(value as number);
    default: return 0;
  }
}

export interface BridgeState {
  isConnected: boolean;
  sendKnobValue: (key: KnobParam, value: number | boolean) => void;
  beginKnobEdit: (key: KnobParam) => void;
  endKnobEdit: (key: KnobParam) => void;
}

export default function useBridge(): BridgeState {
  const connectedRef = useRef(false);

  useEffect(() => {
    installIPlugBridge();
    connectedRef.current = isIPlugWebView();

    // Subscribe to parameter changes from the DSP side
    // (e.g. when the DAW automates a parameter or a preset is loaded)
    const unsubscribe = subscribeToParams((_paramIdx, _normalizedValue) => {
      // Future: update local React state from DSP param changes
    });

    return unsubscribe;
  }, []);

  const sendKnobValue = useCallback((key: KnobParam, value: number | boolean) => {
    if (!connectedRef.current) return;
    const paramIdx = knobToIPlugParam(key);
    if (paramIdx === null) return;
    const normalized = knobValueToNormalized(key, value);
    sendParamChange(paramIdx, normalized);
  }, []);

  const beginKnobEdit = useCallback((key: KnobParam) => {
    if (!connectedRef.current) return;
    const paramIdx = knobToIPlugParam(key);
    if (paramIdx === null) return;
    sendParamBegin(paramIdx);
  }, []);

  const endKnobEdit = useCallback((key: KnobParam) => {
    if (!connectedRef.current) return;
    const paramIdx = knobToIPlugParam(key);
    if (paramIdx === null) return;
    sendParamEnd(paramIdx);
  }, []);

  return {
    isConnected: connectedRef.current,
    sendKnobValue,
    beginKnobEdit,
    endKnobEdit,
  };
}
