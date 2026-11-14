//
// Created by Opus 5.5 on 2026-11-07
//

#pragma once
#include <algorithm>
#include <cstring>
#include <windows.h>
#include <opencv2/core/mat.hpp>

namespace merutilm::rff2 {
    // Turns an 8-bit BGR thumbnail into a GDI bitmap and draws one fitted into a box.
    struct ThumbnailBitmap {
        ThumbnailBitmap() = delete;

        // Null for an empty or unexpected picture; the caller deletes the bitmap.
        static HBITMAP create(const cv::Mat &bgr) {
            if (bgr.empty() || bgr.type() != CV_8UC3) {
                return nullptr;
            }
            BITMAPINFO info{};
            info.bmiHeader.biSize = sizeof(info.bmiHeader);
            info.bmiHeader.biWidth = bgr.cols;
            info.bmiHeader.biHeight = -bgr.rows;
            info.bmiHeader.biPlanes = 1;
            info.bmiHeader.biBitCount = 32;
            info.bmiHeader.biCompression = BI_RGB;
            void *bits = nullptr;
            const HBITMAP bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
            if (bitmap == nullptr || bits == nullptr) {
                return bitmap;
            }
            auto *out = static_cast<unsigned char *>(bits);
            for (int y = 0; y < bgr.rows; ++y) {
                const unsigned char *row = bgr.ptr<unsigned char>(y);
                for (int x = 0; x < bgr.cols; ++x) {
                    out[0] = row[x * 3];
                    out[1] = row[x * 3 + 1];
                    out[2] = row[x * 3 + 2];
                    out[3] = 255;
                    out += 4;
                }
            }
            return bitmap;
        }

        // The largest rectangle of the bitmap's shape that fits box, centered in it.
        static RECT fit(const RECT &box, const int width, const int height) {
            const int boxWidth = std::max<int>(1, box.right - box.left);
            const int boxHeight = std::max<int>(1, box.bottom - box.top);
            if (width <= 0 || height <= 0) {
                return box;
            }
            int w = boxWidth;
            int h = MulDiv(height, boxWidth, width);
            if (h > boxHeight) {
                h = boxHeight;
                w = MulDiv(width, boxHeight, height);
            }
            const int left = box.left + (boxWidth - w) / 2;
            const int top = box.top + (boxHeight - h) / 2;
            return {left, top, left + w, top + h};
        }

        static void draw(const HDC target, const HBITMAP bitmap, const RECT &box) {
            BITMAP info{};
            if (bitmap == nullptr || GetObjectW(bitmap, sizeof(info), &info) == 0) {
                return;
            }
            const RECT placed = fit(box, info.bmWidth, info.bmHeight);
            const HDC source = CreateCompatibleDC(target);
            const auto previous = SelectObject(source, bitmap);
            SetStretchBltMode(target, HALFTONE);
            SetBrushOrgEx(target, 0, 0, nullptr);
            StretchBlt(target, placed.left, placed.top, placed.right - placed.left, placed.bottom - placed.top, source,
                       0, 0, info.bmWidth, info.bmHeight, SRCCOPY);
            SelectObject(source, previous);
            DeleteDC(source);
        }
    };
}
