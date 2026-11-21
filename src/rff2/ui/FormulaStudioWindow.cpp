//
// Created by Gemini 4 Neon on 2026-11-19
// Modified by Gemini 4 Neon on 2026-11-20, 2026-11-21
//

#include "FormulaStudioWindow.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <complex>
#include <condition_variable>
#include <cstdint>
#include <cwctype>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>
#include <commctrl.h>
#include <windowsx.h>

#include "SettingsTheme.hpp"
#include "UiDpi.hpp"
#include "UiLanguage.hpp"
#include "../formula/ExpressionParser.h"
#include "workspace/FormulaInput.hpp"
#include "workspace/PanelBackBuffer.hpp"
#include "workspace/PanelDrawing.hpp"

namespace merutilm::rff2 {
    namespace {
        constexpr auto STUDIO_WINDOW_CLASS = L"RFF2FormulaStudio";
        constexpr UINT WM_STUDIO_PREVIEW_READY = WM_APP + 0x41;
        constexpr UINT_PTR PREVIEW_DEBOUNCE_TIMER = 1;
        constexpr UINT PREVIEW_DEBOUNCE_MS = 45;
        constexpr int EDIT_CONTROL_ID = 1001;

        // Logical window dimensions on a 4px grid: 712px width, 260x160 preview, 48px horizontal and 36px vertical gaps.
        constexpr int STUDIO_WIDTH = 712;
        constexpr int STUDIO_HEIGHT = 536;
        constexpr int PREVIEW_TEX_W = 260;
        constexpr int PREVIEW_TEX_H = 160;
        constexpr int THUMB_TEX_W = 40;
        constexpr int THUMB_TEX_H = 28;
        constexpr size_t MAX_UNDO_HISTORY = 128;

        struct StudioPalette {
            COLORREF bg;
            COLORREF surface;
            COLORREF footerBg;
            COLORREF field;
            COLORREF keyFace;
            COLORREF keyHover;
            COLORREF keyPress;
            COLORREF keyOp;
            COLORREF keyOpHover;
            COLORREF keyVar;
            COLORREF keyVarHover;
            COLORREF hairline;
            COLORREF hairlineStrong;
            COLORREF text1;
            COLORREF text2;
            COLORREF text3;
            COLORREF accent;
            COLORREF accentHover;
            COLORREF accentPress;
            COLORREF accentInk;
            COLORREF accentText;
            COLORREF ok;
            COLORREF okSoft;
            COLORREF err;
            COLORREF errSoft;
        };

        StudioPalette currentPalette() {
            if (highContrastSettingsMode()) {
                const auto &s = settingsTheme();
                return {
                    .bg = s.background,
                    .surface = s.background,
                    .footerBg = s.background,
                    .field = s.textFieldBackground,
                    .keyFace = s.buttonFace,
                    .keyHover = s.radioSelectedBackground,
                    .keyPress = s.radioSelectedBackground,
                    .keyOp = s.buttonFace,
                    .keyOpHover = s.radioSelectedBackground,
                    .keyVar = s.buttonFace,
                    .keyVarHover = s.radioSelectedBackground,
                    .hairline = s.buttonBorder,
                    .hairlineStrong = s.text,
                    .text1 = s.text,
                    .text2 = s.text,
                    .text3 = s.textDisabled,
                    .accent = s.primaryButton,
                    .accentHover = s.primaryButton,
                    .accentPress = s.primaryButtonPressed,
                    .accentInk = s.cardNoteAccent,
                    .accentText = s.primaryButtonText,
                    .ok = s.text,
                    .okSoft = s.background,
                    .err = s.textError,
                    .errSoft = s.background,
                };
            }
            if (darkSettingsMode()) {
                return {
                    .bg = RGB(21, 26, 32),
                    .surface = RGB(27, 33, 40),
                    .footerBg = RGB(18, 22, 28),
                    .field = RGB(32, 38, 45),
                    .keyFace = RGB(35, 42, 50),
                    .keyHover = RGB(43, 51, 61),
                    .keyPress = RGB(28, 34, 41),
                    .keyOp = RGB(27, 32, 39),
                    .keyOpHover = RGB(36, 43, 52),
                    .keyVar = RGB(41, 49, 59),
                    .keyVarHover = RGB(50, 60, 72),
                    .hairline = RGB(42, 49, 58),
                    .hairlineStrong = RGB(58, 67, 78),
                    .text1 = RGB(228, 231, 235),
                    .text2 = RGB(163, 166, 170),
                    .text3 = RGB(124, 130, 138),
                    .accent = RGB(42, 86, 164),
                    .accentHover = RGB(51, 100, 186),
                    .accentPress = RGB(35, 71, 131),
                    .accentInk = RGB(111, 152, 211),
                    .accentText = RGB(238, 243, 250),
                    .ok = RGB(76, 195, 138),
                    .okSoft = RGB(25, 43, 39),
                    .err = RGB(205, 93, 102),
                    .errSoft = RGB(52, 30, 35),
                };
            }
            return {
                .bg = RGB(248, 250, 252),
                .surface = RGB(241, 245, 249),
                .footerBg = RGB(241, 245, 249),
                .field = RGB(255, 255, 255),
                .keyFace = RGB(255, 255, 255),
                .keyHover = RGB(241, 245, 249),
                .keyPress = RGB(226, 232, 240),
                .keyOp = RGB(241, 245, 249),
                .keyOpHover = RGB(230, 236, 243),
                .keyVar = RGB(230, 236, 242),
                .keyVarHover = RGB(219, 227, 236),
                .hairline = RGB(226, 232, 240),
                .hairlineStrong = RGB(203, 213, 225),
                .text1 = RGB(30, 41, 59),
                .text2 = RGB(71, 85, 105),
                .text3 = RGB(100, 116, 139),
                .accent = RGB(37, 99, 235),
                .accentHover = RGB(29, 78, 216),
                .accentPress = RGB(30, 64, 175),
                .accentInk = RGB(29, 78, 216),
                .accentText = RGB(255, 255, 255),
                .ok = RGB(22, 163, 74),
                .okSoft = RGB(232, 248, 239),
                .err = RGB(180, 40, 40),
                .errSoft = RGB(252, 235, 235),
            };
        }

        // Renders a compact complex escape-time preview into BGRA pixels using ExpressionParser.
        bool renderPreviewPixels(const std::string &formula, int width, int height, int maxIter,
                                 std::vector<uint32_t> &outBgra, const std::atomic<uint64_t> *cancelGen = nullptr,
                                 uint64_t myGen = 0) {
            ExpressionParser parser;
            if (!parser.parse(formula)) {
                return false;
            }
            outBgra.assign(static_cast<size_t>(width) * static_cast<size_t>(height), 0xFF0C1016u);
            const double aspect = static_cast<double>(width) / static_cast<double>(height);
            constexpr double scaleY = 3.6;
            const double scaleX = scaleY * aspect;
            constexpr double cx = -0.30;
            constexpr double cy = 0.0;
            constexpr double bailout = 16.0;
            constexpr double bailout2 = bailout * bailout;
            const double logBailout = std::log(bailout);
            const double invW = width > 1 ? 1.0 / static_cast<double>(width - 1) : 0.0;
            const double invH = height > 1 ? 1.0 / static_cast<double>(height - 1) : 0.0;

            for (int py = 0; py < height; ++py) {
                if (cancelGen && cancelGen->load(std::memory_order_relaxed) != myGen) {
                    return false;
                }
                // Match RFF_Super's main canvas orientation where +i points upward.
                const double ci = cy + (0.5 - static_cast<double>(py) * invH) * scaleY;
                uint32_t *row = outBgra.data() + static_cast<size_t>(py) * static_cast<size_t>(width);
                for (int px = 0; px < width; ++px) {
                    const double cr = cx + (static_cast<double>(px) * invW - 0.5) * scaleX;
                    std::complex<double> z{0.0, 0.0};
                    const std::complex<double> c{cr, ci};
                    int it = 0;
                    double prevM2 = 0.0;
                    double m2 = 0.0;
                    for (; it < maxIter; ++it) {
                        z = parser.evaluate(z, c);
                        const double zr = z.real();
                        const double zi = z.imag();
                        prevM2 = m2;
                        m2 = zr * zr + zi * zi;
                        if (!std::isfinite(m2) || m2 > bailout2) {
                            break;
                        }
                    }
                    if (it >= maxIter) {
                        row[px] = 0xFF0C1016u;
                    } else {
                        // Estimate escape degree from consecutive magnitudes so higher-order formulas stay smooth.
                        double smooth = static_cast<double>(it + 1);
                        if (std::isfinite(m2) && m2 > bailout2) {
                            const double currMag = std::sqrt(m2);
                            const double logCurr = std::log(currMag);
                            double degree = 2.0;
                            if (prevM2 > 1.0) {
                                const double d = logCurr / std::log(std::sqrt(prevM2));
                                if (std::isfinite(d) && d > 1.0 && d <= 64.0) {
                                    degree = d;
                                }
                            }
                            const double ratio = logCurr / logBailout;
                            if (ratio > 0.0) {
                                const double frac = std::log(ratio) / std::log(degree);
                                if (std::isfinite(frac)) {
                                    smooth = std::max(0.0, static_cast<double>(it + 1) - frac);
                                }
                            }
                        }
                        const double t = smooth * 0.14;
                        const uint32_t r = static_cast<uint32_t>(std::clamp(
                            static_cast<int>(std::lround(24.0 + 176.0 * (0.5 + 0.5 * std::cos(t + 3.7)))), 0, 255));
                        const uint32_t g = static_cast<uint32_t>(std::clamp(
                            static_cast<int>(std::lround(44.0 + 170.0 * (0.5 + 0.5 * std::cos(t + 4.3)))), 0, 255));
                        const uint32_t b = static_cast<uint32_t>(std::clamp(
                            static_cast<int>(std::lround(78.0 + 165.0 * (0.5 + 0.5 * std::cos(t + 5.1)))), 0, 255));
                        row[px] = 0xFF000000u | (r << 16) | (g << 8) | b;
                    }
                }
            }
            return true;
        }

        std::vector<uint32_t> dimBgra(const std::vector<uint32_t> &src) {
            std::vector<uint32_t> dst(src.size());
            for (size_t i = 0; i < src.size(); ++i) {
                const uint32_t p = src[i];
                const uint32_t r = (((p >> 16) & 0xFFu) * 44u + 10u * 56u) / 100u;
                const uint32_t g = (((p >> 8) & 0xFFu) * 44u + 13u * 56u) / 100u;
                const uint32_t b = ((p & 0xFFu) * 44u + 18u * 56u) / 100u;
                dst[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
            }
            return dst;
        }

        void blitBgra(HDC dc, const RECT &dst, const std::vector<uint32_t> &pixels, int w, int h) {
            if (pixels.size() != static_cast<size_t>(w) * static_cast<size_t>(h) || dst.right <= dst.left ||
                dst.bottom <= dst.top) {
                return;
            }
            BITMAPINFO bmi{};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = w;
            bmi.bmiHeader.biHeight = -h;
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;
            const int prevMode = SetStretchBltMode(dc, HALFTONE);
            StretchDIBits(dc, dst.left, dst.top, dst.right - dst.left, dst.bottom - dst.top, 0, 0, w, h, pixels.data(),
                          &bmi, DIB_RGB_COLORS, SRCCOPY);
            SetStretchBltMode(dc, prevMode);
        }

        bool isVariableKey(const workspace::FormKey &key) {
            return key.text == L"z" || key.text == L"c" || key.text == L"i" || key.text == L"pi" || key.text == L"e";
        }

        class FormulaStudioImpl {
            enum class ItemKind {
                UNDO,
                REDO,
                PAD_KEY,
                CATEGORY_TAB,
                FUNCTION_KEY,
                EXAMPLE_ROW,
                CLOSE_CHECKBOX,
                CLOSE_BUTTON,
                APPLY_BUTTON
            };

            struct HitItem {
                ItemKind kind;
                RECT rect{};
                int index = 0;
                const workspace::FormKey *key = nullptr;
                bool enabled = true;
            };

            HWND hwnd = nullptr;
            HWND owner = nullptr;
            HWND edit = nullptr;
            UINT dpi = 96;
            StudioPalette pal = currentPalette();
            HBRUSH editBrush = nullptr;
            HFONT uiFont = nullptr;
            HFONT uiMediumFont = nullptr;
            HFONT smallFont = nullptr;
            HFONT monoEditFont = nullptr;
            HFONT monoSubFont = nullptr;
            HFONT monoKeyFont = nullptr;
            HFONT monoSmallFont = nullptr;
            workspace::PanelBackBuffer backBuffer;

            std::shared_ptr<const workspace::FormKeypad> keypad = workspace::FormulaInput::keypad();
            std::function<std::wstring(const std::wstring &)> onApply;

            std::vector<std::wstring> history;
            size_t historyIndex = 0;
            bool suppressEditChange = false;

            int activeCategory = 0;
            bool closeOnApply = false;
            bool formulaValid = true;
            std::wstring diagnosticMessage;
            size_t errorStart = std::wstring::npos;
            size_t errorLength = 0;

            std::vector<HitItem> items;
            int hoveredItem = -1;
            int pressedItem = -1;
            int focusedItem = -1;
            bool trackingMouse = false;

            RECT shellRect{};
            RECT diagRect{};
            RECT previewCardRect{};
            RECT previewCanvasRect{};
            RECT previewMetaRect{};

            // Background preview worker state.
            std::thread worker;
            std::mutex workerMutex;
            std::condition_variable workerCv;
            bool stopWorker = false;
            bool hasPendingJob = false;
            std::string pendingFormula;
            uint64_t pendingGen = 0;
            std::atomic<uint64_t> latestGen{0};

            std::mutex resultMutex;
            std::vector<uint32_t> readyPixels;
            uint64_t readyGen = 0;
            int readyMs = 0;
            bool hasReadyResult = false;

            std::vector<uint32_t> previewPixels;
            std::vector<uint32_t> previewDimPixels;
            int previewMs = 0;
            bool hasPreview = false;

            std::vector<std::vector<uint32_t>> exampleThumbs;

            int px(int v) const { return UiDpi::pixels(v, dpi); }

            static HFONT makeFont(int pixelHeight, int weight, const wchar_t *face, DWORD pitchAndFamily) {
                LOGFONTW lf{};
                lf.lfHeight = -pixelHeight;
                lf.lfWeight = weight;
                lf.lfCharSet = DEFAULT_CHARSET;
                lf.lfQuality = CLEARTYPE_QUALITY;
                lf.lfPitchAndFamily = static_cast<BYTE>(pitchAndFamily);
                wcscpy_s(lf.lfFaceName, face);
                return CreateFontIndirectW(&lf);
            }

            void createFonts() {
                releaseFonts();
                const wchar_t *uiFace = UiLanguage::current() == Language::Japanese ? L"Yu Gothic UI" : L"Segoe UI";
                uiFont = makeFont(px(14), FW_NORMAL, uiFace, DEFAULT_PITCH | FF_SWISS);
                uiMediumFont = makeFont(px(13), FW_SEMIBOLD, uiFace, DEFAULT_PITCH | FF_SWISS);
                smallFont = makeFont(px(12), FW_NORMAL, uiFace, DEFAULT_PITCH | FF_SWISS);
                monoEditFont = makeFont(px(18), FW_NORMAL, L"Consolas", FIXED_PITCH | FF_MODERN);
                monoSubFont = makeFont(px(10), FW_NORMAL, L"Consolas", FIXED_PITCH | FF_MODERN);
                monoKeyFont = makeFont(px(14), FW_NORMAL, L"Consolas", FIXED_PITCH | FF_MODERN);
                monoSmallFont = makeFont(px(12), FW_NORMAL, L"Consolas", FIXED_PITCH | FF_MODERN);
            }

            void releaseFonts() {
                for (HFONT *f : {&uiFont, &uiMediumFont, &smallFont, &monoEditFont, &monoSubFont, &monoKeyFont,
                                 &monoSmallFont}) {
                    if (*f) {
                        DeleteObject(*f);
                        *f = nullptr;
                    }
                }
            }

            void rebuildBrush() {
                if (editBrush) {
                    DeleteObject(editBrush);
                }
                editBrush = CreateSolidBrush(pal.field);
            }

            void startWorker() {
                worker = std::thread([this] {
                    for (;;) {
                        std::string formula;
                        uint64_t gen = 0;
                        {
                            std::unique_lock lock(workerMutex);
                            workerCv.wait(lock, [this] { return stopWorker || hasPendingJob; });
                            if (stopWorker) {
                                return;
                            }
                            formula = std::move(pendingFormula);
                            gen = pendingGen;
                            hasPendingJob = false;
                        }
                        const auto t0 = std::chrono::steady_clock::now();
                        std::vector<uint32_t> pixels;
                        if (renderPreviewPixels(formula, PREVIEW_TEX_W, PREVIEW_TEX_H, 52, pixels, &latestGen, gen)) {
                            const int elapsed = std::max(
                                1, static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                                        std::chrono::steady_clock::now() - t0)
                                                        .count()));
                            {
                                std::scoped_lock lock(resultMutex);
                                readyPixels = std::move(pixels);
                                readyGen = gen;
                                readyMs = elapsed;
                                hasReadyResult = true;
                            }
                            if (IsWindow(hwnd)) {
                                PostMessageW(hwnd, WM_STUDIO_PREVIEW_READY, 0, 0);
                            }
                        }
                    }
                });
            }

            void stopWorkerThread() {
                latestGen.fetch_add(1, std::memory_order_relaxed);
                {
                    std::scoped_lock lock(workerMutex);
                    stopWorker = true;
                }
                workerCv.notify_all();
                if (worker.joinable()) {
                    worker.join();
                }
            }

            void schedulePreview(bool immediate) {
                if (!IsWindow(hwnd)) return;
                KillTimer(hwnd, PREVIEW_DEBOUNCE_TIMER);
                if (!formulaValid) {
                    // Cancel any in-flight preview render so stale results never overwrite the current state.
                    latestGen.fetch_add(1, std::memory_order_relaxed);
                    return;
                }
                if (immediate) {
                    dispatchPreviewJob();
                } else {
                    SetTimer(hwnd, PREVIEW_DEBOUNCE_TIMER, PREVIEW_DEBOUNCE_MS, nullptr);
                }
            }

            void dispatchPreviewJob() {
                const std::wstring text = currentEditText();
                std::string narrow;
                narrow.reserve(text.size());
                for (wchar_t ch : text) {
                    if (ch > 127) return;
                    narrow.push_back(static_cast<char>(ch));
                }
                const uint64_t gen = latestGen.fetch_add(1, std::memory_order_relaxed) + 1;
                {
                    std::scoped_lock lock(workerMutex);
                    pendingFormula = std::move(narrow);
                    pendingGen = gen;
                    hasPendingJob = true;
                }
                workerCv.notify_one();
            }

            void renderExampleThumbnails() {
                exampleThumbs.clear();
                exampleThumbs.resize(keypad->examples.size());
                for (size_t i = 0; i < keypad->examples.size(); ++i) {
                    const auto &w = keypad->examples[i].text;
                    std::string narrow(w.begin(), w.end());
                    renderPreviewPixels(narrow, THUMB_TEX_W, THUMB_TEX_H, 32, exampleThumbs[i]);
                }
            }

            std::wstring currentEditText() const {
                if (!IsWindow(edit)) return {};
                const int len = GetWindowTextLengthW(edit);
                std::wstring text(static_cast<size_t>(len) + 1, L'\0');
                GetWindowTextW(edit, text.data(), len + 1);
                text.resize(static_cast<size_t>(len));
                return text;
            }

            void setEditText(const std::wstring &text, DWORD caret) {
                if (!IsWindow(edit)) return;
                suppressEditChange = true;
                SetWindowTextW(edit, text.c_str());
                SendMessageW(edit, EM_SETSEL, caret, caret);
                SendMessageW(edit, EM_SCROLLCARET, 0, 0);
                suppressEditChange = false;
            }

            void pushHistory(const std::wstring &text) {
                if (!history.empty() && history[historyIndex] == text) return;
                if (!history.empty() && historyIndex + 1 < history.size()) {
                    history.resize(historyIndex + 1);
                }
                history.push_back(text);
                if (history.size() > MAX_UNDO_HISTORY) {
                    history.erase(history.begin());
                }
                historyIndex = history.size() - 1;
            }

            void runUndo() {
                if (historyIndex == 0 || history.empty()) return;
                --historyIndex;
                const auto &target = history[historyIndex];
                focusedItem = -1;
                setEditText(target, static_cast<DWORD>(target.size()));
                SetFocus(edit);
                validateFormula(false, false);
            }

            void runRedo() {
                if (history.empty() || historyIndex + 1 >= history.size()) return;
                ++historyIndex;
                const auto &target = history[historyIndex];
                focusedItem = -1;
                setEditText(target, static_cast<DWORD>(target.size()));
                SetFocus(edit);
                validateFormula(false, false);
            }

            void validateFormula(bool recordUndo, bool immediatePreview) {
                const std::wstring text = currentEditText();
                if (recordUndo) {
                    pushHistory(text);
                }
                const auto check = workspace::FormulaInput::check(text);
                formulaValid = check.message.empty();
                errorStart = check.start;
                errorLength = check.length;
                // Stay silent when valid so errors stand out immediately without steady-state clutter.
                diagnosticMessage = formulaValid ? std::wstring{} : check.message;
                syncEnabledFlags();
                schedulePreview(immediatePreview);
                InvalidateRect(hwnd, nullptr, FALSE);
            }

            void syncEnabledFlags() {
                for (size_t i = 0; i < items.size(); ++i) {
                    auto &item = items[i];
                    if (item.kind == ItemKind::UNDO) {
                        item.enabled = historyIndex > 0;
                    } else if (item.kind == ItemKind::REDO) {
                        item.enabled = !history.empty() && historyIndex + 1 < history.size();
                    } else if (item.kind == ItemKind::APPLY_BUTTON) {
                        item.enabled = formulaValid;
                    }
                    if (static_cast<int>(i) == focusedItem && !item.enabled) {
                        focusedItem = -1;
                    }
                }
            }

            void pressFormKey(const workspace::FormKey &key) {
                if (!IsWindow(edit)) return;
                DWORD start = 0, end = 0;
                SendMessageW(edit, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
                SetFocus(edit);
                std::wstring text = currentEditText();
                const DWORD len = static_cast<DWORD>(text.size());
                start = std::min(start, len);
                end = std::clamp(end, start, len);

                std::wstring next = text;
                DWORD caret = start;
                using Action = workspace::FormKey::Action;
                switch (key.action) {
                case Action::INSERT:
                    next = text.substr(0, start) + key.text + text.substr(end);
                    caret = start + static_cast<DWORD>(key.text.size());
                    break;
                case Action::WRAP: {
                    const std::wstring inside = text.substr(start, end - start);
                    const std::wstring wrapped = key.text + L"(" + inside + L")";
                    next = text.substr(0, start) + wrapped + text.substr(end);
                    caret = start + static_cast<DWORD>(inside.empty() ? key.text.size() + 1 : wrapped.size());
                    break;
                }
                case Action::REPLACE:
                    next = key.text;
                    caret = static_cast<DWORD>(next.size());
                    break;
                case Action::CLEAR:
                    next.clear();
                    caret = 0;
                    break;
                case Action::BACKSPACE:
                    if (start != end) {
                        next = text.substr(0, start) + text.substr(end);
                        caret = start;
                    } else if (start > 0) {
                        DWORD from = start - 1;
                        if (text[from] == L'(') {
                            while (from > 0 && std::iswalpha(text[from - 1])) {
                                --from;
                            }
                        }
                        next = text.substr(0, from) + text.substr(start);
                        caret = from;
                    }
                    break;
                case Action::LEFT:
                    caret = start != end ? start : (start > 0 ? start - 1 : 0);
                    SendMessageW(edit, EM_SETSEL, caret, caret);
                    SendMessageW(edit, EM_SCROLLCARET, 0, 0);
                    InvalidateRect(hwnd, &shellRect, FALSE);
                    return;
                case Action::RIGHT:
                    caret = start != end ? end : std::min(end + 1, len);
                    SendMessageW(edit, EM_SETSEL, caret, caret);
                    SendMessageW(edit, EM_SCROLLCARET, 0, 0);
                    InvalidateRect(hwnd, &shellRect, FALSE);
                    return;
                }

                setEditText(next, caret);
                validateFormula(true, key.action == Action::REPLACE);
            }

            void requestClose() const {
                if (IsWindow(hwnd)) {
                    PostMessageW(hwnd, WM_CLOSE, 0, 0);
                }
            }

            void applyFormula() {
                if (!formulaValid || !onApply) return;
                const std::wstring err = onApply(currentEditText());
                if (!err.empty()) {
                    diagnosticMessage = err;
                    InvalidateRect(hwnd, &diagRect, FALSE);
                    return;
                }
                if (closeOnApply) {
                    requestClose();
                }
            }

            void buildLayout() {
                if (pressedItem >= 0 && GetCapture() == hwnd) {
                    ReleaseCapture();
                }
                items.clear();
                hoveredItem = -1;
                pressedItem = -1;
                focusedItem = -1;

                // Top formula bar (y = 0 .. 96; 12px above divider + 24px below = 36px vertical gap).
                shellRect = {px(20), px(16), px(624), px(60)};
                diagRect = {px(20), px(68), px(692), px(84)};

                items.push_back({ItemKind::UNDO, {px(632), px(24), px(660), px(52)}, 0, nullptr, historyIndex > 0});
                items.push_back({ItemKind::REDO, {px(664), px(24), px(692), px(52)}, 0, nullptr,
                                 !history.empty() && historyIndex + 1 < history.size()});

                if (IsWindow(edit)) {
                    HDC dc = GetDC(edit);
                    HGDIOBJ prev = SelectObject(dc, monoEditFont);
                    TEXTMETRICW tm{};
                    GetTextMetricsW(dc, &tm);
                    SelectObject(dc, prev);
                    ReleaseDC(edit, dc);
                    const int editH = std::max(px(22), int(tm.tmHeight) + px(2));
                    const int editTop = shellRect.top + (shellRect.bottom - shellRect.top - editH) / 2;
                    SetWindowPos(edit, nullptr, px(86), editTop, px(526), editH, SWP_NOZORDER | SWP_NOACTIVATE);
                }

                // Left pane: 6-column borderless core pad (x = 20 .. 384, y = 120 .. 336).
                const int leftX = px(20);
                const int leftW = px(364);
                const int gap = px(4);
                const auto edge = [&](int col, int cols, int width) { return MulDiv(col, width + gap, cols); };

                const int padCols = std::max(1, keypad->padColumns);
                const int padH = px(40);
                int col = 0;
                int y = px(120);
                for (size_t i = 0; i < keypad->pad.size(); ++i) {
                    const auto &k = keypad->pad[i];
                    const int span = std::clamp(k.span, 1, padCols);
                    if (col + span > padCols) {
                        col = 0;
                        y += padH + gap;
                    }
                    RECT r{leftX + edge(col, padCols, leftW), y, leftX + edge(col + span, padCols, leftW) - gap,
                           y + padH};
                    items.push_back({ItemKind::PAD_KEY, r, static_cast<int>(i), &k, true});
                    col += span;
                }

                // Underline function category tabs (y = 352 .. 384).
                const int tabsTop = px(352);
                const int tabCount = static_cast<int>(keypad->pages.size());
                for (int i = 0; i < tabCount; ++i) {
                    RECT r{leftX + edge(i, tabCount, leftW), tabsTop, leftX + edge(i + 1, tabCount, leftW) - gap,
                           tabsTop + px(32)};
                    items.push_back({ItemKind::CATEGORY_TAB, r, i, nullptr, true});
                }

                // 5-column borderless function grid (y = 392 .. 460, 2 fixed rows).
                if (!keypad->pages.empty()) {
                    activeCategory = std::clamp(activeCategory, 0, tabCount - 1);
                    const auto &fnKeys = keypad->pages[static_cast<size_t>(activeCategory)].second;
                    const int fnCols = 5;
                    const int fnTop = px(392);
                    const int fnH = px(32);
                    for (size_t i = 0; i < fnKeys.size(); ++i) {
                        const int rIdx = static_cast<int>(i) / fnCols;
                        const int cIdx = static_cast<int>(i) % fnCols;
                        RECT r{leftX + edge(cIdx, fnCols, leftW), fnTop + rIdx * (fnH + gap),
                               leftX + edge(cIdx + 1, fnCols, leftW) - gap, fnTop + rIdx * (fnH + gap) + fnH};
                        items.push_back({ItemKind::FUNCTION_KEY, r, static_cast<int>(i), &fnKeys[i], true});
                    }
                }

                // Right studio pane (x = 408 .. 712, inner x = 432 .. 692, 48px separation from keypad).
                const int rightX = px(432);
                const int rightW = px(260);
                previewCanvasRect = {rightX, px(120), rightX + rightW, px(280)};
                previewMetaRect = {rightX, px(280), rightX + rightW, px(308)};
                previewCardRect = {rightX, px(120), rightX + rightW, px(308)};

                // Monochrome example list below preview (y = 348 .. 460, 4 rows separated by 1px hairlines).
                const int exTop = px(348);
                const int exH = px(28);
                for (size_t i = 0; i < keypad->examples.size(); ++i) {
                    const int top = exTop + static_cast<int>(i) * exH;
                    items.push_back({ItemKind::EXAMPLE_ROW, {rightX, top, rightX + rightW, top + exH},
                                     static_cast<int>(i), &keypad->examples[i], true});
                }

                // Footer controls (y = 480 .. 536).
                items.push_back({ItemKind::CLOSE_CHECKBOX, {px(20), px(496), px(276), px(520)}, 0, nullptr, true});
                items.push_back({ItemKind::CLOSE_BUTTON, {px(448), px(492), px(540), px(524)}, 0, nullptr, true});
                items.push_back({ItemKind::APPLY_BUTTON, {px(548), px(492), px(692), px(524)}, 0, nullptr, formulaValid});

                // Recompute hover immediately so clicking a category tab or changing DPI keeps hover live.
                if (IsWindow(hwnd)) {
                    POINT pt{};
                    if (GetCursorPos(&pt)) {
                        ScreenToClient(hwnd, &pt);
                        hoveredItem = hitTest(pt);
                    }
                }
            }

            // Cycles keyboard focus between the formula box (-1) and footer controls (checkbox, Close, Apply).
            void stepKeyboardFocus(int dir) {
                std::vector<int> stops{-1};
                for (size_t i = 0; i < items.size(); ++i) {
                    const auto kind = items[i].kind;
                    if (items[i].enabled &&
                        (kind == ItemKind::CLOSE_CHECKBOX || kind == ItemKind::CLOSE_BUTTON ||
                         kind == ItemKind::APPLY_BUTTON)) {
                        stops.push_back(static_cast<int>(i));
                    }
                }
                const auto it = std::find(stops.begin(), stops.end(), focusedItem);
                const int cur = it == stops.end() ? 0 : static_cast<int>(it - stops.begin());
                const int next = (cur + dir + static_cast<int>(stops.size())) % static_cast<int>(stops.size());
                focusedItem = stops[static_cast<size_t>(next)];
                if (focusedItem == -1) {
                    SetFocus(edit);
                } else {
                    SetFocus(hwnd);
                }
                InvalidateRect(hwnd, nullptr, FALSE);
            }

            int hitTest(POINT pt) const {
                for (size_t i = 0; i < items.size(); ++i) {
                    if (PtInRect(&items[i].rect, pt)) {
                        return static_cast<int>(i);
                    }
                }
                return -1;
            }

            void activateItem(int idx) {
                if (idx < 0 || idx >= static_cast<int>(items.size())) return;
                const auto item = items[static_cast<size_t>(idx)];
                if (!item.enabled) return;
                switch (item.kind) {
                case ItemKind::UNDO:
                    runUndo();
                    break;
                case ItemKind::REDO:
                    runRedo();
                    break;
                case ItemKind::PAD_KEY:
                case ItemKind::FUNCTION_KEY:
                case ItemKind::EXAMPLE_ROW:
                    if (item.key) {
                        pressFormKey(*item.key);
                    }
                    break;
                case ItemKind::CATEGORY_TAB:
                    if (activeCategory != item.index) {
                        activeCategory = item.index;
                        buildLayout();
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    break;
                case ItemKind::CLOSE_CHECKBOX:
                    closeOnApply = !closeOnApply;
                    InvalidateRect(hwnd, &item.rect, FALSE);
                    break;
                case ItemKind::CLOSE_BUTTON:
                    requestClose();
                    break;
                case ItemKind::APPLY_BUTTON:
                    applyFormula();
                    break;
                }
            }

            void drawVectorIcon(HDC dc, const RECT &r, workspace::FormKey::Action action, COLORREF color) const {
                const int cx = (r.left + r.right) / 2;
                const int cy = (r.top + r.bottom) / 2;
                const int penW = std::max(1, px(1));
                HPEN pen = CreatePen(PS_SOLID, penW, color);
                HGDIOBJ prev = SelectObject(dc, pen);
                const auto poly = [&](std::initializer_list<POINT> pts) {
                    std::vector<POINT> v(pts);
                    Polyline(dc, v.data(), static_cast<int>(v.size()));
                };
                if (action == workspace::FormKey::Action::LEFT) {
                    poly({{cx + px(2), cy - px(5)}, {cx - px(3), cy}, {cx + px(2), cy + px(5)}});
                } else if (action == workspace::FormKey::Action::RIGHT) {
                    poly({{cx - px(2), cy - px(5)}, {cx + px(3), cy}, {cx - px(2), cy + px(5)}});
                } else {
                    // 45-degree beveled tag with a 12x12 inner chamber centered at (cx + 3, cy) and a centered 6x6 cross.
                    poly({{cx - px(9), cy}, {cx - px(3), cy - px(6)}, {cx + px(9), cy - px(6)},
                          {cx + px(9), cy + px(6)}, {cx - px(3), cy + px(6)}, {cx - px(9), cy}});
                    const int xCenter = cx + px(3);
                    const int arm = px(3);
                    poly({{xCenter - arm, cy - arm}, {xCenter + arm, cy + arm}});
                    SetPixel(dc, xCenter + arm, cy + arm, color);
                    poly({{xCenter + arm, cy - arm}, {xCenter - arm, cy + arm}});
                    SetPixel(dc, xCenter - arm, cy + arm, color);
                }
                SelectObject(dc, prev);
                DeleteObject(pen);
            }

            void drawUndoRedoIcon(HDC dc, const RECT &r, bool redo, COLORREF color) const {
                const int cx = (r.left + r.right) / 2;
                const int cy = (r.top + r.bottom) / 2;
                const int penW = std::max(1, px(1));
                HPEN pen = CreatePen(PS_SOLID, penW, color);
                HGDIOBJ prev = SelectObject(dc, pen);
                const int dir = redo ? 1 : -1;
                const std::array<POINT, 3> head = {{{cx + dir * px(1), cy - px(4)},
                                                    {cx + dir * px(4), cy - px(1)},
                                                    {cx + dir * px(1), cy + px(2)}}};
                Polyline(dc, head.data(), static_cast<int>(head.size()));
                SetPixel(dc, cx + dir * px(1), cy + px(2), color);
                const std::array<POINT, 5> shaft = {{{cx + dir * px(4), cy - px(1)},
                                                     {cx - dir * px(2), cy - px(1)},
                                                     {cx - dir * px(4), cy + px(1)},
                                                     {cx - dir * px(4), cy + px(2)},
                                                     {cx - dir * px(2), cy + px(4)}}};
                Polyline(dc, shaft.data(), static_cast<int>(shaft.size()));
                SetPixel(dc, cx - dir * px(2), cy + px(4), color);
                SelectObject(dc, prev);
                DeleteObject(pen);
            }

            void drawErrorUnderline(HDC dc) const {
                if (formulaValid || errorStart == std::wstring::npos || !IsWindow(edit)) return;
                const int len = GetWindowTextLengthW(edit);
                if (len <= 0) return;
                const int start = std::min(static_cast<int>(errorStart), len - 1);
                const int end = std::min(len, start + std::max(1, static_cast<int>(errorLength)));

                RECT editClient{};
                GetClientRect(edit, &editClient);
                POINT origin{0, 0};
                ClientToScreen(edit, &origin);
                ScreenToClient(hwnd, &origin);

                const auto posX = [&](int index) -> int {
                    if (index < len) {
                        return static_cast<int>(static_cast<short>(LOWORD(SendMessageW(edit, EM_POSFROMCHAR, index, 0))));
                    }
                    const std::wstring text = currentEditText();
                    const wchar_t lastCh = text[static_cast<size_t>(len - 1)];
                    HDC m = GetDC(edit);
                    HGDIOBJ prev = SelectObject(m, monoEditFont);
                    SIZE sz{};
                    GetTextExtentPoint32W(m, &lastCh, 1, &sz);
                    SelectObject(m, prev);
                    ReleaseDC(edit, m);
                    return static_cast<int>(static_cast<short>(LOWORD(SendMessageW(edit, EM_POSFROMCHAR, len - 1, 0)))) +
                           sz.cx;
                };

                const int left = origin.x + std::max(0, posX(start));
                const int right = origin.x + std::min(static_cast<int>(editClient.right), posX(end));
                const int top = origin.y + static_cast<int>(editClient.bottom) + px(1);
                const int thickness = std::max(1, px(1));
                if (right > left) {
                    const LONG bottom = std::min<LONG>(shellRect.bottom - px(2), top + thickness);
                    workspace::PanelDrawing::fill(dc, {left, top, right, bottom}, pal.err);
                }
            }

            void paint(HDC target) {
                RECT client{};
                GetClientRect(hwnd, &client);
                const int w = client.right - client.left;
                const int h = client.bottom - client.top;
                HDC dc = backBuffer.begin(target, w, h);
                if (!dc) dc = target;

                const int topSplitY = px(96);
                const int bodySplitX = px(408);
                const int footerY = px(480);

                // Background regions.
                workspace::PanelDrawing::fill(dc, {0, 0, w, footerY}, pal.bg);
                workspace::PanelDrawing::fill(dc, {bodySplitX, topSplitY, w, footerY}, pal.surface);
                workspace::PanelDrawing::fill(dc, {0, footerY, w, h}, pal.footerBg);

                // Structural 1px hairlines.
                workspace::PanelDrawing::fill(dc, {0, topSplitY, w, topSplitY + 1}, pal.hairline);
                workspace::PanelDrawing::fill(dc, {bodySplitX, topSplitY, bodySplitX + 1, footerY}, pal.hairline);
                workspace::PanelDrawing::fill(dc, {0, footerY, w, footerY + 1}, pal.hairline);

                // Formula input shell (plain monochrome text, no syntax coloring).
                const bool editFocused = GetFocus() == edit;
                const COLORREF shellBorder = !formulaValid ? pal.err : (editFocused ? pal.accentInk : pal.hairlineStrong);
                workspace::PanelDrawing::rounded(dc, shellRect, pal.field, shellBorder, px(8));

                // Crisp mathematical prefix "z" + subscript "n+1" + "=".
                workspace::PanelDrawing::text(dc, L"z", {px(34), shellRect.top, px(46), shellRect.bottom}, pal.text3,
                                              monoEditFont, DT_LEFT);
                workspace::PanelDrawing::text(dc, L"n+1", {px(45), shellRect.top + px(11), px(70), shellRect.bottom},
                                              pal.text3, monoSubFont, DT_LEFT);
                workspace::PanelDrawing::text(dc, L"=", {px(68), shellRect.top, px(82), shellRect.bottom}, pal.text3,
                                              monoEditFont, DT_LEFT);

                // Live error underline beneath the EDIT control.
                drawErrorUnderline(dc);

                // Error-only diagnostic line (silent when valid).
                if (!diagnosticMessage.empty()) {
                    workspace::PanelDrawing::text(dc, diagnosticMessage, diagRect, pal.err, smallFont, DT_LEFT);
                }

                // Underline baseline beneath category tabs.
                workspace::PanelDrawing::fill(dc, {px(20), px(383), px(384), px(384)}, pal.hairline);

                // Right studio pane: Preview card.
                workspace::PanelDrawing::rounded(dc, previewCardRect, RGB(10, 13, 18), pal.hairlineStrong, px(8));
                RECT innerCanvas = previewCanvasRect;
                InflateRect(&innerCanvas, -1, -1);
                innerCanvas.bottom = previewCanvasRect.bottom;
                if (hasPreview) {
                    blitBgra(dc, innerCanvas, formulaValid ? previewPixels : previewDimPixels, PREVIEW_TEX_W,
                             PREVIEW_TEX_H);
                } else {
                    workspace::PanelDrawing::fill(dc, innerCanvas, RGB(10, 13, 18));
                }
                if (!formulaValid) {
                    const std::wstring overlay = UiLanguage::text(L"Complete the formula to update the preview");
                    const int pillH = px(28);
                    const int pillTop = innerCanvas.top + (innerCanvas.bottom - innerCanvas.top - pillH) / 2;
                    const RECT pill{innerCanvas.left + px(16), pillTop, innerCanvas.right - px(16), pillTop + pillH};
                    workspace::PanelDrawing::rounded(dc, pill, RGB(15, 20, 27), RGB(58, 67, 78), px(6));
                    workspace::PanelDrawing::text(dc, overlay, pill, RGB(228, 231, 235), smallFont, DT_CENTER);
                }
                RECT metaFill = previewMetaRect;
                metaFill.left += 1;
                metaFill.right -= 1;
                metaFill.bottom -= 1;
                workspace::PanelDrawing::fill(dc, metaFill, pal.bg);
                workspace::PanelDrawing::fill(dc, {metaFill.left, previewMetaRect.top, metaFill.right,
                                                   previewMetaRect.top + 1},
                                              pal.hairline);
                workspace::PanelDrawing::text(dc, UiLanguage::text(L"Full preview"),
                                              {metaFill.left + px(10), previewMetaRect.top,
                                               metaFill.right - px(64), previewMetaRect.bottom},
                                              pal.text3, smallFont, DT_LEFT);
                const std::wstring timeStr = hasPreview ? (std::to_wstring(previewMs) + L" ms") : L"--";
                workspace::PanelDrawing::text(dc, timeStr,
                                              {metaFill.right - px(64), previewMetaRect.top,
                                               metaFill.right - px(10), previewMetaRect.bottom},
                                              pal.text3, monoSmallFont, DT_RIGHT);

                // Examples section label and top hairline in the right studio pane.
                workspace::PanelDrawing::text(dc, UiLanguage::text(L"Examples"),
                                              {px(432), px(324), px(692), px(340)}, pal.text3, smallFont, DT_LEFT);
                workspace::PanelDrawing::fill(dc, {px(432), px(348), px(692), px(349)}, pal.hairline);

                // Paint every interactive item cleanly.
                for (size_t i = 0; i < items.size(); ++i) {
                    const auto &item = items[i];
                    const bool hot = static_cast<int>(i) == hoveredItem && item.enabled;
                    const bool down = static_cast<int>(i) == pressedItem && item.enabled;

                    switch (item.kind) {
                    case ItemKind::UNDO:
                    case ItemKind::REDO: {
                        const COLORREF face = down ? pal.keyPress : (hot ? pal.keyHover : pal.bg);
                        const COLORREF border = hot ? pal.hairlineStrong : face;
                        workspace::PanelDrawing::rounded(dc, item.rect, face, border, px(6));
                        const COLORREF fg = !item.enabled ? pal.text3 : (hot ? pal.text1 : pal.text2);
                        drawUndoRedoIcon(dc, item.rect, item.kind == ItemKind::REDO, fg);
                        break;
                    }
                    case ItemKind::PAD_KEY: {
                        const auto &k = *item.key;
                        const bool isVar = isVariableKey(k);
                        const bool isUtil = k.action != workspace::FormKey::Action::INSERT;
                        const bool isOp = k.symbol || isUtil;
                        COLORREF face = pal.keyFace;
                        if (down) face = pal.keyPress;
                        else if (isVar) face = hot ? pal.keyVarHover : pal.keyVar;
                        else if (isOp) face = hot ? pal.keyOpHover : pal.keyOp;
                        else face = hot ? pal.keyHover : pal.keyFace;
                        const COLORREF border = hot ? pal.hairlineStrong : face;
                        workspace::PanelDrawing::rounded(dc, item.rect, face, border, px(6));

                        if (k.label.empty()) {
                            drawVectorIcon(dc, item.rect, k.action, hot ? pal.text1 : pal.text2);
                        } else {
                            const COLORREF fg = isVar ? pal.text1 : (isOp ? (hot ? pal.text1 : pal.text2) : pal.text1);
                            HFONT font = isUtil ? smallFont : monoKeyFont;
                            workspace::PanelDrawing::text(dc, UiLanguage::text(k.label), item.rect, fg, font, DT_CENTER);
                        }
                        break;
                    }
                    case ItemKind::CATEGORY_TAB: {
                        const bool selected = item.index == activeCategory;
                        const auto &title = keypad->pages[static_cast<size_t>(item.index)].first;
                        workspace::PanelDrawing::text(dc, UiLanguage::text(title), item.rect,
                                                      selected ? pal.text1 : (hot ? pal.text1 : pal.text2),
                                                      selected ? uiMediumFont : smallFont, DT_CENTER);
                        if (selected) {
                            workspace::PanelDrawing::fill(dc, {item.rect.left + px(4), item.rect.bottom - px(2),
                                                               item.rect.right - px(4), item.rect.bottom},
                                                          pal.accentInk);
                        }
                        break;
                    }
                    case ItemKind::FUNCTION_KEY: {
                        const COLORREF face = down ? pal.keyPress : (hot ? pal.keyHover : pal.keyOp);
                        const COLORREF border = hot ? pal.hairlineStrong : face;
                        workspace::PanelDrawing::rounded(dc, item.rect, face, border, px(6));
                        workspace::PanelDrawing::text(dc, item.key->label, item.rect, pal.text1, monoSmallFont, DT_CENTER);
                        break;
                    }
                    case ItemKind::EXAMPLE_ROW: {
                        if (hot || down) {
                            workspace::PanelDrawing::rounded(dc, item.rect, down ? pal.keyPress : pal.keyHover,
                                                             down ? pal.keyPress : pal.keyHover, px(4));
                        }
                        workspace::PanelDrawing::fill(dc, {item.rect.left, item.rect.bottom - 1, item.rect.right,
                                                           item.rect.bottom},
                                                      pal.hairline);

                        const RECT thumbR{item.rect.left + px(4), item.rect.top + px(4), item.rect.left + px(32),
                                          item.rect.top + px(24)};
                        if (static_cast<size_t>(item.index) < exampleThumbs.size() &&
                            !exampleThumbs[static_cast<size_t>(item.index)].empty()) {
                            blitBgra(dc, thumbR, exampleThumbs[static_cast<size_t>(item.index)], THUMB_TEX_W,
                                     THUMB_TEX_H);
                        }
                        workspace::PanelDrawing::border(dc, thumbR, pal.hairline, 1);

                        const RECT nameR{item.rect.left + px(40), item.rect.top, item.rect.left + px(122),
                                         item.rect.bottom};
                        workspace::PanelDrawing::text(dc, UiLanguage::text(item.key->name), nameR, pal.text2, smallFont,
                                                      DT_LEFT);
                        const RECT exprR{item.rect.left + px(122), item.rect.top, item.rect.right - px(4),
                                         item.rect.bottom};
                        workspace::PanelDrawing::text(dc, item.key->label, exprR, pal.text1, monoSmallFont, DT_RIGHT);
                        break;
                    }
                    case ItemKind::CLOSE_CHECKBOX: {
                        const int boxSize = px(16);
                        const int boxTop = item.rect.top + (item.rect.bottom - item.rect.top - boxSize) / 2;
                        const RECT box{item.rect.left, boxTop, item.rect.left + boxSize, boxTop + boxSize};
                        workspace::PanelDrawing::rounded(dc, box, closeOnApply ? pal.accent : pal.field,
                                                         closeOnApply ? pal.accent : (hot ? pal.accentInk : pal.hairlineStrong),
                                                         px(4));
                        if (closeOnApply) {
                            const int penW = std::max(1, static_cast<int>(1.8f * static_cast<float>(dpi) / 96.0f + 0.5f));
                            HPEN pen = CreatePen(PS_SOLID, penW, pal.accentText);
                            HGDIOBJ prev = SelectObject(dc, pen);
                            const std::array<POINT, 3> check = {
                                {{box.left + px(4), box.top + px(8)},
                                 {box.left + px(7), box.top + px(11)},
                                 {box.left + px(12), box.top + px(5)}}};
                            Polyline(dc, check.data(), static_cast<int>(check.size()));
                            SelectObject(dc, prev);
                            DeleteObject(pen);
                        }
                        const RECT labelR{box.right + px(8), item.rect.top, item.rect.right, item.rect.bottom};
                        workspace::PanelDrawing::text(dc, UiLanguage::text(L"Close window after applying"), labelR,
                                                      hot ? pal.text1 : pal.text2, smallFont, DT_LEFT);
                        break;
                    }
                    case ItemKind::CLOSE_BUTTON: {
                        const COLORREF face = down ? pal.keyPress : (hot ? pal.keyHover : pal.keyFace);
                        workspace::PanelDrawing::rounded(dc, item.rect, face, hot ? pal.accentInk : pal.hairlineStrong,
                                                         px(6));
                        workspace::PanelDrawing::text(dc, UiLanguage::text(L"Close"), item.rect, pal.text1,
                                                      uiMediumFont, DT_CENTER);
                        break;
                    }
                    case ItemKind::APPLY_BUTTON: {
                        const COLORREF face = !item.enabled ? pal.surface
                                                            : (down ? pal.accentPress : (hot ? pal.accentHover : pal.accent));
                        const COLORREF border = !item.enabled ? pal.hairline : face;
                        const COLORREF fg = !item.enabled ? pal.text3 : pal.accentText;
                        workspace::PanelDrawing::rounded(dc, item.rect, face, border, px(6));
                        workspace::PanelDrawing::text(dc, UiLanguage::text(L"Apply to Main View"), item.rect, fg,
                                                      uiMediumFont, DT_CENTER);
                        break;
                    }
                    }

                    if (static_cast<int>(i) == focusedItem && item.enabled) {
                        RECT ring = item.rect;
                        InflateRect(&ring, px(2), px(2));
                        workspace::PanelDrawing::border(dc, ring, pal.accentInk, std::max(1, px(1)));
                    }
                }

                if (dc != target) {
                    backBuffer.present(target);
                }
            }

            static LRESULT CALLBACK editSubProc(HWND control, UINT message, WPARAM wParam, LPARAM lParam,
                                                UINT_PTR subclassId, DWORD_PTR data) {
                auto *self = reinterpret_cast<FormulaStudioImpl *>(data);
                if (message == WM_UNDO || message == EM_UNDO) {
                    self->runUndo();
                    return TRUE;
                }
                if (message == WM_KEYDOWN) {
                    const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                    const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                    if (wParam == VK_TAB) {
                        self->stepKeyboardFocus(shift ? -1 : 1);
                        return 0;
                    }
                    if (wParam == VK_RETURN) {
                        self->applyFormula();
                        return 0;
                    }
                    if (wParam == VK_ESCAPE) {
                        self->requestClose();
                        return 0;
                    }
                    if (ctrl && wParam == 'Z') {
                        if (shift) self->runRedo();
                        else self->runUndo();
                        return 0;
                    }
                    if (ctrl && wParam == 'Y') {
                        self->runRedo();
                        return 0;
                    }
                    if (ctrl && wParam == 'A') {
                        SendMessageW(control, EM_SETSEL, 0, -1);
                        return 0;
                    }
                }
                if (message == WM_CHAR && (wParam == VK_RETURN || wParam == VK_ESCAPE || wParam == VK_TAB)) {
                    return 0;
                }
                const LRESULT result = DefSubclassProc(control, message, wParam, lParam);
                if (message == WM_SETFOCUS) {
                    self->focusedItem = -1;
                    InvalidateRect(self->hwnd, nullptr, FALSE);
                } else if (message == WM_KILLFOCUS) {
                    InvalidateRect(self->hwnd, &self->shellRect, FALSE);
                } else if (!self->formulaValid &&
                           (message == WM_KEYDOWN || message == WM_KEYUP || message == WM_CHAR ||
                            message == WM_LBUTTONUP || message == WM_HSCROLL || message == WM_MOUSEWHEEL ||
                            (message == WM_MOUSEMOVE && (wParam & MK_LBUTTON)) || message == EM_SETSEL ||
                            message == EM_SCROLLCARET)) {
                    InvalidateRect(self->hwnd, &self->shellRect, FALSE);
                }
                if (message == WM_NCDESTROY) {
                    RemoveWindowSubclass(control, editSubProc, subclassId);
                }
                return result;
            }

            void positionOnOwner() const {
                RECT rc{0, 0, px(STUDIO_WIDTH), px(STUDIO_HEIGHT)};
                const DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE));
                const DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
                UiDpi::adjustWindowRect(rc, style, exStyle, dpi);
                const int winW = rc.right - rc.left;
                const int winH = rc.bottom - rc.top;

                RECT work{};
                MONITORINFO mi{sizeof(mi)};
                if (GetMonitorInfoW(MonitorFromWindow(owner ? owner : hwnd, MONITOR_DEFAULTTONEAREST), &mi)) {
                    work = mi.rcWork;
                } else {
                    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
                }

                int x = work.left + (work.right - work.left - winW) / 2;
                int y = work.top + (work.bottom - work.top - winH) / 2;
                if (owner && IsWindow(owner)) {
                    RECT ownerRc{};
                    GetWindowRect(owner, &ownerRc);
                    x = ownerRc.left + (ownerRc.right - ownerRc.left - winW) / 2;
                    y = ownerRc.top + (ownerRc.bottom - ownerRc.top - winH) / 2;
                }
                x = std::clamp(x, static_cast<int>(work.left), std::max(static_cast<int>(work.left), static_cast<int>(work.right) - winW));
                y = std::clamp(y, static_cast<int>(work.top), std::max(static_cast<int>(work.top), static_cast<int>(work.bottom) - winH));
                SetWindowPos(hwnd, nullptr, x, y, winW, winH, SWP_NOZORDER | SWP_NOACTIVATE);
            }

          public:
            FormulaStudioImpl(HWND ownerWindow, const std::wstring &initialFormula,
                              std::function<std::wstring(const std::wstring &)> applyCallback)
                : owner(ownerWindow), onApply(std::move(applyCallback)) {
                history.push_back(initialFormula);
                historyIndex = 0;
            }

            ~FormulaStudioImpl() {
                stopWorkerThread();
                releaseFonts();
                if (editBrush) {
                    DeleteObject(editBrush);
                    editBrush = nullptr;
                }
            }

            HWND handle() const { return hwnd; }

            // Brings an already-open studio forward without discarding in-progress formula edits or undo history.
            void bringToFront(std::function<std::wstring(const std::wstring &)> applyCallback) {
                onApply = std::move(applyCallback);
                if (IsIconic(hwnd)) {
                    ShowWindow(hwnd, SW_RESTORE);
                }
                SetForegroundWindow(hwnd);
                SetFocus(edit);
            }

            bool create(const std::wstring &initialFormula) {
                static bool registered = false;
                if (!registered) {
                    WNDCLASSEXW wc{sizeof(wc)};
                    wc.style = CS_HREDRAW | CS_VREDRAW;
                    wc.lpfnWndProc = wndProc;
                    wc.hInstance = GetModuleHandleW(nullptr);
                    wc.hCursor = LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(IDC_ARROW));
                    wc.lpszClassName = STUDIO_WINDOW_CLASS;
                    RegisterClassExW(&wc);
                    registered = true;
                }

                const std::wstring title = UiLanguage::text(L"Formula Studio");
                constexpr DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
                hwnd = CreateWindowExW(0, STUDIO_WINDOW_CLASS, title.c_str(), style, CW_USEDEFAULT, CW_USEDEFAULT,
                                       STUDIO_WIDTH, STUDIO_HEIGHT, owner, nullptr, GetModuleHandleW(nullptr), this);
                if (!hwnd) {
                    return false;
                }

                dpi = UiDpi::forWindow(hwnd);
                pal = currentPalette();
                createFonts();
                rebuildBrush();
                applyDarkWindowFrame(hwnd);
                positionOnOwner();

                edit = CreateWindowExW(0, L"EDIT", initialFormula.c_str(),
                                       WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 100, 24, hwnd,
                                       reinterpret_cast<HMENU>(static_cast<INT_PTR>(EDIT_CONTROL_ID)),
                                       GetModuleHandleW(nullptr), nullptr);
                SendMessageW(edit, WM_SETFONT, reinterpret_cast<WPARAM>(monoEditFont), FALSE);
                SetWindowSubclass(edit, editSubProc, 1, reinterpret_cast<DWORD_PTR>(this));

                renderExampleThumbnails();
                buildLayout();
                startWorker();
                validateFormula(false, true);

                ShowWindow(hwnd, SW_SHOWNORMAL);
                UpdateWindow(hwnd);
                SetFocus(edit);
                SendMessageW(edit, EM_SETSEL, initialFormula.size(), initialFormula.size());
                return true;
            }

            void refreshTheme() {
                if (!IsWindow(hwnd)) return;
                pal = currentPalette();
                rebuildBrush();
                applyDarkWindowFrame(hwnd);
                InvalidateRect(hwnd, nullptr, TRUE);
                if (IsWindow(edit)) {
                    InvalidateRect(edit, nullptr, TRUE);
                }
            }

            bool handleMessageFilter(const MSG &msg) {
                if (!IsWindow(hwnd) || !IsWindowVisible(hwnd)) return false;
                if (msg.hwnd != hwnd && !IsChild(hwnd, msg.hwnd)) return false;
                if (msg.message == WM_KEYDOWN) {
                    const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                    const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                    if (msg.wParam == VK_TAB) {
                        stepKeyboardFocus(shift ? -1 : 1);
                        return true;
                    }
                    if (msg.wParam == VK_SPACE && focusedItem >= 0) {
                        activateItem(focusedItem);
                        return true;
                    }
                    if (msg.wParam == VK_ESCAPE) {
                        requestClose();
                        return true;
                    }
                    if (msg.wParam == VK_RETURN) {
                        if (focusedItem >= 0) activateItem(focusedItem);
                        else applyFormula();
                        return true;
                    }
                    if (ctrl && msg.wParam == 'Z') {
                        if (shift) runRedo();
                        else runUndo();
                        return true;
                    }
                    if (ctrl && msg.wParam == 'Y') {
                        runRedo();
                        return true;
                    }
                }
                return false;
            }

            LRESULT onMessage(UINT message, WPARAM wParam, LPARAM lParam);

            static LRESULT CALLBACK wndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
                auto *self = reinterpret_cast<FormulaStudioImpl *>(GetWindowLongPtrW(window, GWLP_USERDATA));
                if (message == WM_NCCREATE) {
                    auto *cs = reinterpret_cast<CREATESTRUCTW *>(lParam);
                    self = static_cast<FormulaStudioImpl *>(cs->lpCreateParams);
                    self->hwnd = window;
                    SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
                    // Forward to DefWindowProcW so Windows stores the title bar caption from CREATESTRUCTW.
                    return DefWindowProcW(window, message, wParam, lParam);
                }
                if (!self) {
                    return DefWindowProcW(window, message, wParam, lParam);
                }
                return self->onMessage(message, wParam, lParam);
            }
        };

        std::unique_ptr<FormulaStudioImpl> activeStudio;

        LRESULT FormulaStudioImpl::onMessage(UINT message, WPARAM wParam, LPARAM lParam) {
            switch (message) {
            case WM_ERASEBKGND:
                return 1;
            case WM_PAINT: {
                PAINTSTRUCT ps{};
                HDC dc = BeginPaint(hwnd, &ps);
                paint(dc);
                EndPaint(hwnd, &ps);
                return 0;
            }
            case WM_CTLCOLOREDIT: {
                HDC dc = reinterpret_cast<HDC>(wParam);
                SetTextColor(dc, pal.text1);
                SetBkColor(dc, pal.field);
                return reinterpret_cast<LRESULT>(editBrush);
            }
            case WM_COMMAND:
                if (LOWORD(wParam) == EDIT_CONTROL_ID && HIWORD(wParam) == EN_CHANGE && !suppressEditChange) {
                    validateFormula(true, false);
                    return 0;
                }
                break;
            case WM_TIMER:
                if (wParam == PREVIEW_DEBOUNCE_TIMER) {
                    KillTimer(hwnd, PREVIEW_DEBOUNCE_TIMER);
                    dispatchPreviewJob();
                    return 0;
                }
                break;
            case WM_STUDIO_PREVIEW_READY: {
                std::vector<uint32_t> pixels;
                uint64_t resGen = 0;
                int ms = 0;
                bool ok = false;
                {
                    std::scoped_lock lock(resultMutex);
                    if (hasReadyResult) {
                        pixels = std::move(readyPixels);
                        resGen = readyGen;
                        ms = readyMs;
                        hasReadyResult = false;
                        ok = true;
                    }
                }
                // Drop stale results from earlier edits or results arriving after the formula became invalid.
                if (ok && formulaValid && resGen == latestGen.load(std::memory_order_relaxed)) {
                    previewPixels = std::move(pixels);
                    previewDimPixels = dimBgra(previewPixels);
                    previewMs = ms;
                    hasPreview = true;
                    InvalidateRect(hwnd, &previewCardRect, FALSE);
                }
                return 0;
            }
            case WM_MOUSEMOVE: {
                if (!trackingMouse) {
                    TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, hwnd, 0};
                    TrackMouseEvent(&tme);
                    trackingMouse = true;
                }
                const POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                const int hit = hitTest(pt);
                if (hit != hoveredItem) {
                    const int prev = hoveredItem;
                    hoveredItem = hit;
                    if (prev >= 0 && prev < static_cast<int>(items.size())) {
                        InvalidateRect(hwnd, &items[static_cast<size_t>(prev)].rect, FALSE);
                    }
                    if (hoveredItem >= 0) {
                        InvalidateRect(hwnd, &items[static_cast<size_t>(hoveredItem)].rect, FALSE);
                    }
                }
                return 0;
            }
            case WM_MOUSELEAVE:
                trackingMouse = false;
                if (hoveredItem >= 0 && hoveredItem < static_cast<int>(items.size())) {
                    const RECT r = items[static_cast<size_t>(hoveredItem)].rect;
                    hoveredItem = -1;
                    InvalidateRect(hwnd, &r, FALSE);
                }
                return 0;
            case WM_LBUTTONDOWN: {
                if (focusedItem >= 0) {
                    focusedItem = -1;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                const POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                if (PtInRect(&shellRect, pt)) {
                    SetFocus(edit);
                    return 0;
                }
                const int hit = hitTest(pt);
                if (hit >= 0 && items[static_cast<size_t>(hit)].enabled) {
                    pressedItem = hit;
                    SetCapture(hwnd);
                    InvalidateRect(hwnd, &items[static_cast<size_t>(hit)].rect, FALSE);
                }
                return 0;
            }
            case WM_LBUTTONUP: {
                if (pressedItem >= 0) {
                    const int pressed = pressedItem;
                    pressedItem = -1;
                    if (GetCapture() == hwnd) {
                        ReleaseCapture();
                    }
                    if (pressed < static_cast<int>(items.size())) {
                        InvalidateRect(hwnd, &items[static_cast<size_t>(pressed)].rect, FALSE);
                    }
                    const POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                    if (hitTest(pt) == pressed) {
                        activateItem(pressed);
                    }
                }
                return 0;
            }
            case WM_CAPTURECHANGED:
                if (pressedItem >= 0) {
                    const int prev = pressedItem;
                    pressedItem = -1;
                    if (prev < static_cast<int>(items.size())) {
                        InvalidateRect(hwnd, &items[static_cast<size_t>(prev)].rect, FALSE);
                    }
                }
                return 0;
            case WM_SETCURSOR:
                if (LOWORD(lParam) == HTCLIENT) {
                    POINT pt{};
                    GetCursorPos(&pt);
                    ScreenToClient(hwnd, &pt);
                    const int hit = hitTest(pt);
                    if (hit >= 0 && items[static_cast<size_t>(hit)].enabled) {
                        SetCursor(LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(IDC_HAND)));
                        return TRUE;
                    }
                }
                break;
            case WM_DPICHANGED: {
                dpi = HIWORD(wParam);
                createFonts();
                if (IsWindow(edit)) {
                    SendMessageW(edit, WM_SETFONT, reinterpret_cast<WPARAM>(monoEditFont), FALSE);
                }
                RECT rc{0, 0, px(STUDIO_WIDTH), px(STUDIO_HEIGHT)};
                const DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE));
                const DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
                UiDpi::adjustWindowRect(rc, style, exStyle, dpi);
                const auto *suggested = reinterpret_cast<const RECT *>(lParam);
                if (suggested) {
                    SetWindowPos(hwnd, nullptr, suggested->left, suggested->top, rc.right - rc.left,
                                 rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE);
                } else {
                    positionOnOwner();
                }
                buildLayout();
                InvalidateRect(hwnd, nullptr, TRUE);
                return 0;
            }
            case WM_CLOSE:
                DestroyWindow(hwnd);
                return 0;
            case WM_NCDESTROY: {
                KillTimer(hwnd, PREVIEW_DEBOUNCE_TIMER);
                stopWorkerThread();
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                hwnd = nullptr;
                if (activeStudio.get() == this) {
                    activeStudio.reset();
                }
                return 0;
            }
            default:
                break;
            }
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }
    } // namespace

    void FormulaStudioWindow::open(HWND owner, const std::wstring &initialFormula,
                                   std::function<std::wstring(const std::wstring &)> onApply) {
        if (activeStudio && IsWindow(activeStudio->handle())) {
            activeStudio->bringToFront(std::move(onApply));
            return;
        }
        auto studio = std::make_unique<FormulaStudioImpl>(owner, initialFormula, std::move(onApply));
        if (studio->create(initialFormula)) {
            activeStudio = std::move(studio);
        }
    }

    bool FormulaStudioWindow::filterMessage(const MSG &message) {
        return activeStudio && activeStudio->handleMessageFilter(message);
    }

    void FormulaStudioWindow::applyTheme() {
        if (activeStudio) {
            activeStudio->refreshTheme();
        }
    }

    void FormulaStudioWindow::close() {
        if (activeStudio && IsWindow(activeStudio->handle())) {
            DestroyWindow(activeStudio->handle());
        }
        activeStudio.reset();
    }
} // namespace merutilm::rff2
