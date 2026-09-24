//
// Modified by GPT-6 on 2026-09-14, 2026-09-15, 2026-09-21, 2026-09-22
//

#include "ComparisonWorkspace.hpp"
#include "AttributeFormModel.hpp"
#include "AppearanceEditTracker.hpp"
#include "PanelBackBuffer.hpp"
#include "PanelDrawing.hpp"
#include "../RenderScene.hpp"
#include "../SettingsTheme.hpp"
#include "../UiDpi.hpp"
#include <windowsx.h>
#include <limits>
#include <opencv2/imgproc.hpp>

namespace merutilm::rff2::workspace {
    namespace {
        enum class ComparisonMode { LivePreview = 0, Reference = 1, Current = 2, Split = 3 };
        enum class ReferenceSource { FixedA = 0, BeforeLastEdit = 1 };
    } // namespace

    struct ComparisonWorkspace::State {
        RenderScene &scene;
        HWND parent, canvas, window = nullptr;
        std::optional<ShaderAttribute> reference;
        std::shared_ptr<AppearanceEditTracker> edits;
        cv::Mat referenceImage, currentImage, scaledReferenceImage, scaledCurrentImage;
        PanelBackBuffer buffer;
        HFONT font = nullptr;
        UINT dpi = 96;
        ComparisonMode mode = ComparisonMode::LivePreview;
        ReferenceSource source = ReferenceSource::FixedA;
        std::optional<ReferenceSource> shownSource;
        uint64_t editRevision = 0;
        float split = .5f, seconds = 0;
        bool stale = true, dragging = false, panning = false, panMoved = false;
        uint64_t revision = std::numeric_limits<uint64_t>::max();
        std::wstring message = L"Capture current appearance as A, then edit B.";
        State(RenderScene &scene, HWND parent, HWND canvas, std::shared_ptr<AppearanceEditTracker> edits)
            : scene(scene), parent(parent), canvas(canvas), edits(std::move(edits)) {
            applyDpi(UiDpi::forWindow(parent));
        }
        const ShaderAttribute *selectedReference() const {
            return source == ReferenceSource::FixedA
                       ? (reference ? &*reference : nullptr)
                       : (edits && edits->reference() ? &edits->reference()->shader : nullptr);
        }
        ~State() {
            if (IsWindow(window)) {
                DestroyWindow(window);
            }
            if (font) {
                DeleteObject(font);
            }
        }
        void invalidate() {
            if (window) {
                InvalidateRect(window, nullptr, FALSE);
            }
        }
        void hide() {
            if (window) {
                ShowWindow(window, SW_HIDE);
            }
        }
        int px(int value) const {
            return UiDpi::pixels(value, dpi);
        }
        int dividerX(int width) const {
            return std::clamp(int(std::lround(split * width)), 0, width);
        }
        void applyDpi(UINT value) {
            if (!value || (value == dpi && font)) {
                return;
            }
            const auto next = CreateFontW(-UiDpi::pixels(13, value), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                          CLEARTYPE_QUALITY, DEFAULT_PITCH, UiLanguage::fontFace());
            if (!next) {
                return;
            }
            if (font) {
                DeleteObject(font);
            }
            font = next;
            dpi = value;
            invalidate();
        }
        void label(HDC dc, int left, int right, int height, const wchar_t *full, bool trailing,
                   const wchar_t *brief = nullptr) {
            const int margin = px(12), padding = px(8), available = right - left - 2 * margin;
            if (height < margin + px(28) || available <= 0) {
                return;
            }
            const wchar_t compact[] = {full[0], 0};
            const wchar_t *value = UiLanguage::label(full);
            SIZE measured{};
            GetTextExtentPoint32W(dc, value, int(wcslen(value)), &measured);
            if (measured.cx + 2 * padding > available) {
                value = brief ? UiLanguage::label(brief) : compact;
                GetTextExtentPoint32W(dc, value, int(wcslen(value)), &measured);
            }
            const int width = measured.cx + 2 * padding;
            if (width > available) {
                return;
            }
            const int x = trailing ? right - margin - width : left + margin;
            RECT box{x, margin, x + width, margin + px(28)};
            const auto &theme = settingsTheme();
            PanelDrawing::fill(dc, box, theme.textFieldBackground);
            const auto border = CreateSolidBrush(theme.textFieldBorder);
            FrameRect(dc, &box, border);
            DeleteObject(border);
            InflateRect(&box, -padding, 0);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, theme.text);
            DrawTextW(dc, value, -1, &box, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
        void paint(HDC target) {
            RECT bounds;
            GetClientRect(window, &bounds);
            const int w = bounds.right, h = bounds.bottom;
            auto dc = buffer.begin(target, w, h);
            if (!dc) {
                return;
            }
            PanelDrawing::fill(dc, bounds, settingsTheme().background);
            const auto draw = [&](const cv::Mat &source, cv::Mat &scaled) {
                if (source.empty()) {
                    return;
                }
                const cv::Mat *pixels = &source;
                if (source.cols != w || source.rows != h) {
                    if (scaled.cols != w || scaled.rows != h) {
                        cv::resize(source, scaled, cv::Size(w, h), 0, 0,
                                   w < source.cols ? cv::INTER_AREA : cv::INTER_LINEAR);
                    }
                    pixels = &scaled;
                }
                BITMAPINFO info{};
                info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                info.bmiHeader.biWidth = w;
                info.bmiHeader.biHeight = -h;
                info.bmiHeader.biPlanes = 1;
                info.bmiHeader.biBitCount = 32;
                info.bmiHeader.biCompression = BI_RGB;
                SetDIBitsToDevice(dc, 0, 0, w, h, 0, 0, 0, h, pixels->data, &info, DIB_RGB_COLORS);
            };
            if (mode == ComparisonMode::Reference) {
                draw(referenceImage, scaledReferenceImage);
            } else {
                draw(currentImage, scaledCurrentImage);
            }
            const int divider = dividerX(w);
            if (mode == ComparisonMode::Split) {
                const int saved = SaveDC(dc);
                if (saved == 0) {
                    return;
                }
                try {
                    IntersectClipRect(dc, 0, 0, divider, h);
                    draw(referenceImage, scaledReferenceImage);
                } catch (...) {
                    RestoreDC(dc, saved);
                    throw;
                }
                RestoreDC(dc, saved);
                RECT light{divider - px(1), 0, divider, h}, dark{divider, 0, divider + px(1), h};
                FillRect(dc, &light, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
                FillRect(dc, &dark, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            }
            const auto old = SelectObject(dc, font ? font : GetStockObject(DEFAULT_GUI_FONT));
            const auto first = source == ReferenceSource::BeforeLastEdit ? L"Before Edit" : L"A  Reference",
                       second = source == ReferenceSource::BeforeLastEdit ? L"Current" : L"B  Current";
            try {
                if (mode == ComparisonMode::Reference) {
                    label(dc, 0, w, h, first, false,
                          source == ReferenceSource::BeforeLastEdit ? L"Before" : nullptr);
                } else if (mode == ComparisonMode::Current) {
                    label(dc, 0, w, h, second, false,
                          source == ReferenceSource::BeforeLastEdit ? L"Now" : nullptr);
                } else {
                    label(dc, 0, divider, h, first, false,
                          source == ReferenceSource::BeforeLastEdit ? L"Before" : nullptr);
                    label(dc, divider, w, h, second, true,
                          source == ReferenceSource::BeforeLastEdit ? L"Now" : nullptr);
                }
            } catch (...) {
                SelectObject(dc, old);
                throw;
            }
            SelectObject(dc, old);
            buffer.present(target);
        }
        void setSplit(int x) {
            RECT r;
            GetClientRect(window, &r);
            split = std::clamp(float(x) / std::max(1L, r.right), 0.f, 1.f);
            invalidate();
        }
        static LRESULT CALLBACK procedure(HWND hwnd, UINT message, WPARAM w, LPARAM l) {
            auto *self = reinterpret_cast<State *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (message == WM_NCCREATE) {
                self = static_cast<State *>(reinterpret_cast<CREATESTRUCTW *>(l)->lpCreateParams);
                self->window = hwnd;
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            if (!self) {
                return DefWindowProcW(hwnd, message, w, l);
            }
            switch (message) {
            case WM_ERASEBKGND:
                return 1;
            case WM_PAINT: {
                PAINTSTRUCT ps;
                auto dc = BeginPaint(hwnd, &ps);
                try {
                    self->paint(dc);
                } catch (...) {
                    EndPaint(hwnd, &ps);
                    throw;
                }
                EndPaint(hwnd, &ps);
                return 0;
            }
            case WM_PRINTCLIENT:
                self->paint(reinterpret_cast<HDC>(w));
                return 0;
            case WM_GETFONT:
                return reinterpret_cast<LRESULT>(self->font);
            case WM_LBUTTONDOWN: {
                RECT r;
                GetClientRect(hwnd, &r);
                if (self->mode == ComparisonMode::Split &&
                    std::abs(GET_X_LPARAM(l) - self->dividerX(r.right)) <= self->px(10)) {
                    self->dragging = true;
                    SetCapture(hwnd);
                    SetFocus(hwnd);
                    return 0;
                }
                self->panning = true;
                self->panMoved = false;
                SetCapture(hwnd);
                SetFocus(hwnd);
                SendMessageW(self->canvas, message, w, l);
                return 0;
            }
            case WM_MOUSEMOVE:
                if (self->dragging) {
                    self->setSplit(GET_X_LPARAM(l));
                    return 0;
                }
                if (self->panning) {
                    self->panMoved = true;
                    self->hide();
                    SendMessageW(self->canvas, message, w, l);
                    return 0;
                }
                break;
            case WM_LBUTTONUP:
                if (self->panning) {
                    self->panning = false;
                    SendMessageW(self->canvas, message, w, l);
                    ReleaseCapture();
                    return 0;
                }
                if (self->dragging) {
                    self->dragging = false;
                    ReleaseCapture();
                    return 0;
                }
                break;
            case WM_CAPTURECHANGED:
                self->dragging = false;
                if (self->panning) {
                    self->panning = false;
                    SendMessageW(self->canvas, WM_LBUTTONUP, 0, 0);
                }
                return 0;
            case WM_KEYDOWN:
                if (w == VK_ESCAPE) {
                    self->mode = ComparisonMode::LivePreview;
                    self->hide();
                    SetFocus(self->canvas);
                    return 0;
                }
                if (w == VK_SPACE) {
                    self->mode = self->mode == ComparisonMode::Reference ? ComparisonMode::Current
                                                                         : ComparisonMode::Reference;
                    self->invalidate();
                    return 0;
                }
                if ((w == VK_LEFT || w == VK_RIGHT) && self->mode == ComparisonMode::Split) {
                    self->split = std::clamp(self->split + (w == VK_LEFT ? -.01f : .01f), 0.f, 1.f);
                    self->invalidate();
                    return 0;
                }
                break;
            case WM_MOUSEWHEEL:
                self->hide();
                SendMessageW(self->canvas, message, w, l);
                return 0;
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
                self->hide();
                SendMessageW(self->canvas, message, w, l);
                return 0;
            case WM_SETCURSOR: {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hwnd, &pt);
                RECT r;
                GetClientRect(hwnd, &r);
                SetCursor(
                    LoadCursorA(nullptr, self->mode == ComparisonMode::Split &&
                                                 std::abs(pt.x - self->dividerX(r.right)) <= self->px(10)
                                             ? IDC_SIZEWE
                                             : IDC_ARROW));
                return TRUE;
            }
            }
            return DefWindowProcW(hwnd, message, w, l);
        }
        void show() {
            if (!window) {
                WNDCLASSW cls{};
                cls.hInstance = GetModuleHandleW(nullptr);
                cls.lpfnWndProc = procedure;
                cls.lpszClassName = L"RFF.Workspace.Comparison";
                RegisterClassW(&cls);
                window = CreateWindowExW(0, cls.lpszClassName, L"Appearance comparison",
                                         WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS, 0, 0, 1, 1, parent, nullptr,
                                         cls.hInstance, this);
            }
            RECT r;
            GetWindowRect(canvas, &r);
            MapWindowPoints(nullptr, parent, reinterpret_cast<POINT *>(&r), 2);
            RECT old;
            GetWindowRect(window, &old);
            MapWindowPoints(nullptr, parent, reinterpret_cast<POINT *>(&old), 2);
            if (shownSource != source) {
                SetWindowTextW(window, source == ReferenceSource::BeforeLastEdit
                                           ? L"Before Edit and Current appearance comparison"
                                           : L"Fixed A and Current B appearance comparison");
                shownSource = source;
            }
            if (!EqualRect(&r, &old)) {
                SetWindowPos(window, HWND_TOP, r.left, r.top, r.right - r.left, r.bottom - r.top,
                             SWP_NOACTIVATE);
            }
            if (!IsWindowVisible(window)) {
                invalidate();
                UpdateWindow(window);
                SetWindowPos(window, HWND_TOP, 0, 0, 0, 0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
            }
        }
    };
    ComparisonWorkspace::ComparisonWorkspace(RenderScene &scene, HWND parent, HWND canvas,
                                             std::shared_ptr<AppearanceEditTracker> edits)
        : state(std::make_unique<State>(scene, parent, canvas, std::move(edits))) {}
    ComparisonWorkspace::~ComparisonWorkspace() = default;
    void ComparisonWorkspace::applyDpi(UINT dpi) {
        state->applyDpi(dpi);
    }
    void ComparisonWorkspace::applyTheme() {
        state->invalidate();
    }
    bool ComparisonWorkspace::active() const {
        return state->mode != ComparisonMode::LivePreview && state->selectedReference();
    }
    void ComparisonWorkspace::clear() {
        state->hide();
        state->reference.reset();
        if (state->edits) {
            state->edits->clear();
        }
        state->referenceImage.release();
        state->currentImage.release();
        state->scaledReferenceImage.release();
        state->scaledCurrentImage.release();
        state->mode = ComparisonMode::LivePreview;
        state->source = ReferenceSource::FixedA;
        state->stale = true;
        state->message = L"Capture current appearance as A, then edit B.";
    }
    void ComparisonWorkspace::update(bool suspended) {
        auto &comparison = *state;
        if (suspended || !active()) {
            comparison.hide();
            return;
        }
        if (comparison.source == ReferenceSource::BeforeLastEdit && comparison.edits &&
            comparison.editRevision != comparison.edits->revision()) {
            comparison.editRevision = comparison.edits->revision();
            comparison.stale = true;
        }
        const auto &requests = comparison.scene.getRequests();
        if (!comparison.scene.isIdleCompute() || requests.recomputeRequested || requests.resizeRequested ||
            requests.shaderRequested || comparison.scene.isLongJobBusy() ||
            (comparison.panning && comparison.panMoved)) {
            comparison.stale = true;
            comparison.hide();
            comparison.message = L"Comparison waits for the current view to finish.";
            return;
        }
        if (comparison.stale || comparison.revision != comparison.scene.getPreviewRevision()) {
            try {
                auto reference = *comparison.selectedReference();
                reference.camera = comparison.scene.getAttribute().shader.camera;
                auto images = comparison.scene.renderComparison(reference, comparison.seconds);
                if (images.first.empty() || images.second.empty()) {
                    comparison.hide();
                    return;
                }
                comparison.referenceImage = std::move(images.first);
                comparison.currentImage = std::move(images.second);
                comparison.scaledReferenceImage.release();
                comparison.scaledCurrentImage.release();
                comparison.revision = comparison.scene.getPreviewRevision();
                comparison.stale = false;
                comparison.invalidate();
                comparison.message = (comparison.source == ReferenceSource::BeforeLastEdit
                                          ? L"Before edit: " + comparison.edits->reference()->label + L". "
                                          : L"A is fixed. ") +
                                     std::wstring(L"Both share view, quality and time (") +
                                     AttributeFormModel::number(comparison.seconds) +
                                     L" s). Export uses current settings.";
            } catch (const std::exception &) {
                comparison.hide();
                comparison.mode = ComparisonMode::LivePreview;
                comparison.message = L"Comparison failed. Current settings are unchanged.";
                return;
            }
        }
        comparison.show();
    }
    WorkspaceForm ComparisonWorkspace::form() {
        auto self = shared_from_this();
        WorkspaceForm form;
        form.title = L"A/B Comparison";
        form.groups = {L"Compare Appearance"};
        form.fields = {
            {"compare.source",
             0,
             L"Reference Source",
             L"Fixed A is captured manually. Before Last Edit records the start of an appearance edit "
             L"automatically.",
             [self] { return std::to_wstring(static_cast<int>(self->state->source)); },
             {{L"0", L"Fixed A"}, {L"1", L"Before Last Edit"}}},
            {"compare.mode",
             0,
             L"View",
             L"Space switches reference and current on the canvas. Escape returns to live preview.",
             [self] { return std::to_wstring(static_cast<int>(self->state->mode)); },
             {{L"0", L"Live Preview"},
              {L"1", L"Reference"},
              {L"2", L"Current"},
              {L"3", L"Split Reference / Current"}}},
            {"compare.split",
             0,
             L"Split Position (%)",
             L"0 to 100. Drag the divider or use arrow keys.",
             [self] { return AttributeFormModel::number(self->state->split * 100); },
             {}},
            {"compare.seconds",
             0,
             L"Comparison Time (s)",
             L"Both looks use this fixed animation time, from 0 to 86400 seconds.",
             [self] { return AttributeFormModel::number(self->state->seconds); },
             {}}};
        for (auto &field : form.fields) {
            field.persisted = false;
        }
        form.fields[2].validate = AttributeFormModel::rangeValidation<float>(0, 100);
        form.fields[3].validate = AttributeFormModel::rangeValidation<float>(0, 86400);
        form.apply = [self](const FormDraft &draft) {
            auto &comparison = *self->state;
            int source = static_cast<int>(comparison.source), mode = static_cast<int>(comparison.mode);
            float split = comparison.split * 100, seconds = comparison.seconds;
            for (const auto &[id, text] : draft) {
                if (id == "compare.source") {
                    if (!AttributeFormModel::parse(text, source) || source < 0 || source > 1) {
                        return std::wstring(L"Choose a reference source.");
                    }
                } else if (id == "compare.mode") {
                    if (!AttributeFormModel::parse(text, mode) || mode < 0 || mode > 3) {
                        return std::wstring(L"Choose a comparison view.");
                    }
                } else if (id == "compare.split") {
                    if (!AttributeFormModel::parse(text, split) || split < 0 || split > 100) {
                        return std::wstring(L"Split position must be from 0 to 100.");
                    }
                } else if (id == "compare.seconds") {
                    if (!AttributeFormModel::parse(text, seconds) || seconds < 0 || seconds > 86400) {
                        return std::wstring(L"Time must be from 0 to 86400 seconds.");
                    }
                } else {
                    return std::wstring(L"Unknown comparison setting.");
                }
            }
            if (mode && source == 0 && !comparison.reference) {
                return std::wstring(L"Capture A before choosing a comparison view.");
            }
            if (mode && source == 1 && (!comparison.edits || !comparison.edits->reference())) {
                return std::wstring(L"Edit an appearance setting before comparing with Before Last Edit.");
            }
            if (seconds != comparison.seconds || source != static_cast<int>(comparison.source)) {
                comparison.stale = true;
            }
            comparison.source = static_cast<ReferenceSource>(source);
            comparison.mode = static_cast<ComparisonMode>(mode);
            comparison.split = split / 100;
            comparison.seconds = seconds;
            comparison.invalidate();
            if (!mode) {
                comparison.hide();
            }
            comparison.message = source == 1
                                     ? L"Before Last Edit updates automatically for each appearance edit. "
                                       L"Fixed A stays separate."
                                     : L"Capture Current as A records a fixed reference.";
            return std::wstring{};
        };
        form.actions = {{0, L"Capture Current as A",
                         [self] {
                             auto &comparison = *self->state;
                             comparison.reference = comparison.scene.getAttribute().shader;
                             comparison.source = ReferenceSource::FixedA;
                             comparison.stale = true;
                             if (comparison.mode == ComparisonMode::LivePreview) {
                                 comparison.mode = ComparisonMode::Split;
                             }
                             comparison.message =
                                 L"A captured. Edit any appearance settings to compare with B.";
                         }},
                        {0, L"Toggle Reference / Current",
                         [self] {
                             auto &comparison = *self->state;
                             if (comparison.selectedReference()) {
                                 comparison.mode = comparison.mode == ComparisonMode::Reference
                                                       ? ComparisonMode::Current
                                                       : ComparisonMode::Reference;
                                 comparison.invalidate();
                             }
                         }},
                        {0, L"Clear Comparison", [self] { self->clear(); }, true}};
        form.canUndo = form.canRedo = form.undo = form.redo = [] { return false; };
        form.clearHistory = [self] { self->clear(); };
        form.status = [self] { return self->state->message; };
        return form;
    }
} // namespace merutilm::rff2::workspace
