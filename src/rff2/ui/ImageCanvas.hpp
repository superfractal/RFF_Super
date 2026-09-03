//
// Created by Opus 5 on 2026-09-03.
//

#pragma once

#include <filesystem>
#include <string>
#include <windows.h>
#include <opencv2/core/mat.hpp>

namespace merutilm::rff2 {
    // The picture Load Image puts on the canvas. It is a child of the main window sitting exactly over
    // the Vulkan canvas, which is why the picture is inside the main window rather than beside it in
    // one of its own: the canvas carries WS_CLIPSIBLINGS, so what it presents stops at the edge of
    // this. The fractal is left untouched underneath and comes back the moment this is taken away.
    class ImageCanvas final {
        HWND window = nullptr;
        // The picture being shown, 8-bit BGRA. Empty when the file could not be read, which is drawn
        // as a line of text saying so rather than as a blank.
        cv::Mat source;
        // The picture resized to the rectangle it is drawn in, rebuilt only when that changes.
        HBITMAP scaled = nullptr;
        SIZE scaledSize = {0, 0};
        HFONT messageFont = nullptr;
        // What is said in place of a picture that could not be read.
        std::wstring message;

        void releaseScaled();

        void ensureScaled(int width, int height);

        void paint();

    public:
        // Public only because the window class this is registered on is set up outside the class.
        static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

        ImageCanvas() = default;

        ~ImageCanvas();

        ImageCanvas(const ImageCanvas &) = delete;

        ImageCanvas &operator=(const ImageCanvas &) = delete;

        ImageCanvas(ImageCanvas &&) = delete;

        ImageCanvas &operator=(ImageCanvas &&) = delete;

        // Reads the file and puts it over the canvas, creating the child window the first time.
        // False when the file is not a picture this build can decode, which still shows the window
        // carrying the reason rather than leaving the fractal up under a status line about a picture.
        bool show(HWND parent, const RECT &area, const std::filesystem::path &image);

        // Follows the canvas when the window is resized.
        void layout(const RECT &area) const;

        // Takes the picture away, which is what puts the fractal back on screen.
        void hide();

        [[nodiscard]] bool visible() const;
    };
}
