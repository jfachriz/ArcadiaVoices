#pragma once
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>

// A snapshot of all parameter values for a preset
struct PresetSnapshot {
    int id;
    char name[64];
    float timeMs;         // ms
    float feedbackPct;    // 0-110
    float mixPct;         // 0-100
    float modPct;         // 0-100
    float pitchL;         // semitones, -24 to +24
    float pitchR;         // semitones, -24 to +24
    bool powerOn;
    bool syncMode;        // false = Free, true = Synced
    int noteDivision;     // index into NOTE_DIVISIONS

    PresetSnapshot() : id(0), timeMs(350.0f), feedbackPct(35.0f),
                       mixPct(50.0f), modPct(20.0f), pitchL(-2.0f),
                       pitchR(1.0f), powerOn(true), syncMode(false),
                       noteDivision(4) { // 1/8 in the 14-element division list
        name[0] = '\0';
    }
};

// In-memory preset manager. Handles preset list, loading, and save-as-new.
// Presets are serialized via iPlug2's state system (Phase 2 concern), not
// to individual files. Default factory presets are created in Init().
class PresetManager {
public:
    PresetManager() : mActivePresetId(1) {}

    // Initialize with factory presets
    void Init() {
        mPresets.clear();

        // Preset 1: "Multi Voice" — the default
        PresetSnapshot p1;
        p1.id = 1;
        std::strncpy(p1.name, "multi voice", sizeof(p1.name) - 1);
        p1.timeMs = 350.0f;
        p1.feedbackPct = 35.0f;
        p1.mixPct = 50.0f;
        p1.modPct = 20.0f;
        p1.pitchL = -2.0f;
        p1.pitchR = 1.0f;
        p1.powerOn = true;
        p1.syncMode = false;
        p1.noteDivision = 4; // 1/8
        mPresets.push_back(p1);

        // Preset 2: "Chorus Room" — dense, modulated, equal mix
        PresetSnapshot p2;
        p2.id = 2;
        std::strncpy(p2.name, "chorus room", sizeof(p2.name) - 1);
        p2.timeMs = 150.0f;
        p2.feedbackPct = 40.0f;
        p2.mixPct = 50.0f;
        p2.modPct = 80.0f;
        p2.pitchL = 0.0f;   // no pitch shift
        p2.pitchR = 0.0f;
        p2.powerOn = true;
        p2.syncMode = false;
        p2.noteDivision = 4; // 1/8
        mPresets.push_back(p2);

        // Preset 3: "Deep Space" — long, ambient, heavy pitch
        PresetSnapshot p3;
        p3.id = 3;
        std::strncpy(p3.name, "deep space", sizeof(p3.name) - 1);
        p3.timeMs = 1200.0f;
        p3.feedbackPct = 80.0f;
        p3.mixPct = 60.0f;
        p3.modPct = 40.0f;
        p3.pitchL = -7.0f;  // perfect fifth down
        p3.pitchR = 4.0f;   // major third up
        p3.powerOn = true;
        p3.syncMode = false;
        p3.noteDivision = 4; // 1/8
        mPresets.push_back(p3);

        mActivePresetId = 1;
    }

    // Get preset by ID
    const PresetSnapshot* GetPreset(int id) const {
        for (const auto& p : mPresets) {
            if (p.id == id) return &p;
        }
        return nullptr;
    }

    // Get preset by index in list (not ID)
    const PresetSnapshot* GetPresetByIndex(int index) const {
        if (index < 0 || index >= static_cast<int>(mPresets.size())) return nullptr;
        return &mPresets[index];
    }

    // Get preset by name
    const PresetSnapshot* GetPresetByName(const char* name) const {
        for (const auto& p : mPresets) {
            if (std::strcmp(p.name, name) == 0) return &p;
        }
        return nullptr;
    }

    // Load a preset by ID — returns pointer to the preset, or nullptr if not found
    const PresetSnapshot* LoadPreset(int id) {
        const auto* preset = GetPreset(id);
        if (preset) {
            mActivePresetId = id;
        }
        return preset;
    }

    // Save current parameter values as a new preset
    // Returns the new preset ID
    int SaveAsNew(const char* name, const PresetSnapshot& snapshot) {
        PresetSnapshot newPreset = snapshot;
        newPreset.id = GetNextId();
        if (name && name[0]) {
            std::strncpy(newPreset.name, name, sizeof(newPreset.name) - 1);
        } else {
            std::snprintf(newPreset.name, sizeof(newPreset.name), "user preset %d", newPreset.id);
        }
        mPresets.push_back(newPreset);
        mActivePresetId = newPreset.id;
        return newPreset.id;
    }

    // Update a preset's parameter snapshot (overwrite existing)
    bool UpdatePreset(int id, const PresetSnapshot& snapshot) {
        for (auto& p : mPresets) {
            if (p.id == id) {
                const int oldId = p.id;
                char oldName[64];
                std::strncpy(oldName, p.name, sizeof(oldName) - 1);
                p = snapshot;
                p.id = oldId;
                std::strncpy(p.name, oldName, sizeof(p.name) - 1);
                return true;
            }
        }
        return false;
    }

    // Rename a preset by ID
    bool RenamePreset(int id, const char* newName) {
        for (auto& p : mPresets) {
            if (p.id == id) {
                std::strncpy(p.name, newName, sizeof(p.name) - 1);
                p.name[sizeof(p.name) - 1] = '\0';
                return true;
            }
        }
        return false;
    }

    // Delete a preset by ID. Returns false if the preset doesn't exist or
    // if it's the last remaining preset (never allow empty preset bank).
    bool DeletePreset(int id) {
        if (mPresets.size() <= 1) return false;
        for (auto it = mPresets.begin(); it != mPresets.end(); ++it) {
            if (it->id == id) {
                mPresets.erase(it);
                // If we deleted the active preset, switch to the first available
                if (mActivePresetId == id && !mPresets.empty()) {
                    mActivePresetId = mPresets.front().id;
                }
                return true;
            }
        }
        return false;
    }

    // Clear all presets (no factory reset — for restore from serialized state)
    void Clear() {
        mPresets.clear();
        mActivePresetId = 1;
    }

    // Restore a preset with its original ID (for UnserializeState — does NOT
    // auto-assign a new ID like SaveAsNew does, and does NOT overwrite the
    // active preset ID — that is set later by the explicit LoadPreset call
    // after all presets are restored).
    void RestorePreset(const PresetSnapshot& preset) {
        mPresets.push_back(preset);
    }

    // Get active preset ID
    int ActivePresetId() const { return mActivePresetId; }

    // Get active preset
    const PresetSnapshot* ActivePreset() const {
        return GetPreset(mActivePresetId);
    }

    // Get number of presets
    int Count() const { return static_cast<int>(mPresets.size()); }

    // Get all presets (for serialization/UI)
    const std::vector<PresetSnapshot>& AllPresets() const { return mPresets; }

    // Clear all presets and re-initialize (for reset)
    void Reset() {
        mPresets.clear();
        Init();
    }

private:
    std::vector<PresetSnapshot> mPresets;
    int mActivePresetId;

    // Find the next available ID
    int GetNextId() const {
        int maxId = 0;
        for (const auto& p : mPresets) {
            if (p.id > maxId) maxId = p.id;
        }
        return maxId + 1;
    }
};
