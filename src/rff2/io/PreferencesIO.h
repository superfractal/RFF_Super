//
// Created by Opus 5 on 2026-09-01
// Modified by GPT-6 on 2026-09-18, 2026-09-19, 2026-09-23
// Modified by Opus 5.5 on 2026-11-01
//

#pragma once
#include <cstdint>

namespace merutilm::rff2 {
    // The View menu's Dark Mode and the Timeline Editor's Light/Dark switch, kept beside the program
    // so the next start opens in the colors the last one was left in.
    struct PreferencesIO {
        PreferencesIO() = delete;

        static constexpr uint32_t MAGIC = 0x52464650; // "RFFP"
        static constexpr uint32_t VERSION = 1;

        static bool &showSettingDescriptions() {
            static bool showDescriptions = true;
            return showDescriptions;
        }

        static bool &legacySettingsUI() {
            static bool useLegacySettings = false;
            return useLegacySettings;
        }

        // Threads the Timeline Editor reads keyframes on when preloading previews to RAM; 0 picks the count itself.
        static constexpr uint8_t MAX_PREVIEW_LOAD_THREADS = 64;
        static uint8_t &previewLoadThreads() {
            static uint8_t threads = 0;
            return threads;
        }

        // Step the Timeline Editor rounds added, dragged and scrubbed keyframe positions to: 0 = 1, 1 = 0.1, 2 = 0.01.
        static constexpr uint8_t MAX_KEYFRAME_SNAP = 2;
        static uint8_t &keyframeSnap() {
            static uint8_t snap = 0;
            return snap;
        }
        static float keyframeSnapStep() {
            return keyframeSnap() == 2 ? 0.01f : keyframeSnap() == 1 ? 0.1f : 1.0f;
        }

        // Reads the file into the live flags. A missing or unreadable file leaves them at their defaults.
        static void load();

        // Writes the live flags out, called wherever one of them is flipped so nothing is owed at shutdown.
        static void save();
    };
}
