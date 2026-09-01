//
// Created by Merutilm on 2025-06-08.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-18, 2026-08-21, 2026-08-23, 2026-08-24, 2026-08-31.
// Modified by Opus 5 on 2026-08-12, 2026-08-14, 2026-08-18, 2026-08-19, 2026-08-26, 2026-08-31
//

#include "CallbackVideo.hpp"

#include <cwchar>

#include "../constants/Constants.hpp"
#include "IOUtilities.h"
#include "Callback.hpp"
#include "TimelineWindow.hpp"
#include "VideoWindow.hpp"
#include "../io/RFFStaticMapBinary.h"
#include "../preset/shader/bloom/ShdBloomPresets.h"
#include "../preset/shader/fog/ShdFogPresets.h"
#include "../preset/shader/slope/ShdSlopePresets.h"
#include "../preset/shader/stripe/ShdStripePresets.h"


namespace merutilm::rff2 {

    const std::function<void(SettingsMenu &, RenderScene &)> CallbackVideo::TIMELINE_EDITOR = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        TimelineWindow::open(settingsMenu, scene, GetActiveWindow());
    };

    static bool containsExistingVideoKeyframes(const std::filesystem::path &dir) {
        std::error_code error;
        std::filesystem::directory_iterator entry(dir, error);
        const std::filesystem::directory_iterator end;
        while (!error && entry != end) {
            std::error_code typeError;
            if (entry->is_regular_file(typeError)) {
                const std::wstring extension = entry->path().extension().wstring();
                if (_wcsicmp(extension.c_str(), L".rfm") == 0 || _wcsicmp(extension.c_str(), L".rfmz") == 0 ||
                    _wcsicmp(extension.c_str(), L".png") == 0) {
                    return true;
                }
            }
            entry.increment(error);
        }
        return false;
    }

    struct ScopedVideoLock {
        RenderScene& scene;
        ScopedVideoLock(RenderScene& s) : scene(s) {
            scene.setVideoGenerationActive(true);
        }
        ~ScopedVideoLock() {
            scene.setVideoGenerationActive(false);
        }
    };

    struct ScopedVideoExport {
        RenderScene &scene;

        explicit ScopedVideoExport(RenderScene &scene) : scene(scene) {
            scene.setVideoExportActive(true);
        }

        ~ScopedVideoExport() {
            scene.setVideoExportActive(false);
        }
    };


    const std::function<void(SettingsMenu &, RenderScene &)> CallbackVideo::DATA_SETTINGS = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        auto &[defaultZoomIncrement, isStatic] = scene.getAttribute().video.data;
        auto window = std::make_unique<SettingsWindow>(L"Set Data");

        window->registerSectionHeader(L"Keyframes", false);
        window->registerTextInput<float>(L"Zoom Step per Keyframe", &defaultZoomIncrement,
                                         Unparser::FLOAT,
                                         Parser::FLOAT,
                                         [](const float &v) { return v > 1; },
                                         Callback::NOTHING, L"Set zoom step per keyframe",
                                         L"How much the view zooms in between two adjacent keyframes (log scale).");

        window->registerSectionHeader(L"Source Mode");
        window->registerCheckboxInput(L"Render from PNG images", &isStatic, Callback::NOTHING, L"Render from PNG images",
                                  L"Builds the video from PNG images instead of data files. A PNG is a finished picture and carries no iteration data, so the stripe and the slope are unavailable; Color, Fog and Bloom still run over it and the Timeline Editor can animate them, which is how a PNG video is faded, exposed or hazed.");

        window->setWindowCloseFunction([&settingsMenu] {
            settingsMenu.setCurrentActiveSettingsWindow(nullptr);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window));
    };

    const std::function<void(SettingsMenu &, RenderScene &)> CallbackVideo::ANIMATION_SETTINGS = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        auto &[overZoom, showText, mps] = scene.getAttribute().video.animation;
        auto window = std::make_unique<SettingsWindow>(L"Set Animation");
        window->registerSectionHeader(L"Zoom Motion", false);
        window->registerTextInput<float>(L"Extra Final Zoom-in", &overZoom, Unparser::floatFixed(0), Parser::FLOAT,
                                         ValidCondition::POSITIVE_FLOAT_ZERO, Callback::NOTHING, L"Extra Final Zoom-in",
                                         L"Adds extra zoom-in at the very end of the video.");
        window->registerTextInput<float>(L"Zoom Speed", &mps, Unparser::floatTrim(7), Parser::FLOAT, ValidCondition::POSITIVE_FLOAT,
                                         Callback::NOTHING, L"Zoom Speed",
                                         L"Number of keyframes shown per second - higher = faster zoom.");
        window->registerSectionHeader(L"Overlay");
        window->registerCheckboxInput(L"Show Zoom Ratio", &showText, Callback::NOTHING, L"Show Zoom Ratio", L"Display the zoom ratio on the video.");

        window->setWindowCloseFunction([&settingsMenu] {
            settingsMenu.setCurrentActiveSettingsWindow(nullptr);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window));
    };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackVideo::EXPORT_SETTINGS = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        auto window = std::make_unique<SettingsWindow>(L"Set Export");
        auto &[fps, bitrate, lossless, keyframeAA, colorAA, autoCreateVideo, pauseMainPreview,
               pauseKeyframePreview, compressKeyframes, hdrTransfer, hdrPeakNits] =
                scene.getAttribute().video.exportation;
        window->registerSectionHeader(L"Video Output", false);
        window->registerTextInput<float>(L"Frame Rate (FPS)", &fps, Unparser::floatFixed(2), Parser::FLOAT, ValidCondition::POSITIVE_FLOAT,
                                         Callback::NOTHING, L"Set video frame rate", L"Frames per second of the exported video.");
        // The field is uint32_t, so it must be parsed as one: the U_SHORT parser masked with 0xFFFF,
        // which silently turned an entered 100000 into 34464 instead of rejecting it.
        const HWND bitrateRow = window->registerTextInput<uint32_t>(L"Bitrate (kbps)", &bitrate, Unparser::U_LONG, Parser::U_LONG,
                                            [](const uint32_t &v) { return v >= 1 && v <= 1000000; },
                                            Callback::NOTHING, L"Set the bitrate",
                                            L"Video bitrate in kbps. Higher = better quality but larger file. "
                                            L"1..1000000 (1 Gbps). Ignored while Lossless is on.");
        // The toggle sits under the row it greys out, so it captures that row's handle directly.
        window->registerCheckboxInput(L"Lossless", &lossless,
                                      [winPtr = window.get(), bitrateRow, losslessPtr = &lossless] {
                                          winPtr->setRowEnabled(bitrateRow, !*losslessPtr);
                                      }, L"Lossless",
                                      L"Encodes every frame exactly as rendered (x264 -qp 0 in RGB), so the video "
                                      L"carries no compression artifacts at all. Bitrate is ignored and the file is "
                                      L"always written as .mkv, because mp4 players cannot decode RGB H.264. Expect "
                                      L"files tens of times larger than a lossy export, and play them in VLC / mpv - "
                                      L"browsers and the Windows player will not open them. Best used as a master to "
                                      L"re-encode from.");
        window->setRowEnabled(bitrateRow, !lossless);
        window->registerSectionHeader(L"HDR Output");
        // The nits row only means anything under PQ, whose code values are absolute brightness.
        auto peakRow = std::make_shared<HWND>(nullptr);
        window->registerRadioButtonInput<VidHdrTransfer>(L"Transfer", &hdrTransfer,
                                                         [winPtr = window.get(), peakRow, transferPtr = &hdrTransfer] {
                                                             if (*peakRow != nullptr) {
                                                                 winPtr->setRowEnabled(
                                                                     *peakRow, *transferPtr == VidHdrTransfer::PQ);
                                                             }
                                                         }, L"Set Transfer",
                                                         L"What the exported pixels mean. SDR is the tone-mapped 8-bit picture every earlier version wrote. HDR10 (PQ) and HLG send 10-bit BT.2020 through x265 instead, carrying the light Bloom puts above white rather than clipping it. Both need Shader > HDR switched on - without the float chain there is nothing above white to carry - and both ignore Lossless, which is an RGB x264 mode x265 has no equivalent for. The preview window shows the encoded values directly, so it looks flat while an HDR export runs; the file is what is correct.");
        *peakRow = window->registerTextInput<float>(L"Peak Brightness (nits)", &hdrPeakNits,
                                                    Unparser::floatFixed(0), Parser::FLOAT,
                                                    ValidCondition::floatInRange(100.0f, 10000.0f),
                                                    Callback::NOTHING, L"Set Peak Brightness",
                                                    L"The display brightness the HDR headroom lands on, in nits, and what the file is mastered for. 1000 is the usual HDR10 target; 4000 and 10000 exist for brighter displays. Only PQ stores absolute brightness, so HLG ignores this.\nUp/Down arrows nudge by 100 (Shift = 1000).", 100.0);
        window->setRowEnabled(*peakRow, hdrTransfer == VidHdrTransfer::PQ);
        window->registerSectionHeader(L"Transition Cleanup");
        window->registerTextInput<uint32_t>(L"Keyframe-boundary anti-aliasing", &keyframeAA, Unparser::U_LONG, Parser::U_LONG,
                                            [](const uint32_t &v) { return v >= 1 && v <= 8; },
                                            Callback::NOTHING, L"Keyframe-boundary anti-aliasing",
                                            L"Removes the single-frame brightness pop at each keyframe transition by "
                                            L"supersampling only those frames (NxN). 1 = off, 2..4 typical (4 removes it). "
                                            L"Only the few transition frames are slower.");
        window->registerTextInput<uint32_t>(L"Color-animation anti-aliasing", &colorAA, Unparser::U_LONG, Parser::U_LONG,
                                            [](const uint32_t &v) { return v >= 1 && v <= 8; },
                                            Callback::NOTHING, L"Color-animation anti-aliasing",
                                            L"Removes Psychedelic / Color-Animation-Speed judder by rendering each "
                                            L"frame N times across its time slice and averaging (temporal "
                                            L"supersampling). 1 = off, 2..4 typical. Every frame is N times slower, "
                                            L"so raise it only when the color flow looks stuttery.");
        window->registerSectionHeader(L"Keyframe Files");
        window->registerCheckboxInput(L"Compress keyframes", &compressKeyframes, Callback::NOTHING,
                                      L"Compress keyframes",
                                      L"Writes generated keyframes as .rfmz instead of .rfm, taking 2 to 4 times less "
                                      L"room for the same folder - how much depends on the view. Nothing is given up: "
                                      L"the packing is lossless, so the video comes out identical either way. Writing "
                                      L"a keyframe takes about 50 ms longer at 1920x1080, against the seconds or "
                                      L"minutes its rendering already takes. Video export reads both forms, and a "
                                      L"folder may hold a mix of them.");
        window->registerSectionHeader(L"Automation");
        window->registerCheckboxInput(L"Auto-create video after keyframes", &autoCreateVideo, Callback::NOTHING,
                                      L"Auto-create video after keyframes",
                                      L"When enabled, the video is built automatically once keyframe generation "
                                      L"finishes, saved with a timestamped file name. When disabled (default), only "
                                      L"the keyframes are generated and you export the video manually.");
        window->registerSectionHeader(L"Performance");
        window->registerCheckboxInput(L"Pause preview during video export", &pauseMainPreview, Callback::NOTHING,
                                      L"Pause preview during video export",
                                      L"Holds the main window's picture from the moment Export Zooming Video is "
                                      L"started - the folder and file prompts included - so the whole GPU goes to the "
                                      L"export. The picture resumes on its own when the export ends.");
        window->registerCheckboxInput(L"Pause keyframe preview", &pauseKeyframePreview,
                                      Callback::NOTHING,
                                      L"Pause keyframe preview",
                                      L"The same while keyframes are being generated. Each keyframe still has to be "
                                      L"computed, so this saves less than the export does - only the drawing of the "
                                      L"finished keyframe to the window. The status bar keeps reporting the zoom "
                                      L"ratio, period and elapsed time throughout.");
        window->registerHelpButton(
            L"Export Guide",
            {
                {L"Frame Rate controls smoothness.",
                 L"Higher FPS makes motion smoother but increases render time and file size."},
                {L"Bitrate controls compression quality.",
                 L"Raise it when the final video shows blocky compression artifacts."},
            });

        window->setWindowCloseFunction([&settingsMenu] {
            settingsMenu.setCurrentActiveSettingsWindow(nullptr);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window));
    };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackVideo::GENERATE_VID_KEYFRAME = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        // Generation writes the shader itself (a static video disables stripe/slope/fog/bloom), so
        // the shader panels go; the rest are left open. Closed here rather than on the worker below:
        // a window can only be destroyed from the thread that owns it, and that is this one.
        settingsMenu.closeShaderSettingsWindows();
        // Generation is nothing but computed views, so it answers a recovery still waiting for
        // approval: held back, its first keyframe would never come.
        scene.setComputeHold(false);
        scene.getBackgroundThreads().createThread(
            [&scene](BackgroundThread &thread) {
                ScopedVideoLock lock(scene);
                const auto &state = scene.getState();
                const auto dirPtr = IOUtilities::ioDirectoryDialog(L"Folder to generate keyframes");

                float &logZoom = scene.getAttribute().fractal.logZoom;
                if (dirPtr == nullptr) {
                    return;
                }

                if (const HWND hwnd = scene.getWindowContext().getWindow().getWindowHandle(); !IsWindow(hwnd) || !IsWindowVisible(hwnd)) {
                    MessageBoxW(nullptr, L"Target Window already been destroyed", L"FATAL", MB_OK | MB_ICONERROR);
                    return;
                }

                const auto &dir = *dirPtr;
                if (containsExistingVideoKeyframes(dir) &&
                    MessageBoxW(scene.getWindowContext().getWindow().getWindowHandle(),
                                L"The selected folder already contains .rfmz, .rfm or .png files.\n\n"
                                L"Generating keyframes here may mix existing and newly generated files. Continue?",
                                L"Existing keyframes found",
                                MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) {
                    return;
                }
                bool nextFrame = false;
                Attribute &settings = scene.getAttribute();
                const VideoAttribute &videoSettings = settings.video;

                // Every wait here can also end because the thread was asked to stop, which is what
                // a shutdown does: the render loop that would answer the request is already gone,
                // so there is nothing left to wait for and the run leaves instead.
                const auto idle = [&scene] {
                    return !scene.getRequests().recomputeRequested && scene.isIdleCompute();
                };
                if (videoSettings.data.isStatic) {
                    settings.shader.stripe = ShdStripePresets::Disabled().genStripe();
                    settings.shader.slope = ShdSlopePresets::Disabled().genSlope();
                    settings.shader.fog = ShdFogPresets::Disabled().genFog();
                    settings.shader.bloom = BloomPresets::Disabled().genBloom();
                    scene.getRequests().requestShader();
                    if (!thread.waitUntil([&scene] { return !scene.getRequests().shaderRequested; })) {
                        return;
                    }
                }
                const float increment = std::log10(videoSettings.data.defaultZoomIncrement);
                while (logZoom > Constants::Fractal::ZOOM_MIN) {
                    if (state.interruptRequested() || nextFrame) {
                        //incomplete frame
                        scene.getRequests().requestRecompute();
                    }
                    // Waited on every pass, not only after a request of its own: a compute already
                    // running when generation started is still filling the map this would export.
                    if (!thread.waitUntil(idle)) {
                        return;
                    }
                    if (state.interruptRequested()) {
                        return;
                    }
                    if (videoSettings.data.isStatic) {
                        const std::filesystem::path path = IOUtilities::generateFileName(
                            dir, Constants::Extension::IMAGE);
                        scene.getRequests().requestCreateImage(path, false);
                        if (!thread.waitUntil([&scene] { return !scene.getRequests().createImageRequested; })) {
                            return;
                        }
                        RFFStaticMapBinary(logZoom, scene.getIterationBufferWidth(settings), scene.getIterationBufferHeight(settings)).exportAsKeyframe(dir);
                    } else {
                        scene.generateMap().exportAsKeyframe(dir, videoSettings.exportation.compressKeyframes);
                    }
                    logZoom -= increment;
                    nextFrame = true;
                }

                if (state.interruptRequested()) {
                    vkh::logger::w_log(L"Keyframe generation cancelled.");
                    return;
                }

                if (!videoSettings.exportation.autoCreateVideo) {
                    vkh::logger::w_log(L"Keyframe generation complete. Auto video creation is disabled.");
                    return;
                }

                // Timestamped file name so each auto-export is kept separately (rff_YYYYMMDD_HHMMSS.mp4).
                SYSTEMTIME lt;
                GetLocalTime(&lt);
                wchar_t nameBuf[64];
                swprintf(nameBuf, std::size(nameBuf), L"rff_%04u%02u%02u_%02u%02u%02u.%ls",
                         static_cast<unsigned>(lt.wYear), static_cast<unsigned>(lt.wMonth),
                         static_cast<unsigned>(lt.wDay), static_cast<unsigned>(lt.wHour),
                         static_cast<unsigned>(lt.wMinute), static_cast<unsigned>(lt.wSecond),
                         videoSettings.exportation.lossless
                             ? Constants::Extension::VIDEO_LOSSLESS
                             : Constants::Extension::VIDEO);
                const auto saveFile = dir / nameBuf;
                ScopedVideoExport exportScope(scene);
                VideoWindow::createVideo(scene.engine, scene.getAttribute(), dir, saveFile);
            });
    };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackVideo::EXPORT_ZOOM_VID = [
            ](SettingsMenu &settingsMenu, RenderScene &scene) {
        // The export runs off the attribute for as long as it lasts, so every panel closes. Closed
        // here rather than on the worker below: a window can only be destroyed from its own thread.
        settingsMenu.closeAllSettingsWindows();
        // The panel a recovery is waiting on has just been closed with the rest, so the view it was
        // holding is released: nothing would be left to press once the export is over.
        scene.setComputeHold(false);
        scene.getBackgroundThreads().createThread([&scene](const BackgroundThread &) {
            // Opened before the prompts, not after them: the export is under way from the moment it
            // is chosen, and the preview it holds back would otherwise go on drawing at full rate
            // for as long as the user takes to pick a folder and a file name.
            ScopedVideoExport exportScope(scene);
            const auto openPtr = IOUtilities::ioDirectoryDialog(L"Select Sample Keyframe folder");

            if (openPtr == nullptr) {
                return;
            }
            const auto &open = *openPtr;
            // Offer the container the export will actually write, so the name picked here is the name that appears.
            const auto savePtr = IOUtilities::ioFileDialog(L"Save Video Location", Constants::Extension::DESC_VIDEO, IOUtilities::SAVE_FILE,
                                                           scene.getAttribute().video.exportation.lossless
                                                               ? Constants::Extension::VIDEO_LOSSLESS
                                                               : Constants::Extension::VIDEO);
            if (savePtr == nullptr) {
                return;
            }
            const auto &save = *savePtr;
            VideoWindow::createVideo(scene.engine, scene.getAttribute(), open, save);
        });
    };
}
