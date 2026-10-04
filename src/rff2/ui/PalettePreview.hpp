//
// Modified by GPT-6 on 2026-10-01
//

#pragma once
#include <windows.h>
#include <algorithm>
#include <cmath>
#include "../attr/ShdPaletteAttribute.h"

namespace merutilm::rff2 {
    // Approximate display color of a frozen iteration value. Computed in double precision so a
    // large iteration value keeps the correct cycle phase (float getMidColor would lose it).
    inline COLORREF freezeSwatchColor(const ShdPaletteAttribute &pal, const double iter) {
        const int n = static_cast<int>(pal.colors.size());
        if (n == 0) {
            return RGB(0, 0, 0);
        }
        auto chan = [&](const int ch, const float interval) -> float {
            double ratio = std::fmod(iter / static_cast<double>(interval) + pal.offsetRatio, 1.0);
            if (ratio < 0.0) {
                ratio += 1.0;
            }
            const double f = ratio * static_cast<double>(n);
            const int i0 = static_cast<int>(f) % n;
            const int i1 = (i0 + 1) % n;
            const float d = static_cast<float>(f - std::floor(f));
            return std::lerp(pal.colors[i0][ch], pal.colors[i1][ch], d);
        };
        return RGB(
            static_cast<BYTE>(std::round(std::clamp(chan(0, pal.iterationInterval.r), 0.0f, 1.0f) * 255.0f)),
            static_cast<BYTE>(std::round(std::clamp(chan(1, pal.iterationInterval.g), 0.0f, 1.0f) * 255.0f)),
            static_cast<BYTE>(std::round(std::clamp(chan(2, pal.iterationInterval.b), 0.0f, 1.0f) * 255.0f)));
    }


    // Palette interpolation retains the implementation and color-space provenance recorded in NOTICE.
    inline void drawPalettePreview(const HDC hdc, const RECT &rc, const ShdPaletteAttribute &pal,
                                   const ShdPalColorSmoothingMethod method,
                                   const ShdPalColorInterpolationMethod interp) {
        const int w = rc.right - rc.left;
        const int h = rc.bottom - rc.top;
        if (w <= 0 || h <= 0) {
            return;
        }
        const int n = static_cast<int>(pal.colors.size());
        if (n == 0) {
            FillRect(hdc, &rc, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
            return;
        }
        // Stock DC_BRUSH + SetDCBrushColor lets us fill one 1px column per color without
        // allocating a brush per pixel.
        const auto oldBrush = SelectObject(hdc, GetStockObject(DC_BRUSH));
        for (int x = 0; x < w; ++x) {
            const float t = w == 1 ? 0.0f : static_cast<float>(x) / static_cast<float>(w - 1); // 0..1
            glm::vec4 c;
            switch (method) {
            case ShdPalColorSmoothingMethod::NONE: {
                const int bands = std::max(1, std::min(n, w / 10));
                const int band = std::min(static_cast<int>(t * static_cast<float>(bands)), bands - 1);
                const auto i = std::min(static_cast<uint64_t>(band) * n / bands, static_cast<uint64_t>(n - 1));
                c = pal.colors[i];
                break;
            }
            case ShdPalColorSmoothingMethod::REVERSED: {
                const int bands = std::max(1, std::min(n, w / 10));
                const float scaled = t * static_cast<float>(bands);
                const int band = std::min(static_cast<int>(scaled), bands - 1);
                const float local = scaled - static_cast<float>(band); // 0..1 within the band
                const float p = (static_cast<float>(band) + (1.0f - local)) / static_cast<float>(bands);
                const float f = p * static_cast<float>(n);
                const int i0 = static_cast<int>(f) % n;
                const int i1 = (i0 + 1) % n;
                const float d = f - std::floor(f);
                c = blendPaletteColors(pal.colors[i0], pal.colors[i1], d, interp);
                break;
            }
            case ShdPalColorSmoothingMethod::NORMAL: {
                const float f = t * static_cast<float>(n);
                const int i0 = static_cast<int>(f) % n;
                const int i1 = (i0 + 1) % n;
                const float d = f - std::floor(f);
                c = blendPaletteColors(pal.colors[i0], pal.colors[i1], d, interp);
                break;
            }
            }
            // Drawn over the blend, as the palette upload lays it over the colors it sends.
            if (const float lineCoverage = pal.bandLineCoverage(t); lineCoverage > 0.0f) {
                c = glm::mix(c, pal.bandLineColor, lineCoverage);
            }
            const COLORREF rgb = RGB(static_cast<BYTE>(std::round(std::clamp(c.r, 0.0f, 1.0f) * 255.0f)),
                                     static_cast<BYTE>(std::round(std::clamp(c.g, 0.0f, 1.0f) * 255.0f)),
                                     static_cast<BYTE>(std::round(std::clamp(c.b, 0.0f, 1.0f) * 255.0f)));
            SetDCBrushColor(hdc, rgb);
            RECT col = {rc.left + x, rc.top, rc.left + x + 1, rc.bottom};
            FillRect(hdc, &col, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        }
        SelectObject(hdc, oldBrush);
    }

    inline std::vector<uint32_t> palettePreviewPixels(const ShdPaletteAttribute &palette, int width,
        ShdPalColorSmoothingMethod smoothing, ShdPalColorInterpolationMethod interpolation) {
        if (width <= 0) return {};
        HDC dc = CreateCompatibleDC(nullptr);
        if (!dc) return {};
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = width;
        info.bmiHeader.biHeight = -1;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        void *pixels = nullptr;
        HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
        if (!bitmap) { DeleteDC(dc); return {}; }
        const auto previous = SelectObject(dc, bitmap);
        drawPalettePreview(dc, {0, 0, width, 1}, palette, smoothing, interpolation);
        GdiFlush();
        std::vector<uint32_t> result(width);
        const auto *data = static_cast<const uint32_t *>(pixels);
        for (int x = 0; x < width; ++x) result[x] = RGB((data[x] >> 16) & 255, (data[x] >> 8) & 255, data[x] & 255);
        SelectObject(dc, previous);
        DeleteObject(bitmap);
        DeleteDC(dc);
        return result;
    }
}
