//
// Created by Merutilm on 2025-05-16.
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-05, 2026-08-07, 2026-08-15, 2026-08-19, 2026-08-20
// Modified by GPT-5 on 2026-08-21
// Modified by GPT-6 on 2026-09-10, 2026-09-11, 2026-09-17, 2026-09-23
// Modified by Opus 5.5 on 2026-10-07, 2026-10-12, 2026-10-25
//

#pragma once
#include <filesystem>
#include <functional>

#include "RenderScene.hpp"
#include "SettingsMenu.hpp"

namespace merutilm::rff2 {
    struct CallbackShader {
        static const std::function<void(SettingsMenu &, RenderScene &)> PALETTE;
        static const std::function<void(SettingsMenu &, RenderScene &)> TEXTURE;
        static const std::function<void(SettingsMenu &, RenderScene &)> PATTERN;
        static const std::function<void(SettingsMenu &, RenderScene &)> WARP;
        static const std::function<void(SettingsMenu &, RenderScene &)> STRIPE;
        static const std::function<void(SettingsMenu &, RenderScene &)> SLOPE;
        static const std::function<void(SettingsMenu &, RenderScene &)> COLOR;
        static const std::function<void(SettingsMenu &, RenderScene &)> FOG;
        static const std::function<void(SettingsMenu &, RenderScene &)> EFFECTS;
        static const std::function<void(SettingsMenu &, RenderScene &)> BLOOM;
        static const std::function<void(SettingsMenu &, RenderScene &)> HDR;
        static const std::function<void(SettingsMenu &, RenderScene &)> LAYERS;
        // Settings that belong to no other panel, such as Escape Angle Coloring.
        static const std::function<void(SettingsMenu &, RenderScene &)> OTHER;
        // Orbit Trap coloring and the trap settings still to come.
        static const std::function<void(SettingsMenu &, RenderScene &)> ORBIT_TRAP;
        static const std::function<void(SettingsMenu &, RenderScene &)> LOAD_KFR_PALETTE;
        static const std::function<void(SettingsMenu &, RenderScene &)> SAVE_PRESET;
        // Puts an appearance preset on the scene; File > Load File hands it every .rfsp.
        static void loadPreset(SettingsMenu &settingsMenu, RenderScene &scene, const std::filesystem::path &path);
        static const std::function<void(SettingsMenu &, RenderScene &)> IMPORT_COLOR;
    };
}
