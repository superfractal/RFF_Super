//
// Modified by GPT-6 on 2026-09-13, 2026-09-22, 2026-09-23
//

#pragma once
#include <windows.h>

namespace merutilm::rff2::workspace {
    class PanelBackBuffer {
        HDC memoryContext = nullptr;
        HBITMAP bufferBitmap = nullptr;
        HGDIOBJ previousBitmap = nullptr;
        int bufferWidth = 0;
        int bufferHeight = 0;

    public:
        PanelBackBuffer() = default;
        PanelBackBuffer(const PanelBackBuffer &) = delete;
        PanelBackBuffer &operator=(const PanelBackBuffer &) = delete;
        ~PanelBackBuffer() { reset(); }

        void reset() {
            if (previousBitmap) {
                SelectObject(memoryContext, previousBitmap);
            }
            if (bufferBitmap) {
                DeleteObject(bufferBitmap);
            }
            if (memoryContext) {
                DeleteDC(memoryContext);
            }
            previousBitmap = nullptr;
            bufferBitmap = nullptr;
            memoryContext = nullptr;
            bufferWidth = 0;
            bufferHeight = 0;
        }

        HDC begin(HDC target, int width, int height) {
            if (width <= 0 || height <= 0) {
                return nullptr;
            }
            if (bufferWidth != width || bufferHeight != height) {
                reset();
                memoryContext = CreateCompatibleDC(target);
                bufferBitmap = CreateCompatibleBitmap(target, width, height);
                if (!memoryContext || !bufferBitmap) {
                    reset();
                    return nullptr;
                }
                previousBitmap = SelectObject(memoryContext, bufferBitmap);
                if (!previousBitmap || previousBitmap == HGDI_ERROR) {
                    previousBitmap = nullptr;
                    reset();
                    return nullptr;
                }
                bufferWidth = width;
                bufferHeight = height;
            }
            return memoryContext;
        }

        void present(HDC target) const {
            if (memoryContext) {
                BitBlt(target, 0, 0, bufferWidth, bufferHeight, memoryContext, 0, 0, SRCCOPY);
            }
        }
    };
}
