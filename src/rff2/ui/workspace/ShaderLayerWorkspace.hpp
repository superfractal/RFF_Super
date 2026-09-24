//
// Modified by GPT-6 on 2026-09-17, 2026-09-18, 2026-09-21, 2026-09-23
// Modified by GPT-5 on 2026-09-17
//

#pragma once
#include "ShaderLayerModel.hpp"
#include "WorkspaceContent.hpp"
#include "WorkspaceButton.hpp"
#include "AccessibleControl.hpp"
#include "PanelBackBuffer.hpp"
#include "../../vulkan/OrderedShaderLayers.hpp"
#include <windowsx.h>

namespace merutilm::rff2::workspace {
    class ShaderLayerWorkspace final : public WorkspaceContent {
        WorkspaceContentContext context;
        std::shared_ptr<ShaderLayerModel> model;
        HWND window = nullptr, list = nullptr, help = nullptr, status = nullptr;
        std::array<HWND, 8> buttons{};
        std::vector<std::wstring> labels;
        std::vector<ShdLayer> rows;
        std::function<void(ShdLayer)> openSettings;
        std::wstring shownStatus;
        std::vector<HWND> controls;
        WorkspaceComboDrawing::Context drawing;
        PanelBackBuffer listBuffer;
        int width = 1, height = 1, scroll = 0, wheel = 0;
        bool syncing = false;
        bool dragCandidate = false, dragging = false;
        ShdLayer dragSource{};
        POINT dragStart{}, dragPoint{};
        int dropSlot = -1;
        ShdLayerOrder dragOrder;
        int px(int n) const {
            return int(n * context.scale + .5f);
        }
        bool editable() const {
            return !context.editable || context.editable();
        }
        std::vector<ShdLayer> activeRows() const {
            std::vector<ShdLayer> result;
            for (auto it = model->order().layers.rbegin(); it != model->order().layers.rend(); ++it) {
                if (shaderLayerActive(model->shader(), *it, false)) {
                    result.push_back(*it);
                }
            }
            return result;
        }
        int selectedRow() const {
            const auto it = std::find(rows.begin(), rows.end(), model->selected);
            return it == rows.end() ? -1 : int(it - rows.begin());
        }
        bool moveToRow(int row) {
            if (row < 0 || row >= int(rows.size())) {
                return false;
            }
            const auto &order = model->order().layers;
            return model->moveTo(int(std::find(order.begin(), order.end(), rows[row]) - order.begin()));
        }
        void openSelected() {
            select();
            if (openSettings && selectedRow() >= 0) {
                openSettings(model->selected);
            }
        }
        static void enableControl(HWND control, bool enabled) {
            if (bool(IsWindowEnabled(control)) != enabled) {
                EnableWindow(control, enabled);
            }
        }
        static void positionControl(HWND control, int x, int y, int w, int h) {
            RECT before;
            GetWindowRect(control, &before);
            MapWindowPoints(nullptr, GetParent(control), reinterpret_cast<POINT *>(&before), 2);
            if (before.left != x || before.top != y || before.right - before.left != w ||
                before.bottom - before.top != h) {
                SetWindowPos(control, nullptr, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
        void select() {
            const int row = int(SendMessageW(list, LB_GETCURSEL, 0, 0));
            if (row >= 0 && row < int(ShdLayerOrder::COUNT)) {
                model->selected = ShdLayer(SendMessageW(list, LB_GETITEMDATA, row, 0));
            }
        }
        void invoke(int action) {
            if (!editable()) {
                return;
            }
            select();
            bool changed = false;
            const int row = selectedRow();
            if (action <= 3 && row < 0) {
                return;
            }
            if (action == 0) {
                changed = moveToRow(row - 1);
            } else if (action == 1) {
                changed = moveToRow(row + 1);
            } else if (action == 2) {
                changed = moveToRow(0);
            } else if (action == 3) {
                changed = moveToRow(int(rows.size()) - 1);
            } else if (action == 4 || action == 5) {
                if (context.history) {
                    context.history(action == 5);
                    refresh();
                }
            } else if (action == 6) {
                changed = model->reset();
            } else if (action == 7 && row >= 0) {
                changed = model->toggleVisible();
            }
            if (changed) {
                refresh();
                if (context.changed) {
                    context.changed();
                }
            }
        }
        void cancelDrag() {
            const bool painted = dragging;
            dragCandidate = false;
            dragging = false;
            dropSlot = -1;
            KillTimer(list, 71);
            if (GetCapture() == list) {
                ReleaseCapture();
            }
            if (painted) {
                InvalidateRect(list, nullptr, FALSE);
            }
        }
        void finishDrag(LPARAM l) {
            const bool valid = editable() && dragOrder == model->order() &&
                               activeRows() == rows;
            const bool commit = valid && dragging && dropSlot >= 0;
            const auto hit = SendMessageW(list, LB_ITEMFROMPOINT, 0, l);
            const bool open = valid && !dragging && !HIWORD(hit) &&
                              int(LOWORD(hit)) == selectedRow() &&
                              GET_X_LPARAM(l) >= px(26);
            const auto source = std::find(rows.begin(), rows.end(), dragSource);
            const int from = source == rows.end() ? -1 : int(source - rows.begin());
            const int to = dropSlot - (dropSlot > from ? 1 : 0);
            cancelDrag();
            if (commit && from >= 0) {
                model->selected = dragSource;
            }
            if (commit && from >= 0 && moveToRow(to)) {
                refresh();
                if (context.changed) {
                    context.changed();
                }
            }
            if (open) {
                openSelected();
            }
        }
        void updateDrop(POINT point) {
            dragPoint = point;
            RECT bounds;
            GetClientRect(list, &bounds);
            const int previous = dropSlot;
            if (rows.empty() || point.x < 0 || point.x >= bounds.right) {
                dropSlot = -1;
                if (previous != dropSlot) {
                    InvalidateRect(list, nullptr, FALSE);
                }
                return;
            }
            const auto hit =
                SendMessageW(list, LB_ITEMFROMPOINT, 0,
                             MAKELPARAM(point.x, std::clamp(point.y, 0L, std::max(0L, bounds.bottom - 1))));
            const int row = std::clamp(int(LOWORD(hit)), 0, int(rows.size()) - 1);
            RECT item{};
            SendMessageW(list, LB_GETITEMRECT, row, LPARAM(&item));
            dropSlot = std::clamp(row + (point.y >= (item.top + item.bottom) / 2), 0, int(rows.size()));
            if (previous != dropSlot) {
                InvalidateRect(list, nullptr, FALSE);
            }
        }
        void drawDrop(HDC dc) {
            if (!dragging || dropSlot < 0 || rows.empty()) {
                return;
            }
            RECT item{};
            const int row = std::min(dropSlot, int(rows.size()) - 1);
            SendMessageW(list, LB_GETITEMRECT, row, LPARAM(&item));
            int y = dropSlot == int(rows.size()) ? item.bottom : item.top;
            PanelDrawing::fill(dc, {item.left, y - px(1), item.right, y + px(2)}, context.theme.accent);
        }
        void drawVisibilityMark(HDC dc, RECT bounds, bool visible, bool selected) const {
            const auto fill = selected ? context.theme.selected : context.theme.field;
            const auto edge = visible ? context.theme.secondary : context.theme.track;
            PanelDrawing::rounded(dc, bounds, fill, edge, px(2));
            if (!visible) {
                return;
            }
            HPEN pen = CreatePen(PS_SOLID, std::max(1, px(1)), context.theme.accent);
            const auto old = SelectObject(dc, pen);
            const int x = bounds.left, y = bounds.top, w = bounds.right - bounds.left,
                      h = bounds.bottom - bounds.top;
            POINT tick[] = {
                {x + w / 5, y + h * 5 / 12}, {x + w * 7 / 16, y + h * 2 / 3}, {x + w * 4 / 5, y + h / 4}};
            Polyline(dc, tick, 3);
            SelectObject(dc, old);
            DeleteObject(pen);
        }
        void drawLayerRow(const DRAWITEMSTRUCT &item) const {
            if (item.itemID >= labels.size()) {
                return;
            }
            const auto layer = ShdLayer(item.itemData);
            const bool visible = model->order().visible(layer),
                       selected = (item.itemState & ODS_SELECTED) != 0;
            const auto &theme = context.theme;
            PanelDrawing::fill(item.hDC, item.rcItem, selected ? theme.selected : theme.field);
            RECT check = item.rcItem;
            check.left += px(8);
            check.right = check.left + px(12);
            check.top += (check.bottom - check.top - px(12)) / 2;
            check.bottom = check.top + px(12);
            drawVisibilityMark(item.hDC, check, visible, selected);
            RECT text = item.rcItem;
            text.left += px(28);
            text.right -= px(4);
            PanelDrawing::text(item.hDC, labels[item.itemID], text,
                               selected  ? theme.selectedForeground
                               : visible ? theme.foreground
                                         : theme.secondary,
                               context.font);
            if ((item.itemState & ODS_FOCUS) && !(item.itemState & ODS_NOFOCUSRECT)) {
                DrawFocusRect(item.hDC, &item.rcItem);
            }
        }
        void paintList(HDC dc) {
            RECT bounds;
            GetClientRect(list, &bounds);
            PanelDrawing::fill(dc, bounds, context.theme.field);
            const int selected = int(SendMessageW(list, LB_GETCURSEL, 0, 0));
            const bool focus =
                GetFocus() == list && !(SendMessageW(list, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS);
            for (int row = std::max(0, int(SendMessageW(list, LB_GETTOPINDEX, 0, 0))); row < int(rows.size());
                 ++row) {
                RECT item{};
                if (SendMessageW(list, LB_GETITEMRECT, row, LPARAM(&item)) == LB_ERR ||
                    item.top >= bounds.bottom) {
                    break;
                }
                DRAWITEMSTRUCT draw{ODT_LISTBOX,
                                    12,
                                    UINT(row),
                                    ODA_DRAWENTIRE,
                                    UINT(row == selected ? ODS_SELECTED | (focus ? ODS_FOCUS : 0) : 0),
                                    list,
                                    dc,
                                    item,
                                    ULONG_PTR(SendMessageW(list, LB_GETITEMDATA, row, 0))};
                drawLayerRow(draw);
            }
            drawDrop(dc);
        }
        void showOrderMenu(POINT point) {
            cancelDrag();
            SetFocus(list);
            if (point.x == -1 && point.y == -1) {
                RECT bounds;
                GetWindowRect(list, &bounds);
                point = {bounds.left + px(16), bounds.top + px(16)};
            }
            const auto menu = CreatePopupMenu();
            if (!menu) {
                return;
            }
            const UINT available = editable() ? MF_ENABLED : MF_GRAYED;
            AppendMenuW(menu, MF_STRING | available | (model->order().enabled ? MF_CHECKED : MF_UNCHECKED), 1,
                        UiLanguage::text(L"Custom Layer Order").c_str());
            AppendMenuW(menu, MF_STRING | available, 2, UiLanguage::text(L"Restore Original Order").c_str());
            const int command = TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
                                                 point.x, point.y, window, nullptr);
            DestroyMenu(menu);
            if (!editable()) {
                return;
            }
            if (command == 1) {
                const bool changed = model->enable(!model->order().enabled);
                refresh();
                if (changed && context.changed) {
                    context.changed();
                }
            } else if (command == 2) {
                invoke(6);
            }
        }
        void focusStep(HWND current, int step) {
            std::vector<HWND> enabled;
            for (auto control : controls) {
                if (IsWindowEnabled(control)) {
                    enabled.push_back(control);
                }
            }
            auto at = std::find(enabled.begin(), enabled.end(), current);
            int next = at == enabled.end() ? (step > 0 ? 0 : int(enabled.size()) - 1)
                                           : int(at - enabled.begin()) + step;
            if (next < 0 || next >= int(enabled.size())) {
                if (context.leaveFocus) {
                    context.leaveFocus(step);
                }
                return;
            }
            SetFocus(enabled[next]);
        }
        void reveal(HWND control) {
            RECT r;
            GetWindowRect(control, &r);
            MapWindowPoints(nullptr, window, reinterpret_cast<POINT *>(&r), 2);
            const int delta = r.top < 0 ? r.top : r.bottom > height ? r.bottom - height : 0;
            if (delta) {
                scroll += delta;
                layout(width, height);
            }
        }
        static LRESULT CALLBACK childProc(HWND child, UINT msg, WPARAM w, LPARAM l, UINT_PTR id,
                                          DWORD_PTR data) {
            auto &self = *reinterpret_cast<ShaderLayerWorkspace *>(data);
            if (child == self.list) {
                if (msg == WM_CONTEXTMENU) {
                    self.showOrderMenu({GET_X_LPARAM(l), GET_Y_LPARAM(l)});
                    return 0;
                }
                if (msg == WM_LBUTTONDOWN) {
                    POINT point{GET_X_LPARAM(l), GET_Y_LPARAM(l)};
                    const auto hit = SendMessageW(child, LB_ITEMFROMPOINT, 0, l);
                    if (!HIWORD(hit) && LOWORD(hit) < self.rows.size()) {
                        SetFocus(child);
                        SendMessageW(child, LB_SETCURSEL, LOWORD(hit), 0);
                        self.select();
                        if (point.x < self.px(26)) {
                            self.invoke(7);
                            return 0;
                        }
                        self.refresh();
                        if (self.editable()) {
                            if (GetCapture() != child) {
                                SetCapture(child);
                            }
                            self.dragCandidate = true;
                            self.dragSource = self.model->selected;
                            self.dragStart = point;
                            self.dragOrder = self.model->order();
                        }
                        return 0;
                    }
                }
                if (msg == WM_MOUSEMOVE && self.dragCandidate && (w & MK_LBUTTON)) {
                    if (!self.editable() || self.model->order() != self.dragOrder ||
                        self.activeRows() != self.rows) {
                        self.cancelDrag();
                        self.refresh();
                        return 0;
                    }
                    POINT point{GET_X_LPARAM(l), GET_Y_LPARAM(l)};
                    if (!self.dragging && (std::abs(point.x - self.dragStart.x) >= self.px(4) ||
                                           std::abs(point.y - self.dragStart.y) >= self.px(4))) {
                        self.dragging = true;
                        if (GetCapture() != child) {
                            SetCapture(child);
                        }
                        SetTimer(child, 71, 60, nullptr);
                    }
                    if (self.dragging) {
                        self.updateDrop(point);
                        return 0;
                    }
                }
                if (msg == WM_TIMER && w == 71 && self.dragging) {
                    if (self.activeRows() != self.rows) {
                        self.cancelDrag();
                        self.refresh();
                        return 0;
                    }
                    RECT r;
                    GetClientRect(child, &r);
                    const int oldTop = int(SendMessageW(child, LB_GETTOPINDEX, 0, 0));
                    int top = oldTop;
                    if (self.dragPoint.y < self.px(20)) {
                        --top;
                    } else if (self.dragPoint.y > r.bottom - self.px(20)) {
                        ++top;
                    }
                    top = std::clamp(top, 0, std::max(0, int(self.rows.size()) - 1));
                    if (top != oldTop) {
                        SendMessageW(child, LB_SETTOPINDEX, top, 0);
                        self.updateDrop(self.dragPoint);
                        InvalidateRect(child, nullptr, FALSE);
                    }
                    return 0;
                }
                if (msg == WM_LBUTTONUP && self.dragCandidate) {
                    self.finishDrag(l);
                    return 0;
                }
                if (msg == WM_CANCELMODE || (msg == WM_CAPTURECHANGED && self.dragCandidate)) {
                    self.cancelDrag();
                    return 0;
                }
                if (msg == WM_KEYDOWN && w == VK_ESCAPE && self.dragCandidate) {
                    self.cancelDrag();
                    return 0;
                }
                if (msg == WM_KEYDOWN && w == VK_SPACE) {
                    self.invoke(7);
                    return 0;
                }
                if (msg == WM_KEYDOWN && w == VK_RETURN) {
                    self.openSelected();
                    return 0;
                }
                if (msg == WM_KEYDOWN && (w == VK_UP || w == VK_DOWN) && !(GetKeyState(VK_SHIFT) & 0x8000) &&
                    !(GetKeyState(VK_CONTROL) & 0x8000)) {
                    self.invoke(w == VK_UP ? 0 : 1);
                    return 0;
                }
                if (msg == WM_CHAR && w == VK_SPACE) {
                    return 0;
                }
                if (msg == WM_ERASEBKGND) {
                    return 1;
                }
                if (msg == WM_PRINTCLIENT) {
                    self.paintList(HDC(w));
                    return 0;
                }
                if (msg == WM_PAINT) {
                    PAINTSTRUCT ps;
                    auto target = BeginPaint(child, &ps);
                    try {
                        RECT r;
                        GetClientRect(child, &r);
                        if (auto dc = self.listBuffer.begin(target, r.right, r.bottom)) {
                            self.paintList(dc);
                            self.listBuffer.present(target);
                        }
                    } catch (...) {
                        EndPaint(child, &ps);
                        throw;
                    }
                    EndPaint(child, &ps);
                    return 0;
                }
            }
            if (msg == WM_SETFOCUS) {
                self.reveal(child);
                self.refresh();
            }
            if (msg == WM_KILLFOCUS && child == self.list) {
                PostMessageW(self.window, WM_APP + 1, 0, 0);
            }
            if (msg == WM_GETDLGCODE) {
                return DefSubclassProc(child, msg, w, l) | DLGC_WANTTAB | DLGC_WANTARROWS;
            }
            if (msg == WM_KEYDOWN && w == VK_TAB) {
                self.focusStep(child, (GetKeyState(VK_SHIFT) & 0x8000) ? -1 : 1);
                return 0;
            }
            if (msg == WM_KEYDOWN && w == VK_RETURN && child != self.list) {
                SendMessageW(child, BM_CLICK, 0, 0);
                return 0;
            }
            if (msg == WM_CHAR && (w == VK_TAB || w == VK_RETURN)) {
                return 0;
            }
            if ((msg == WM_SYSKEYDOWN || msg == WM_KEYDOWN) && (GetKeyState(VK_MENU) & 0x8000) &&
                child == self.list) {
                if (w == VK_UP || w == VK_DOWN || w == VK_HOME || w == VK_END) {
                    self.invoke(w == VK_UP ? 0 : w == VK_DOWN ? 1 : w == VK_HOME ? 2 : 3);
                    return 0;
                }
            }
            if (msg == WM_KEYDOWN && (GetKeyState(VK_CONTROL) & 0x8000) && (w == 'Z' || w == 'Y')) {
                self.invoke(w == 'Y' || (GetKeyState(VK_SHIFT) & 0x8000) ? 5 : 4);
                return 0;
            }
            if (msg == WM_NCDESTROY) {
                RemoveWindowSubclass(child, childProc, id);
            }
            return DefSubclassProc(child, msg, w, l);
        }
        static LRESULT CALLBACK procedure(HWND handle, UINT msg, WPARAM w, LPARAM l) {
            auto *self = reinterpret_cast<ShaderLayerWorkspace *>(GetWindowLongPtrW(handle, GWLP_USERDATA));
            if (msg == WM_NCCREATE) {
                self =
                    static_cast<ShaderLayerWorkspace *>(reinterpret_cast<CREATESTRUCTW *>(l)->lpCreateParams);
                self->window = handle;
                SetWindowLongPtrW(handle, GWLP_USERDATA, LONG_PTR(self));
            }
            if (!self) {
                return DefWindowProcW(handle, msg, w, l);
            }
            switch (msg) {
            case WM_APP + 1:
                self->refresh();
                return 0;
            case WM_CONTEXTMENU:
                self->showOrderMenu({GET_X_LPARAM(l), GET_Y_LPARAM(l)});
                return 0;
            case WM_ERASEBKGND:
                return 1;
            case WM_SETFOCUS:
                self->focus();
                return 0;
            case WM_COMMAND:
                if (self->syncing) {
                    return 0;
                }
                if (HWND(l) == self->list && HIWORD(w) == LBN_SELCHANGE) {
                    self->select();
                    self->refresh();
                    return 0;
                }
                if (HIWORD(w) == BN_CLICKED && LOWORD(w) >= 100 && LOWORD(w) < 108) {
                    self->invoke(LOWORD(w) - 100);
                    return 0;
                }
                break;
            case WM_MEASUREITEM:
                reinterpret_cast<MEASUREITEMSTRUCT *>(l)->itemHeight = self->px(24);
                return TRUE;
            case WM_DRAWITEM: {
                auto &item = *reinterpret_cast<DRAWITEMSTRUCT *>(l);
                if (item.CtlType == ODT_BUTTON) {
                    WorkspaceButton::draw(item, self->drawing);
                    return TRUE;
                }
                if (item.CtlType == ODT_LISTBOX) {
                    self->drawLayerRow(item);
                    return TRUE;
                }
                break;
            }
            case WM_CTLCOLORLISTBOX:
            case WM_CTLCOLORSTATIC: {
                auto dc = HDC(w);
                auto bg =
                    msg == WM_CTLCOLORLISTBOX ? self->context.theme.field : self->context.theme.background;
                SetTextColor(dc, HWND(l) == self->status ? self->context.theme.secondary
                                                         : self->context.theme.foreground);
                SetBkColor(dc, bg);
                SetDCBrushColor(dc, bg);
                return LRESULT(GetStockObject(DC_BRUSH));
            }
            case WM_MOUSEWHEEL:
                self->wheel += GET_WHEEL_DELTA_WPARAM(w);
                self->scroll -= self->wheel / WHEEL_DELTA * self->px(40);
                self->wheel %= WHEEL_DELTA;
                self->layout(self->width, self->height);
                return 0;
            case WM_VSCROLL: {
                SCROLLINFO info{sizeof(info), SIF_TRACKPOS};
                GetScrollInfo(handle, SB_VERT, &info);
                switch (LOWORD(w)) {
                case SB_LINEUP:
                    self->scroll -= self->px(32);
                    break;
                case SB_LINEDOWN:
                    self->scroll += self->px(32);
                    break;
                case SB_PAGEUP:
                    self->scroll -= self->height;
                    break;
                case SB_PAGEDOWN:
                    self->scroll += self->height;
                    break;
                case SB_THUMBTRACK:
                    self->scroll = info.nTrackPos;
                    break;
                }
                self->layout(self->width, self->height);
                return 0;
            }
            case WM_PRINTCLIENT: {
                RECT r;
                GetClientRect(handle, &r);
                PanelDrawing::fill(HDC(w), r, self->context.theme.background);
                return 0;
            }
            case WM_PAINT: {
                PAINTSTRUCT ps;
                auto dc = BeginPaint(handle, &ps);
                RECT r;
                GetClientRect(handle, &r);
                PanelDrawing::fill(dc, r, self->context.theme.background);
                EndPaint(handle, &ps);
                return 0;
            }
            }
            return DefWindowProcW(handle, msg, w, l);
        }

      public:
        ShaderLayerWorkspace(WorkspaceContentContext context, std::shared_ptr<ShaderLayerModel> model,
                             std::function<void(ShdLayer)> openSettings = {})
            : context(std::move(context)), model(std::move(model)), openSettings(std::move(openSettings)),
              drawing{&this->context.theme, this->context.font, this->context.scale} {
            WNDCLASSW cls{};
            cls.lpfnWndProc = procedure;
            cls.hInstance = GetModuleHandleW(nullptr);
            cls.lpszClassName = L"RFF.Workspace.ShaderLayers";
            cls.hCursor = LoadCursor(nullptr, IDC_ARROW);
            RegisterClassW(&cls);
            window = CreateWindowExW(WS_EX_CONTROLPARENT, cls.lpszClassName,
                                     UiLanguage::text(L"Shader Layers").c_str(),
                                     WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_VSCROLL, 0, 0, 1, 1,
                                     this->context.parent, nullptr, cls.hInstance, this);
            const auto child = [&](const wchar_t *type, const wchar_t *title, DWORD style, int id) {
                return CreateWindowExW(0, type, UiLanguage::text(title).c_str(),
                                       WS_CHILD | WS_VISIBLE | style, 0, 0, 1, 1, window, HMENU(INT_PTR(id)),
                                       cls.hInstance, nullptr);
            };
            help = child(L"STATIC", L"Shader Layers", SS_LEFT, 11);
            list = child(WC_LISTBOXW, L"Layer Order",
                         WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED |
                             LBS_HASSTRINGS,
                         12);
            status = child(L"STATIC", L"", SS_LEFT, 13);
            controls = {list};
            const wchar_t *names[] = {L"Move Up", L"Move Down", L"Bring to Front", L"Send to Back",
                                      L"Undo",    L"Redo",      L"Reset",          L"Hide Layer"};
            for (int i : {0, 1, 6}) {
                buttons[i] = child(L"BUTTON", names[i], WS_TABSTOP | BS_PUSHBUTTON, 100 + i);
                WorkspaceButton::attach(buttons[i]);
                AccessibleControl::describe(buttons[i], names[i], L"Layer Order",
                                            L"layers.action." + std::to_wstring(i));
                controls.push_back(buttons[i]);
            }
            for (auto control : controls) {
                SetWindowSubclass(control, childProc, 7, DWORD_PTR(this));
            }
            AccessibleControl::describe(
                list, L"Layer Order",
                L"Higher layers are applied later. Up and Down move the selected layer. Shift with an arrow "
                L"selects another layer. Space shows or hides it.",
                L"layers.selected");
            metrics(this->context.font, this->context.scale);
            themeChanged();
            refresh();
        }
        ~ShaderLayerWorkspace() override {
            cancelDrag();
            if (IsWindow(window)) {
                DestroyWindow(window);
            }
        }
        HWND handle() const {
            return window;
        }
        void layout(int w, int h) override {
            width = w;
            height = h;
            positionControl(window, 0, 0, w, h);
            const int statusHeight = px(44);
            const int content = std::max(h, px(180) + statusHeight);
            scroll = std::clamp(scroll, 0, content - h);
            SCROLLINFO info{sizeof(info), SIF_RANGE | SIF_PAGE | SIF_POS};
            info.nMax = content - 1;
            info.nPage = std::max(1, h);
            info.nPos = scroll;
            SetScrollInfo(window, SB_VERT, &info, TRUE);
            RECT r;
            GetClientRect(window, &r);
            const int x = px(16), gap = px(8), available = std::max(1, int(r.right) - 2 * x);
            const auto place = [&](HWND control, int left, int top, int cw, int ch) {
                positionControl(control, left, top - scroll, cw, ch);
            };
            place(help, x, px(16), available, px(24));
            const int listTop = px(48), listHeight = std::max(px(72), content - px(108) - statusHeight);
            place(list, x, listTop, available, listHeight);
            int next = listTop + listHeight + px(10);
            const int cell = std::max(1, (available - 2 * gap) / 3);
            const int order[] = {0, 1, 6};
            for (int i = 0; i < 3; ++i) {
                place(buttons[order[i]], x + i * (cell + gap), next,
                      i == 2 ? available - 2 * (cell + gap) : cell, px(28));
            }
            place(status, x, next + px(38), available, statusHeight);
        }
        void refresh() override {
            if (!list) {
                return;
            }
            syncing = true;
            auto nextRows = activeRows();
            if (dragCandidate && (!editable() || model->order() != dragOrder || nextRows != rows)) {
                cancelDrag();
            }
            std::vector<std::wstring> next;
            for (auto layer : nextRows) {
                auto label = shaderLayerName(layer);
                if (!model->order().visible(layer)) {
                    label += UiLanguage::text(L" (hidden)");
                }
                next.push_back(std::move(label));
            }
            const bool changed = next != labels || nextRows != rows;
            const int previousRow = selectedRow();
            rows = std::move(nextRows);
            if (selectedRow() < 0 && !rows.empty()) {
                model->selected = rows[std::clamp(previousRow, 0, int(rows.size()) - 1)];
            }
            const int selected = selectedRow();
            if (changed) {
                const int top = int(SendMessageW(list, LB_GETTOPINDEX, 0, 0));
                SendMessageW(list, WM_SETREDRAW, FALSE, 0);
                if (SendMessageW(list, LB_GETCOUNT, 0, 0) != LRESULT(rows.size())) {
                    SendMessageW(list, LB_RESETCONTENT, 0, 0);
                    for (const auto &label : next) {
                        SendMessageW(list, LB_ADDSTRING, 0, LPARAM(label.c_str()));
                    }
                } else {
                    for (int row = 0; row < int(rows.size()); ++row) {
                        if (next[row] != labels[row]) {
                            SendMessageW(list, LB_DELETESTRING, row, 0);
                            SendMessageW(list, LB_INSERTSTRING, row, LPARAM(next[row].c_str()));
                        }
                    }
                }
                labels = std::move(next);
                for (int row = 0; row < int(rows.size()); ++row) {
                    SendMessageW(list, LB_SETITEMDATA, row, LPARAM(rows[row]));
                }
                SendMessageW(list, LB_SETTOPINDEX, std::max(0, top), 0);
                SendMessageW(list, LB_SETCURSEL, selected, 0);
                SendMessageW(list, WM_SETREDRAW, TRUE, 0);
                InvalidateRect(list, nullptr, FALSE);
            }
            if (SendMessageW(list, LB_GETCURSEL, 0, 0) != selected) {
                SendMessageW(list, LB_SETCURSEL, selected, 0);
            }
            const bool canEdit = editable();
            for (int i : {0, 1}) {
                enableControl(buttons[i], canEdit && selected >= 0 &&
                                              (i ? selected + 1 < int(rows.size()) : selected > 0));
            }
            enableControl(buttons[6], canEdit && model->order() != ShdLayerOrder{});
            auto text = UiLanguage::text(
                GetFocus() == list
                    ? L"\u2191/\u2193: Move  \u00b7  Space: Show/hide\nTop layers are applied last."
                    : L"Higher layers are applied later.");
            if (model->order().enabled && model->shader().slope.paletteColorMix > 0 &&
                shaderLayerActive(model->shader(), ShdLayer::SURFACE_COLOR)) {
                int color = -1, lastMaterial = -1;
                for (int i = 0; i < int(ShdLayerOrder::COUNT); ++i) {
                    auto layer = model->order().layers[i];
                    if (layer == ShdLayer::SURFACE_COLOR) {
                        color = i;
                    }
                    if (uint32_t(layer) >= 11 && uint32_t(layer) <= 17 &&
                        shaderLayerActive(model->shader(), layer)) {
                        lastMaterial = i;
                    }
                }
                if (color < lastMaterial) {
                    text = UiLanguage::text(
                        L"Material color is below a material.\nMove Material Color & Rim up to tint it.");
                }
            }
            if (!canEdit) {
                text = UiLanguage::text(
                    L"Layers are locked while settings are pending\nor a render/export job is running.");
            }
            if (text != shownStatus) {
                shownStatus = std::move(text);
                SetWindowTextW(status, shownStatus.c_str());
            }
            syncing = false;
        }
        void metrics(HFONT font, float scale) override {
            context.font = font;
            context.scale = scale;
            drawing.font = font;
            drawing.scale = scale;
            for (auto control : controls) {
                SendMessageW(control, WM_SETFONT, WPARAM(font), TRUE);
            }
            for (auto control : {help, status}) {
                SendMessageW(control, WM_SETFONT, WPARAM(font), TRUE);
            }
            SendMessageW(list, LB_SETITEMHEIGHT, 0, px(24));
            layout(width, height);
        }
        void themeChanged() override {
            for (auto control : controls) {
                applyDarkThemeClass(control, false);
            }
            RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
        }
        void focus(std::string_view field = {}) override {
            SetFocus(list);
            if (field == "layers.enabled") {
                PostMessageW(list, WM_CONTEXTMENU, WPARAM(list), MAKELPARAM(-1, -1));
            }
        }
    };
} // namespace merutilm::rff2::workspace
