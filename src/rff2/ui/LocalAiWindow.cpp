//
// Modified by GPT-6 on 2026-09-20, 2026-09-21, 2026-09-22, 2026-09-23, 2026-09-24, 2026-09-25
//

#include "LocalAiWindow.hpp"
#include "SettingsMenu.hpp"
#include "CallbackExplore.hpp"
#include "../locator/MandelbrotLocator.h"
#include "NativeDialogs.hpp"
#include "UiDpi.hpp"
#include "IOUtilities.h"
#include "../io/LocalAiSettings.hpp"
#include "../io/ConfigIO.h"
#include "../constants/ExtensionConstants.hpp"
#include <mutex>
#include <thread>
#include <sstream>
#include <oleacc.h>
#include <uxtheme.h>
#include <iomanip>
#include <opencv2/imgcodecs.hpp>
#include "workspace/WorkspaceButton.hpp"
#include "workspace/WorkspaceEditDrawing.hpp"

namespace merutilm::rff2 {
    namespace {
        struct Job {
            std::atomic_bool cancel{false};
            std::atomic_bool done{false};
            std::mutex mutex;
            std::string log;
            std::string error;
            LocalAiSettings::Statistics statistics;
            int channel = 0;
            std::optional<LocalAiSettings::Result> result;
            std::optional<LocalAiSettings::ZoomTarget> destination;
            void append(const std::string &text) {
                std::lock_guard lock(mutex);
                channel = 0;
                log += text + "\r\n";
            }
            void token(const std::string &text, bool reasoning = false) {
                std::lock_guard lock(mutex);
                const int next = reasoning ? 1 : 2;
                if (channel != next) {
                    log += reasoning ? "\r\n[Reasoning]\r\n" : "\r\n[Response]\r\n";
                    channel = next;
                }
                log += text;
            }
        };
        struct LocateJob {
            std::atomic_bool done = false;
            std::atomic_bool cancel = false;
            std::optional<fp_complex> center;
            float zoom = 0;
            std::string error;
        };
        struct State {
            HWND input{};
            HWND output{};
            HWND generate{};
            HWND apply{};
            HWND undo{};
            HWND cancel{};
            HWND newChat{};
            HWND errorLimit{};
            HWND autoRefine{};
            HWND refinementLimit{};
            HWND zoomFactor{};
            HWND startZoom{};
            HWND locate{};
            HWND appearanceTab{};
            HWND exploreTab{};
            HWND zoomInput{};
            HWND zoomLimit{};
            HWND zoomErrors{};
            HWND retryLocate{};
            HWND repeatExplore{};
            HWND retryDecrement{};
            bool useMinibrot = false;
            bool locateNext = false;
            bool retryLower = false;
            bool repeatCycles = false;
            int cycles = 0;
            int retries = 0;
            std::shared_ptr<LocateJob> locating;
            bool exploring = false;
            bool viewingZoom = false;
            bool verifyZoom = false;
            std::optional<FractalAttribute> beforeZoom;
            double attemptedX = 0;
            double attemptedY = 0;
            double attemptedFactor = 2;
            int lostStructureRetries = 0;
            std::vector<std::pair<double, double>> rejectedTargets;
            LocalAiSettings::Json iterationEvidence;
            std::vector<std::string> candidateImages;
            std::string appearanceLog;
            std::string explorationLog;
            bool autoStart = false;
            bool newChatPending = false;

            std::shared_ptr<Job> job;
            std::optional<LocalAiSettings::Result> ready;
            std::optional<ShaderAttribute> before;
            std::string appliedSignature;
            RenderScene *scene{};
            std::string shown;
            HWND context{};
            HWND speed{};
            HWND model{};
            std::mutex metadataMutex;
            LocalAiSettings::Json info = {{"context", -1}, {"llama", false}};
            std::string modelName;
            std::atomic_bool metadataBusy{false};
            std::atomic_bool closed{false};
            int metadataTicks = 25;
            size_t promptBytes = 0;
            bool completed = false;
            LocalAiSettings::Statistics statistics;
            bool vision = false;
            bool refining = false;
            bool capturePending = false;
            int rounds = 0;
            int maxRounds = 3;
            double bestScore = -1;
            std::optional<ShaderAttribute> best;
            std::string instruction;
            std::string history;
            std::string expectedShader;
            std::string expectedView;
            std::string sessionLog;
            LocalAiSettings::Json connection;
            std::chrono::steady_clock::time_point captureStarted;
            uint64_t capturedRevision = 0;
            bool revisionTracked = false;
            int staleImageRetries = 0;
        };
        std::string signature(const ShaderAttribute &shader) {
            auto attributes = RenderScene::genDefaultAttr();
            attributes.shader = shader;
            std::ostringstream stream(std::ios::binary);
            ConfigIO::write(stream, attributes, 1, 1);
            return stream.str();
        }
        std::string viewSignature(RenderScene &scene) {
            auto attributes = scene.getAttribute();
            attributes.shader = ShaderAttribute{};
            if (attributes.fractal.autoMaxIteration) {
                attributes.fractal.maxIteration = 0;
            }
            const auto size = scene.documentCanvasSize();
            std::ostringstream stream(std::ios::binary);
            ConfigIO::write(stream, attributes, size.cx, size.cy);
            return stream.str();
        }
        void rememberCurrentView(const std::shared_ptr<State> &state) {
            state->expectedShader = signature(state->scene->getAttribute().shader);
            state->expectedView = viewSignature(*state->scene);
        }
        std::string pngDataUrl(const cv::Mat &pixels) {
            std::vector<uchar> png;
            if (!cv::imencode(".png", pixels, png)) {
                throw std::runtime_error("Cannot encode the rendered image");
            }
            if (png.size() > 24 * 1024 * 1024 - 32) {
                throw std::runtime_error("Rendered PNG is too large for Local LLM (24 MiB limit)");
            }
            static constexpr char alphabet[] =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string encoded = "data:image/png;base64,";
            encoded.reserve(22 + ((png.size() + 2) / 3) * 4);
            for (size_t byteIndex = 0; byteIndex < png.size(); byteIndex += 3) {
                const uint32_t packedBytes =
                    (uint32_t(png[byteIndex]) << 16) |
                    (byteIndex + 1 < png.size() ? uint32_t(png[byteIndex + 1]) << 8 : 0) |
                    (byteIndex + 2 < png.size() ? png[byteIndex + 2] : 0);
                encoded += alphabet[(packedBytes >> 18) & 63];
                encoded += alphabet[(packedBytes >> 12) & 63];
                encoded += byteIndex + 1 < png.size() ? alphabet[(packedBytes >> 6) & 63] : '=';
                encoded += byteIndex + 2 < png.size() ? alphabet[packedBytes & 63] : '=';
            }
            return encoded;
        }
        std::string inputText(HWND field) {
            std::wstring text(GetWindowTextLengthW(field) + 1, 0);
            const int charactersRead = GetWindowTextW(field, text.data(), int(text.size()));
            text.resize(charactersRead);
            if (text.empty()) {
                return {};
            }

            const int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), charactersRead,
                                                  nullptr, 0, nullptr, nullptr);
            if (bytes == 0) {
                throw std::runtime_error("AI input contains invalid Unicode text.");
            }

            std::string utf8Text(bytes, 0);
            if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), charactersRead,
                                    utf8Text.data(), bytes, nullptr, nullptr) != bytes) {
                throw std::runtime_error("Cannot convert AI input to UTF-8.");
            }
            return utf8Text;
        }
        std::wstring normalizeOutputLineEndings(const std::wstring &text,
                                                bool previousWasCarriageReturn) {
            std::wstring normalized;
            for (const auto character : text) {
                if (character == L'\n' && !previousWasCarriageReturn) {
                    normalized += L'\r';
                }
                normalized += character;
                previousWasCarriageReturn = character == L'\r';
            }
            return normalized;
        }
        void output(const std::shared_ptr<State> &state, const std::string &text) {
            (state->viewingZoom ? state->explorationLog : state->appearanceLog) = text;
            if (text == state->shown) {
                return;
            }
            const bool append = text.starts_with(state->shown);
            const auto changedText = append ? text.substr(state->shown.size()) : text;
            const auto wideText = UiLanguage::utf8(changedText);
            const bool previousWasCarriageReturn =
                append && !state->shown.empty() && state->shown.back() == '\r';
            const auto normalized = normalizeOutputLineEndings(wideText, previousWasCarriageReturn);
            const HWND field = state->output;
            SCROLLINFO scroll{sizeof(SCROLLINFO), SIF_ALL};
            GetScrollInfo(field, SB_VERT, &scroll);
            DWORD start = 0, end = 0;
            SendMessageW(field, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
            const bool follow =
                state->shown.empty() || (scroll.nPos + int(scroll.nPage) >= scroll.nMax && start == end);
            const auto first = SendMessageW(field, EM_GETFIRSTVISIBLELINE, 0, 0);
            const bool visible = IsWindowVisible(field);
            if (visible) {
                SendMessageW(field, WM_SETREDRAW, FALSE, 0);
            }
            if (append) {
                SendMessageW(field, EM_SETSEL, GetWindowTextLengthW(field), -1);
                SendMessageW(field, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(normalized.c_str()));
            } else {
                SetWindowTextW(field, normalized.c_str());
            }
            if (follow) {
                SendMessageW(field, EM_SETSEL, GetWindowTextLengthW(field), -1);
                SendMessageW(field, EM_LINESCROLL, 0, SendMessageW(field, EM_GETLINECOUNT, 0, 0));
            } else {
                const auto length = DWORD(GetWindowTextLengthW(field));
                SendMessageW(field, EM_SETSEL, std::min(start, length), std::min(end, length));
                SendMessageW(field, EM_LINESCROLL, 0,
                             first - SendMessageW(field, EM_GETFIRSTVISIBLELINE, 0, 0));
            }
            state->shown = text;
            if (visible) {
                SendMessageW(field, WM_SETREDRAW, TRUE, 0);
                RedrawWindow(field, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE);
            }
        }
        bool unchanged(const std::shared_ptr<State> &state) {
            return signature(state->scene->getAttribute().shader) == state->expectedShader &&
                   viewSignature(*state->scene) == state->expectedView &&
                   (!state->revisionTracked || state->scene->getPreviewRevision() == state->capturedRevision);
        }
        void busy(const std::shared_ptr<State> &state, bool active) {
            for (auto control :
                 {state->generate, state->input, state->errorLimit, state->autoRefine, state->refinementLimit,
                  state->startZoom, state->locate, state->appearanceTab, state->exploreTab, state->zoomInput,
                  state->zoomLimit, state->zoomErrors}) {
                EnableWindow(control, !active);
            }
            EnableWindow(state->zoomFactor, !active || state->exploring);
            const bool locateEnabled =
                !active && SendMessageW(state->locate, BM_GETCHECK, 0, 0) == BST_CHECKED;
            EnableWindow(state->retryLocate, locateEnabled);
            EnableWindow(state->repeatExplore, locateEnabled);
            EnableWindow(state->retryDecrement,
                         locateEnabled && SendMessageW(state->retryLocate, BM_GETCHECK, 0, 0) == BST_CHECKED);
            EnableWindow(state->cancel, active);
            EnableWindow(state->undo, !active && state->before.has_value());
        }
        void finishRefinement(const std::shared_ptr<State> &state, const std::string &reason,
                              bool restoreBest = true) {
            if (state->refining && restoreBest && state->best && unchanged(state)) {
                state->scene->getAttribute().shader = *state->best;
                state->scene->getRequests().requestShader();
                state->appliedSignature = signature(*state->best);
                state->expectedShader = state->appliedSignature;
            }
            state->refining = false;
            state->exploring = false;
            state->capturePending = false;
            state->ready.reset();
            state->completed = true;
            state->locateNext = false;
            state->verifyZoom = false;
            state->beforeZoom.reset();
            state->candidateImages.clear();
            EnableWindow(state->apply, FALSE);
            busy(state, false);
            output(state, state->sessionLog + "\r\n" + reason);
        }
        void queueCapture(const std::shared_ptr<State> &state) {
            state->revisionTracked = false;
            state->capturePending = true;
            state->captureStarted = std::chrono::steady_clock::now();
            busy(state, true);
            output(state, state->sessionLog + "\r\nWaiting for the rendered image...");
        }
        void startRequest(const std::shared_ptr<State> &state, const std::string &image) {
            if (state->sessionLog.size() > 200000) {
                state->sessionLog =
                    "[Earlier log omitted]\r\n" + state->sessionLog.substr(state->sessionLog.size() - 100000);
            }
            const auto original = state->scene->getAttribute().shader;
            const auto instruction = state->instruction;
            const auto history = state->history;
            const auto connection = state->connection;
            auto job = std::make_shared<Job>();
            state->completed = false;
            state->statistics = {};
            state->job = job;
            try {
                state->ready.reset();
                EnableWindow(state->apply, FALSE);
                busy(state, true);
                if (state->exploring) {
                    job->append(state->sessionLog + "\r\nAI zoom: " + std::to_string(state->rounds) + " / " +
                                std::to_string(state->maxRounds));
                    const auto evidence = state->iterationEvidence;
                    const auto crops = state->candidateImages;
                    std::thread([job, instruction, connection, image, evidence, crops] {
                        try {
                            auto runtime = connection;
                            const auto info = LocalAiSettings::serverInfo(runtime, job->cancel);
                            runtime["runtime_context"] = info.at("context");
                            runtime["runtime_llama"] = info.at("llama");
                            job->destination = LocalAiSettings::chooseZoomTarget(
                                instruction, image, runtime, job->cancel,
                                [job](const auto &s) { job->append(s); }, [job](const auto &s) { job->token(s); },
                                [job](const auto &s) { job->token(s, true); },
                                [job](const auto &stats) {
                                    std::lock_guard lock(job->mutex);
                                    job->statistics = stats;
                                },
                                {}, evidence, crops);
                        } catch (const std::exception &e) {
                            job->error = e.what();
                        }
                        job->done = true;
                    }).detach();
                    return;
                }
                job->append(state->sessionLog + "\r\n" +
                            (state->vision ? "Image evaluation " + std::to_string(state->rounds) + " / " +
                                                 std::to_string(state->maxRounds)
                                           : "Generating settings"));
                std::thread([job, original, instruction, connection, image, history]() mutable {
                    try {
                        auto runtime = connection;
                        const auto info = LocalAiSettings::serverInfo(runtime, job->cancel);
                        runtime["runtime_context"] = info.at("context");
                        runtime["runtime_llama"] = info.at("llama");
                        job->result = LocalAiSettings::generate(
                            original, instruction, runtime, job->cancel,
                            [job](const auto &text) { job->append("\r\n" + text); }, {},
                            [job](const auto &text) { job->token(text); },
                            [job](const auto &text) { job->token(text, true); },
                            [job](const auto &stats) {
                                std::lock_guard lock(job->mutex);
                                job->statistics = stats;
                            },
                            image, history);
                    } catch (const std::exception &e) {
                        job->error = e.what();
                    }
                    job->done = true;
                }).detach();
            } catch (...) {
                state->job.reset();
                throw;
            }
        }
        void applyProposal(const std::shared_ptr<State> &state) {
            if (!unchanged(state)) {
                throw std::runtime_error(
                    "The view or appearance changed. Generate again for the current image.");
            }
            auto &current = state->scene->getAttribute().shader;
            auto next = LocalAiSettings::apply(current, state->ready->patch);
            if (next.slope.lustreRelief && next.slope.reliefZoomReference < 0) {
                next.slope.reliefZoomReference = state->scene->getAttribute().fractal.logZoom;
            }
            if (signature(next) == signature(current)) {
                throw std::runtime_error("No further setting change was proposed.");
            }
            if (!state->refining) {
                state->before = current;
            }
            state->history += "\nApplied: " + state->ready->patch.dump();
            current = std::move(next);
            state->appliedSignature = signature(current);
            state->expectedShader = state->appliedSignature;
            state->scene->getRequests().requestShader();
            state->ready.reset();
            EnableWindow(state->apply, FALSE);
            if (state->vision) {
                state->refining = true;
                ++state->rounds;
                queueCapture(state);
            } else {
                EnableWindow(state->undo, TRUE);
                output(state, "Settings applied.");
            }
        }

        struct BufferedText {
            workspace::PanelBackBuffer buffer;
            bool edit = false;
            static LRESULT CALLBACK procedure(HWND w, UINT m, WPARAM wp, LPARAM lp, UINT_PTR id,
                                              DWORD_PTR data) {
                auto *self = reinterpret_cast<BufferedText *>(data);
                if (m == WM_ERASEBKGND) {
                    return 1;
                }
                if (m == WM_PAINT) {
                    PAINTSTRUCT paint{};
                    const auto target = BeginPaint(w, &paint);
                    try {
                        RECT r{};
                        GetClientRect(w, &r);
                        const auto memory = self->buffer.begin(target, r.right, r.bottom);
                        const auto dc = memory ? memory : target;
                        const auto theme = workspace::WorkspaceTheme::current();
                        workspace::PanelDrawing::fill(dc, r, self->edit ? theme.field : theme.background);
                        if (self->edit) {
                            DefSubclassProc(w, WM_PRINTCLIENT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT);
                        } else {
                            std::wstring text(GetWindowTextLengthW(w) + 1, 0);
                            GetWindowTextW(w, text.data(), int(text.size()));
                            text.resize(wcslen(text.c_str()));
                            const auto font = reinterpret_cast<HFONT>(SendMessageW(w, WM_GETFONT, 0, 0));
                            const auto old = SelectObject(dc, font);
                            SetTextColor(dc, theme.foreground);
                            SetBkMode(dc, TRANSPARENT);
                            DrawTextW(dc, text.c_str(), int(text.size()), &r,
                                      DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);
                            SelectObject(dc, old);
                        }
                        if (memory) {
                            self->buffer.present(target);
                        }
                    } catch (...) {
                        EndPaint(w, &paint);
                        throw;
                    }
                    EndPaint(w, &paint);
                    return 0;
                }
                if (m == WM_NCDESTROY) {
                    RemoveWindowSubclass(w, procedure, id);
                    delete self;
                }
                return DefSubclassProc(w, m, wp, lp);
            }
            static void attach(HWND window, bool edit = false) {
                SetWindowSubclass(window, procedure, 8831,
                                  reinterpret_cast<DWORD_PTR>(new BufferedText{{}, edit}));
            }
        };

        struct View {
            HWND connectionButton{};
            HWND settingsHeading{};
            HWND afterHeading{};
            HWND host{};
            HWND model{};
            HWND context{};
            HWND speed{};
            HWND promptLabel{};
            HWND resultLabel{};
            HWND errorLabel{};
            HWND refinementLabel{};
            HWND zoomLabel{};
            HWND zoomPromptLabel{};
            HWND zoomLimitLabel{};
            HWND decrementLabel{};
            bool zoomTab = false;
            HFONT font{};
            workspace::WorkspaceTheme theme = workspace::WorkspaceTheme::current();
            workspace::WorkspaceButton::Context drawing{&theme, nullptr, 1};
            std::shared_ptr<State> state;
            std::vector<std::pair<HWND, std::function<void()>>> actions;
            int px(int n) const {
                return int(n * drawing.scale + .5f);
            }
            HWND control(const wchar_t *type, const wchar_t *title, DWORD style) {
                const auto label = UiLanguage::text(title);
                auto w = CreateWindowExW(0, type, label.c_str(), WS_CHILD | WS_VISIBLE | style, 0, 0, 1, 1,
                                         host, nullptr, GetModuleHandleW(nullptr), nullptr);
                SendMessageW(w, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
                return w;
            }
            View(SettingsWindow &frame, std::shared_ptr<State> value) : state(std::move(value)) {
                drawing.scale = float(UiDpi::forWindow(frame.getWindow())) / 96.f;
                host = frame.registerOwnerDrawnPanel(px(824), [](HDC dc, const RECT &rect) {
                    workspace::PanelDrawing::fill(dc, rect, workspace::WorkspaceTheme::current().background);
                });
                SetWindowLongPtrW(host, GWL_EXSTYLE,
                                  GetWindowLongPtrW(host, GWL_EXSTYLE) | WS_EX_CONTROLPARENT);
                SetWindowLongPtrW(host, GWL_STYLE, GetWindowLongPtrW(host, GWL_STYLE) | WS_CLIPCHILDREN);
                font = CreateFontW(-px(14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH,
                                   UiLanguage::fontFace());
                drawing.font = font;
                model = control(L"STATIC", L"Checking local connection...", 0);
                context = control(L"STATIC", L"Context: checking...", 0);
                speed = control(L"STATIC", L"Generation: -- tokens/s", 0);
                for (const auto label : {model, context, speed}) {
                    BufferedText::attach(label);
                }
                promptLabel = control(L"STATIC", L"Desired appearance", 0);
                errorLabel = control(L"STATIC", L"Error limit (1-100)", 0);
                refinementLabel = control(L"STATIC", L"Max changes", 0);
                zoomLabel = control(L"STATIC", L"Zoom per AI step (1 < factor <= 100)", 0);
                zoomPromptLabel = control(L"STATIC", L"Features to explore", 0);
                zoomLimitLabel = control(L"STATIC", L"Exploration steps (1-1000)", 0);
                decrementLabel = control(L"STATIC", L"Retry log zoom decrease", 0);
                settingsHeading = control(L"STATIC", L"Exploration settings", 0);
                afterHeading = control(L"STATIC", L"After exploration", 0);
                resultLabel = control(L"STATIC", L"Result / generation log", 0);
                SetWindowSubclass(host, procedure, 8830, reinterpret_cast<DWORD_PTR>(this));
            }
            ~View() {
                DeleteObject(font);
            }
            HWND registerPrimaryButton(const std::wstring &title, std::function<void()> action) {
                auto w = control(L"BUTTON", title.c_str(), WS_TABSTOP | BS_PUSHBUTTON);
                workspace::WorkspaceButton::attach(w, title == L"Generate settings");
                actions.emplace_back(w, std::move(action));
                return w;
            }
            void layout() {
                if (!state->locate) {
                    return;
                }
                RECT r{};
                GetClientRect(host, &r);
                const int width = std::max(px(200), int(r.right) - px(24)), gap = px(12), button = px(34),
                          third = (width - gap * 2) / 3;
                auto place = [&](HWND w, int x, int y, int width, int height) {
                    MoveWindow(w, x + px(12), y + px(8), std::max(1, width), std::max(1, height), TRUE);
                };
                place(state->appearanceTab, 0, 0, (width - gap) / 2, button);
                place(state->exploreTab, (width + gap) / 2, 0, (width - gap) / 2, button);
                place(state->newChat, 0, px(50), px(160), button);
                place(connectionButton, width - px(200), px(50), px(200), button);
                ShowWindow(model, SW_HIDE);
                ShowWindow(context, SW_SHOW);
                ShowWindow(speed, SW_SHOW);
                const int appearanceVisibility = zoomTab ? SW_HIDE : SW_SHOW;
                const int zoomVisibility = zoomTab ? SW_SHOW : SW_HIDE;
                for (auto w :
                     {promptLabel, state->input, state->autoRefine, refinementLabel, state->refinementLimit,
                      state->generate, state->apply, state->undo, state->errorLimit}) {
                    ShowWindow(w, appearanceVisibility);
                }
                for (auto w :
                     {zoomPromptLabel, state->zoomInput, zoomLabel, state->zoomFactor, zoomLimitLabel,
                      state->zoomLimit, state->startZoom, state->locate, state->zoomErrors,
                      state->retryLocate, state->repeatExplore, state->retryDecrement, decrementLabel}) {
                    ShowWindow(w, zoomVisibility);
                }
                const HWND activeTab = zoomTab ? state->exploreTab : state->appearanceTab;
                const HWND inactiveTab = zoomTab ? state->appearanceTab : state->exploreTab;
                SetPropW(activeTab, L"RFF.Button.Primary", reinterpret_cast<HANDLE>(1));
                RemovePropW(inactiveTab, L"RFF.Button.Primary");
                InvalidateRect(state->appearanceTab, nullptr, FALSE);
                InvalidateRect(state->exploreTab, nullptr, FALSE);
                const int field = px(88), fieldX = width - field, indent = px(24);
                for (auto w : {settingsHeading, afterHeading}) {
                    ShowWindow(w, zoomVisibility);
                }
                place(zoomTab ? zoomPromptLabel : promptLabel, 0, px(102), width, px(22));
                place(zoomTab ? state->zoomInput : state->input, 0, px(130), width, px(64));
                place(settingsHeading, 0, px(216), width, px(22));
                auto row = [&](HWND label, HWND edit, int y, int inset = 0) {
                    place(label, inset, px(y + 5), fieldX - gap - inset, px(24));
                    place(edit, fieldX, px(y), field, px(30));
                };
                row(zoomLabel, state->zoomFactor, 246);
                row(zoomLimitLabel, state->zoomLimit, 288);
                row(errorLabel, zoomTab ? state->zoomErrors : state->errorLimit, zoomTab ? 330 : 218);
                place(state->autoRefine, 0, px(266), width, button);
                row(refinementLabel, state->refinementLimit, 310, indent);
                place(afterHeading, 0, px(384), width, px(22));
                place(state->locate, 0, px(414), width, px(30));
                place(state->retryLocate, indent, px(454), width - indent, px(30));
                place(state->repeatExplore, indent, px(494), width - indent, px(30));
                row(decrementLabel, state->retryDecrement, 540, indent);
                const int actionY = zoomTab ? 588 : 364, logY = actionY + 80;
                place(zoomTab ? state->startZoom : state->generate, 0, px(actionY), width - px(128) - gap,
                      button);
                place(state->cancel, width - px(128), px(actionY), px(128), button);
                place(resultLabel, 0, px(actionY + 50), width, px(22));
                const int footer = std::max(px(logY + 90), int(r.bottom) - button - px(20));
                place(state->output, 0, px(logY), width,
                      footer - px(logY) - gap - (zoomTab ? 0 : button + gap));
                place(state->apply, 0, footer - button - gap, (width - gap) / 2, button);
                place(state->undo, (width + gap) / 2, footer - button - gap, (width - gap) / 2, button);
                place(context, 0, footer, width - px(140) - gap, px(44));
                place(speed, width - px(140), footer, px(140), px(24));
                std::vector<HWND> order{state->appearanceTab, state->exploreTab, state->newChat,
                                        connectionButton, zoomTab ? state->zoomInput : state->input};
                order.reserve(24);
                if (zoomTab) {
                    order.insert(order.end(), {state->zoomFactor, state->zoomLimit, state->zoomErrors,
                                               state->locate, state->retryLocate, state->repeatExplore,
                                               state->retryDecrement, state->startZoom});
                } else {
                    order.insert(order.end(), {state->errorLimit, state->autoRefine, state->refinementLimit,
                                               state->generate});
                }
                order.insert(order.end(), {state->cancel, state->output, state->apply, state->undo});
                HWND previous = HWND_TOP;
                for (auto control : order) {
                    SetWindowPos(control, previous, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    previous = control;
                }
            }
            static LRESULT CALLBACK checkboxProcedure(HWND w, UINT m, WPARAM wp, LPARAM lp, UINT_PTR id,
                                                      DWORD_PTR data) {
                auto *self = reinterpret_cast<View *>(data);
                if (m == WM_ERASEBKGND) {
                    return 1;
                }
                if (m == WM_PAINT || m == WM_PRINTCLIENT) {
                    PAINTSTRUCT paint{};
                    const auto target = m == WM_PAINT ? BeginPaint(w, &paint) : reinterpret_cast<HDC>(wp);
                    RECT rect{};
                    GetClientRect(w, &rect);
                    workspace::PanelBackBuffer buffer;
                    try {
                        auto dc = buffer.begin(target, rect.right, rect.bottom);
                        if (!dc) {
                            dc = target;
                        }
                        const auto theme = workspace::WorkspaceTheme::current();
                        const bool enabled = IsWindowEnabled(w),
                                   checked = SendMessageW(w, BM_GETCHECK, 0, 0) == BST_CHECKED;
                        workspace::PanelDrawing::fill(dc, rect, theme.background);
                        const int size = self->px(16), left = self->px(2), top = (rect.bottom - size) / 2;
                        RECT box{left, top, left + size, top + size};
                        const auto &colors = settingsTheme();
                        const auto face = checked
                                              ? (enabled ? colors.checkboxChecked : colors.checkboxDisabledChecked)
                                              : theme.field;
                        workspace::PanelDrawing::rounded(dc, box, face, colors.checkboxBorder, self->px(3));
                        if (checked) {
                            const auto pen = CreatePen(PS_SOLID, std::max(1, self->px(2)),
                                                       colors.checkboxMark);
                            const auto old = SelectObject(dc, pen);
                            MoveToEx(dc, left + self->px(3), top + self->px(8), nullptr);
                            LineTo(dc, left + self->px(7), top + self->px(12));
                            LineTo(dc, left + self->px(13), top + self->px(4));
                            SelectObject(dc, old);
                            DeleteObject(pen);
                        }
                        std::wstring label(GetWindowTextLengthW(w) + 1, L'\0');
                        GetWindowTextW(w, label.data(), int(label.size()));
                        label.resize(wcslen(label.c_str()));
                        RECT text = rect;
                        text.left = box.right + self->px(9);
                        workspace::PanelDrawing::text(dc, label, text,
                                                      enabled ? theme.foreground : theme.secondary, self->font);
                        if (GetFocus() == w && !(SendMessageW(w, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS)) {
                            RECT focus = rect;
                            InflateRect(&focus, -1, -1);
                            DrawFocusRect(dc, &focus);
                        }
                        if (dc != target) {
                            buffer.present(target);
                        }
                    } catch (...) {
                        if (m == WM_PAINT) {
                            EndPaint(w, &paint);
                        }
                        throw;
                    }
                    if (m == WM_PAINT) {
                        EndPaint(w, &paint);
                    }
                    return 0;
                }
                const bool visualChange = m == BM_SETCHECK || m == BM_SETSTATE || m == WM_ENABLE ||
                                          m == WM_SETFOCUS || m == WM_KILLFOCUS || m == WM_UPDATEUISTATE ||
                                          m == WM_SETTEXT;
                const bool suspend = visualChange && (GetWindowLongPtrW(w, GWL_STYLE) & WS_VISIBLE) != 0;
                if (suspend) {
                    DefSubclassProc(w, WM_SETREDRAW, FALSE, 0);
                }
                const auto result = DefSubclassProc(w, m, wp, lp);
                if (suspend) {
                    DefSubclassProc(w, WM_SETREDRAW, TRUE, 0);
                }
                if (visualChange) {
                    InvalidateRect(w, nullptr, FALSE);
                }
                if (m == WM_NCDESTROY) {
                    RemoveWindowSubclass(w, checkboxProcedure, id);
                }
                return result;
            }
            static LRESULT CALLBACK procedure(HWND w, UINT m, WPARAM wp, LPARAM lp, UINT_PTR id,
                                              DWORD_PTR data) {
                auto *self = reinterpret_cast<View *>(data);
                if (m == WM_SIZE) {
                    self->layout();
                }
                if (m == WM_COMMAND && HIWORD(wp) == BN_CLICKED &&
                    (reinterpret_cast<HWND>(lp) == self->state->locate ||
                     reinterpret_cast<HWND>(lp) == self->state->retryLocate)) {
                    busy(self->state, false);
                }
                if (m == WM_COMMAND && HIWORD(wp) == BN_CLICKED) {
                    for (const auto &[control, invoke] : self->actions) {
                        if (control == reinterpret_cast<HWND>(lp)) {
                            invoke();
                            return 0;
                        }
                    }
                }
                if (m == WM_DRAWITEM) {
                    workspace::WorkspaceButton::draw(*reinterpret_cast<DRAWITEMSTRUCT *>(lp), self->drawing);
                    return TRUE;
                }
                if (m == WM_CTLCOLOREDIT || m == WM_CTLCOLORSTATIC) {
                    self->theme = workspace::WorkspaceTheme::current();
                    auto dc = reinterpret_cast<HDC>(wp);
                    const bool edit = reinterpret_cast<HWND>(lp) == self->state->input ||
                                      reinterpret_cast<HWND>(lp) == self->state->errorLimit ||
                                      reinterpret_cast<HWND>(lp) == self->state->refinementLimit ||
                                      reinterpret_cast<HWND>(lp) == self->state->zoomFactor ||
                                      reinterpret_cast<HWND>(lp) == self->state->zoomInput ||
                                      reinterpret_cast<HWND>(lp) == self->state->zoomLimit ||
                                      reinterpret_cast<HWND>(lp) == self->state->zoomErrors ||
                                      reinterpret_cast<HWND>(lp) == self->state->retryDecrement ||
                                      reinterpret_cast<HWND>(lp) == self->state->output;
                    SetTextColor(dc, self->theme.foreground);
                    SetBkColor(dc, edit ? self->theme.field : self->theme.background);
                    SetDCBrushColor(dc, edit ? self->theme.field : self->theme.background);
                    return reinterpret_cast<LRESULT>(GetStockObject(DC_BRUSH));
                }
                if (m == WM_NCDESTROY) {
                    RemoveWindowSubclass(w, procedure, id);
                    delete self;
                }
                return DefSubclassProc(w, m, wp, lp);
            }
        };

        HWND editor(View &view, bool readonly, const wchar_t *accessibleName) {
            auto edit = view.control(L"EDIT", L"",
                                     WS_TABSTOP | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL |
                                         ES_WANTRETURN | (readonly ? ES_READONLY : 0));
            if (readonly) {
                BufferedText::attach(edit, true);
            }
            SendMessageW(edit, EM_SETLIMITTEXT, readonly ? 2000000 : 8000, 0);
            SendMessageW(edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN,
                         MAKELPARAM(view.px(8), view.px(8)));
            IAccPropServices *access = nullptr;
            if (SUCCEEDED(CoCreateInstance(CLSID_AccPropServices, nullptr, CLSCTX_INPROC_SERVER,
                                           IID_IAccPropServices, reinterpret_cast<void **>(&access)))) {
                access->SetHwndPropStr(edit, OBJID_CLIENT, CHILDID_SELF, PROPID_ACC_NAME, accessibleName);
                access->Release();
            }
            return edit;
        }
        void caption(HWND window, const std::string &value) {
            const auto text = UiLanguage::utf8(value);
            std::wstring old(GetWindowTextLengthW(window) + 1, 0);
            GetWindowTextW(window, old.data(), int(old.size()));
            old.resize(wcslen(old.c_str()));
            if (old != text) {
                const bool visible = IsWindowVisible(window);
                if (visible) {
                    SendMessageW(window, WM_SETREDRAW, FALSE, 0);
                }
                SetWindowTextW(window, text.c_str());
                if (visible) {
                    SendMessageW(window, WM_SETREDRAW, TRUE, 0);
                    InvalidateRect(window, nullptr, FALSE);
                }
            }
        }
        void refreshMetrics(const std::shared_ptr<State> &state) {
            if (++state->metadataTicks >= 25 && !state->job && !state->metadataBusy.exchange(true)) {
                state->metadataTicks = 0;
                try {
                    std::thread([state] {
                        try {
                            const auto connection = LocalAiSettings::readConnection();
                            auto info = LocalAiSettings::serverInfo(connection, state->closed);
                            std::lock_guard lock(state->metadataMutex);
                            state->info = info;
                            state->modelName = connection.at("model").get<std::string>();
                        } catch (const std::exception &) {
                            std::lock_guard lock(state->metadataMutex);
                            state->info = {{"context", -1}, {"llama", false}};
                            state->modelName = "Connection unavailable";
                        }
                        state->metadataBusy = false;
                    }).detach();
                } catch (const std::exception &) {
                    state->metadataBusy = false;
                }
            }
            auto stats = state->statistics;
            bool invalidUnicodeInput = false;
            if (!state->job && !state->completed) {
                {
                    std::lock_guard lock(state->metadataMutex);
                    stats.context = state->info.value("context", int64_t(-1));
                }
                try {
                    stats.prompt = int64_t((state->promptBytes + inputText(state->input).size() + 35) / 4);
                } catch (const std::runtime_error &) {
                    stats.prompt = -1;
                    invalidUnicodeInput = true;
                }
                stats.generated = 0;
            }
            if (stats.context < 0) {
                std::lock_guard lock(state->metadataMutex);
                stats.context = state->info.value("context", int64_t(-1));
            }
            const auto count = [](int64_t n) { return n < 0 ? std::string("--") : std::to_string(n); };
            const auto mark = stats.estimated ? "~" : "";
            const int64_t used = stats.prompt < 0 ? -1 : stats.prompt + std::max<int64_t>(0, stats.generated);
            std::string text;
            if (invalidUnicodeInput) {
                text = "Context: invalid Unicode input";
            } else {
                std::string usedText = "calculating";
                if (used >= 0) {
                    usedText = std::string(mark) + count(used);
                }
                text = "Context: " + usedText + " / " + count(stats.context) + " tokens";
            }
            text += "\r\nGenerated: " + std::string(mark) + count(stats.generated) + " tokens";
            caption(state->context, text);
            std::ostringstream rate;
            if (stats.tokensPerSecond >= 0 && std::isfinite(stats.tokensPerSecond)) {
                rate << mark << std::fixed << std::setprecision(1) << stats.tokensPerSecond;
            } else {
                rate << "--";
            }
            rate << " tok/s";
            caption(state->speed, rate.str());
            std::string modelName;
            {
                std::lock_guard lock(state->metadataMutex);
                modelName = state->modelName;
            }
            caption(state->model, modelName.empty() ? "Checking local connection..." : "Model: " + modelName);
        }
        void newChat(const std::shared_ptr<State> &state) {
            state->exploring = false;
            state->capturePending = false;
            state->refining = false;
            state->sessionLog.clear();
            state->history.clear();
            state->best.reset();
            busy(state, false);
            state->ready.reset();
            state->completed = false;
            state->statistics = {};
            state->metadataTicks = 25;
            SetWindowTextW(state->input, L"");
            EnableWindow(state->apply, FALSE);
            SetWindowTextW(state->zoomInput, L"");
            output(state, "Enter a description, generate, then apply the proposed settings.");
            SetFocus(state->input);
        }
        void finishLocateJob(const std::shared_ptr<State> &state) {
            auto result = state->locating;
            state->locating.reset();
            if (state->newChatPending) {
                state->newChatPending = false;
                finishRefinement(state, "Cancelled.");
                newChat(state);
                return;
            }
            if (result->cancel) {
                finishRefinement(state, "Minibrot search cancelled. Current location retained.");
                return;
            }
            try {
                if (!unchanged(state)) {
                    throw std::runtime_error("View changed during minibrot search; result discarded.");
                }
                if (!result->error.empty() || !result->center) {
                    const auto reason = result->error.empty()
                                            ? std::string("Minibrot center did not converge.")
                                            : result->error;
                    if (!state->retryLower) {
                        throw std::runtime_error(reason);
                    }
                    auto &fractal = state->scene->getAttribute().fractal;
                    fractal.logZoom = LocalAiSettings::retryLogZoom(
                        fractal.logZoom, inputText(state->retryDecrement), Constants::Fractal::ZOOM_MIN);
                    state->scene->getRequests().requestRecompute();
                    state->locateNext = true;
                    ++state->retries;
                    rememberCurrentView(state);
                    state->sessionLog += "\r\n" + reason + " Locate retry " + std::to_string(state->retries) +
                                         ": same center, log zoom " + std::to_string(fractal.logZoom) + ".";
                    queueCapture(state);
                    return;
                }
                auto &fractal = state->scene->getAttribute().fractal;
                fractal.center = *result->center;
                fractal.logZoom = result->zoom;
                state->scene->getRequests().requestRecompute();
                ++state->cycles;
                state->sessionLog +=
                    "\r\nLocated minibrot. Completed searches: " + std::to_string(state->cycles) + ".";
                if (state->repeatCycles) {
                    state->rounds = 0;
                    state->retries = 0;
                }
                rememberCurrentView(state);
                queueCapture(state);
            } catch (const std::exception &e) {
                finishRefinement(state, e.what());
            }
            return;
        }
        LRESULT CALLBACK tick(HWND w, UINT m, WPARAM wp, LPARAM lp, UINT_PTR id, DWORD_PTR data) {
            auto &state = *reinterpret_cast<std::shared_ptr<State> *>(data);
            if (m == WM_DESTROY && !state->closed.exchange(true)) {
                if (state->locating) {
                    state->locating->cancel = true;
                    state->scene->getState().interrupt();
                }
                if (state->refining && state->best && unchanged(state)) {
                    state->scene->getAttribute().shader = *state->best;
                    state->scene->getRequests().requestShader();
                }
                state->capturePending = false;
                KillTimer(w, 8821);
                if (state->job) {
                    state->job->cancel = true;
                }
                try {
                    std::thread([] {
                        try {
                            LocalAiSettings::stopConfiguredServer();
                        } catch (const std::exception &) {
                        }
                    }).detach();
                } catch (const std::exception &) {
                }
            }
            if (m == WM_TIMER && wp == 8821) {
                refreshMetrics(state);
            }
            if (m == WM_TIMER && wp == 8821 && state->locating && state->locating->done) {
                finishLocateJob(state);
                return 0;
            }
            if (m == WM_TIMER && wp == 8821 && state->capturePending && !state->job) {
                try {
                    if (!unchanged(state)) {
                        throw std::runtime_error(
                            "Stopped: the view or appearance changed outside Local LLM.");
                    }
                    if (state->scene->isImageBrowsing() || state->scene->getVideoGenerationActive() ||
                        state->scene->getVideoExportActive()) {
                        throw std::runtime_error("Local LLM image evaluation is unavailable during image "
                                                 "browsing or video rendering.");
                    }
                    if (std::chrono::steady_clock::now() - state->captureStarted > std::chrono::minutes(10)) {
                        throw std::runtime_error("Timed out waiting for a completed render.");
                    }
                    auto images = state->scene->renderComparison(state->scene->getAttribute().shader, 0.f);
                    if (!images.second.empty()) {
                        state->capturedRevision = state->scene->getPreviewRevision();
                        state->revisionTracked = true;
                        if (state->exploring) {
                            const auto [sample, limit] = state->scene->sampleIterationsForAi();
                            auto evidence = LocalAiSettings::analyzeIterations(sample, limit);
                            if (state->verifyZoom) {
                                state->verifyZoom = false;
                                if (!evidence["has_structure"].get<bool>()) {
                                    state->scene->getAttribute().fractal = *state->beforeZoom;
                                    state->beforeZoom.reset();
                                    state->scene->getRequests().requestRecompute();
                                    --state->rounds;
                                    state->locateNext = false;
                                    rememberCurrentView(state);
                                    state->revisionTracked = false;
                                    state->rejectedTargets.emplace_back(state->attemptedX, state->attemptedY);
                                    ++state->lostStructureRetries;
                                    state->sessionLog +=
                                        "\r\nIteration data shows no supported structure after zoom. "
                                        "Restored the previous center and log zoom.";
                                    if (state->lostStructureRetries >=
                                        LocalAiSettings::errorLimit(state->connection)) {
                                        throw std::runtime_error(
                                            "Structure-loss retry limit reached. Previous location retained; "
                                            "Locate Minibrot was not started.");
                                    }
                                    const double lower = LocalAiSettings::smallerExplorationZoom(
                                        std::min(state->attemptedFactor,
                                                 LocalAiSettings::zoomFactor(inputText(state->zoomFactor))));
                                    SetWindowTextW(state->zoomFactor,
                                                   UiLanguage::utf8(std::to_string(lower)).c_str());
                                    state->sessionLog += " Retry with another candidate at factor " +
                                                         std::to_string(lower) + ".";
                                    queueCapture(state);
                                    return 0;
                                }
                                state->beforeZoom.reset();
                                state->rejectedTargets.clear();
                                state->lostStructureRetries = 0;
                                state->sessionLog += "\r\nIteration structure retained after zoom.";
                            }
                            auto &candidates = evidence["candidates"];
                            for (auto it = candidates.begin(); it != candidates.end();) {
                                bool rejected = false;
                                for (const auto &[x, y] : state->rejectedTargets) {
                                    const double dx = (*it)["x"].get<double>() - x,
                                                 dy = (*it)["y"].get<double>() - y;
                                    if (dx * dx + dy * dy < 0.01) {
                                        rejected = true;
                                        break;
                                    }
                                }
                                if (rejected) {
                                    it = candidates.erase(it);
                                } else {
                                    ++it;
                                }
                            }
                            evidence["rejected_targets"] = LocalAiSettings::Json::array();
                            for (const auto &[x, y] : state->rejectedTargets) {
                                evidence["rejected_targets"].push_back({{"x", x}, {"y", y}});
                            }
                            state->iterationEvidence = std::move(evidence);
                            state->candidateImages.clear();
                            if (!state->locateNext && state->rounds < state->maxRounds) {
                                if (state->iterationEvidence["candidates"].empty()) {
                                    throw std::runtime_error(
                                        "No remaining numerically supported targets. Exploration stopped "
                                        "without guessing a coordinate.");
                                }
                                for (const auto &candidate : state->iterationEvidence["candidates"]) {
                                    const auto &box = candidate["crop"];
                                    const int x0 = int(box[0].get<double>() * images.second.cols),
                                              y0 = int(box[1].get<double>() * images.second.rows),
                                              x1 = std::min(
                                                  images.second.cols,
                                                  int(std::ceil(box[2].get<double>() * images.second.cols))),
                                              y1 = std::min(
                                                  images.second.rows,
                                                  int(std::ceil(box[3].get<double>() * images.second.rows)));
                                    state->candidateImages.push_back(pngDataUrl(images.second(
                                        cv::Rect(x0, y0, std::max(1, x1 - x0), std::max(1, y1 - y0)))));
                                }
                                state->sessionLog +=
                                    "\r\nIteration candidates: " +
                                    std::to_string(state->iterationEvidence["candidates"].size()) + ".";
                            }
                        }
                        if (state->exploring && state->locateNext) {
                            state->capturePending = false;
                            state->locateNext = false;
                            auto job = std::make_shared<LocateJob>();
                            state->locating = job;
                            auto *scene = state->scene;
                            const auto *reference = scene->getCurrentPerturbator();
                            if (!reference) {
                                state->locating.reset();
                                throw std::runtime_error(
                                    "No reference orbit is available for Locate Minibrot.");
                            }
                            output(state, state->sessionLog + "\r\nLocating minibrot...");
                            try {
                                scene->getState().createThread([job, scene,
                                                                reference](std::stop_token token) {
                                    try {
                                        const auto result = MandelbrotLocator::locateMinibrot(
                                            scene->getState(), reference, scene->getApproxTableCache(),
                                            [](uint64_t, int) {}, [](uint64_t, float) {}, [](float) {});
                                        if (token.stop_requested()) {
                                            job->cancel = true;
                                        }
                                        if (result && !job->cancel) {
                                            const auto &found = result->perturbator->getCalculationSettings();
                                            job->center = found.center;
                                            job->zoom =
                                                found.logZoom - MandelbrotLocator::MINIBROT_LOG_ZOOM_OFFSET;
                                        }
                                    } catch (const std::exception &e) {
                                        job->error = e.what();
                                    }
                                    job->done = true;
                                });
                            } catch (...) {
                                state->locating.reset();
                                throw;
                            }
                        } else if (state->exploring && state->rounds >= state->maxRounds) {
                            finishRefinement(state, "AI exploration complete. Current location retained.");
                        } else {
                            auto image = pngDataUrl(images.second);
                            state->capturePending = false;
                            startRequest(state, image);
                        }
                    }
                } catch (const std::exception &e) {
                    finishRefinement(state, e.what());
                }
            }
            if (m == WM_TIMER && wp == 8821 && state->job) {
                auto job = state->job;
                {
                    std::lock_guard lock(job->mutex);
                    state->statistics = job->statistics;
                    if (!job->cancel && state->shown != job->log) {
                        output(state, job->log);
                    }
                }
                if (job->done.load()) {
                    state->job.reset();
                    state->sessionLog = job->log;
                    state->completed = true;
                    if (state->newChatPending) {
                        state->newChatPending = false;
                        finishRefinement(state, "Cancelled.");
                        newChat(state);
                        return 0;
                    }
                    if (job->cancel) {
                        finishRefinement(state, state->exploring
                                                    ? "AI zoom stopped. Current location retained."
                                                    : "Cancelled. The best evaluated appearance is retained "
                                                      "when the view is unchanged.");
                        return 0;
                    }
                    if (!job->error.empty()) {
                        finishRefinement(state, job->error);
                        return 0;
                    }
                    try {
                        if (signature(state->scene->getAttribute().shader) != state->expectedShader) {
                            throw std::runtime_error("Stopped: appearance settings changed while waiting for "
                                                     "AI. Generate again for the current appearance.");
                        }
                        if (viewSignature(*state->scene) != state->expectedView) {
                            throw std::runtime_error("Stopped: view or calculation settings changed while "
                                                     "waiting for AI. Generate again for the current view.");
                        }
                        if (state->revisionTracked &&
                            state->scene->getPreviewRevision() != state->capturedRevision) {
                            const auto revision = state->scene->getPreviewRevision();
                            state->sessionLog +=
                                "\r\nRender data updated while settings stayed unchanged (revision " +
                                std::to_string(state->capturedRevision) + " -> " + std::to_string(revision) +
                                "). Discarded the stale AI response.";
                            if (++state->staleImageRetries >=
                                LocalAiSettings::errorLimit(state->connection)) {
                                throw std::runtime_error(
                                    "Render data keeps changing. Image refresh retry limit reached; wait for "
                                    "rendering to settle and try again.");
                            }
                            state->sessionLog += " Capturing the latest image for another evaluation.";
                            queueCapture(state);
                            return 0;
                        }
                        state->staleImageRetries = 0;
                        if (state->exploring && job->destination) {
                            const auto &target = *job->destination;
                            if (target.stop) {
                                if (state->useMinibrot) {
                                    state->rounds = state->maxRounds;
                                    state->locateNext = true;
                                    state->sessionLog += "\r\nAI exploration ended: " + target.summary;
                                    queueCapture(state);
                                } else {
                                    finishRefinement(state, "AI exploration stopped: " + target.summary);
                                }
                                return 0;
                            }
                            const double factor = LocalAiSettings::zoomFactor(inputText(state->zoomFactor));
                            state->beforeZoom = state->scene->getAttribute().fractal;
                            state->scene->zoomToImagePoint(target.x, target.y, factor);
                            ++state->rounds;
                            state->verifyZoom = true;
                            state->attemptedX = target.x;
                            state->attemptedY = target.y;
                            state->attemptedFactor = factor;
                            state->sessionLog += "\r\nZoom " + std::to_string(state->rounds) +
                                                 ": x=" + std::to_string(target.x) +
                                                 ", y=" + std::to_string(target.y) +
                                                 ", factor=" + std::to_string(factor) + ". " + target.summary;
                            rememberCurrentView(state);
                            state->locateNext = state->useMinibrot && state->rounds >= state->maxRounds;
                            queueCapture(state);
                            return 0;
                        }
                        if (job->result) {
                            state->ready = job->result;
                            if (state->vision) {
                                const auto &result = *job->result;
                                state->history += "\nEvaluation " + std::to_string(state->rounds) +
                                                  ": score=" + std::to_string(result.score) + " " +
                                                  result.summary;
                                const bool improved = result.score > state->bestScore;
                                if (improved) {
                                    state->bestScore = result.score;
                                    state->best = state->scene->getAttribute().shader;
                                }
                                if (state->refining &&
                                    (!improved || result.satisfied || result.patch["changes"].empty() ||
                                     state->rounds >= state->maxRounds)) {
                                    const char *stopReason;
                                    if (!improved) {
                                        stopReason = "No score improvement.";
                                    } else if (result.satisfied) {
                                        stopReason = "Goal satisfied.";
                                    } else if (state->rounds >= state->maxRounds) {
                                        stopReason = "Refinement limit reached.";
                                    } else {
                                        stopReason = "No further change proposed.";
                                    }
                                    finishRefinement(state,
                                                     "Finished after " + std::to_string(state->rounds) +
                                                         " applied changes. Best model score: " +
                                                         std::to_string(state->bestScore) + " / 100. " +
                                                         stopReason);
                                    return 0;
                                }
                                if (state->refining) {
                                    applyProposal(state);
                                    return 0;
                                }
                                if (result.patch["changes"].empty()) {
                                    finishRefinement(state, "No setting change needed. " + result.summary);
                                    return 0;
                                }
                                if (state->autoStart) {
                                    applyProposal(state);
                                    return 0;
                                }
                            }
                            std::string detail = job->result->summary + "\r\n";
                            const auto catalog =
                                LocalAiSettings::catalog(state->scene->getAttribute().shader);
                            for (const auto &[key, value] : job->result->patch["changes"].items()) {
                                detail += "\r\n" + catalog.at(key).at("description").get<std::string>() +
                                          ": " + value.dump();
                            }
                            output(
                                state,
                                detail +
                                    (state->vision
                                         ? "\r\nApply and refine starts automatic image evaluation (up to " +
                                               std::to_string(state->maxRounds) + " changes)."
                                         : "") +
                                    "\r\n\r\nGeneration log:\r\n" + job->log);
                            EnableWindow(state->apply, TRUE);
                        }
                        busy(state, false);
                    } catch (const std::exception &e) {
                        finishRefinement(state, e.what());
                    }
                }
            }
            if (m == WM_NCDESTROY) {
                state->closed = true;
                if (state->job) {
                    state->job->cancel = true;
                }
                KillTimer(w, 8821);
                RemoveWindowSubclass(w, tick, id);
                delete reinterpret_cast<std::shared_ptr<State> *>(data);
            }
            return DefSubclassProc(w, m, wp, lp);
        }
    } // namespace
    void LocalAiWindow::open(SettingsMenu &menu, RenderScene &scene) {
        auto frame = std::make_unique<SettingsWindow>(L"Local LLM", 780);
        const auto state = std::make_shared<State>();
        state->scene = &scene;
        auto *panel = new View(*frame, state);
        state->context = panel->context;
        state->speed = panel->speed;
        state->model = panel->model;
        state->appearanceTab = panel->registerPrimaryButton(L"Appearance settings", [panel] {
            panel->zoomTab = false;
            panel->state->viewingZoom = false;
            const auto log = panel->state->appearanceLog;
            output(panel->state, log);
            panel->layout();
        });
        state->exploreTab = panel->registerPrimaryButton(L"AI zoom exploration", [panel] {
            panel->zoomTab = true;
            panel->state->viewingZoom = true;
            const auto log = panel->state->explorationLog;
            output(panel->state, log);
            panel->layout();
        });
        try {
            state->promptBytes = LocalAiSettings::systemPrompt(scene.getAttribute().shader).size();
        } catch (const std::exception &) {
        }
        state->newChat = panel->registerPrimaryButton(L"New chat", [state] {
            if (state->locating) {
                state->locating->cancel = true;
                state->scene->getState().interrupt();
                state->newChatPending = true;
                return;
            }
            if (state->job) {
                state->job->cancel = true;
                state->newChatPending = true;
                output(state, "Cancelling generation before starting a new chat...");
            } else {
                if (state->refining) {
                    finishRefinement(state, "Cancelled.");
                }
                newChat(state);
            }
        });
        state->errorLimit =
            panel->control(L"EDIT", L"3", WS_TABSTOP | WS_BORDER | ES_NUMBER | ES_AUTOHSCROLL);
        workspace::WorkspaceEditDrawing::attach(state->errorLimit, panel->drawing);
        SendMessageW(state->errorLimit, EM_SETLIMITTEXT, 3, 0);
        IAccPropServices *access = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_AccPropServices, nullptr, CLSCTX_INPROC_SERVER,
                                       IID_IAccPropServices, reinterpret_cast<void **>(&access)))) {
            access->SetHwndPropStr(state->errorLimit, OBJID_CLIENT, CHILDID_SELF, PROPID_ACC_NAME,
                                   UiLanguage::label(L"Error limit (1-100)"));
            access->Release();
        }
        try {
            SetWindowTextW(
                state->errorLimit,
                std::to_wstring(LocalAiSettings::errorLimit(LocalAiSettings::readConnection())).c_str());
        } catch (const std::exception &) {
        }
        state->input = editor(*panel, false, UiLanguage::label(L"Desired appearance"));
        state->autoRefine =
            panel->control(L"BUTTON", L"Automatically apply and refine", WS_TABSTOP | BS_AUTOCHECKBOX);
        SetWindowSubclass(state->autoRefine, View::checkboxProcedure, 8832,
                          reinterpret_cast<DWORD_PTR>(panel));
        state->refinementLimit =
            panel->control(L"EDIT", L"3", WS_TABSTOP | WS_BORDER | ES_NUMBER | ES_AUTOHSCROLL);
        workspace::WorkspaceEditDrawing::attach(state->refinementLimit, panel->drawing);
        SendMessageW(state->refinementLimit, EM_SETLIMITTEXT, 2, 0);
        if (SUCCEEDED(CoCreateInstance(CLSID_AccPropServices, nullptr, CLSCTX_INPROC_SERVER,
                                       IID_IAccPropServices, reinterpret_cast<void **>(&access)))) {
            access->SetHwndPropStr(state->refinementLimit, OBJID_CLIENT, CHILDID_SELF, PROPID_ACC_NAME,
                                   UiLanguage::label(L"Max changes (1-10)"));
            access->Release();
        }
        try {
            SetWindowTextW(
                state->refinementLimit,
                std::to_wstring(LocalAiSettings::refinementLimit(LocalAiSettings::readConnection())).c_str());
        } catch (const std::exception &) {
        }
        state->generate = panel->registerPrimaryButton(L"Generate settings", [state] {
            if (state->job || state->capturePending || state->locating) {
                return;
            }
            try {
                const auto instruction = inputText(state->input);
                if (instruction.empty()) {
                    throw std::runtime_error("Enter the desired appearance first.");
                }
                auto connection = LocalAiSettings::readConnection();
                const bool autoStart = SendMessageW(state->autoRefine, BM_GETCHECK, 0, 0) == BST_CHECKED;
                if (autoStart && !connection.value("vision", false)) {
                    throw std::runtime_error("Automatic refinement requires vision: true in "
                                             "local-ai.json and a vision-capable server.");
                }
                const auto roundsText = inputText(state->refinementLimit);
                if (roundsText.empty() || roundsText.find_first_not_of("0123456789") != std::string::npos ||
                    roundsText.size() > 2) {
                    SetFocus(state->refinementLimit);
                    throw std::runtime_error("Max changes must be an integer from 1 to 10.");
                }
                connection["max_refinements"] = std::stoi(roundsText);
                LocalAiSettings::refinementLimit(connection);
                const auto limitText = inputText(state->errorLimit);
                if (limitText.empty() || limitText.find_first_not_of("0123456789") != std::string::npos ||
                    limitText.size() > 3 || std::stoi(limitText) < 1 || std::stoi(limitText) > 100) {
                    SetFocus(state->errorLimit);
                    throw std::runtime_error("Error limit must be an integer from 1 to 100.");
                }
                const int limit = std::stoi(limitText);
                LocalAiSettings::saveErrorLimit(limit);
                connection["max_errors"] = limit;
                state->instruction = instruction;
                state->connection = connection;
                state->vision = connection.value("vision", false);
                state->maxRounds = LocalAiSettings::refinementLimit(connection);
                state->autoStart = autoStart;
                state->exploring = false;
                state->rounds = 0;
                state->bestScore = -1;
                state->best.reset();
                state->history.clear();
                state->sessionLog.clear();
                state->refining = false;
                state->ready.reset();
                rememberCurrentView(state);
                state->revisionTracked = false;
                state->staleImageRetries = 0;
                SetWindowTextW(state->apply,
                               UiLanguage::label(state->vision ? L"Apply and refine" : L"Apply settings"));
                EnableWindow(state->apply, FALSE);
                if (state->vision) {
                    queueCapture(state);
                } else {
                    startRequest(state, {});
                }
            } catch (const std::exception &e) {
                state->job.reset();
                finishRefinement(state, e.what());
            }
        });
        state->cancel = panel->registerPrimaryButton(L"Cancel", [state] {
            if (state->locating) {
                state->locating->cancel = true;
                state->scene->getState().interrupt();
                output(state, "Cancelling minibrot search...");
                return;
            }
            if (state->job) {
                state->job->cancel = true;
                output(state, "Cancellation requested. Waiting for the pending request to stop...");
            } else if (state->capturePending) {
                finishRefinement(state, state->exploring ? "AI zoom stopped. Current location retained."
                                                         : "Cancelled. The best evaluated appearance is "
                                                           "retained when the view is unchanged.");
            }
        });
        state->output = editor(*panel, true, UiLanguage::label(L"Result"));
        output(state, "Enter a description, generate, then apply the proposed settings.");
        state->apply = panel->registerPrimaryButton(L"Apply settings", [state] {
            if (!state->ready || state->job || state->capturePending) {
                return;
            }
            try {
                applyProposal(state);
            } catch (const std::exception &e) {
                finishRefinement(state, e.what());
            }
        });
        state->undo = panel->registerPrimaryButton(L"Undo AI changes", [state] {
            auto &current = state->scene->getAttribute().shader;
            if (!state->before) {
                return;
            }
            if (signature(current) != state->appliedSignature) {
                output(state, "Appearance has changed since applying AI settings. Undo was skipped to "
                              "preserve your newer edits.");
                return;
            }
            current = *state->before;
            state->before.reset();
            state->scene->getRequests().requestShader();
            EnableWindow(state->undo, FALSE);
            output(state, "AI appearance changes undone.");
        });
        state->zoomFactor = panel->control(L"EDIT", L"2", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);
        workspace::WorkspaceEditDrawing::attach(state->zoomFactor, panel->drawing);
        SendMessageW(state->zoomFactor, EM_SETLIMITTEXT, 16, 0);
        state->zoomInput = editor(*panel, false, UiLanguage::label(L"Features to explore"));
        state->zoomLimit = panel->control(L"EDIT", L"3", WS_TABSTOP | WS_BORDER | ES_NUMBER | ES_AUTOHSCROLL);
        workspace::WorkspaceEditDrawing::attach(state->zoomLimit, panel->drawing);
        SendMessageW(state->zoomLimit, EM_SETLIMITTEXT, 4, 0);
        state->zoomErrors =
            panel->control(L"EDIT", L"3", WS_TABSTOP | WS_BORDER | ES_NUMBER | ES_AUTOHSCROLL);
        workspace::WorkspaceEditDrawing::attach(state->zoomErrors, panel->drawing);
        SendMessageW(state->zoomErrors, EM_SETLIMITTEXT, 3, 0);
        if (SUCCEEDED(CoCreateInstance(CLSID_AccPropServices, nullptr, CLSCTX_INPROC_SERVER,
                                       IID_IAccPropServices, reinterpret_cast<void **>(&access)))) {
            access->SetHwndPropStr(state->zoomFactor, OBJID_CLIENT, CHILDID_SELF, PROPID_ACC_NAME,
                                   UiLanguage::label(L"Zoom per AI step (1 < factor <= 100)"));
            access->Release();
        }
        state->startZoom = panel->registerPrimaryButton(L"Start AI zoom", [state] {
            if (state->job || state->capturePending || state->locating) {
                return;
            }
            try {
                auto connection = LocalAiSettings::readConnection();
                if (!connection.value("vision", false)) {
                    throw std::runtime_error("AI zoom requires vision: true and a vision-capable server.");
                }
                LocalAiSettings::zoomFactor(inputText(state->zoomFactor));
                if (state->scene->getAttribute().fractal.projectionMethod != FrtProjectionMethod::PLANAR) {
                    throw std::runtime_error("AI zoom currently requires planar projection.");
                }
                const auto count = inputText(state->zoomLimit);
                if (count.empty() || count.find_first_not_of("0123456789") != std::string::npos ||
                    count.size() > 4 || std::stoi(count) < 1 || std::stoi(count) > 1000) {
                    throw std::runtime_error("Exploration steps must be an integer from 1 to 1000.");
                }
                state->maxRounds = std::stoi(count);
                const auto errors = inputText(state->zoomErrors);
                if (errors.empty() || errors.find_first_not_of("0123456789") != std::string::npos ||
                    errors.size() > 3) {
                    throw std::runtime_error("Error limit must be an integer from 1 to 100.");
                }
                connection["max_errors"] = std::stoi(errors);
                LocalAiSettings::errorLimit(connection);
                state->instruction = inputText(state->zoomInput);
                if (state->instruction.empty()) {
                    throw std::runtime_error("Describe the features you want to explore first.");
                }
                state->useMinibrot = SendMessageW(state->locate, BM_GETCHECK, 0, 0) == BST_CHECKED;
                state->locateNext = false;
                state->retryLower =
                    state->useMinibrot && SendMessageW(state->retryLocate, BM_GETCHECK, 0, 0) == BST_CHECKED;
                state->repeatCycles = state->useMinibrot &&
                                      SendMessageW(state->repeatExplore, BM_GETCHECK, 0, 0) == BST_CHECKED;
                state->cycles = 0;
                state->retries = 0;
                state->verifyZoom = false;
                state->beforeZoom.reset();
                state->rejectedTargets.clear();
                state->lostStructureRetries = 0;
                state->candidateImages.clear();
                if (state->retryLower) {
                    LocalAiSettings::retryLogZoom(100, inputText(state->retryDecrement), 0);
                }
                if (state->useMinibrot &&
                    (state->scene->getAttribute().fractal.formulaType != FractalFormulaType::MANDELBROT ||
                     state->scene->getAttribute().fractal.reuseReferenceMethod !=
                         FrtReuseReferenceMethod::DISABLED)) {
                    throw std::runtime_error(
                        "Locate Minibrot requires the Mandelbrot formula and disabled reference reuse.");
                }
                if (!state->scene->isIdleCompute() || state->scene->isLongJobBusy() ||
                    state->scene->getRequests().recomputeRequested) {
                    throw std::runtime_error("Wait for rendering to finish before starting AI zoom.");
                }
                state->scene->getState().cancel();
                state->connection = connection;
                state->vision = true;
                state->exploring = true;
                state->refining = false;
                state->ready.reset();
                state->sessionLog.clear();
                state->rounds = 0;
                state->staleImageRetries = 0;
                rememberCurrentView(state);
                EnableWindow(state->apply, FALSE);
                queueCapture(state);
            } catch (const std::exception &e) {
                finishRefinement(state, e.what());
            }
        });
        state->locate =
            panel->control(L"BUTTON", L"Locate Minibrot after exploration", WS_TABSTOP | BS_AUTOCHECKBOX);
        state->retryLocate = panel->control(
            L"BUTTON", L"On failure, lower log zoom and retry Locate Minibrot", WS_TABSTOP | BS_AUTOCHECKBOX);
        SendMessageW(state->retryLocate, BM_SETCHECK, BST_CHECKED, 0);
        state->retryDecrement = panel->control(L"EDIT", L"0.5", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL);
        workspace::WorkspaceEditDrawing::attach(state->retryDecrement, panel->drawing);
        SendMessageW(state->retryDecrement, EM_SETLIMITTEXT, 16, 0);
        state->repeatExplore = panel->control(L"BUTTON", L"After success, repeat exploration until cancelled",
                                              WS_TABSTOP | BS_AUTOCHECKBOX);
        if (SUCCEEDED(CoCreateInstance(CLSID_AccPropServices, nullptr, CLSCTX_INPROC_SERVER,
                                       IID_IAccPropServices, reinterpret_cast<void **>(&access)))) {
            access->SetHwndPropStr(state->zoomLimit, OBJID_CLIENT, CHILDID_SELF, PROPID_ACC_NAME,
                                   UiLanguage::label(L"Exploration steps (1-1000)"));
            access->SetHwndPropStr(state->zoomErrors, OBJID_CLIENT, CHILDID_SELF, PROPID_ACC_NAME,
                                   UiLanguage::label(L"Error limit (1-100)"));
            access->SetHwndPropStr(state->retryDecrement, OBJID_CLIENT, CHILDID_SELF, PROPID_ACC_NAME,
                                   UiLanguage::label(L"Retry log zoom decrease"));
            access->Release();
        }
        for (auto checkbox : {state->locate, state->retryLocate, state->repeatExplore}) {
            SetWindowSubclass(checkbox, View::checkboxProcedure, 8832, reinterpret_cast<DWORD_PTR>(panel));
        }
        SetWindowPos(state->retryDecrement, state->repeatExplore, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        busy(state, false);
        EnableWindow(state->apply, FALSE);
        EnableWindow(state->undo, FALSE);
        EnableWindow(state->cancel, FALSE);
        SetWindowSubclass(frame->getWindow(), tick, 8821,
                          reinterpret_cast<DWORD_PTR>(new std::shared_ptr<State>(state)));
        SetTimer(frame->getWindow(), 8821, 200, nullptr);
        panel->connectionButton = panel->registerPrimaryButton(L"Connection information", [panel] {
            std::wstring info;
            for (auto label : {panel->model, panel->context, panel->speed}) {
                std::wstring text(GetWindowTextLengthW(label) + 1, L'\0');
                GetWindowTextW(label, text.data(), int(text.size()));
                text.resize(wcslen(text.c_str()));
                info += text + L"\r\n";
            }
            MessageBoxW(panel->host, info.c_str(),
                        UiLanguage::text(L"Local LLM / Connection information").c_str(), MB_OK | MB_ICONINFORMATION);
        });
        panel->layout();
        refreshMetrics(state);
        menu.setCurrentActiveSettingsWindow(std::move(frame));
        SetFocus(state->input);
    }
} // namespace merutilm::rff2
