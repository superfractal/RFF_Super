//
// Created by Merutilm on 2025-07-07.
// Modified by Opus 5 on 2026-08-05
//

#include "GraphicsContextWindow.hpp"
#include <chrono>
#include <windows.h>

namespace merutilm::vkh {
    GraphicsContextWindowImpl::GraphicsContextWindowImpl(const HWND window) : window(window) {
    }

    void GraphicsContextWindowImpl::renderOnce() const {
        for (const auto &renderer: renderers) {
            renderer();
        }
    }

    void GraphicsContextWindowImpl::start() const {
        MSG message;
        using namespace std::chrono;
        auto started = high_resolution_clock::now();

        while (true) {
            if (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&message);
                DispatchMessageW(&message);

                if (message.message == WM_QUIT) {
                    break;
                }
            }

            auto now = high_resolution_clock::now();

            // Sub-millisecond comparison. Truncating to whole milliseconds turned a 60 FPS request
            // into roughly 57, which beats against a 60 Hz display: with MAILBOX presentation a
            // vblank then finds no new image and repeats the previous one, showing up as periodic
            // judder in anything that animates.
            if (const duration<float> elapsed = now - started; elapsed.count() * framerate >= 1.0f) {
                started = now;
                for (const auto &renderer: renderers) {
                    renderer();
                }
            }
        }
    }
}
