//
// Created by Merutilm on 2025-05-14.
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-15, 2026-08-31
// Modified by GPT-5 on 2026-08-21
// Modified by GPT-6 on 2026-09-18, 2026-09-23
//

#pragma once
#include <functional>

#include "RenderScene.hpp"
#include "SettingsMenu.hpp"

namespace merutilm::rff2 {
    struct CallbackRender {
        using MenuCallback = std::function<void(SettingsMenu &, RenderScene &)>;
        using ToggleCallback = std::function<bool *(RenderScene &, bool)>;

        static const MenuCallback SET_CLARITY;

        static const ToggleCallback LINEAR_INTERPOLATION;
        static const ToggleCallback DITHER;
        static const ToggleCallback SMOOTH_ZOOM;
        static const ToggleCallback BOUNDARY_TRACE_FILL;
        static const ToggleCallback PREVIEW_2COLOR;
        static const ToggleCallback COARSE_PREVIEW;
    };
}
