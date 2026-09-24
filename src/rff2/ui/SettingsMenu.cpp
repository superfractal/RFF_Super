//
// Created by Merutilm on 2025-05-14.
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-05, 2026-08-06, 2026-08-07, 2026-08-08, 2026-08-09, 2026-08-10, 2026-08-11, 2026-08-12, 2026-08-13, 2026-08-14, 2026-08-15, 2026-08-16, 2026-08-17, 2026-08-18, 2026-08-19, 2026-08-20, 2026-08-23, 2026-08-27, 2026-08-31, 2026-09-03
// Modified by GPT-5 on 2026-08-18, 2026-08-21, 2026-08-23, 2026-08-24, 2026-08-26, 2026-08-27, 2026-09-01
// Modified by GPT-6 on 2026-09-08, 2026-09-10, 2026-09-11, 2026-09-13, 2026-09-14, 2026-09-15, 2026-09-17, 2026-09-18, 2026-09-19, 2026-09-20, 2026-09-21, 2026-09-22, 2026-09-23
//

#include "NativeDialogs.hpp"
#include "LocalAiWindow.hpp"
#include "SettingsMenu.hpp"
#include "UiLanguage.hpp"
#include "../io/PreferencesIO.h"
#include "UiDpi.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <windows.h>
#include <unordered_map>

#include "../constants/Constants.hpp"
#include "CallbackDebug.hpp"
#include "CallbackExplore.hpp"
#include "CallbackFile.hpp"
#include "CallbackFractal.hpp"
#include "CallbackRender.hpp"
#include "CallbackShader.hpp"
#include "CallbackVideo.hpp"
#include "RenderScene.hpp"
#include "../preset/render/RenderPresets.h"
#include "../preset/resolution/ResolutionPresets.h"
#include "../preset/calc/CalculationPresets.h"
#include "../preset/shader/bloom/ShdBloomPresets.h"
#include "../preset/shader/color/ShdColorPresets.h"
#include "../preset/shader/example/ShdExamplePresets.h"
#include "../preset/shader/fog/ShdFogPresets.h"
#include "../preset/shader/palette/ShdPalettePresets.h"
#include "../preset/shader/slope/ShdSlopePresets.h"
#include "../preset/shader/stripe/ShdStripePresets.h"

namespace merutilm::rff2 {
    // Solid fill behind the bar and every popup, rebuilt whenever the theme's panel color changes.
    static HBRUSH menuBackgroundBrush() {
        static HBRUSH brush = nullptr;
        static COLORREF color = 0;
        if (brush == nullptr || color != settingsTheme().background) {
            if (brush != nullptr) {
                DeleteObject(brush);
            }
            color = settingsTheme().background;
            brush = CreateSolidBrush(color);
        }
        return brush;
    }

    // The face the system would have drawn the menu in, so an owner-drawn bar keeps the size and
    // weight of the one it replaces.
    bool SettingsMenu::MenuMetrics::refresh(const UINT targetDpi) {
        if (targetDpi == 0) {
            return false;
        }
        using Query = BOOL(WINAPI *)(UINT, UINT, PVOID, UINT, UINT);
        static const auto query = reinterpret_cast<Query>(
            GetProcAddress(GetModuleHandleW(L"user32.dll"), "SystemParametersInfoForDpi"));
        NONCLIENTMETRICSW system = {};
        system.cbSize = sizeof(system);
        if (!query || !query(SPI_GETNONCLIENTMETRICS, sizeof(system), &system, 0, targetDpi)) {
            if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(system), &system, 0)) {
                GetObjectW(GetStockObject(DEFAULT_GUI_FONT), sizeof(system.lfMenuFont), &system.lfMenuFont);
            }
            const UINT sourceDpi = UiDpi::forWindow(nullptr);
            system.lfMenuFont.lfHeight = MulDiv(system.lfMenuFont.lfHeight, targetDpi, sourceDpi);
            system.lfMenuFont.lfWidth = MulDiv(system.lfMenuFont.lfWidth, targetDpi, sourceDpi);
        }
        const HFONT replacement = CreateFontIndirectW(&system.lfMenuFont);
        if (replacement == nullptr) {
            return false;
        }
        const HDC dc = GetDC(nullptr);
        if (dc == nullptr) {
            DeleteObject(replacement);
            return false;
        }
        const auto previousFont = SelectObject(dc, replacement);
        TEXTMETRICW text = {};
        const bool measured = GetTextMetricsW(dc, &text) != FALSE;
        SelectObject(dc, previousFont);
        ReleaseDC(nullptr, dc);
        if (!measured) {
            DeleteObject(replacement);
            return false;
        }
        if (font) {
            DeleteObject(font);
        }
        font = replacement;
        dpi = targetDpi;
        averageCharWidth = std::max(1, static_cast<int>(text.tmAveCharWidth));
        return true;
    }

    SettingsMenu::MenuMetrics::~MenuMetrics() {
        if (font) {
            DeleteObject(font);
        }
    }

    static bool refreshMenuItemMeasurement(const SettingsMenu::MenuEntry &entry, const bool dark) {
        const auto caption = UiLanguage::utf8(entry.text);
        MENUITEMINFOW item = {};
        item.cbSize = sizeof(item);
        item.fMask =
            MIIM_FTYPE | MIIM_ID | MIIM_STATE | MIIM_SUBMENU | MIIM_DATA | MIIM_CHECKMARKS | MIIM_BITMAP;
        if (!GetMenuItemInfoW(entry.parent, entry.position, TRUE, &item)) {
            return false;
        }
        if (!dark) {
            item.fMask |= MIIM_STRING;
            item.dwTypeData = const_cast<LPWSTR>(caption.c_str());
            item.cch = static_cast<UINT>(caption.size());
        }
        // A fresh item discards Windows' cached owner-draw dimensions; RemoveMenu keeps the existing submenu alive.
        if (!InsertMenuItemW(entry.parent, entry.position, TRUE, &item)) {
            return false;
        }
        if (RemoveMenu(entry.parent, entry.position + 1, MF_BYPOSITION)) {
            return true;
        }
        RemoveMenu(entry.parent, entry.position, MF_BYPOSITION);
        return false;
    }

    // The columns the menu itself lays a popup item out in: the check bitmap's own width plus the
    // gap to the caption on the left, and the submenu arrow's width on the right. Kept to the
    // system metric so an owner-drawn popup comes out the width the system would have given it.
    int SettingsMenu::MenuMetrics::gutter() const {
        // Owner-draw needs a full bitmap column before the native caption gap begins.
        return 2 * UiDpi::metric(SM_CXMENUCHECK, dpi) + averageCharWidth / 2;
    }

    int SettingsMenu::MenuMetrics::rightPadding() const {
        return 3 * UiDpi::metric(SM_CXMENUCHECK, dpi);
    }

    int SettingsMenu::MenuMetrics::arrowColumn() const {
        return UiDpi::metric(SM_CXMENUCHECK, dpi);
    }

    SettingsMenu::SettingsMenu(const HMENU hMenubar) : menubar(hMenubar) {
        menuMetrics.refresh(UiDpi::forWindow(nullptr));
        configure();
    }

    SettingsMenu::~SettingsMenu() {
        DestroyMenu(menubar);
        for (const auto &menu : childMenus) {
            DestroyMenu(menu);
        }
    }

    void SettingsMenu::attachMasterWindow(const HWND window) {
        masterWindow = window;
        menuMetrics.owner = window;
        menuMetrics.refresh(UiDpi::forWindow(window));
        menuNativeSizesCaptured = false;
    }

    void SettingsMenu::applyDpi(const UINT dpi, const bool force) {
        if ((!force && menuMetrics.dpi == dpi) || !menuMetrics.refresh(dpi)) {
            return;
        }
        menuNativeSizesCaptured = false;
        menuMeasurementsDirty = true;
        if (force) {
            for (MenuEntry &entry : menuEntries) {
                entry.nativeWidth = 0;
                entry.nativeHeight = 0;
            }
        }
        applyMenuTheme();
    }

    void SettingsMenu::captureNativeMenuSizes() {
        // Only while the bar is still the system's to size; once the items are owner-drawn the
        // rects come back as whatever was last measured here, which would lock that guess in.
        if (menuNativeSizesCaptured || menuOwnerDrawn || masterWindow == nullptr || menubar == nullptr ||
            UiDpi::forWindow(masterWindow) != menuMetrics.dpi) {
            return;
        }
        bool captured = false;
        for (MenuEntry &entry : menuEntries) {
            if (!entry.topLevel) {
                continue;
            }
            // Screen coordinates, but only the size is wanted, and a bar item has one whether or
            // not the bar has wrapped to a second row.
            if (RECT rect; GetMenuItemRect(masterWindow, menubar, entry.position, &rect) &&
                           rect.right > rect.left && rect.bottom > rect.top) {
                entry.nativeWidth = static_cast<int>(rect.right - rect.left);
                entry.nativeHeight = static_cast<int>(rect.bottom - rect.top);
                entry.nativeDpi = menuMetrics.dpi;
                captured = true;
            }
        }
        menuNativeSizesCaptured = captured;
    }

    void SettingsMenu::applyMenuTheme() {
        // Before anything is made owner-drawn, while the bar is still sized by the system.
        captureNativeMenuSizes();
        const bool dark = darkSettingsMode() && !highContrastSettingsMode();
        const bool refreshMeasurements = menuMeasurementsDirty || menuOwnerDrawn != dark;
        bool measurementsRefreshed = true;
        // MIM_APPLYTOSUBMENUS reaches every popup hanging off the bar, so the fill is set once.
        MENUINFO menuInfo = {};
        menuInfo.cbSize = sizeof(menuInfo);
        menuInfo.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
        // Only dark needs a brush of its own. Light clears it back to none, which is what returns
        // the menu to the themed drawing path the system lays a popup out with.
        menuInfo.hbrBack = dark ? menuBackgroundBrush() : nullptr;
        SetMenuInfo(menubar, &menuInfo);

        for (MenuEntry &entry : menuEntries) {
            // MIIM_FTYPE cannot move an item between MFT_STRING and MFT_OWNERDRAW - the display
            // type is fixed once the item exists, and asking through it silently leaves the item
            // owner-drawn, which is what kept the menu drawing itself after a switch back to light.
            // MIIM_TYPE is the older pair that carries the type and its data together, and it can -
            // but it rewrites fType whole, so MFT_RIGHTJUSTIFY is put back from what add() recorded
            // rather than read off an item whose type is about to be replaced.
            const auto caption = UiLanguage::utf8(entry.text);
            MENUITEMINFOW info = {};
            info.cbSize = sizeof(info);
            info.fMask = MIIM_TYPE | MIIM_DATA;
            info.fType = (entry.rightJustified ? static_cast<UINT>(MFT_RIGHTJUSTIFY) : 0u) |
                         (dark ? static_cast<UINT>(MFT_OWNERDRAW) : static_cast<UINT>(MFT_STRING));
            info.dwItemData = reinterpret_cast<ULONG_PTR>(&entry);
            // Under MIIM_TYPE dwTypeData is read as the caption of a string item and as the item's
            // own value for an owner-drawn one; both are what this entry already holds.
            info.dwTypeData = dark ? reinterpret_cast<LPWSTR>(&entry) : const_cast<LPWSTR>(caption.c_str());
            info.cch = static_cast<UINT>(caption.size());
            SetMenuItemInfoW(entry.parent, entry.position, TRUE, &info);
            if (refreshMeasurements && !refreshMenuItemMeasurement(entry, dark)) {
                measurementsRefreshed = false;
            }
        }
        menuMeasurementsDirty = !measurementsRefreshed;
        menuOwnerDrawn = dark;
        if (masterWindow != nullptr) {
            DrawMenuBar(masterWindow);
            paintMenuBarUnderline(masterWindow);
        }
    }

    void SettingsMenu::paintMenuBarUnderline(const HWND window) {
        if (!darkSettingsMode() || highContrastSettingsMode() || window == nullptr ||
            GetMenu(window) == nullptr) {
            return;
        }
        MENUBARINFO bar = {};
        bar.cbSize = sizeof(bar);
        if (!GetMenuBarInfo(window, OBJID_MENU, 0, &bar)) {
            return;
        }
        RECT frame;
        GetWindowRect(window, &frame);
        POINT clientTop = {0, 0};
        ClientToScreen(window, &clientTop);
        // Everything between the bottom of the bar and the top of the client area belongs to the
        // frame, which draws a light rule there that the bar's own background never reaches.
        const RECT line = {bar.rcBar.left - frame.left, bar.rcBar.bottom - frame.top,
                           bar.rcBar.right - frame.left, clientTop.y - frame.top};
        if (line.bottom <= line.top || line.right <= line.left) {
            return;
        }
        const HDC hdc = GetWindowDC(window);
        FillRect(hdc, &line, menuBackgroundBrush());
        ReleaseDC(window, hdc);
    }

    bool SettingsMenu::measureMenuItem(const HWND owner, MEASUREITEMSTRUCT *measure) {
        if (measure == nullptr || measure->CtlType != ODT_MENU) {
            return false;
        }
        const auto *entry = reinterpret_cast<const MenuEntry *>(measure->itemData);
        if (entry == nullptr || entry->metrics == nullptr) {
            return false;
        }
        const HDC hdc = GetDC(owner);
        const auto &layout = *entry->metrics;
        const auto previousFont = SelectObject(hdc, layout.font);
        SIZE size = {};
        const auto caption = UiLanguage::utf8(entry->text);
        GetTextExtentPoint32W(hdc, caption.c_str(), static_cast<int>(caption.size()), &size);
        TEXTMETRICW metrics;
        GetTextMetricsW(hdc, &metrics);
        SelectObject(hdc, previousFont);
        ReleaseDC(owner, hdc);

        if (entry->topLevel) {
            // The width the system had given this item, so the bar reads exactly as it does in
            // light mode. Never narrower than the caption: the menu font here and the one the
            // system drew with need not agree to the pixel, and a clipped bar item is worse than
            // one a shade wide. The guessed padding is the fallback for a bar never measured.
            const int fallback = size.cx + 2 * metrics.tmAveCharWidth;
            const int finalWidth =
                entry->nativeWidth > 0
                    ? std::max<int>(MulDiv(entry->nativeWidth, layout.dpi, entry->nativeDpi),
                                    size.cx + metrics.tmAveCharWidth)
                    : fallback;
            // Windows adds the check-mark bitmap width minus its shared edge to an owner-drawn item.
            const int systemAddedWidth =
                std::max(0, UiDpi::metric(SM_CXMENUCHECK, UiDpi::forWindow(layout.owner)) - 1);
            measure->itemWidth = std::max<int>(1, finalWidth - systemAddedWidth);
            measure->itemHeight =
                entry->nativeHeight > 0
                    ? std::max<int>(MulDiv(entry->nativeHeight, layout.dpi, entry->nativeDpi),
                                    metrics.tmHeight)
                    : std::max<int>(UiDpi::metric(SM_CYMENU, layout.dpi), metrics.tmHeight);
        } else {
            measure->itemWidth = layout.gutter() + size.cx + layout.rightPadding();
            const int contentHeight =
                std::max<int>(UiDpi::metric(SM_CYMENU, layout.dpi), metrics.tmHeight + metrics.tmHeight / 3);
            // Native popup rows retain one system border above and below their content height.
            measure->itemHeight = contentHeight + 2 * UiDpi::metric(SM_CYBORDER, layout.dpi);
        }
        return true;
    }

    bool SettingsMenu::drawMenuItem(const DRAWITEMSTRUCT *draw) {
        if (draw == nullptr || draw->CtlType != ODT_MENU) {
            return false;
        }
        const auto *entry = reinterpret_cast<const MenuEntry *>(draw->itemData);
        if (entry == nullptr || entry->metrics == nullptr) {
            return false;
        }
        const auto caption = UiLanguage::utf8(entry->text);
        const RECT rc = draw->rcItem;
        const bool dark = darkSettingsMode();
        const bool disabled = (draw->itemState & (ODS_GRAYED | ODS_DISABLED)) != 0;
        const bool selected = (draw->itemState & (ODS_SELECTED | ODS_HOTLIGHT)) != 0 && !disabled;
        // Outside dark mode the system's own menu colors, so an item that is still owner-drawn for
        // any reason is not a light-gray panel pretending to be a menu.
        const COLORREF back = selected ? dark ? settingsTheme().primaryButton : GetSysColor(COLOR_HIGHLIGHT)
                              : dark   ? settingsTheme().background
                                       : GetSysColor(COLOR_MENU);
        const COLORREF fore =
            disabled   ? dark ? settingsTheme().textDisabled : GetSysColor(COLOR_GRAYTEXT)
            : selected ? dark ? settingsTheme().primaryButtonText : GetSysColor(COLOR_HIGHLIGHTTEXT)
            : dark     ? settingsTheme().text
                       : GetSysColor(COLOR_MENUTEXT);
        const HBRUSH backBrush = CreateSolidBrush(back);
        FillRect(draw->hDC, &rc, backBrush);
        DeleteObject(backBrush);

        const auto &layout = *entry->metrics;
        const auto previousFont = SelectObject(draw->hDC, layout.font);
        SetBkMode(draw->hDC, TRANSPARENT);
        SetBkColor(draw->hDC, back);
        SetTextColor(draw->hDC, fore);
        if (entry->topLevel) {
            RECT tr = rc;
            DrawTextW(draw->hDC, caption.c_str(), -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        } else {
            RECT tr = rc;
            tr.left += layout.gutter();
            tr.right -= layout.rightPadding();
            DrawTextW(draw->hDC, caption.c_str(), -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            // The system draws neither the tick nor the submenu arrow for an owner-drawn item.
            const int cy = (rc.top + rc.bottom) / 2;
            if ((draw->itemState & ODS_CHECKED) != 0) {
                // Sized against the row rather than against SM_CXMENUCHECK, which is the width of
                // the whole check column and draws a tick far heavier than the caption beside it.
                const int box = std::max<int>(6, (rc.bottom - rc.top) / 2);
                const int cx = rc.left + layout.arrowColumn();
                const HPEN pen = CreatePen(PS_SOLID, std::max(1, box / 6), fore);
                const auto oldPen = SelectObject(draw->hDC, pen);
                POINT tick[3] = {
                    {cx - box / 2, cy + box / 12},
                    {cx - box / 8, cy + box / 3},
                    {cx + box / 2, cy - box / 3},
                };
                Polyline(draw->hDC, tick, 3);
                SelectObject(draw->hDC, oldPen);
                DeleteObject(pen);
            }
            if (entry->submenu) {
                // Align the contrasting arrow with the native glyph using the owning window's actual DPI.
                const int nativeColumn = UiDpi::metric(SM_CXMENUCHECK, UiDpi::forWindow(layout.owner));
                const int arrow = std::max(2, nativeColumn / 3);
                const HBRUSH fill = CreateSolidBrush(fore);
                const auto oldBrush = SelectObject(draw->hDC, fill);
                const auto oldPen = SelectObject(draw->hDC, GetStockObject(NULL_PEN));
                const int right = rc.right - nativeColumn / 2;
                POINT head[3] = {
                    {right - arrow, cy - arrow},
                    {right, cy},
                    {right - arrow, cy + arrow},
                };
                Polygon(draw->hDC, head, 3);
                SelectObject(draw->hDC, oldPen);
                SelectObject(draw->hDC, oldBrush);
                DeleteObject(fill);
            }
        }
        SelectObject(draw->hDC, previousFont);
        return true;
    }

    HMENU SettingsMenu::addWorkspaceItem(HMENU target, std::string_view child, int workspace, int section,
                                         const std::function<void(SettingsMenu &, RenderScene &)> &fallback) {
        return addChildItem(target, child,
                            [workspace, section, fallback](SettingsMenu &menu, RenderScene &scene) {
                                if (menu.workspaceNavigation && !PreferencesIO::legacySettingsUI()) {
                                    menu.workspaceNavigation(workspace, section);
                                } else {
                                    fallback(menu, scene);
                                }
                            });
    }

    void SettingsMenu::configure() {
        HMENU currentMenu = nullptr;
        HMENU subMenu1 = nullptr;
        HMENU subMenu2 = nullptr;

        currentMenu = addChildMenu(menubar, "File");
        addChildItem(currentMenu, "New Document", CallbackFile::NEW_CONFIG);
        addChildItem(currentMenu, "Save Settings",
                     [](SettingsMenu &, RenderScene &scene) { CallbackFile::saveCurrentConfig(scene); });
        // Every save first, then every load, each group in the same order of kind:
        // the map, the picture, then the settings.
        addChildItem(currentMenu, "Save Map", CallbackFile::SAVE_MAP);
        addChildItem(currentMenu, "Save Image", CallbackFile::SAVE_IMAGE);
        addChildItem(currentMenu, "Save Location / Settings", CallbackFile::SAVE_CONFIG);
        addChildItem(currentMenu, "Load Map", CallbackFile::LOAD_MAP);
        addChildItem(currentMenu, "Load Image", CallbackFile::LOAD_IMAGE);
        addChildItem(currentMenu, "Load Location / Settings", CallbackFile::LOAD_CONFIG);

        currentMenu = addChildMenu(menubar, "Fractal");
        addWorkspaceItem(currentMenu, "Reference", 0, 2, CallbackFractal::REFERENCE);
        addWorkspaceItem(currentMenu, "Iterations", 0, 1, CallbackFractal::ITERATIONS);
        addWorkspaceItem(currentMenu, "MP-Approximation", 0, 3, CallbackFractal::MPA);
        addChildCheckbox(currentMenu, "Automatic Iterations", CallbackFractal::AUTOMATIC_ITERATIONS);
        addChildCheckbox(currentMenu, "Absolute Iteration Mode", CallbackFractal::ABSOLUTE_ITERATION_MODE);
        addWorkspaceItem(currentMenu, "Formula", 0, 4, CallbackFractal::FORMULA);
        // The 360 projections are still being worked on, so a release build does not offer them.
        // Projection is now available in every build.
        addWorkspaceItem(currentMenu, "Projection", 0, 5, CallbackFractal::PROJECTION);

        currentMenu = addChildMenu(menubar, "Render");
        addWorkspaceItem(currentMenu, "Render Properties", 3, 2, CallbackRender::SET_CLARITY);
        addWorkspaceItem(currentMenu, "Rendering FPS", 20, 0, CallbackRender::SET_CLARITY);
        addChildCheckbox(currentMenu, "Linear Interpolation", CallbackRender::LINEAR_INTERPOLATION);
        addChildCheckbox(currentMenu, "Boundary Trace Fill", CallbackRender::BOUNDARY_TRACE_FILL);
        addChildCheckbox(currentMenu, "2-Color Preview Mode", CallbackRender::PREVIEW_2COLOR);
        addChildCheckbox(currentMenu, "Coarse Preview", CallbackRender::COARSE_PREVIEW);
        addChildCheckbox(currentMenu, "Smooth Zooming", CallbackRender::SMOOTH_ZOOM);
        addChildCheckbox(currentMenu, "Dither", CallbackRender::DITHER);
        currentMenu = addChildMenu(menubar, "Shader");
        addWorkspaceItem(currentMenu, "Palette", 16, 0, CallbackShader::PALETTE);
        addWorkspaceItem(currentMenu, "Texture", 11, 0, CallbackShader::TEXTURE);
        addWorkspaceItem(currentMenu, "Pattern", 12, 0, CallbackShader::PATTERN);
        addWorkspaceItem(currentMenu, "Warp", 13, 0, CallbackShader::WARP);
        addWorkspaceItem(currentMenu, "Stripe", 13, 1, CallbackShader::STRIPE);
        addWorkspaceItem(currentMenu, "Slope", 10, 0, CallbackShader::SLOPE);
        addWorkspaceItem(currentMenu, "Color", 14, 0, CallbackShader::COLOR);
        addWorkspaceItem(currentMenu, "Effects", 15, 0, CallbackShader::EFFECTS);
        addWorkspaceItem(currentMenu, "Fog", 14, 2, CallbackShader::FOG);
        addWorkspaceItem(currentMenu, "Bloom", 14, 1, CallbackShader::BLOOM);
        addWorkspaceItem(currentMenu, "HDR", 3, 4, CallbackShader::HDR);
        addWorkspaceItem(currentMenu, "Shader Layers", 18, 0, CallbackShader::LAYERS);
        addWorkspaceItem(currentMenu, "Load KFR Color", 16, 5, CallbackShader::LOAD_KFR_PALETTE);
        addWorkspaceItem(currentMenu, "Import Color", 16, 5, CallbackShader::IMPORT_COLOR);
        addChildItem(currentMenu, "Local AI appearance",
                     [](SettingsMenu &menu, RenderScene &scene) { LocalAiWindow::open(menu, scene); });
        addChildItem(currentMenu, "Save Shader Preset", CallbackShader::SAVE_PRESET);
        addChildItem(currentMenu, "Load Shader Preset", CallbackShader::LOAD_PRESET);

        currentMenu = addChildMenu(menubar, "Preset");
        subMenu1 = addChildMenu(currentMenu, "Calculation");
        addPresetExecutor(subMenu1, CalculationPresets::UltraFast());
        addPresetExecutor(subMenu1, CalculationPresets::Fast());
        addPresetExecutor(subMenu1, CalculationPresets::Normal());
        addPresetExecutor(subMenu1, CalculationPresets::Best());
        addPresetExecutor(subMenu1, CalculationPresets::UltraBest());
        addPresetExecutor(subMenu1, CalculationPresets::Stable());
        addPresetExecutor(subMenu1, CalculationPresets::MoreStable());
        addPresetExecutor(subMenu1, CalculationPresets::UltraStable());
        subMenu1 = addChildMenu(currentMenu, "Render");
        addPresetExecutor(subMenu1, RenderPresets::Potato());
        addPresetExecutor(subMenu1, RenderPresets::Low());
        addPresetExecutor(subMenu1, RenderPresets::Medium());
        addPresetExecutor(subMenu1, RenderPresets::High());
        addPresetExecutor(subMenu1, RenderPresets::Ultra());
        addPresetExecutor(subMenu1, RenderPresets::Extreme());
        subMenu1 = addChildMenu(currentMenu, "Resolution");
        addPresetExecutor(subMenu1, ResolutionPresets::L1());
        addPresetExecutor(subMenu1, ResolutionPresets::L2());
        addPresetExecutor(subMenu1, ResolutionPresets::L3());
        addPresetExecutor(subMenu1, ResolutionPresets::L4());
        addPresetExecutor(subMenu1, ResolutionPresets::L5());
        addPresetExecutor(subMenu1, ResolutionPresets::L6());
        subMenu1 = addChildMenu(currentMenu, "Shader");
        subMenu2 = addChildMenu(subMenu1, "Palette");
        addPresetExecutor(subMenu2, ShdPalettePresets::LongRandom64());
        addPresetExecutor(subMenu2, ShdPalettePresets::RandomSmooth());
        addPresetExecutor(subMenu2, ShdPalettePresets::Classic1());
        addPresetExecutor(subMenu2, ShdPalettePresets::Classic2());
        addPresetExecutor(subMenu2, ShdPalettePresets::ArcticAurora());
        addPresetExecutor(subMenu2, ShdPalettePresets::Azure());
        addPresetExecutor(subMenu2, ShdPalettePresets::Cinematic());
        addPresetExecutor(subMenu2, ShdPalettePresets::CrimsonMagma());
        addPresetExecutor(subMenu2, ShdPalettePresets::DeepSpace());
        addPresetExecutor(subMenu2, ShdPalettePresets::Desert());
        addPresetExecutor(subMenu2, ShdPalettePresets::ElectricDreams());
        addPresetExecutor(subMenu2, ShdPalettePresets::Flame());
        addPresetExecutor(subMenu2, ShdPalettePresets::GlossyBerry());
        addPresetExecutor(subMenu2, ShdPalettePresets::GlossyCyber());
        addPresetExecutor(subMenu2, ShdPalettePresets::GlossyFire());
        addPresetExecutor(subMenu2, ShdPalettePresets::GlossyForest());
        addPresetExecutor(subMenu2, ShdPalettePresets::GlossyIce());
        addPresetExecutor(subMenu2, ShdPalettePresets::GlossyMetal());
        addPresetExecutor(subMenu2, ShdPalettePresets::GlossyNeon());
        addPresetExecutor(subMenu2, ShdPalettePresets::GlossyOcean());
        addPresetExecutor(subMenu2, ShdPalettePresets::GlossyPastel());
        addPresetExecutor(subMenu2, ShdPalettePresets::GlossySunset());
        addPresetExecutor(subMenu2, ShdPalettePresets::LongRainbow7());
        addPresetExecutor(subMenu2, ShdPalettePresets::MidnightNeon());
        addPresetExecutor(subMenu2, ShdPalettePresets::MistyForest());
        addPresetExecutor(subMenu2, ShdPalettePresets::PastelDream());
        addPresetExecutor(subMenu2, ShdPalettePresets::Rainbow());
        addPresetExecutor(subMenu2, ShdPalettePresets::VolcanicAsh());
        addPresetExecutor(subMenu2, ShdPalettePresets::LongRandom64_2());
        subMenu2 = addChildMenu(subMenu1, "Stripe");
        addPresetExecutor(subMenu2, ShdStripePresets::Disabled());
        addPresetExecutor(subMenu2, ShdStripePresets::SlowAnimated());
        addPresetExecutor(subMenu2, ShdStripePresets::FastAnimated());
        addPresetExecutor(subMenu2, ShdStripePresets::Smooth());
        addPresetExecutor(subMenu2, ShdStripePresets::SmoothTranslucent());
        subMenu2 = addChildMenu(subMenu1, "Slope");
        addPresetExecutor(subMenu2, ShdSlopePresets::Disabled());
        addPresetExecutor(subMenu2, ShdSlopePresets::Normal1());
        addPresetExecutor(subMenu2, ShdSlopePresets::Normal2());
        subMenu2 = addChildMenu(subMenu1, "Color");
        addPresetExecutor(subMenu2, ShdColorPresets::Disabled());
        addPresetExecutor(subMenu2, ShdColorPresets::WeakContrast());
        addPresetExecutor(subMenu2, ShdColorPresets::HighContrast());
        addPresetExecutor(subMenu2, ShdColorPresets::Dull());
        addPresetExecutor(subMenu2, ShdColorPresets::Vivid());
        subMenu2 = addChildMenu(subMenu1, "Fog");
        addPresetExecutor(subMenu2, ShdFogPresets::Disabled());
        addPresetExecutor(subMenu2, ShdFogPresets::Low());
        addPresetExecutor(subMenu2, ShdFogPresets::Medium());
        addPresetExecutor(subMenu2, ShdFogPresets::High());
        addPresetExecutor(subMenu2, ShdFogPresets::Ultra());
        subMenu2 = addChildMenu(subMenu1, "Bloom");
        addPresetExecutor(subMenu2, BloomPresets::Disabled());
        addPresetExecutor(subMenu2, BloomPresets::Highlighted());
        addPresetExecutor(subMenu2, BloomPresets::HighlightedStrong());
        addPresetExecutor(subMenu2, BloomPresets::Weak());
        addPresetExecutor(subMenu2, BloomPresets::Normal());
        addPresetExecutor(subMenu2, BloomPresets::Strong());
        // Read out of the example folder rather than written down here, so another look is added to
        // this menu by dropping its settings file in and naming the file what the menu should say.
        if (const std::vector<ShdExamplePresets::FromFile> examples = ShdExamplePresets::collect();
            !examples.empty()) {
            subMenu2 = addChildMenu(subMenu1, "Example");
            for (const auto &example : examples) {
                addFullShaderPreset(subMenu2, example);
            }
        }
        currentMenu = addChildMenu(menubar, "Video");
        addWorkspaceItem(currentMenu, "Data Settings", 2, 4, CallbackVideo::DATA_SETTINGS);
        addWorkspaceItem(currentMenu, "Animation Settings", 2, 0, CallbackVideo::ANIMATION_SETTINGS);
        addWorkspaceItem(currentMenu, "Timeline Editor", 2, 3, CallbackVideo::TIMELINE_EDITOR);
        addWorkspaceItem(currentMenu, "Export Settings", 3, 3, CallbackVideo::EXPORT_SETTINGS);
        addChildItem(currentMenu, "Generate Video Keyframe", CallbackVideo::GENERATE_VID_KEYFRAME);
        addChildItem(currentMenu, "Export Zooming Video", CallbackVideo::EXPORT_ZOOM_VID);
        currentMenu = addChildMenu(menubar, "View");
        const auto languageMenu = addChildMenu(currentMenu, "Language / 言語 (restart required)");
        for (const auto language : {Language::English, Language::Japanese}) {
            addChildItem(languageMenu, language == Language::English ? "English" : "日本語",
                         [languageMenu, language](SettingsMenu &, RenderScene &) {
                             UiLanguage::choose(language);
                             PreferencesIO::save();
                             CheckMenuRadioItem(languageMenu, 0, 1, UINT(language), MF_BYPOSITION);
                         });
        }
        CheckMenuRadioItem(languageMenu, 0, 1, UINT(UiLanguage::preferred()), MF_BYPOSITION);
        add(
            currentMenu, "Show Setting Descriptions",
            [](SettingsMenu &, RenderScene &) { PreferencesIO::save(); }, false, true,
            [](RenderScene &, bool) -> bool * { return &PreferencesIO::showSettingDescriptions(); });
        add(
            currentMenu, "Use Legacy Settings UI",
            [](SettingsMenu &, RenderScene &) { PreferencesIO::save(); }, false, true,
            [](RenderScene &, bool) -> bool * { return &PreferencesIO::legacySettingsUI(); });
        addChildCheckbox(currentMenu, "Dark Mode",
                         [this]([[maybe_unused]] RenderScene &scene, const bool executeMode) -> bool * {
                             if (executeMode) {
                                 // Both of these are posted rather than applied here: the caller
                                 // flips the flag only after this returns, so anything drawn now
                                 // would still be reading the theme that is on its way out.
                                 for (const auto &active : activeSettingsWindows) {
                                     active.window->scheduleThemeRefresh();
                                 }
                                 if (masterWindow != nullptr) {
                                     PostMessageW(masterWindow, Constants::Win32::WM_MAIN_THEME_CHANGED, 0,
                                                  0);
                                 }
                             }
                             return &darkSettingsModeFlag();
                         });
        currentMenu = addChildMenu(menubar, "Explore");
        addChildItem(currentMenu, "Recompute", CallbackExplore::RECOMPUTE);
        addChildItem(currentMenu, "Cancel", CallbackExplore::CANCEL_RENDER);
        addChildItem(currentMenu, "Reset", CallbackExplore::RESET);
        addChildItem(currentMenu, "Find Center", CallbackExplore::FIND_CENTER);
        addChildItem(currentMenu, "Locate Minibrot", CallbackExplore::LOCATE_MINIBROT);
#ifndef NDEBUG
        currentMenu = addChildMenu(menubar, "Debug");
        addChildItem(currentMenu, "Dump Scene State", CallbackDebug::DUMP_SCENE_STATE);
        // Timestamp marks cost a write per pass, so they are recorded only while this is on. Turning
        // it on drops what an earlier measurement gathered, which would otherwise be averaged in.
        addChildCheckbox(currentMenu, "Measure GPU Pass Times",
                         [](RenderScene &scene, const bool executeMode) -> bool * {
                             if (executeMode && !scene.passTimingFlag()) {
                                 scene.clearPassTiming();
                             }
                             return &scene.passTimingFlag();
                         });
        addChildItem(currentMenu, "Show GPU Pass Times", CallbackDebug::SHOW_PASS_TIMES);
#endif

        // Added last and pushed to the right edge of the bar, where a help mark has sat since the
        // menu bar was invented, so it is found without crowding the groups that are worked in.
        currentMenu = addChildMenu(menubar, "?");
        addChildItem(currentMenu, "Version", [](const SettingsMenu &, const RenderScene &) {
            NativeDialogs::message(
                nullptr, std::string("RFF_Super ").append(Constants::Win32::APPLICATION_VERSION).c_str(),
                "Version", MB_OK | MB_ICONINFORMATION);
        });
        // MFT_RIGHTJUSTIFY right-aligns this menu and every one after it, so it must stay the last
        // one added. MIIM_FTYPE leaves the caption and the popup it carries alone.
        MENUITEMINFOW rightJustified = {};
        rightJustified.cbSize = sizeof(rightJustified);
        rightJustified.fMask = MIIM_FTYPE;
        rightJustified.fType = MFT_STRING | MFT_RIGHTJUSTIFY;
        const auto lastPosition = static_cast<UINT>(GetMenuItemCount(menubar) - 1);
        SetMenuItemInfoW(menubar, lastPosition, TRUE, &rightJustified);
        // Remembered, because applyMenuTheme rewrites this item's type whole every time the theme
        // changes and would otherwise drop it back among the menus on the left.
        for (MenuEntry &entry : menuEntries) {
            if (entry.parent == menubar && entry.position == lastPosition) {
                entry.rightJustified = true;
            }
        }
    }

    HMENU SettingsMenu::addChildMenu(const HMENU target, const std::string_view child) {
        return add(
            target, child,
            [](const SettingsMenu &, const RenderScene &) {
                //it is "MENU", The callback not required
            },
            true, false, std::nullopt);
    }

    HMENU SettingsMenu::addChildItem(const HMENU target, const std::string_view child,
                                     const std::function<void(SettingsMenu &, RenderScene &)> &callback) {
        return add(target, child, callback, false, false, std::nullopt);
    }

    HMENU SettingsMenu::addChildCheckbox(const HMENU target, const std::string_view child,
                                         const std::function<bool *(RenderScene &, bool)> &checkboxAction) {
        return add(target, child, [](SettingsMenu &, RenderScene &) {}, false, true, checkboxAction);
    }

    HMENU SettingsMenu::add(const HMENU target, const std::string_view child,
                            const std::function<void(SettingsMenu &, RenderScene &)> &callback,
                            const bool hasChild, const bool hasCheckbox,
                            const std::optional<std::function<bool *(RenderScene &, bool)>> &checkboxAction) {
        // CreatePopupMenu, not CreateMenu: the latter makes a menu *bar*, and one hung off an item
        // as a submenu is laid out as a bar the moment the menu is drawn without visual styles -
        // which is what a custom MIM_BACKGROUND brush switches the whole menu over to. The popup
        // then came up the right size and completely empty.
        const HMENU hmenu = hasChild ? CreatePopupMenu() : nullptr;

        if (hasChild) {
            AppendMenuW(target, MF_POPUP, reinterpret_cast<UINT_PTR>(hmenu), UiLanguage::utf8(child).c_str());
        } else {
            AppendMenuW(target, 0, Constants::Win32::ID_MENUS + count++, UiLanguage::utf8(child).c_str());
            callbacks.emplace_back(callback);
            hasCheckboxes.emplace_back(hasCheckbox);
            checkboxActions.emplace_back(checkboxAction);
        }

        // The caption is kept here and pinned to the item as its item data, which is the only thing
        // an owner-drawn item is handed back when it has to draw itself. See applyMenuTheme.
        const auto position = static_cast<UINT>(GetMenuItemCount(target) - 1);
        menuEntries.push_back(MenuEntry{target, position, std::string(child), hasChild, target == menubar});
        menuEntries.back().metrics = &menuMetrics;
        MENUITEMINFOW itemData = {};
        itemData.cbSize = sizeof(itemData);
        itemData.fMask = MIIM_DATA;
        itemData.dwItemData = reinterpret_cast<ULONG_PTR>(&menuEntries.back());
        SetMenuItemInfoW(target, position, TRUE, &itemData);

        if (hmenu != nullptr) {
            childMenus.push_back(hmenu);
        }
        return hmenu;
    }

    MenuModel SettingsMenu::menuModel(RenderScene &scene) const {
        MenuModel model;
        std::unordered_map<HMENU, int> parents{{menubar, 0}};
        for (const auto &entry : menuEntries) {
            const auto parent = parents.find(entry.parent);
            if (parent == parents.end()) {
                continue;
            }
            const int parentId = parent->second;
            MenuNode node;
            node.id = int(model.nodes.size());
            node.caption = UiLanguage::utf8(entry.text);
            if (!entry.text.empty()) {
                node.access = std::towupper(static_cast<unsigned char>(entry.text.front()));
            }
            if (const auto tab = node.caption.find(L'\t'); tab != std::wstring::npos) {
                node.shortcut = node.caption.substr(tab + 1);
                node.caption.resize(tab);
            }
            node.rightAligned = entry.rightJustified;
            MENUITEMINFOW info{};
            info.cbSize = sizeof(info);
            info.fMask = MIIM_ID | MIIM_STATE | MIIM_FTYPE | MIIM_SUBMENU;
            if (!GetMenuItemInfoW(entry.parent, entry.position, TRUE, &info)) {
                continue;
            }
            node.separator = (info.fType & MFT_SEPARATOR) != 0;
            node.enabled = (info.fState & (MFS_DISABLED | MFS_GRAYED)) == 0;
            if (entry.submenu) {
                parents[info.hSubMenu] = node.id;
            } else {
                node.command = info.wID;
                const int index = getIndex(info.wID);
                node.enabled = node.enabled && !scene.isLongJobBusy() && checkIndex(index);
                node.checkable = checkIndex(index) && hasCheckboxes[index];
                if (node.checkable && !scene.isLongJobBusy()) {
                    if (const bool *value = getBool(scene, info.wID, false)) {
                        node.checked = *value;
                    }
                }
            }
            model.nodes[parentId].children.push_back(node.id);
            model.nodes.push_back(std::move(node));
        }
        return model;
    }

    void SettingsMenu::dispatchCommand(RenderScene &scene, const UINT menuID) {
        if (scene.isLongJobBusy() || !checkIndex(getIndex(menuID))) {
            return;
        }
        const auto state = GetMenuState(menubar, menuID, MF_BYCOMMAND);
        if (state == UINT(-1) || (state & (MF_DISABLED | MF_GRAYED))) {
            return;
        }
        if (hasCheckbox(menuID)) {
            bool *value = getBool(scene, menuID, true);
            if (!value) {
                return;
            }
            *value = !*value;
            const bool checked = *value;
            executeAction(scene, menuID);
            CheckMenuItem(menubar, menuID, MF_BYCOMMAND | (checked ? MF_CHECKED : MF_UNCHECKED));
        } else {
            executeAction(scene, menuID);
        }
    }

    void SettingsMenu::executeAction(RenderScene &scene, const int menuID) {
        std::erase_if(activeSettingsWindows, [](const ActiveSettingsWindow &active) {
            return !IsWindow(active.window->getWindow());
        });
        if (const auto id = getIndex(menuID); checkIndex(id)) {
            callbacks[id](*this, scene);
        }
    }

    bool SettingsMenu::hasCheckbox(const int menuID) {
        if (const auto id = getIndex(menuID); checkIndex(id)) {
            return hasCheckboxes[id];
        }
        return false;
    }

    void SettingsMenu::setCurrentActiveSettingsWindow(std::unique_ptr<SettingsWindow> &&scene,
                                                      const SettingsWindowKind kind) {
        if (scene == nullptr) {
            return;
        }
        std::erase_if(activeSettingsWindows, [](const ActiveSettingsWindow &active) {
            return !IsWindow(active.window->getWindow());
        });
        const int cascade = static_cast<int>(activeSettingsWindows.size() % 8);
        if (cascade > 0) {
            RECT rect;
            MONITORINFO monitorInfo = {sizeof(monitorInfo)};
            GetWindowRect(scene->getWindow(), &rect);
            const int wantedOffset = Constants::Win32::settingsScaled(16) * cascade;
            if (GetMonitorInfoW(MonitorFromWindow(scene->getWindow(), MONITOR_DEFAULTTONEAREST),
                                &monitorInfo)) {
                // Keep the stagger visible without letting the settings window cross the work-area bottom.
                const int availableOffset = std::max(0L, monitorInfo.rcWork.bottom - rect.bottom);
                OffsetRect(&rect, 0, std::min(wantedOffset, availableOffset));
            } else {
                OffsetRect(&rect, 0, wantedOffset);
            }
            SetWindowPos(scene->getWindow(), nullptr, rect.left, rect.top, 0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        activeSettingsWindows.emplace_back(ActiveSettingsWindow{std::move(scene), kind});
    }

    void SettingsMenu::closeAllSettingsWindows() {
        // Each destructor destroys its own window, which takes its rows with it. WM_CLOSE is not
        // sent, so no panel's close function runs back into this vector while it is being emptied.
        activeSettingsWindows.clear();
    }

    HMENU SettingsMenu::addFullShaderPreset(HMENU target, const ShdExamplePresets::FromFile &preset) {
        return add(
            target, preset.getName(),
            [p1 = preset](SettingsMenu &menu, RenderScene &scene) {
                // Every shader panel is bound to the values this replaces, so none of them can stay open.
                menu.closeShaderSettingsWindows();
                scene.changePreset(p1);
                CallbackFile::warnMissingTextureImages(scene.getAttribute().shader);
            },
            false, false, std::nullopt);
    }

    void SettingsMenu::closeShaderSettingsWindows() {
        std::erase_if(activeSettingsWindows, [](const ActiveSettingsWindow &active) {
            return active.kind != SettingsWindowKind::OTHER;
        });
    }

    void SettingsMenu::closePaletteSettingsWindows() {
        std::erase_if(activeSettingsWindows, [](const ActiveSettingsWindow &active) {
            return active.kind == SettingsWindowKind::SHADER_PALETTE;
        });
    }

    bool *SettingsMenu::getBool(RenderScene &scene, const int menuID, const bool executeMode) const {
        if (const auto id = getIndex(menuID); checkIndex(id)) {
            return checkboxActions[id] == std::nullopt ? nullptr : (*checkboxActions[id])(scene, executeMode);
        }
        return nullptr;
    }

    int SettingsMenu::getIndex(const int menuID) {
        return menuID - Constants::Win32::ID_MENUS;
    }

    bool SettingsMenu::checkIndex(const int index) const {
        return index >= 0 && index < callbacks.size();
    }
} // namespace merutilm::rff2
