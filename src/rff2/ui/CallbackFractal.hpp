//
// Created by Merutilm on 2025-05-14.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-31
//

#pragma once
#include <functional>

#include "RenderScene.hpp"
#include "SettingsMenu.hpp"

namespace merutilm::rff2 {
    struct CallbackFractal {
        static const std::function<void(SettingsMenu&, RenderScene&)> REFERENCE;
        static const std::function<void(SettingsMenu&, RenderScene&)> ITERATIONS;
        static const std::function<void(SettingsMenu&, RenderScene&)> MPA;
        static const std::function<bool*(RenderScene&, bool)> AUTOMATIC_ITERATIONS;
        static const std::function<bool*(RenderScene&, bool)> ABSOLUTE_ITERATION_MODE;
        static const std::function<void(SettingsMenu&, RenderScene&)> FORMULA;
        static const std::function<void(SettingsMenu&, RenderScene&)> PROJECTION;
    };
}
