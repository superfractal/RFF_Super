//
// Modified by GPT-6 on 2026-09-17, 2026-09-18, 2026-09-20, 2026-09-22, 2026-09-23
//

#include "CustomMenu.hpp"
#include "MenuAccessibility.hpp"
#include "SettingsTheme.hpp"
#include "UiDpi.hpp"
#include <commctrl.h>
#include <windowsx.h>
#include <stdexcept>
#include <utility>

namespace merutilm::rff2 {
    namespace {
        constexpr UINT DispatchMessage = WM_APP + 0x362;
        constexpr UINT_PTR HoverTimer = 0x364;
        void fill(HDC dc, RECT rect, COLORREF color) {
            const auto brush = CreateSolidBrush(color);
            if (brush) {
                FillRect(dc, &rect, brush);
                DeleteObject(brush);
            }
        }
        struct Bitmap {
            HDC dc = nullptr;
            HBITMAP bitmap = nullptr;
            HGDIOBJ old = nullptr;
            Bitmap(int width, int height) {
                BITMAPINFO info{};
                info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                info.bmiHeader.biWidth = width;
                info.bmiHeader.biHeight = -height;
                info.bmiHeader.biPlanes = 1;
                info.bmiHeader.biBitCount = 32;
                info.bmiHeader.biCompression = BI_RGB;
                dc = CreateCompatibleDC(nullptr);
                void *pixels = nullptr;
                if (dc) {
                    bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
                }
                if (bitmap) {
                    old = SelectObject(dc, bitmap);
                }
            }
            ~Bitmap() {
                if (old) {
                    SelectObject(dc, old);
                }
                if (bitmap) {
                    DeleteObject(bitmap);
                }
                if (dc) {
                    DeleteDC(dc);
                }
            }
            explicit operator bool() const {
                return dc && bitmap && old && old != HGDI_ERROR;
            }
        };
    } // namespace

    struct CustomMenu::Impl {
        struct Popup {
            HWND window = nullptr;
            int parent = 0, selected = -1, scroll = 0, capacity = 0, row = 0, edge = 0, scrollBand = 0;
            RECT bounds{};
            std::vector<int> items;
            std::unique_ptr<Bitmap> surface;
            ~Popup() {
                if (IsWindow(window)) {
                    DestroyWindow(window);
                }
            }
        };
        HWND owner = nullptr, bar = nullptr, savedFocus = nullptr;
        HFONT font = nullptr;
        UINT dpi = 96, showDelay = 400;
        int barHeight = 24, popupRowHeight = 22, averageCharWidth = 7, width = 1, root = -1, pressed = -1,
            hoverNode = -1, hoverDepth = -1;
        ULONG generation = 1;
        bool active = false, keyboard = false, closing = false, altPending = false, suppressAltUp = false,
             consumeRelease = false;
        POINT lastPointer{};
        std::vector<std::pair<int, RECT>> barItems;
        std::vector<std::unique_ptr<Popup>> popups;
        MenuModel model;
        size_t canonicalCount = 1;
        std::function<MenuModel()> read;
        std::function<void(UINT)> dispatch;
        std::unique_ptr<MenuAccessibility> accessibility;

        static LRESULT CALLBACK procedure(HWND window, UINT message, WPARAM w, LPARAM l) {
            auto *self = reinterpret_cast<Impl *>(GetWindowLongPtrW(window, GWLP_USERDATA));
            if (message == WM_NCCREATE) {
                self = static_cast<Impl *>(reinterpret_cast<CREATESTRUCTW *>(l)->lpCreateParams);
                SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            if (!self) {
                return DefWindowProcW(window, message, w, l);
            }
            if (message == WM_ERASEBKGND) {
                return 1;
            }
            if (message == WM_MOUSEACTIVATE && window != self->bar) {
                return MA_NOACTIVATE;
            }
            if (message == WM_GETOBJECT && window == self->bar && self->accessibility) {
                const auto result = self->accessibility->object(w, l);
                if (result) {
                    return result;
                }
            }
            if (message == WM_PAINT) {
                PAINTSTRUCT ps;
                const auto dc = BeginPaint(window, &ps);
                if (window == self->bar) {
                    self->paintBar(dc);
                }
                EndPaint(window, &ps);
                return 0;
            }
            if (message == WM_PRINTCLIENT && window == self->bar) {
                self->paintBar(reinterpret_cast<HDC>(w));
                return 0;
            }
            if (message == WM_TIMER && w == HoverTimer) {
                self->hover();
                return 0;
            }
            if (message == DispatchMessage) {
                if (ULONG(l) == self->generation && w && w < 0x10000) {
                    self->dispatch(UINT(w));
                }
                return 0;
            }
            if (message == MenuAccessibility::ActionMessage) {
                self->accessibleAction(LOWORD(w), HIWORD(w), ULONG(l));
                return 0;
            }
            if (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST) {
                POINT point{GET_X_LPARAM(l), GET_Y_LPARAM(l)};
                if (message != WM_MOUSEWHEEL && message != WM_MOUSEHWHEEL) {
                    ClientToScreen(window, &point);
                }
                if (self->pointer(message, w, point)) {
                    return 0;
                }
            }
            return DefWindowProcW(window, message, w, l);
        }

        static LRESULT CALLBACK ownerProcedure(HWND window, UINT message, WPARAM w, LPARAM l, UINT_PTR,
                                               DWORD_PTR data) {
            auto *self = reinterpret_cast<Impl *>(data);
            if (message == WM_CANCELMODE || message == WM_DESTROY || message == WM_MOVE ||
                message == WM_SIZE || message == WM_DPICHANGED || message == WM_THEMECHANGED ||
                message == WM_SETTINGCHANGE || (message == WM_ACTIVATE && LOWORD(w) == WA_INACTIVE) ||
                (message == WM_CAPTURECHANGED && reinterpret_cast<HWND>(l) != window)) {
                self->cancel(false);
            }
            if (message == WM_NCDESTROY) {
                RemoveWindowSubclass(window, ownerProcedure, 0x363);
            }
            return DefSubclassProc(window, message, w, l);
        }

        Impl(HWND owner, std::function<MenuModel()> read, std::function<void(UINT)> dispatch)
            : owner(owner), read(std::move(read)), dispatch(std::move(dispatch)) {
            WNDCLASSEXW klass{sizeof(klass)};
            klass.lpfnWndProc = procedure;
            klass.hInstance = GetModuleHandleW(nullptr);
            klass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
            klass.lpszClassName = L"RFF.CustomMenu";
            if (!RegisterClassExW(&klass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
                throw std::runtime_error("Cannot register menu window");
            }
            bar = CreateWindowExW(0, klass.lpszClassName, L"Menu", WS_CHILD | WS_VISIBLE, 0, 0, 1, 24, owner,
                                  nullptr, klass.hInstance, this);
            if (!bar) {
                throw std::runtime_error("Cannot create menu bar");
            }
            SetWindowSubclass(owner, ownerProcedure, 0x363, reinterpret_cast<DWORD_PTR>(this));
            accessibility = std::make_unique<MenuAccessibility>(bar);
            refresh();
        }
        ~Impl() {
            cancel(false);
            RemoveWindowSubclass(owner, ownerProcedure, 0x363);
            accessibility.reset();
            if (IsWindow(bar)) {
                DestroyWindow(bar);
            }
            if (font) {
                DeleteObject(font);
            }
        }
        int px(int value) const {
            return std::max(1, UiDpi::pixels(value, dpi));
        }
        int leftInset() const {
            return 2 * UiDpi::metric(SM_CXMENUCHECK, dpi) + averageCharWidth / 2;
        }
        int rightInset() const {
            return 4 * UiDpi::metric(SM_CXMENUCHECK, dpi) - UiDpi::metric(SM_CXBORDER, dpi);
        }
        int barPadding() const {
            return 2 * averageCharWidth + 2 * UiDpi::metric(SM_CXBORDER, dpi);
        }
        void refresh() {
            cancel(false);
            dpi = UiDpi::forWindow(owner);
            NONCLIENTMETRICSW metrics{};
            metrics.cbSize = sizeof(metrics);
            using Query = BOOL(WINAPI *)(UINT, UINT, PVOID, UINT, UINT);
            const auto query = reinterpret_cast<Query>(
                GetProcAddress(GetModuleHandleW(L"user32.dll"), "SystemParametersInfoForDpi"));
            if (!query || !query(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0, dpi)) {
                SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
                metrics.lfMenuFont.lfHeight =
                    MulDiv(metrics.lfMenuFont.lfHeight, dpi, UiDpi::forWindow(nullptr));
            }
            const auto replacement = CreateFontIndirectW(&metrics.lfMenuFont);
            if (replacement) {
                if (font) {
                    DeleteObject(font);
                }
                font = replacement;
            }
            if (!font) {
                font = CreateFontW(-px(12), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
                                   L"Segoe UI");
            }
            const auto dc = GetDC(bar);
            const auto old = SelectObject(dc, font);
            TEXTMETRICW tm{};
            GetTextMetricsW(dc, &tm);
            averageCharWidth = std::max(1, int(tm.tmAveCharWidth));
            barHeight = std::max(UiDpi::metric(SM_CYMENU, dpi), int(tm.tmHeight));
            popupRowHeight = std::max(UiDpi::metric(SM_CYMENU, dpi), int(tm.tmHeight + tm.tmHeight / 3)) +
                             2 * UiDpi::metric(SM_CYBORDER, dpi);
            SelectObject(dc, old);
            ReleaseDC(bar, dc);
            SystemParametersInfoW(SPI_GETMENUSHOWDELAY, 0, &showDelay, 0);
            model = read();
            canonicalCount = model.nodes.size();
            ++generation;
            layout(width);
        }
        int textWidth(HDC dc, const std::wstring &text) const {
            SIZE size{};
            const auto label = MenuModel::label(text);
            GetTextExtentPoint32W(dc, label.c_str(), int(label.size()), &size);
            return size.cx;
        }
        void layout(int targetWidth) {
            if (active) {
                cancel(false);
            }
            width = std::max(1, targetWidth);
            barItems.clear();
            model.nodes.resize(canonicalCount);
            const auto dc = GetDC(bar);
            const auto old = SelectObject(dc, font);
            int help = 0, helpWidth = 0;
            for (int id : model.at(0).children) {
                if (model.at(id).rightAligned) {
                    help = id;
                    helpWidth = textWidth(dc, model.at(id).caption) + barPadding();
                }
            }
            std::vector<int> hidden;
            int x = 0;
            const int moreWidth = px(36);
            const auto roots = model.at(0).children;
            for (size_t i = 0; i < roots.size(); ++i) {
                const int id = roots[i];
                if (id == help) {
                    continue;
                }
                const int itemWidth = textWidth(dc, model.at(id).caption) + barPadding();
                int remaining = 0;
                for (size_t j = i + 1; j < roots.size(); ++j) {
                    if (roots[j] != help) {
                        remaining += textWidth(dc, model.at(roots[j]).caption) + barPadding();
                    }
                }
                const int limit =
                    width - helpWidth - (x + itemWidth + remaining > width - helpWidth ? moreWidth : 0);
                if (hidden.empty() && x + itemWidth <= limit) {
                    barItems.push_back({id, {x, 0, x + itemWidth, barHeight}});
                    x += itemWidth;
                } else {
                    hidden.push_back(id);
                }
            }
            if (!hidden.empty()) {
                const int id = int(model.nodes.size());
                MenuNode overflow;
                overflow.id = id;
                overflow.caption = L"More";
                overflow.children = std::move(hidden);
                model.nodes.push_back(std::move(overflow));
                const int end = std::max(x + 1, std::min(width - helpWidth, x + moreWidth));
                barItems.push_back({id, {x, 0, end, barHeight}});
            }
            if (help) {
                barItems.push_back({help, {std::max(0, width - helpWidth), 0, width, barHeight}});
            }
            SelectObject(dc, old);
            ReleaseDC(bar, dc);
            SetWindowPos(bar, HWND_TOP, 0, 0, width, barHeight, SWP_NOACTIVATE);
            InvalidateRect(bar, nullptr, FALSE);
            publishAccessibility();
        }
        void drawItem(HDC dc, int id, RECT rect, bool selected, bool top) const {
            const auto &n = model.at(id);
            // Theme colors and their palette provenance are maintained in SettingsTheme.hpp and NOTICE.
            const auto &theme = settingsTheme();
            const bool high = highContrastSettingsMode();
            const auto back = selected ? (high ? GetSysColor(COLOR_HIGHLIGHT) : theme.radioSelectedBackground)
                                       : (high ? GetSysColor(COLOR_MENU) : theme.background);
            const auto fore = !n.enabled         ? (high ? GetSysColor(COLOR_GRAYTEXT) : theme.textDisabled)
                              : selected && high ? GetSysColor(COLOR_HIGHLIGHTTEXT)
                                                 : theme.text;
            fill(dc, rect, back);
            SetTextColor(dc, fore);
            SetBkMode(dc, TRANSPARENT);
            if (n.separator) {
                RECT line{rect.left + px(24), (rect.top + rect.bottom) / 2, rect.right - px(8),
                          (rect.top + rect.bottom) / 2 + 1};
                fill(dc, line, theme.sectionFrame);
                return;
            }
            RECT text = rect;
            text.left += top ? averageCharWidth : leftInset();
            text.right -= top ? averageCharWidth : rightInset();
            UINT format = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | (top ? DT_CENTER : DT_LEFT);
            BOOL cues = FALSE;
            SystemParametersInfoW(SPI_GETKEYBOARDCUES, 0, &cues, 0);
            if (!keyboard && !cues) {
                format |= DT_HIDEPREFIX;
            }
            if (!n.shortcut.empty() && !top) {
                RECT shortcut = text;
                DrawTextW(dc, n.shortcut.c_str(), -1, &shortcut,
                          DT_RIGHT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
                text.right -= textWidth(dc, n.shortcut) + 2 * averageCharWidth;
            }
            const auto caption = top && n.caption == L"More" ? L"\x00bb" : n.caption.c_str();
            DrawTextW(dc, caption, -1, &text, format);
            const int cy = (rect.top + rect.bottom) / 2;
            if (!top && n.checked) {
                const int box = std::max(6, int(rect.bottom - rect.top) / 2),
                          cx = rect.left + UiDpi::metric(SM_CXMENUCHECK, dpi);
                const auto pen = CreatePen(PS_SOLID, std::max(1, box / 6), fore);
                const auto oldPen = SelectObject(dc, pen);
                POINT tick[] = {{cx - box / 2, cy + box / 12},
                                {cx - box / 8, cy + box / 3},
                                {cx + box / 2, cy - box / 3}};
                Polyline(dc, tick, 3);
                SelectObject(dc, oldPen);
                DeleteObject(pen);
            }
            if (!top && !n.children.empty()) {
                const int column = UiDpi::metric(SM_CXMENUCHECK, dpi), arrow = std::max(2, column / 3),
                          right = rect.right - column / 2;
                const auto brush = CreateSolidBrush(fore);
                const auto oldBrush = SelectObject(dc, brush);
                const auto oldPen = SelectObject(dc, GetStockObject(NULL_PEN));
                POINT head[] = {{right - arrow, cy - arrow}, {right, cy}, {right - arrow, cy + arrow}};
                Polygon(dc, head, 3);
                SelectObject(dc, oldPen);
                SelectObject(dc, oldBrush);
                DeleteObject(brush);
            }
            if (selected && keyboard) {
                InflateRect(&rect, -2, -2);
                DrawFocusRect(dc, &rect);
            }
        }
        void paintBar(HDC target) const {
            Bitmap bitmap(width, barHeight);
            if (!bitmap) {
                return;
            }
            const auto old = SelectObject(bitmap.dc, font);
            fill(bitmap.dc, {0, 0, width, barHeight}, settingsTheme().background);
            for (const auto &[id, rect] : barItems) {
                drawItem(bitmap.dc, id, rect, active && id == root, true);
            }
            fill(bitmap.dc, {0, barHeight - 1, width, barHeight}, settingsTheme().sectionFrame);
            BitBlt(target, 0, 0, width, barHeight, bitmap.dc, 0, 0, SRCCOPY);
            SelectObject(bitmap.dc, old);
        }
        RECT barRect(int id) const {
            for (const auto &[node, rect] : barItems) {
                if (node == id) {
                    RECT r = rect;
                    MapWindowPoints(bar, nullptr, reinterpret_cast<POINT *>(&r), 2);
                    return r;
                }
            }
            return {};
        }
        RECT rowRect(const Popup &p, int index) const {
            const int y = p.bounds.top + p.edge + p.scrollBand + (index - p.scroll) * p.row;
            return {p.bounds.left + p.edge, y, p.bounds.right - p.edge, y + p.row};
        }
        bool present(Popup &p) {
            const int w = p.bounds.right - p.bounds.left, h = p.bounds.bottom - p.bounds.top;
            auto bitmap = std::make_unique<Bitmap>(w, h);
            if (!*bitmap) {
                return false;
            }
            const auto old = SelectObject(bitmap->dc, font);
            fill(bitmap->dc, {0, 0, w, h}, settingsTheme().background);
            for (int i = p.scroll; i < std::min(int(p.items.size()), p.scroll + p.capacity); ++i) {
                RECT rect = rowRect(p, i);
                OffsetRect(&rect, -p.bounds.left, -p.bounds.top);
                drawItem(bitmap->dc, p.items[i], rect, i == p.selected, false);
            }
            if (p.scrollBand) {
                SetTextColor(bitmap->dc, settingsTheme().text);
                SetBkMode(bitmap->dc, TRANSPARENT);
                RECT upper{p.edge, p.edge, w - p.edge, p.edge + p.scrollBand},
                    lower{p.edge, h - p.edge - p.scrollBand, w - p.edge, h - p.edge};
                DrawTextW(bitmap->dc, p.scroll ? L"\x25b4" : L"", -1, &upper,
                          DT_CENTER | DT_SINGLELINE | DT_VCENTER);
                DrawTextW(bitmap->dc, p.scroll + p.capacity < int(p.items.size()) ? L"\x25be" : L"", -1,
                          &lower, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
            }
            const auto border = CreateSolidBrush(highContrastSettingsMode() ? GetSysColor(COLOR_MENUTEXT)
                                                                            : settingsTheme().sectionFrame);
            RECT frame{0, 0, w, h};
            if (border) {
                FrameRect(bitmap->dc, &frame, border);
                DeleteObject(border);
            }
            SelectObject(bitmap->dc, old);
            GdiFlush();
            POINT position{p.bounds.left, p.bounds.top}, source{};
            SIZE size{w, h};
            if (!UpdateLayeredWindow(p.window, nullptr, &position, &size, bitmap->dc, &source, 0, nullptr,
                                     ULW_OPAQUE)) {
                const auto error = GetLastError();
                const auto message = L"RFF custom menu publication failed: " + std::to_wstring(error) + L"\n";
                OutputDebugStringW(message.c_str());
                return false;
            }
            p.surface = std::move(bitmap);
            return true;
        }
        void begin() {
            if (active) {
                return;
            }
            model = read();
            canonicalCount = model.nodes.size();
            layout(width);
            ++generation;
            active = true;
            savedFocus = GetFocus();
            SetFocus(bar);
            GetCursorPos(&lastPointer);
            SetCapture(owner);
        }
        void trim(size_t count) {
            while (popups.size() > count) {
                popups.pop_back();
            }
            KillTimer(bar, HoverTimer);
            hoverNode = hoverDepth = -1;
        }
        bool open(int parent, RECT anchor, size_t depth, bool selectFirst = false) {
            const auto current = read();
            for (size_t i = 1; i < std::min(canonicalCount, current.nodes.size()); ++i) {
                model.nodes[i].enabled = current.nodes[i].enabled;
                model.nodes[i].checked = current.nodes[i].checked;
            }
            const auto &node = model.at(parent);
            if (node.children.empty()) {
                return false;
            }
            MONITORINFO monitor{sizeof(monitor)};
            if (!GetMonitorInfoW(MonitorFromRect(&anchor, MONITOR_DEFAULTTONEAREST), &monitor)) {
                return false;
            }
            Popup candidate;
            candidate.parent = parent;
            candidate.items = node.children;
            candidate.row = popupRowHeight;
            candidate.edge = UiDpi::metric(SM_CXEDGE, dpi) + UiDpi::metric(SM_CXBORDER, dpi);
            const auto dc = GetDC(bar);
            const auto old = SelectObject(dc, font);
            int w = 1;
            for (int id : node.children) {
                const auto &item = model.at(id);
                const int shortcut =
                    item.shortcut.empty() ? 0 : textWidth(dc, item.shortcut) + 2 * averageCharWidth;
                w = std::max(w, textWidth(dc, item.caption) + shortcut + leftInset() + rightInset() +
                                    2 * candidate.edge);
            }
            SelectObject(dc, old);
            ReleaseDC(bar, dc);
            const int maxHeight = monitor.rcWork.bottom - monitor.rcWork.top;
            candidate.capacity = std::max(1, (maxHeight - 2 * candidate.edge) / candidate.row);
            if (int(node.children.size()) > candidate.capacity) {
                candidate.scrollBand = px(18);
                candidate.capacity =
                    std::max(1, (maxHeight - 2 * candidate.edge - 2 * candidate.scrollBand) / candidate.row);
            }
            candidate.capacity = std::min(candidate.capacity, int(node.children.size()));
            const int h = 2 * candidate.edge + 2 * candidate.scrollBand + candidate.capacity * candidate.row;
            candidate.bounds = MenuLayout::place(anchor, w, h, monitor.rcWork, depth > 0);
            if (selectFirst) {
                for (int i = 0; i < int(candidate.items.size()); ++i) {
                    if (selectable(candidate.items[i])) {
                        candidate.selected = i;
                        break;
                    }
                }
            }
            candidate.scroll = std::max(0, candidate.selected - candidate.capacity + 1);
            if (depth < popups.size()) {
                candidate.window = popups[depth]->window;
            } else {
                candidate.window = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                                                   L"RFF.CustomMenu", L"", WS_POPUP, 0, 0, 1, 1, owner,
                                                   nullptr, GetModuleHandleW(nullptr), this);
            }
            if (!candidate.window) {
                cancel();
                return false;
            }
            const bool reused = depth < popups.size();
            if (!present(candidate)) {
                if (reused) {
                    candidate.window = nullptr;
                }
                cancel();
                return false;
            }
            if (reused) {
                popups[depth]->window = nullptr;
            }
            trim(depth);
            auto popup = std::make_unique<Popup>();
            popup->window = std::exchange(candidate.window, nullptr);
            popup->parent = candidate.parent;
            popup->items = std::move(candidate.items);
            popup->selected = candidate.selected;
            popup->row = candidate.row;
            popup->edge = candidate.edge;
            popup->capacity = candidate.capacity;
            popup->scrollBand = candidate.scrollBand;
            popup->bounds = candidate.bounds;
            popup->surface = std::move(candidate.surface);
            ShowWindow(popup->window, SW_SHOWNOACTIVATE);
            popups.push_back(std::move(popup));
            publishAccessibility();
            return true;
        }
        bool selectable(int id) const {
            return model.at(id).enabled && !model.at(id).separator;
        }
        wchar_t accessKey(int id) const {
            return model.at(id).access ? model.at(id).access : MenuModel::mnemonic(model.at(id).caption);
        }
        void openRoot(int id, bool first = false) {
            begin();
            root = id;
            trim(1);
            RECT anchor = barRect(id);
            anchor.bottom -= UiDpi::metric(SM_CYBORDER, dpi);
            if (!open(id, anchor, 0, first)) {
                cancel();
            }
            InvalidateRect(bar, nullptr, FALSE);
        }
        void cancel(bool restore = true) {
            if (closing) {
                return;
            }
            closing = true;
            const bool wasActive = active;
            active = false;
            root = pressed = -1;
            trim(0);
            ++generation;
            // Native move/resize also captures the owner; only an active menu owns capture here.
            if (wasActive && GetCapture() == owner) {
                ReleaseCapture();
            }
            if (wasActive && restore && GetForegroundWindow() == owner && IsWindow(savedFocus) &&
                IsWindowEnabled(savedFocus)) {
                SetFocus(savedFocus);
            }
            savedFocus = nullptr;
            InvalidateRect(bar, nullptr, FALSE);
            publishAccessibility();
            closing = false;
        }
        bool repaint(Popup &p) {
            if (present(p)) {
                publishAccessibility();
                return true;
            }
            cancel();
            return false;
        }
        bool select(size_t depth, int index) {
            if (depth >= popups.size()) {
                return false;
            }
            auto &p = *popups[depth];
            if (index < 0 || index >= int(p.items.size()) || !selectable(p.items[index])) {
                return false;
            }
            if (p.selected == index && index >= p.scroll && index < p.scroll + p.capacity) {
                return true;
            }
            if (p.selected != index) {
                trim(depth + 1);
                p.selected = index;
            }
            p.scroll = std::clamp(p.scroll, std::max(0, index - p.capacity + 1), index);
            return repaint(p);
        }
        void activate(size_t depth, int index) {
            if (depth >= popups.size()) {
                return;
            }
            auto &p = *popups[depth];
            if (index < 0 || index >= int(p.items.size())) {
                return;
            }
            const int id = p.items[index];
            if (!selectable(id)) {
                return;
            }
            if (!model.at(id).children.empty()) {
                if (select(depth, index)) {
                    open(id, rowRect(p, index), depth + 1, true);
                }
            } else {
                const UINT command = model.at(id).command;
                cancel();
                if (command) {
                    PostMessageW(bar, DispatchMessage, command, generation);
                }
            }
        }
        void hover() {
            KillTimer(bar, HoverTimer);
            const int depth = hoverDepth, id = hoverNode;
            hoverNode = hoverDepth = -1;
            if (!active || depth < 0 || depth >= int(popups.size())) {
                return;
            }
            auto &p = *popups[depth];
            const auto found = std::find(p.items.begin(), p.items.end(), id);
            if (found == p.items.end()) {
                return;
            }
            const int index = int(found - p.items.begin());
            if (!select(depth, index)) {
                return;
            }
            if (!model.at(id).children.empty()) {
                open(id, rowRect(p, index), depth + 1);
            }
        }
        void schedule(int depth, int id) {
            if (hoverDepth == depth && hoverNode == id) {
                return;
            }
            hoverDepth = depth;
            hoverNode = id;
            SetTimer(bar, HoverTimer, std::max(1u, showDelay), nullptr);
        }
        bool pointer(UINT message, WPARAM w, POINT point) {
            const bool down = message == WM_LBUTTONDOWN, up = message == WM_LBUTTONUP,
                       move = message == WM_MOUSEMOVE;
            if (!active && !down) {
                return false;
            }
            if (move && point.x == lastPointer.x && point.y == lastPointer.y) {
                return active;
            }
            if (move) {
                keyboard = false;
            }
            const POINT origin = barOrigin();
            const POINT barPoint{point.x - origin.x, point.y - origin.y};
            for (const auto &[id, bounds] : barItems) {
                if (PtInRect(&bounds, barPoint)) {
                    if (down) {
                        if (active && root == id) {
                            cancel();
                            consumeRelease = true;
                            return true;
                        }
                        openRoot(id);
                        pressed = id;
                    } else if (move && active && root != id) {
                        openRoot(id);
                    } else if (up) {
                        pressed = -1;
                    }
                    lastPointer = point;
                    return active || down;
                }
            }
            if (!active) {
                return false;
            }
            for (int depth = int(popups.size()) - 1; depth >= 0; --depth) {
                auto &p = *popups[depth];
                if (!PtInRect(&p.bounds, point)) {
                    continue;
                }
                if (message == WM_MOUSEWHEEL) {
                    trim(depth + 1);
                    p.scroll = std::clamp(p.scroll - GET_WHEEL_DELTA_WPARAM(w) / WHEEL_DELTA, 0,
                                          std::max(0, int(p.items.size()) - p.capacity));
                    repaint(p);
                    return true;
                }
                const int localY = point.y - p.bounds.top - p.edge;
                if (p.scrollBand && (localY < p.scrollBand || localY >= p.scrollBand + p.capacity * p.row)) {
                    if (down) {
                        trim(depth + 1);
                        p.scroll = std::clamp(p.scroll + (localY < p.scrollBand ? -1 : 1), 0,
                                              std::max(0, int(p.items.size()) - p.capacity));
                        repaint(p);
                    }
                    return true;
                }
                const int index = p.scroll + (localY - p.scrollBand) / p.row;
                if (index < 0 || index >= int(p.items.size()) || !selectable(p.items[index])) {
                    return true;
                }
                const int id = p.items[index];
                if (move) {
                    bool towardChild = false;
                    if (depth + 1 < int(popups.size()) && p.selected != index) {
                        const auto &child = *popups[depth + 1];
                        const bool right = child.bounds.left >= p.bounds.right;
                        towardChild = (right ? point.x > lastPointer.x : point.x < lastPointer.x) &&
                                      point.y >= child.bounds.top - px(24) &&
                                      point.y <= child.bounds.bottom + px(24);
                    }
                    if (towardChild) {
                        schedule(depth, id);
                    } else if (!select(depth, index)) {
                        return true;
                    } else {
                        if (!model.at(id).children.empty() && depth + 1 >= int(popups.size())) {
                            schedule(depth, id);
                        }
                    }
                }
                if (down) {
                    pressed = id;
                    if (!select(depth, index)) {
                        return true;
                    }
                    if (!model.at(id).children.empty()) {
                        open(id, rowRect(p, index), depth + 1);
                    }
                }
                if (up && pressed >= 0) {
                    pressed = -1;
                    activate(depth, index);
                }
                lastPointer = point;
                return true;
            }
            if (down || message == WM_RBUTTONDOWN || message == WM_MBUTTONDOWN) {
                cancel();
                consumeRelease = true;
            }
            lastPointer = point;
            return true;
        }
        POINT barOrigin() const {
            POINT point{};
            ClientToScreen(bar, &point);
            return point;
        }
        void rootStep(int step) {
            const auto found = std::find_if(barItems.begin(), barItems.end(),
                                            [&](const auto &entry) { return entry.first == root; });
            const int index = found == barItems.end() ? 0 : int(found - barItems.begin());
            const int next = (index + step + int(barItems.size())) % int(barItems.size());
            if (popups.empty()) {
                root = barItems[next].first;
                InvalidateRect(bar, nullptr, FALSE);
                publishAccessibility();
            } else {
                openRoot(barItems[next].first, true);
            }
        }
        bool key(UINT key) {
            keyboard = true;
            InvalidateRect(bar, nullptr, FALSE);
            if (key == VK_ESCAPE) {
                if (popups.size() > 1) {
                    trim(popups.size() - 1);
                    publishAccessibility();
                } else if (!popups.empty()) {
                    trim(0);
                    publishAccessibility();
                } else {
                    cancel();
                }
                return true;
            }
            if (key == VK_TAB) {
                cancel();
                return false;
            }
            if (popups.empty()) {
                if (key == VK_LEFT || key == VK_RIGHT) {
                    rootStep(key == VK_LEFT ? -1 : 1);
                    return true;
                }
                if (key == VK_DOWN || key == VK_UP || key == VK_RETURN || key == VK_SPACE) {
                    openRoot(root, true);
                    return true;
                }
            } else {
                const size_t depth = popups.size() - 1;
                auto &p = *popups.back();
                if (key == VK_LEFT) {
                    if (depth) {
                        trim(depth);
                        publishAccessibility();
                    } else {
                        rootStep(-1);
                    }
                    return true;
                }
                if (key == VK_RIGHT) {
                    if (p.selected >= 0 && !model.at(p.items[p.selected]).children.empty()) {
                        activate(depth, p.selected);
                    } else {
                        rootStep(1);
                    }
                    return true;
                }
                if (key == VK_RETURN || key == VK_SPACE) {
                    activate(depth, p.selected);
                    return true;
                }
                if (key == VK_DOWN || key == VK_UP || key == VK_HOME || key == VK_END) {
                    const int count = int(p.items.size()), step = key == VK_UP || key == VK_END ? -1 : 1;
                    int index = key == VK_HOME ? -1 : key == VK_END ? count : p.selected;
                    for (int i = 0; i < count; ++i) {
                        index = (index + step + count) % count;
                        if (selectable(p.items[index])) {
                            select(depth, index);
                            break;
                        }
                    }
                    return true;
                }
            }
            return true;
        }
        void mnemonic(wchar_t character, bool top = false) {
            character = std::towupper(character);
            std::vector<int> matches;
            if (top || popups.empty()) {
                for (const auto &[id, rect] : barItems) {
                    if (accessKey(id) == character) {
                        matches.push_back(id);
                    }
                }
                if (!matches.empty()) {
                    const auto current = std::find(matches.begin(), matches.end(), root);
                    const auto next = current == matches.end() || current + 1 == matches.end()
                                          ? matches.begin()
                                          : current + 1;
                    openRoot(*next, true);
                }
            } else {
                auto &p = *popups.back();
                for (int i = 0; i < int(p.items.size()); ++i) {
                    if (selectable(p.items[i]) && accessKey(p.items[i]) == character) {
                        matches.push_back(i);
                    }
                }
                if (matches.empty()) {
                    return;
                }
                const auto next =
                    std::find_if(matches.begin(), matches.end(), [&](int i) { return i > p.selected; });
                const int index = next == matches.end() ? matches.front() : *next;
                if (!select(popups.size() - 1, index)) {
                    return;
                }
                if (matches.size() == 1) {
                    activate(popups.size() - 1, index);
                }
            }
        }
        bool filter(const MSG &message) {
            if (!IsWindowEnabled(owner)) {
                cancel(false);
                return false;
            }
            const bool belongs = message.hwnd == owner || IsChild(owner, message.hwnd) || message.hwnd == bar;
            if (!belongs && std::none_of(popups.begin(), popups.end(),
                                         [&](const auto &p) { return p->window == message.hwnd; })) {
                return false;
            }
            if (message.message >= WM_MOUSEFIRST && message.message <= WM_MOUSELAST) {
                if (consumeRelease && (message.message == WM_LBUTTONUP || message.message == WM_RBUTTONUP ||
                                       message.message == WM_MBUTTONUP)) {
                    consumeRelease = false;
                    return true;
                }
                if (message.message == WM_LBUTTONDOWN || message.message == WM_RBUTTONDOWN ||
                    message.message == WM_MBUTTONDOWN) {
                    consumeRelease = false;
                }
                if (active) {
                    POINT point{GET_X_LPARAM(message.lParam), GET_Y_LPARAM(message.lParam)};
                    if (message.message != WM_MOUSEWHEEL && message.message != WM_MOUSEHWHEEL) {
                        ClientToScreen(message.hwnd, &point);
                    }
                    return pointer(message.message, message.wParam, point);
                }
                return false;
            }
            const bool control = GetKeyState(VK_CONTROL) & 0x8000, alt = GetKeyState(VK_MENU) & 0x8000;
            if (message.message == WM_SYSKEYDOWN && message.wParam == VK_MENU && !control) {
                altPending = true;
                return true;
            }
            if (message.message == WM_SYSKEYUP && message.wParam == VK_MENU) {
                if (suppressAltUp) {
                    suppressAltUp = false;
                    altPending = false;
                    return true;
                }
                if (altPending) {
                    altPending = false;
                    if (active) {
                        cancel();
                    } else {
                        begin();
                        keyboard = true;
                        root = barItems.front().first;
                        publishAccessibility();
                        InvalidateRect(bar, nullptr, FALSE);
                    }
                    return true;
                }
            }
            if (message.message == WM_SYSKEYDOWN && message.wParam != VK_MENU) {
                altPending = false;
                suppressAltUp = true;
            }
            if ((message.message == WM_KEYUP || message.message == WM_SYSKEYUP) && message.wParam == VK_F10 &&
                !(GetKeyState(VK_SHIFT) & 0x8000)) {
                return true;
            }
            if ((message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN) &&
                message.wParam == VK_F10 && !control && !alt && !(GetKeyState(VK_SHIFT) & 0x8000)) {
                if (active) {
                    cancel();
                } else {
                    begin();
                    keyboard = true;
                    root = barItems.front().first;
                    publishAccessibility();
                    InvalidateRect(bar, nullptr, FALSE);
                }
                return true;
            }
            if (message.message == WM_SYSKEYDOWN &&
                (message.wParam == VK_F4 || message.wParam == VK_SPACE || message.wParam == VK_TAB)) {
                cancel(false);
                return false;
            }
            if (message.message == WM_SYSCHAR && !control) {
                if (!active) {
                    begin();
                    keyboard = true;
                }
                mnemonic(wchar_t(message.wParam), true);
                return true;
            }
            if (active && control && (message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN)) {
                cancel();
                return false;
            }
            if (active && (message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN)) {
                switch (message.wParam) {
                case VK_ESCAPE:
                case VK_TAB:
                case VK_LEFT:
                case VK_RIGHT:
                case VK_UP:
                case VK_DOWN:
                case VK_HOME:
                case VK_END:
                case VK_RETURN:
                case VK_SPACE:
                    return key(UINT(message.wParam));
                default:
                    TranslateMessage(&message);
                    return true;
                }
            }
            if (active && message.message == WM_CHAR) {
                mnemonic(wchar_t(message.wParam));
                return true;
            }
            if (active && (message.message == WM_KEYUP || message.message == WM_SYSKEYUP)) {
                return true;
            }
            return false;
        }
        void accessibleAction(int id, int action, ULONG revision) {
            if (revision != generation || id <= 0 || id >= int(model.nodes.size()) || !selectable(id)) {
                return;
            }
            keyboard = true;
            for (const auto &[item, rect] : barItems) {
                if (item == id) {
                    if (action == MenuAccessibility::Collapse) {
                        cancel();
                    } else {
                        openRoot(id, true);
                    }
                    return;
                }
            }
            for (size_t depth = 0; depth < popups.size(); ++depth) {
                auto &popup = *popups[depth];
                const auto found = std::find(popup.items.begin(), popup.items.end(), id);
                if (found == popup.items.end()) {
                    continue;
                }
                const int index = int(found - popup.items.begin());
                if (action == MenuAccessibility::Collapse) {
                    trim(depth + 1);
                    publishAccessibility();
                } else if (action == MenuAccessibility::Focus) {
                    select(depth, index);
                } else {
                    activate(depth, index);
                }
                return;
            }
        }
        void publishAccessibility() {
            if (!accessibility) {
                return;
            }
            std::vector<MenuAccessibleNode> nodes;
            MenuAccessibleNode menuBarNode;
            menuBarNode.bar = true;
            menuBarNode.name = L"Menu";
            GetWindowRect(bar, &menuBarNode.bounds);
            for (const auto &[id, rect] : barItems) {
                menuBarNode.children.push_back(id);
            }
            nodes.push_back(menuBarNode);
            const auto appendItem = [&](int id, int parent, RECT bounds, bool focused) {
                const auto &menuNode = model.at(id);
                MenuAccessibleNode accessibleNode;
                accessibleNode.id = id;
                accessibleNode.parent = parent;
                accessibleNode.name = MenuModel::label(menuNode.caption);
                const auto key = accessKey(id);
                if (key) {
                    accessibleNode.accessKey = std::wstring(parent == 0 ? L"Alt+" : L"") + key;
                }
                accessibleNode.shortcut = menuNode.shortcut;
                accessibleNode.bounds = bounds;
                accessibleNode.enabled = menuNode.enabled;
                accessibleNode.focused = focused;
                accessibleNode.checkable = menuNode.checkable;
                accessibleNode.checked = menuNode.checked;
                accessibleNode.expandable = !menuNode.children.empty();
                const auto openChild = std::find_if(popups.begin(), popups.end(),
                                                [&](const auto &popup) { return popup->parent == id; });
                if (openChild != popups.end()) {
                    accessibleNode.expanded = true;
                    accessibleNode.children.push_back(0x10000 + id);
                }
                nodes.push_back(std::move(accessibleNode));
            };
            for (const auto &[id, rect] : barItems) {
                appendItem(id, 0, barRect(id), active && popups.empty() && root == id);
            }
            for (size_t depth = 0; depth < popups.size(); ++depth) {
                const auto &popup = *popups[depth];
                MenuAccessibleNode menu;
                menu.id = 0x10000 + popup.parent;
                menu.parent = popup.parent;
                menu.menu = true;
                menu.name = MenuModel::label(model.at(popup.parent).caption);
                menu.bounds = popup.bounds;
                for (int id : popup.items) {
                    if (!model.at(id).separator) {
                        menu.children.push_back(id);
                    }
                }
                nodes.push_back(std::move(menu));
                for (int i = 0; i < int(popup.items.size()); ++i) {
                    if (!model.at(popup.items[i]).separator) {
                        appendItem(popup.items[i], 0x10000 + popup.parent,
                             i >= popup.scroll && i < popup.scroll + popup.capacity ? rowRect(popup, i) : RECT{},
                             active && depth + 1 == popups.size() && popup.selected == i);
                    }
                }
            }
            accessibility->update(std::move(nodes), generation);
        }
    };
    CustomMenu::CustomMenu(HWND owner, std::function<MenuModel()> read, std::function<void(UINT)> dispatch)
        : impl(std::make_unique<Impl>(owner, std::move(read), std::move(dispatch))) {}
    CustomMenu::~CustomMenu() = default;
    bool CustomMenu::filter(const MSG &message) {
        return impl->filter(message);
    }
    void CustomMenu::refresh() {
        impl->refresh();
    }
    void CustomMenu::layout(int width) {
        impl->layout(width);
    }
    int CustomMenu::height() const {
        return impl->barHeight;
    }
    void CustomMenu::cancel() {
        impl->cancel();
    }
    bool CustomMenu::requested() {
        wchar_t value[8]{};
        const DWORD length = GetEnvironmentVariableW(L"RFF_CUSTOM_MENUS", value, 8);
        return length != 1 || value[0] != L'0';
    }
} // namespace merutilm::rff2
