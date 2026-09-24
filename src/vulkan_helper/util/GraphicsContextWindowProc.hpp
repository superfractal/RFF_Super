//
// Created by Merutilm on 2025-08-24.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include "../impl/GraphicsContextWindow.hpp"

namespace merutilm::vkh {
    struct GraphicsContextWindowProc {
        explicit GraphicsContextWindowProc() = delete;

        static LRESULT CALLBACK WinProc(const HWND hwnd, const UINT message, const WPARAM wparam,
                                        const LPARAM lparam) {
            const auto window = reinterpret_cast<GraphicsContextWindowPtr>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

            if (window == nullptr) {
                return DefWindowProcW(hwnd, message, wparam, lparam);
            }

            if (message == WM_DESTROY) {
                PostQuitMessage(0);
            }

            return runListeners(*window, hwnd, message, wparam, lparam);
        }

        static LRESULT runListeners(GraphicsContextWindowRef window, const HWND hwnd, const UINT message, const WPARAM wparam,
                                    const LPARAM lparam) {
            const auto &listeners = window.getListeners();
            const auto listener = listeners.find(message);
            if (listener != listeners.end()) {
                return listener->second(window, hwnd, wparam, lparam);
            }
            return DefWindowProcW(hwnd, message, wparam, lparam);
        }
    };
}
