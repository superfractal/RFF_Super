//
// Created by Merutilm on 2025-06-08.
// Modified by Opus 5 on 2026-08-18
// Modified by GPT-5 on 2026-08-18, 2026-08-24
//

#pragma once
#include <functional>

#include "RenderScene.hpp"
#include "SettingsMenu.hpp"

namespace merutilm::rff2 {
    struct CallbackVideo {
        static const std::function<void(SettingsMenu &, RenderScene &)> DATA_SETTINGS;
        static const std::function<void(SettingsMenu &, RenderScene &)> ANIMATION_SETTINGS;
        static const std::function<void(SettingsMenu &, RenderScene &)> TIMELINE_EDITOR;
        static const std::function<void(SettingsMenu &, RenderScene &)> EXPORT_SETTINGS;
        static const std::function<void(SettingsMenu &, RenderScene &)> GENERATE_VID_KEYFRAME;
        static const std::function<void(SettingsMenu &, RenderScene &)> EXPORT_ZOOM_VID;
    };
}
