//
// Modified by GPT-6 on 2026-09-13, 2026-09-14, 2026-09-23
//

#pragma once

#include "../SettingsTheme.hpp"

namespace merutilm::rff2::workspace {
    // Uses the shared SettingsTheme palette; its third-party attribution is recorded in NOTICE.
    struct WorkspaceTheme {
        COLORREF background;
        COLORREF foreground;
        COLORREF secondary;
        COLORREF accent;
        COLORREF selected;
        COLORREF selectedForeground;
        COLORREF field;
        COLORREF track;
        COLORREF error;

        static WorkspaceTheme current() {
            const auto& shared = settingsTheme();
            return {
                .background = shared.background,
                .foreground = shared.text,
                .secondary = shared.rangeText,
                .accent = shared.cardNoteAccent,
                .selected = shared.radioSelectedBackground,
                .selectedForeground = shared.radioSelectedText,
                .field = shared.textFieldBackground,
                .track = shared.sliderTrack,
                .error = shared.textError
            };
        }
    };
}
