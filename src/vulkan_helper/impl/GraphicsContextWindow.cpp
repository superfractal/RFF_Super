//
// Created by Merutilm on 2025-07-07.
// Modified by Opus 5 on 2026-08-05
// Modified by GPT-6 on 2026-09-14, 2026-09-15, 2026-09-23
//

#include "GraphicsContextWindow.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <windows.h>

namespace merutilm::vkh {
    GraphicsContextWindowImpl::GraphicsContextWindowImpl(const HWND window) : window(window) {
    }

    void GraphicsContextWindowImpl::renderOnce() const {
        for (const auto &renderer: renderers) {
            renderer();
        }
    }

    void GraphicsContextWindowImpl::dispatchMessage(const MSG &message) const {
        if (messageFilter && messageFilter(message)) {
            return;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    void GraphicsContextWindowImpl::start() const {
        constexpr int maxInputMessages = 64;
        constexpr int maxPaintMessages = 32;
        constexpr auto inputTimeBudget = std::chrono::milliseconds(4);
        constexpr auto paintTimeBudget = std::chrono::milliseconds(2);

        MSG message;
        using namespace std::chrono;
        auto started = steady_clock::now();

        while (true) {
            const auto inputDeadline = steady_clock::now() + inputTimeBudget;
            for (int handled = 0;
                 handled < maxInputMessages && PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE);
                 ++handled) {
                dispatchMessage(message);
                if (message.message == WM_QUIT) {
                    return;
                }
                if (steady_clock::now() >= inputDeadline) {
                    break;
                }
            }
            // Paint pending UI changes before starting another potentially expensive render.
            const auto paintDeadline = steady_clock::now() + paintTimeBudget;
            for (int painted = 0;
                 painted < maxPaintMessages && PeekMessageW(&message, nullptr, WM_PAINT, WM_PAINT, PM_REMOVE);
                 ++painted) {
                dispatchMessage(message);
                if (message.message == WM_QUIT) {
                    return;
                }
                if (steady_clock::now() >= paintDeadline) {
                    break;
                }
            }

            auto now = steady_clock::now();

            if (framerate > 0.0f && std::isfinite(framerate)) {
                const duration<float> remaining = duration<float>(1.0f / framerate) - (now - started);
                if (remaining > duration<float>::zero()) {
                    const auto milliseconds = duration_cast<std::chrono::milliseconds>(remaining).count();
                    const DWORD timeout = static_cast<DWORD>(std::min<int64_t>(1000, std::max<int64_t>(1, milliseconds)));
                    MsgWaitForMultipleObjectsEx(0, nullptr, timeout, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
                    continue;
                }
            }

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
