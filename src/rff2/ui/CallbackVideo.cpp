//
// Created by Merutilm on 2025-06-08.
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-12, 2026-08-14, 2026-08-18, 2026-08-19, 2026-08-26, 2026-08-31
// Modified by GPT-5 on 2026-08-18, 2026-08-21, 2026-08-23, 2026-08-24, 2026-08-31
// Modified by GPT-6 on 2026-09-08, 2026-09-14, 2026-09-15, 2026-09-18, 2026-09-22, 2026-09-23, 2026-09-24, 2026-09-25, 2026-09-26, 2026-10-02
// Modified by Opus 5.5 on 2026-10-04
//

#include "NativeDialogs.hpp"
#include "CallbackVideo.hpp"
#include "../io/PreferencesIO.h"
#include "../io/ConfigIO.h"

#include <cwchar>
#include <fstream>
#include <cmath>

#include "../constants/Constants.hpp"
#include "IOUtilities.h"
#include "Callback.hpp"
#include "../attr/NumericSettingLimits.hpp"
#include "TimelineWindow.hpp"
#include "VideoWindow.hpp"
#include "../io/RFFStaticMapBinary.h"
#include "../io/MapLimits.hpp"
#include "../preset/shader/bloom/ShdBloomPresets.h"
#include "../preset/shader/fog/ShdFogPresets.h"
#include "../preset/shader/slope/ShdSlopePresets.h"
#include "../preset/shader/stripe/ShdStripePresets.h"

namespace merutilm::rff2 {

    const std::function<void(SettingsMenu &, RenderScene &)> CallbackVideo::TIMELINE_EDITOR =
        [](SettingsMenu &settingsMenu, RenderScene &scene) {
            if (settingsMenu.workspaceNavigation && !PreferencesIO::legacySettingsUI()) {
                settingsMenu.workspaceNavigation(2, 3);
                return;
            }
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
        RenderScene &scene;
        ScopedVideoLock(RenderScene &s) : scene(s) {
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

    const std::function<void(SettingsMenu &, RenderScene &)> CallbackVideo::DATA_SETTINGS = [](SettingsMenu &
                                                                                                   settingsMenu,
                                                                                               RenderScene
                                                                                                   &scene) {
        auto &[defaultZoomIncrement, isStatic, cameraPadding, cameraScale, sourceScale] =
            scene.getAttribute().video.data;
        auto window = std::make_unique<SettingsWindow>(L"Set Data");

        window->registerSectionHeader(L"Keyframes", false);
        window->registerTextInput<float>(
            L"Zoom Step per Keyframe", &defaultZoomIncrement, Unparser::FLOAT, Parser::FLOAT,
            [](const float &v) { return std::isfinite(v) && v > 1; }, Callback::NOTHING, L"Set zoom step per keyframe",
            L"How many times the view is magnified from one keyframe to the next; 2 means 2x. Greater than 1.");

        window->registerCheckboxInput(
            L"Rotation / 360 padding", &cameraPadding, Callback::NOTHING, L"Rotation / 360 padding",
            L"Adds coverage to planar keyframes while preserving the current rotation. 360 projections keep their current framing without padding. Regenerate existing keyframes to add coverage. The output keeps its original size. All-angle rotation needs a scale at least diagonal / shorter side; the value is raised automatically when necessary.");
        window->registerTextInput<uint32_t>(
            L"Camera padding scale", &cameraScale, Unparser::U_LONG, Parser::U_LONG,
            [](const uint32_t &v) { return v >= 2 && v <= 64; }, Callback::NOTHING, L"Camera padding scale",
            L"2 to 64. Width and height are multiplied by this value, increasing memory and disk usage by its square. 360 Panorama Range is capped by the saved coverage; increase this before generation to show more distant detail.");
        window->registerSectionHeader(L"Source Mode");
        window->registerCheckboxInput(
            L"Render from PNG images", &isStatic, Callback::NOTHING, L"Render from PNG images",
            L"Builds the video from PNG images instead of data files. A PNG is a finished picture and carries no iteration data, so the stripe and the slope are unavailable; Color, Fog and Bloom still run over it and the Timeline Editor can animate them, which is how a PNG video is faded, exposed or hazed.");

        window->setWindowCloseFunction(
            [&settingsMenu] { settingsMenu.setCurrentActiveSettingsWindow(nullptr); });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window));
    };

    const std::function<void(SettingsMenu &, RenderScene &)> CallbackVideo::CAMERA_SETTINGS = [](SettingsMenu &
                                                                                                     settingsMenu,
                                                                                                 RenderScene
                                                                                                     &scene) {
        auto &camera = scene.getAttribute().shader.camera;
        auto &timeline = scene.getAttribute().video.timeline;
        auto window = std::make_unique<SettingsWindow>(L"Video Camera / Rotation / 360\u00B0");
        window->registerSectionHeader(L"Video Camera", false);
        window->registerButton(L"Match Planar Framing", L"Match Planar Framing", [&scene] {
            const HWND timeline = scene.getRequests().shaderEditListener.load(std::memory_order_acquire);
            if (IsWindow(timeline)) TimelineWindow::matchCameraToPlanar(timeline);
            else NativeDialogs::message(nullptr, L"Open the Timeline Editor and load keyframes before matching the camera framing.", L"Match Planar Framing", MB_OK | MB_ICONINFORMATION);
        }, L"Match Planar Framing", L"At the playhead, match the planar scale using 360 Camera, Ground layout and Pitch -90. Rotation is preserved.");
        window->registerStaticText(L"Changes are recorded at the Timeline Editor playhead.");
        window->registerStaticText(
            L"Rotation and 360 modes require Rotation / 360 padding when generating keyframes.");
        window->registerSelectionInput<VidRotationMode>(
            L"Rotation Mode", &timeline.rotationMode, [&scene] { scene.getRequests().requestShader(); },
            L"Rotation Mode",
            L"Constant Period replaces rotation keys and keeps turning during zoom holds. Other camera tracks still apply.");
        window->registerTextInput<float>(
            L"Seconds per Turn", &timeline.rotationPeriod, Unparser::floatTrim(3), Parser::FLOAT,
            [](const float &v) { return std::isfinite(v) && v >= 0.01f && v <= 86400.0f; },
            [&scene] { scene.getRequests().requestShader(); }, L"Seconds per Turn",
            L"Duration of one complete turn, 0.01 to 86400 seconds. Used by Constant Period.");
        window->registerSelectionInput<VidRotationDirection>(
            L"Rotation Direction", &timeline.rotationDirection,
            [&scene] { scene.getRequests().requestShader(); }, L"Rotation Direction",
            L"Direction of continuous rotation. Used by Constant Period.");
        window->registerTextInput<float>(
            L"Rotation Start Angle", &timeline.rotationStartAngle, Unparser::floatTrim(2), Parser::FLOAT,
            [](const float &v) { return std::isfinite(v) && v >= -360000.0f && v <= 360000.0f; },
            [&scene] { scene.getRequests().requestShader(); }, L"Rotation Start Angle",
            L"Angle at video time zero, in degrees. Used by Constant Period.");
        window->registerTextInput<float>(
            L"Rotation", &camera.rotation, Unparser::floatFixed(1), Parser::FLOAT,
            [](const float &v) { return std::isfinite(v) && v >= -360000 && v <= 360000; },
            [&scene] { scene.getRequests().requestShader(); }, L"Rotation",
            L"Degrees. Values beyond 360 allow multiple turns. This is yaw under a 360 projection.");
        window->registerRadioButtonInput<FrtProjectionMethod>(
            L"Projection", &camera.projection, [&scene] { scene.getRequests().requestShader(); },
            L"Projection",
            L"Planar, 360 Camera, or 360 Equirectangular. The timeline holds each mode until the next key; use a 2:1 output for equirectangular video.");
        window->registerSliderInput(
            L"Pitch", &camera.pitch, -90, 90, Unparser::floatFixed(1), Parser::FLOAT,
            [](const float &v) { return std::isfinite(v) && v >= -90 && v <= 90; },
            [&scene] { scene.getRequests().requestShader(); }, L"Pitch",
            L"Camera elevation in degrees. Used by 360 Camera.");
        window->registerSliderInput(
            L"Field of View", &camera.fov, 1, 179, Unparser::floatFixed(1), Parser::FLOAT,
            [](const float &v) { return std::isfinite(v) && v >= 1 && v <= 179; },
            [&scene] { scene.getRequests().requestShader(); }, L"Field of View",
            L"Horizontal field of view in degrees. Used by 360 Camera.");
        window->registerSliderInput(
            L"Panorama Range", &camera.range, 0, 6, Unparser::floatFixed(2), Parser::FLOAT,
            [](const float &v) { return std::isfinite(v) && v >= 0 && v <= 6; },
            [&scene] { scene.getRequests().requestShader(); }, L"Panorama Range",
            L"Log10 of the projection radius limit. The saved padding caps this range; larger ranges require generating keyframes with more padding.");
        window->registerRadioButtonInput<FrtPanoramaLayout>(
            L"Layout", &camera.layout, [&scene] { scene.getRequests().requestShader(); }, L"Layout",
            L"Ground places the plane below the viewer. Full Sphere wraps it around the whole sphere.");
        window->setWindowCloseFunction(
            [&settingsMenu] { settingsMenu.setCurrentActiveSettingsWindow(nullptr); });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window));
    };

    const std::function<void(SettingsMenu &, RenderScene &)> CallbackVideo::ANIMATION_SETTINGS =
        [](SettingsMenu &settingsMenu, RenderScene &scene) {
            auto &[overZoom, showText, mps] = scene.getAttribute().video.animation;
            auto window = std::make_unique<SettingsWindow>(L"Set Animation");
            window->registerSectionHeader(L"Zoom Motion", false);
            window->registerTextInput<float>(L"Extra Final Zoom-in", &overZoom, Unparser::floatFixed(0),
                                             Parser::FLOAT, NumericSettingLimits::acceptsOverZoom,
                                             Callback::NOTHING, L"Extra Final Zoom-in",
                                             L"Adds extra zoom-in at the very end of the video, from 0 to 8.");
            window->registerTextInput<float>(L"Zoom Speed", &mps, Unparser::floatTrim(7), Parser::FLOAT,
                                             ValidCondition::POSITIVE_FLOAT, Callback::NOTHING, L"Zoom Speed",
                                             L"Number of keyframes shown per second - higher = faster zoom.");
            window->registerSectionHeader(L"Overlay");
            window->registerCheckboxInput(
                L"Show Zoom Ratio", &scene.getAttribute().video.timeline.zoomOverlay.visible,
                Callback::NOTHING, L"Show Zoom Ratio", L"Display the zoom ratio on the video.");

            window->setWindowCloseFunction(
                [&settingsMenu] { settingsMenu.setCurrentActiveSettingsWindow(nullptr); });
            settingsMenu.setCurrentActiveSettingsWindow(std::move(window));
        };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackVideo::EXPORT_SETTINGS = [](SettingsMenu &
                                                                                                     settingsMenu,
                                                                                                 RenderScene
                                                                                                     &scene) {
        auto window = std::make_unique<SettingsWindow>(L"Set Export");
        auto &[fps, bitrate, lossless, keyframeAA, colorAA, autoCreateVideo, pauseMainPreview,
               pauseKeyframePreview, compressKeyframes, hdrTransfer, hdrPeakNits, showExportPreview, gpuSubmitSplit] =
            scene.getAttribute().video.exportation;
        window->registerSectionHeader(L"Video Output", false);
        window->registerTextInput<float>(
            L"Frame Rate (FPS)", &fps, Unparser::floatFixed(2), Parser::FLOAT,
            NumericSettingLimits::acceptsFps,
            Callback::NOTHING, L"Set video frame rate", L"Frames per second of the exported video, from 1 to 1000. Fractional rates are supported.");
        // The field is uint32_t, so it must be parsed as one: the U_SHORT parser masked with 0xFFFF,
        // which silently turned an entered 100000 into 34464 instead of rejecting it.
        const HWND bitrateRow = window->registerTextInput<uint32_t>(
            L"Bitrate (kbps)", &bitrate, Unparser::U_LONG, Parser::U_LONG,
            [](const uint32_t &v) { return v >= 1 && v <= 1000000; }, Callback::NOTHING, L"Set the bitrate",
            L"Video bitrate in kbps. Higher = better quality but larger file. "
            L"1..1000000 (1 Gbps). Ignored while Lossless is on.");
        // The toggle sits under the row it greys out, so it captures that row's handle directly.
        window->registerCheckboxInput(
            L"Lossless", &lossless,
            [winPtr = window.get(), bitrateRow, losslessPtr = &lossless] {
                winPtr->setRowEnabled(bitrateRow, !*losslessPtr);
            },
            L"Lossless",
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
        window->registerRadioButtonInput<VidHdrTransfer>(
            L"Transfer", &hdrTransfer,
            [winPtr = window.get(), peakRow, transferPtr = &hdrTransfer] {
                if (*peakRow != nullptr) {
                    winPtr->setRowEnabled(*peakRow, *transferPtr == VidHdrTransfer::PQ);
                }
            },
            L"Set Transfer",
            L"What the exported pixels mean. SDR is the tone-mapped 8-bit picture every earlier version wrote. HDR10 (PQ) and HLG send 10-bit BT.2020 through x265 instead, carrying the light Bloom puts above white rather than clipping it. Both need Shader > HDR switched on - without the float chain there is nothing above white to carry - and both ignore Lossless, which is an RGB x264 mode x265 has no equivalent for. The preview window shows the encoded values directly, so it looks flat while an HDR export runs; the file is what is correct.");
        *peakRow = window->registerTextInput<float>(
            L"Peak Brightness (nits)", &hdrPeakNits, Unparser::floatFixed(0), Parser::FLOAT,
            ValidCondition::floatInRange(100.0f, 10000.0f), Callback::NOTHING, L"Set Peak Brightness",
            L"The display brightness the HDR headroom lands on, in nits, and what the file is mastered for. 1000 is the usual HDR10 target; 4000 and 10000 exist for brighter displays. Only PQ stores absolute brightness, so HLG ignores this.\nUp/Down arrows nudge by 100 (Shift = 1000).",
            100.0);
        window->setRowEnabled(*peakRow, hdrTransfer == VidHdrTransfer::PQ);
        window->registerSectionHeader(L"Transition Cleanup");
        window->registerTextInput<uint32_t>(
            L"Keyframe-boundary anti-aliasing", &keyframeAA, Unparser::U_LONG, Parser::U_LONG,
            [](const uint32_t &v) { return v >= 1 && v <= 8; }, Callback::NOTHING,
            L"Keyframe-boundary anti-aliasing",
            L"Removes the single-frame brightness pop at each keyframe transition by "
            L"supersampling only those frames (NxN). 1 = off, 2..4 typical (4 removes it). "
            L"Only the few transition frames are slower.");
        window->registerTextInput<uint32_t>(
            L"Color-animation anti-aliasing", &colorAA, Unparser::U_LONG, Parser::U_LONG,
            [](const uint32_t &v) { return v >= 1 && v <= 8; }, Callback::NOTHING,
            L"Color-animation anti-aliasing",
            L"Removes Psychedelic / Color-Animation-Speed judder by rendering each "
            L"frame N times across its time slice and averaging (temporal "
            L"supersampling). 1 = off, 2..4 typical. Every frame is N times slower, "
            L"so raise it only when the color flow looks stuttery.");
        window->registerSectionHeader(L"Keyframe Files");
        window->registerCheckboxInput(
            L"Compress keyframes", &compressKeyframes, Callback::NOTHING, L"Compress keyframes",
            L"Writes generated keyframes as .rfmz instead of .rfm, taking 2 to 4 times less "
            L"room for the same folder - how much depends on the view. Nothing is given up: "
            L"the packing is lossless, so the video comes out identical either way. Writing "
            L"a keyframe takes about 50 ms longer at 1920x1080, against the seconds or "
            L"minutes its rendering already takes. Video export reads both forms, and a "
            L"folder may hold a mix of them.");
        window->registerSectionHeader(L"Automation");
        window->registerCheckboxInput(
            L"Auto-create video after keyframes", &autoCreateVideo, Callback::NOTHING,
            L"Auto-create video after keyframes",
            L"When enabled, the video is built automatically once keyframe generation "
            L"finishes, saved with a timestamped file name. When disabled (default), only "
            L"the keyframes are generated and you export the video manually.");
        window->registerSectionHeader(L"Performance");
        window->registerCheckboxInput(
            L"Show Video Export Preview", &showExportPreview, Callback::NOTHING, L"Show Video Export Preview",
            L"Shows the video while exporting. Turn off to reduce display work; progress and cancellation remain available.");
        window->registerTextInput<uint32_t>(
            L"GPU Work Split", &gpuSubmitSplit, Unparser::U_LONG, Parser::U_LONG,
            [](const uint32_t &v) { return v <= 64; }, Callback::NOTHING, L"GPU Work Split",
            L"Sends each video frame to the GPU in smaller parts, so a long export at a high resolution "
            L"or Supersampling is not stopped by VK_ERROR_DEVICE_LOST. Windows resets a GPU that is busy "
            L"with one piece of work for about 2 seconds. 0 = off (one part per frame), 1 = one part per "
            L"shader pass, 2..64 = also splits the fractal pass into that many horizontal bands. The "
            L"video is identical either way; higher values export slightly slower.");
        window->registerCheckboxInput(
            L"Pause preview during video export", &pauseMainPreview, Callback::NOTHING,
            L"Pause preview during video export",
            L"Holds the main window's picture from the moment Export Zooming Video is "
            L"started - the folder and file prompts included - so the whole GPU goes to the "
            L"export. The preview's VRAM is freed for the export as well. The picture resumes "
            L"on its own when the export ends.");
        window->registerCheckboxInput(
            L"Pause keyframe preview", &pauseKeyframePreview, Callback::NOTHING, L"Pause keyframe preview",
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

        window->setWindowCloseFunction(
            [&settingsMenu] { settingsMenu.setCurrentActiveSettingsWindow(nullptr); });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window));
    };
    static void generateVideoKeyframes(RenderScene &scene, BackgroundThread &thread,
                                       const std::filesystem::path &directory = {},
                                       const std::filesystem::path &output = {},
                                       const std::shared_ptr<ExportProgress> &progress = {}, bool *keyframesReady = nullptr) {
        const auto message = [&progress](HWND owner, const wchar_t *text, const wchar_t *title, UINT flags) {
            if (progress) {
                progress->report(ExportProgress::Phase::FAILED, text);
                throw std::runtime_error("Keyframe generation failed; see progress log");
            }
            return NativeDialogs::message(owner, text, title, flags);
        };
        ScopedVideoLock lock(scene);
        const auto &state = scene.getState();
        const auto dirPtr = directory.empty() ? IOUtilities::ioDirectoryDialog(L"Folder to generate keyframes")
                                                : std::make_unique<std::filesystem::path>(directory);

        float &logZoom = scene.getAttribute().fractal.logZoom;
        if (dirPtr == nullptr) {
            return;
        }

        if (const HWND hwnd = scene.getWindowContext().getWindow().getWindowHandle();
            !IsWindow(hwnd) || !IsWindowVisible(hwnd)) {
            message(nullptr, L"Target Window already been destroyed", L"FATAL",
                                   MB_OK | MB_ICONERROR);
            return;
        }

        const auto &dir = *dirPtr;
        if (containsExistingVideoKeyframes(dir) &&
            message(
                scene.getWindowContext().getWindow().getWindowHandle(),
                L"The selected folder already contains .rfmz, .rfm or .png files.\n\n"
                L"Generating keyframes here may mix existing and newly generated files. Continue?",
                L"Existing keyframes found", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) {
            return;
        }
        if ((scene.getAttribute().video.data.cameraPadding ||
             std::filesystem::exists(dir / "camera-source.txt")) &&
            containsExistingVideoKeyframes(dir)) {
            message(
                nullptr,
                L"Camera padding requires an empty keyframe folder. Select a new folder to avoid mixing different coverage.",
                L"Camera keyframes", MB_OK | MB_ICONERROR);
            return;
        }
        bool nextFrame = false;
        Attribute &settings = scene.getAttribute();
        const VideoAttribute &videoSettings = settings.video;
        if (!MapLimits::valid(scene.getIterationBufferWidth(settings), scene.getIterationBufferHeight(settings))) {
            message(nullptr, L"Keyframes require 1 to 200,000,000 pixels. Reduce resolution or internal scale.",
                                   L"Keyframe generation", MB_OK | MB_ICONERROR);
            return;
        }
        const float increment = std::log10(videoSettings.data.defaultZoomIncrement);
        const auto validNextZoom = [increment](const float zoom) {
            const float next = zoom - increment;
            return std::isfinite(zoom) && std::isfinite(increment) && increment > 0 &&
                   std::isfinite(next) && next < zoom;
        };
        if (!validNextZoom(logZoom)) {
            message(nullptr, L"Zoom step is invalid or too small at this depth. Increase Zoom Step per Keyframe.",
                                   L"Keyframe generation", MB_OK | MB_ICONERROR);
            return;
        }
        struct RestoreEffects {
            RenderScene &scene;
            ShdStripeAttribute stripe;
            ShdSlopeAttribute slope;
            ShdFogAttribute fog;
            ShdBloomAttribute bloom;
            bool changed;
            void restore() {
                if (!changed) return;
                auto &shader = scene.getAttribute().shader;
                shader.stripe = stripe;
                shader.slope = slope;
                shader.fog = fog;
                shader.bloom = bloom;
                scene.getRequests().requestShader();
                changed = false;
            }
            ~RestoreEffects() { restore(); }
        } restoreEffects{scene, settings.shader.stripe, settings.shader.slope,
                         settings.shader.fog, settings.shader.bloom, videoSettings.data.isStatic};

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
        if (!thread.waitUntil(idle)) {
            return;
        }
        struct RestoreCameraSource {
            RenderScene &scene;
            uint32_t sourceScale;
            bool changed = false;
            void restore() {
                if (!changed) {
                    return;
                }
                auto &a = scene.getAttribute();
                a.video.data.sourceScale = sourceScale;
                scene.getRequests().requestResize();
                scene.getRequests().requestRecompute();
                changed = false;
            }
            ~RestoreCameraSource() {
                restore();
            }
        } restore{scene, settings.video.data.sourceScale};
        if (videoSettings.data.cameraPadding &&
            effectiveProjection(settings.fractal.projectionMethod) == FrtProjectionMethod::PLANAR) {
            const double w = scene.getIterationBufferWidth(settings);
            const double h = scene.getIterationBufferHeight(settings);
            if (w <= 0 || h <= 0) {
                return;
            }
            const auto scale =
                std::max(videoSettings.data.cameraScale,
                         static_cast<uint32_t>(std::ceil((std::hypot(w, h) + 8.0) / std::min(w, h))));
            if (w <= 0 || h <= 0 || scale > 64 || w * scale > 65535 || h * scale > 65535 ||
                !MapLimits::valid(static_cast<uint32_t>(w * scale), static_cast<uint32_t>(h * scale)) ||
                settings.render.clarityMultiplier * settings.render.ssaa * scale >
                    scene.getMaxInternalScale()) {
                message(
                    nullptr,
                    L"Camera padding exceeds the GPU limit or keyframe limits (65535 per side, 200,000,000 pixels). Reduce resolution or padding scale.",
                    L"Camera keyframes", MB_OK | MB_ICONERROR);
                return;
            }
            std::ofstream metadata(dir / "camera-source.txt", std::ios::trunc);
            metadata << "RFF_CAMERA_1 " << scale << '\n';
            metadata.close();
            if (metadata.fail()) {
                message(nullptr, L"Cannot save camera-source.txt.",
                                       L"Camera keyframes", MB_OK | MB_ICONERROR);
                return;
            }
            restore.changed = true;
            settings.video.data.sourceScale = scale;
            scene.getRequests().requestResize();
            nextFrame = true;
        } else {
            std::error_code metadataError;
            std::filesystem::remove(dir / "camera-source.txt", metadataError);
            if (metadataError) {
                message(nullptr, L"Cannot remove stale camera-source.txt.",
                                       L"Camera keyframes", MB_OK | MB_ICONERROR);
                return;
            }
        }
        while (logZoom > Constants::Fractal::ZOOM_MIN) {
            if (thread.stopToken().stop_requested()) return;
            if (progress) progress->report(ExportProgress::Phase::RENDERING,
                L"Generating keyframes; log zoom " + std::to_wstring(logZoom));
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
            if (!validNextZoom(logZoom)) {
                message(nullptr, L"Zoom step no longer advances at this depth. Increase Zoom Step per Keyframe.",
                                       L"Keyframe generation", MB_OK | MB_ICONERROR);
                return;
            }
            scene.updateIterationStatus();
            const uint32_t keyframeId = (videoSettings.data.isStatic
                ? IOUtilities::fileNameCount(dir, Constants::Extension::IMAGE)
                : RFFDynamicMapBinary::keyframeCount(dir)) + 1;
            const auto configPath = dir / IOUtilities::fileNameFormat(keyframeId, Constants::Extension::CONFIG);
            std::error_code configError;
            if (std::filesystem::exists(configPath, configError) || configError) {
                message(nullptr, L"Cannot save keyframe settings: the file already exists or the folder is inaccessible.",
                                       L"Keyframe generation", MB_OK | MB_ICONERROR);
                return;
            }
            auto frameSettings = settings;
            frameSettings.fractal.maxIteration = scene.getMapMaxIteration();
            if (restoreEffects.changed) {
                frameSettings.shader.stripe = restoreEffects.stripe;
                frameSettings.shader.slope = restoreEffects.slope;
                frameSettings.shader.fog = restoreEffects.fog;
                frameSettings.shader.bloom = restoreEffects.bloom;
            }
            if (restore.changed) {
                frameSettings.video.data.sourceScale = restore.sourceScale;
            }
            const auto frameSize = scene.documentCanvasSize();
            if (videoSettings.data.isStatic) {
                const uint32_t imageCount = IOUtilities::fileNameCount(dir, Constants::Extension::IMAGE);
                const uint32_t mapCount = IOUtilities::fileNameCount(dir, Constants::Extension::STATIC_MAP);
                if (imageCount != mapCount) {
                    message(nullptr, L"The keyframe folder has unmatched images and maps.",
                                           L"Keyframe generation", MB_OK | MB_ICONERROR);
                    return;
                }
                const auto imagePath = dir / IOUtilities::fileNameFormat(imageCount + 1, Constants::Extension::IMAGE);
                const auto mapPath = dir / IOUtilities::fileNameFormat(mapCount + 1, Constants::Extension::STATIC_MAP);
                auto progress = std::make_shared<ExportProgress>();
                scene.getRequests().requestCreateImage(imagePath, false, progress);
                if (!thread.waitUntil(
                        [&scene] { return !scene.getRequests().createImageRequested; })) {
                    return;
                }
                if (progress->snapshot().phase != ExportProgress::Phase::COMPLETED) {
                    message(nullptr, L"Cannot save the keyframe image.",
                                           L"Keyframe generation", MB_OK | MB_ICONERROR);
                    return;
                }
                RFFStaticMapBinary(logZoom, scene.getIterationBufferWidth(settings),
                                   scene.getIterationBufferHeight(settings), scene.getMapMaxIteration())
                    .exportFile(mapPath);
                std::error_code mapError;
                if (!std::filesystem::is_regular_file(mapPath, mapError)) {
                    std::error_code removeError;
                    std::filesystem::remove(imagePath, removeError);
                    message(nullptr, L"Cannot save the keyframe map.",
                                           L"Keyframe generation", MB_OK | MB_ICONERROR);
                    return;
                }
            } else {
                const auto map = scene.generateMap();
                const uint32_t id = keyframeId;
                const bool compressed = videoSettings.exportation.compressKeyframes;
                const auto path = dir / IOUtilities::fileNameFormat(
                    id, compressed ? Constants::Extension::COMPRESSED_MAP : Constants::Extension::DYNAMIC_MAP);
                std::error_code mapError;
                const bool saved = compressed ? map.exportCompressedFile(path)
                                              : (map.exportFile(path), std::filesystem::is_regular_file(path, mapError));
                if (!saved) {
                    message(nullptr, L"Cannot save the keyframe map.",
                                           L"Keyframe generation", MB_OK | MB_ICONERROR);
                    return;
                }
            }
            if (!ConfigIO::save(configPath, frameSettings, frameSize.cx, frameSize.cy)) {
                message(nullptr, L"Cannot save the keyframe settings. Generation has stopped.",
                                       L"Keyframe generation", MB_OK | MB_ICONERROR);
                return;
            }
            logZoom -= increment;
            nextFrame = true;
        }

        if (state.interruptRequested()) {
            vkh::logger::w_log(L"Keyframe generation cancelled.");
            return;
        }

        restore.restore();
        restoreEffects.restore();
        if (keyframesReady) *keyframesReady = true;
        if (!progress && !videoSettings.exportation.autoCreateVideo) {
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
                 videoSettings.exportation.lossless ? Constants::Extension::VIDEO_LOSSLESS
                                                    : Constants::Extension::VIDEO);
        const auto saveFile = output.empty() ? dir / nameBuf : output;
        ScopedVideoExport exportScope(scene);
        VideoWindow::createVideo(scene.engine, scene.getAttribute(), dir, saveFile, progress, thread.stopToken());
    }

    void CallbackVideo::automaticVideo(RenderScene &scene, const std::filesystem::path &directory,
                                      const std::filesystem::path &output,
                                      const std::shared_ptr<AutomaticJob> &job, bool exportOnly) {
        if (scene.getVideoGenerationActive() || scene.getVideoExportActive())
            throw std::runtime_error("Another video operation is active");
        if (directory.empty() || output.empty() || std::filesystem::exists(output))
            throw std::runtime_error("Automatic video needs a new output path");
        if (!exportOnly && containsExistingVideoKeyframes(directory))
            throw std::runtime_error("Automatic keyframes require a fresh folder");
        job->progress->report(ExportProgress::Phase::QUEUED, L"Starting automatic video...");
        scene.setComputeHold(false);
        scene.setVideoGenerationActive(true);
        try {
            scene.getBackgroundThreads().createThread([&scene, directory, output, job, exportOnly](BackgroundThread &thread) {
                struct Done {
                    RenderScene &scene;
                    std::shared_ptr<AutomaticJob> job;
                    ~Done() { scene.setVideoGenerationActive(false); job->done = true; }
                } done{scene, job};
                ExportCompletion completion{job->progress};
                std::stop_callback onCancel(job->stop.get_token(), [&thread] { thread.requestStop(); });
                try {
                    if (thread.stopToken().stop_requested()) return;
                    if (exportOnly) {
                        job->keyframesReady = true;
                        ScopedVideoExport exportScope(scene);
                        VideoWindow::createVideo(scene.engine, scene.getAttribute(), directory, output,
                                                 job->progress, thread.stopToken());
                    } else {
                        generateVideoKeyframes(scene, thread, directory, output, job->progress, &job->keyframesReady);
                    }
                } catch (const std::exception &e) {
                    if (job->progress->snapshot().phase != ExportProgress::Phase::FAILED)
                        job->progress->report(ExportProgress::Phase::FAILED, UiLanguage::utf8(e.what()));
                } catch (...) {
                    job->progress->report(ExportProgress::Phase::FAILED, L"Unexpected video failure.");
                }
            });
        } catch (...) {
            scene.setVideoGenerationActive(false);
            throw;
        }
    }

    const std::function<void(SettingsMenu &, RenderScene &)> CallbackVideo::GENERATE_VID_KEYFRAME =
        [](SettingsMenu &settingsMenu, RenderScene &scene) {
            // Generation writes the shader itself (a static video disables stripe/slope/fog/bloom), so
            // the shader panels go; the rest are left open. Closed here rather than on the worker below:
            // a window can only be destroyed from the thread that owns it, and that is this one.
            settingsMenu.closeShaderSettingsWindows();
            // Generation is nothing but computed views, so it answers a recovery still waiting for
            // approval: held back, its first keyframe would never come.
            scene.setComputeHold(false);
            scene.getBackgroundThreads().createThread([&scene](BackgroundThread &thread) {
                generateVideoKeyframes(scene, thread);
            });
        };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackVideo::EXPORT_ZOOM_VID =
        [](SettingsMenu &settingsMenu, RenderScene &scene) {
            // The export runs off the attribute for as long as it lasts, so every panel closes. Closed
            // here rather than on the worker below: a window can only be destroyed from its own thread.
            settingsMenu.closeAllSettingsWindows();
            // The panel a recovery is waiting on has just been closed with the rest, so the view it was
            // holding is released: nothing would be left to press once the export is over.
            scene.setComputeHold(false);
            scene.getBackgroundThreads().createThread([&scene](const BackgroundThread &thread) {
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
                const auto savePtr = IOUtilities::ioFileDialog(
                    L"Save Video Location", Constants::Extension::DESC_VIDEO, IOUtilities::SAVE_FILE,
                    scene.getAttribute().video.exportation.lossless ? Constants::Extension::VIDEO_LOSSLESS
                                                                    : Constants::Extension::VIDEO);
                if (savePtr == nullptr) {
                    return;
                }
                const auto &save = *savePtr;
                VideoWindow::createVideo(scene.engine, scene.getAttribute(), open, save, {}, thread.stopToken());
            });
        };
}
