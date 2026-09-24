//
// Modified by GPT-6 on 2026-09-17, 2026-09-21
//

#pragma once
#include "FormWorkspace.hpp"

namespace merutilm::rff2::workspace {
    class DockedAppearancePanel {
        HWND window = nullptr, body = nullptr, title = nullptr, positionButton = nullptr,
             closeButton = nullptr;
        std::unique_ptr<FormWorkspace> content;
        std::function<void(int)> command;
        std::function<void(int)> leaveFocus;
        const WorkspaceTheme &theme;
        HFONT font;
        float scale;
        int side = 0;
        WorkspaceComboDrawing::Context drawing;
        int px(int n) const {
            return int(n * scale + .5f);
        }
        static LRESULT CALLBACK buttonProc(HWND child, UINT message, WPARAM wParam, LPARAM lParam,
                                           UINT_PTR id, DWORD_PTR data) {
            auto &self = *reinterpret_cast<DockedAppearancePanel *>(data);
            if (message == WM_GETDLGCODE) {
                return DefSubclassProc(child, message, wParam, lParam) | DLGC_WANTTAB;
            }
            if (message == WM_KEYDOWN && wParam == VK_TAB) {
                const bool back = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                if (child == self.positionButton) {
                    if (back) {
                        if (self.leaveFocus) {
                            self.leaveFocus(-1);
                        }
                    } else {
                        SetFocus(self.closeButton);
                    }
                } else if (back) {
                    SetFocus(self.positionButton);
                } else {
                    self.content->focus();
                }
                return 0;
            }
            if (message == WM_KEYDOWN && wParam == VK_RETURN) {
                SendMessageW(child, BM_CLICK, 0, 0);
                return 0;
            }
            if (message == WM_CHAR && (wParam == VK_TAB || wParam == VK_RETURN)) {
                return 0;
            }
            if (message == WM_NCDESTROY) {
                RemoveWindowSubclass(child, buttonProc, id);
            }
            return DefSubclassProc(child, message, wParam, lParam);
        }
        static LRESULT CALLBACK procedure(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) {
            auto *self = reinterpret_cast<DockedAppearancePanel *>(GetWindowLongPtrW(handle, GWLP_USERDATA));
            if (message == WM_NCCREATE) {
                self = static_cast<DockedAppearancePanel *>(
                    reinterpret_cast<CREATESTRUCTW *>(lParam)->lpCreateParams);
                self->window = handle;
                SetWindowLongPtrW(handle, GWLP_USERDATA, LONG_PTR(self));
            }
            if (!self) {
                return DefWindowProcW(handle, message, wParam, lParam);
            }
            if (message == WM_SETFOCUS) {
                if (self->content) {
                    self->content->focus();
                }
                return 0;
            }
            if (message == WM_ERASEBKGND) {
                return 1;
            }
            if (message == WM_COMMAND && HIWORD(wParam) == BN_CLICKED) {
                if (HWND(lParam) == self->closeButton) {
                    self->command(3);
                    return 0;
                }
                if (HWND(lParam) == self->positionButton) {
                    const auto menu = CreatePopupMenu();
                    const wchar_t *names[] = {L"Dock Left", L"Dock Right", L"Dock Bottom"};
                    for (int i = 0; i < 3; ++i) {
                        AppendMenuW(menu, MF_STRING | (i == self->side ? MF_CHECKED : 0), i + 1,
                                    UiLanguage::text(names[i]).c_str());
                    }
                    RECT r;
                    GetWindowRect(self->positionButton, &r);
                    const auto result = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, r.left, r.bottom,
                                                       0, handle, nullptr);
                    DestroyMenu(menu);
                    if (result) {
                        self->command(int(result) - 1);
                    }
                    return 0;
                }
            }
            if (message == WM_DRAWITEM) {
                WorkspaceButton::draw(*reinterpret_cast<DRAWITEMSTRUCT *>(lParam), self->drawing);
                return TRUE;
            }
            if (message == WM_CTLCOLORSTATIC) {
                SetTextColor(HDC(wParam), self->theme.foreground);
                SetBkColor(HDC(wParam), self->theme.background);
                SetDCBrushColor(HDC(wParam), self->theme.background);
                return LRESULT(GetStockObject(DC_BRUSH));
            }
            if (message == WM_PRINTCLIENT) {
                RECT r;
                GetClientRect(handle, &r);
                PanelDrawing::fill(HDC(wParam), r, self->theme.background);
                return 0;
            }
            if (message == WM_PAINT) {
                PAINTSTRUCT ps;
                auto dc = BeginPaint(handle, &ps);
                RECT r;
                GetClientRect(handle, &r);
                PanelDrawing::fill(dc, r, self->theme.background);
                EndPaint(handle, &ps);
                return 0;
            }
            return DefWindowProcW(handle, message, wParam, lParam);
        }

      public:
        DockedAppearancePanel(HWND parent, WorkspaceForm form, HFONT font, const WorkspaceTheme &theme,
                              float scale, int side, std::function<void()> changed,
                              std::function<void(int)> command, std::function<void(int)> leaveFocus)
            : command(std::move(command)), leaveFocus(std::move(leaveFocus)), theme(theme), font(font),
              scale(scale), side(side), drawing{&theme, font, scale} {
            WNDCLASSW cls{};
            cls.lpfnWndProc = procedure;
            cls.hInstance = GetModuleHandleW(nullptr);
            cls.lpszClassName = L"RFF.Workspace.AppearancePanel";
            cls.hCursor = LoadCursor(nullptr, IDC_ARROW);
            RegisterClassW(&cls);
            window =
                CreateWindowExW(WS_EX_CONTROLPARENT, cls.lpszClassName, UiLanguage::text(form.title).c_str(),
                                WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 1, 1, parent, nullptr,
                                cls.hInstance, this);
            body = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0, 1, 1,
                                   window, nullptr, cls.hInstance, nullptr);
            title = CreateWindowExW(0, L"STATIC", UiLanguage::text(form.title).c_str(),
                                    WS_CHILD | WS_VISIBLE | SS_ENDELLIPSIS, 0, 0, 1, 1, window, nullptr,
                                    cls.hInstance, nullptr);
            positionButton = CreateWindowExW(0, L"BUTTON", UiLanguage::text(L"Panel Position").c_str(),
                                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 1, 1,
                                             window, nullptr, cls.hInstance, nullptr);
            closeButton =
                CreateWindowExW(0, L"BUTTON", L"×", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 0, 0,
                                1, 1, window, nullptr, cls.hInstance, nullptr);
            WorkspaceButton::attach(positionButton);
            WorkspaceButton::attach(closeButton);
            AccessibleControl::describe(positionButton, L"Panel Position",
                                        L"Dock this panel on the left, right or bottom.",
                                        L"appearance.panel.position");
            AccessibleControl::describe(closeButton, L"Remove Panel",
                                        L"Close this panel while keeping its settings.",
                                        L"appearance.panel.remove");
            for (auto control : {positionButton, closeButton}) {
                SetWindowSubclass(control, buttonProc, 9, DWORD_PTR(this));
            }
            content = std::make_unique<FormWorkspace>(body, std::move(form), font, theme, scale,
                                                      std::move(changed), [this](int direction) {
                                                          if (direction < 0) {
                                                              SetFocus(closeButton);
                                                          } else if (this->leaveFocus) {
                                                              this->leaveFocus(1);
                                                          }
                                                      });
            metrics(font, scale);
            content->show(true);
        }
        ~DockedAppearancePanel() {
            content.reset();
            if (IsWindow(window)) {
                DestroyWindow(window);
            }
        }
        FormWorkspace &form() {
            return *content;
        }
        HWND handle() const {
            return window;
        }
        void setSide(int value) {
            side = value;
        }
        void layout(RECT r, bool visible) {
            SetWindowPos(window, nullptr, r.left, r.top, std::max(1L, r.right - r.left),
                         std::max(1L, r.bottom - r.top), SWP_NOZORDER | SWP_NOACTIVATE);
            ShowWindow(window, visible ? SW_SHOWNA : SW_HIDE);
            const int width = std::max(1L, r.right - r.left), height = std::max(1L, r.bottom - r.top),
                      head = px(40);
            SetWindowPos(title, nullptr, px(12), px(12), std::max(1, width - px(150)), px(24),
                         SWP_NOZORDER | SWP_NOACTIVATE);
            SetWindowPos(positionButton, nullptr, width - px(126), px(6), px(80), px(28),
                         SWP_NOZORDER | SWP_NOACTIVATE);
            SetWindowPos(closeButton, nullptr, width - px(38), px(6), px(28), px(28),
                         SWP_NOZORDER | SWP_NOACTIVATE);
            SetWindowPos(body, nullptr, 0, head, width, std::max(1, height - head),
                         SWP_NOZORDER | SWP_NOACTIVATE);
            content->layout(width, std::max(1, height - head), true);
        }
        void metrics(HFONT replacement, float value) {
            font = replacement;
            scale = value;
            drawing.font = font;
            drawing.scale = scale;
            for (auto h : {title, positionButton, closeButton}) {
                SendMessageW(h, WM_SETFONT, WPARAM(font), TRUE);
            }
            if (content) {
                content->applyMetrics(font, scale);
            }
        }
        void applyTheme() {
            for (auto h : {positionButton, closeButton}) {
                applyDarkThemeClass(h, false);
            }
            content->applyTheme();
            InvalidateRect(window, nullptr, TRUE);
        }
    };
} // namespace merutilm::rff2::workspace
