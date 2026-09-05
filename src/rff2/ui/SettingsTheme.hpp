// Modified by GPT-5 on 2026-08-26, 2026-08-27
// Modified by Opus 5 on 2026-08-27, 2026-09-01, 2026-09-03

#pragma once

#include <atomic>
#include <optional>
#include <windows.h>

namespace merutilm::rff2 {
    struct SettingsThemeColors {
        COLORREF background;
        COLORREF text;
        COLORREF textDisabled;
        COLORREF textError;
        COLORREF textEdited;
        COLORREF textModified;
        COLORREF tooltipBackground;
        COLORREF tooltipText;
        COLORREF checkboxChecked;
        COLORREF checkboxBorder;
        COLORREF checkboxMark;
        COLORREF checkboxDisabledChecked;
        COLORREF sectionFrame;
        COLORREF sliderTrack;
        COLORREF sliderFill;
        COLORREF sliderThumbFace;
        COLORREF sliderThumbBorder;
        COLORREF sliderTrackDisabled;
        COLORREF sliderDisabled;
        COLORREF rangeText;
        COLORREF previewBorder;
        COLORREF previewBorderSelected;
        COLORREF radioSelectedBackground;
        COLORREF radioSelectedBorder;
        COLORREF primaryButton;
        COLORREF primaryButtonPressed;
        COLORREF primaryButtonText;
        COLORREF cardNoteBackground;
        COLORREF cardNoteBorder;
        COLORREF cardNoteAccent;
        COLORREF cardNoteTitle;
        COLORREF textFieldBackground;
        COLORREF textFieldBorder;
        COLORREF buttonFace;
        COLORREF buttonFacePressed;
        COLORREF controlDisabledFace;
        COLORREF buttonBorder;
    };

    // Selected UI colors use Tailwind CSS v3's MIT-licensed palette; see NOTICE.
    inline constexpr SettingsThemeColors LIGHT_SETTINGS_THEME = {
        .background = RGB(248, 250, 252), .text = RGB(30, 41, 59), .textDisabled = RGB(148, 163, 184),
        .textError = RGB(255, 0, 0), .textEdited = RGB(146, 64, 14), .textModified = RGB(3, 105, 161),
        .tooltipBackground = RGB(248, 250, 252), .tooltipText = RGB(15, 23, 42),
        .checkboxChecked = RGB(37, 99, 235), .checkboxBorder = RGB(148, 163, 184),
        .checkboxMark = RGB(255, 255, 255), .checkboxDisabledChecked = RGB(100, 116, 139),
        .sectionFrame = RGB(226, 232, 240), .sliderTrack = RGB(226, 232, 240),
        .sliderFill = RGB(37, 99, 235), .sliderThumbFace = RGB(255, 255, 255),
        .sliderThumbBorder = RGB(203, 213, 225), .sliderTrackDisabled = RGB(241, 245, 249),
        .sliderDisabled = RGB(203, 213, 225), .rangeText = RGB(100, 116, 139),
        .previewBorder = RGB(148, 163, 184), .previewBorderSelected = RGB(37, 99, 235),
        .radioSelectedBackground = RGB(239, 246, 255), .radioSelectedBorder = RGB(147, 197, 253),
        .primaryButton = RGB(37, 99, 235), .primaryButtonPressed = RGB(29, 78, 216),
        .primaryButtonText = RGB(255, 255, 255), .cardNoteBackground = RGB(239, 246, 255),
        .cardNoteBorder = RGB(191, 219, 254), .cardNoteAccent = RGB(37, 99, 235),
        .cardNoteTitle = RGB(15, 23, 42), .textFieldBackground = RGB(255, 255, 255),
        .textFieldBorder = RGB(203, 213, 225), .buttonFace = RGB(255, 255, 255),
        .buttonFacePressed = RGB(226, 232, 240), .controlDisabledFace = RGB(241, 245, 249),
        .buttonBorder = RGB(203, 213, 225)
    };

    inline constexpr SettingsThemeColors DARK_SETTINGS_THEME = {
        .background = RGB(21, 26, 32), .text = RGB(203, 205, 208), .textDisabled = RGB(88, 89, 92),
        .textError = RGB(205, 93, 102), .textEdited = RGB(215, 166, 106), .textModified = RGB(127, 168, 216),
        .tooltipBackground = RGB(21, 26, 32), .tooltipText = RGB(203, 205, 208),
        .checkboxChecked = RGB(42, 86, 164), .checkboxBorder = RGB(88, 89, 92),
        .checkboxMark = RGB(203, 205, 208), .checkboxDisabledChecked = RGB(88, 89, 92),
        .sectionFrame = RGB(52, 59, 68), .sliderTrack = RGB(52, 59, 68),
        .sliderFill = RGB(42, 86, 164), .sliderThumbFace = RGB(203, 205, 208),
        .sliderThumbBorder = RGB(88, 89, 92), .sliderTrackDisabled = RGB(37, 43, 50),
        .sliderDisabled = RGB(88, 89, 92), .rangeText = RGB(157, 160, 164),
        .previewBorder = RGB(88, 89, 92), .previewBorderSelected = RGB(42, 86, 164),
        .radioSelectedBackground = RGB(28, 39, 53), .radioSelectedBorder = RGB(42, 86, 164),
        .primaryButton = RGB(42, 86, 164), .primaryButtonPressed = RGB(35, 71, 131),
        .primaryButtonText = RGB(203, 205, 208), .cardNoteBackground = RGB(26, 36, 48),
        .cardNoteBorder = RGB(42, 86, 164), .cardNoteAccent = RGB(111, 152, 211),
        .cardNoteTitle = RGB(203, 205, 208), .textFieldBackground = RGB(32, 38, 45),
        .textFieldBorder = RGB(88, 89, 92), .buttonFace = RGB(32, 38, 45),
        .buttonFacePressed = RGB(42, 48, 56), .controlDisabledFace = RGB(27, 32, 38),
        .buttonBorder = RGB(88, 89, 92)
    };

    inline bool &darkSettingsModeFlag() {
        static bool dark = false;
        return dark;
    }

    inline bool darkSettingsMode() {
        return darkSettingsModeFlag();
    }

    inline const SettingsThemeColors &settingsTheme() {
        return darkSettingsMode() ? DARK_SETTINGS_THEME : LIGHT_SETTINGS_THEME;
    }

    // The Timeline Editor's own Light/Dark switch, which stands apart from the View menu's flag above.
    inline std::atomic_bool &timelineLightModeFlag() {
        static std::atomic_bool light{false};
        return light;
    }

    inline bool timelineLightMode() {
        return timelineLightModeFlag().load(std::memory_order_relaxed);
    }

    // Puts one Light/Dark choice in front of every color above for as long as it is in scope, which
    // is what draws a panel opened from the Timeline Editor under that editor's switch instead.
    class ScopedSettingsMode {
        bool previous;
        bool overridden;

    public:
        explicit ScopedSettingsMode(const std::optional<bool> dark)
            : previous(darkSettingsModeFlag()), overridden(dark.has_value()) {
            if (overridden) {
                darkSettingsModeFlag() = *dark;
            }
        }

        ~ScopedSettingsMode() {
            if (overridden) {
                darkSettingsModeFlag() = previous;
            }
        }

        ScopedSettingsMode(const ScopedSettingsMode &) = delete;

        ScopedSettingsMode &operator=(const ScopedSettingsMode &) = delete;

        ScopedSettingsMode(ScopedSettingsMode &&) = delete;

        ScopedSettingsMode &operator=(ScopedSettingsMode &&) = delete;
    };

    // The native theme class a control is looked up under. Only worth setting on controls whose own
    // painting is left to comctl32 - anything drawn here already carries the theme's colors, and the
    // dark class only changes the path comctl32 takes to draw it.
    inline void applyDarkThemeClass(const HWND control, const bool combo) {
        using SetWindowThemeFn = HRESULT (WINAPI *)(HWND, LPCWSTR, LPCWSTR);
        static const auto setWindowTheme = reinterpret_cast<SetWindowThemeFn>(
            GetProcAddress(LoadLibraryW(L"uxtheme.dll"), "SetWindowTheme"));
        if (setWindowTheme == nullptr || control == nullptr) {
            return;
        }
        if (darkSettingsMode()) {
            setWindowTheme(control, combo ? L"DarkMode_CFD" : L"DarkMode_Explorer", nullptr);
        } else {
            setWindowTheme(control, nullptr, nullptr);
        }
    }

    // Takes a control off the visual styles entirely, in both modes. Used for the controls this
    // program paints itself: with a theme open comctl32 keeps a say in how they are drawn, and the
    // dark app class is where that showed - a themed static stops erasing behind its own text.
    inline void disableThemeClass(const HWND control) {
        using SetWindowThemeFn = HRESULT (WINAPI *)(HWND, LPCWSTR, LPCWSTR);
        static const auto setWindowTheme = reinterpret_cast<SetWindowThemeFn>(
            GetProcAddress(LoadLibraryW(L"uxtheme.dll"), "SetWindowTheme"));
        if (setWindowTheme != nullptr && control != nullptr) {
            setWindowTheme(control, L"", L"");
        }
    }

    // Title bar, border and caption text of a top-level window, which live outside the client area
    // and can only be recolored through DWM.
    inline void applyDarkWindowFrame(const HWND window) {
        using DwmSetWindowAttributeFn = HRESULT (WINAPI *)(HWND, DWORD, LPCVOID, DWORD);
        static const auto dwmSetWindowAttribute = reinterpret_cast<DwmSetWindowAttributeFn>(
            GetProcAddress(LoadLibraryW(L"dwmapi.dll"), "DwmSetWindowAttribute"));
        if (dwmSetWindowAttribute == nullptr || window == nullptr) {
            return;
        }
        const BOOL dark = darkSettingsMode();
        if (FAILED(dwmSetWindowAttribute(window, 20, &dark, sizeof(dark)))) {
            dwmSetWindowAttribute(window, 19, &dark, sizeof(dark));
        }
        constexpr COLORREF defaultColor = 0xFFFFFFFF;
        constexpr COLORREF noBorderColor = 0xFFFFFFFE;
        const COLORREF borderColor = dark ? noBorderColor : defaultColor;
        const COLORREF captionColor = dark ? settingsTheme().background : defaultColor;
        const COLORREF captionTextColor = dark ? settingsTheme().text : defaultColor;
        dwmSetWindowAttribute(window, 34, &borderColor, sizeof(borderColor));
        dwmSetWindowAttribute(window, 35, &captionColor, sizeof(captionColor));
        dwmSetWindowAttribute(window, 36, &captionTextColor, sizeof(captionTextColor));
        SetWindowPos(window, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
}
