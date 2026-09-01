//
// Created by Merutilm on 2025-05-13.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21, 2026-08-23, 2026-08-26.
// Modified by Opus 5 on 2026-08-06, 2026-08-11, 2026-08-12, 2026-08-13, 2026-08-14, 2026-08-23, 2026-08-26, 2026-08-27, 2026-08-31, 2026-09-01
//

#include "SettingsWindow.hpp"
#include "../constants/Constants.hpp"

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <optional>
#include <stdexcept>


namespace merutilm::rff2 {
    // Posted once from the constructor: it lands only after the caller finishes registering every
    // row (the build runs without pumping the queue), so the panel is shown fully formed in one
    // shot instead of growing row by row, and its composited surface is settled before any input.
    static constexpr UINT WM_SW_FINALIZE = WM_APP + 1;
    // Deferred "the panel's surface changed": posted by schedulePanelRepaint, handled once the
    // caller's whole batch of row changes has been applied. See schedulePanelRepaint.
    static constexpr UINT WM_SW_FLUSH_PAINT = WM_APP + 2;
    static constexpr UINT WM_SW_THEME_CHANGED = WM_APP + 3;

    // Shorthand for the settings-window DPI + shrink scale applied to every design pixel /
    // font size in this file (see Constants::Win32::settingsScaled).
    static int sc(const int designPx) {
        return Constants::Win32::settingsScaled(designPx);
    }

    static RECT getWindowWorkArea(const HWND window) {
        MONITORINFO monitorInfo = {sizeof(monitorInfo)};
        if (GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitorInfo)) {
            return monitorInfo.rcWork;
        }
        RECT workArea;
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, FALSE);
        return workArea;
    }

    static int resolveSettingsWindowLeft(const int rightSide, const int masterLeft, const int masterRight,
                                         const int outerWidth, const RECT &workArea) {
        const int workCenter = workArea.left + (workArea.right - workArea.left) / 2;
        const int masterCenter = masterLeft + (masterRight - masterLeft) / 2;
        const int preferredLeft = masterCenter > workCenter ? masterLeft - outerWidth : rightSide;
        const int maxLeft = std::max(static_cast<int>(workArea.left),
                                     static_cast<int>(workArea.right) - outerWidth);
        return std::clamp(preferredLeft, static_cast<int>(workArea.left), maxLeft);
    }

    // Work-area room kept free beside the panel's height: the frame it gains over its client area
    // plus a small gap, so its bottom edge never sits flush against the taskbar.
    static constexpr int SETTINGS_VERTICAL_MARGIN = 60;

    // The tallest client area a panel may take. It fills the work area below the edge it snaps to,
    // as before, but is never held below the master window's own height: the panel may float up
    // from that edge, so a master window sitting near the bottom of the screen no longer squeezes
    // every panel down to the sliver of space left under it. The work area is the hard ceiling.
    static int maxSettingsClientHeight(const RECT &workArea, const int snapTop, const int masterHeight,
                                       const int inputHeight) {
        const int availBelow = static_cast<int>(workArea.bottom) - snapTop;
        const int workHeight = static_cast<int>(workArea.bottom - workArea.top);
        return std::max(inputHeight, std::min(workHeight, std::max(availBelow, masterHeight)) -
                                     SETTINGS_VERTICAL_MARGIN);
    }

    // Vertical twin of resolveSettingsWindowLeft: prefer the snap edge, and slide up by however far
    // the panel would otherwise hang past the bottom of the work area.
    static int resolveSettingsWindowTop(const int snapTop, const int outerHeight, const RECT &workArea) {
        const int maxTop = std::max(static_cast<int>(workArea.top),
                                    static_cast<int>(workArea.bottom) - outerHeight);
        return std::clamp(snapTop, static_cast<int>(workArea.top), maxTop);
    }

    struct SettingsThemeBrushes {
        HBRUSH background = nullptr;
        HBRUSH checked = nullptr;
        HBRUSH textField = nullptr;
        HBRUSH disabledControl = nullptr;

        void ensure(const SettingsThemeColors &theme) {
            if (background != nullptr) {
                return;
            }
            background = CreateSolidBrush(theme.background);
            checked = CreateSolidBrush(theme.checkboxChecked);
            textField = CreateSolidBrush(theme.textFieldBackground);
            disabledControl = CreateSolidBrush(theme.controlDisabledFace);
        }
    };

    // One set per mode rather than one rebuilt as the mode flips: both are live at once now that a
    // panel opened from the Timeline Editor is drawn in its own colors beside panels in the app's.
    static SettingsThemeBrushes &themeBrushes() {
        static SettingsThemeBrushes brushes[2];
        SettingsThemeBrushes &set = brushes[darkSettingsMode() ? 1 : 0];
        set.ensure(settingsTheme());
        return set;
    }

    static HBRUSH windowBackgroundBrush() { return themeBrushes().background; }

    static HBRUSH checkedCheckboxBrush() { return themeBrushes().checked; }

    static HBRUSH textFieldBrush() { return themeBrushes().textField; }

    static HBRUSH disabledControlBrush() { return themeBrushes().disabledControl; }

    // The panel window itself keeps the native dark class: its scrollbar is the one piece of it
    // comctl32 still draws, and applyDarkWindowFrame recolors the title bar it wears.
    static void applyNativeSettingsWindowTheme(const HWND window) {
        applyDarkWindowFrame(window);
        applyDarkThemeClass(window, false);
    }

    // Paints a soft, rounded near-white button background into rc.
    static void drawRoundedButtonFace(const HDC hdc, const RECT &rc, const bool pressed, const bool focused,
                                      const bool enabled = true) {
        FillRect(hdc, &rc, windowBackgroundBrush());
        const HPEN pen = CreatePen(PS_SOLID, 1, settingsTheme().buttonBorder);
        const HBRUSH brush = CreateSolidBrush(!enabled
                                                  ? settingsTheme().controlDisabledFace
                                                  : pressed
                                                      ? settingsTheme().buttonFacePressed
                                                      : settingsTheme().buttonFace);
        const auto oldPen = SelectObject(hdc, pen);
        const auto oldBrush = SelectObject(hdc, brush);
        const int r = sc(Constants::Win32::BUTTON_CORNER_RADIUS);
        // The bounding box is right/bottom exclusive and GDI already draws the outline inside it,
        // so the exact rect paints every row and column of the control. Passing right-1/bottom-1
        // shrank the face a second time, leaving its last row unpainted: everything centred on the
        // control (a button's label, the colour swatch) then sat half a pixel below what you see.
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, r, r);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(pen);
        DeleteObject(brush);
        if (focused) {
            RECT f = rc;
            InflateRect(&f, -sc(4), -sc(4));
            DrawFocusRect(hdc, &f);
        }
    }

    // Paints a filled blue accent ("primary") button face into rc.
    static void drawPrimaryButtonFace(const HDC hdc, const RECT &rc, const bool pressed, const bool focused) {
        FillRect(hdc, &rc, windowBackgroundBrush());
        const COLORREF c = pressed
                               ? settingsTheme().primaryButtonPressed
                               : settingsTheme().primaryButton;
        const HPEN pen = CreatePen(PS_SOLID, 1, c);
        const HBRUSH brush = CreateSolidBrush(c);
        const auto oldPen = SelectObject(hdc, pen);
        const auto oldBrush = SelectObject(hdc, brush);
        const int r = sc(Constants::Win32::BUTTON_CORNER_RADIUS);
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, r, r);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(pen);
        DeleteObject(brush);
        if (focused) {
            RECT f = rc;
            InflateRect(&f, -sc(4), -sc(4));
            DrawFocusRect(hdc, &f);
        }
    }

    // The closed face of a dropdown: the rounded button face every other value control carries, the
    // chosen value on it, and a small arrow at the right in place of the theme's own. rc is the
    // control's whole client rect wherever this is called from, so the two paths that draw the face
    // - the control's paint and the edit item the control asks its owner for - put down the same
    // pixels, and the second cannot leave part of the first behind.
    static void paintSelectionBoxFace(const HWND box, const HDC hdc, const RECT &rc, const HFONT font) {
        const bool enabled = IsWindowEnabled(box);
        const bool dropped = SendMessageW(box, CB_GETDROPPEDSTATE, 0, 0) != 0;
        // No focus frame: the control takes the focus the moment it is pressed, so the dotted
        // rectangle came up under the pointer and stood there until the button was let go.
        drawRoundedButtonFace(hdc, rc, dropped && enabled, false, enabled);

        const int arrowSlot = sc(Constants::Win32::GAP_SETTINGS_INPUT);
        const COLORREF textColor = enabled ? settingsTheme().text : settingsTheme().textDisabled;
        if (const auto selected = static_cast<int>(SendMessageW(box, CB_GETCURSEL, 0, 0)); selected >= 0) {
            const int len = static_cast<int>(SendMessageW(box, CB_GETLBTEXTLEN, selected, 0));
            std::wstring text(std::max(len, 0) + 1, L'\0');
            SendMessageW(box, CB_GETLBTEXT, selected, reinterpret_cast<LPARAM>(text.data()));
            RECT textRc = {rc.left + sc(10), rc.top, rc.right - arrowSlot, rc.bottom};
            const auto oldFont = SelectObject(hdc, font);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, textColor);
            DrawTextW(hdc, text.c_str(), -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            SelectObject(hdc, oldFont);
        }

        // A small solid triangle in the label's color, so it dims with a row that is switched off.
        const int cx = rc.right - arrowSlot / 2;
        const int cy = (rc.top + rc.bottom) / 2;
        const int a = std::max(sc(4), 3);
        POINT arrow[3] = {{cx - a, cy - a / 2}, {cx + a, cy - a / 2}, {cx, cy + a}};
        const HPEN arrowPen = CreatePen(PS_SOLID, 1, textColor);
        const HBRUSH arrowBrush = CreateSolidBrush(textColor);
        const auto oldPen = SelectObject(hdc, arrowPen);
        const auto oldBrush = SelectObject(hdc, arrowBrush);
        Polygon(hdc, arrow, 3);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(arrowBrush);
        DeleteObject(arrowPen);
    }

    // Fills rc with a rounded, lightly-tinted card face + 1px border.
    static void drawCardFace(const HDC hdc, const RECT &rc, const COLORREF fill, const COLORREF border) {
        FillRect(hdc, &rc, windowBackgroundBrush());
        const HPEN pen = CreatePen(PS_SOLID, 1, border);
        const HBRUSH brush = CreateSolidBrush(fill);
        const auto oldPen = SelectObject(hdc, pen);
        const auto oldBrush = SelectObject(hdc, brush);
        const int r = sc(Constants::Win32::CARD_CORNER_RADIUS);
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, r, r);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(pen);
        DeleteObject(brush);
    }

    // Filled blue info circle with a centered white "i" glyph.
    static void drawInfoIcon(const HDC hdc, const RECT &rc, const HFONT glyphFont, const COLORREF color) {
        const HPEN pen = CreatePen(PS_SOLID, 1, color);
        const HBRUSH brush = CreateSolidBrush(color);
        const auto oldPen = SelectObject(hdc, pen);
        const auto oldBrush = SelectObject(hdc, brush);
        Ellipse(hdc, rc.left, rc.top, rc.right, rc.bottom);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(pen);
        DeleteObject(brush);
        const auto oldFont = SelectObject(hdc, glyphFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, settingsTheme().primaryButtonText);
        RECT tr = rc;
        DrawTextW(hdc, L"i", -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, oldFont);
    }

    // Bar of 2*half thickness centered on cy, running x0..x1 with fully rounded caps.
    static void fillPill(const HDC hdc, const int x0, const int x1, const int cy, const int half,
                         const COLORREF color) {
        if (x1 <= x0 || half <= 0) {
            return;
        }
        const HPEN pen = CreatePen(PS_SOLID, 1, color);
        const HBRUSH brush = CreateSolidBrush(color);
        const auto oldPen = SelectObject(hdc, pen);
        const auto oldBrush = SelectObject(hdc, brush);
        RoundRect(hdc, x0, cy - half, x1, cy + half, half * 2, half * 2);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(pen);
        DeleteObject(brush);
    }

    // Circle of radius r centered on (cx, cy), outlined in `border` at `penWidth`.
    static void fillDisc(const HDC hdc, const int cx, const int cy, const int r, const COLORREF fill,
                         const COLORREF border, const int penWidth) {
        if (r <= 0) {
            return;
        }
        const HPEN pen = CreatePen(PS_SOLID, penWidth, border);
        const HBRUSH brush = CreateSolidBrush(fill);
        const auto oldPen = SelectObject(hdc, pen);
        const auto oldBrush = SelectObject(hdc, brush);
        Ellipse(hdc, cx - r, cy - r, cx + r, cy + r);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(pen);
        DeleteObject(brush);
    }

    void SettingsWindow::applyRoundedRegion(const HWND control) {
        RECT rc;
        GetClientRect(control, &rc);
        const int w = rc.right - rc.left;
        const int h = rc.bottom - rc.top;
        if (w <= 0 || h <= 0) {
            return;
        }
        const int r = sc(Constants::Win32::BUTTON_CORNER_RADIUS);
        // CreateRoundRectRgn's right/bottom are exclusive, so add 1 to cover the last pixel.
        // The system takes ownership of the region (bRedraw = TRUE).
        SetWindowRgn(control, CreateRoundRectRgn(0, 0, w + 1, h + 1, r, r), TRUE);
    }

    // The dark app theme class belongs only on the controls this panel does NOT paint: the combo
    // boxes and the tooltips. Every other control here is owner-drawn, custom-drawn or colored
    // through WM_CTLCOLOR*, and the dark class left open on one of those puts comctl32 back in the
    // drawing path - which is what made a themed static stop erasing behind its own text and leave
    // the previous line showing under the new one (row labels reading doubled in dark mode). Those
    // are taken off the visual styles in dark mode and put back on the default class in light,
    // where nothing has ever gone wrong with them.
    void SettingsWindow::applyNativeControlTheme(const HWND control) {
        wchar_t className[64] = {};
        GetClassNameW(control, className, 64);
        if (std::wcscmp(className, WC_COMBOBOXW) == 0) {
            applyDarkThemeClass(control, true);
        } else if (std::wcscmp(className, TOOLTIPS_CLASSW) == 0) {
            applyDarkThemeClass(control, false);
        } else if (darkSettingsMode()) {
            disableThemeClass(control);
        } else {
            applyDarkThemeClass(control, false);
        }
    }

    // Reads a control's text without assuming a length bound.
    static std::wstring windowText(const HWND wnd) {
        const int len = GetWindowTextLengthW(wnd);
        std::wstring buf(len + 1, L'\0');
        GetWindowTextW(wnd, buf.data(), len + 1);
        buf.resize(len);
        return buf;
    }

    // Width of `text` in `edit`'s own font.
    static int editTextWidth(const HWND edit, const std::wstring &text) {
        if (text.empty()) {
            return 0;
        }
        const HDC dc = GetDC(edit);
        const auto old = SelectObject(dc, reinterpret_cast<HFONT>(SendMessageW(edit, WM_GETFONT, 0, 0)));
        SIZE sz;
        GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &sz);
        SelectObject(dc, old);
        ReleaseDC(edit, dc);
        return sz.cx;
    }

    // The y of the field's single text line, centered in the client area. The control is a
    // single-line edit, which always lays its line against the top, so both the text we draw and
    // the caret we reposition come from here instead.
    int SettingsWindow::editTextTop(const HWND edit, const TEXTMETRICW &tm) {
        RECT rc;
        GetClientRect(edit, &rc);
        return std::max(0, (static_cast<int>(rc.bottom - rc.top) - static_cast<int>(tm.tmHeight)) / 2);
    }

    // Keep the field's alignment matched to its value: ES_RIGHT for a value that fits, cleared for
    // one that does not. ES_RIGHT lays an over-wide line off the right of the field and never
    // scrolls it, so leaving it on is what hid a long coordinate behind its own leading sign; ES_LEFT
    // scrolls such a value normally. The style takes effect on the spot, so the control's caret,
    // selection and hit testing stay native in both modes.
    // The single-line control parks its caret against the top of the client area; lift it onto the
    // centered line the text is drawn on. Never call this from inside WM_PAINT: BeginPaint hides the
    // caret and EndPaint restores it, and moving it in between smeared the glyphs it sat over, which
    // is what made the value look like it was shaking vertically.
    // Hand a message to the control with painting switched off, then re-align the field, put the
    // caret back on the drawn line and queue one repaint of the whole thing. Measured: an unsuppressed
    // scroll leaves less than half the client untouched, the rest having been blitted by the control.
    LRESULT SettingsWindow::relayoutQuietly(const HWND edit, const UINT message, const WPARAM wParam,
                                            const LPARAM lParam) {
        SendMessageW(edit, WM_SETREDRAW, FALSE, 0);
        const LRESULT res = DefSubclassProc(edit, message, wParam, lParam);
        SendMessageW(edit, WM_SETREDRAW, TRUE, 0);
        layoutEditText(edit);
        InvalidateRect(edit, nullptr, FALSE);
        return res;
    }

    void SettingsWindow::layoutEditText(const HWND edit) {
        RECT fr;
        SendMessageW(edit, EM_GETRECT, 0, reinterpret_cast<LPARAM>(&fr));
        const bool fits = editTextWidth(edit, windowText(edit)) <= fr.right - fr.left;
        const auto style = static_cast<DWORD>(GetWindowLongPtrW(edit, GWL_STYLE));
        if (const DWORD wanted = fits ? style | ES_RIGHT : style & ~ES_RIGHT; wanted != style) {
            SetWindowLongPtrW(edit, GWL_STYLE, wanted);
            InvalidateRect(edit, nullptr, FALSE);
        }
    }

    SettingsWindow::SettingsWindow(const std::wstring &name, const int width, const int labelWidth,
                                   const int inputHeight)
        : windowWidth(Constants::Win32::settingsWindowScaled(width)),
          labelWidth(labelWidth > 0 ? Constants::Win32::settingsWindowScaled(labelWidth) : labelWidth),
          inputHeight(sc(inputHeight)) {
        // Ensure the trackbar (slider) window class is registered before any
        // SettingsWindow creates one. Idempotent, so calling it per-window is fine.
        static const bool commonControlsReady = [] {
            INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_BAR_CLASSES};
            return InitCommonControlsEx(&icc) != FALSE;
        }();
        (void) commonControlsReady;
        int fractalRight; // RFF render area's right edge, in screen coordinates
        // The panel's owner (see the CreateWindowExW below). Null only if RFF's own window cannot be
        // found, which leaves the panel unowned - the same window it used to be.
        const HWND master = FindWindowW(Constants::Win32::CLASS_MASTER_WINDOW, nullptr);
        {
            RECT cr;
            RECT masterRect;
            if (master && GetClientRect(master, &cr) && GetWindowRect(master, &masterRect)) {
                POINT topLeft = {cr.left, cr.top};
                POINT topRight = {cr.right, cr.top};
                ClientToScreen(master, &topLeft);
                ClientToScreen(master, &topRight);
                masterLeft = topLeft.x;
                fractalRight = topRight.x;
                masterRight = topRight.x;
                masterHeight = cr.bottom - cr.top;
                snapTop = masterRect.top;
            } else {
                // Fallback: RFF is placed with its render area's left edge at x = 0.
                masterLeft = 0;
                fractalRight = Constants::Win32::INIT_RENDER_SCENE_WIDTH;
                masterRight = fractalRight;
                masterHeight = Constants::Win32::INIT_RENDER_SCENE_HEIGHT;
                snapTop = 0;
            }
            // Reserve the settings frame's non-client border + a vertical scrollbar (most panels
            // are tall enough to scroll) and fill the remaining width exactly.
            // A non-positive width explicitly opts into using the remaining screen width.
            if (width <= 0) {
                RECT nc = {0, 0, 0, 0};
                AdjustWindowRectEx(&nc, Constants::Win32::STYLE_SETTINGS_WINDOW, FALSE,
                                   Constants::Win32::STYLE_EX_SETTINGS_WINDOW);
                const int border = nc.right - nc.left;
                const int availOuter = GetSystemMetrics(SM_CXSCREEN) - fractalRight;
                windowWidth = std::max(200, availOuter - border - GetSystemMetrics(SM_CXVSCROLL));
            }
            snapLeft = fractalRight;
        }

        // WS_CLIPCHILDREN, and it has to be on the window that is actually created: the style
        // constant carrying it is only ever handed to AdjustWindowRectEx for frame metrics, which
        // ignores it, so the panel had been running without it. The rows are separate windows, and
        // without this the panel's own background fill goes straight over them - every repaint of
        // the surface wiped the whole panel blank and the rows came back one at a time afterwards.
        //
        // `master` is the owner, not a parent: the style carries no WS_CHILD, so this stays a
        // top-level window that merely rides above the window it belongs to. That is what replaces
        // the WS_EX_TOPMOST the panels used to carry, which pinned them over every other program on
        // the desktop.
        window = CreateWindowExW(Constants::Win32::STYLE_EX_SETTINGS_WINDOW, Constants::Win32::CLASS_SETTINGS_WINDOW,
                                 name.data(),
                                 WS_SYSMENU | WS_CLIPCHILDREN, snapLeft, snapTop,
                                 windowWidth, 0, master, nullptr, nullptr, nullptr);

        if (window) {
            RECT wr;
            POINT clientOrigin = {0, 0};
            GetWindowRect(window, &wr);
            ClientToScreen(window, &clientOrigin);
            const int leftFrame = static_cast<int>(clientOrigin.x - wr.left);
            snapLeft = fractalRight - leftFrame;
            const RECT workArea = getWindowWorkArea(window);
            const int outerWidth = static_cast<int>(wr.right - wr.left);
            const int windowLeft = resolveSettingsWindowLeft(
                snapLeft, masterLeft, masterRight, outerWidth, workArea);
            SetWindowPos(window, nullptr, windowLeft, snapTop, 0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        applyNativeSettingsWindowTheme(window);
        // Stay hidden while the caller registers rows (each one resizes/repaints the window);
        // WM_SW_FINALIZE reveals it once, fully built. See WM_SW_FINALIZE.
        PostMessageW(window, WM_SW_FINALIZE, 0, 0);
        font = reinterpret_cast<LPARAM>(CreateFontW(sc(Constants::Win32::FONT_SIZE), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                                    CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS,
                                                    Constants::Win32::uiFontFace()));
        // Bold variant for section header titles / primary button labels.
        headerFont = reinterpret_cast<LPARAM>(CreateFontW(sc(Constants::Win32::FONT_SIZE_SECTION_HEADER), 0, 0, 0, FW_BOLD,
                                                          FALSE, FALSE, FALSE,
                                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                                          CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                                          DEFAULT_PITCH | FF_SWISS,
                                                          Constants::Win32::uiFontFace()));
        // Small variant for the muted min/max range labels tucked under sliders.
        smallFont = reinterpret_cast<LPARAM>(CreateFontW(sc(Constants::Win32::FONT_SIZE_RANGE_LABEL), 0, 0, 0, FW_NORMAL,
                                                         FALSE, FALSE, FALSE,
                                                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                                         CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                                         DEFAULT_PITCH | FF_SWISS,
                                                         Constants::Win32::uiFontFace()));
    }

    SettingsWindow::~SettingsWindow() {
        if (IsWindow(window)) {
            DestroyWindow(window);
        }
    }

    void SettingsWindow::scheduleThemeRefresh() const {
        if (IsWindow(window)) {
            PostMessageW(window, WM_SW_THEME_CHANGED, 0, 0);
        }
    }

    void SettingsWindow::setDarkOverride(const bool dark) {
        darkOverride = dark;
        // The frame, the control classes and the tooltips were dressed under the View menu's flag.
        const ScopedSettingsMode themeScope = scopedMode(this);
        refreshTheme();
    }

    SettingsWindow *SettingsWindow::of(const HWND panelWindow) {
        if (!IsWindow(panelWindow)) {
            return nullptr;
        }
        wchar_t className[64] = {};
        GetClassNameW(panelWindow, className, 64);
        if (std::wcscmp(className, Constants::Win32::CLASS_SETTINGS_WINDOW) != 0) {
            return nullptr;
        }
        return reinterpret_cast<SettingsWindow *>(GetWindowLongPtrW(panelWindow, GWLP_USERDATA));
    }

    void SettingsWindow::refreshTheme() const {
        applyNativeSettingsWindowTheme(window);
        wchar_t className[64] = {};
        for (const HWND child : createdChildWindows) {
            if (!IsWindow(child)) {
                continue;
            }
            applyNativeControlTheme(child);
            GetClassNameW(child, className, 64);
            if (std::wcscmp(className, TOOLTIPS_CLASSW) == 0) {
                SendMessageW(child, TTM_SETTIPBKCOLOR, settingsTheme().tooltipBackground, 0);
                SendMessageW(child, TTM_SETTIPTEXTCOLOR, settingsTheme().tooltipText, 0);
            }
        }
        RedrawWindow(window, nullptr, nullptr,
                     RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }


    int SettingsWindow::getFixedNameWidth() const {
        if (labelWidth > 0) {
            return labelWidth;
        }
        RECT rect;
        GetClientRect(window, &rect);
        return (rect.right - rect.left) / Constants::Win32::SETTINGS_LABEL_WIDTH_DIVISOR;
    }

    int SettingsWindow::getFixedValueWidth() const {
        RECT rect;
        GetClientRect(window, &rect);
        return rect.right - rect.left - getFixedNameWidth() - sc(Constants::Win32::GAP_SETTINGS_INPUT);
    }

    void SettingsWindow::adjustWindowHeight() {
        contentHeight = getYOffset() + inputHeight + sc(Constants::Win32::GAP_SETTINGS_INPUT);
        applyContentSizing();
        // Once the panel grows tall enough to scroll, collapsing becomes worthwhile: latch it on
        // and reveal the disclosure arrows of every section registered so far.
        if (!sectionsCollapsible && contentHeight > viewportHeight) {
            sectionsCollapsible = true;
            for (const auto &s : sections) {
                ShowWindow(s.arrow, SW_SHOWNA);
            }
        }
    }

    void SettingsWindow::applyContentSizing() {
        resizeToContent(false);
        refreshScrollInfo();
    }

    void SettingsWindow::resizeToContent(const bool keepPosition) {
        const RECT wa = getWindowWorkArea(window);
        const int maxClientH = maxSettingsClientHeight(wa, snapTop, masterHeight, inputHeight);

        const bool needScroll = contentHeight > maxClientH;
        viewportHeight = needScroll ? maxClientH : contentHeight;

        // Toggle WS_VSCROLL to match. A style change must be committed with SWP_FRAMECHANGED.
        const LONG style = GetWindowLongW(window, GWL_STYLE);
        const bool hasScroll = (style & WS_VSCROLL) != 0;
        if (needScroll != hasScroll) {
            SetWindowLongW(window, GWL_STYLE, needScroll ? (style | WS_VSCROLL) : (style & ~WS_VSCROLL));
        }

        RECT rect = {0, 0, windowWidth, viewportHeight};
        AdjustWindowRectEx(&rect, Constants::Win32::STYLE_SETTINGS_WINDOW, FALSE,
                           Constants::Win32::STYLE_EX_SETTINGS_WINDOW);
        int outerW = rect.right - rect.left;
        const int outerH = rect.bottom - rect.top;
        if (needScroll) {
            outerW += GetSystemMetrics(SM_CXVSCROLL);
        }

        const UINT flags = SWP_NOZORDER | (needScroll != hasScroll ? SWP_FRAMECHANGED : 0);
        if (keepPosition) {
            // A section toggle only changes the height; leave the window where the user put it,
            // unless expanding one would now hang it past the bottom of the work area.
            RECT wr;
            GetWindowRect(window, &wr);
            const int windowTop = resolveSettingsWindowTop(static_cast<int>(wr.top), outerH, wa);
            if (windowTop == wr.top) {
                SetWindowPos(window, nullptr, 0, 0, outerW, outerH, flags | SWP_NOMOVE);
            } else {
                SetWindowPos(window, nullptr, static_cast<int>(wr.left), windowTop, outerW, outerH, flags);
            }
        } else {
            const int windowLeft = resolveSettingsWindowLeft(snapLeft, masterLeft, masterRight, outerW, wa);
            const int windowTop = resolveSettingsWindowTop(snapTop, outerH, wa);
            SetWindowPos(window, nullptr, windowLeft, windowTop, outerW, outerH, flags);
        }
    }

    void SettingsWindow::captureSectionLayout() {
        if (sectionLayoutCaptured) {
            return;
        }
        sectionLayoutCaptured = true;
        // At capture time nothing is collapsed, so the current content height is the full
        // expanded height and the last section's body runs to the very bottom.
        expandedContentHeight = contentHeight;
        if (!sections.empty() && sections.back().bodyEnd < 0) {
            sections.back().bodyEnd = expandedContentHeight;
        }
        // Record every laid-out child's content-space (pre-scroll) top. Tooltips float and must
        // never be moved or shown by the reflow, so skip them.
        wchar_t className[64];
        for (const HWND child : createdChildWindows) {
            GetClassNameW(child, className, 64);
            if (wcscmp(className, TOOLTIPS_CLASSW) == 0) {
                continue;
            }
            RECT r;
            GetWindowRect(child, &r);
            POINT p = {r.left, r.top};
            ScreenToClient(window, &p);
            childOriginalTop.emplace(child, p.y + scrollY);
        }
    }

    void SettingsWindow::relayoutSections(const int changedIndex) {
        captureSectionLayout();

        int collapsedTotal = 0;
        for (const auto &s : sections) {
            if (s.collapsed) {
                collapsedTotal += s.bodyEnd - s.bodyTop;
            }
        }
        contentHeight = expandedContentHeight - collapsedTotal;
        if (!sections.empty() && sections.back().collapsed) {
            contentHeight += inputHeight + 2 * sc(Constants::Win32::GAP_SETTINGS_INPUT);
        }

        // Work out the new viewport / clamped scroll for the new height before touching anything,
        // so we can pick the smooth path only when the scroll position stays put.
        const RECT wa = getWindowWorkArea(window);
        const int maxClientH = maxSettingsClientHeight(wa, snapTop, masterHeight, inputHeight);
        const int newViewport = contentHeight > maxClientH ? maxClientH : contentHeight;
        const int newScrollY = std::clamp(scrollY, 0, std::max(0, contentHeight - newViewport));
        const bool valid = changedIndex >= 0 && changedIndex < static_cast<int>(sections.size());

        // Smooth path: a single toggle shifts everything below the section uniformly by its body
        // height, so a ScrollWindowEx blit slides the on-screen rows (pixels + child windows)
        // without flicker; an absolute pass then fixes any rows the blit couldn't reach (scrolled
        // off-screen) and reveals/hides this section's own rows. Taken whenever the scroll offset
        // doesn't have to change (the common case).
        if (valid && newScrollY == scrollY) {
            const CollapsibleSection &cs = sections[changedIndex];
            const int delta = cs.bodyEnd - cs.bodyTop;
            int offsetAbove = 0; // collapsed bodies fully above this section (its own state aside)
            for (const auto &s : sections) {
                if (s.collapsed && s.bodyEnd <= cs.bodyTop) {
                    offsetAbove += s.bodyEnd - s.bodyTop;
                }
            }
            const int bodyTopWin = cs.bodyTop - offsetAbove - scrollY;
            RECT client;
            GetClientRect(window, &client);

            if (cs.collapsed) {
                // Hide this section's body rows so the blit fills their space with the rows below,
                // slide the region below the header up, then shrink the window.
                for (const HWND child : createdChildWindows) {
                    const auto it = childOriginalTop.find(child);
                    if (it != childOriginalTop.end() && it->second >= cs.bodyTop &&
                        it->second < cs.bodyEnd) {
                        ShowWindow(child, SW_HIDE);
                    }
                }
                RECT rc = {0, std::max(0, bodyTopWin), client.right, client.bottom};
                if (delta > 0 && rc.top < rc.bottom) {
                    ScrollWindowEx(window, 0, -delta, &rc, &rc, nullptr, nullptr,
                                   SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE);
                }
                resizeToContent(true);
            } else {
                // Grow first, then slide the region below the header down to open the gap.
                resizeToContent(true);
                GetClientRect(window, &client);
                RECT rc = {0, std::max(0, bodyTopWin), client.right, client.bottom};
                if (delta > 0 && rc.top < rc.bottom) {
                    ScrollWindowEx(window, 0, delta, &rc, &rc, nullptr, nullptr,
                                   SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE);
                }
            }
            // Correct anything the blit missed (off-screen rows) and place this section's body
            // rows; skipUnchanged makes the already-slid on-screen rows no-ops, so no extra flash.
            positionSectionChildren(true);
            refreshScrollInfo();
            // Flush the rows' own paints in the same frame as the panel's. The panel is
            // WS_CLIPCHILDREN, so the blit above carries the background and the section frames but
            // not a single pixel of any control: the children are moved by SW_SCROLLCHILDREN and
            // repaint themselves from scratch. UpdateWindow paints only this window, leaving them to
            // the next trip through the message loop - one frame of background sitting where the
            // rows should already be, which is the flash seen below a section as it folds. No
            // RDW_INVALIDATE: nothing extra is marked dirty, the pending paints are only brought
            // forward.
            RedrawWindow(window, nullptr, nullptr, RDW_UPDATENOW | RDW_ALLCHILDREN);
            return;
        }

        // Fallback (the scroll offset must change, e.g. collapsing near the very bottom): the shift
        // isn't uniform, so place every row at its absolute slot instead of blitting.
        //
        // Nothing here suppresses redrawing in between. WM_SETREDRAW only ever reached this window,
        // never its rows, so the rows went on painting themselves - at half-applied positions, with
        // the panel underneath frozen - and the RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN that
        // ended it then threw away every row's pixels and drew the panel again from nothing. That
        // full rebuild is the flash on folding a section near the bottom. Between the resize and the
        // repaint below no message is pumped, so no intermediate state can reach the screen anyway,
        // and DeferWindowPos applies the moves as one batch that carries each row's pixels with it.
        resizeToContent(true);
        scrollY = newScrollY;
        positionSectionChildren(false);
        refreshScrollInfo();
        repaintPanel();
    }

    void SettingsWindow::repaintPanel() const {
        // Mark the panel's own surface - background and section frames - and only then paint. The
        // first call deliberately leaves RDW_ALLCHILDREN off: adding it would mark every row dirty
        // as well and repaint the whole panel for what is usually one row's worth of change. The
        // second adds no dirt of its own; it paints what is already owed, the surface and the rows
        // together, so the panel and the rows standing on it are never a paint apart.
        RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE);
        RedrawWindow(window, nullptr, nullptr, RDW_UPDATENOW | RDW_ALLCHILDREN);
    }

    void SettingsWindow::flushPendingRepaint() {
        if (!repaintPending) {
            return;
        }
        repaintPending = false;
        repaintPanel();
    }

    void SettingsWindow::schedulePanelRepaint() {
        // Mark the surface dirty now, paint once, later. Callers that flip several rows in one go
        // (switching Animation Mode to Linear hides four of them) would otherwise force a full
        // synchronous repaint per row, each one showing a different half-applied layout - a burst
        // of frames that reads as a flash. The posted message is handled after the whole batch has
        // been applied, and a second row flipped before it arrives does not queue another.
        RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE);
        if (!repaintPending) {
            repaintPending = true;
            PostMessageW(window, WM_SW_FLUSH_PAINT, 0, 0);
        }
    }

    void SettingsWindow::positionSectionChildren(const bool skipUnchanged) {
        HDWP hdwp = BeginDeferWindowPos(static_cast<int>(childOriginalTop.size()));
        for (const HWND child : createdChildWindows) {
            const auto it = childOriginalTop.find(child);
            if (it == childOriginalTop.end()) {
                continue; // tooltip (or otherwise excluded)
            }
            const int top = it->second;
            bool folded = false;
            int offset = 0; // collapsed body height located strictly above this control
            for (const auto &s : sections) {
                if (!s.collapsed) {
                    continue;
                }
                if (top >= s.bodyEnd) {
                    offset += s.bodyEnd - s.bodyTop;
                } else if (top >= s.bodyTop) {
                    folded = true;
                    break;
                }
            }
            const bool wantVisible = !folded && !rowHidden.contains(child);
            const int target = top - offset - scrollY;
            RECT r;
            GetWindowRect(child, &r);
            POINT p = {r.left, r.top};
            ScreenToClient(window, &p);
            // Skip rows already in their final state (e.g. the ones the blit just slid) so the pass
            // only touches off-screen rows and this section's body, adding no redundant repaint.
            if (skipUnchanged && (IsWindowVisible(child) != FALSE) == wantVisible && p.y == target) {
                continue;
            }
            if (!hdwp) {
                continue;
            }
            // A hidden row is moved to its slot as well. Rows the caller hid (setRowVisible, e.g.
            // the Animation Shape parameters that only apply to one mode) used to keep the
            // coordinates they had when a section above them was folded, and then reappeared at
            // that stale position the moment the caller showed them again. The blit path happens
            // to drag hidden children along, which is why this only bit on the path that has to
            // move the scroll offset.
            hdwp = DeferWindowPos(hdwp, child, nullptr, p.x, target, 0, 0,
                                  (wantVisible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) |
                                  SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        if (hdwp) {
            EndDeferWindowPos(hdwp);
        }
    }

    void SettingsWindow::paintSectionFrames(const HDC hdc) const {
        if (sections.empty()) {
            return;
        }
        RECT client;
        GetClientRect(window, &client);
        const int gap = sc(Constants::Win32::GAP_SETTINGS_INPUT);
        // Breathing room between the frame and the rows it encloses. Half a gap on every side,
        // so the frame clears its rows without widening the space between two sections.
        const int inset = gap / 2;
        const int radius = sc(Constants::Win32::CARD_CORNER_RADIUS);

        const auto clientRectOf = [this](const HWND child) {
            RECT r = {};
            GetWindowRect(child, &r);
            POINT topLeft = {r.left, r.top};
            POINT bottomRight = {r.right, r.bottom};
            ScreenToClient(window, &topLeft);
            ScreenToClient(window, &bottomRight);
            return RECT{topLeft.x, topLeft.y, bottomRight.x, bottomRight.y};
        };

        std::vector<RECT> bands(sections.size(), RECT{0, 0, 0, 0});
        std::vector<bool> measured(sections.size(), false);
        // An owner-drawn panel paints a face of its own (the note cards, the frozen-color strip),
        // so a section holding nothing else is already enclosed - framing it too reads as a box
        // inside a box. Only sections with at least one ordinary row get a frame.
        std::vector<bool> framed(sections.size(), false);

        // Each frame is measured from where its controls actually sit right now rather than from
        // the stored layout, so it follows scrolling and folding without a second copy of that
        // bookkeeping. A band opens at its own heading - the title belongs to the card it names,
        // not to the space above it - and runs to the last row underneath.
        std::vector<int> headerBottom;
        headerBottom.reserve(sections.size());
        std::unordered_set<HWND> sectionChrome;
        for (size_t i = 0; i < sections.size(); ++i) {
            const RECT headerRect = clientRectOf(sections[i].header);
            headerBottom.push_back(headerRect.bottom);
            sectionChrome.insert(sections[i].header);
            sectionChrome.insert(sections[i].arrow);
            if (!sections[i].collapsed) {
                bands[i] = headerRect;
                measured[i] = true;
            }
        }

        wchar_t className[64];
        for (const HWND child : createdChildWindows) {
            if (sectionChrome.contains(child) || !IsWindowVisible(child)) {
                continue;
            }
            // Tooltips are popups of their own: their screen coordinates say nothing about
            // where the row they belong to sits (see captureSectionLayout).
            GetClassNameW(child, className, 64);
            if (wcscmp(className, TOOLTIPS_CLASSW) == 0) {
                continue;
            }
            const RECT r = clientRectOf(child);
            // The band a row belongs to is the last heading above it.
            int index = -1;
            for (size_t i = 0; i < sections.size(); ++i) {
                if (r.top >= headerBottom[i]) {
                    index = static_cast<int>(i);
                }
            }
            if (index < 0 || sections[index].collapsed) {
                continue;
            }
            if (!cardPainters.contains(child)) {
                framed[index] = true;
            }
            if (!measured[index]) {
                bands[index] = r;
                measured[index] = true;
                continue;
            }
            bands[index].top = std::min(bands[index].top, r.top);
            bands[index].bottom = std::max(bands[index].bottom, r.bottom);
        }

        const HPEN pen = CreatePen(PS_SOLID, sc(Constants::Win32::SECTION_FRAME_THICKNESS),
                                   settingsTheme().sectionFrame);
        const auto previousPen = SelectObject(hdc, pen);
        const auto previousBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        for (size_t i = 0; i < sections.size(); ++i) {
            if (!measured[i] || !framed[i]) {
                continue; // folded, empty, or already enclosed by a panel of its own
            }
            const int top = bands[i].top - inset;
            const int bottom = bands[i].bottom + inset;
            if (bottom < client.top || top > client.bottom) {
                continue;
            }
            RoundRect(hdc, client.left + inset, top, client.right - inset, bottom, radius, radius);
        }
        SelectObject(hdc, previousBrush);
        SelectObject(hdc, previousPen);
        DeleteObject(pen);
    }

    bool SettingsWindow::isControlFolded(const HWND control) const {
        if (!sectionLayoutCaptured) {
            return false;
        }
        const auto it = childOriginalTop.find(control);
        if (it == childOriginalTop.end()) {
            return false;
        }
        const int top = it->second;
        for (const auto &s : sections) {
            if (s.collapsed && top >= s.bodyTop && top < s.bodyEnd) {
                return true;
            }
        }
        return false;
    }

    void SettingsWindow::toggleSection(const int sectionIndex) {
        // Short panels don't expose the arrows, so headers there must not fold.
        if (!sectionsCollapsible) {
            return;
        }
        if (sectionIndex < 0 || sectionIndex >= static_cast<int>(sections.size())) {
            return;
        }
        CollapsibleSection &s = sections[sectionIndex];
        s.collapsed = !s.collapsed;
        // U+25B6 (right) when folded, U+25BC (down) when open.
        const wchar_t glyph[2] = {static_cast<wchar_t>(s.collapsed ? 0x25B6 : 0x25BC), 0};
        SetWindowTextW(s.arrow, glyph);
        relayoutSections(sectionIndex);
    }

    void SettingsWindow::refreshScrollInfo() {
        const int maxScroll = std::max(0, contentHeight - viewportHeight);
        if (scrollY > maxScroll) {
            scrollTo(maxScroll);
        }
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = 0;
        si.nMax = std::max(0, contentHeight - 1);
        si.nPage = static_cast<UINT>(viewportHeight);
        si.nPos = scrollY;
        SetScrollInfo(window, SB_VERT, &si, TRUE);
    }

    void SettingsWindow::scrollTo(int newY) {
        const int maxScroll = std::max(0, contentHeight - viewportHeight);
        newY = std::clamp(newY, 0, maxScroll);
        const int dy = scrollY - newY; // > 0: content shifts down (scrolling up)
        if (dy == 0) {
            return;
        }
        scrollY = newY;
        // Move every child control (and invalidate the exposed strip) in one shot.
        ScrollWindowEx(window, 0, dy, nullptr, nullptr, nullptr, nullptr,
                       SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE);
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask = SIF_POS;
        si.nPos = scrollY;
        SetScrollInfo(window, SB_VERT, &si, TRUE);
        // The rows repaint themselves after a scroll for the same reason they do after a fold
        // (see relayoutSections), so bring their paints forward into this frame too.
        RedrawWindow(window, nullptr, nullptr, RDW_UPDATENOW | RDW_ALLCHILDREN);
    }

    int SettingsWindow::getIndex(const HWND wnd) {
        const int id = GetDlgCtrlID(wnd);
        // Labels, dividers, trackbars and tooltips are created without a control id, and GetDlgCtrlID
        // reports 0 for them, which the arithmetic below maps onto row 0. Every sweep over
        // createdChildWindows looking for "the controls of row 0" then matched all of them.
        if (id < Constants::Win32::ID_OPTIONS || id >= Constants::Win32::ID_SECTION_TOGGLE) {
            return -1;
        }
        return id - Constants::Win32::ID_OPTIONS & 0x007f;
    }

    bool SettingsWindow::isCheckbox(const HWND wnd) {
        return GetDlgCtrlID(wnd) & Constants::Win32::ID_OPTIONS_CHECKBOX_FLAG;
    }


    int SettingsWindow::getRadioIndex(const HWND wnd) {
        const int offset = GetDlgCtrlID(wnd) - Constants::Win32::ID_OPTIONS;
        return offset / Constants::Win32::ID_OPTIONS_RADIO;
    }

    int SettingsWindow::rowBoxSize() const {
        int d = sc(Constants::Win32::SETTINGS_CHECKBOX_SIZE);
        // Nudge the box so that (inputHeight - d) is even. Centring is (inputHeight - d) / 2, and an
        // odd leftover has to put the spare pixel on one side, which left every checkbox and radio
        // circle half a pixel off the row's centre (6px above, 7px below at 96 dpi).
        if (((inputHeight - d) & 1) != 0) {
            ++d;
        }
        return d;
    }

    int SettingsWindow::getRadioButtonWidth(const std::wstring &text, const int maxWidth) const {
        const HDC hdc = GetDC(window);
        const auto oldFont = SelectObject(hdc, reinterpret_cast<HFONT>(font));
        SIZE textSize = {};
        GetTextExtentPoint32W(hdc, text.data(), static_cast<int>(text.size()), &textSize);
        SelectObject(hdc, oldFont);
        ReleaseDC(window, hdc);
        const int d = rowBoxSize();
        const int desired = sc(8) + d + sc(8) + textSize.cx + sc(12);
        return std::min(maxWidth, std::max(inputHeight, desired));
    }

    bool SettingsWindow::checkIndex(const int index) const {
        return index >= 0 && index < callbacks.size();
    }

    int SettingsWindow::getYOffset() const {
        return yCursor + topMargin;
    }

    void SettingsWindow::advanceRow() {
        yCursor += inputHeight + sc(Constants::Win32::GAP_SETTINGS_INPUT) + sc(extraRowGap);
    }

    void SettingsWindow::advancePixels(const int usedHeight) {
        yCursor += usedHeight + sc(Constants::Win32::GAP_SETTINGS_INPUT) + sc(extraRowGap);
    }

    void SettingsWindow::setExtraRowGap(const int extraPx) {
        extraRowGap = extraPx;
    }

    HFONT SettingsWindow::createExtraFont(const int fontSize, const bool bold) {
        const HFONT f = CreateFontW(sc(fontSize), 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, Constants::Win32::uiFontFace());
        extraFonts.push_back(f);
        return f;
    }

    HWND SettingsWindow::createLabel(const std::wstring &settingsName, const std::wstring &descriptionTitle,
                                     const std::wstring &descriptionDetail, const int nw, const HFONT labelFont) {
        const int pad = sc(Constants::Win32::SETTINGS_LABEL_LEFT_PADDING);
        const HWND text = CreateWindowExW(0, WC_STATICW, settingsName.data(),
                                          Constants::Win32::STYLE_LABEL, pad,
                                          getYOffset(), nw - pad,
                                          inputHeight, window, nullptr,
                                          GetModuleHandleW(nullptr), nullptr);
        // A static with no font of its own falls back to the stock SYSTEM_FONT, a raster face that
        // matches nothing else in the panel: every row label was drawn in it while its own value
        // field, the section heading above it and the radio labels beside it were all in the window
        // font. Pass the window font unless the caller asked to enlarge just this row.
        SendMessageW(text, WM_SETFONT,
                     labelFont ? reinterpret_cast<WPARAM>(labelFont) : static_cast<WPARAM>(font), TRUE);
        subclassLabel(text);
        const HWND tooltip = CreateWindowExW(Constants::Win32::STYLE_EX_TOOLTIP, TOOLTIPS_CLASSW, nullptr,
                                            Constants::Win32::STYLE_TOOLTIP,
                                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, window, nullptr,
                                            nullptr, nullptr);
        const auto str = descriptionDetail.data();
        TTTOOLINFOW toolInfo = {};
        toolInfo.cbSize = 6 > SendMessage(tooltip, CCM_GETVERSION, 0, 0) ? TTTOOLINFOW_V2_SIZE : sizeof(TOOLINFOW); // WTF?
        toolInfo.hwnd = text;
        toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        toolInfo.uId = reinterpret_cast<UINT_PTR>(text);
        toolInfo.lpszText = const_cast<LPWSTR>(str);


        SendMessageW(tooltip, TTM_SETMAXTIPWIDTH, 0, windowWidth);
        SendMessageW(tooltip, TTM_SETTIPBKCOLOR, settingsTheme().tooltipBackground, 0);
        SendMessageW(tooltip, TTM_SETTIPTEXTCOLOR, settingsTheme().tooltipText, 0);
        SendMessageW(tooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&toolInfo));
        SendMessageW(tooltip, TTM_SETTITLEW, 0, reinterpret_cast<LPARAM>(descriptionTitle.data()));
        SendMessageW(tooltip, WM_SETFONT, font, TRUE);

        createdChildWindows.push_back(tooltip);
        createdChildWindows.push_back(text);
        return text;
    }

    void SettingsWindow::registerRowControls(const int index, std::vector<HWND> &&controls) {
        auto &group = rowControlGroups[index];
        for (const HWND control : controls) {
            if (control != nullptr) {
                group.push_back(control);
            }
        }
    }



    // Everything WM_DRAWITEM paints, into dis->hDC. The caller has that pointed at an off-screen
    // bitmap, so none of the steps below - the wipe to the panel color, the face, the label -
    // is on screen on its own. Returns false when the item belongs to nobody here.
    bool SettingsWindow::drawOwnerDrawnItem(SettingsWindow &wnd, const DRAWITEMSTRUCT *dis) {

        // A dropdown: its closed face, and the rows it drops.
        if (dis->CtlType == ODT_COMBOBOX && wnd.selectionBoxes.contains(dis->hwndItem)) {
            // The face. A combo box redraws it on its own whenever the value changes - outside any
            // paint of the control's - and asks its owner for the contents; unanswered, that pass
            // wiped the value off the face and left it blank until the next repaint of the panel.
            // The whole client rect is painted, of which the item clips out the part it covers.
            if (dis->itemState & ODS_COMBOBOXEDIT) {
                RECT client;
                GetClientRect(dis->hwndItem, &client);
                paintSelectionBoxFace(dis->hwndItem, dis->hDC, client, reinterpret_cast<HFONT>(wnd.font));
                return true;
            }
            const RECT rc = dis->rcItem;
            const bool highlighted = (dis->itemState & ODS_SELECTED) != 0;
            const HBRUSH face = CreateSolidBrush(highlighted
                                                     ? settingsTheme().radioSelectedBackground
                                                     : settingsTheme().textFieldBackground);
            FillRect(dis->hDC, &rc, face);
            DeleteObject(face);
            if (dis->itemID != static_cast<UINT>(-1)) {
                const int len = static_cast<int>(SendMessageW(dis->hwndItem, CB_GETLBTEXTLEN, dis->itemID, 0));
                std::wstring text(std::max(len, 0) + 1, L'\0');
                SendMessageW(dis->hwndItem, CB_GETLBTEXT, dis->itemID, reinterpret_cast<LPARAM>(text.data()));
                RECT textRc = {rc.left + sc(10), rc.top, rc.right - sc(10), rc.bottom};
                const auto oldFont = SelectObject(dis->hDC, reinterpret_cast<HFONT>(wnd.font));
                SetBkMode(dis->hDC, TRANSPARENT);
                SetTextColor(dis->hDC, settingsTheme().text);
                DrawTextW(dis->hDC, text.c_str(), -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                SelectObject(dis->hDC, oldFont);
            }
            return true;
        }

        // Owner-drawn full-panel "card": it paints its entire client rect itself.
        if (const auto cit = wnd.cardPainters.find(dis->hwndItem); cit != wnd.cardPainters.end()) {
            // The panel carries no font of its own, so a painter that draws text without
            // selecting one got the stock "System" bitmap face - the frozen-color and
            // texture-path panels both read that way. Start every painter on the body font;
            // the ones that want another (the note card) still select it themselves.
            const auto previousFont = SelectObject(dis->hDC, reinterpret_cast<HFONT>(wnd.font));
            (cit->second)(dis->hDC, dis->rcItem);
            SelectObject(dis->hDC, previousFont);
            return true;
        }

        // Owner-drawn color button: a normal push button with a small
        // black-framed color swatch embedded on its left side.
        if (const auto it = wnd.colorSwatches.find(dis->hwndItem); it != wnd.colorSwatches.end()) {
            RECT rc = dis->rcItem;
            const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
            const bool enabled = IsWindowEnabled(dis->hwndItem);

            // Soft rounded near-white face (matches the plain push buttons).
            drawRoundedButtonFace(dis->hDC, rc, pressed && enabled,
                                  (dis->itemState & ODS_FOCUS) != 0, enabled);

            // Inner content rect (shifted 1px when pressed, like a real button). The inset is
            // scaled: a fixed 4px kept the same size as the row grew with DPI, so the swatch
            // went from 64% of the button's height at 96 dpi to 78% at 192.
            RECT content = rc;
            InflateRect(&content, -sc(4), -sc(4));
            if (pressed) {
                OffsetRect(&content, 1, 1);
            }

            // Color swatch square on the left, vertically centered.
            const int swatchSlotSide = content.bottom - content.top;
            const int side = std::max(1, (swatchSlotSide * 9 + 5) / 10);
            const int swatchInset = (swatchSlotSide - side) / 2;
            RECT sr = {content.left + swatchInset, content.top + swatchInset,
                       content.left + swatchInset + side, content.top + swatchInset + side};
            const HBRUSH fill = CreateSolidBrush(enabled ? it->second() : settingsTheme().controlDisabledFace);
            FillRect(dis->hDC, &sr, fill);
            DeleteObject(fill);
            FrameRect(dis->hDC, &sr, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

            // Button text in the area to the right of the swatch.
            const int len = GetWindowTextLengthW(dis->hwndItem) + 1;
            std::wstring text(len, L'\0');
            GetWindowTextW(dis->hwndItem, text.data(), len);
            RECT textRc = {content.left + swatchSlotSide + sc(Constants::Win32::GAP_SETTINGS_COLOR_SWATCH), content.top,
                           content.right, content.bottom};
            const auto oldFont = SelectObject(dis->hDC, reinterpret_cast<HFONT>(wnd.font));
            SetBkMode(dis->hDC, TRANSPARENT);
            SetTextColor(dis->hDC, enabled ? settingsTheme().text : settingsTheme().textDisabled);
            DrawTextW(dis->hDC, text.c_str(), -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(dis->hDC, oldFont);
            return true;
        }

        const int index = getIndex(dis->hwndItem);
        if (!wnd.checkIndex(index)) {
            return false;
        }
        if (isCheckbox(dis->hwndItem)) {
            const bool checked = std::any_cast<bool>(wnd.references[index]);
            const bool enabled = IsWindowEnabled(dis->hwndItem);
            RECT rc = dis->rcItem;
            const int w = rc.right - rc.left;
            const int h = rc.bottom - rc.top;
            const int boxRadius = sc(Constants::Win32::CHECKBOX_CORNER_RADIUS);
            FillRect(dis->hDC, &rc, windowBackgroundBrush());
            // Rounded face, like every other control. A checked-but-disabled box is filled
            // grey and keeps the white tick: drawing a grey tick on the near-white disabled
            // face left it invisible.
            const COLORREF face = !checked
                                      ? enabled
                                            ? settingsTheme().textFieldBackground
                                            : settingsTheme().controlDisabledFace
                                      : enabled
                                            ? settingsTheme().checkboxChecked
                                            : settingsTheme().checkboxDisabledChecked;
            const HPEN boxPen = CreatePen(PS_SOLID, 1, settingsTheme().checkboxBorder);
            const HBRUSH boxBrush = CreateSolidBrush(face);
            const auto oldBoxPen = SelectObject(dis->hDC, boxPen);
            const auto oldBoxBrush = SelectObject(dis->hDC, boxBrush);
            RoundRect(dis->hDC, rc.left, rc.top, rc.right, rc.bottom, boxRadius, boxRadius);
            SelectObject(dis->hDC, oldBoxPen);
            SelectObject(dis->hDC, oldBoxBrush);
            DeleteObject(boxPen);
            DeleteObject(boxBrush);
            // Checkmark. The stroke is derived from the box so it keeps its weight as the
            // box grows with DPI; a fixed 2px left the tick hairline on a large box.
            if (checked) {
                const HPEN pen = CreatePen(PS_SOLID, std::max(1, w * 2 / 15),
                                            settingsTheme().checkboxMark);
                const auto oldPen = SelectObject(dis->hDC, pen);
                POINT pts[3] = {
                    {rc.left + w * 25 / 100, rc.top + h * 52 / 100},
                    {rc.left + w * 43 / 100, rc.top + h * 70 / 100},
                    {rc.left + w * 75 / 100, rc.top + h * 30 / 100},
                };
                Polyline(dis->hDC, pts, 3);
                SelectObject(dis->hDC, oldPen);
                DeleteObject(pen);
            }
            if (dis->itemState & ODS_FOCUS) {
                const HPEN focusPen = CreatePen(PS_SOLID, 1, settingsTheme().checkboxChecked);
                const auto oldFocusPen = SelectObject(dis->hDC, focusPen);
                const auto oldFocusBrush = SelectObject(dis->hDC, GetStockObject(NULL_BRUSH));
                RoundRect(dis->hDC, rc.left, rc.top, rc.right, rc.bottom, boxRadius, boxRadius);
                SelectObject(dis->hDC, oldFocusBrush);
                SelectObject(dis->hDC, oldFocusPen);
                DeleteObject(focusPen);
            }
            return true;
        }

        // Owner-drawn radio button: circle (sized to the checkbox) + label text.
        if (wnd.enumValues[index] != nullptr) {
            const int radioIndex = getRadioIndex(dis->hwndItem);
            const std::any &value = (*wnd.enumValues[index])[radioIndex];
            const bool selected =
                (*wnd.unparsers[index])(wnd.references[index]) == (*wnd.unparsers[index])(value);
            const bool enabled = IsWindowEnabled(dis->hwndItem);
            const auto previewIt = wnd.rowPreviews.find(dis->hwndItem);
            const bool hasPreview = previewIt != wnd.rowPreviews.end();
            const std::wstring text = (*wnd.unparsers[index])(value);

            RECT rc = dis->rcItem;
            RECT radioRc = rc;
            radioRc.right = rc.left + wnd.getRadioButtonWidth(text, rc.right - rc.left);
            FillRect(dis->hDC, &rc, windowBackgroundBrush());
            if (selected && enabled) {
                const HPEN rowPen = CreatePen(PS_SOLID, 1, settingsTheme().radioSelectedBorder);
                const HBRUSH rowBrush = CreateSolidBrush(settingsTheme().radioSelectedBackground);
                const auto oldRowPen = SelectObject(dis->hDC, rowPen);
                const auto oldRowBrush = SelectObject(dis->hDC, rowBrush);
                RoundRect(dis->hDC, radioRc.left, radioRc.top, radioRc.right, radioRc.bottom,
                          sc(Constants::Win32::BUTTON_CORNER_RADIUS), sc(Constants::Win32::BUTTON_CORNER_RADIUS));
                SelectObject(dis->hDC, oldRowPen);
                SelectObject(dis->hDC, oldRowBrush);
                DeleteObject(rowPen);
                DeleteObject(rowBrush);
            } else if (dis->itemState & ODS_FOCUS) {
                const HPEN focusPen = CreatePen(PS_SOLID, 1, settingsTheme().radioSelectedBorder);
                const auto oldFocusPen = SelectObject(dis->hDC, focusPen);
                const auto oldFocusBrush = SelectObject(dis->hDC, GetStockObject(NULL_BRUSH));
                RoundRect(dis->hDC, radioRc.left, radioRc.top, radioRc.right, radioRc.bottom,
                          sc(Constants::Win32::BUTTON_CORNER_RADIUS), sc(Constants::Win32::BUTTON_CORNER_RADIUS));
                SelectObject(dis->hDC, oldFocusBrush);
                SelectObject(dis->hDC, oldFocusPen);
                DeleteObject(focusPen);
            }

            const int d = wnd.rowBoxSize();
            const int left = rc.left + sc(8);
            const int top = (rc.top + rc.bottom - d) / 2;

            const HPEN borderPen = CreatePen(PS_SOLID, 1, settingsTheme().checkboxBorder);
            const auto oldPen = SelectObject(dis->hDC, borderPen);
            const auto oldBrush = SelectObject(dis->hDC, textFieldBrush());
            Ellipse(dis->hDC, left, top, left + d, top + d);
            if (selected) {
                // Filled inner dot in the highlight color. A NULL pen leaves the bounding
                // box's right/bottom edge unpainted, so the dot comes out a pixel narrower
                // than asked for and hangs up-left of the ring; outlining it with a pen in
                // its own color paints that edge back and lands the dot dead center.
                const int inset = d / 4;
                const HPEN dotPen = CreatePen(PS_SOLID, 1,
                                              settingsTheme().checkboxChecked);
                const auto oldDotPen = SelectObject(dis->hDC, dotPen);
                SelectObject(dis->hDC, checkedCheckboxBrush());
                Ellipse(dis->hDC, left + inset, top + inset, left + d - inset, top + d - inset);
                SelectObject(dis->hDC, oldDotPen);
                DeleteObject(dotPen);
            }
            SelectObject(dis->hDC, oldBrush);
            SelectObject(dis->hDC, oldPen);
            DeleteObject(borderPen);

            const int previewLeft = rc.left + (rc.right - rc.left) * 42 / 100;

            // Label text to the right of the circle.
            RECT textRc = {left + d + sc(8), rc.top,
                           radioRc.right - sc(4),
                           rc.bottom};
            const auto oldFont = SelectObject(dis->hDC, reinterpret_cast<HFONT>(wnd.font));
            SetBkMode(dis->hDC, TRANSPARENT);
            SetTextColor(dis->hDC, enabled ? settingsTheme().text : settingsTheme().textDisabled);
            DrawTextW(dis->hDC, text.c_str(), -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            SelectObject(dis->hDC, oldFont);

            if (hasPreview) {
                RECT pr = {previewLeft, rc.top + 2, rc.right - 1, rc.bottom - 2};
                (previewIt->second)(dis->hDC, pr);
                // Frame: highlight color for the chosen option, soft gray otherwise.
                const HBRUSH frame = CreateSolidBrush(selected
                                                          ? settingsTheme().previewBorderSelected
                                                          : settingsTheme().previewBorder);
                FrameRect(dis->hDC, &pr, frame);
                DeleteObject(frame);
            }
            return true;
        }

        // Owner-drawn plain push button: soft rounded near-white face with a
        // centered label (reached when it is not a checkbox, radio, or color button).
        {
            RECT rc = dis->rcItem;
            const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
            const bool enabled = IsWindowEnabled(dis->hwndItem);
            const bool primary = wnd.primaryButtons.contains(dis->hwndItem);
            if (primary) {
                drawPrimaryButtonFace(dis->hDC, rc, pressed && enabled, (dis->itemState & ODS_FOCUS) != 0);
            } else {
                drawRoundedButtonFace(dis->hDC, rc, pressed && enabled,
                                      (dis->itemState & ODS_FOCUS) != 0, enabled);
            }

            const int len = GetWindowTextLengthW(dis->hwndItem) + 1;
            std::wstring text(len, L'\0');
            GetWindowTextW(dis->hwndItem, text.data(), len);
            RECT textRc = rc;
            if (pressed) {
                OffsetRect(&textRc, 1, 1);
            }
            const auto oldFont = SelectObject(dis->hDC,
                                              reinterpret_cast<HFONT>(primary ? wnd.headerFont : wnd.font));
            SetBkMode(dis->hDC, TRANSPARENT);
            SetTextColor(dis->hDC, !enabled
                                       ? settingsTheme().textDisabled
                                       : primary
                                        ? settingsTheme().primaryButtonText
                                        : settingsTheme().text);
            DrawTextW(dis->hDC, text.c_str(), -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(dis->hDC, oldFont);
            return true;
        }
        return false;
    }

    LRESULT SettingsWindow::settingsWindowProc(const HWND window, const UINT message, const WPARAM wParam,
                                               const LPARAM lParam) {
        const auto self = reinterpret_cast<SettingsWindow *>(GetWindowLongPtr(window, GWLP_USERDATA));
        // Every color below is read through the flag this holds, panel by panel.
        const ScopedSettingsMode themeScope = scopedMode(self);
        SettingsWindow &wnd = *self;

        switch (message) {
            case WM_SW_FINALIZE: {
                // The whole panel is built now. Pre-capture the section layout so the first toggle
                // isn't doing extra work, then reveal it and force one full composited paint so the
                // surface is settled before the user can interact (no first-collapse flicker).
                if (wnd.sectionsCollapsible) {
                    wnd.captureSectionLayout();
                }
                // The rows exist only now, so this is the first moment their controls can be given
                // the theme's native class - a panel opened with dark mode already on never gets a
                // refresh of its own to do it.
                wnd.refreshTheme();
                ShowWindow(window, SW_SHOW);
                RedrawWindow(window, nullptr, nullptr,
                             RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
                return 0;
            }
            case WM_SW_FLUSH_PAINT: {
                // A batch that was already flushed at the end of its own message leaves this
                // behind; there is nothing owed, and repainting anyway would cost a full pass.
                wnd.flushPendingRepaint();
                return 0;
            }
            case WM_SW_THEME_CHANGED: {
                wnd.refreshTheme();
                return 0;
            }
            // Erasing is done inside WM_PAINT instead: the class brush erases on its own pass, and
            // that blank frame between the erase and the rows redrawing is what flashed across the
            // panel every time a row was shown or hidden.
            case WM_ERASEBKGND: return 1;
            case WM_PAINT: {
                // WM_PAINT can arrive before the constructor has attached the instance; paint the
                // background anyway (the frames are drawn from live control positions, so there is
                // nothing of them to draw yet).
                PAINTSTRUCT ps;
                const HDC hdc = BeginPaint(window, &ps);
                const int w = ps.rcPaint.right - ps.rcPaint.left;
                const int h = ps.rcPaint.bottom - ps.rcPaint.top;
                if (w > 0 && h > 0) {
                    // Compose off-screen and blit once. Painted straight onto the window, the wipe
                    // to the background color and the frames drawn back over it are two separate
                    // frames on screen, so every repaint that covers a whole section - folding one,
                    // showing or hiding a row - blinked its outline away and back.
                    const HDC mem = CreateCompatibleDC(hdc);
                    const HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
                    const auto oldBmp = SelectObject(mem, bmp);
                    // Client coordinates throughout, so the painters below need no offset of their own.
                    SetViewportOrgEx(mem, -ps.rcPaint.left, -ps.rcPaint.top, nullptr);
                    FillRect(mem, &ps.rcPaint, windowBackgroundBrush());
                    if (GetWindowLongPtr(window, GWLP_USERDATA)) {
                        wnd.paintSectionFrames(mem);
                    }
                    BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top, w, h,
                           mem, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
                    SelectObject(mem, oldBmp);
                    DeleteObject(bmp);
                    DeleteDC(mem);
                }
                EndPaint(window, &ps);
                return 0;
            }
            case WM_COMMAND: {
                if (const int ctlId = LOWORD(wParam);
                    ctlId >= Constants::Win32::ID_SECTION_TOGGLE &&
                    ctlId < Constants::Win32::ID_SECTION_TOGGLE + static_cast<int>(wnd.sections.size())) {
                    if (HIWORD(wParam) == STN_CLICKED) {
                        wnd.toggleSection(ctlId - Constants::Win32::ID_SECTION_TOGGLE);
                    }
                    return 0;
                }
                const auto editor = reinterpret_cast<HWND>(lParam);
                if (
                    const int index = getIndex(editor);
                    wnd.checkIndex(index) &&
                    LOWORD(wParam) != 0
                ) {
                    if (isCheckbox(editor)) {
                        // Owner-drawn checkbox: toggle the state we track ourselves.
                        std::any value = !std::any_cast<bool>(wnd.references[index]);
                        (*wnd.callbacks[index])(value);
                        wnd.references[index] = value;
                        // Repaint so the checked-state fill updates immediately.
                        InvalidateRect(editor, nullptr, TRUE);
                    } else if (HIWORD(wParam) == CBN_SELCHANGE) {
                        const auto combobox = (HWND) lParam;
                        const auto selectedIndex = static_cast<int>(SendMessage(combobox, CB_GETCURSEL, 0, 0));
                        std::any &value = (*wnd.enumValues[index])[selectedIndex];
                        (*wnd.callbacks[index])(value);
                        wnd.references[index] = value;
                    } else if (HIWORD(wParam) == BN_CLICKED) {
                        if (getRadioIndex(editor) >= 0 && wnd.enumValues[index] != nullptr) {
                             // This IS a radio button group because enumValues is populated
                            const int radioIndex = getRadioIndex(editor);
                            std::any &value = (*wnd.enumValues[index])[radioIndex];
                            (*wnd.callbacks[index])(value);
                            wnd.references[index] = value;
                            // Owner-drawn radios: repaint the whole group so the previous
                            // selection clears and the new one fills. Painted here and now, not
                            // left for the message loop: the button that was clicked redraws
                            // itself the moment this returns, so a merely invalidated group let
                            // the new selection appear a frame before the old one let go, and
                            // both read as filled in between.
                            for (const HWND child: wnd.createdChildWindows) {
                                if (getIndex(child) == index) {
                                    InvalidateRect(child, nullptr, TRUE);
                                    UpdateWindow(child);
                                }
                            }
                        } else {
                            // This is a push button
                            std::any dummy;
                            (*wnd.callbacks[index])(dummy);
                        }
                    }
                }

                // The callback above may have shown or hidden rows. Those changes are collected
                // rather than painted one at a time, and this is the end of the batch: paint it
                // here so the panel settles in the same frame as the control that was clicked,
                // instead of a trip round the message loop later.
                wnd.flushPendingRepaint();
                return 0;
            }
            case WM_HSCROLL: {
                const auto bar = reinterpret_cast<HWND>(lParam);
                const auto it = wnd.trackbarToSlider.find(bar);
                if (it == wnd.trackbarToSlider.end()) {
                    return DefWindowProcW(window, message, wParam, lParam);
                }
                SettingsWindow::SliderBinding &sb = wnd.sliders[it->second];
                if (!wnd.checkIndex(sb.index)) {
                    return 0;
                }
                constexpr int res = Constants::Win32::SLIDER_RESOLUTION;
                const auto pos = static_cast<int>(SendMessage(bar, TBM_GETPOS, 0, 0));
                const double t = pos / static_cast<double>(res);
                const double floorValue = sliderFloor(sb);
                const double raw = sb.logScale
                                       ? sliderWindowValue(sb, sb.currentBase, t)
                                       : sb.minValue + (sb.maxValue - sb.minValue) * t;
                auto value = static_cast<float>(std::clamp(raw, floorValue, sb.maxValue));
                if (sb.wholeSteps) {
                    value = static_cast<float>(std::llround(value));
                }
                if (value < static_cast<float>(floorValue)) {
                    value = static_cast<float>(floorValue);
                }
                std::any v = value;
                (*wnd.callbacks[sb.index])(v);
                wnd.references[sb.index] = v;
                wnd.modified[sb.index] = true;
                wnd.edited[sb.index] = false;
                {
                    const std::wstring newText = wnd.currValueToString(sb.index);
                    wchar_t prevBuf[128] = {};
                    GetWindowTextW(sb.textField, prevBuf, 128);
                    if (newText != prevBuf)
                        SetWindowTextW(sb.textField, newText.data());
                }

                if (sb.logScale && LOWORD(wParam) == TB_ENDTRACK) {
                    // Released: if parked at an end, slide the decade window over so the
                    // current value sits at the opposite end, ready to continue.
                    const double topBase = sliderTopBase(sb.minValue, sb.maxValue);
                    double newBase = sb.currentBase;
                    if (pos >= res) {
                        newBase = std::min(sb.currentBase * 10.0, topBase);
                    } else if (pos <= 0) {
                        newBase = std::max(sb.currentBase / 10.0, sb.minValue);
                    }
                    sb.currentBase = newBase;
                    const double nt = sliderWindowFraction(sb, newBase, value);
                    SendMessage(bar, TBM_SETPOS, TRUE, static_cast<int>(std::lround(nt * res)));
                }
                return 0;
            }
            case WM_VSCROLL: {
                // The window's own vertical scrollbar (shown when content overflows the
                // screen). Trackbars send WM_HSCROLL, so there is no conflict here.
                SCROLLINFO si = {};
                si.cbSize = sizeof(si);
                si.fMask = SIF_ALL;
                GetScrollInfo(window, SB_VERT, &si);
                int pos = si.nPos;
                switch (LOWORD(wParam)) {
                    case SB_LINEUP: pos -= sc(Constants::Win32::SETTINGS_INPUT_HEIGHT); break;
                    case SB_LINEDOWN: pos += sc(Constants::Win32::SETTINGS_INPUT_HEIGHT); break;
                    case SB_PAGEUP: pos -= static_cast<int>(si.nPage); break;
                    case SB_PAGEDOWN: pos += static_cast<int>(si.nPage); break;
                    case SB_THUMBTRACK:
                    case SB_THUMBPOSITION: pos = si.nTrackPos; break;
                    case SB_TOP: pos = 0; break;
                    case SB_BOTTOM: pos = si.nMax; break;
                    default: break;
                }
                wnd.scrollTo(pos);
                return 0;
            }
            case WM_MOUSEWHEEL: {
                // Scroll three rows per wheel notch when the content overflows.
                const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
                const int step = sc(Constants::Win32::SETTINGS_INPUT_HEIGHT) * 3;
                wnd.scrollTo(wnd.scrollY - delta * step / WHEEL_DELTA);
                return 0;
            }
            case WM_CLOSE: {
                for (const auto hwnd: wnd.createdChildWindows) DestroyWindow(hwnd);
                DestroyWindow(window);
                wnd.windowCloseFunction();
                return 0;
            }
            case WM_DESTROY: {
                DeleteObject(reinterpret_cast<HGDIOBJ>(wnd.font));
                DeleteObject(reinterpret_cast<HGDIOBJ>(wnd.headerFont));
                DeleteObject(reinterpret_cast<HGDIOBJ>(wnd.smallFont));
                for (const HFONT f : wnd.extraFonts) {
                    DeleteObject(f);
                }
                return 0;
            }
            case WM_CTLCOLOREDIT: {
                const auto hdcEdit = (HDC) wParam;
                const auto hwndEdit = (HWND) lParam;
                const int index = getIndex(hwndEdit);

                if (!wnd.checkIndex(index)) {
                    return DefWindowProcW(window, message, wParam, lParam);
                }

                if (wnd.error[index]) {
                    SetTextColor(hdcEdit, settingsTheme().textError);
                } else if (wnd.edited[index]) {
                    SetTextColor(hdcEdit, settingsTheme().textEdited);
                } else if (wnd.modified[index]) {
                    SetTextColor(hdcEdit, settingsTheme().textModified);
                } else {
                    SetTextColor(hdcEdit, settingsTheme().text);
                }
                // Light fill (clipped to the rounded region) gives the borderless field a
                // visible rounded-box shape against the white panel.
                SetBkColor(hdcEdit, settingsTheme().textFieldBackground);
                return (INT_PTR) textFieldBrush();
            }

            case WM_CTLCOLORLISTBOX: {
                const auto hdc = reinterpret_cast<HDC>(wParam);
                SetTextColor(hdc, settingsTheme().text);
                SetBkColor(hdc, settingsTheme().textFieldBackground);
                return reinterpret_cast<INT_PTR>(textFieldBrush());
            }

            case WM_NOTIFY: {
                // Trackbars draw through custom draw rather than WM_DRAWITEM. Take over the whole
                // control at the pre-paint stage and report back that nothing else is to be drawn:
                // the groove, the pointer and the background are all ours from here. Anything else
                // that notifies the panel (the tooltips are children of it too) falls through.
                const auto header = reinterpret_cast<NMHDR *>(lParam);
                if (header->code == NM_CUSTOMDRAW && GetWindowLongPtr(window, GWLP_USERDATA)) {
                    if (const auto it = wnd.trackbarToSlider.find(header->hwndFrom);
                        it != wnd.trackbarToSlider.end()) {
                        if (const auto custom = reinterpret_cast<NMCUSTOMDRAW *>(lParam);
                            custom->dwDrawStage == CDDS_PREPAINT) {
                            paintSlider(wnd.sliders[it->second], custom->hdc);
                        }
                        return CDRF_SKIPDEFAULT;
                    }
                }
                return DefWindowProcW(window, message, wParam, lParam);
            }
            case WM_DRAWITEM: {
                const auto dis = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
                // Compose the item off-screen and blit it in one go. An owner-drawn button erases
                // its face through the parent (WM_CTLCOLORBTN) before this ever runs, and the
                // painters below then wipe and rebuild it step by step - every one of those steps
                // used to land on screen, so a row flashed each time it was enabled, moved or
                // scrolled past.
                const RECT rc = dis->rcItem;
                const int w = rc.right - rc.left;
                const int h = rc.bottom - rc.top;
                if (w <= 0 || h <= 0) {
                    return TRUE;
                }
                const HDC face = dis->hDC;
                const HDC mem = CreateCompatibleDC(face);
                const HBITMAP bmp = CreateCompatibleBitmap(face, w, h);
                const auto oldBmp = SelectObject(mem, bmp);
                // Item coordinates throughout, so every painter below is left as it was written.
                SetViewportOrgEx(mem, -rc.left, -rc.top, nullptr);
                // The painters that leave part of the rect alone (the corners outside a rounded
                // face) expect the panel behind them, which is what the erase would have left.
                FillRect(mem, &rc, windowBackgroundBrush());
                dis->hDC = mem;
                const bool painted = drawOwnerDrawnItem(wnd, dis);
                dis->hDC = face;
                if (painted) {
                    BitBlt(face, rc.left, rc.top, w, h, mem, rc.left, rc.top, SRCCOPY);
                }
                SelectObject(mem, oldBmp);
                DeleteObject(bmp);
                DeleteDC(mem);
                if (!painted) {
                    return DefWindowProcW(window, message, wParam, lParam);
                }
                return TRUE;
            }
            case WM_CTLCOLORSTATIC: {
                const auto hwndStatic = (HWND) lParam;
                const auto hdc = (HDC) wParam;
                SetBkMode(hdc, TRANSPARENT);
                // Transparent text still blends against the DC's background color, and the default
                // white is not what these labels sit on once the panel goes dark.
                SetBkColor(hdc, settingsTheme().background);
                if (!IsWindowEnabled(hwndStatic)) {
                    SetTextColor(hdc, settingsTheme().textDisabled);
                    return (INT_PTR) windowBackgroundBrush();
                }
                // Slider min/max range labels and note text: muted gray on the panel face.
                if (wnd.rangeLabels.contains(hwndStatic) || wnd.noteLabels.contains(hwndStatic)) {
                    SetTextColor(hdc, settingsTheme().rangeText);
                    return (INT_PTR) windowBackgroundBrush();
                }
                SetTextColor(hdc, settingsTheme().text);
                return IsWindowEnabled(hwndStatic)
                           ? (INT_PTR) windowBackgroundBrush()
                           : DefWindowProcW(window, message, wParam, lParam);
            }
            case WM_CTLCOLORBTN: {
                // An owner-drawn button erases its whole face with this brush before it asks the
                // panel to draw the item, and there is no way to opt out of that pass. Unanswered,
                // the default is the OS button face - a gray wipe across the row every time one of
                // these repaints. Hand back the panel's own color so the erase cannot be seen.
                return (INT_PTR) windowBackgroundBrush();
            }
            default: return DefWindowProcW(window, message, wParam, lParam);
        }
    }

    void SettingsWindow::subclassLabel(const HWND label) const {
        SetWindowSubclass(label, labelProc, 1, reinterpret_cast<DWORD_PTR>(this));
    }

    // A static draws its own text, and a DISABLED one is drawn embossed: the caption once in the
    // system's 3D highlight color offset by a pixel, then again in its shadow color on top. On the
    // near-white panel the highlight copy lands on white and is never seen; on the dark one it is a
    // second, raised copy of every greyed label - the doubled, chiselled look the rows had. The
    // colors come from the system and cannot be pointed at the theme, so the text is drawn here
    // instead, the way every other control in this panel already is.
    LRESULT SettingsWindow::labelProc(const HWND window, const UINT message, const WPARAM wParam,
                                      const LPARAM lParam, const UINT_PTR uIdSubclass,
                                      const DWORD_PTR dwRefData) {
        const ScopedSettingsMode themeScope = scopedMode(reinterpret_cast<const SettingsWindow *>(dwRefData));
        if (message == WM_NCDESTROY) {
            RemoveWindowSubclass(window, labelProc, uIdSubclass);
        }
        // Erasing is done inside WM_PAINT, which composes off-screen.
        if (message == WM_ERASEBKGND) {
            return 1;
        }
        if (message == WM_PAINT) {
            const auto *wnd = reinterpret_cast<const SettingsWindow *>(dwRefData);
            PAINTSTRUCT ps;
            const HDC hdc = BeginPaint(window, &ps);
            RECT rc;
            GetClientRect(window, &rc);
            const int w = rc.right - rc.left;
            const int h = rc.bottom - rc.top;
            if (w > 0 && h > 0) {
                const HDC mem = CreateCompatibleDC(hdc);
                const HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
                const auto oldBmp = SelectObject(mem, bmp);
                FillRect(mem, &rc, windowBackgroundBrush());
                const auto labelFont = reinterpret_cast<HFONT>(SendMessageW(window, WM_GETFONT, 0, 0));
                const auto oldFont = SelectObject(mem, labelFont != nullptr
                                                           ? labelFont
                                                           : GetStockObject(DEFAULT_GUI_FONT));
                SetBkMode(mem, TRANSPARENT);
                SetBkColor(mem, settingsTheme().background);
                const bool muted = wnd != nullptr &&
                                   (wnd->rangeLabels.contains(window) || wnd->noteLabels.contains(window));
                SetTextColor(mem, !IsWindowEnabled(window)
                                      ? settingsTheme().textDisabled
                                      : muted
                                            ? settingsTheme().rangeText
                                            : settingsTheme().text);
                // The alignment the static was created with, in the flags DrawText names it by.
                const LONG style = GetWindowLongW(window, GWL_STYLE);
                const LONG alignment = style & SS_TYPEMASK;
                UINT format = alignment == SS_RIGHT
                                  ? DT_RIGHT
                                  : alignment == SS_CENTER
                                        ? DT_CENTER
                                        : DT_LEFT;
                format |= (style & SS_CENTERIMAGE) != 0
                              ? DT_VCENTER | DT_SINGLELINE
                              : DT_TOP | DT_WORDBREAK;
                const std::wstring text = windowText(window);
                DrawTextW(mem, text.c_str(), -1, &rc, format);
                SelectObject(mem, oldFont);
                BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
                SelectObject(mem, oldBmp);
                DeleteObject(bmp);
                DeleteDC(mem);
            }
            EndPaint(window, &ps);
            return 0;
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }

    LRESULT SettingsWindow::textFieldProc(const HWND window, const UINT message, const WPARAM wParam,
                                          const LPARAM lParam,
                                          [[maybe_unused]] UINT_PTR uIdSubclass,
                                          [[maybe_unused]] DWORD_PTR dwRefData) {
        const auto self = reinterpret_cast<SettingsWindow *>(GetWindowLongPtr(window, GWLP_USERDATA));
        const ScopedSettingsMode themeScope = scopedMode(self);
        auto &wnd = *self;

        // Skip the pre-paint erase: our WM_PAINT double-buffers, so a separate erase only flashes blank.
        if (message == WM_ERASEBKGND) {
            return 1;
        }

        // Double-buffer the repaint: the edit erases then draws text within one WM_PAINT, and that mid-paint blank flashes (worse as the text widens), so render into a memory DC and blit once.
        if (message == WM_PAINT) {
            RECT rc;
            GetClientRect(window, &rc);
            const int cw = rc.right - rc.left;
            const int ch = rc.bottom - rc.top;
            // Where the caret belongs once the line below has been laid out. Applied after EndPaint:
            // BeginPaint hides the caret and EndPaint restores it, so moving it in between smears
            // whatever it was sitting over. The control cannot be left to do this itself — it stops
            // moving its caret while painting is suppressed, stranding it at the left margin.
            POINT caretTarget = {-1, -1};
            PAINTSTRUCT ps;
            const HDC hdc = BeginPaint(window, &ps);
            // Windows scrolls this control by blitting its pixels sideways and invalidating only the uncovered strip; dropping that clip lets the full-width blit below land every time, so a scroll can never strand the previous view's digits in the untouched part.
            SelectClipRgn(hdc, nullptr);
            if (cw > 0 && ch > 0) {
                const HDC mem = CreateCompatibleDC(hdc);
                const HBITMAP bmp = CreateCompatibleBitmap(hdc, cw, ch);
                const auto oldBmp = SelectObject(mem, bmp);
                FillRect(mem, &rc, IsWindowEnabled(window) ? textFieldBrush() : disabledControlBrush());
                // Draw the text ourselves, vertically centered: a single-line edit always lays its line against the top of the client area.
                {
                    const std::wstring buf = windowText(window);
                    const int index = getIndex(window);
                    COLORREF color = settingsTheme().text;
                    if (!IsWindowEnabled(window)) {
                        color = settingsTheme().textDisabled;
                    } else if (wnd.checkIndex(index)) {
                        if (wnd.error[index]) color = settingsTheme().textError;
                        else if (wnd.edited[index]) color = settingsTheme().textEdited;
                        else if (wnd.modified[index]) color = settingsTheme().textModified;
                    }
                    const auto oldTextFont = SelectObject(mem, reinterpret_cast<HFONT>(
                                                              SendMessageW(window, WM_GETFONT, 0, 0)));
                    SetBkMode(mem, TRANSPARENT);
                    SetTextColor(mem, color);
                    // The single-line control lays its text against the top of the client area, so
                    // both the line we draw and the caret we move below are centered from here.
                    TEXTMETRICW tm;
                    GetTextMetricsW(mem, &tm);
                    const int lineTop = editTextTop(window, tm);
                    const int lineBottom = lineTop + tm.tmHeight;
                    const int textLen = static_cast<int>(buf.size());
                    if (textLen > 0) {
                        RECT fr;
                        SendMessageW(window, EM_GETRECT, 0, reinterpret_cast<LPARAM>(&fr));
                        // The packed return of EM_GETSEL is 16-bit and wraps on a long coordinate, so take the offsets through the out-parameters instead.
                        DWORD rawStart = 0;
                        DWORD rawEnd = 0;
                        SendMessageW(window, EM_GETSEL, reinterpret_cast<WPARAM>(&rawStart),
                                     reinterpret_cast<LPARAM>(&rawEnd));
                        const int selStart = std::min(static_cast<int>(rawStart), textLen);
                        const int selEnd = std::min(static_cast<int>(rawEnd), textLen);
                        // Width of the first `count` characters, i.e. that caret position's offset from the start of the line.
                        const auto prefixWidth = [&](const int count) {
                            SIZE sz = {0, 0};
                            if (count > 0) {
                                GetTextExtentPoint32W(mem, buf.c_str(), count, &sz);
                            }
                            return static_cast<int>(sz.cx);
                        };
                        const int startX = prefixWidth(selStart);
                        const int endX = prefixWidth(selEnd);
                        const int totalX = selEnd == textLen ? endX : prefixWidth(textLen);

                        // Where character 0 sits. A value that fits is right-aligned (the control is
                        // in ES_RIGHT then, and puts it in the same place); one that overflows may be
                        // scrolled anywhere between its tail and its head.
                        const int tailOrigin = fr.right - totalX;
                        const int headOrigin = std::max<int>(tailOrigin, fr.left);
                        // Ask the control where a character it currently shows actually sits: that pins character 0 exactly and needs no guess about which end of a selection the caret is on.
                        // The caret cannot answer this. GetCaretPos lags the control by an event and reads back a far-off sentinel when the field owns no caret, and deriving the origin from it slid the text sideways by a character on every keypress.
                        const auto originAt = [&](const int index) -> std::optional<int> {
                            const int k = std::min(index, textLen - 1);
                            const LRESULT pos = SendMessageW(window, EM_POSFROMCHAR, k, 0);
                            if (pos == -1) {
                                return std::nullopt;
                            }
                            // Off-screen positions may be truncated to 16 bits on a long value, so only trust one the field is actually showing.
                            if (const int x = static_cast<short>(LOWORD(pos)); x >= rc.left && x <= rc.right) {
                                return x - prefixWidth(k);
                            }
                            return std::nullopt;
                        };
                        std::optional<int> reported = originAt(selEnd);
                        if (!reported) {
                            reported = originAt(selStart);
                        }
                        const int originX = std::clamp(reported.value_or(headOrigin), tailOrigin, headOrigin);
                        const bool focused = GetFocus() == window;
                        if (focused) {
                            // A plain caret and a forward selection both sit at the far end of the selection; if that falls outside the field the selection runs backwards, so the caret is at the near end.
                            const int atEnd = originX + endX;
                            const int atStart = originX + startX;
                            const bool endVisible = atEnd >= fr.left && atEnd <= fr.right;
                            caretTarget = {std::min(endVisible ? atEnd : atStart, static_cast<int>(fr.right) - 1), lineTop};
                        }
                        RECT tr = {originX, lineTop, rc.right, rc.bottom};

                        // The control's own selection highlight never runs (its WM_PAINT is replaced), so draw the selected run here or a select-all would leave no visible mark at all.
                        // Only while focused, or every field the user has visited would keep showing a live-looking selection after they moved on.
                        const int hlLeft = std::max<int>(originX + startX, fr.left);
                        const int hlRight = std::min<int>(originX + endX, fr.right);
                        const bool drawSelection = focused && selStart != selEnd && hlRight > hlLeft;
                        if (drawSelection) {
                            RECT hl = {hlLeft, lineTop, hlRight, lineBottom};
                            const HBRUSH selBrush = CreateSolidBrush(
                                darkSettingsMode() ? settingsTheme().primaryButton : GetSysColor(COLOR_HIGHLIGHT));
                            FillRect(mem, &hl, selBrush);
                            DeleteObject(selBrush);
                        }
                        DrawTextW(mem, buf.c_str(), -1, &tr, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP);
                        // Redraw the same line clipped to the highlight so the selected digits stay readable against it.
                        if (drawSelection) {
                            SaveDC(mem);
                            IntersectClipRect(mem, hlLeft, rc.top, hlRight, rc.bottom);
                            SetTextColor(mem, darkSettingsMode()
                                                  ? settingsTheme().primaryButtonText
                                                  : GetSysColor(COLOR_HIGHLIGHTTEXT));
                            DrawTextW(mem, buf.c_str(), -1, &tr, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP);
                            RestoreDC(mem, -1);
                        }
                    }
                    SelectObject(mem, oldTextFont);
                }
                // 1px rounded border so the field reads clearly against the white panel.
                const HPEN pen = CreatePen(PS_SOLID, 1, settingsTheme().textFieldBorder);
                const auto oldPen = SelectObject(mem, pen);
                const auto oldBrush = SelectObject(mem, GetStockObject(NULL_BRUSH));
                RoundRect(mem, rc.left, rc.top, rc.right, rc.bottom,
                          sc(Constants::Win32::BUTTON_CORNER_RADIUS), sc(Constants::Win32::BUTTON_CORNER_RADIUS));
                SelectObject(mem, oldPen);
                SelectObject(mem, oldBrush);
                DeleteObject(pen);
                BitBlt(hdc, 0, 0, cw, ch, mem, 0, 0, SRCCOPY);
                SelectObject(mem, oldBmp);
                DeleteObject(bmp);
                DeleteDC(mem);
            }
            EndPaint(window, &ps);
            if (caretTarget.x >= 0 && GetFocus() == window) {
                if (POINT current; !GetCaretPos(&current) || current.x != caretTarget.x ||
                                   current.y != caretTarget.y) {
                    SetCaretPos(caretTarget.x, caretTarget.y);
                }
            }
            return 0;
        }

        if (message == WM_SIZE) {
            const LRESULT res = DefSubclassProc(window, message, wParam, lParam);
            applyRoundedRegion(window);
            layoutEditText(window);
            return res;
        }

        // Flag the row as edited only for input that can actually change the text. Setting it on
        // every WM_KEYDOWN turned a field amber ("typed but not committed") for a plain caret move -
        // Home, End, an arrow key, even Ctrl+C - while its value had not moved at all.
        {
            const auto markEdited = [&] {
                if (const int index = getIndex(window); wnd.checkIndex(index)) {
                    wnd.edited[index] = true;
                }
            };
            switch (message) {
                case WM_PASTE:
                case WM_CUT:
                case WM_CLEAR:
                case WM_UNDO:
                case EM_REPLACESEL:
                    markEdited();
                    break;
                // Control characters are commands rather than text, and Backspace / Delete arrive
                // as WM_KEYDOWN below. WM_SETTEXT is deliberately absent: it is how a committed
                // value is written back, which is the opposite of an edit.
                case WM_CHAR:
                    if (wParam >= L' ') {
                        markEdited();
                    }
                    break;
                case WM_KEYDOWN:
                    if (wParam == VK_DELETE || wParam == VK_BACK) {
                        markEdited();
                    }
                    break;
                default:
                    break;
            }
        }

        // Every message here can edit the text, move the caret or scroll the view, and the control
        // handles all three by drawing straight to the screen: it blits the line sideways to scroll and
        // repaints in place to edit, both clipped to its own line box, which sits above the vertically
        // centered line this field draws. Letting any of that reach the screen tore the digits along
        // that boundary, which is what made a scrolling value look like it was shaking.
        // So the control gets to process them with painting switched off, and the field is then
        // re-aligned, its caret put back on the drawn line, and the whole thing repainted in one blit.
        switch (message) {
            case WM_SETTEXT:
            case WM_PASTE:
            case WM_CUT:
            case WM_CLEAR:
            case WM_UNDO:
            case EM_REPLACESEL:
            case EM_SETSEL:
            case EM_SCROLLCARET:
            case EM_LINESCROLL:
            case WM_HSCROLL:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_LBUTTONDBLCLK:
            // The control auto-scrolls a drag that has run off the edge on a timer of its own.
            case WM_TIMER:
                return relayoutQuietly(window, message, wParam, lParam);
            // Only while dragging: a plain hover changes nothing and would repaint on every move.
            case WM_MOUSEMOVE:
                if (wParam & MK_LBUTTON) {
                    return relayoutQuietly(window, message, wParam, lParam);
                }
                break;
            // Focus is left to paint normally: suppressing redraw across it interferes with the
            // control creating and showing its caret.
            case WM_SETFOCUS:
            case WM_KILLFOCUS: {
                const LRESULT res = DefSubclassProc(window, message, wParam, lParam);
                layoutEditText(window);
                InvalidateRect(window, nullptr, FALSE);
                return res;
            }
            default:
                break;
        }

        // Swallow the Enter/Esc characters so committing or cancelling a value does not beep.
        if (message == WM_CHAR && (wParam == VK_RETURN || wParam == L'\n' || wParam == VK_ESCAPE)) {
            return 0;
        }

        if (message == WM_KEYDOWN) {
            const int index = getIndex(window);
            // Up/Down arrows on a slider-backed field nudge its value (and move the paired
            // trackbar) instead of doing nothing in this single-line edit control.
            if ((wParam == VK_UP || wParam == VK_DOWN) && wnd.checkIndex(index)) {
                // Shift = coarse (x10 per press); plain = fine step.
                const bool coarse = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                if (const auto sit = wnd.indexToSlider.find(index); sit != wnd.indexToSlider.end()) {
                    wnd.nudgeSliderValue(sit->second, wParam == VK_UP ? 1 : -1, coarse);
                    return 0;
                }
                // Plain (non-slider) float fields that opted into arrow nudging (linear or
                // decade-adaptive).
                if (wnd.textFieldArrowSteps.contains(index) ||
                    wnd.textFieldDecadeRanges.contains(index)) {
                    wnd.nudgeTextFieldValue(window, index, wParam == VK_UP ? 1 : -1, coarse);
                    return 0;
                }
            }
            if (wParam == VK_ESCAPE && wnd.checkIndex(index)) {
                SetWindowTextW(window, (*wnd.unparsers[index])(wnd.references[index]).data());
                return 0;
            }
            if (wParam == VK_RETURN && wnd.checkIndex(index)) {
                const int length = GetWindowTextLengthW(window) + 1; //include NULL character
                std::wstring buf(length, '\0');
                GetWindowTextW(window, buf.data(), length);

                const HDC hdc = GetDC(window);
                try {
                    if (std::any value = (*wnd.parsers[index])(buf);
                        (*wnd.validConditions[index])(value)
                    ) {
                        (*wnd.callbacks[index])(value);
                        wnd.references[index] = value;
                        wnd.modified[index] = true;
                        wnd.edited[index] = false;
                    } else {
                        wnd.callError(index);
                    }
                } catch (const std::invalid_argument &) {
                    wnd.callError(index);
                } catch (const std::out_of_range &) {
                    // A number too large for the target type (an overflowing Cycle Length, say) used to
                    // escape as an uncaught exception; report it like any other invalid entry.
                    wnd.callError(index);
                }
                SetWindowTextW(window, wnd.currValueToString(index).data());
                // Keep the paired slider in sync when this field belongs to one.
                if (const auto sit = wnd.indexToSlider.find(index); sit != wnd.indexToSlider.end()) {
                    SliderBinding &sb = wnd.sliders[sit->second];
                    setTrackbarFromValue(sb, static_cast<double>(std::any_cast<float>(wnd.references[index])));
                }
                ReleaseDC(window, hdc);
                return 0;
            }
        }

        // Typing and caret keys fall through to here; they edit the text or move the view, so they get the same treatment.
        if (message == WM_CHAR || message == WM_KEYDOWN) {
            return relayoutQuietly(window, message, wParam, lParam);
        }

        return DefSubclassProc(window, message, wParam, lParam);
    }

    LRESULT SettingsWindow::ownerDrawnButtonProc(const HWND window, const UINT message, const WPARAM wParam,
                                                 const LPARAM lParam,
                                                 [[maybe_unused]] UINT_PTR uIdSubclass,
                                                 [[maybe_unused]] DWORD_PTR dwRefData) {
        // An owner-drawn button fills its whole face with the parent's WM_CTLCOLORBTN brush before
        // it asks for WM_DRAWITEM, and the panel cannot answer that in a way that costs nothing:
        // the wipe and the finished face are two separate writes to the screen, so a display
        // refresh landing between them catches the button as a blank patch. It happens on any
        // repaint - being enabled, being clicked, being moved by a fold - which is why it showed
        // up as an occasional blink rather than something reproducible. WM_DRAWITEM composes the
        // face off-screen over the panel color and covers every pixel of the item, so the erase
        // has nothing left to contribute.
        if (message == WM_ERASEBKGND) {
            return 1;
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }

    // Row height, theme and owner-drawing a dropdown needs once its items are in place. A combo box
    // sizes its closed face from its item height plus a frame of its own, so the frame is measured
    // from what the first request produced and taken off the second - the face then stands exactly
    // as tall as the fields and buttons in the rows around it, whatever the DPI.
    void SettingsWindow::finishSelectionBox(const HWND combobox) {
        SendMessageW(combobox, WM_SETFONT, font, TRUE);
        SendMessageW(combobox, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), inputHeight);
        RECT rc;
        GetWindowRect(combobox, &rc);
        if (const int frame = rc.bottom - rc.top - inputHeight; frame > 0 && frame < inputHeight) {
            SendMessageW(combobox, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), inputHeight - frame);
        }
        SendMessageW(combobox, CB_SETITEMHEIGHT, 0, inputHeight);
        applyNativeControlTheme(combobox);
        selectionBoxes.insert(combobox);
        SetWindowSubclass(combobox, selectionBoxProc, 1, reinterpret_cast<DWORD_PTR>(this));
    }

    // A combo box draws its own closed face - a square system frame, a system-colored fill and the
    // theme's own arrow - and none of that follows the panel. The face is painted here instead, on
    // the rounded button face every other value control in the column carries.
    LRESULT SettingsWindow::selectionBoxProc(const HWND window, const UINT message, const WPARAM wParam,
                                             const LPARAM lParam,
                                             [[maybe_unused]] UINT_PTR uIdSubclass,
                                             const DWORD_PTR dwRefData) {
        const auto wnd = reinterpret_cast<SettingsWindow *>(dwRefData);
        const ScopedSettingsMode themeScope = scopedMode(wnd);
        switch (message) {
            // The face below covers every pixel of the control, so an erase pass only flashes the
            // panel color through it.
            case WM_ERASEBKGND:
                return 1;
            case WM_PAINT: {
                PAINTSTRUCT ps;
                const HDC dc = BeginPaint(window, &ps);
                RECT rc;
                GetClientRect(window, &rc);
                const int w = rc.right - rc.left;
                const int h = rc.bottom - rc.top;
                if (w <= 0 || h <= 0) {
                    EndPaint(window, &ps);
                    return 0;
                }
                // Composed off-screen for the same reason the owner-drawn buttons are: the face,
                // the label and the arrow are separate writes, and a refresh landing between them
                // catches the control half drawn.
                const HDC mem = CreateCompatibleDC(dc);
                const HBITMAP bmp = CreateCompatibleBitmap(dc, w, h);
                const auto oldBmp = SelectObject(mem, bmp);
                paintSelectionBoxFace(window, mem, rc, reinterpret_cast<HFONT>(wnd->font));
                BitBlt(dc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
                SelectObject(mem, oldBmp);
                DeleteObject(bmp);
                DeleteDC(mem);
                EndPaint(window, &ps);
                return 0;
            }
            // Everything that changes what the face shows: the value, the focus ring, the pressed
            // look while the list is down, and the greyed face of a row switched off.
            case WM_SETFOCUS:
            case WM_KILLFOCUS:
            case WM_ENABLE:
            case WM_MOUSEWHEEL:
            case WM_KEYDOWN:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case CB_SETCURSEL:
            case CB_SHOWDROPDOWN:
            case CB_SELECTSTRING: {
                const LRESULT result = DefSubclassProc(window, message, wParam, lParam);
                InvalidateRect(window, nullptr, FALSE);
                return result;
            }
            default:
                break;
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }

    LRESULT SettingsWindow::trackbarProc(const HWND window, const UINT message, const WPARAM wParam,
                                         const LPARAM lParam,
                                         [[maybe_unused]] UINT_PTR uIdSubclass,
                                         [[maybe_unused]] DWORD_PTR dwRefData) {
        // Skip the erase pass: paintSlider covers every pixel of the control, so a separate erase
        // only flashes the panel color through the bar.
        if (message == WM_ERASEBKGND) {
            return 1;
        }

        // Repaint the whole bar, never the strip the control asked for. Moving the thumb invalidates
        // the trackbar's own idea of it, which is narrower than the circle drawn in its place - so
        // the far side of that circle would be left behind, a crescent at a time, all the way along
        // a drag. Invalidating before the control's BeginPaint widens the update region it is about
        // to validate, so this costs one paint, not a second one.
        if (message == WM_PAINT) {
            InvalidateRect(window, nullptr, FALSE);
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }


    void SettingsWindow::callError(const int index) {
        error[index] = true;
        MessageBox(window, "Invalid value!", "Error", MB_OK | MB_ICONERROR);
        error[index] = false;
    }


    std::wstring SettingsWindow::currValueToString(const int index) const {
        return (*unparsers[index])(references[index]);
    }


    HWND SettingsWindow::registerButton(const std::wstring &settingsName, const std::wstring &buttonText,
                                        std::function<void()> &&callback,
                                        const std::wstring &descriptionTitle, const std::wstring &descriptionDetail) {
        // Inline layout: label on the left, button on the right, matching the
        // checkbox/text-input rows.
        const int nw = getFixedNameWidth();
        const int vw = getFixedValueWidth();

        const int index = count;
        const HWND label = createLabel(settingsName, descriptionTitle, descriptionDetail, nw);

        const HWND button = CreateWindowExW(0, WC_BUTTONW, buttonText.data(),
                                            Constants::Win32::STYLE_PUSHBUTTON,
                                            nw,
                                            getYOffset(),
                                            vw,
                                            inputHeight,
                                            window,
                                            reinterpret_cast<HMENU>(Constants::Win32::ID_OPTIONS + index),
                                            nullptr,
                                            nullptr);

        SendMessage(button, WM_SETFONT, font, TRUE);
        SetWindowSubclass(button, ownerDrawnButtonProc, 1, 0);
        createdChildWindows.push_back(button);
        registerRowControls(index, {label, button});

        // We need to register a dummy action so the index matches
        bool dummy = false;
        references.emplace_back(dummy);
        unparsers.emplace_back(std::make_unique<std::function<std::wstring(const std::any &)>>([](const std::any &) { return L""; }));
        parsers.emplace_back(nullptr);
        validConditions.emplace_back(nullptr);

        // The callback for button needs to be wrapped to match signature
        callbacks.emplace_back(std::make_unique<std::function<void(std::any &)>>([callback](std::any &) {
            callback();
        }));

        enumValues.emplace_back(nullptr);
        error.emplace_back(false);
        edited.emplace_back(false);
        modified.emplace_back(false);
        // No value of its own to write back to: a button is not a setting row.
        valuePointers.emplace_back(nullptr);
        ++count;
        advanceRow();

        adjustWindowHeight();
        return button;
    }


    // Highest decade window a log slider opens: the largest power of ten strictly below maxValue,
    // so that window ([topBase, maxValue]) still has somewhere to run. Every base being a power of
    // ten is what makes a value stepping up past a decade land back at the left end of the bar.
    // Capping the base at maxValue/10 instead put a 0.1-20 slider's top window at [2, 20], off the
    // grid the windows below it use: a value crossing 10 moved the thumb *backwards*, from the far
    // right of [1, 10] to the middle of [2, 20].
    double SettingsWindow::sliderTopBase(const double minValue, const double maxValue) {
        // The epsilon keeps a maxValue that is itself a power of ten (10000) on the window below it
        // (1000) whichever side of the integer its log10 lands on.
        return std::max(minValue, std::pow(10.0, std::ceil(std::log10(maxValue) - 1e-9) - 1.0));
    }

    // Decades the window starting at `base` actually spans: a full one everywhere except the top
    // window, which stops at maxValue. Positions are normalized by this, so the whole bar stays
    // usable even where that last window is a fraction of a decade wide ([10, 20] is 0.3 of one).
    double SettingsWindow::sliderWindowDecades(const double base, const double maxValue) {
        return std::max(1e-6, std::log10(std::min(maxValue / base, 10.0)));
    }

    double SettingsWindow::sliderFloor(const SliderBinding &binding) {
        return binding.zeroStop ? 0.0 : binding.minValue;
    }

    // The bottom window carries the zero stop, and is the only window on the slider that is not
    // logarithmic: it runs linearly from 0 to minValue*10 so that 0 lands on the bar's left end.
    double SettingsWindow::sliderWindowFraction(const SliderBinding &binding, const double base,
                                                const double value) {
        if (binding.zeroStop && base <= binding.minValue) {
            return std::clamp(value / (binding.minValue * 10.0), 0.0, 1.0);
        }
        return std::clamp(std::log10(value / base) / sliderWindowDecades(base, binding.maxValue), 0.0, 1.0);
    }

    double SettingsWindow::sliderWindowValue(const SliderBinding &binding, const double base, const double t) {
        if (binding.zeroStop && base <= binding.minValue) {
            return t * binding.minValue * 10.0;
        }
        return base * std::pow(10.0, t * sliderWindowDecades(base, binding.maxValue));
    }

    void SettingsWindow::setTrackbarFromValue(SliderBinding &binding, double value) {
        value = std::clamp(value, sliderFloor(binding), binding.maxValue);
        if (!binding.logScale) {
            const double span = binding.maxValue - binding.minValue;
            const double t = span <= 0.0 ? 0.0 : (value - binding.minValue) / span;
            const auto pos = static_cast<int>(std::lround(std::clamp(t, 0.0, 1.0) * Constants::Win32::SLIDER_RESOLUTION));
            SendMessage(binding.trackbar, TBM_SETPOS, TRUE, pos);
            return;
        }
        // The zero-stop window is picked by range rather than by decade: it is the only one that can
        // hold 0, whose log10 is -inf and so names no decade at all.
        const double base = binding.zeroStop && value < binding.minValue * 10.0
                                ? binding.minValue
                                : std::clamp(std::pow(10.0, std::floor(std::log10(value))),
                                             binding.minValue,
                                             sliderTopBase(binding.minValue, binding.maxValue));
        binding.currentBase = base;
        const auto pos = static_cast<int>(std::lround(
            sliderWindowFraction(binding, base, value) * Constants::Win32::SLIDER_RESOLUTION));
        SendMessage(binding.trackbar, TBM_SETPOS, TRUE, pos);
    }


    // Parks the thumb at each end and reads back where the control put it. The trackbar's channel
    // rect is not this range - it is inset from the control's edges by less than half a thumb, so
    // the thumb center stops a few pixels short of either end of it - and drawing a track the thumb
    // visibly never reaches is exactly what the captions under the bar were misaligned against
    // before. Both come from here now. Redrawing must be on: with it off the control defers the
    // recalculation and hands back the thumb's previous rect.
    void SettingsWindow::measureTrackTravel(const HWND bar, int &left, int &right) {
        const auto thumbCenter = [bar](const int pos) {
            SendMessageW(bar, TBM_SETPOS, TRUE, pos);
            RECT thumb = {};
            SendMessageW(bar, TBM_GETTHUMBRECT, 0, reinterpret_cast<LPARAM>(&thumb));
            return static_cast<int>((thumb.left + thumb.right) / 2);
        };
        left = thumbCenter(0);
        right = thumbCenter(Constants::Win32::SLIDER_RESOLUTION);
    }


    // The slider's whole face, drawn in place of the trackbar's own groove and pointer: a thin
    // rounded track with everything left of the thumb filled in the accent color, and a round thumb
    // riding on it. GDI draws no antialiased curve, so the control is rendered at
    // SLIDER_SUPERSAMPLE times its size and shrunk with a halftone blit, which averages the steps
    // along the circle and the pill's caps away.
    void SettingsWindow::paintSlider(const SliderBinding &binding, const HDC hdc) {
        RECT client;
        GetClientRect(binding.trackbar, &client);
        const int w = client.right - client.left;
        const int h = client.bottom - client.top;
        if (w <= 0 || h <= 0) {
            return;
        }
        // The thumb is drawn where the control itself put it, so it stays under the cursor through
        // a drag no matter how the trackbar maps position to pixels.
        RECT thumb = {};
        SendMessageW(binding.trackbar, TBM_GETTHUMBRECT, 0, reinterpret_cast<LPARAM>(&thumb));
        const int thumbX = static_cast<int>((thumb.left + thumb.right) / 2);
        const bool enabled = IsWindowEnabled(binding.trackbar) != FALSE;

        constexpr int ss = Constants::Win32::SLIDER_SUPERSAMPLE;
        // Never bigger than the room left beyond the ends of the thumb's travel, or the circle
        // would be cut off by the control's own edge at the ends of the bar.
        const int radius = std::max(1, std::min({
            sc(Constants::Win32::SLIDER_THUMB_DIAMETER) / 2, h / 2,
            binding.trackLeft, w - binding.trackRight
        }));
        const int half = std::max(1, std::min(sc(Constants::Win32::SLIDER_TRACK_THICKNESS), h) / 2) * ss;

        const HDC mem = CreateCompatibleDC(hdc);
        const HBITMAP bmp = CreateCompatibleBitmap(hdc, w * ss, h * ss);
        const auto oldBmp = SelectObject(mem, bmp);
        RECT memRc = {0, 0, w * ss, h * ss};
        FillRect(mem, &memRc, windowBackgroundBrush());

        // Center of the pixel each of these lands on, once magnified.
        const int cy = h * ss / 2;
        const int cx = thumbX * ss + ss / 2;
        const int left = binding.trackLeft * ss + ss / 2;
        const int right = binding.trackRight * ss + ss / 2;

        // The track, then the travelled part over it. Both are pills; the round right cap of the
        // filled one sits under the thumb, which covers it.
        fillPill(mem, left - half, right + half, cy, half,
                 enabled ? settingsTheme().sliderTrack : settingsTheme().sliderTrackDisabled);
        fillPill(mem, left - half, cx + half, cy, half,
                 enabled ? settingsTheme().sliderFill : settingsTheme().sliderDisabled);

        // A plain white disc with a thin outline. The white is what keeps the thumb readable where
        // it stands on the filled part of the track, which is the accent color.
        fillDisc(mem, cx, cy, radius * ss, settingsTheme().sliderThumbFace,
                 enabled ? settingsTheme().sliderThumbBorder : settingsTheme().sliderDisabled, ss);

        SetStretchBltMode(hdc, HALFTONE);
        SetBrushOrgEx(hdc, 0, 0, nullptr);
        StretchBlt(hdc, 0, 0, w, h, mem, 0, 0, w * ss, h * ss, SRCCOPY);

        SelectObject(mem, oldBmp);
        DeleteObject(bmp);
        DeleteDC(mem);
    }


    void SettingsWindow::nudgeSliderValue(const int sliderIdx, const int direction, const bool coarse) {
        SliderBinding &sb = sliders[sliderIdx];
        if (!checkIndex(sb.index)) {
            return;
        }
        const double current = static_cast<double>(std::any_cast<float>(references[sb.index]));
        const double floorValue = sliderFloor(sb);
        float value;
        if (sb.logScale) {
            const double factor = coarse ? 10.0 : 1.1220184543019633;
            const double raw = direction > 0 ? current * factor : current / factor;
            value = static_cast<float>(sb.wholeSteps
                                           ? std::llround(std::clamp(raw, floorValue, sb.maxValue))
                                           : std::clamp(raw, floorValue, sb.maxValue));
            const auto curWhole = static_cast<float>(std::llround(current));
            if (sb.wholeSteps && value == curWhole) {
                value = static_cast<float>(std::clamp(
                    static_cast<double>(curWhole) + direction, floorValue, sb.maxValue));
            }
        } else {
            const double step = (sb.maxValue - sb.minValue) / (coarse ? 10.0 : 100.0);
            const double raw = current + step * direction;
            value = static_cast<float>(std::clamp(raw, sb.minValue, sb.maxValue));
            if (sb.wholeSteps) {
                value = static_cast<float>(std::llround(value));
            }
        }
        std::any v = value;
        (*callbacks[sb.index])(v);
        references[sb.index] = v;
        modified[sb.index] = true;
        edited[sb.index] = false;
        SetWindowTextW(sb.textField, currValueToString(sb.index).data());
        setTrackbarFromValue(sb, static_cast<double>(value));
    }


    void SettingsWindow::nudgeTextFieldValue(const HWND field, const int index, const int direction,
                                             const bool coarse) {
        if (!checkIndex(index)) {
            return;
        }
        const auto decadeIt = textFieldDecadeRanges.find(index);
        const auto it = textFieldArrowSteps.find(index);
        if (decadeIt == textFieldDecadeRanges.end() && it == textFieldArrowSteps.end()) {
            return;
        }
        // Only plain float fields support arrow nudging; bail out for any other stored type.
        const float *cur = std::any_cast<float>(&references[index]);
        if (cur == nullptr) {
            return;
        }

        if (decadeIt != textFieldDecadeRanges.end()) {
            const auto [lo, hi] = decadeIt->second;
            const double rawCur = static_cast<double>(*cur);
            float value;
            if (direction < 0 && rawCur <= lo) {
                value = 0.0f;
            } else if (direction > 0 && rawCur < lo) {
                value = static_cast<float>(lo);
            } else {
                const double current = std::clamp(rawCur, lo, hi);
                int exp = static_cast<int>(std::floor(std::log10(std::max(current, lo)) + 1e-9));
                // When stepping DOWN off an exact power of ten (e.g. 1000 -> 900, not 1000 -> 0),
                // drop to the band below so the step matches that lower decade.
                if (direction < 0 && std::abs(current - std::pow(10.0, exp)) <= current * 1e-9) {
                    --exp;
                }
                const double decade = std::pow(10.0, exp);
                const double step = coarse ? decade * 10.0 : decade;
                // Snap onto the decade grid so repeated presses stay on clean multiples.
                const double snapped = std::round((current + step * direction) / step) * step;
                value = static_cast<float>(std::clamp(snapped, lo, hi));
            }
            std::any v = value;
            if (validConditions[index] && !(*validConditions[index])(v)) {
                return;
            }
            (*callbacks[index])(v);
            references[index] = v;
            modified[index] = true;
            edited[index] = false;
            SetWindowTextW(field, currValueToString(index).data());
            return;
        }

        const double step = it->second;
        const double delta = (coarse ? step * 10.0 : step) * direction;
        const auto value = static_cast<float>(std::round((static_cast<double>(*cur) + delta) / step) * step);
        std::any v = value;
        // Reject steps that would leave the valid range (acts as a clamp at the bounds).
        if (validConditions[index] && !(*validConditions[index])(v)) {
            return;
        }
        (*callbacks[index])(v);
        references[index] = v;
        modified[index] = true;
        edited[index] = false;
        SetWindowTextW(field, currValueToString(index).data());
    }


    HWND SettingsWindow::registerSliderInput(const std::wstring &settingsName, float *ptr,
                                             const float minValue, const float maxValue,
                                             std::function<std::wstring(const float &)> &&unparser,
                                             std::function<float(std::wstring &)> &&parser,
                                             std::function<bool(const float &)> &&validCondition,
                                             std::function<void()> &&callback,
                                             const std::wstring &descriptionTitle,
                                             const std::wstring &descriptionDetail,
                                             const std::wstring &trailingButtonText,
                                             std::function<void()> &&trailingButtonCallback) {
        const int nw = getFixedNameWidth();
        const int vw = getFixedValueWidth();
        const int textGap = sc(Constants::Win32::GAP_SETTINGS_COLOR_SWATCH);
        // Reserve a trailing button area only when there actually is a button, so a
        // plain (button-less) slider uses the full value-column width.
        const bool hasButton = !trailingButtonText.empty();
        const int buttonGap = hasButton ? sc(Constants::Win32::GAP_SETTINGS_COLOR_SWATCH * 5) : 0;
        const int buttonW = hasButton ? vw * 27 / 100 : 0;
        const int textW = vw * 3 / 10;
        const int sliderW = vw - textW - buttonW - textGap - buttonGap;
        const int y = getYOffset();

        // The index this row registers into references/callbacks (registerActions
        // increments count, so capture it first).
        const int index = count;
        const HWND label = createLabel(settingsName, descriptionTitle, descriptionDetail, nw);

        const HWND text = CreateWindowExW(0, WC_EDITW, unparser(*ptr).data(),
                                          Constants::Win32::STYLE_TEXT_FIELD, nw, y, textW,
                                          inputHeight, window,
                                          reinterpret_cast<HMENU>(Constants::Win32::ID_OPTIONS + count),
                                          nullptr, nullptr);
        SetWindowSubclass(text, textFieldProc, 1, 0);
        SetWindowLongPtr(text, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        SendMessage(text, WM_SETFONT, font, TRUE);
        SendMessage(text, EM_SETLIMITTEXT, 0, 0);
        createdChildWindows.push_back(text);

        SendMessage(text, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(sc(8), sc(8)));
        applyRoundedRegion(text);
        layoutEditText(text);

        // Thinner trackbar, vertically centered in the row, leaving a gap underneath for
        // the min/max range labels.
        const int barH = std::min(inputHeight, std::max(
            sc(Constants::Win32::SETTINGS_SLIDER_HEIGHT), GetSystemMetrics(SM_CYHSCROLL)));
        const int barY = y + (inputHeight - barH) / 2;
        const int barX = nw + textW + textGap;
        const HWND bar = CreateWindowExW(0, TRACKBAR_CLASSW, L"", Constants::Win32::STYLE_TRACKBAR,
                                         barX, barY, sliderW, barH, window,
                                         nullptr, nullptr, nullptr);
        SendMessage(bar, TBM_SETRANGE, TRUE, MAKELONG(0, Constants::Win32::SLIDER_RESOLUTION));
        SendMessage(bar, WM_SETFONT, font, TRUE);
        // The face is owner-drawn from the parent's WM_NOTIFY; this only keeps the control from
        // painting over it (see trackbarProc).
        SetWindowSubclass(bar, trackbarProc, 1, 0);
        createdChildWindows.push_back(bar);

        int trackLeft = 0;
        int trackRight = 0;
        measureTrackTravel(bar, trackLeft, trackRight);

        std::vector<HWND> rowControls = {label, text, bar};
        const bool showRangeLabels = maxValue >= 1000.0f;
        HWND minLabel = nullptr;
        if (showRangeLabels) {
            const int labelY = barY + barH;
            // Stop short of where the next row begins: running the static all the way down to it
            // left the two boxes touching, so a range label and the row under it had nothing
            // between them. Never shorter than the label's own line, or the text would clip.
            const int labelH = std::max(sc(Constants::Win32::FONT_SIZE_RANGE_LABEL),
                                        y + inputHeight + sc(Constants::Win32::GAP_SETTINGS_INPUT)
                                        - labelY - sc(4));
            // Line the captions up with the ends of the drawn track, not with the trackbar's own
            // rect: the track spans the thumb's travel, which is inset from the control's edges, so
            // a caption pinned to the window edge sits visibly outside the bar it is labelling.
            const bool hasTravel = trackRight > trackLeft;
            const int channelLeft = barX + (hasTravel ? trackLeft : 0);
            const int channelRight = barX + (hasTravel ? trackRight : sliderW);
            const int labelW = std::max(1, (channelRight - channelLeft) / 2);
            minLabel = CreateWindowExW(0, WC_STATICW, unparser(minValue).data(),
                                       WS_CHILD | WS_VISIBLE | SS_LEFT, channelLeft, labelY,
                                       labelW, labelH, window, nullptr,
                                       GetModuleHandleW(nullptr), nullptr);
            const HWND maxLabel = CreateWindowExW(0, WC_STATICW, unparser(maxValue).data(),
                                                  WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                                  channelRight - labelW, labelY,
                                                  labelW, labelH, window, nullptr,
                                                  GetModuleHandleW(nullptr), nullptr);
            SendMessage(minLabel, WM_SETFONT, smallFont, TRUE);
            SendMessage(maxLabel, WM_SETFONT, smallFont, TRUE);
            rangeLabels.insert(minLabel);
            rangeLabels.insert(maxLabel);
            subclassLabel(minLabel);
            subclassLabel(maxLabel);
            createdChildWindows.push_back(minLabel);
            createdChildWindows.push_back(maxLabel);
            rowControls.push_back(minLabel);
            rowControls.push_back(maxLabel);
        }
        registerRowControls(index, std::move(rowControls));

        advanceRow();
        registerActions<float>(ptr, unparser, std::optional{std::move(parser)},
                               std::optional{std::move(validCondition)}, std::move(callback), std::nullopt);

        const auto sliderIdx = static_cast<int>(sliders.size());
        const bool logScale = minValue > 0.0f && maxValue / minValue >= 100.0f;
        const bool wholeSteps = maxValue - minValue > 10.0f;
        sliders.push_back(SliderBinding{
            bar, text, index, static_cast<double>(minValue), static_cast<double>(maxValue),
            static_cast<double>(minValue), logScale, wholeSteps, minLabel, trackLeft, trackRight
        });
        trackbarToSlider.emplace(bar, sliderIdx);
        indexToSlider.emplace(index, sliderIdx);
        setTrackbarFromValue(sliders[sliderIdx], static_cast<double>(*ptr));

        if (!trailingButtonText.empty()) {
            const int buttonX = nw + textW + textGap + sliderW + buttonGap;
            const HWND button = CreateWindowExW(0, WC_BUTTONW, trailingButtonText.data(),
                                                Constants::Win32::STYLE_PUSHBUTTON, buttonX, y, buttonW,
                                                inputHeight, window,
                                                reinterpret_cast<HMENU>(Constants::Win32::ID_OPTIONS + count),
                                                nullptr, nullptr);
            SendMessage(button, WM_SETFONT, font, TRUE);
            SetWindowSubclass(button, ownerDrawnButtonProc, 1, 0);
            createdChildWindows.push_back(button);

            // Dummy action entries so the control index lines up (mirrors registerButton).
            bool dummy = false;
            references.emplace_back(dummy);
            unparsers.emplace_back(std::make_unique<std::function<std::wstring(const std::any &)>>(
                [](const std::any &) { return L""; }));
            parsers.emplace_back(nullptr);
            validConditions.emplace_back(nullptr);
            callbacks.emplace_back(std::make_unique<std::function<void(std::any &)>>(
                [cb = std::move(trailingButtonCallback)](std::any &) { if (cb) cb(); }));
            enumValues.emplace_back(nullptr);
            error.emplace_back(false);
            edited.emplace_back(false);
            modified.emplace_back(false);
            // No value of its own to write back to: a button is not a setting row.
            valuePointers.emplace_back(nullptr);
            ++count;
        }

        adjustWindowHeight();
        return text;
    }


    HWND SettingsWindow::registerColorButton(const std::wstring &settingsName, const std::wstring &buttonText,
                                             std::function<COLORREF()> &&colorProvider,
                                             std::function<void()> &&callback,
                                             const std::wstring &descriptionTitle,
                                             const std::wstring &descriptionDetail,
                                             const void *boundValue) {
        // Same inline layout as registerButton, but the button is owner-drawn so a
        // small black-framed color swatch can be painted inside it (see WM_DRAWITEM).
        const int nw = getFixedNameWidth();
        const int vw = getFixedValueWidth();

        const int index = count;
        const HWND label = createLabel(settingsName, descriptionTitle, descriptionDetail, nw);

        const HWND button = CreateWindowExW(0, WC_BUTTONW, buttonText.data(),
                                            WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_OWNERDRAW,
                                            nw,
                                            getYOffset(),
                                            vw,
                                            inputHeight,
                                            window,
                                            reinterpret_cast<HMENU>(Constants::Win32::ID_OPTIONS + index),
                                            nullptr,
                                            nullptr);

        SendMessage(button, WM_SETFONT, font, TRUE);
        SetWindowSubclass(button, ownerDrawnButtonProc, 1, 0);
        colorSwatches.emplace(button, std::move(colorProvider));
        createdChildWindows.push_back(button);
        registerRowControls(index, {label, button});

        // Dummy action so the index matches (mirrors registerButton), but the
        // callback also repaints the button so a newly-picked color shows at once.
        bool dummy = false;
        references.emplace_back(dummy);
        unparsers.emplace_back(std::make_unique<std::function<std::wstring(const std::any &)>>([](const std::any &) { return L""; }));
        parsers.emplace_back(nullptr);
        validConditions.emplace_back(nullptr);
        callbacks.emplace_back(std::make_unique<std::function<void(std::any &)>>([callback, button](std::any &) {
            callback();
            InvalidateRect(button, nullptr, TRUE);
        }));
        enumValues.emplace_back(nullptr);
        error.emplace_back(false);
        edited.emplace_back(false);
        modified.emplace_back(false);
        // The picker writes the color through the callback rather than through this, but the row is
        // a setting like any other and is named here by the color it stands for.
        valuePointers.emplace_back(boundValue);
        ++count;
        advanceRow();

        adjustWindowHeight();
        return button;
    }


    void SettingsWindow::setSliderFractionalSteps(const HWND textField) {
        const int index = getIndex(textField);
        if (!checkIndex(index)) {
            return;
        }
        if (const auto it = indexToSlider.find(index); it != indexToSlider.end()) {
            sliders[it->second].wholeSteps = false;
        }
    }


    void SettingsWindow::setSliderZeroStop(const HWND textField) {
        const int index = getIndex(textField);
        if (!checkIndex(index)) {
            return;
        }
        if (const auto it = indexToSlider.find(index); it != indexToSlider.end()) {
            SliderBinding &sb = sliders[it->second];
            sb.zeroStop = true;
            // The bar was placed while the binding still had no zero stop, so re-place it now that
            // the bottom window's mapping has changed under it.
            setTrackbarFromValue(sb, static_cast<double>(std::any_cast<float>(references[index])));
            // The caption under the left end was written from the declared minimum, which is only
            // there to put the decades on a power-of-ten grid. What the bar can actually reach now
            // is 0, so say so.
            if (sb.minLabel && unparsers[index]) {
                SetWindowTextW(sb.minLabel, (*unparsers[index])(std::any{0.0f}).c_str());
            }
        }
    }


    void SettingsWindow::setFloatValueByField(const HWND textField, const float value) {
        const int index = getIndex(textField);
        if (!checkIndex(index)) {
            return;
        }
        // Only a value that actually moves counts as modified. Mirroring a field onto the value it
        // already holds - which is what the Palette window's G/B link does the moment the panel is
        // built, and what an animation-mode switch does for the shape fields it leaves alone - was
        // painting those fields in the "changed" color before the user had touched anything.
        if (const float *previous = std::any_cast<float>(&references[index]);
            previous == nullptr || *previous != value) {
            modified[index] = true;
        }
        references[index] = value;
        edited[index] = false;
        SetWindowTextW(textField, currValueToString(index).data());
        if (const auto it = indexToSlider.find(index); it != indexToSlider.end()) {
            setTrackbarFromValue(sliders[it->second], static_cast<double>(value));
        }
    }


    void SettingsWindow::setRadioValueByGroup(const std::vector<HWND> &items, const std::any &value) {
        if (items.empty()) {
            return;
        }
        const int index = getIndex(items.front());
        if (!checkIndex(index)) {
            return;
        }
        references[index] = value;
        // Owner-drawn radios read their selected state from references[index]; repaint the
        // whole group so the previous selection clears and the new one fills.
        for (const HWND item : items) {
            InvalidateRect(item, nullptr, TRUE);
        }
    }


    void SettingsWindow::setCheckboxValue(const HWND checkbox, const bool value) {
        const int index = getIndex(checkbox);
        if (!checkIndex(index)) {
            return;
        }
        // The owner-drawn tick reads references[index], same as a radio's fill does.
        references[index] = value;
        InvalidateRect(checkbox, nullptr, TRUE);
    }


    void SettingsWindow::registerStaticText(const std::wstring &text) {
        RECT rect;
        GetClientRect(window, &rect);
        const int width = rect.right - rect.left - sc(Constants::Win32::GAP_SETTINGS_INPUT) * 2;

        // Measure the real wrapped height: these notes are long single-line strings with no
        // explicit newlines, so counting '\n' under-reports the rows the static word-wraps to.
        const HDC hdc = GetDC(window);
        const auto oldFont = SelectObject(hdc, reinterpret_cast<HFONT>(font));
        RECT calc = {0, 0, width, 0};
        DrawTextW(hdc, text.data(), -1, &calc, DT_LEFT | DT_WORDBREAK | DT_CALCRECT);
        SelectObject(hdc, oldFont);
        ReleaseDC(window, hdc);
        const int textHeight = calc.bottom - calc.top;

        const HWND staticText = CreateWindowExW(0, WC_STATICW, text.data(),
                                                WS_CHILD | WS_VISIBLE | SS_LEFT,
                                                sc(Constants::Win32::GAP_SETTINGS_INPUT),
                                                getYOffset(),
                                                width,
                                                textHeight,
                                                window, nullptr,
                                                GetModuleHandleW(nullptr), nullptr);

        SendMessage(staticText, WM_SETFONT, font, TRUE);
        noteLabels.insert(staticText);
        subclassLabel(staticText);
        createdChildWindows.push_back(staticText);
        // Advance the cursor by the exact measured text height (+ a gap), no row rounding.
        advancePixels(textHeight);
        adjustWindowHeight();
    }


    HWND SettingsWindow::registerOwnerDrawnPanel(const int pixelHeight,
                                                 std::function<void(HDC, const RECT &)> &&painter,
                                                 const std::wstring &rowLabel,
                                                 const std::wstring &descriptionTitle,
                                                 const std::wstring &descriptionDetail) {
        RECT rect;
        GetClientRect(window, &rect);
        const bool asRow = !rowLabel.empty();
        const int margin = sc(Constants::Win32::GAP_SETTINGS_INPUT);
        const int height = pixelHeight > 0 ? pixelHeight : inputHeight;
        const int left = asRow ? getFixedNameWidth() : margin;
        const int width = asRow ? getFixedValueWidth() : rect.right - rect.left - margin * 2;
        if (asRow) {
            createLabel(rowLabel, descriptionTitle, descriptionDetail, left);
        }
        const HWND panel = CreateWindowExW(0, WC_STATICW, L"",
                                           WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                           left, getYOffset(), width, height,
                                           window, nullptr, GetModuleHandleW(nullptr), nullptr);
        cardPainters.emplace(panel, std::move(painter));
        createdChildWindows.push_back(panel);
        if (asRow) {
            // Standing in the value column, it has to carry the same rounded corners as the text
            // fields above and below it.
            applyRoundedRegion(panel);
        }
        advancePixels(height);
        adjustWindowHeight();
        return panel;
    }


    HWND SettingsWindow::registerPrimaryButton(const std::wstring &buttonText, std::function<void()> &&callback,
                                               const std::wstring &descriptionTitle,
                                               const std::wstring &descriptionDetail) {
        (void) descriptionTitle;
        (void) descriptionDetail;
        RECT rect;
        GetClientRect(window, &rect);
        const int margin = sc(Constants::Win32::GAP_SETTINGS_INPUT);
        const int width = rect.right - rect.left - margin * 2;
        const int h = sc(Constants::Win32::PRIMARY_BUTTON_HEIGHT);
        const int gap = sc(Constants::Win32::GAP_SETTINGS_INPUT);
        // A little extra space above the prominent button sets it apart from the card above.
        const int y = getYOffset() + gap;

        const HWND button = CreateWindowExW(0, WC_BUTTONW, buttonText.data(),
                                            Constants::Win32::STYLE_PUSHBUTTON, margin, y, width, h, window,
                                            reinterpret_cast<HMENU>(Constants::Win32::ID_OPTIONS + count),
                                            nullptr, nullptr);
        SendMessage(button, WM_SETFONT, headerFont, TRUE);
        SetWindowSubclass(button, ownerDrawnButtonProc, 1, 0);
        primaryButtons.insert(button);
        createdChildWindows.push_back(button);

        // Dummy action entries so the control index lines up (mirrors registerButton).
        bool dummy = false;
        references.emplace_back(dummy);
        unparsers.emplace_back(std::make_unique<std::function<std::wstring(const std::any &)>>(
            [](const std::any &) { return L""; }));
        parsers.emplace_back(nullptr);
        validConditions.emplace_back(nullptr);
        callbacks.emplace_back(std::make_unique<std::function<void(std::any &)>>(
            [callback](std::any &) { callback(); }));
        enumValues.emplace_back(nullptr);
        error.emplace_back(false);
        edited.emplace_back(false);
        modified.emplace_back(false);
        // No value of its own to write back to: a button is not a setting row.
        valuePointers.emplace_back(nullptr);
        ++count;

        // The button consumed `gap` (above) + h; advancePixels adds the trailing gap below.
        advancePixels(gap + h);
        adjustWindowHeight();
        return button;
    }


    void SettingsWindow::registerNotesCard(const std::wstring &title,
                                           const std::vector<std::pair<std::wstring, std::wstring>> &notes) {
        RECT rect;
        GetClientRect(window, &rect);
        const int margin = sc(Constants::Win32::GAP_SETTINGS_INPUT);
        const int width = rect.right - rect.left - margin * 2;
        // titleGap sets the header apart from the notes; without it the first heading runs
        // straight into the title and the card reads as one undivided block of text.
        const int pad = sc(16), topPad = sc(8), bulletIndent = sc(22), titleH = sc(34),
                  titleGap = sc(12), lineGap = sc(2), noteGap = sc(14);
        const int textLeft = pad + bulletIndent;
        const int textWidth = width - textLeft - pad;

        const HFONT bodyFont = reinterpret_cast<HFONT>(font);
        const HFONT boldFont = reinterpret_cast<HFONT>(headerFont);

        // Measure each note's wrapped heading/detail height up front so the card is sized
        // to fit (these lines word-wrap, so a fixed per-note height would clip them).
        const HDC mdc = GetDC(window);
        // Where a heading's glyphs actually sit inside its first line. DrawText lays the line out
        // from the top of the font cell, and that cell opens with tmInternalLeading of empty space,
        // so the visible run spans [internalLeading, ascent] and its middle is their average. The
        // bullet used to take a fixed sc(9), which put it 4px above this.
        const int headingCentre = [&] {
            const auto of = SelectObject(mdc, boldFont);
            TEXTMETRICW htm;
            GetTextMetricsW(mdc, &htm);
            SelectObject(mdc, of);
            return static_cast<int>(htm.tmInternalLeading + htm.tmAscent) / 2;
        }();
        auto measure = [&](const HFONT fnt, const std::wstring &s) {
            const auto of = SelectObject(mdc, fnt);
            RECT c = {0, 0, textWidth, 0};
            DrawTextW(mdc, s.c_str(), -1, &c, DT_LEFT | DT_WORDBREAK | DT_CALCRECT);
            SelectObject(mdc, of);
            return static_cast<int>(c.bottom - c.top);
        };
        std::vector<int> headH, detailH;
        headH.reserve(notes.size());
        detailH.reserve(notes.size());
        int notesH = 0;
        for (size_t i = 0; i < notes.size(); ++i) {
            const int th = measure(boldFont, notes[i].first);
            const int dh = measure(bodyFont, notes[i].second);
            headH.push_back(th);
            detailH.push_back(dh);
            notesH += th + lineGap + dh;
            if (i + 1 < notes.size()) {
                notesH += noteGap;
            }
        }
        ReleaseDC(window, mdc);
        const int height = topPad + titleH + titleGap + notesH + pad;

        auto painter = [=, this](const HDC hdc, const RECT &rc) {
            drawCardFace(hdc, rc, settingsTheme().cardNoteBackground, settingsTheme().cardNoteBorder);
            SetBkMode(hdc, TRANSPARENT);
            const int x = rc.left + pad;
            int y = rc.top + topPad;

            // Header: info icon + bold blue title. The title is centered in the titleH band,
            // so center the icon on that same band instead of pinning it to the top - a fixed
            // top offset leaves it riding above the text it labels.
            const int icoSz = sc(26);
            const int icoTop = y + (titleH - icoSz) / 2;
            RECT ico = {x, icoTop, x + icoSz, icoTop + icoSz};
            drawInfoIcon(hdc, ico, boldFont, settingsTheme().cardNoteAccent);
            SelectObject(hdc, boldFont);
            SetTextColor(hdc, settingsTheme().cardNoteAccent);
            RECT tr = {x + icoSz + sc(12), y, rc.right - pad, y + titleH};
            DrawTextW(hdc, title.c_str(), -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            y += titleH + titleGap;

            const int tl = rc.left + textLeft;
            const int tw = rc.right - pad - tl;
            for (size_t i = 0; i < notes.size(); ++i) {
                // Round bullet aligned to the heading's first line. Outlined in its own color
                // rather than with a NULL pen, which would clip the bullet's right/bottom edge
                // and leave the little disc looking lopsided (see the radio dot).
                const HBRUSH bb = CreateSolidBrush(settingsTheme().cardNoteAccent);
                const HPEN bp = CreatePen(PS_SOLID, 1, settingsTheme().cardNoteAccent);
                const auto ob = SelectObject(hdc, bb);
                const auto op = SelectObject(hdc, bp);
                const int bx = rc.left + pad + sc(6);
                const int bd = sc(7);
                // Centre the disc on the heading's glyphs. Measured against the rasteriser rather
                // than derived: GDI leaves the top row of a disc this small empty, so its ink sits
                // one row lower than the bounding box says.
                const int by = y + headingCentre - bd / 2;
                Ellipse(hdc, bx, by, bx + bd, by + bd);
                SelectObject(hdc, op);
                SelectObject(hdc, ob);
                DeleteObject(bb);
                DeleteObject(bp);

                // Heading line (bold, dark).
                SelectObject(hdc, boldFont);
                SetTextColor(hdc, settingsTheme().cardNoteTitle);
                RECT hr = {tl, y, tl + tw, y + headH[i]};
                DrawTextW(hdc, notes[i].first.c_str(), -1, &hr, DT_LEFT | DT_WORDBREAK);
                y += headH[i] + lineGap;

                // Detail line (muted).
                SelectObject(hdc, bodyFont);
                SetTextColor(hdc, settingsTheme().rangeText);
                RECT dr = {tl, y, tl + tw, y + detailH[i]};
                DrawTextW(hdc, notes[i].second.c_str(), -1, &dr, DT_LEFT | DT_WORDBREAK);
                y += detailH[i] + noteGap;
            }
        };
        // Stand the card off from the rows around it. At the plain row gap the card butts up
        // against the last setting and whatever follows, so a block of prose reads as one more
        // control in the stack; the extra breathing room on both sides marks it as an aside.
        const int standoff = sc(Constants::Win32::GAP_SETTINGS_INPUT);
        yCursor += standoff;
        registerOwnerDrawnPanel(height, std::move(painter));
        yCursor += standoff;
        adjustWindowHeight();
    }


    void SettingsWindow::registerHelpButton(const std::wstring &title,
                                            const std::vector<std::pair<std::wstring, std::wstring>> &notes) {
        std::wstring text;
        for (size_t i = 0; i < notes.size(); ++i) {
            text += notes[i].first + L"\n" + notes[i].second;
            if (i + 1 < notes.size()) {
                text += L"\n\n";
            }
        }
        registerButton(L"Help", L"Open Guide", [parent = window, title, text] {
            MessageBoxW(parent, text.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
        }, title, text);
    }


    void SettingsWindow::registerInsertChips(const HWND targetField, const std::vector<std::wstring> &chips,
                                             const bool replaceWhole) {
        const int margin = sc(Constants::Win32::GAP_SETTINGS_INPUT);
        RECT rect;
        GetClientRect(window, &rect);
        const int avail = rect.right - rect.left - margin * 2;
        const int rowH = inputHeight;
        // Step rows by the same pitch as normal element rows so the laid-out height
        // matches the cursor advance below.
        const int rowStep = rowH + sc(Constants::Win32::GAP_SETTINGS_INPUT);
        const int chipGap = sc(Constants::Win32::GAP_SETTINGS_COLOR_SWATCH);
        const int padX = sc(16); // horizontal text padding inside each chip
        const int baseY = getYOffset();

        // Measure chip widths with the window font so each chip hugs its label.
        const HDC hdc = GetDC(window);
        const auto oldFont = SelectObject(hdc, reinterpret_cast<HFONT>(font));

        int x = 0;
        int row = 0;
        for (const std::wstring &chip : chips) {
            SIZE sz;
            GetTextExtentPoint32W(hdc, chip.data(), static_cast<int>(chip.size()), &sz);
            int w = sz.cx + padX * 2;
            if (w > avail) {
                w = avail;
            }
            // Wrap to the next row when this chip would overflow the current one.
            if (x > 0 && x + w > avail) {
                x = 0;
                ++row;
            }
            const int chipX = margin + x;
            const int chipY = baseY + row * rowStep;

            const HWND btn = CreateWindowExW(0, WC_BUTTONW, chip.data(),
                                             Constants::Win32::STYLE_PUSHBUTTON, chipX, chipY, w, rowH,
                                             window,
                                             reinterpret_cast<HMENU>(Constants::Win32::ID_OPTIONS + count),
                                             nullptr, nullptr);
            SendMessage(btn, WM_SETFONT, font, TRUE);
            SetWindowSubclass(btn, ownerDrawnButtonProc, 1, 0);
            createdChildWindows.push_back(btn);

            // Dummy action entries so the control index lines up (mirrors registerButton).
            bool dummy = false;
            references.emplace_back(dummy);
            unparsers.emplace_back(std::make_unique<std::function<std::wstring(const std::any &)>>(
                [](const std::any &) { return L""; }));
            parsers.emplace_back(nullptr);
            validConditions.emplace_back(nullptr);
            callbacks.emplace_back(std::make_unique<std::function<void(std::any &)>>(
                [this, targetField, snippet = chip, replaceWhole](std::any &) {
                    if (replaceWhole) {
                        // Select all (for undo) then overwrite with the full example.
                        SendMessageW(targetField, EM_SETSEL, 0, -1);
                    }
                    SendMessageW(targetField, EM_REPLACESEL, TRUE,
                                 reinterpret_cast<LPARAM>(snippet.data()));
                    SetFocus(targetField);
                    // Color the field as edited so it reads as "unsaved, press Enter".
                    if (const int ti = getIndex(targetField); checkIndex(ti)) {
                        edited[ti] = true;
                    }
                    InvalidateRect(targetField, nullptr, TRUE);
                }));
            enumValues.emplace_back(nullptr);
            error.emplace_back(false);
            edited.emplace_back(false);
            modified.emplace_back(false);
            // No value of its own to write back to: a button is not a setting row.
            valuePointers.emplace_back(nullptr);
            ++count;

            x += w + chipGap;
        }

        SelectObject(hdc, oldFont);
        ReleaseDC(window, hdc);

        // (row + 1) laid-out rows of chips.
        yCursor += (row + 1) * (inputHeight + sc(Constants::Win32::GAP_SETTINGS_INPUT));
        adjustWindowHeight();
    }


    void SettingsWindow::registerSectionHeader(const std::wstring &title, const bool separateFromPrevious) {
        if (yCursor == 0) {
            topMargin = sc(Constants::Win32::GAP_SETTINGS_INPUT / 3);
        }
        RECT rect;
        GetClientRect(window, &rect);
        const int fullWidth = rect.right - rect.left;
        const int pad = sc(Constants::Win32::SETTINGS_LABEL_LEFT_PADDING);
        const int margin = sc(Constants::Win32::GAP_SETTINGS_INPUT);
        const int gap = sc(Constants::Win32::GAP_SETTINGS_INPUT);

        // Keep the heading from hugging the previous section's last row: nudge it down by half
        // a gap (skipped for the very first header, where topMargin already insets it).
        if (yCursor != 0) {
            yCursor += gap / 2;
        }
        const int y = getYOffset();

        // Each header is its own collapsible section; the disclosure arrow + title share this id.
        const int sectionIndex = static_cast<int>(sections.size());
        const auto toggleId = reinterpret_cast<HMENU>(Constants::Win32::ID_SECTION_TOGGLE + sectionIndex);
        // This header opens a new band, so the previous section's body ends at this header's top.
        if (!sections.empty()) {
            sections.back().bodyEnd = separateFromPrevious ? y : y + gap;
        }

        // A full gap above the title keeps the heading from looking cramped against the
        // section that ends above it.
        const int headerY = y + gap;
        // Clickable disclosure arrow tucked into the title's left indent. The glyph is built from
        // its code point (U+25BC open / U+25B6 folded) so the source stays plain ASCII. It starts
        // hidden and is only shown once the panel grows long enough to be worth collapsing.
        const wchar_t arrowGlyph[2] = {static_cast<wchar_t>(0x25BC), 0};
        const HWND arrow = CreateWindowExW(0, WC_STATICW, arrowGlyph,
                                           WS_CHILD | (sectionsCollapsible ? WS_VISIBLE : 0) |
                                           SS_CENTER | SS_CENTERIMAGE | SS_NOTIFY,
                                           margin, headerY, pad - margin, inputHeight,
                                           window, toggleId, GetModuleHandleW(nullptr), nullptr);
        SendMessage(arrow, WM_SETFONT, font, TRUE);
        subclassLabel(arrow);
        createdChildWindows.push_back(arrow);

        const HWND header = CreateWindowExW(0, WC_STATICW, title.data(),
                                            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE | SS_NOTIFY,
                                            pad, headerY,
                                            fullWidth - pad - margin,
                                            inputHeight,
                                            window, toggleId, GetModuleHandleW(nullptr), nullptr);
        SendMessage(header, WM_SETFONT, headerFont, TRUE);
        subclassLabel(header);
        createdChildWindows.push_back(header);

        // advanceRow() only accounts for the title sitting half a row down; add the rest so
        // the first item still keeps a full gap below the (now lower) title.
        advanceRow();
        yCursor += gap;
        sections.push_back(CollapsibleSection{header, arrow, getYOffset(), -1, false});
        adjustWindowHeight();
    }


    void SettingsWindow::setRowPreview(const HWND control, std::function<void(HDC, const RECT &)> &&painter) {
        rowPreviews[control] = std::move(painter);
        SetWindowPos(control, nullptr, 0, 0, getFixedValueWidth(), inputHeight,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        InvalidateRect(control, nullptr, TRUE);
    }


    void SettingsWindow::setRowEnabled(const HWND control, const bool enabled) {
        const int index = getIndex(control);
        const auto it = rowControlGroups.find(index);
        if (it == rowControlGroups.end()) {
            if (IsWindowEnabled(control) == enabled) {
                return;
            }
            EnableWindow(control, enabled);
            InvalidateRect(control, nullptr, TRUE);
            return;
        }
        for (const HWND item : it->second) {
            if (IsWindowEnabled(item) == enabled) {
                continue;
            }
            EnableWindow(item, enabled);
            InvalidateRect(item, nullptr, TRUE);
        }
    }

    void SettingsWindow::disableRowsInObjectExcept(const void *object, const size_t size,
                                                   const std::unordered_set<const void *> &kept) {
        if (object == nullptr) {
            return;
        }
        const auto *begin = static_cast<const std::byte *>(object);
        const auto *end = begin + size;
        for (int index = 0; index < static_cast<int>(valuePointers.size()); ++index) {
            const void *value = valuePointers[index];
            const auto *at = static_cast<const std::byte *>(value);
            // A row bound outside the object is one the panel holds for itself - which layer it is
            // pointed at, what it is staging - and is left as it is.
            if (value == nullptr || std::less<>{}(at, begin) || !std::less<>{}(at, end) || kept.contains(value)) {
                continue;
            }
            const auto it = rowControlGroups.find(index);
            if (it == rowControlGroups.end()) {
                continue;
            }
            for (const HWND item: it->second) {
                if (!IsWindowEnabled(item)) {
                    continue;
                }
                EnableWindow(item, FALSE);
                InvalidateRect(item, nullptr, TRUE);
            }
        }
    }

    void SettingsWindow::setRowVisible(const HWND control, const bool visible) {
        const int index = getIndex(control);
        const auto it = rowControlGroups.find(index);
        // Track the caller's intent and apply it, but keep a row hidden while its section is
        // folded so a collapsed section can't be partially re-revealed through this path.
        // Reports whether the row's shown state actually moved: a slider drag re-asserts the same
        // visibility on every mouse move, and repainting the window for that flashes the panel.
        const auto apply = [&](const HWND item) {
            if (visible) {
                rowHidden.erase(item);
            } else {
                rowHidden.insert(item);
            }
            const bool show = visible && !isControlFolded(item);
            if (show == ((GetWindowLongPtrW(item, GWL_STYLE) & WS_VISIBLE) != 0)) {
                return false;
            }
            ShowWindow(item, show ? SW_SHOWNA : SW_HIDE);
            return true;
        };
        if (it == rowControlGroups.end()) {
            if (apply(control)) {
                // Repaint for the same reason as the grouped path below: the section frame is
                // drawn around the rows that are actually showing.
                schedulePanelRepaint();
            }
            return;
        }
        bool changed = false;
        for (const HWND item : it->second) {
            changed |= apply(item);
        }
        if (changed) {
            // Not a bare InvalidateRect: that left the panel's surface to be redrawn on the next
            // trip through the message loop, one frame after the row it belongs to had already
            // appeared or gone.
            schedulePanelRepaint();
        }
    }


    void SettingsWindow::setWindowCloseFunction(std::function<void()> &&function) {
        this->windowCloseFunction = std::move(function);
    }
}
