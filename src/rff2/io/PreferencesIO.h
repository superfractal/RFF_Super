//
// Created by Opus 5 on 2026-09-01
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

        // Reads the file into the live flags. A missing or unreadable file leaves them at their defaults.
        static void load();

        // Writes the live flags out, called wherever one of them is flipped so nothing is owed at shutdown.
        static void save();
    };
}
