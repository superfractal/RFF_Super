//
// Modified by GPT-6 on 2026-09-14, 2026-09-15, 2026-09-22
//

#include "SurfaceResetMenu.hpp"
#include "../SettingsMenu.hpp"
#include "../UiDpi.hpp"
#include <oleacc.h>

namespace merutilm::rff2::workspace {
    namespace {
        struct ResetMenuEntry {
            MSAAMENUINFO accessibility{};
            SettingsMenu::MenuEntry paint;
            std::wstring caption;
            bool separator = false;
            bool note = false;
        };
    } // namespace
    bool SurfaceResetMenu::measure(HWND owner, MEASUREITEMSTRUCT *item) {
        if (!item || item->CtlType != ODT_MENU || !item->itemData) {
            return false;
        }
        auto *entry = reinterpret_cast<ResetMenuEntry *>(item->itemData);
        if (entry->accessibility.dwMSAASignature != MSAA_MENU_SIG) {
            return false;
        }
        if (entry->separator) {
            item->itemWidth = 0;
            item->itemHeight = UiDpi::pixels(9, entry->paint.metrics->dpi);
            return true;
        }
        auto measureRequest = *item;
        measureRequest.itemData = reinterpret_cast<ULONG_PTR>(&entry->paint);
        if (!SettingsMenu::measureMenuItem(owner, &measureRequest)) {
            return false;
        }
        item->itemWidth = measureRequest.itemWidth;
        item->itemHeight = measureRequest.itemHeight;
        return true;
    }
    bool SurfaceResetMenu::draw(const DRAWITEMSTRUCT *item) {
        if (!item || item->CtlType != ODT_MENU || !item->itemData) {
            return false;
        }
        const auto *entry = reinterpret_cast<const ResetMenuEntry *>(item->itemData);
        if (entry->accessibility.dwMSAASignature != MSAA_MENU_SIG) {
            return false;
        }
        if (entry->separator || entry->note) {
            const auto &metrics = *entry->paint.metrics;
            const bool dark = darkSettingsMode();
            const auto backgroundBrush =
                CreateSolidBrush(dark ? settingsTheme().background : GetSysColor(COLOR_MENU));
            FillRect(item->hDC, &item->rcItem, backgroundBrush);
            DeleteObject(backgroundBrush);
            auto contentBounds = item->rcItem;
            contentBounds.left += metrics.gutter();
            contentBounds.right -= metrics.rightPadding();
            if (entry->separator) {
                contentBounds.top = (contentBounds.top + contentBounds.bottom) / 2;
                contentBounds.bottom = contentBounds.top + 1;
                const auto separatorBrush =
                    CreateSolidBrush(dark ? settingsTheme().sectionFrame : GetSysColor(COLOR_3DSHADOW));
                FillRect(item->hDC, &contentBounds, separatorBrush);
                DeleteObject(separatorBrush);
            } else {
                const auto previousFont = SelectObject(item->hDC, metrics.font);
                SetBkMode(item->hDC, TRANSPARENT);
                SetTextColor(item->hDC, dark ? settingsTheme().rangeText : GetSysColor(COLOR_MENUTEXT));
                DrawTextW(item->hDC, entry->caption.c_str(), -1, &contentBounds,
                          DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
                SelectObject(item->hDC, previousFont);
            }
            return true;
        }
        auto drawRequest = *item;
        drawRequest.itemData = reinterpret_cast<ULONG_PTR>(&entry->paint);
        return SettingsMenu::drawMenuItem(&drawRequest);
    }
    int SurfaceResetMenu::show(HWND owner, POINT point, const std::vector<Item> &items, UINT dpi) {
        const auto popupMenu = CreatePopupMenu();
        if (!popupMenu) {
            return 0;
        }
        SettingsMenu::MenuMetrics metrics;
        metrics.owner = owner;
        metrics.refresh(dpi);
        std::deque<ResetMenuEntry> entries;
        const auto append = [&](int command, std::wstring caption, bool enabled, bool separator = false) {
            caption = UiLanguage::text(caption);
            std::wstring escapedCaption;
            for (wchar_t character : caption) {
                escapedCaption += character;
                if (character == L'&') {
                    escapedCaption += character;
                }
            }
            const int utf8Length = WideCharToMultiByte(
                CP_UTF8, 0, escapedCaption.data(), int(escapedCaption.size()), nullptr, 0, nullptr, nullptr);
            std::string utf8Caption(utf8Length, '\0');
            WideCharToMultiByte(CP_UTF8, 0, escapedCaption.data(), int(escapedCaption.size()),
                                utf8Caption.data(), utf8Length, nullptr, nullptr);
            entries.push_back(
                {{},
                 {popupMenu, UINT(GetMenuItemCount(popupMenu)), std::move(utf8Caption), false, false},
                 std::move(caption)});
            auto &entry = entries.back();
            entry.paint.metrics = &metrics;
            entry.separator = separator;
            entry.note = command == 0 && !separator;
            entry.accessibility = {MSAA_MENU_SIG, DWORD(entry.caption.size()), entry.caption.data()};
            AppendMenuW(popupMenu,
                        MF_OWNERDRAW | (separator ? MF_SEPARATOR : 0) | (enabled ? MF_ENABLED : MF_GRAYED),
                        command, reinterpret_cast<LPCWSTR>(&entry));
        };
        for (const auto &item : items) {
            if (item.command == APPEARANCE) {
                append(0, L"", false, true);
            }
            append(item.command, item.label, item.enabled);
        }
        append(0, L"", false, true);
        append(0, L"All: Classic 1 palette; default shading and effects", false);
        append(0, L"Keeps color motion, camera, view and Rendering FPS", false);
        const auto backgroundBrush =
            CreateSolidBrush(darkSettingsMode() ? settingsTheme().background : GetSysColor(COLOR_MENU));
        MENUINFO menuInfo{sizeof(menuInfo)};
        menuInfo.fMask = MIM_BACKGROUND;
        menuInfo.hbrBack = backgroundBrush;
        SetMenuInfo(popupMenu, &menuInfo);
        const int selectedCommand = TrackPopupMenuEx(
            popupMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON, point.x, point.y, owner, nullptr);
        DestroyMenu(popupMenu);
        DeleteObject(backgroundBrush);
        return selectedCommand;
    }
} // namespace merutilm::rff2::workspace
