//
// Created by Merutilm on 2025-06-08.
// Modified by Opus 5 on 2026-08-18
// Modified by GPT-5 on 2026-08-18, 2026-08-24
// Modified by GPT-6 on 2026-09-08, 2026-09-23
//

#pragma once
#include <functional>

#include "RenderScene.hpp"
#include "SettingsMenu.hpp"

namespace merutilm::rff2 {
    struct CallbackVideo {
        using MenuCallback = std::function<void(SettingsMenu &, RenderScene &)>;

        static const MenuCallback DATA_SETTINGS;
        static const MenuCallback CAMERA_SETTINGS;
        static const MenuCallback ANIMATION_SETTINGS;
        static const MenuCallback TIMELINE_EDITOR;
        static const MenuCallback EXPORT_SETTINGS;
        static const MenuCallback GENERATE_VID_KEYFRAME;
        static const MenuCallback EXPORT_ZOOM_VID;
    };
}
