//
// Modified by GPT-6 on 2026-09-23
//

#include "TimelineWindow.hpp"
#include "IOUtilities.h"
#include "NativeDialogs.hpp"
#include "UiLanguage.hpp"
#include "../io/TimelineJsonIO.hpp"
#include "../io/TimelineAiBundle.hpp"
#include "../io/AudioTimelineIO.hpp"
#include "../video/VideoFrameSource.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <algorithm>
#include <cstring>
#include <format>
#include <stdexcept>

namespace merutilm::rff2 {
    namespace {
        class Clipboard {
        public:
            explicit Clipboard(HWND owner) {
                if (!OpenClipboard(owner)) throw std::runtime_error("Clipboard is busy. Try again.");
            }
            ~Clipboard() { CloseClipboard(); }
            void clear() {
                if (!EmptyClipboard()) throw std::runtime_error("Cannot clear clipboard");
            }
            void put(UINT format, const void *data, size_t bytes) {
                HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
                if (!memory) throw std::bad_alloc();
                void *target = GlobalLock(memory);
                if (!target) { GlobalFree(memory); throw std::bad_alloc(); }
                std::memcpy(target, data, bytes);
                GlobalUnlock(memory);
                if (!SetClipboardData(format, memory)) {
                    GlobalFree(memory);
                    throw std::runtime_error("Cannot write clipboard");
                }
            }
        };
        std::wstring wide(std::string_view text) {
            const int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
            if (!n && !text.empty()) throw std::runtime_error("Invalid UTF-8 text");
            std::wstring result(n, L'\0');
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), result.data(), n);
            return result;
        }
        void copyText(HWND window, const std::string &text) {
            const auto value = wide(text);
            Clipboard clipboard(window);
            clipboard.clear();
            clipboard.put(CF_UNICODETEXT, value.c_str(), (value.size() + 1) * sizeof(wchar_t));
        }
        std::string clipboardText(HWND window) {
            Clipboard clipboard(window);
            const HANDLE memory = GetClipboardData(CF_UNICODETEXT);
            if (!memory) throw std::runtime_error("Copy a complete timeline JSON document first");
            const size_t bytes = GlobalSize(memory);
            if (!bytes || bytes > TimelineJsonIO::maximumBytes * 2 + 2)
                throw std::runtime_error("Clipboard text exceeds 16 MiB");
            const auto *data = static_cast<const wchar_t *>(GlobalLock(memory));
            if (!data) throw std::runtime_error("Cannot read clipboard");
            const size_t length = wcsnlen(data, bytes / sizeof(wchar_t));
            std::wstring value(data, length);
            GlobalUnlock(memory);
            if (length == bytes / sizeof(wchar_t)) throw std::runtime_error("Invalid clipboard text");
            const int n = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), int(value.size()), nullptr, 0, nullptr, nullptr);
            if (!n || size_t(n) > TimelineJsonIO::maximumBytes) throw std::runtime_error("Invalid or oversized clipboard JSON");
            std::string text(n, '\0');
            WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), int(value.size()), text.data(), n, nullptr, nullptr);
            return text;
        }
        void copyImage(HWND window, const cv::Mat &image) {
            cv::Mat bgra;
            cv::cvtColor(image, bgra, cv::COLOR_BGR2BGRA);
            BITMAPINFOHEADER header{};
            header.biSize = sizeof(header);
            header.biWidth = bgra.cols;
            header.biHeight = -bgra.rows;
            header.biPlanes = 1;
            header.biBitCount = 32;
            header.biCompression = BI_RGB;
            header.biSizeImage = DWORD(bgra.total() * bgra.elemSize());
            std::vector<unsigned char> dib(sizeof(header) + header.biSizeImage);
            std::memcpy(dib.data(), &header, sizeof(header));
            std::memcpy(dib.data() + sizeof(header), bgra.data, header.biSizeImage);
            std::vector<unsigned char> png;
            if (!cv::imencode(".png", image, png)) throw std::runtime_error("Cannot encode image sheet");
            const UINT pngFormat = RegisterClipboardFormatW(L"PNG");
            if (!pngFormat) throw std::runtime_error("Cannot register PNG clipboard format");
            Clipboard clipboard(window);
            clipboard.clear();
            clipboard.put(CF_DIB, dib.data(), dib.size());
            clipboard.put(pngFormat, png.data(), png.size());
        }
    }

    void TimelineWindow::importTimelineJson(const std::string &text, const std::filesystem::path &document) {
        if (exporting) return;
        auto loaded = TimelineJsonIO::parse(text);
        if (!document.empty()) AudioTimelineIO::resolvePaths(loaded.audio, document);
        if (frameSource && loaded.estimateKeyframes != float(frameSource->getFrameCount()))
            throw std::runtime_error("estimateKeyframes does not match the current folder. Copy its current JSON and keep estimateKeyframes unchanged.");
        setPlaying(false);
        lastUndoStep = 0;
        attribute.video.timeline = std::move(loaded);
        ensureEditableTracks();
        recordUndoStep();
        applyRestoredTimeline(VidTimelineAttribute(attribute.video.timeline));
        refreshInspector(true);
    }

    void TimelineWindow::openAiExchangeMenu() {
        if (exporting || !commitFieldEdit()) return;
        for (;;) {
            const bool busy = aiImagesBusy.load();
            const bool ready = frameSource && previewScene && previewWorker.joinable();
            const HMENU menu = CreatePopupMenu();
            const auto add = [&](UINT id, const wchar_t *label, UINT flags = 0) {
                AppendMenuW(menu, MF_STRING | flags, id, UiLanguage::label(label));
            };
            add(2, L"Image grid: 2 x 2", (aiImageSide == 2 ? MF_CHECKED : 0) | (busy ? MF_GRAYED : 0));
            add(3, L"Image grid: 3 x 3", (aiImageSide == 3 ? MF_CHECKED : 0) | (busy ? MF_GRAYED : 0));
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            add(4, busy ? L"Cancel image generation" : L"Save prompt + JSON and all images to folder...",
                !busy && !ready ? MF_GRAYED : 0);
            POINT at{aiButton.left, aiButton.bottom};
            ClientToScreen(window, &at);
            const UINT chosen = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_LEFTALIGN, at.x, at.y, 0, window, nullptr);
            DestroyMenu(menu);
            if (chosen == 2 || chosen == 3) {
                aiImageSide = int(chosen);
                aiImagePage = 0;
                continue;
            }
            if (chosen == 4) {
                try {
                    if (busy) aiImagesCancel.request_stop();
                    else saveAiBundle();
                } catch (const std::exception &e) {
                    NativeDialogs::message(window, e.what(), "AI exchange", MB_OK | MB_ICONERROR);
                }
            }
            return;
        }
    }
    void TimelineWindow::requestAiImages() {
        if (!frameSource || !previewScene || !previewWorker.joinable() || aiImagesBusy.exchange(true)) return;
        aiImagesCancel = std::stop_source{};
        aiSavedPages = 0;
        aiTotalPages = 0;
        setPlaying(false);
        if (sourceAttribute) attribute.shader = sourceAttribute->shader;
        {
            std::scoped_lock lock(previewRequestMutex);
            requestedAiImageSide = aiImageSide;
            requestedAiImagePage = aiImagePage;
            requestedPreviewDepth = previewDepth;
            requestedPreviewSec = previewSeconds();
            requestedPreviewTimeline = attribute.video.timeline;
            requestedPreviewShader = attribute.shader;
            requestedPreviewSchedule = previewScheduleSnapshot;
            ++previewRequestGeneration;
        }
        previewRequestCondition.notify_one();
        InvalidateRect(window, nullptr, FALSE);
    }

    void TimelineWindow::saveAiBundle() {
        if (!frameSource || !previewScene || !previewWorker.joinable() || aiImagesBusy.load()) return;
        const auto parent = IOUtilities::ioDirectoryDialog(L"Choose where to create the AI exchange folder");
        if (!parent) return;
        setPlaying(false);
        if (sourceAttribute) attribute.shader = sourceAttribute->shader;
        auto request = std::make_shared<AiBundleRequest>();
        request->parent = *parent;
        request->timeline = attribute.video.timeline;
        request->shader = attribute.shader;
        request->mapping = previewScheduleSnapshot;
        request->prompt = TimelineJsonIO::systemPrompt(request->shader);
        const uint32_t count = frameSource->getFrameCount();
        nlohmann::json context{{"startDepth", schedule.getStartDepth()}, {"endDepth", schedule.getEndDepth()},
            {"totalSeconds", schedule.getTotalSeconds()}, {"source", frameSource->isStatic() ? "PNG" : "RFM/RFMZ"},
            {"keyframeCount", count}, {"defaultSpeed", attribute.video.animation.mps},
            {"imageGrid", aiImageSide}, {"imagePageCount", (count - 1) / uint32_t(aiImageSide * aiImageSide) + 1}};
        request->prompt += "\nRead-only current context:\n" + context.dump(2) +
            "\nThis folder saves all image pages as page_000001.png onward, in playback order. "
            "The user may attach only some pages. Consider only images actually attached; do not assume unseen frames. "
            "backup/images.json lists each page's keyframe range; its image filenames are relative to this folder's root. "
            "The backup folder is for local restoration and does not need to be attached. Return only the editable timeline JSON below.\n";
        aiImagesCancel = std::stop_source{};
        aiSavedPages = 0;
        aiTotalPages = (count - 1) / uint32_t(aiImageSide * aiImageSide) + 1;
        {
            std::scoped_lock lock(previewRequestMutex);
            requestedAiBundle = std::move(request);
            requestedAiImageSide = aiImageSide;
            requestedAiImagePage = 0;
            requestedPreviewDepth = previewDepth;
            requestedPreviewSec = previewSeconds();
            requestedPreviewTimeline = attribute.video.timeline;
            requestedPreviewShader = attribute.shader;
            requestedPreviewSchedule = previewScheduleSnapshot;
            aiImagesBusy = true;
            ++previewRequestGeneration;
        }
        previewRequestCondition.notify_one();
        InvalidateRect(window, nullptr, FALSE);
    }

    cv::Mat TimelineWindow::renderAiImagePage(int side, uint32_t page, const VidTimelineAttribute &timeline,
                                             const ShaderAttribute &shader, const TimelineSchedule &mapping,
                                             uint64_t generation, const std::function<bool()> &cancelled) {
        constexpr int tileWidth = 512, tileHeight = 320, labelHeight = 32;
        cv::Mat sheet(side * (tileHeight + labelHeight), side * tileWidth, CV_8UC3, cv::Scalar(24, 24, 24));
        const uint32_t count = frameSource->getFrameCount(), perPage = uint32_t(side * side);
        for (uint32_t i = 0; i < perPage; ++i) {
            if (cancelled()) return {};
            const uint64_t offset = uint64_t(page) * perPage + i;
            if (offset >= count) break;
            const uint32_t key = count - uint32_t(offset);
            const float seconds = mapping.timeAt(float(key));
            cv::Mat image;
            if (!renderFramePreview(float(key), seconds, timeline, shader, mapping, generation, &image) || image.empty())
                throw std::runtime_error("A keyframe preview failed. The image page was not completed.");
            if (image.channels() == 4) cv::cvtColor(image, image, cv::COLOR_BGRA2BGR);
            if (image.type() != CV_8UC3) throw std::runtime_error("Unsupported preview image format");
            const double scale = std::min(double(tileWidth) / image.cols, double(tileHeight) / image.rows);
            cv::Mat resized;
            cv::resize(image, resized, cv::Size(std::max(1, int(image.cols * scale)), std::max(1, int(image.rows * scale))), 0, 0, cv::INTER_AREA);
            const int x = int(i % side) * tileWidth, y = int(i / side) * (tileHeight + labelHeight);
            resized.copyTo(sheet(cv::Rect(x + (tileWidth - resized.cols) / 2, y + (tileHeight - resized.rows) / 2, resized.cols, resized.rows)));
            cv::putText(sheet, std::format("K{} | {:.3f}s | page {}", key, seconds, page + 1),
                {x + 8, y + tileHeight + 22}, cv::FONT_HERSHEY_SIMPLEX, .55, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
        }
        return sheet;
    }

    void TimelineWindow::renderAiImages(int side, uint32_t page, const VidTimelineAttribute &timeline,
                                        const ShaderAttribute &shader, const TimelineSchedule &mapping,
                                        uint64_t generation, std::stop_token stop,
                                        const std::shared_ptr<const AiBundleRequest> &bundle) {
        cv::Mat sheet;
        std::string error;
        std::filesystem::path saved;
        const auto cancelToken = aiImagesCancel.get_token();
        const auto cancelled = [&] { return stop.stop_requested() || cancelToken.stop_requested(); };
        bool wasCancelled = false;
        try {
            if (bundle) {
                const auto result = TimelineAiBundle::save(bundle->parent, bundle->timeline, bundle->prompt,
                    side, frameSource->getFrameCount(),
                    [&](uint32_t at) { return renderAiImagePage(side, at, bundle->timeline, bundle->shader,
                        *bundle->mapping, generation, cancelled); }, cancelled,
                    [&](uint32_t done, uint32_t total) {
                        aiSavedPages = done;
                        aiTotalPages = total;
                        PostMessageW(window, WM_APP + 0x272, 0, 0);
                    });
                saved = result.directory;
                error = result.error;
                wasCancelled = result.cancelled;
            } else {
                sheet = renderAiImagePage(side, page, timeline, shader, mapping, generation, cancelled);
                wasCancelled = cancelled();
            }
        } catch (const std::exception &e) { sheet.release(); error = e.what(); }
        {
            std::scoped_lock lock(previewBitmapMutex);
            aiImageSheet = std::move(sheet);
            aiImageError = std::move(error);
            aiImageGeneration = generation;
            aiSavedDirectory = std::move(saved);
            aiSaveCancelled = wasCancelled;
        }
        PostMessageW(window, WM_APP + 0x270, static_cast<WPARAM>(generation), 0);
    }

    void TimelineWindow::finishAiImages(uint64_t generation) {
        cv::Mat sheet;
        std::string error;
        std::filesystem::path directory;
        bool cancelled = false;
        {
            std::scoped_lock lock(previewBitmapMutex);
            if (generation != aiImageGeneration || !aiImagesBusy.exchange(false)) return;
            sheet = std::move(aiImageSheet);
            error = std::move(aiImageError);
            aiImageGeneration = 0;
            directory = std::move(aiSavedDirectory);
            cancelled = aiSaveCancelled;
        }
        InvalidateRect(window, nullptr, FALSE);
        try {
            if (!directory.empty()) {
                const auto message = UiLanguage::text(!error.empty() ? L"Saving stopped before completion. Saved files remain in:" :
                    cancelled ? L"Saving cancelled. Saved files remain in:" : L"Prompt, JSON and all image pages saved to:") +
                    L"\n" + directory.wstring() + (error.empty() ? L"" : L"\n\n" + wide(error));
                NativeDialogs::message(window, message.c_str(), L"AI exchange", MB_OK | (error.empty() ? MB_ICONINFORMATION : MB_ICONERROR));
                return;
            }
            if (!error.empty()) throw std::runtime_error(error);
            if (cancelled) {
                NativeDialogs::message(window, L"Image generation cancelled.", L"AI exchange", MB_OK);
                return;
            }
            if (sheet.empty()) throw std::runtime_error("No image sheet was rendered");
            copyImage(window, sheet);
            NativeDialogs::message(window, L"Image page copied. Paste it into your AI chat.", L"AI exchange", MB_OK);
        } catch (const std::exception &e) {
            NativeDialogs::message(window, e.what(), "AI exchange", MB_OK | MB_ICONERROR);
        }
    }
}
