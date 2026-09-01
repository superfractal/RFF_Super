//
// Created by Opus 5 on 2026-08-14.
// Modified by Opus 5 on 2026-08-15.
//

#pragma once
#include <filesystem>

#include "RenderScene.hpp"
#include "SettingsMenu.hpp"
#include "../io/RecoveryIO.h"

namespace merutilm::rff2 {
    // The panel a start after a run that did not finish opens: what that run was working on, and
    // the four ways of taking it up again. Generation is held until one of them is taken, because
    // the settings being offered back may be the ones that ended that run - or the ones too heavy
    // to have waited for. reason is what the panel says of the ending; the choices are the same.
    struct RecoveryPrompt {
        RecoveryPrompt() = delete;

        static void offer(SettingsMenu &settingsMenu, RenderScene &scene,
                          const std::filesystem::path &snapshot, RecoveryReason reason);
    };
}
