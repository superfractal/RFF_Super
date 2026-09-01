//
// Created by Merutilm on 2025-05-14.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-15, 2026-08-31
//

#pragma once
#include <functional>
#include "RenderScene.hpp"
#include "SettingsMenu.hpp"

namespace merutilm::rff2 {
    struct CallbackRender {
        static const std::function<void(SettingsMenu &, RenderScene &)> SET_CLARITY;
        static const std::function<bool*(RenderScene &, bool)> LINEAR_INTERPOLATION;
        static const std::function<bool*(RenderScene &, bool)> DITHER;
        static const std::function<bool*(RenderScene &, bool)> BOUNDARY_TRACE_FILL;
        static const std::function<bool*(RenderScene &, bool)> PREVIEW_2COLOR;
        static const std::function<bool*(RenderScene &, bool)> COARSE_PREVIEW;
    };
}
