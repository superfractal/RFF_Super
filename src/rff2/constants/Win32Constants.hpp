//
// Created by Merutilm on 2025-08-09.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21, 2026-08-27.
// Modified by Opus 5 on 2026-08-06, 2026-08-11, 2026-08-12, 2026-08-14, 2026-08-23, 2026-08-27, 2026-08-31, 2026-09-01, 2026-09-02, 2026-09-03
// Modified by SuperFractal on 2026-08-24, 2026-08-25
//

#pragma once
#include <algorithm>
#include <cwchar>
#include <windows.h>
#include <commctrl.h>

namespace merutilm::rff2::Constants::Win32 {

        constexpr int INIT_RENDER_SCENE_WIDTH = 1280;
        constexpr int INIT_RENDER_SCENE_HEIGHT = 720;
        constexpr int BASELINE_DISPLAY_WIDTH = 1920;
        constexpr int BASELINE_DISPLAY_HEIGHT = 1080;
        constexpr int MIN_WINDOW_WIDTH = 300;
        constexpr int MIN_WINDOW_HEIGHT = 300;
        constexpr float INIT_RENDER_SCENE_FPS = 60;
        constexpr int INIT_SETTINGS_WINDOW_WIDTH = 760;
        constexpr int PROGRESS_BAR_HEIGHT = 40;
        constexpr int SETTINGS_INPUT_HEIGHT = 34;
        constexpr int SETTINGS_CHECKBOX_SIZE = 18;
        constexpr int GAP_SETTINGS_INPUT = 16;
        constexpr int GAP_SETTINGS_COLOR_SWATCH = 6;
        constexpr int SETTINGS_LABEL_WIDTH_DIVISOR = 2;
        constexpr int MAX_AMOUNT_COMBOBOX = 7;
        // The running version, shown by the dialog the main window's ? menu opens. Bump it
        // together with the CHANGELOG heading of the release being prepared.
        constexpr auto APPLICATION_VERSION = "v2.2.0.3";
        constexpr auto CLASS_MASTER_WINDOW = L"RFF2MW";
        constexpr auto CLASS_SETTINGS_WINDOW = L"RFF2SW";
        constexpr auto CLASS_VIDEO_WINDOW = L"RFF2VW";
        constexpr auto CLASS_VIDEO_RENDER_WINDOW = L"RFF2VRW";
        constexpr auto CLASS_VK_RENDER_SCENE = L"RFF2VRS";
        constexpr auto CLASS_BOX_ZOOM_OVERLAY = L"RFF2BZO";
        constexpr auto CLASS_IMAGE_VIEWER_WINDOW = L"RFF2IVW";
        // Windows 11's own UI face, hinted for the 12-16px band these windows draw at. Windows 10
        // and earlier do not have it, and the GDI font mapper answers a face name it cannot resolve
        // with a silent substitution (a gothic face, on a Japanese system) instead of an error - so
        // the mapper is asked once and the name it actually returned is what gets used. See
        // uiFontFace() below.
        constexpr auto FONT_PREFERRED = L"Segoe UI Variable Text";
        constexpr auto FONT_FALLBACK = L"Segoe UI";
        constexpr int FONT_SIZE = 25;
        constexpr DWORD STYLE_EX_TOOLTIP = WS_EX_TOPMOST;
        // No WS_EX_COMPOSITED here, tempting as it is: it would buffer the panel and its rows
        // together, but the rows are moved by ScrollWindowEx, and a composited window never
        // repaints what that moved - scrolling leaves whole sections blank.
        // No WS_EX_TOPMOST: that held the panels over every other application, not just over RFF,
        // so anything switched to in front of them was covered. They are created owned by the master
        // window instead, which keeps them above the window they belong to and nothing else.
        constexpr DWORD STYLE_EX_SETTINGS_WINDOW = WS_EX_TOOLWINDOW;
        // Only ever passed to AdjustWindowRectEx, to size the frame around a client area. The window
        // itself is created in SettingsWindow's constructor, and WS_CLIPCHILDREN - which this
        // function ignores - has to be set there to have any effect.
        constexpr DWORD STYLE_SETTINGS_WINDOW = WS_SYSMENU | WS_BORDER | WS_CLIPCHILDREN;
        constexpr DWORD STYLE_TOOLTIP = TTS_NOPREFIX | TTS_BALLOON | TTS_ALWAYSTIP;
        constexpr DWORD STYLE_LABEL = WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE | SS_NOTIFY;
        constexpr int SETTINGS_LABEL_LEFT_PADDING = 48;
        constexpr DWORD STYLE_RADIOBUTTON = WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_OWNERDRAW;
        constexpr DWORD STYLE_PUSHBUTTON = WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_OWNERDRAW;
        constexpr DWORD STYLE_CHECKBOX = WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_OWNERDRAW;
        // Single line and left-aligned to start with. ES_MULTILINE breaks a run of digits into a new
        // line every 1024 characters, which stops a deep-zoom coordinate scrolling horizontally at all,
        // and ES_RIGHT lays an over-wide line off the right of the field and never scrolls it either.
        // layoutEditText turns ES_RIGHT back on for values that do fit, where it behaves.
        constexpr DWORD STYLE_TEXT_FIELD = WS_CHILD | WS_TABSTOP | WS_VISIBLE | ES_AUTOHSCROLL;
        constexpr DWORD STYLE_TRACKBAR = WS_CHILD | WS_TABSTOP | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS;
        constexpr int SLIDER_RESOLUTION = 10000;
        // Owner-drawn and borderless: the panel paints the closed face and the dropped rows itself,
        // so they carry the same rounded face and theme colors as the rows around them.
        constexpr DWORD STYLE_COMBOBOX = WS_CHILD | WS_TABSTOP | WS_VISIBLE | CBS_DROPDOWNLIST |
                                         CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL;
        constexpr int ID_MENUS = 0x2000;
        constexpr int ID_OPTIONS = 0x1000;
        constexpr int ID_OPTIONS_RADIO = 0x0100;
        constexpr int ID_OPTIONS_CHECKBOX_FLAG = 0x0080;
        constexpr int ID_SECTION_TOGGLE = 0x4000;
        // Posted to the master window when the dark-mode flag has been flipped, so the frame, the
        // menu bar and the status bar are redressed after the flip rather than before it.
        constexpr UINT WM_MAIN_THEME_CHANGED = WM_APP + 1;
        // Drives the canvas while a menu is up. The menu runs a modal loop of its own, so the main
        // loop - and with it every present - stops for as long as the menu is open; a timer is the
        // one thing that still reaches the master window from inside that loop.
        constexpr UINT_PTR TIMER_MENU_LOOP_RENDER = 1;
        // Roughly one frame at 60 Hz. The menu's loop delivers timer messages at its own pace, so
        // this is a ceiling on how fresh the canvas is kept, not a promise of a rate.
        constexpr UINT TIMER_MENU_LOOP_RENDER_INTERVAL = 16;
        // Selected UI colors use Tailwind CSS v3's MIT-licensed palette; see NOTICE.
        constexpr COLORREF COLOR_PROGRESS_BACKGROUND_PROG = RGB(37, 99, 235);
        constexpr COLORREF COLOR_PROGRESS_BACKGROUND_BACK = RGB(15, 23, 42);
        constexpr COLORREF COLOR_PROGRESS_TEXT_PROG = RGB(255, 255, 255);
        constexpr COLORREF COLOR_PROGRESS_TEXT_BACK = RGB(255, 255, 255);
        constexpr COLORREF COLOR_TEXT_ERROR = RGB(255, 0, 0);
        constexpr COLORREF COLOR_TEXT_EDITED = RGB(146, 64, 14);
        constexpr COLORREF COLOR_TEXT_MODIFIED = RGB(3, 105, 161);
        constexpr COLORREF COLOR_TEXT_DEFAULT = RGB(30, 41, 59);
        constexpr COLORREF COLOR_TEXT_DISABLED = RGB(148, 163, 184);
        constexpr COLORREF COLOR_LABEL_BACKGROUND = RGB(248, 250, 252);
        constexpr COLORREF COLOR_WINDOW_BACKGROUND = RGB(248, 250, 252);
        constexpr COLORREF COLOR_TOOLTIP_BACKGROUND = RGB(248, 250, 252);
        constexpr COLORREF COLOR_TOOLTIP_TEXT = RGB(15, 23, 42);
        constexpr COLORREF COLOR_CHECKBOX_CHECKED_BACKGROUND = RGB(37, 99, 235);
        constexpr COLORREF COLOR_CHECKBOX_BORDER = RGB(148, 163, 184);
        constexpr COLORREF COLOR_CHECKBOX_MARK = RGB(255, 255, 255);
        // A disabled tick used to be drawn in the disabled text grey on the near-white disabled face,
        // which left it at about 1.5:1 - not readable as a mark at all. Filling the box instead and
        // keeping the tick white mirrors the enabled state and lands near its own contrast (~2.5:1).
        constexpr COLORREF COLOR_CHECKBOX_DISABLED_CHECKED = RGB(100, 116, 139);
        // Every other control face is rounded; the checkbox was the one square element. Its own
        // radius, because the button radius is most of the way to a circle on a box this small.
        constexpr int CHECKBOX_CORNER_RADIUS = 6;

        // Section grouping: a bold group title over a thin light-gray frame drawn around the
        // rows below it, so each category of settings reads as one card. This replaced a plain
        // horizontal rule above the title - with the frame in place the rule doubled up with the
        // bottom edge of the card above it.
        constexpr COLORREF COLOR_SECTION_FRAME = RGB(226, 232, 240); // #E2E8F0
        constexpr int SECTION_FRAME_THICKNESS = 1;
        constexpr int FONT_SIZE_SECTION_HEADER = 31;
        // Drives the trackbar thumb: a horizontal trackbar sizes its thumb from the control
        // height, so a short bar leaves a thumb too narrow to grab comfortably.
        constexpr int SETTINGS_SLIDER_HEIGHT = 26;

        // Owner-drawn slider face: a thin rounded track whose travelled part is filled in the accent
        // color, with a round thumb riding on it, in place of the trackbar's own gray groove and
        // pointer (the one part of the panel still drawn in the OS's classic control style).
        constexpr int SLIDER_TRACK_THICKNESS = 6;
        constexpr int SLIDER_THUMB_DIAMETER = 20;
        // GDI draws no antialiased curves, so the bar is rendered at this multiple of its size and
        // shrunk with a halftone blit; the thumb's outline is visibly stepped drawn at 1:1.
        constexpr int SLIDER_SUPERSAMPLE = 4;
        constexpr COLORREF COLOR_SLIDER_TRACK = RGB(226, 232, 240);
        constexpr COLORREF COLOR_SLIDER_FILL = RGB(37, 99, 235);
        constexpr COLORREF COLOR_SLIDER_THUMB_FACE = RGB(255, 255, 255);
        constexpr COLORREF COLOR_SLIDER_THUMB_BORDER = RGB(203, 213, 225);
        constexpr COLORREF COLOR_SLIDER_TRACK_DISABLED = RGB(241, 245, 249);
        constexpr COLORREF COLOR_SLIDER_DISABLED = RGB(203, 213, 225);

        constexpr int FONT_SIZE_RANGE_LABEL = 16;
        constexpr COLORREF COLOR_RANGE_LABEL = RGB(100, 116, 139);
        constexpr COLORREF COLOR_PREVIEW_BORDER = RGB(148, 163, 184);
        constexpr COLORREF COLOR_PREVIEW_BORDER_SELECTED = RGB(37, 99, 235);
        constexpr COLORREF COLOR_RADIO_SELECTED_BG = RGB(239, 246, 255);
        constexpr COLORREF COLOR_RADIO_SELECTED_BORDER = RGB(147, 197, 253);

        // Soft, near-white owner-drawn push / color buttons with rounded corners.
        constexpr int BUTTON_CORNER_RADIUS = 12;

        // Filled accent ("primary") owner-drawn button, e.g. the export action button.
        constexpr int PRIMARY_BUTTON_HEIGHT = 44;
        constexpr COLORREF COLOR_PRIMARY_BUTTON = RGB(37, 99, 235);          // #2563EB
        constexpr COLORREF COLOR_PRIMARY_BUTTON_PRESSED = RGB(29, 78, 216);  // #1D4ED8
        constexpr COLORREF COLOR_PRIMARY_BUTTON_TEXT = RGB(255, 255, 255);

        // Rounded, lightly-tinted "card" panel (export dialog notes).
        constexpr int CARD_CORNER_RADIUS = 12;
        // Notes (info) card: light blue tint + blue accent.
        constexpr COLORREF COLOR_CARD_NOTE_BG = RGB(239, 246, 255);
        constexpr COLORREF COLOR_CARD_NOTE_BORDER = RGB(191, 219, 254);
        constexpr COLORREF COLOR_CARD_NOTE_ACCENT = RGB(37, 99, 235);
        constexpr COLORREF COLOR_CARD_NOTE_TITLE = RGB(15, 23, 42);
        // Light fill behind the (borderless, rounded) numeric text fields so they read as
        // input boxes against the white settings panel.
        constexpr COLORREF COLOR_TEXT_FIELD_BACKGROUND = RGB(255, 255, 255);
        // Soft border drawn around the rounded text fields so they read clearly against white.
        constexpr COLORREF COLOR_TEXT_FIELD_BORDER = RGB(203, 213, 225);
        constexpr COLORREF COLOR_BUTTON_FACE = RGB(255, 255, 255);
        constexpr COLORREF COLOR_BUTTON_FACE_PRESSED = RGB(226, 232, 240);
        constexpr COLORREF COLOR_CONTROL_DISABLED_FACE = RGB(241, 245, 249);
        constexpr COLORREF COLOR_BUTTON_BORDER = RGB(203, 213, 225);

        // ---- Settings-window UI scaling ------------
        constexpr double SETTINGS_UI_BASE_SCALE = 0.81;

        inline double initialWindowScale() {
            static const double scale = [] {
                const double minimumScale = std::max(
                    static_cast<double>(MIN_WINDOW_WIDTH) / INIT_RENDER_SCENE_WIDTH,
                    static_cast<double>(MIN_WINDOW_HEIGHT) / INIT_RENDER_SCENE_HEIGHT);
                const double displayScale = std::min(
                    static_cast<double>(GetSystemMetrics(SM_CXSCREEN)) / BASELINE_DISPLAY_WIDTH,
                    static_cast<double>(GetSystemMetrics(SM_CYSCREEN)) / BASELINE_DISPLAY_HEIGHT);
                return std::max(minimumScale, displayScale);
            }();
            return scale;
        }

        inline double settingsUiScale() {
            static const double scale = [] {
                const HDC hdc = GetDC(nullptr);
                const int dpi = hdc ? GetDeviceCaps(hdc, LOGPIXELSX) : 96;
                if (hdc) {
                    ReleaseDC(nullptr, hdc);
                }
                return SETTINGS_UI_BASE_SCALE * static_cast<double>(dpi) / 96.0;
            }();
            return scale;
        }

        inline int settingsWindowScaled(const int designPx) {
            if (designPx <= 0) {
                return designPx;
            }
            const int v = static_cast<int>(designPx * SETTINGS_UI_BASE_SCALE * initialWindowScale() + 0.5);
            return v < 1 ? 1 : v;
        }

        // Scale a settings-window design pixel/font value. Rounds to the nearest pixel and
        // never returns 0 for a positive input, so 1px lines (dividers) survive scaling.
        inline int settingsScaled(const int designPx) {
            if (designPx <= 0) {
                return designPx;
            }
            const int v = static_cast<int>(designPx * settingsUiScale() + 0.5);
            return v < 1 ? 1 : v;
        }

        // The UI face every window draws with. FONT_PREFERRED is asked for once and kept only if
        // the mapper really gave it back; GetTextFace reports the substituted face, not the
        // requested one, so a system without it falls back to Segoe UI rather than to whatever
        // the mapper picked on its own.
        inline const wchar_t *uiFontFace() {
            static const wchar_t *face = []() -> const wchar_t * {
                const HDC hdc = GetDC(nullptr);
                if (!hdc) {
                    return FONT_FALLBACK;
                }
                const HFONT probe = CreateFontW(settingsScaled(FONT_SIZE), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, FONT_PREFERRED);
                wchar_t resolved[LF_FACESIZE] = {};
                const HGDIOBJ previous = SelectObject(hdc, probe);
                GetTextFaceW(hdc, LF_FACESIZE, resolved);
                SelectObject(hdc, previous);
                DeleteObject(probe);
                ReleaseDC(nullptr, hdc);
                return std::wcscmp(resolved, FONT_PREFERRED) == 0 ? FONT_PREFERRED : FONT_FALLBACK;
            }();
            return face;
        }

        // Process-lifetime font for the video progress bar, which paints its own text and so carried
        // no font at all - GDI drew that readout in the stock "System" bitmap face. Never destroyed:
        // it stays in use for as long as that window exists. The status bar deliberately does not
        // use this; it keeps the face Windows gives it, which is how it has always looked.
        inline HFONT sharedUiFont() {
            static const HFONT font = CreateFontW(settingsScaled(FONT_SIZE), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, uiFontFace());
            return font;
        }

        // Shift+drag box-zoom rubber-band overlay
        constexpr DWORD STYLE_BOX_ZOOM_OVERLAY = WS_POPUP;
        constexpr DWORD STYLE_EX_BOX_ZOOM_OVERLAY = WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW |
                                                    WS_EX_NOACTIVATE | WS_EX_TOPMOST;
        constexpr COLORREF COLOR_BOX_ZOOM_OVERLAY = RGB(0, 200, 255);
        // White halo drawn on both sides of the cyan line so the box stays visible
        // against any fractal background (dark, bright, or cyan-ish).
        constexpr COLORREF COLOR_BOX_ZOOM_OUTLINE = RGB(255, 255, 255);
        constexpr BYTE BOX_ZOOM_OVERLAY_ALPHA = 180;
        constexpr int BOX_ZOOM_BORDER_THICKNESS = 3;       // cyan line thickness
        constexpr int BOX_ZOOM_OUTLINE_THICKNESS = 2;      // white halo thickness on each side
        constexpr int BOX_ZOOM_MIN_DRAG_PIXELS = 6;

        // Maps the up / down arrow keys pass over at a time while a folder of maps is walked.
        constexpr int MAP_BROWSE_COARSE_STEP = 10;
}
