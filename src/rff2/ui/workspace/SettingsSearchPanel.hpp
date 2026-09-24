//
// Modified by GPT-6 on 2026-09-14, 2026-09-15, 2026-09-22, 2026-09-23
//

#pragma once
#include "../UiLanguage.hpp"
#include "SettingsSearchIndex.hpp"
#include "AccessibleItems.hpp"
#include "PanelBackBuffer.hpp"
#include "PanelDrawing.hpp"
#include "WorkspaceTheme.hpp"
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <windowsx.h>

namespace merutilm::rff2::workspace {
    class SettingsSearchPanel {
        struct Row {
            RECT bounds{};
            RECT label{};
            RECT path{};
            RECT value{};
            RECT state{};
        };
        HWND window = nullptr;
        HFONT font;
        HFONT heading;
        const WorkspaceTheme &theme;
        float scale;
        int scroll = 0;
        int contentHeight = 0;
        int selected = 0;
        int wheelRemainder = 0;
        std::vector<SettingsSearchResult> results;
        std::vector<Row> rows;
        PanelBackBuffer buffer;
        std::unique_ptr<AccessibleItems> accessibility;
        std::function<void(SettingsSearchTarget)> open;
        std::function<void(bool)> leave;
        static constexpr long browseId = 1;

        int px(int value) const {
            return int(value * scale + .5f);
        }
        RECT bounds() const {
            RECT clientBounds{};
            GetClientRect(window, &clientBounds);
            return clientBounds;
        }
        RECT offset(RECT rectangle) const {
            OffsetRect(&rectangle, 0, -scroll);
            return rectangle;
        }
        RECT browseBounds() const {
            return {px(20), px(174), bounds().right - px(20), px(210)};
        }
        std::wstring countText() const {
            return std::to_wstring(results.size()) +
                   (results.size() == 1 ? L" result across all settings" : L" results across all settings");
        }
        int measure(HDC dc, std::wstring_view value, int width) const {
            RECT textBounds{0, 0, std::max(1, width), 0};
            UiLanguage::drawText(dc, value.data(), int(value.size()), &textBounds,
                                 DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
            return textBounds.bottom;
        }
        void wrap(HDC dc, std::wstring_view value, RECT textBounds, COLORREF color) const {
            SelectObject(dc, font);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, color);
            UiLanguage::drawText(dc, value.data(), int(value.size()), &textBounds,
                                 DT_WORDBREAK | DT_NOPREFIX);
        }
        void invalidate() {
            InvalidateRect(window, nullptr, FALSE);
            if (accessibility) {
                accessibility->changed();
            }
        }
        void setScroll(int value) {
            scroll = std::clamp(value, 0, std::max(0, contentHeight - int(bounds().bottom)));
            SCROLLINFO info{sizeof(info), SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL};
            info.nMax = std::max(0, contentHeight - 1);
            info.nPage = std::max(1L, bounds().bottom);
            info.nPos = scroll;
            SetScrollInfo(window, SB_VERT, &info, TRUE);
            invalidate();
        }
        void arrange() {
            if (!window) {
                return;
            }
            const auto clientBounds = bounds();
            const int inset = px(20);
            const int width = std::max(1, int(clientBounds.right) - 2 * inset);
            const auto dc = GetDC(window);
            const auto previousFont = SelectObject(dc, font);
            rows.clear();
            int top = px(88);
            for (const auto &result : results) {
                Row row;
                row.bounds = {px(8), top - px(12), clientBounds.right - px(8), 0};
                row.label = {inset, top, clientBounds.right - inset, top + measure(dc, result.label, width)};
                row.path = {inset, row.label.bottom + px(6), clientBounds.right - inset, 0};
                row.path.bottom = row.path.top + measure(dc, result.path, width);
                row.value = {inset, row.path.bottom + px(6), clientBounds.right - inset,
                             row.path.bottom + px(26)};
                SIZE stateSize{};
                GetTextExtentPoint32W(dc, result.state.data(), int(result.state.size()), &stateSize);
                row.state = row.value;
                row.state.left = std::max(row.value.left + width / 2, row.value.right - stateSize.cx);
                if (!result.state.empty()) {
                    row.value.right = row.state.left - px(12);
                }
                row.bounds.bottom = row.value.bottom + px(12);
                top = row.bounds.bottom + px(13);
                rows.push_back(row);
            }
            SelectObject(dc, previousFont);
            ReleaseDC(window, dc);
            contentHeight = results.empty() ? px(234) : top;
            setScroll(scroll);
        }
        void reveal() {
            const auto selectedBounds = results.empty() ? browseBounds() : rows[selected].bounds;
            if (selectedBounds.top < scroll) {
                setScroll(selectedBounds.top);
            } else if (selectedBounds.bottom > scroll + bounds().bottom) {
                setScroll(std::min(int(selectedBounds.top), int(selectedBounds.bottom - bounds().bottom)));
            } else {
                invalidate();
            }
        }
        void select(int index) {
            selected = std::clamp(index, 0, std::max(0, int(results.size()) - 1));
            SetFocus(window);
            reveal();
        }
        void activate() {
            if (results.empty()) {
                leave(true);
                return;
            }
            const auto target = results[selected].target;
            open(target);
        }
        std::vector<AccessibleItem> items() const {
            std::vector<AccessibleItem> accessibleItems;
            if (results.empty()) {
                accessibleItems.push_back({browseId, L"Browse settings",
                                           L"Clear the search and return to the current settings.", L"Browse",
                                           offset(browseBounds())});
            }
            for (size_t rowIndex = 0; rowIndex < results.size(); ++rowIndex) {
                const auto &entry = results[rowIndex];
                AccessibleItem item;
                item.id = entry.identity;
                item.name = entry.label;
                item.help = entry.path + L". " + entry.state;
                item.value = entry.value;
                item.action = L"Open setting";
                item.role = ROLE_SYSTEM_LISTITEM;
                item.bounds = offset(rows[rowIndex].bounds);
                item.state |= STATE_SYSTEM_SELECTABLE;
                if (int(rowIndex) == selected) {
                    item.state |= STATE_SYSTEM_SELECTED;
                }
                accessibleItems.push_back(std::move(item));
            }
            const auto client = bounds();
            for (auto &item : accessibleItems) {
                if (GetFocus() == window && (results.empty() || item.id == results[selected].identity)) {
                    item.state |= STATE_SYSTEM_FOCUSED;
                }
                if (!IsWindowVisible(window)) {
                    item.state |= STATE_SYSTEM_INVISIBLE;
                }
                RECT intersection;
                if (!IntersectRect(&intersection, &client, &item.bounds)) {
                    item.state |= STATE_SYSTEM_OFFSCREEN;
                }
            }
            return accessibleItems;
        }
        bool operate(long id, bool activateItem) {
            if (!IsWindowVisible(window)) {
                return false;
            }
            if (id == browseId && results.empty()) {
                select(0);
                if (activateItem) {
                    activate();
                }
                return true;
            }
            const auto found = std::find_if(results.begin(), results.end(),
                                            [=](const auto &result) { return result.identity == id; });
            if (found == results.end()) {
                return false;
            }
            select(int(found - results.begin()));
            if (activateItem) {
                activate();
            }
            return true;
        }
        void paint(HDC dc) const {
            const auto client = bounds();
            PanelDrawing::fill(dc, client, theme.background);
            PanelDrawing::text(dc, L"Search settings",
                               offset({px(20), px(12), client.right - px(20), px(40)}), theme.foreground,
                               heading);
            PanelDrawing::text(dc, countText(), offset({px(20), px(44), client.right - px(20), px(66)}),
                               theme.secondary, font);
            if (results.empty()) {
                wrap(dc, L"No matching settings.\nTry FPS, reflection, emission or color.",
                     offset({px(20), px(92), client.right - px(20), px(164)}), theme.foreground);
                const auto button = offset(browseBounds());
                PanelDrawing::rounded(dc, button, theme.field,
                                      GetFocus() == window ? theme.accent : theme.track, px(2));
                PanelDrawing::text(dc, L"Browse settings", button, theme.foreground, font, DT_CENTER);
                return;
            }
            for (size_t rowIndex = 0; rowIndex < results.size(); ++rowIndex) {
                const auto &row = rows[rowIndex];
                const auto rowBounds = offset(row.bounds);
                if (rowBounds.bottom <= 0 || rowBounds.top >= client.bottom) {
                    continue;
                }
                if (int(rowIndex) == selected) {
                    PanelDrawing::fill(dc, rowBounds, theme.selected);
                    if (GetFocus() == window) {
                        PanelDrawing::fill(
                            dc, {rowBounds.left, rowBounds.top, rowBounds.left + px(2), rowBounds.bottom},
                            theme.accent);
                    }
                }
                const bool contrastSelected = int(rowIndex) == selected && highContrastSettingsMode();
                const auto foreground =
                    int(rowIndex) == selected ? theme.selectedForeground : theme.foreground;
                const auto secondary = contrastSelected ? theme.selectedForeground : theme.secondary;
                const auto &result = results[rowIndex];
                wrap(dc, result.label, offset(row.label), foreground);
                wrap(dc, result.path, offset(row.path), secondary);
                PanelDrawing::text(dc, result.value, offset(row.value), foreground, font);
                PanelDrawing::text(dc, result.state, offset(row.state), secondary, font, DT_RIGHT);
                PanelDrawing::fill(
                    dc, {px(20), rowBounds.bottom, client.right - px(20), rowBounds.bottom + 1}, theme.track);
            }
        }
        static LRESULT CALLBACK procedure(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) {
            auto *self = reinterpret_cast<SettingsSearchPanel *>(GetWindowLongPtrW(handle, GWLP_USERDATA));
            if (message == WM_NCCREATE) {
                self = static_cast<SettingsSearchPanel *>(
                    reinterpret_cast<CREATESTRUCTW *>(lParam)->lpCreateParams);
                self->window = handle;
                SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            if (!self) {
                return DefWindowProcW(handle, message, wParam, lParam);
            }
            switch (message) {
            case WM_ERASEBKGND:
                return 1;
            case WM_SIZE:
                self->arrange();
                return 0;
            case WM_SETFOCUS:
            case WM_KILLFOCUS:
                self->invalidate();
                return 0;
            case WM_GETOBJECT:
                if (self->accessibility && static_cast<LONG>(lParam) == OBJID_CLIENT) {
                    return self->accessibility->object(wParam);
                }
                break;
            case WM_GETDLGCODE:
                return DLGC_WANTARROWS | DLGC_WANTTAB | DLGC_WANTCHARS;
            case WM_KEYDOWN:
                if (wParam == VK_ESCAPE) {
                    self->leave(true);
                    return 0;
                }
                if (wParam == VK_TAB) {
                    self->leave(false);
                    return 0;
                }
                if (wParam == VK_RETURN || wParam == VK_SPACE) {
                    self->activate();
                    return 0;
                }
                if (wParam == VK_UP || wParam == VK_DOWN) {
                    self->select(self->selected + (wParam == VK_UP ? -1 : 1));
                    return 0;
                }
                if (wParam == VK_HOME || wParam == VK_END) {
                    self->select(wParam == VK_HOME ? 0 : int(self->results.size()) - 1);
                    return 0;
                }
                if (wParam == VK_PRIOR || wParam == VK_NEXT) {
                    self->select(self->selected +
                                 (wParam == VK_PRIOR ? -1 : 1) *
                                     std::max(1, int(self->bounds().bottom) / self->px(112)));
                    return 0;
                }
                break;
            case WM_CHAR:
                if (wParam == VK_RETURN || wParam == VK_SPACE || wParam == VK_TAB || wParam == VK_ESCAPE) {
                    return 0;
                }
                break;
            case WM_LBUTTONDOWN: {
                const POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) + self->scroll};
                if (self->results.empty()) {
                    const auto browse = self->browseBounds();
                    if (PtInRect(&browse, point)) {
                        self->select(0);
                        self->activate();
                    }
                    return 0;
                }
                for (size_t rowIndex = 0; rowIndex < self->rows.size(); ++rowIndex) {
                    if (PtInRect(&self->rows[rowIndex].bounds, point)) {
                        self->select(int(rowIndex));
                        self->activate();
                        break;
                    }
                }
                return 0;
            }
            case WM_MOUSEWHEEL: {
                self->wheelRemainder += GET_WHEEL_DELTA_WPARAM(wParam);
                const int wheelSteps = self->wheelRemainder / WHEEL_DELTA;
                self->wheelRemainder %= WHEEL_DELTA;
                self->setScroll(self->scroll - wheelSteps * self->px(80));
                return 0;
            }
            case WM_VSCROLL: {
                int position = self->scroll;
                const int scrollCode = LOWORD(wParam);
                if (scrollCode == SB_LINEUP) {
                    position -= self->px(32);
                } else if (scrollCode == SB_LINEDOWN) {
                    position += self->px(32);
                } else if (scrollCode == SB_PAGEUP) {
                    position -= self->bounds().bottom;
                } else if (scrollCode == SB_PAGEDOWN) {
                    position += self->bounds().bottom;
                } else if (scrollCode == SB_TOP) {
                    position = 0;
                } else if (scrollCode == SB_BOTTOM) {
                    position = self->contentHeight;
                } else if (scrollCode == SB_THUMBTRACK || scrollCode == SB_THUMBPOSITION) {
                    SCROLLINFO info{sizeof(info), SIF_TRACKPOS};
                    GetScrollInfo(handle, SB_VERT, &info);
                    position = info.nTrackPos;
                }
                self->setScroll(position);
                return 0;
            }
            case WM_PRINTCLIENT:
                self->paint(reinterpret_cast<HDC>(wParam));
                return 0;
            case WM_PAINT: {
                PAINTSTRUCT paintState;
                const auto targetDc = BeginPaint(handle, &paintState);
                try {
                    const auto clientBounds = self->bounds();
                    if (const auto dc = self->buffer.begin(targetDc, clientBounds.right,
                                                           clientBounds.bottom)) {
                        self->paint(dc);
                        self->buffer.present(targetDc);
                    }
                } catch (...) {
                    EndPaint(handle, &paintState);
                    throw;
                }
                EndPaint(handle, &paintState);
                return 0;
            }
            }
            return DefWindowProcW(handle, message, wParam, lParam);
        }

      public:
        SettingsSearchPanel(HWND parent, HFONT font, HFONT heading, const WorkspaceTheme &theme, float scale,
                            std::function<void(SettingsSearchTarget)> open, std::function<void(bool)> leave)
            : font(font), heading(heading), theme(theme), scale(scale), open(std::move(open)),
              leave(std::move(leave)) {
            WNDCLASSW windowClass{};
            windowClass.hInstance = GetModuleHandleW(nullptr);
            windowClass.lpfnWndProc = procedure;
            windowClass.lpszClassName = L"RFF.Workspace.SearchResults";
            windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            RegisterClassW(&windowClass);
            window = CreateWindowExW(0, windowClass.lpszClassName, L"Search settings",
                                     WS_CHILD | WS_TABSTOP | WS_VSCROLL, 0, 0, 1, 1, parent, nullptr,
                                     windowClass.hInstance, this);
            applyDarkThemeClass(window, false);
            accessibility = std::make_unique<AccessibleItems>(
                window, [this] { return L"Search settings. " + countText(); }, [this] { return items(); },
                [this](long id, bool action) { return operate(id, action); });
        }
        ~SettingsSearchPanel() {
            accessibility.reset();
            if (IsWindow(window)) {
                DestroyWindow(window);
            }
        }
        HWND handle() const {
            return window;
        }
        void update(std::vector<SettingsSearchResult> value, bool reset = true) {
            const long selectedIdentity = results.empty() ? 0 : results[selected].identity;
            results = std::move(value);
            selected = 0;
            if (reset) {
                scroll = 0;
            } else {
                for (size_t resultIndex = 0; resultIndex < results.size(); ++resultIndex) {
                    if (results[resultIndex].identity == selectedIdentity) {
                        selected = int(resultIndex);
                        break;
                    }
                }
            }
            arrange();
        }
        void show(bool visible) {
            ShowWindow(window, visible ? SW_SHOWNA : SW_HIDE);
        }
        void layout(int width, int height) {
            SetWindowPos(window, HWND_TOP, 0, 0, width, height, SWP_NOACTIVATE);
        }
        void focus() {
            select(selected);
        }
        void applyTheme() {
            applyDarkThemeClass(window, false);
            invalidate();
        }
        void applyMetrics(HFONT replacement, HFONT title, float newScale) {
            font = replacement;
            heading = title;
            scale = newScale;
            arrange();
            reveal();
        }
    };
} // namespace merutilm::rff2::workspace
