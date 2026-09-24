//
// Modified by GPT-6 on 2026-09-17, 2026-09-22
//

#pragma once
#include "SettingsMenu.hpp"
#include "workspace/ShaderLayerModel.hpp"
#include "../vulkan/OrderedShaderLayers.hpp"

namespace merutilm::rff2 {
    class ShaderLayerWindow {
        struct State {
            std::shared_ptr<workspace::ShaderLayerModel> model;
            SettingsWindow *panel = nullptr;
            HWND list = nullptr, enabled = nullptr, up = nullptr, down = nullptr, front = nullptr,
                 back = nullptr, undo = nullptr, redo = nullptr;
            bool enabledValue = false;
            ShdLayerOrder shown{};
            ShdLayer selection = ShdLayer::BAND_LINE;
            uint32_t activeMask = 0;
            uint32_t active() const {
                uint32_t mask = 0;
                for (auto layer : ShdLayerOrder::defaults()) {
                    if (shaderLayerActive(model->shader(), layer)) {
                        mask |= 1u << (uint32_t(layer) - 1);
                    }
                }
                return mask;
            }
            void refresh() {
                const auto &order = model->order();
                shown = order;
                selection = model->selected;
                activeMask = active();
                SendMessageW(list, WM_SETREDRAW, FALSE, 0);
                const int previousTopIndex = int(SendMessageW(list, LB_GETTOPINDEX, 0, 0));
                SendMessageW(list, LB_RESETCONTENT, 0, 0);
                for (int layerIndex = int(ShdLayerOrder::COUNT) - 1; layerIndex >= 0; --layerIndex) {
                    const auto layer = order.layers[layerIndex];
                    auto label = workspace::shaderLayerName(layer);
                    if (!shaderLayerActive(model->shader(), layer)) {
                        label += UiLanguage::text(L" (inactive)");
                    }
                    const auto row =
                        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
                    SendMessageW(list, LB_SETITEMDATA, row, LPARAM(layer));
                    if (layer == model->selected) {
                        SendMessageW(list, LB_SETCURSEL, row, 0);
                    }
                }
                SendMessageW(list, LB_SETTOPINDEX, std::max(0, previousTopIndex), 0);
                SendMessageW(list, WM_SETREDRAW, TRUE, 0);
                InvalidateRect(list, nullptr, FALSE);
                enabledValue = order.enabled;
                panel->setCheckboxValue(enabled, enabledValue);
                panel->setRowEnabled(up, model->position() < int(ShdLayerOrder::COUNT) - 1);
                panel->setRowEnabled(down, model->position() > 0);
                panel->setRowEnabled(front, model->position() < int(ShdLayerOrder::COUNT) - 1);
                panel->setRowEnabled(back, model->position() > 0);
                panel->setRowEnabled(undo, model->canUndo());
                panel->setRowEnabled(redo, model->canRedo());
            }
            void select() {
                const auto row = SendMessageW(list, LB_GETCURSEL, 0, 0);
                if (row != LB_ERR) {
                    model->selected = ShdLayer(SendMessageW(list, LB_GETITEMDATA, row, 0));
                }
            }
            void move(int step) {
                select();
                model->move(step);
                refresh();
                SendMessageW(list, LB_SETTOPINDEX,
                             std::max(0, int(ShdLayerOrder::COUNT) - 1 - model->position() - 4), 0);
            }
            void moveTo(bool front) {
                select();
                move((front ? int(ShdLayerOrder::COUNT) - 1 : 0) - model->position());
            }
        };
        static LRESULT CALLBACK listProc(HWND window, UINT message, WPARAM w, LPARAM l, UINT_PTR id,
                                         DWORD_PTR data) {
            auto &state = *reinterpret_cast<State *>(data);
            if (message == WM_GETDLGCODE) {
                return DefSubclassProc(window, message, w, l) | DLGC_WANTARROWS;
            }
            if ((message == WM_SYSKEYDOWN || message == WM_KEYDOWN) && (GetKeyState(VK_MENU) & 0x8000) &&
                (w == VK_UP || w == VK_DOWN)) {
                state.move(w == VK_UP ? 1 : -1);
                return 0;
            }
            if ((message == WM_SYSKEYDOWN || message == WM_KEYDOWN) && (GetKeyState(VK_MENU) & 0x8000) &&
                (w == VK_HOME || w == VK_END)) {
                state.moveTo(w == VK_HOME);
                return 0;
            }
            if (message == WM_KEYDOWN && (GetKeyState(VK_CONTROL) & 0x8000) && (w == 'Z' || w == 'Y')) {
                state.model->restore(w == 'Y' || (GetKeyState(VK_SHIFT) & 0x8000));
                state.refresh();
                return 0;
            }
            if (message == WM_NCDESTROY) {
                RemoveWindowSubclass(window, listProc, id);
            }
            return DefSubclassProc(window, message, w, l);
        }
        static LRESULT CALLBACK hostProc(HWND window, UINT message, WPARAM w, LPARAM l, UINT_PTR id,
                                         DWORD_PTR data) {
            auto &state = *reinterpret_cast<State *>(data);
            if (message == WM_SIZE) {
                MoveWindow(state.list, 0, 0, LOWORD(l), HIWORD(l), TRUE);
                SendMessageW(state.list, WM_SETFONT, SendMessageW(state.enabled, WM_GETFONT, 0, 0), TRUE);
                return 0;
            }
            if (message == WM_TIMER) {
                if (state.shown != state.model->order() || state.selection != state.model->selected ||
                    state.activeMask != state.active()) {
                    state.refresh();
                }
                return 0;
            }
            if (message == WM_COMMAND && HIWORD(w) == LBN_SELCHANGE) {
                state.select();
                state.refresh();
                return 0;
            }
            if (message == WM_CTLCOLORLISTBOX) {
                const auto &theme = settingsTheme();
                SetTextColor(HDC(w), theme.text);
                SetBkColor(HDC(w), theme.textFieldBackground);
                SetDCBrushColor(HDC(w), theme.textFieldBackground);
                return LRESULT(GetStockObject(DC_BRUSH));
            }
            if (message == WM_NCDESTROY) {
                RemoveWindowSubclass(window, hostProc, id);
            }
            return DefSubclassProc(window, message, w, l);
        }

      public:
        static void open(SettingsMenu &menu, std::shared_ptr<workspace::ShaderLayerModel> model) {
            auto state = std::make_shared<State>();
            state->model = std::move(model);
            auto window = std::make_unique<SettingsWindow>(L"Shader Layers", 480);
            state->panel = window.get();
            state->enabledValue = state->model->order().enabled;
            state->enabled = window->registerCheckboxInput(
                L"Custom Layer Order", &state->enabledValue,
                [state] {
                    state->model->enable(state->enabledValue);
                    state->refresh();
                },
                L"Layer Order",
                L"Moving a layer enables custom compositing. Disabling restores the original rendering.");
            window->registerStaticText(
                L"Top layers are applied last. Select a layer, then move it up or down.");
            const HWND host = window->registerOwnerDrawnPanel(Constants::Win32::settingsScaled(300),
                                                              [](HDC, const RECT &) {});
            SetWindowLongPtrW(host, GWL_EXSTYLE, GetWindowLongPtrW(host, GWL_EXSTYLE) | WS_EX_CONTROLPARENT);
            RECT rect;
            GetClientRect(host, &rect);
            state->list = CreateWindowExW(
                WS_EX_CLIENTEDGE, WC_LISTBOXW, UiLanguage::text(L"Layer Order").c_str(),
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, 0, 0,
                rect.right, rect.bottom, host, nullptr, GetModuleHandleW(nullptr), nullptr);
            SendMessageW(state->list, WM_SETFONT, SendMessageW(state->enabled, WM_GETFONT, 0, 0), TRUE);
            SetWindowSubclass(host, hostProc, 1, DWORD_PTR(state.get()));
            SetWindowSubclass(state->list, listProc, 1, DWORD_PTR(state.get()));
            SetTimer(host, 1, 250, nullptr);
            state->up = window->registerButton(
                L"Move Up", L"Move Up", [state] { state->move(1); }, L"Move Up",
                L"Apply this layer later. Shortcut: Alt+Up.");
            state->down = window->registerButton(
                L"Move Down", L"Move Down", [state] { state->move(-1); }, L"Move Down",
                L"Apply this layer earlier. Shortcut: Alt+Down.");
            state->front = window->registerButton(
                L"Bring to Front", L"Bring to Front", [state] { state->moveTo(true); }, L"Bring to Front",
                L"Apply this layer last. Shortcut: Alt+Home.");
            state->back = window->registerButton(
                L"Send to Back", L"Send to Back", [state] { state->moveTo(false); }, L"Send to Back",
                L"Apply this layer first. Shortcut: Alt+End.");
            state->undo = window->registerButton(
                L"Undo", L"Undo",
                [state] {
                    state->model->restore(false);
                    state->refresh();
                },
                L"Undo", L"Undo a layer order change. Ctrl+Z in the list.");
            state->redo = window->registerButton(
                L"Redo", L"Redo",
                [state] {
                    state->model->restore(true);
                    state->refresh();
                },
                L"Redo", L"Redo a layer order change. Ctrl+Y in the list.");
            window->registerButton(
                L"Reset", L"Restore Original Order",
                [state] {
                    state->model->reset();
                    state->refresh();
                },
                L"Restore Original Order",
                L"Restore the original rendering without changing effect values. This can be undone.");
            window->registerHelpButton(
                L"Layer Order",
                {{L"Compositing",
                  L"All visual shader effects can be reordered. Palette and Warp supply the coordinates; "
                  L"display encoding and antialiasing remain at the output. Custom compositing can change "
                  L"brightness. Save Settings or Save Shader Preset stores the order."}});
            window->setWindowCloseFunction([state] {});
            state->refresh();
            SendMessageW(state->list, LB_SETTOPINDEX,
                         std::max(0, int(ShdLayerOrder::COUNT) - 1 - state->model->position() - 4), 0);
            menu.setCurrentActiveSettingsWindow(std::move(window), SettingsMenu::SettingsWindowKind::SHADER);
        }
    };
}
