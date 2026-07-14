// iPlug2 WebView bridge for Arcadia Voices
// Handles bidirectional communication between the React UI and the C++ DSP

export const enum IPlugParam {
  Time = 0,
  Feedback = 1,
  Mix = 2,
  Mod = 3,
  PitchL = 4,
  PitchR = 5,
  Power = 6,
  SyncMode = 7,
  NoteDivision = 8,
}

export const enum IPlugMsgTag {
  PitchReadoutL = 0,
  PitchReadoutR = 1,
  PresetList = 2,
  PowerState = 3,
}

// ---- Map functions (WebView side) ----
// WebView params are normalized 0-1; DSP receives real units.

export function timeToNormalized(knobValue: number): number {
  return knobValue / 100;
}

export function feedbackToNormalized(knobValue: number): number {
  return knobValue / 100;
}

export function mixToNormalized(knobValue: number): number {
  return knobValue / 100;
}

export function modToNormalized(knobValue: number): number {
  return knobValue / 100;
}

// Pitch: knob 0-100 → -24..+24 semitones → normalized 0-1
export function pitchToNormalized(knobValue: number): number {
  const semitones = Math.round((knobValue / 100) * 48 - 24);
  return (semitones + 24) / 48;
}

export function powerToNormalized(on: boolean): number {
  return on ? 1.0 : 0.0;
}

export function syncToNormalized(enabled: boolean): number {
  return enabled ? 1.0 : 0.0;
}

export function divisionToNormalized(index: number): number {
  return index / 13;
}

// ---- Bridge window type ----
interface IPlugWindow extends Window {
  IPlugSendMsg?: (message: unknown) => void;
  SPVFD?: (paramIdx: number, normalizedValue: number) => void;
  SAMFD?: (msgTag: number, dataSize: number, data: string) => void;
  SCVFD?: (ctrlTag: number, value: number) => void;
  SCMFD?: (ctrlTag: number, msgTag: number, dataSize: number, data: string) => void;
  OnParamChange?: (paramIdx: number, value: number) => void;
  OnMessage?: (msgTag: number, dataSize: number, data: string) => void;
}

// ---- Listener system ----
type ParamListener = (paramIdx: number, normalizedValue: number) => void;
type MessageListener = (msgTag: number, data: string) => void;
type ControlListener = (ctrlTag: number, value: number) => void;

const paramListeners = new Set<ParamListener>();
const msgListeners = new Map<number, MessageListener>();
const idListeners = new Map<string, MessageListener>();
const lastParamValues = new Map<number, number>();

function hostWindow(): IPlugWindow {
  return window as unknown as IPlugWindow;
}

export function isIPlugWebView(): boolean {
  return typeof hostWindow().IPlugSendMsg === "function";
}

// ---- UI → DSP ----

export function sendParamChange(paramIdx: IPlugParam, normalizedValue: number) {
  const win = hostWindow();
  if (win.IPlugSendMsg) {
    win.IPlugSendMsg({
      msg: "SPVFUI",
      paramIdx,
      value: Math.max(0, Math.min(1, normalizedValue)),
    });
  }
}

export function sendParamBegin(paramIdx: IPlugParam) {
  hostWindow().IPlugSendMsg?.({
    msg: "BPCFUI",
    paramIdx,
  });
}

export function sendParamEnd(paramIdx: IPlugParam) {
  hostWindow().IPlugSendMsg?.({
    msg: "EPCFUI",
    paramIdx,
  });
}

export function sendArbitraryMsg(msgTag: number, data: string) {
  hostWindow().IPlugSendMsg?.({
    msg: "SAMFUI",
    msgTag,
    ctrlTag: -1,
    data: btoa(data),
  });
}

// ---- DSP → UI (subscribe) ----

export function subscribeToParams(listener: ParamListener): () => void {
  paramListeners.add(listener);
  lastParamValues.forEach((v, i) => listener(i, v));
  return () => paramListeners.delete(listener);
}

export function subscribeToMessages(msgTag: IPlugMsgTag, listener: MessageListener): () => void {
  msgListeners.set(msgTag, listener);
  return () => msgListeners.delete(msgTag);
}

// Subscribe to incoming messages by the JSON "id" field
export function subscribeToMessageId(id: string, listener: MessageListener): () => void {
  idListeners.set(id, listener);
  return () => idListeners.delete(id);
}

// ---- Install bridge (called once at startup) ----

export function installIPlugBridge() {
  const win = hostWindow();

  // If already installed, skip
  if (win.SPVFD) return;

  // Called by C++ when a parameter changes
  win.SPVFD = (paramIdx: number, normalizedValue: number) => {
    lastParamValues.set(paramIdx, normalizedValue);
    paramListeners.forEach((fn) => fn(paramIdx, normalizedValue));
  };

  // Called by C++ when an arbitrary message arrives
  win.SAMFD = (_msgTag: number, dataSize: number, data: string) => {
    try {
      const decoded = atob(data);
      const msg = JSON.parse(decoded) as { id?: string; msgTag?: number };
      // Dispatch by numeric msgTag first
      const tag = msg.msgTag ?? _msgTag;
      const tagHandler = msgListeners.get(tag);
      if (tagHandler) {
        tagHandler(tag, decoded);
      }
      // Also dispatch by string id for JSON-identified messages
      if (msg.id) {
        const idHandler = idListeners.get(msg.id);
        if (idHandler) {
          idHandler(tag, decoded);
        }
      }
    } catch {
      // Non-JSON data — ignore
    }
  };

  // Called by C++ for control value updates
  win.SCVFD = (_ctrlTag: number, _value: number) => {
    // Not used in v1
  };

  win.SCMFD = (_ctrlTag: number, _msgTag: number, _dataSize: number, _data: string) => {
    // Not used in v1
  };

  // For debugging / dev tools
  win.OnParamChange = win.SPVFD;
  win.OnMessage = (_msgTag, _dataSize, _data) => {
    // Forward to SAMFD
  };
}
