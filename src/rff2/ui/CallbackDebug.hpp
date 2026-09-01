//
// Created by Opus 5 on 2026-08-14.
// Modified by Opus 5 on 2026-08-15, 2026-08-31
// Modified by GPT-5 on 2026-09-01
//

#pragma once
#include <functional>

#include "RenderScene.hpp"
#include "SettingsMenu.hpp"

namespace merutilm::rff2 {
    struct CallbackDebug {
        static const std::function<void(SettingsMenu &, RenderScene &)> DUMP_SCENE_STATE;
        static const std::function<void(SettingsMenu &, RenderScene &)> SHOW_PASS_TIMES;
    };
}
