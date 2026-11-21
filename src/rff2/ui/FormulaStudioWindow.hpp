//
// Created by Gemini 4 Neon on 2026-11-19
//

#pragma once

#include <functional>
#include <string>
#include <windows.h>

namespace merutilm::rff2 {
    // Dedicated Formula Studio window with calculator keys, live validation and an escape-time preview.
    struct FormulaStudioWindow {
        FormulaStudioWindow() = delete;

        // Opens the studio window or brings it forward with the given initial formula.
        static void open(HWND owner, const std::wstring &initialFormula,
                         std::function<std::wstring(const std::wstring &)> onApply);

        // Handles Enter, Escape and formula-level Undo/Redo when the studio window is active.
        static bool filterMessage(const MSG &message);

        // Redraws the open window in the current light, dark or high-contrast theme.
        static void applyTheme();

        // Closes the window ahead of application shutdown.
        static void close();
    };
}
