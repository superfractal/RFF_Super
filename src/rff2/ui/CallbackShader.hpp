//
// Created by Merutilm on 2025-05-16.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-05, 2026-08-07, 2026-08-15, 2026-08-19, 2026-08-20
//

#pragma once
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
        static const std::function<void(SettingsMenu &, RenderScene &)> BLOOM;
        static const std::function<void(SettingsMenu &, RenderScene &)> HDR;
        static const std::function<void(SettingsMenu &, RenderScene &)> LOAD_KFR_PALETTE;
        static const std::function<void(SettingsMenu &, RenderScene &)> SAVE_PRESET;
        static const std::function<void(SettingsMenu &, RenderScene &)> LOAD_PRESET;
    };
}
