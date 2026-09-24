//
// Modified by GPT-6 on 2026-09-14, 2026-09-16, 2026-09-18, 2026-09-21, 2026-09-22, 2026-09-23, 2026-09-24
//

#pragma once
#include "../NativeDialogs.hpp"
#include "AttributeFormModel.hpp"
#include "FormValues.hpp"
#include "../../attr/NumericSettingLimits.hpp"
#include "../RenderScene.hpp"
#include "../VideoWindow.hpp"
#include "../IOUtilities.h"
#include "../../video/VideoFrameSource.hpp"

namespace merutilm::rff2::workspace {
    class ExportWorkspace : public std::enable_shared_from_this<ExportWorkspace> {
        RenderScene &scene;
        HWND owner;
        std::shared_ptr<AttributeFormModel> model;
        std::shared_ptr<ExportProgress> progress = std::make_shared<ExportProgress>();
        std::jthread worker;
        std::atomic<bool> workerRunning{false};
        std::filesystem::path imagePath, videoPath, keyframes;
        float lastClarity;
        uint32_t lastSsaa;
        std::optional<SIZE> proposedSize;
        struct Edit {
            FormDraft before, after;
            SIZE beforeSize, afterSize;
            uint64_t serial;
        };
        std::deque<Edit> undoEntries, redoEntries;
        HistoryOrder order;
        WorkspaceForm basic;
        SIZE canvasSize() const {
            return scene.documentCanvasSize();
        }
        static bool sameSize(SIZE a, SIZE b) {
            return a.cx == b.cx && a.cy == b.cy;
        }
        FormDraft values() const {
            FormDraft result;
            for (const auto &field : basic.fields) {
                result[field.id] = field.read();
            }
            return result;
        }
        bool restore(bool redo) {
            if (busy()) {
                return false;
            }
            auto &sourceHistory = redo ? redoEntries : undoEntries;
            auto &destinationHistory = redo ? undoEntries : redoEntries;
            if (sourceHistory.empty() || (redo && !order.validRedo())) {
                return false;
            }
            const auto &edit = sourceHistory.back();
            const auto current = values();
            for (const auto &[id, value] : redo ? edit.before : edit.after) {
                if (current.at(id) != value) {
                    undoEntries.clear();
                    redoEntries.clear();
                    return false;
                }
            }
            const bool resize = !sameSize(edit.beforeSize, edit.afterSize);
            if (resize && !sameSize(canvasSize(), redo ? edit.beforeSize : edit.afterSize)) {
                undoEntries.clear();
                redoEntries.clear();
                return false;
            }
            const auto target = resize ? (redo ? edit.afterSize : edit.beforeSize) : canvasSize();
            proposedSize = target;
            struct ResetSize {
                std::optional<SIZE> &size;
                ~ResetSize() {
                    size.reset();
                }
            } reset{proposedSize};
            if (!basic.apply(redo ? edit.after : edit.before).empty()) {
                return false;
            }
            basic.clearHistory();
            if (resize) {
                scene.wndRequestClientSize(target.cx, target.cy);
            }
            if (!redo) {
                order.prepareUndo(redoEntries);
            }
            destinationHistory.push_back(std::move(sourceHistory.back()));
            sourceHistory.pop_back();
            return true;
        }
        void fail(std::wstring text) {
            progress->report(ExportProgress::Phase::FAILED, std::move(text));
        }
        bool destination(const std::filesystem::path &path) {
            if (path.empty()) {
                fail(L"Choose an output file first.");
                return false;
            }
            std::error_code error;
            const auto folder = path.has_parent_path() ? path.parent_path() : std::filesystem::current_path();
            if (!std::filesystem::is_directory(folder, error)) {
                fail(L"The destination folder does not exist.");
                return false;
            }
            if (std::filesystem::exists(path, error) &&
                NativeDialogs::message(owner, (L"Replace this file?\n\n" + path.wstring()).c_str(),
                                       L"Replace output", MB_YESNO | MB_ICONQUESTION) != IDYES) {
                return false;
            }
            return true;
        }
        std::wstring apply(const FormDraft &draft) {
            if (busy()) {
                return L"Wait for the export to finish, or cancel it before applying settings.";
            }
            auto nextImagePath = imagePath, nextVideoPath = videoPath, nextKeyframesPath = keyframes;
            const auto beforeSize = canvasSize();
            const auto before = values();
            uint16_t width = beforeSize.cx, height = beforeSize.cy;
            FormDraft attributes;
            for (const auto &[id, value] : draft) {
                if (id == "export.imagePath") {
                    nextImagePath = value;
                } else if (id == "export.videoPath") {
                    nextVideoPath = value;
                } else if (id == "export.keyframes") {
                    nextKeyframesPath = value;
                } else if (id == "export.width" || id == "export.height") {
                    uint16_t n;
                    if (!AttributeFormModel::parse(value, n) || n < 64 || n > 16384) {
                        return L"Canvas width and height must be 64 to 16384.";
                    }
                    (id == "export.width" ? width : height) = n;
                } else {
                    attributes[id] = value;
                }
            }
            proposedSize = SIZE{width, height};
            struct ResetSize {
                std::optional<SIZE> &size;
                ~ResetSize() {
                    size.reset();
                }
            } reset{proposedSize};
            const auto error = model->apply(attributes);
            if (!error.empty()) {
                return error;
            }
            imagePath = std::move(nextImagePath);
            videoPath = std::move(nextVideoPath);
            keyframes = std::move(nextKeyframesPath);
            const SIZE afterSize{width, height};
            if (!sameSize(afterSize, beforeSize)) {
                scene.wndRequestClientSize(width, height);
            }
            Edit edit{{}, {}, beforeSize, afterSize, 0};
            const auto after = values();
            for (const auto &[id, value] : before) {
                if (after.at(id) != value) {
                    edit.before[id] = value;
                    edit.after[id] = after.at(id);
                }
            }
            if (!edit.before.empty() || !sameSize(beforeSize, afterSize)) {
                edit.serial = order.commit();
                undoEntries.push_back(std::move(edit));
                if (undoEntries.size() > 128) {
                    undoEntries.pop_front();
                }
                redoEntries.clear();
            }
            basic.clearHistory();
            return L"";
        }
        void chooseImage() {
            if (busy()) {
                return;
            }
            if (auto file = IOUtilities::ioFileDialog(L"Export Image", Constants::Extension::DESC_IMAGE,
                                                      IOUtilities::SAVE_FILE, Constants::Extension::IMAGE)) {
                imagePath = *file;
            }
        }
        void chooseVideo() {
            if (busy()) {
                return;
            }
            const auto &a = scene.getAttribute();
            const bool lossless =
                a.video.exportation.lossless &&
                (!a.shader.hdr.use || a.video.exportation.hdrTransfer == VidHdrTransfer::SDR);
            if (auto file = IOUtilities::ioFileDialog(
                    L"Export Video", Constants::Extension::DESC_VIDEO, IOUtilities::SAVE_FILE,
                    lossless ? Constants::Extension::VIDEO_LOSSLESS : Constants::Extension::VIDEO)) {
                videoPath = *file;
            }
        }
        void chooseKeyframes() {
            if (busy()) {
                return;
            }
            if (auto folder = IOUtilities::ioDirectoryDialog(L"Video Keyframe Folder")) {
                keyframes = *folder;
            }
        }

      public:
        ExportWorkspace(RenderScene &scene, HWND owner)
            : scene(scene), owner(owner), lastClarity(scene.getAttribute().render.clarityMultiplier),
              lastSsaa(scene.getAttribute().render.ssaa) {
            model = std::make_shared<AttributeFormModel>(
                [this]() -> Attribute & { return this->scene.getAttribute(); },
                [this] {
                    const auto &r = this->scene.getAttribute().render;
                    if (r.clarityMultiplier != lastClarity || r.ssaa != lastSsaa) {
                        lastClarity = r.clarityMultiplier;
                        lastSsaa = r.ssaa;
                        this->scene.getRequests().requestResize();
                        this->scene.getRequests().requestRecompute();
                    }
                    this->scene.getRequests().requestShader();
                });
            model->setValidator([this](const Attribute &a) -> std::wstring {
                const auto width = proposedSize ? proposedSize->cx : this->scene.getClientWidth(),
                           height = proposedSize ? proposedSize->cy : this->scene.getClientHeight();
                const auto maximum = this->scene.getMaxInternalScale(width, height);
                if (maximum > 0 &&
                    double(a.render.clarityMultiplier) * a.render.ssaa * a.video.data.sourceScale > maximum) {
                    return L"Resolution, clarity and supersampling exceed the GPU render-size limit.";
                }
                if (a.render.clarityMultiplier != lastClarity || a.render.ssaa != lastSsaa ||
                    width != this->scene.getClientWidth() || height != this->scene.getClientHeight()) {
                    const auto warning = this->scene.checkRenderMemoryBudget(a, width, height);
                    if (!warning.empty() &&
                        NativeDialogs::message(this->owner, (warning + L"\n\nApply these settings?").c_str(),
                                               L"Render memory", MB_YESNO | MB_ICONWARNING) != IDYES) {
                        return L"Quality change cancelled.";
                    }
                }
                return L"";
            });
            const float limit = std::numeric_limits<float>::max();
            model->numeric(
                "export.clarity", 2, L"Clarity", L"Scales the canvas resolution for both preview and output.",
                [](auto &a) -> auto & { return a.render.clarityMultiplier; }, std::nextafter(.01f, 1.f),
                limit);
            model->numeric(
                "export.ssaa", 2, L"Supersampling",
                L"1 to 8. Computes extra samples and downsamples the output.",
                [](auto &a) -> auto & { return a.render.ssaa; }, uint32_t(1), uint32_t(8));
            model->choice("export.interpolation", 2, L"Linear Interpolation", L"Smooths the final image.",
                          [](auto &a) -> auto & { return a.render.linearInterpolation; });
            model->choice("export.dither", 2, L"Dither", L"Reduces banding in smooth 8-bit gradients.",
                          [](auto &a) -> auto & { return a.render.dither; });
            model->numeric(
                "export.fps", 3, L"Video Frame Rate", L"Frames per second; 1 to 1000. Fractional rates are supported.",
                [](auto &a) -> auto & { return a.video.exportation.fps; }, NumericSettingLimits::minimumFps,
                NumericSettingLimits::maximumFps);
            model->numeric(
                "export.bitrate", 3, L"Video Bitrate (kbps)", L"1 to 1000000. Ignored for lossless output.",
                [](auto &a) -> auto & { return a.video.exportation.bitrate; }, uint32_t(1),
                uint32_t(1000000));
            model->choice("export.lossless", 3, L"Lossless Video",
                          L"SDR RGB video in MKV. HDR uses the selected HDR transfer.",
                          [](auto &a) -> auto & { return a.video.exportation.lossless; });
            model->numeric(
                "export.keyframeAA", 3, L"Keyframe Transition Samples",
                L"1 to 8. Improves transitions between saved maps.",
                [](auto &a) -> auto & { return a.video.exportation.keyframeAA; }, uint32_t(1), uint32_t(8));
            model->numeric(
                "export.colorAA", 3, L"Color Animation Samples", L"1 to 8 temporal samples per video frame.",
                [](auto &a) -> auto & { return a.video.exportation.colorAA; }, uint32_t(1), uint32_t(8));
            model->choice("export.hdr", 4, L"HDR Rendering",
                          L"Keeps highlights above white for tone mapping and HDR video.",
                          [](auto &a) -> auto & { return a.shader.hdr.use; });
            model->choice("export.transfer", 4, L"Video Transfer",
                          L"SDR, HDR10 (PQ), or HLG. HDR video requires HDR Rendering.",
                          [](auto &a) -> auto & { return a.video.exportation.hdrTransfer; });
            model->numeric(
                "export.peak", 4, L"HDR Peak Brightness (nits)", L"100 to 10000. Used by PQ output.",
                [](auto &a) -> auto & { return a.video.exportation.hdrPeakNits; }, 100.f, 10000.f);
            model->choice("export.toneMap", 4, L"Tone Mapping",
                          L"Curve used for SDR preview and image output. MFR modes use BT.2020 luminance and "
                          L"preserve source light; enable HDR Rendering. False Color shows -16 to +16 EV.",
                          [](auto &a) -> auto & { return a.shader.hdr.method; });
            model->numeric(
                "export.exposure", 4, L"Exposure", L"Exposure adjustment in stops.",
                [](auto &a) -> auto & { return a.shader.hdr.exposure; }, NumericSettingLimits::hdrExposure.minimum, NumericSettingLimits::hdrExposure.maximum);
            model->numeric(
                "export.mfrPeak", 4, L"MFR Mastering Peak (nits)",
                L"MFR display metadata: reference white is 203 nits. Does not change source light. 100 to "
                L"10000.",
                [](auto &a) -> auto & { return a.shader.hdr.mfrPeakNits; }, 100.f, 10000.f);
            model->numeric(
                "export.headroom", 4, L"Highlight Headroom", L"Linear highlight range before tone mapping.",
                [](auto &a) -> auto & { return a.shader.hdr.headroom; }, NumericSettingLimits::hdrHeadroom.minimum, NumericSettingLimits::hdrHeadroom.maximum);
            model->choice("export.compress", 5, L"Compress Keyframes",
                          L"Saves lossless RFMZ maps during keyframe generation.",
                          [](auto &a) -> auto & { return a.video.exportation.compressKeyframes; });
            model->choice("export.autoVideo", 5, L"Create Video After Keyframes",
                          L"Starts video output after keyframe generation completes.",
                          [](auto &a) -> auto & { return a.video.exportation.autoCreateVideo; });
            model->choice("export.showPreview", 5, L"Show Video Export Preview",
                          L"Shows the video while exporting. Turn off to reduce display work; progress and "
                          L"cancellation remain available.",
                          [](auto &a) -> auto & { return a.video.exportation.showExportPreview; });
            model->choice("export.pauseVideo", 5, L"Pause Preview During Export",
                          L"Reserves the GPU for the video export.",
                          [](auto &a) -> auto & { return a.video.exportation.pauseMainPreview; });
            model->choice("export.pauseKeyframes", 5, L"Pause Preview During Generation",
                          L"Holds the picture while calculating new keyframes.",
                          [](auto &a) -> auto & { return a.video.exportation.pauseKeyframePreview; });
            basic = model->form(L"", {});
        }
        ~ExportWorkspace() {
            cancel();
            if (worker.joinable()) {
                worker.join();
            }
        }
        bool ownsActiveJob() const {
            return workerRunning || progress->active();
        }
        bool busy() const {
            return ownsActiveJob() || scene.isLongJobBusy() || scene.getVideoExportActive() ||
                   scene.getVideoGenerationActive();
        }
        void cancel() {
            progress->cancelRequested = true;
            if (scene.isLongJobBusy()) {
                scene.requestLongJobCancel();
            }
        }
        std::shared_ptr<ExportProgress> job() const {
            return progress;
        }
        void startImage() {
            if (busy()) {
                return;
            }
            if (!destination(imagePath)) {
                return;
            }
            progress = std::make_shared<ExportProgress>();
            progress->report(ExportProgress::Phase::QUEUED, L"Waiting for the render to finish...");
            scene.getRequests().requestCreateImage(imagePath, true, progress);
        }
        void startVideo() {
            if (busy()) {
                return;
            }
            auto a = scene.getAttribute();
            std::wstring error;
            auto source = VideoFrameSource::open(keyframes, a.video.data.isStatic, error);
            if (!source) {
                fail(error);
                return;
            }
            a.video.data.isStatic = source->isStatic();
            if (a.video.exportation.hdrTransfer != VidHdrTransfer::SDR && !a.shader.hdr.use) {
                fail(L"Enable HDR Rendering or choose SDR before exporting.");
                return;
            }
            if (a.video.exportation.lossless &&
                (!a.shader.hdr.use || a.video.exportation.hdrTransfer == VidHdrTransfer::SDR)) {
                videoPath.replace_extension(Constants::Extension::VIDEO_LOSSLESS);
            }
            if (!destination(videoPath)) {
                return;
            }
            if (worker.joinable()) {
                worker.join();
            }
            std::wstring startupFailureMessage = L"The video export worker could not start.";
            std::wstring workerFailureMessage =
                L"Video export failed. Check the source and destination.";
            progress = std::make_shared<ExportProgress>();
            progress->report(ExportProgress::Phase::QUEUED, L"Starting video export...");
            workerRunning = true;
            scene.setVideoExportActive(true);
            try {
                worker =
                    std::jthread([this, a = std::move(a), input = keyframes, output = videoPath,
                                  job = progress, failureMessage = std::move(workerFailureMessage)]() mutable {
                        ExportCompletion completion{job};
                        try {
                            VideoWindow::createVideo(scene.engine, a, input, output, job);
                        } catch (const std::exception &) {
                            job->report(ExportProgress::Phase::FAILED,
                                        std::move(failureMessage));
                        }
                        scene.setVideoExportActive(false);
                        workerRunning = false;
                    });
            } catch (const std::exception &) {
                scene.setVideoExportActive(false);
                workerRunning = false;
                progress->report(ExportProgress::Phase::FAILED, std::move(startupFailureMessage));
            }
        }
        WorkspaceForm form() {
            auto self = shared_from_this();
            auto result =
                model->form(L"Export", {L"Image", L"Video", L"Resolution & Quality", L"Video Encoding",
                                        L"HDR & Tone Mapping", L"Keyframe Output"});
            const auto field = [&](const char *id, int group, const wchar_t *label, const wchar_t *hint,
                                   auto read) {
                result.fields.push_back({id, group, label, hint, read, {}});
            };
            field("export.imagePath", 0, L"Image File",
                  L"PNG or another supported image format. Existing files require confirmation.",
                  [self] { return self->imagePath.wstring(); });
            field("export.keyframes", 1, L"Keyframe Folder", L"Uses existing RFM, RFMZ or PNG keyframes.",
                  [self] { return self->keyframes.wstring(); });
            field("export.videoPath", 1, L"Video File", L"MP4 for normal video; lossless SDR uses MKV.",
                  [self] { return self->videoPath.wstring(); });
            field("export.width", 2, L"Canvas Width",
                  L"Base width, 64 to 16384. Clarity scales the image output.",
                  [self] { return AttributeFormModel::number(self->canvasSize().cx); });
            field("export.height", 2, L"Canvas Height",
                  L"Base height, 64 to 16384. Video size comes from its keyframes.",
                  [self] { return AttributeFormModel::number(self->canvasSize().cy); });
            for (auto &entry : result.fields) {
                if (entry.id == "export.imagePath" || entry.id == "export.videoPath" ||
                    entry.id == "export.keyframes") {
                    entry.persisted = false;
                }
            }
            for (auto &entry : result.fields) {
                if (entry.id == "export.width" || entry.id == "export.height") {
                    entry.validate = AttributeFormModel::rangeValidation<uint16_t>(64, 16384);
                }
            }
            std::stable_sort(result.fields.begin(), result.fields.end(), [](const auto &a, const auto &b) {
                if (a.group != b.group) {
                    return a.group < b.group;
                }
                if (a.group != 2) {
                    return false;
                }
                const auto rank = [](std::string_view id) {
                    constexpr std::array<std::string_view, 6> ids{"export.width",         "export.height",
                                                                  "export.clarity",       "export.ssaa",
                                                                  "export.interpolation", "export.dither"};
                    return std::find(ids.begin(), ids.end(), id) - ids.begin();
                };
                return rank(a.id) < rank(b.id);
            });
            result.apply = [self](const FormDraft &d) { return self->apply(d); };
            result.inspect = [self, fields = result.fields](const FormDraft &draft) {
                FormFeedback feedback;
                const FormValues values(fields, draft);
                const auto width = values.number<uint32_t>("export.width"),
                           height = values.number<uint32_t>("export.height"),
                           ssaa = values.number<uint32_t>("export.ssaa");
                const auto clarity = values.number<float>("export.clarity");
                if (width && height && ssaa && clarity && *width >= 64 && *width <= 16384 && *height >= 64 &&
                    *height <= 16384 && *ssaa >= 1 && *ssaa <= 8 && *clarity > .01f) {
                    const auto maximum = self->scene.getMaxInternalScale(*width, *height);
                    if (maximum > 0 &&
                        double(*clarity) * *ssaa * self->scene.getAttribute().video.data.sourceScale >
                            maximum) {
                        feedback.errors["export.width"] =
                            L"GPU limit: lower Canvas Width or another marked value.";
                        feedback.errors["export.height"] =
                            L"GPU limit: lower Canvas Height or another marked value.";
                        feedback.errors["export.clarity"] =
                            L"GPU limit: lower Clarity or another marked value.";
                        feedback.errors["export.ssaa"] =
                            L"GPU limit: lower Supersampling or another marked value.";
                        feedback.summary =
                            L"Combined render size exceeds the GPU limit. Lower one or more marked values.";
                    }
                }
                const auto hdr = values.number<int>("export.hdr"),
                           transfer = values.number<int>("export.transfer"),
                           lossless = values.number<int>("export.lossless");
                if (hdr && transfer && !*hdr && *transfer != int(VidHdrTransfer::SDR)) {
                    feedback.hints["export.transfer"] =
                        L"Video export needs HDR Rendering set to On, or Video Transfer set to SDR.";
                }
                if (hdr && !*hdr) {
                    feedback.hints["export.toneMap"] =
                        L"HDR Rendering is Off. This curve is kept for when HDR Rendering is On.";
                }
                const auto toneMap = values.number<int>("export.toneMap");
                if (hdr && *hdr && toneMap && *toneMap >= 4) {
                    feedback.hints["export.headroom"] =
                        L"MFR SDR ignores Headroom. Use Exposure and MFR Mastering Peak.";
                }
                if (toneMap && *toneMap < 4) {
                    feedback.hints["export.mfrPeak"] =
                        L"Used by MFR Shoulder and MFR Log View. Select an MFR display mode to try it.";
                }
                if (hdr && *hdr && toneMap && *toneMap == 7) {
                    feedback.hints["export.toneMap"] =
                        L"EV legend: -16 navy, -8 blue, 0 gray, +8 yellow, +16 red. Zero light is black.";
                }

                if (hdr && transfer && lossless && *lossless &&
                    (!*hdr || *transfer == int(VidHdrTransfer::SDR))) {
                    feedback.hints["export.bitrate"] = L"Lossless SDR output ignores Video Bitrate. Turn "
                                                       L"Lossless Video Off to use this value.";
                }
                if (transfer && *transfer != int(VidHdrTransfer::PQ)) {
                    feedback.hints["export.peak"] = L"Used by HDR10 (PQ) video. This value is kept when "
                                                    L"another Video Transfer is selected.";
                }
                return feedback;
            };
            result.canUndo = [self] { return !self->busy() && !self->undoEntries.empty(); };
            result.canRedo = [self] {
                return !self->busy() && self->order.validRedo() && !self->redoEntries.empty();
            };
            result.undo = [self] { return self->restore(false); };
            result.redo = [self] { return self->restore(true); };
            result.bindHistory = [self](auto domain) {
                self->undoEntries.clear();
                self->redoEntries.clear();
                self->order.bind(std::move(domain));
            };
            result.undoOrder = [self] {
                return self->undoEntries.empty() ? 0 : self->undoEntries.back().serial;
            };
            result.redoOrder = [self] {
                return !self->order.validRedo() || self->redoEntries.empty()
                           ? 0
                           : self->redoEntries.back().serial;
            };
            result.clearHistory = [self, base = result.clearHistory] {
                base();
                self->undoEntries.clear();
                self->redoEntries.clear();
                self->lastClarity = self->scene.getAttribute().render.clarityMultiplier;
                self->lastSsaa = self->scene.getAttribute().render.ssaa;
            };
            result.status = [self] { return self->progress->snapshot().message; };
            result.actions = {{0, L"Choose Image File", [self] { self->chooseImage(); }},
                              {0, L"Export Image", [self] { self->startImage(); }},
                              {0, L"Cancel Export", [self] { self->cancel(); }, true, true},
                              {1, L"Choose Keyframe Folder", [self] { self->chooseKeyframes(); }},
                              {1, L"Choose Video File", [self] { self->chooseVideo(); }},
                              {1, L"Export Video", [self] { self->startVideo(); }},
                              {1, L"Cancel Export", [self] { self->cancel(); }, true, true}};
            return result;
        }
    };
} // namespace merutilm::rff2::workspace
