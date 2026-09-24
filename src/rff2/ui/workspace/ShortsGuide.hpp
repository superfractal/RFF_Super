//
// Modified by GPT-6 on 2026-09-20, 2026-09-23
//

#pragma once
#include <windows.h>
#include <gdiplus.h>

namespace merutilm::rff2::workspace {
    struct ShortsGuide {
        bool visible = false;
        bool showDescriptions = false;
        int top = 10;
        int bottom = 20;
        int left = 6;
        int right = 18;

        RECT safeRect(RECT image) const {
            const int imageWidth = image.right - image.left;
            const int imageHeight = image.bottom - image.top;
            return {image.left + imageWidth * left / 100, image.top + imageHeight * top / 100,
                    image.right - imageWidth * right / 100, image.bottom - imageHeight * bottom / 100};
        }

        void paint(HDC dc, RECT image) const {
            if (!visible || IsRectEmpty(&image)) {
                return;
            }
            struct Runtime {
                ULONG_PTR token = 0;
                Runtime() {
                    Gdiplus::GdiplusStartupInput input;
                    Gdiplus::GdiplusStartup(&token, &input, nullptr);
                }
                ~Runtime() {
                    if (token) {
                        Gdiplus::GdiplusShutdown(token);
                    }
                }
            };
            static Runtime runtime;
            if (!runtime.token) {
                return;
            }
            const auto safe = safeRect(image);
            const RECT bands[] = {
                {image.left, image.top, image.right, safe.top},
                {image.left, safe.bottom, image.right, image.bottom},
                {image.left, safe.top, safe.left, safe.bottom},
                {safe.right, safe.top, image.right, safe.bottom}
            };
            {
                Gdiplus::Graphics graphics(dc);
                graphics.SetClip(Gdiplus::Rect(image.left, image.top,
                                              image.right - image.left, image.bottom - image.top));
                Gdiplus::SolidBrush tint(Gdiplus::Color(32, 55, 100, 120));
                for (const auto &band : bands) {
                    const Gdiplus::Rect rectangle(band.left, band.top,
                                                  band.right - band.left, band.bottom - band.top);
                    graphics.FillRectangle(&tint, rectangle);
                }
                Gdiplus::Pen edge(Gdiplus::Color(150, 150, 205, 215), 1);
                edge.SetDashStyle(Gdiplus::DashStyleDash);
                graphics.DrawRectangle(&edge, Gdiplus::Rect(safe.left, safe.top,
                                                            safe.right - safe.left, safe.bottom - safe.top));
            }
        }
    };
}
