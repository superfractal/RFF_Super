//
// Created by Opus 5.5 on 2026-11-01
//

#pragma once
#include "AccessibleItems.hpp"
#include "PanelBackBuffer.hpp"
#include "PanelDrawing.hpp"
#include "WorkspaceTheme.hpp"
#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <windowsx.h>

namespace merutilm::rff2::workspace {
    // The timeline inspector's Selected / Settings / Add tabs; Settings and Add list their sections.
    class TimelineInspectorNav {
      public:
        struct Entry {
            int group;
            std::wstring label;
        };
        enum Tab { SELECTED, SETTINGS, ADD, TAB_COUNT };
        struct Callbacks {
            std::function<bool()> canLeaveForm;
            std::function<void(int)> open;
            std::function<void()> relayout;
            std::function<void(int)> leave;
        };

      private:
        static constexpr std::array<const wchar_t *, TAB_COUNT> tabLabels = {L"Selected", L"Settings", L"Add"};
        static constexpr std::array<const wchar_t *, TAB_COUNT> tabHelp = {
            L"Show the values of the selected timeline key.", L"Show settings that apply to the whole timeline.",
            L"Choose parameters to animate."};
        static constexpr long tabId = 10, backId = 20, rowId = 100;
        // An open section's title row takes the form's own 52-pixel header, with the back chevron before the name.
        // The list keeps the main settings pages' gaps: 24 pixels under the tabs, then plain rows 36 high and 40 apart.
        static constexpr int tabTop = 4, tabHeight = 32, backTop = 40, backHeight = 52, listTop = tabTop + tabHeight + 24,
                             rowStep = 40, rowHeight = 36;

        HWND window = nullptr;
        const WorkspaceTheme &theme;
        HFONT font = nullptr;
        float scale = 1.f;
        Callbacks callbacks;
        std::array<std::vector<Entry>, TAB_COUNT> entries;
        int tab = SELECTED;
        std::array<int, TAB_COUNT> opened{0, -1, -1};
        bool listing = false;
        long focusId = tabId, hoverId = 0;
        int scroll = 0;
        PanelBackBuffer buffer;
        std::unique_ptr<AccessibleItems> accessible;

        int px(int value) const {
            return int(value * scale + .5f);
        }
        RECT box(int x, int y, int width, int height) const {
            return {px(x), px(y), px(x + width), px(y + height)};
        }
        int width() const {
            RECT r{};
            GetClientRect(window, &r);
            return std::max(1, int(r.right / scale));
        }
        std::vector<const Entry *> rows() const {
            std::vector<const Entry *> result;
            if (listing && tab != SELECTED) {
                for (const auto &entry : entries[tab]) {
                    result.push_back(&entry);
                }
            }
            return result;
        }
        RECT tabBounds(int index) const {
            const int w = width() - 16, left = 8 + w * index / TAB_COUNT, right = 8 + w * (index + 1) / TAB_COUNT;
            return box(left, tabTop, right - left, tabHeight);
        }
        RECT backBounds() const {
            return box(8, backTop + 6, width() - 16, 40);
        }
        const Entry *openedEntry() const {
            const auto found =
                std::ranges::find_if(entries[tab], [this](const Entry &e) { return e.group == opened[tab]; });
            return found == entries[tab].end() ? nullptr : &*found;
        }
        RECT rowBounds(int index) const {
            auto r = box(8, listTop + index * rowStep, width() - 16, rowHeight);
            OffsetRect(&r, 0, -scroll);
            return r;
        }

        std::vector<AccessibleItem> items() const {
            std::vector<AccessibleItem> result;
            for (int i = 0; i < TAB_COUNT; ++i) {
                result.push_back({tabId + i, tabLabels[i], tabHelp[i], L"Select", tabBounds(i), ROLE_SYSTEM_PAGETAB,
                                  STATE_SYSTEM_FOCUSABLE | STATE_SYSTEM_SELECTABLE |
                                      (tab == i ? STATE_SYSTEM_SELECTED : 0)});
            }
            if (showsTitle()) {
                result.push_back({backId, L"Back to List", L"Show this tab's list again.", L"Open", backBounds(),
                                  ROLE_SYSTEM_PUSHBUTTON, STATE_SYSTEM_FOCUSABLE});
            }
            const auto shown = rows();
            for (int i = 0; i < int(shown.size()); ++i) {
                result.push_back({rowId + i, shown[i]->label, L"Open these settings.", L"Open", rowBounds(i),
                                  ROLE_SYSTEM_LISTITEM,
                                  STATE_SYSTEM_FOCUSABLE | STATE_SYSTEM_SELECTABLE |
                                      (shown[i]->group == opened[tab] ? STATE_SYSTEM_SELECTED : 0)});
            }
            return result;
        }
        long hit(int x, int y) const {
            for (const auto &item : items()) {
                if (PtInRect(&item.bounds, POINT{x, y})) {
                    return item.id;
                }
            }
            return 0;
        }
        bool leaveForm() {
            return !callbacks.canLeaveForm || callbacks.canLeaveForm();
        }
        void changed(bool layout) {
            if (layout && callbacks.relayout) {
                callbacks.relayout();
            }
            InvalidateRect(window, nullptr, FALSE);
            if (accessible) {
                accessible->changed();
            }
        }
        void reveal(long id) {
            if (id < rowId) {
                return;
            }
            const int index = int(id - rowId);
            RECT client{};
            GetClientRect(window, &client);
            const int top = px(listTop + index * rowStep), bottom = top + px(rowHeight);
            if (top - px(listTop) < scroll) {
                scroll = std::max(0, top - px(listTop));
            } else if (bottom > scroll + client.bottom) {
                scroll = bottom - client.bottom;
            }
        }
        void setScroll(int value) {
            RECT client{};
            GetClientRect(window, &client);
            const int content = px(listTop + int(rows().size()) * rowStep + 8);
            scroll = std::clamp(value, 0, std::max(0, content - int(client.bottom)));
            changed(false);
        }

      public:
        TimelineInspectorNav(HWND parent, HFONT font, const WorkspaceTheme &theme, float scale,
                             Callbacks callbacks)
            : theme(theme), font(font), scale(scale), callbacks(std::move(callbacks)) {
            static const bool registered = [] {
                WNDCLASSW cls{};
                cls.lpfnWndProc = procedure;
                cls.hInstance = GetModuleHandleW(nullptr);
                cls.hCursor = LoadCursor(nullptr, IDC_ARROW);
                cls.lpszClassName = L"RFF.Timeline.InspectorNav";
                return RegisterClassW(&cls) != 0;
            }();
            (void)registered;
            window = CreateWindowExW(0, L"RFF.Timeline.InspectorNav", L"", WS_CHILD | WS_TABSTOP, 0, 0, 1, 1, parent,
                                     nullptr, GetModuleHandleW(nullptr), this);
            accessible = std::make_unique<AccessibleItems>(
                window, [] { return std::wstring(L"Timeline settings navigation"); }, [this] { return items(); },
                [this](long id, bool) { return activate(id); });
        }
        ~TimelineInspectorNav() {
            accessible.reset();
            if (IsWindow(window)) {
                SetWindowLongPtrW(window, GWLP_USERDATA, 0);
                DestroyWindow(window);
            }
        }
        TimelineInspectorNav(const TimelineInspectorNav &) = delete;
        TimelineInspectorNav &operator=(const TimelineInspectorNav &) = delete;

        HWND handle() const {
            return window;
        }
        bool listingSections() const {
            return listing;
        }
        // True while the strip draws the open section's title, which the form then leaves out.
        bool showsTitle() const {
            return !listing && tab != SELECTED;
        }
        // The strip's height above the form; a list takes the whole pane.
        int formOffset() const {
            return px(showsTitle() ? backTop + backHeight : backTop);
        }
        void setEntries(std::vector<Entry> settings, std::vector<Entry> add) {
            entries[SETTINGS] = std::move(settings);
            entries[ADD] = std::move(add);
            // A PNG source offers fewer parameters, so a section opened before may no longer be there.
            for (const int t : {SETTINGS, ADD}) {
                if (!std::ranges::any_of(entries[t], [this, t](const Entry &e) { return e.group == opened[t]; })) {
                    opened[t] = -1;
                    listing = listing || tab == t;
                }
            }
            changed(false);
        }
        void setScale(float value, HFONT next) {
            if (scale == value && font == next) {
                return;
            }
            scale = value;
            font = next;
            changed(false);
        }
        // Follows a group the window opened itself, such as Audio after Add Audio File.
        void sync(int group) {
            int next = SELECTED;
            for (const int candidate : {SETTINGS, ADD}) {
                if (std::ranges::any_of(entries[candidate], [group](const Entry &e) { return e.group == group; })) {
                    next = candidate;
                }
            }
            tab = next;
            opened[tab] = group;
            listing = false;
            if (focusId >= rowId) {
                focusId = tabId + tab;
            }
            changed(false);
        }
        void showList(int which) {
            if (!leaveForm()) {
                return;
            }
            tab = std::clamp(which, int(SETTINGS), int(ADD));
            listing = true;
            scroll = 0;
            focusId = tabId + tab;
            changed(true);
        }
        void focus() {
            SetFocus(window);
        }
        void repaint() {
            changed(false);
        }
        bool activate(long id) {
            if (id >= tabId && id < tabId + TAB_COUNT) {
                const int next = int(id - tabId);
                if (next == tab && (next == SELECTED || listing)) {
                    focusId = id;
                    changed(false);
                    return true;
                }
                if (!leaveForm()) {
                    return false;
                }
                tab = next;
                focusId = id;
                scroll = 0;
                listing = tab != SELECTED && opened[tab] < 0;
                if (!listing && callbacks.open) {
                    callbacks.open(opened[tab]);
                }
                changed(true);
                return true;
            }
            if (id == backId) {
                if (!leaveForm()) {
                    return false;
                }
                listing = true;
                focusId = tabId + tab;
                changed(true);
                return true;
            }
            const auto shown = rows();
            if (id >= rowId && id < rowId + long(shown.size())) {
                if (!leaveForm()) {
                    return false;
                }
                opened[tab] = shown[size_t(id - rowId)]->group;
                listing = false;
                focusId = tabId + tab;
                if (callbacks.open) {
                    callbacks.open(opened[tab]);
                }
                changed(true);
                return true;
            }
            return false;
        }

      private:
        bool key(WPARAM key) {
            const auto list = items();
            if (key == VK_TAB) {
                if (callbacks.leave) {
                    callbacks.leave((GetKeyState(VK_SHIFT) & 0x8000) ? -1 : 1);
                }
                return true;
            }
            if (key == VK_RETURN || key == VK_SPACE) {
                activate(focusId);
                return true;
            }
            if (key == VK_BACK && !listing && tab != SELECTED) {
                activate(backId);
                return true;
            }
            if (focusId >= tabId && focusId < tabId + TAB_COUNT && (key == VK_LEFT || key == VK_RIGHT)) {
                const int next = std::clamp(int(focusId - tabId) + (key == VK_LEFT ? -1 : 1), 0, TAB_COUNT - 1);
                activate(tabId + next);
                return true;
            }
            if (key != VK_UP && key != VK_DOWN) {
                return false;
            }
            // Up and Down walk the tabs, the back row and the section rows in order.
            std::vector<long> order{tabId + tab};
            for (const auto &item : list) {
                if (item.id == backId || item.id >= rowId) {
                    order.push_back(item.id);
                }
            }
            auto found = std::ranges::find(order, focusId);
            int index = found == order.end() ? 0 : int(found - order.begin());
            index = std::clamp(index + (key == VK_UP ? -1 : 1), 0, int(order.size()) - 1);
            focusId = order[size_t(index)];
            reveal(focusId);
            changed(false);
            return true;
        }
        void paint(HDC target) {
            RECT client{};
            GetClientRect(window, &client);
            const HDC dc = buffer.begin(target, client.right, client.bottom);
            if (!dc) {
                return;
            }
            PanelDrawing::fill(dc, client, theme.background);
            for (int i = 0; i < TAB_COUNT; ++i) {
                const auto r = tabBounds(i);
                if (i == tab) {
                    PanelDrawing::fill(dc, r, theme.selected);
                } else if (hoverId == tabId + i) {
                    PanelDrawing::fill(dc, r, theme.field);
                }
                PanelDrawing::text(dc, tabLabels[i], r, i == tab ? theme.accent : theme.secondary, font, DT_CENTER);
            }
            PanelDrawing::fill(dc, box(8, tabTop + tabHeight + 2, width() - 16, 1), theme.track);
            // The title row matches the form's header: name 12 pixels down, rule at 51, the chevron in the left margin.
            if (showsTitle()) {
                const auto r = backBounds();
                if (hoverId == backId) {
                    PanelDrawing::fill(dc, r, theme.field);
                }
                PanelDrawing::text(dc, L"‹", {r.left, r.top, px(30), r.bottom}, theme.accent, font, DT_CENTER);
                if (const auto *entry = openedEntry()) {
                    PanelDrawing::text(dc, entry->label, {px(32), r.top, r.right - px(12), r.bottom},
                                       theme.foreground, font);
                }
                PanelDrawing::fill(dc, box(20, backTop + 51, width() - 40, 1), theme.track);
            }
            const int save = SaveDC(dc);
            IntersectClipRect(dc, 0, px(listTop - 2), client.right, client.bottom);
            const auto shown = rows();
            for (int i = 0; i < int(shown.size()); ++i) {
                const auto r = rowBounds(i);
                const bool chosen = shown[i]->group == opened[tab];
                if (chosen) {
                    PanelDrawing::fill(dc, r, theme.selected);
                } else if (hoverId == rowId + i) {
                    PanelDrawing::fill(dc, r, theme.field);
                }
                PanelDrawing::text(dc, shown[i]->label, {r.left + px(12), r.top, r.right - px(4), r.bottom},
                                   chosen ? theme.selectedForeground : theme.secondary, font);
            }
            RestoreDC(dc, save);
            if (GetFocus() == window) {
                for (const auto &item : items()) {
                    if (item.id == focusId) {
                        auto r = item.bounds;
                        InflateRect(&r, -1, -1);
                        PanelDrawing::border(dc, r, theme.accent, std::max(2, px(1)));
                    }
                }
            }
            buffer.present(target);
        }
        static LRESULT CALLBACK procedure(HWND hwnd, UINT message, WPARAM w, LPARAM l) {
            if (message == WM_NCCREATE) {
                SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                                  reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW *>(l)->lpCreateParams));
            }
            auto *self = reinterpret_cast<TimelineInspectorNav *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (!self) {
                return DefWindowProcW(hwnd, message, w, l);
            }
            switch (message) {
            case WM_ERASEBKGND:
                return 1;
            case WM_PAINT: {
                PAINTSTRUCT ps;
                const HDC dc = BeginPaint(hwnd, &ps);
                self->paint(dc);
                EndPaint(hwnd, &ps);
                return 0;
            }
            case WM_GETDLGCODE:
                return DLGC_WANTARROWS | DLGC_WANTTAB | DLGC_WANTCHARS;
            case WM_SETFOCUS:
            case WM_KILLFOCUS:
                self->changed(false);
                return 0;
            case WM_KEYDOWN:
                if (self->key(w)) {
                    return 0;
                }
                break;
            case WM_CHAR:
                if (w == VK_TAB || w == VK_RETURN || w == VK_SPACE || w == VK_BACK) {
                    return 0;
                }
                break;
            case WM_LBUTTONDOWN: {
                SetFocus(hwnd);
                if (const long id = self->hit(GET_X_LPARAM(l), GET_Y_LPARAM(l))) {
                    self->focusId = id;
                    self->activate(id);
                }
                return 0;
            }
            case WM_MOUSEMOVE: {
                const long id = self->hit(GET_X_LPARAM(l), GET_Y_LPARAM(l));
                if (id != self->hoverId) {
                    self->hoverId = id;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                TRACKMOUSEEVENT tracking{sizeof(tracking), TME_LEAVE, hwnd, 0};
                TrackMouseEvent(&tracking);
                SetCursor(LoadCursor(nullptr, id ? IDC_HAND : IDC_ARROW));
                return 0;
            }
            case WM_MOUSELEAVE:
                self->hoverId = 0;
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            case WM_MOUSEWHEEL:
                if (self->listing) {
                    self->setScroll(self->scroll - GET_WHEEL_DELTA_WPARAM(w) * self->px(self->rowStep) / WHEEL_DELTA);
                }
                return 0;
            case WM_SIZE:
                self->setScroll(self->scroll);
                return 0;
            case WM_GETOBJECT:
                if (static_cast<DWORD>(l) == static_cast<DWORD>(OBJID_CLIENT) && self->accessible) {
                    return self->accessible->object(w);
                }
                break;
            default:
                break;
            }
            return DefWindowProcW(hwnd, message, w, l);
        }
    };
}
