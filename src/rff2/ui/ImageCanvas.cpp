//
// Created by Opus 5 on 2026-09-03.
// Modified by Opus 5 on 2026-09-04
//

#include "ImageCanvas.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <format>

#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

#include "IOUtilities.h"
#include "SettingsTheme.hpp"
#include "../constants/Constants.hpp"

namespace merutilm::rff2 {
    namespace {
        constexpr auto IMAGE_CANVAS_CLASS = L"RFF2IMC";

        // Brings a decoded file to the one form the blit wants: 8 bits a channel, four channels,
        // blue first. A 16-bit save - which is what Save Image writes - comes down to 8 here, so the
        // picture is shown as a screen can show it rather than as the file holds it.
        cv::Mat toDisplayForm(const cv::Mat &decoded) {
            if (decoded.empty()) {
                return {};
            }
            cv::Mat working = decoded;
            if (working.depth() == CV_16U) {
                working.convertTo(working, CV_8U, 1.0 / 257.0);
            } else if (working.depth() == CV_32F || working.depth() == CV_64F) {
                working.convertTo(working, CV_8U, 255.0);
            } else if (working.depth() != CV_8U) {
                working.convertTo(working, CV_8U);
            }
            switch (working.channels()) {
                case 1: cv::cvtColor(working, working, cv::COLOR_GRAY2BGRA);
                    break;
                case 3: cv::cvtColor(working, working, cv::COLOR_BGR2BGRA);
                    break;
                case 4: break;
                default: return {};
            }
            return working;
        }

        HBITMAP createDIB(const HWND window, const cv::Mat &bgra) {
            if (bgra.empty() || bgra.type() != CV_8UC4) {
                return nullptr;
            }
            BITMAPINFO info = {};
            info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            info.bmiHeader.biWidth = bgra.cols;
            // Negative height is a top-down bitmap, which is the order the rows already sit in.
            info.bmiHeader.biHeight = -bgra.rows;
            info.bmiHeader.biPlanes = 1;
            info.bmiHeader.biBitCount = 32;
            info.bmiHeader.biCompression = BI_RGB;
            void *pixels = nullptr;
            const HDC hdc = GetDC(window);
            const HBITMAP bitmap = CreateDIBSection(hdc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
            ReleaseDC(window, hdc);
            if (bitmap == nullptr || pixels == nullptr) {
                if (bitmap != nullptr) {
                    DeleteObject(bitmap);
                }
                return nullptr;
            }
            const size_t rowBytes = static_cast<size_t>(bgra.cols) * 4;
            for (int y = 0; y < bgra.rows; ++y) {
                std::memcpy(static_cast<unsigned char *>(pixels) + static_cast<size_t>(y) * rowBytes,
                            bgra.ptr(y), rowBytes);
            }
            return bitmap;
        }

        void registerImageCanvasClass() {
            static const bool registered = [] {
                WNDCLASSEXW wc = {};
                wc.cbSize = sizeof(wc);
                wc.hInstance = GetModuleHandleW(nullptr);
                wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
                wc.lpfnWndProc = ImageCanvas::windowProc;
                wc.lpszClassName = IMAGE_CANVAS_CLASS;
                return RegisterClassExW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
            }();
            (void) registered;
        }
    }

    ImageCanvas::~ImageCanvas() {
        releaseScaled();
        if (window != nullptr) {
            DestroyWindow(window);
            window = nullptr;
        }
        if (messageFont != nullptr) {
            DeleteObject(messageFont);
        }
    }

    void ImageCanvas::setDismissCallback(std::function<void()> callback) {
        dismiss = std::move(callback);
    }

    bool ImageCanvas::visible() const {
        return window != nullptr && IsWindowVisible(window);
    }

    void ImageCanvas::releaseScaled() {
        if (scaled != nullptr) {
            DeleteObject(scaled);
            scaled = nullptr;
        }
        scaledSize = {0, 0};
    }

    bool ImageCanvas::show(const HWND parent, const RECT &area, const std::filesystem::path &image) {
        registerImageCanvasClass();
        if (window == nullptr) {
            // Created after the canvas, so it stands above it among the main window's children and
            // the canvas clips what it presents to the part this does not cover.
            window = CreateWindowExW(0, IMAGE_CANVAS_CLASS, L"", WS_CHILD | WS_CLIPSIBLINGS,
                                     area.left, area.top, area.right - area.left, area.bottom - area.top,
                                     parent, nullptr, GetModuleHandleW(nullptr), this);
            if (window == nullptr) {
                return false;
            }
            messageFont = CreateFontW(Constants::Win32::settingsScaled(26), 0, 0, 0, FW_NORMAL, FALSE, FALSE,
                                      FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                      CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS,
                                      Constants::Win32::uiFontFace());
        }
        source = toDisplayForm(IOUtilities::readImage(image, cv::IMREAD_UNCHANGED));
        message = source.empty() ? std::format(L"{} could not be read.", image.filename().wstring())
                                 : std::wstring();
        releaseScaled();
        layout(area);
        ShowWindow(window, SW_SHOWNA);
        InvalidateRect(window, nullptr, TRUE);
        UpdateWindow(window);
        return !source.empty();
    }

    void ImageCanvas::layout(const RECT &area) const {
        if (window == nullptr) {
            return;
        }
        // The z-order is left alone: this window is created after the canvas and so already stands
        // over it, and re-inserting it at the top on every step of the folder asks the window
        // manager to work out what is in front of what while the canvas is presenting into the same
        // pixels. Only where it sits and how large it is is set here.
        SetWindowPos(window, nullptr, area.left, area.top, area.right - area.left,
                     area.bottom - area.top, SWP_NOACTIVATE | SWP_NOZORDER);
    }

    void ImageCanvas::hide() {
        if (window == nullptr) {
            return;
        }
        ShowWindow(window, SW_HIDE);
        releaseScaled();
        // The picture is dropped with the window: it is only ever as large as the canvas, but there
        // is no reason to hold it once the fractal is back.
        source = cv::Mat();
        message.clear();
        // Nothing is repainted here: the canvas below has gone on presenting the whole time, and the
        // frame it draws next is what fills the part this was covering.
    }

    void ImageCanvas::ensureScaled(const int width, const int height) {
        if (scaled != nullptr && scaledSize.cx == width && scaledSize.cy == height) {
            return;
        }
        releaseScaled();
        if (source.empty() || width <= 0 || height <= 0) {
            return;
        }
        cv::Mat resized;
        if (width == source.cols && height == source.rows) {
            resized = source;
        } else {
            // Area averaging is what keeps a shrunken fractal from breaking into aliasing; growing
            // one has no detail to average, so it takes the cheaper filter.
            cv::resize(source, resized, cv::Size(width, height), 0, 0,
                       width < source.cols ? cv::INTER_AREA : cv::INTER_LINEAR);
        }
        scaled = createDIB(window, resized);
        if (scaled != nullptr) {
            scaledSize = {width, height};
        }
    }

    void ImageCanvas::paint() {
        PAINTSTRUCT ps = {};
        const HDC hdc = BeginPaint(window, &ps);
        RECT client = {};
        GetClientRect(window, &client);
        const HBRUSH background = CreateSolidBrush(settingsTheme().background);

        if (source.empty()) {
            FillRect(hdc, &client, background);
            const HGDIOBJ previousFont = messageFont != nullptr ? SelectObject(hdc, messageFont) : nullptr;
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, settingsTheme().textDisabled);
            DrawTextW(hdc, message.c_str(), -1, &client,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            if (previousFont != nullptr) {
                SelectObject(hdc, previousFont);
            }
            DeleteObject(background);
            EndPaint(window, &ps);
            return;
        }

        // Shrunk to fit, and no further: a picture smaller than the canvas is left at its own size
        // rather than blown up into the pixels it does not have.
        const int cw = static_cast<int>(client.right - client.left);
        const int ch = static_cast<int>(client.bottom - client.top);
        const double scale = std::min({
            static_cast<double>(cw) / source.cols, static_cast<double>(ch) / source.rows, 1.0
        });
        const int width = std::max(1, static_cast<int>(std::lround(source.cols * scale)));
        const int height = std::max(1, static_cast<int>(std::lround(source.rows * scale)));
        const int x = (cw - width) / 2;
        const int y = (ch - height) / 2;

        // Only the margins are filled, so the picture itself is never painted over first and the
        // canvas does not flash while the arrow keys are held down.
        const RECT margins[4] = {
            {client.left, client.top, client.right, y},
            {client.left, y + height, client.right, client.bottom},
            {client.left, y, x, y + height},
            {x + width, y, client.right, y + height}
        };
        for (const RECT &margin: margins) {
            if (margin.right > margin.left && margin.bottom > margin.top) {
                FillRect(hdc, &margin, background);
            }
        }

        ensureScaled(width, height);
        if (scaled != nullptr) {
            const HDC memory = CreateCompatibleDC(hdc);
            const HGDIOBJ previous = SelectObject(memory, scaled);
            BitBlt(hdc, x, y, width, height, memory, 0, 0, SRCCOPY);
            SelectObject(memory, previous);
            DeleteDC(memory);
        }

        DeleteObject(background);
        EndPaint(window, &ps);
    }

    LRESULT CALLBACK ImageCanvas::windowProc(const HWND hwnd, const UINT message, const WPARAM wParam,
                                             const LPARAM lParam) {
        auto *self = reinterpret_cast<ImageCanvas *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            const auto *create = reinterpret_cast<CREATESTRUCTW *>(lParam);
            self = static_cast<ImageCanvas *>(create->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (self == nullptr) {
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }
        switch (message) {
            case WM_ERASEBKGND:
                // Painted whole below, margins and all.
                return 1;
            case WM_PAINT:
                self->paint();
                return 0;
            case WM_SIZE:
                InvalidateRect(hwnd, nullptr, TRUE);
                return 0;
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_XBUTTONDOWN:
            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL:
                // Reaching for the fractal is how the picture is left: the press that would have
                // panned or zoomed takes this away instead, and the one after it lands on the canvas
                // and works as it always does. The message itself goes no further - it is spent on
                // getting here, so the view underneath is never moved by the press that uncovered it.
                if (self->dismiss) {
                    self->dismiss();
                    return 0;
                }
                break;
            case WM_NCDESTROY:
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                self->window = nullptr;
                return DefWindowProcW(hwnd, message, wParam, lParam);
            default:
                break;
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}
