//
// Created by Opus 5 on 2026-08-14.
// Modified by Opus 5 on 2026-08-15, 2026-08-31
// Modified by GPT-5 on 2026-09-01
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <functional>

#include "RenderScene.hpp"
#include "SettingsMenu.hpp"

namespace merutilm::rff2 {
    struct CallbackDebug {
        using MenuCallback = std::function<void(SettingsMenu &, RenderScene &)>;

        static const MenuCallback DUMP_SCENE_STATE;
        static const MenuCallback SHOW_PASS_TIMES;
    };
}
