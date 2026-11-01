//
// Modified by GPT-6 on 2026-09-18, 2026-09-22
// Modified by Opus 5.5 on 2026-10-13
//

#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include "PipelineTimingHistory.hpp"
#include "SettingsTheme.hpp"
#include "UiDpi.hpp"
#include "UiLanguage.hpp"
#include "../../vulkan_helper/impl/PipelinePreparation.hpp"

namespace merutilm::rff2 {
    class PipelinePreparationWindow {
        HWND window = nullptr;
        HWND owner = nullptr;
        bool restoreOwner = false;
        UINT dpi = 96;
        HFONT font = nullptr;
        HFONT headingFont = nullptr;
        HBRUSH background = nullptr;
        SettingsThemeColors theme = settingsTheme();
        std::array<HWND, 5> labels{};
        double elapsed = 0;
        double expected = 0;
        bool animation = true;
        int lastSecond = -1;
        // Offered while what is left can finish behind a working window; see PipelinePreparation::Job::deferrable.
        bool skippable = false;
        inline static std::unique_ptr<PipelinePreparationWindow> active;

        static std::filesystem::path historyPath() {
            std::array<wchar_t, 32768> buffer{};
            GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
            return std::filesystem::path(buffer.data()).parent_path() / L"pipeline-timings.txt";
        }
        static PipelineTimingHistory &history() {
            static PipelineTimingHistory value(historyPath());
            return value;
        }
        int px(int value) const {
            return MulDiv(value, dpi, 96);
        }
        static std::wstring clock(double seconds) {
            const int whole = std::max(0, static_cast<int>(seconds));
            const int remainder = whole % 60;
            return std::to_wstring(whole / 60) + L":" + (remainder < 10 ? L"0" : L"") +
                   std::to_wstring(remainder);
        }
        RECT skipButton() const {
            RECT client;
            GetClientRect(window, &client);
            return {client.right - px(168), px(264), client.right - px(28), px(300)};
        }
        static std::wstring footer(bool skippable) {
            return UiLanguage::text(skippable ? L"Skip to start now with simplified shading. Full shading is "
                                                L"applied automatically when preparation finishes."
                                              : L"This window closes automatically when preparation finishes.");
        }
        void setSkippable(bool value) {
            if (skippable == value || !window) {
                return;
            }
            skippable = value;
            SetWindowTextW(labels[4], footer(skippable).c_str());
            InvalidateRect(window, nullptr, FALSE);
        }
        static std::wstring stage(const std::string &shader) {
            if (shader.find("slope") != std::string::npos) {
                return UiLanguage::text(L"Surface, materials and effects");
            }
            if (shader.find("iteration") != std::string::npos ||
                shader.find("map_iter") != std::string::npos) {
                return UiLanguage::text(L"Palette and animation");
            }
            if (shader.find("bloom") != std::string::npos || shader.find("blur") != std::string::npos) {
                return UiLanguage::text(L"Bloom and blur");
            }
            return UiLanguage::text(L"Image processing");
        }
        static LRESULT CALLBACK procedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
            auto *self =
                reinterpret_cast<PipelinePreparationWindow *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (message == WM_NCCREATE) {
                self = static_cast<PipelinePreparationWindow *>(
                    reinterpret_cast<CREATESTRUCTW *>(lParam)->lpCreateParams);
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            if (!self) {
                return DefWindowProcW(hwnd, message, wParam, lParam);
            }
            if (message == WM_CLOSE) {
                return 0;
            }
            if (message == WM_MOUSEACTIVATE) {
                return MA_NOACTIVATE;
            }
            if (message == WM_SETCURSOR && self->skippable) {
                POINT cursor;
                GetCursorPos(&cursor);
                ScreenToClient(hwnd, &cursor);
                const RECT button = self->skipButton();
                if (PtInRect(&button, cursor)) {
                    SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32649)));
                    return TRUE;
                }
            }
            if (message == WM_LBUTTONUP && self->skippable) {
                const POINT point{static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam))};
                const RECT button = self->skipButton();
                if (PtInRect(&button, point)) {
                    // The batch hands the deferrable pipelines off on its next wait and closes this window.
                    vkh::PipelinePreparation::skipRequested = true;
                    self->setSkippable(false);
                }
                return 0;
            }
            if (message == WM_ERASEBKGND) {
                return 1;
            }
            if (message == WM_CTLCOLORSTATIC) {
                SetTextColor(reinterpret_cast<HDC>(wParam), self->theme.text);
                SetBkColor(reinterpret_cast<HDC>(wParam), self->theme.background);
                return reinterpret_cast<LRESULT>(self->background);
            }
            if (message == WM_PAINT || message == WM_PRINTCLIENT) {
                PAINTSTRUCT paint{};
                HDC dc = message == WM_PAINT ? BeginPaint(hwnd, &paint) : reinterpret_cast<HDC>(wParam);
                RECT client;
                GetClientRect(hwnd, &client);
                FillRect(dc, &client, self->background);
                RECT bar{self->px(28), self->px(107), client.right - self->px(28), self->px(113)};
                HBRUSH track = CreateSolidBrush(self->theme.sliderTrack);
                FillRect(dc, &bar, track);
                DeleteObject(track);
                RECT fill = bar;
                if (self->expected > 0 && self->elapsed < self->expected) {
                    fill.right = fill.left + static_cast<int>((bar.right - bar.left) *
                                                              std::min(0.95, self->elapsed / self->expected));
                } else {
                    const int width = (bar.right - bar.left) / 5;
                    const double phase = self->animation ? std::fmod(self->elapsed * 0.35, 1.0) : 0.5;
                    fill.left += static_cast<int>((bar.right - bar.left - width) * phase);
                    fill.right = fill.left + width;
                }
                HBRUSH accent = CreateSolidBrush(self->theme.cardNoteAccent);
                FillRect(dc, &fill, accent);
                DeleteObject(accent);
                if (self->skippable) {
                    RECT button = self->skipButton();
                    HBRUSH face = CreateSolidBrush(self->theme.buttonFace);
                    FillRect(dc, &button, face);
                    DeleteObject(face);
                    HBRUSH border = CreateSolidBrush(self->theme.buttonBorder);
                    FrameRect(dc, &button, border);
                    DeleteObject(border);
                    const HGDIOBJ previousFont = SelectObject(dc, self->font);
                    SetBkMode(dc, TRANSPARENT);
                    SetTextColor(dc, self->theme.text);
                    DrawTextW(dc, UiLanguage::label(L"Skip"), -1, &button, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    SelectObject(dc, previousFont);
                }
                if (message == WM_PAINT) {
                    EndPaint(hwnd, &paint);
                }
                return 0;
            }
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }

        void releaseResources() noexcept {
            if (vkh::PipelinePreparation::interactiveWindow == window) {
                vkh::PipelinePreparation::interactiveWindow = nullptr;
            }
            if (window) {
                DestroyWindow(window);
            }
            if (restoreOwner && IsWindow(owner)) {
                EnableWindow(owner, TRUE);
            }
            if (font) {
                DeleteObject(font);
            }
            if (headingFont) {
                DeleteObject(headingFont);
            }
            if (background) {
                DeleteObject(background);
            }
        }

      public:
        PipelinePreparationWindow(const vkh::PipelinePreparation::Progress &progress, double estimate)
            : expected(estimate) {
            try {
                owner = GetAncestor(progress.window, GA_ROOT);
                for (HWND parent = progress.window; parent; parent = GetParent(parent)) {
                    wchar_t className[64]{};
                    GetClassNameW(parent, className, 64);
                    if (std::wstring_view(className) == L"RFF2TLW") {
                        theme = settingsTheme(!timelineLightMode());
                        break;
                    }
                }
                dpi = UiDpi::forWindow(owner);
                BOOL animate = TRUE;
                SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animate, 0);
                animation = animate != FALSE;
                background = CreateSolidBrush(theme.background);
                font = CreateFontW(-px(15), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH,
                                   UiLanguage::fontFace());
                headingFont = CreateFontW(-px(20), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                          DEFAULT_PITCH, UiLanguage::fontFace());
                WNDCLASSW cls{};
                cls.lpfnWndProc = procedure;
                cls.hInstance = GetModuleHandleW(nullptr);
                cls.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32514));
                cls.lpszClassName = L"RFFPipelinePreparation";
                RegisterClassW(&cls);
                MONITORINFO monitor{sizeof(monitor)};
                GetMonitorInfoW(MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST), &monitor);
                RECT anchor = monitor.rcWork;
                if (IsWindowVisible(owner) && !IsIconic(owner)) {
                    GetWindowRect(owner, &anchor);
                }
                skippable = progress.skippable;
                const int width = px(540), height = px(skippable ? 320 : 265);
                const int x = std::clamp((anchor.left + anchor.right - width) / 2, monitor.rcWork.left,
                                         std::max(monitor.rcWork.left, monitor.rcWork.right - width));
                const int y = std::clamp((anchor.top + anchor.bottom - height) / 2, monitor.rcWork.top,
                                         std::max(monitor.rcWork.top, monitor.rcWork.bottom - height));
                window = CreateWindowExW(WS_EX_TOOLWINDOW, cls.lpszClassName,
                                         UiLanguage::label(L"Preparing GPU shaders"),
                                         WS_POPUP | WS_BORDER | WS_CLIPCHILDREN, x, y, width, height,
                                         IsWindowVisible(owner) ? owner : nullptr, nullptr, cls.hInstance, this);
                if (!window) {
                    return;
                }
                const std::array<int, 5> tops{24, 65, 134, 165, 216};
                for (size_t i = 0; i < labels.size(); ++i) {
                    labels[i] = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT, px(28),
                                              px(tops[i]), width - px(60),
                                              px(i == 0   ? 32
                                                 : i == 4 ? 38
                                                          : 28),
                                              window, nullptr, cls.hInstance, nullptr);
                    SendMessageW(labels[i], WM_SETFONT, reinterpret_cast<WPARAM>(i == 0 ? headingFont : font),
                                 FALSE);
                }
                SetWindowTextW(labels[0], UiLanguage::label(L"Preparing GPU shaders"));
                const auto description = UiLanguage::text(L"Current pipeline: ") + stage(progress.shader);
                SetWindowTextW(labels[1], description.c_str());
                SetWindowTextW(labels[4], footer(skippable).c_str());
                vkh::PipelinePreparation::interactiveWindow = window;
                restoreOwner = IsWindowEnabled(owner) != FALSE;
                if (restoreOwner) {
                    EnableWindow(owner, FALSE);
                }
                update(progress.seconds);
                ShowWindow(window, SW_SHOWNOACTIVATE);
                RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
            } catch (...) {
                releaseResources();
                throw;
            }
        }
        ~PipelinePreparationWindow() {
            releaseResources();
        }
        void update(double seconds) {
            elapsed = seconds;
            if (!window) {
                return;
            }
            const int whole = static_cast<int>(seconds);
            if (whole != lastSecond) {
                lastSecond = whole;
                const auto elapsedText = UiLanguage::text(L"Elapsed: ") + clock(seconds);
                SetWindowTextW(labels[2], elapsedText.c_str());
                std::wstring remaining;
                if (expected <= 0) {
                    remaining =
                        UiLanguage::text(L"Time remaining: measuring this pipeline for the first time");
                } else if (seconds >= expected) {
                    remaining =
                        UiLanguage::text(L"Taking longer than estimated. Preparation is still running.");
                } else {
                    remaining = UiLanguage::text(L"Estimated time remaining: ") +
                                clock(std::ceil(expected - seconds)) +
                                UiLanguage::text(L" (based on previous runs)");
                }
                SetWindowTextW(labels[3], remaining.c_str());
            }
            RECT bar{px(28), px(107), px(512), px(113)};
            InvalidateRect(window, &bar, FALSE);
            RedrawWindow(window, nullptr, nullptr, RDW_UPDATENOW | RDW_ALLCHILDREN);
        }
        static void observe(const vkh::PipelinePreparation::Progress &progress) {
            const std::string key = progress.device + "|" + progress.shader;
            if (progress.skipped) {
                active.reset();
                return;
            }
            // A compile a skip left running ends with no window up for it, and maybe another one's showing.
            if (progress.background) {
                if (progress.finished && progress.succeeded) {
                    history().record(key, progress.seconds);
                }
                return;
            }
            if (progress.finished) {
                active.reset();
                if (progress.succeeded) {
                    history().record(key, progress.seconds);
                }
                return;
            }
            if (!active) {
                active = std::make_unique<PipelinePreparationWindow>(progress, history().estimate(key));
            } else {
                active->setSkippable(progress.skippable);
                active->update(progress.seconds);
            }
        }
        static void install() {
            vkh::PipelinePreparation::observer = observe;
        }
    };
} // namespace merutilm::rff2
