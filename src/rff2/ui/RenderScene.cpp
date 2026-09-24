//
// Created by Merutilm on 2025-08-08.
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-05, 2026-08-06, 2026-08-07, 2026-08-08, 2026-08-10, 2026-08-12, 2026-08-13, 2026-08-14, 2026-08-15, 2026-08-17, 2026-08-19, 2026-08-23, 2026-08-24, 2026-08-26, 2026-08-27, 2026-08-31, 2026-09-01, 2026-09-03, 2026-09-04
// Modified by GPT-5 on 2026-08-21, 2026-08-23, 2026-08-27, 2026-08-31, 2026-09-01
// Modified by GPT-6 on 2026-09-08, 2026-09-10, 2026-09-11, 2026-09-13, 2026-09-14, 2026-09-15, 2026-09-16, 2026-09-17, 2026-09-18, 2026-09-20, 2026-09-21, 2026-09-23, 2026-09-24, 2026-09-25
// Modified by Opus 5.5 on 2026-09-23
//

#include "NativeDialogs.hpp"
#include "RenderScene.hpp"
#include "../attr/NumericSettingLimits.hpp"
#include "VideoRenderScene.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cwctype>

#include "CallbackExplore.hpp"
#include "IOUtilities.h"
#include "../io/RFFStaticMapBinary.h"
#include "../../vulkan_helper/executor/RenderPassFullscreenRecorder.hpp"
#include "workspace/PreviewGeometry.hpp"
#include "../vulkan/RCC1.hpp"
#include "../vulkan/GPCIterationPalette.hpp"
#include "../calc/dex_exp.h"
#include "../formula/DeepMandelbrotPerturbator.h"
#include "../formula/LightMandelbrotPerturbator.h"
#include "../formula/CustomFormulaPerturbator.h"
#include "../locator/MandelbrotLocator.h"
#include "../parallel/ParallelArrayDispatcher.h"
#include "../parallel/ParallelDispatcher.h"
#include "../attr/Selectable.h"
#include "../preset/calc/CalculationPresets.h"
#include "../preset/render/RenderPresets.h"
#include "../preset/shader/bloom/ShdBloomPresets.h"
#include "../preset/shader/color/ShdColorPresets.h"
#include "../preset/shader/fog/ShdFogPresets.h"
#include "../preset/shader/palette/ShdPalettePresets.h"
#include "../preset/shader/slope/ShdSlopePresets.h"
#include "../preset/shader/stripe/ShdStripePresets.h"
#include "../vulkan/RCC4.hpp"
#include "../vulkan/RCCDownsampleForBlur.hpp"
#include "../vulkan/RCC3.hpp"
#include "../vulkan/RCC5.hpp"
#include "../vulkan/RCCPresent.hpp"
#include "../vulkan/RCCStatic2Image.hpp"
#include "../vulkan/SharedDescriptorTemplate.hpp"
#include "../vulkan/SharedImageContextIndices.hpp"
#include "opencv2/opencv.hpp"
#include "../io/KFRColorLoader.hpp"
#include "../io/RecoveryIO.h"


namespace merutilm::rff2 {
    namespace {
        void prepareVideoPipelines(vkh::EngineRef engine, const Attribute &source, bool hdr = false) {
            HWND window = CreateWindowExW(0, L"STATIC", L"", WS_POPUP, 0, 0, 64, 64,
                                          nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
            if (!window) {
                throw std::runtime_error("Failed to create video preparation window");
            }
            struct WindowScope {
                vkh::EngineRef engine;
                HWND window;
                uint32_t index = 1;
                bool attached = false;
                ~WindowScope() {
                    if (attached) {
                        engine.detachWindowContext(index);
                    }
                    DestroyWindow(window);
                }
            } scope{engine, window, hdr ? Constants::VulkanWindow::VIDEO_PREPARATION_WINDOW_ATTACHMENT_INDEX
                                        : Constants::VulkanWindow::VIDEO_WINDOW_ATTACHMENT_INDEX};
            auto attribute = source;
            attribute.render.ssaa = 1;
            attribute.video.data.sourceScale = 1;
            attribute.shader.slope.studio.use = false;
            attribute.shader.layerOrder.enabled = false;
            auto *context = engine.attachWindowContext(window, scope.index);
            scope.attached = true;
            attribute.shader.hdr.use = hdr;
            const std::function<void()> prepareNext = hdr ? std::function<void()>{}
                                                          : [&] { prepareVideoPipelines(engine, source, true); };
            VideoRenderScene scene(engine, *context, {32, 32}, attribute, prepareNext);
        }
    }

    RenderScene::RenderScene(vkh::EngineRef engine, vkh::WindowContextRef wc,
                             std::array<std::wstring, Constants::Status::LENGTH> *
                             statusMessageRef, std::mutex *statusMessageMutexRef) : EngineHandler(
                                                     engine),
                                                 wc(wc), attr(genDefaultAttr()),
                                                 statusMessageRef(statusMessageRef),
                                                 statusMessageMutexRef(statusMessageMutexRef) {
        RenderScene::init();
    }

    RenderScene::~RenderScene() {
        RenderScene::destroy();
    }

    void RenderScene::init() {
        refreshCanvasExtent();
        refreshSharedImgContext();
        attachRenderContext();
        initRenderer(true);
        refreshRenderContext();
        refreshResizeParams();
        applyShaderAttr(attr);
        wndRequestFPS();
        requests.requestRecompute();
    }


    void RenderScene::attachRenderContext() const {
        const auto swapchainImageContextGetter = [this] {
            auto &swapchain = wc.getSwapchain();
            return vkh::ImageContext::fromSwapchain(swapchain);
        };
        wc.attachRenderContext<RCC0>(wc.core,
                                     [this] { return getInternalImageExtent(); },
                                     swapchainImageContextGetter);
        wc.attachRenderContext<RCC1>(wc.core,
                                     [this] { return getInternalImageExtent(); },
                                     swapchainImageContextGetter);
        wc.attachRenderContext<RCC2>(wc.core,
                                     [this] { return getInternalImageExtent(); },
                                     swapchainImageContextGetter);
        wc.attachRenderContext<RCCDownsampleForBlur>(wc.core,
                                                     [this] { return getBlurredImageExtent(); },
                                                     swapchainImageContextGetter);
        wc.attachRenderContext<RCC3>(wc.core,
                                     [this] { return getInternalImageExtent(); },
                                     swapchainImageContextGetter);
        wc.attachRenderContext<RCC4>(wc.core,
                                     [this] { return getInternalImageExtent(); },
                                     swapchainImageContextGetter);
        wc.attachRenderContext<RCC5>(wc.core,
                                     [this] { return getInternalImageExtent(); },
                                     swapchainImageContextGetter);
        wc.attachRenderContext<RCCPresent>(wc.core,
                                           [this] { return getSwapchainRenderContextExtent(); },
                                           swapchainImageContextGetter);
    }

    void RenderScene::resolveWindowResizeEnd() const {
        if (wc.getWindow().isUnrenderable()) {
            return;
        }
        wc.core.getLogicalDevice().waitDeviceIdle();

        vkh::SwapchainRef swapchain = wc.getSwapchain();
        swapchain.recreate();
    }

    void RenderScene::recoverStaleSwapchain() const {
        if (wc.getWindow().isUnrenderable()) {
            return;
        }
        // Pending output/quality changes rebuild attachments and framebuffers together in applyResize.
        if (requests.resizeRequested) {
            return;
        }
        // Only a swapchain that no longer matches its surface is worth rebuilding. Some drivers
        // report SUBOPTIMAL for a reason a recreate cannot clear - HDR metadata, a scaling mode -
        // and rebuilding on every one of those would spin the render thread without ever settling.
        const auto [surfaceWidth, surfaceHeight] = wc.getSwapchain().populateSwapchainExtent();
        const auto [swapchainWidth, swapchainHeight] = wc.getSwapchain().getCurrentExtent();
        if (surfaceWidth == swapchainWidth && surfaceHeight == swapchainHeight) {
            return;
        }
        // The canvas size is left where it is, so the iteration buffer and the images drawn into stay the size they were built at together and the map already computed stays on screen, rescaled onto the new present images by the last pass.
        wc.core.getLogicalDevice().waitDeviceIdle();
        wc.getSwapchain().recreate();
        wc.getRenderContext(RCCPresent::CONTEXT_INDEX).recreate();
        renderer->rendererPresent->renderContextRefreshed();
        const auto [presentWidth, presentHeight] = getSwapchainRenderContextExtent();
        renderer->rendererPresent->setRescaledResolution({presentWidth, presentHeight});
    }


    void RenderScene::endSmoothZoom() {
        if (!smoothZoomActive) return;
        waitFramesInFlight();
        renderer->rendererPresent->heldFrame = -1;
        renderer->rendererPresent->setZoomTransform(1.0f, 0.0f, 0.0f);
        renderer->rendererIteration->setPreviewAnimationPaused(smoothZoomWasPaused);
        smoothZoomActive = false;
        smoothZoomDirty = false;
        smoothZoomDragging = false;
        smoothZoomGeneration = 0;
        smoothZoomCaptureWider = false;
        smoothZoomOriginal.reset();
    }

    void RenderScene::restoreSmoothZoomSource() {
        if (!smoothZoomActive || smoothZoomCaptureWider || !smoothZoomOriginal) return;
        state.cancel();
        waitFramesInFlight();
        attr.fractal = *smoothZoomOriginal;
        lastMaxIteration = smoothZoomOriginalMax;
        lastPeriod = smoothZoomOriginalPeriod;
        lastLogZoom = smoothZoomOriginalLog;
        const auto* source = reinterpret_cast<const double*>(renderer->iterationStagingBufferContext->getContext().mappedMemory);
        for (uint32_t i = 0; i < iterationMatrix->getLength(); ++i) iterationMatrix->storeRelaxed(i, source[i]);
        renderer->rendererIteration->setMaxIteration(static_cast<double>(lastMaxIteration));
        previewUploadPending = false;
        idleCompute = true;
        ++previewRevision;
    }

    bool RenderScene::beginSmoothNavigation() {
        if (smoothZoomActive) return true;
        if (!smoothZoomEnabled || renderer->lastShadedFrame < 0 || computeHold || longJobBusy.load() ||
            isVideoGenerationActive || isVideoExportActive ||
            effectiveProjection(attr.fractal.projectionMethod) != FrtProjectionMethod::PLANAR) return false;
        state.cancel();
        idleCompute = true;
        previewUploadPending = false;
        smoothZoomOriginal = attr.fractal;
        smoothZoomOriginalMax = lastMaxIteration;
        smoothZoomOriginalPeriod = lastPeriod;
        smoothZoomOriginalLog = lastLogZoom;
        smoothZoomFrom = smoothZoomTarget = smoothZoomView = {};
        smoothZoomActive = true;
        smoothZoomFailed = false;
        smoothZoomSourceExact = false;
        smoothZoomNeedsPreview = true;
        smoothZoomGeneration = 0;
        smoothZoomWasPaused = isPreviewAnimationPaused();
        renderer->rendererIteration->setPreviewAnimationPaused(true);
        renderer->rendererIteration->previewClock.held = renderer->lastShadedTime;
        renderer->rendererIteration->phases = renderer->lastShadedPhases;
        renderer->rendererPresent->heldFrame = renderer->lastShadedFrame;
        smoothZoomStarted = smoothZoomLastTick = std::chrono::steady_clock::now();
        return true;
    }

    FractalAttribute RenderScene::smoothNavigationCamera(const SmoothZoomMotion::View view) const {
        Attribute source = attr;
        source.fractal = *smoothZoomOriginal;
        auto camera = source.fractal;
        camera.logZoom = static_cast<float>(static_cast<double>(camera.logZoom) - std::log10(view.scale));
        const double w = getIterationBufferWidth(source), h = getIterationBufferHeight(source);
        const double cx = 0.5 + 0.5 / w, cy = 0.5 - 0.5 / h;
        const double dx = (view.x + view.scale * cx - cx) * w;
        const double dy = -(view.y + view.scale * cy - cy) * h;
        const double radians = camera.rotation * std::numbers::pi / 180.0;
        const double resolution = static_cast<double>(source.render.clarityMultiplier) * source.render.ssaa;
        camera.center = camera.center.addCenterDouble(
            dex::value((dx * std::cos(radians) - dy * std::sin(radians)) / resolution) / getDivisor(source),
            dex::value((dx * std::sin(radians) + dy * std::cos(radians)) / resolution) / getDivisor(source),
            Perturbator::logZoomToExp10(camera.logZoom));
        return camera;
    }

    void RenderScene::retargetSmoothNavigation() {
        smoothZoomFrom = smoothZoomView;
        smoothZoomStarted = smoothZoomInputAt = std::chrono::steady_clock::now();
        smoothZoomDirty = true;
        smoothZoomFailed = false;
        smoothZoomSourceExact = false;
        smoothZoomNeedsPreview = true;
        backgroundThreads.notifyAll();
    }

    void RenderScene::settleSmoothNavigation() {
        if (!smoothZoomActive) return;
        if (smoothZoomView.scale != smoothZoomTarget.scale || smoothZoomView.x != smoothZoomTarget.x || smoothZoomView.y != smoothZoomTarget.y)
            attr.fractal = smoothNavigationCamera(smoothZoomView);
        smoothZoomTarget = smoothZoomView;
        smoothZoomPending = 0;
        retargetSmoothNavigation();
    }

    void RenderScene::tickSmoothZoom() {
        const auto now = std::chrono::steady_clock::now();
        if (!smoothZoomEnabled && !smoothZoomActive) smoothZoomPending = 0;
        if (smoothZoomActive && now - smoothZoomLastTick > std::chrono::milliseconds(100))
            smoothZoomStarted += now - smoothZoomLastTick - std::chrono::milliseconds(100);
        smoothZoomLastTick = now;

        if (smoothZoomCaptureWider) {
            if (renderer->shadedRevision == smoothZoomCandidateRevision) return;
            renderer->rendererPresent->heldFrame = renderer->lastShadedFrame;
            smoothZoomCaptureWider = false;
            smoothZoomFrom = SmoothZoomMotion::relative(smoothZoomFrom, smoothZoomCandidate);
            smoothZoomTarget = SmoothZoomMotion::relative(smoothZoomTarget, smoothZoomCandidate);
            smoothZoomView = SmoothZoomMotion::relative(smoothZoomView, smoothZoomCandidate);
            smoothZoomOriginal = *smoothZoomCandidateFractal;
            smoothZoomOriginalMax = lastMaxIteration;
            smoothZoomOriginalPeriod = lastPeriod;
            smoothZoomOriginalLog = lastLogZoom;
            smoothZoomFrom = smoothZoomView;
            smoothZoomStarted = now;
            smoothZoomGeneration = 0;
            smoothZoomSourceExact = smoothZoomExactCandidate && smoothZoomCandidateFullQuality && !smoothZoomDirty && smoothZoomPending == 0;
            if (!smoothZoomDirty && smoothZoomPending == 0) smoothZoomNeedsPreview = false;
        }

        if (smoothZoomPending != 0 && beginSmoothNavigation()) {
            const double steps = std::clamp(smoothZoomPending, -4.0, 4.0);
            smoothZoomPending -= steps;
            const float oldZoom = attr.fractal.logZoom;
            const float newZoom = std::clamp(static_cast<float>(oldZoom + steps * Constants::Fractal::ZOOM_INTERVAL),
                                           Constants::Fractal::ZOOM_MIN, Constants::Fractal::ZOOM_DEADLINE);
            const double delta = static_cast<double>(newZoom) - oldZoom;
            if (delta != 0) {
                const auto w = getIterationBufferWidth(attr), h = getIterationBufferHeight(attr);
                const auto x = std::min<uint16_t>(smoothZoomMouseX, w - 1), y = std::min<uint16_t>(smoothZoomMouseY, h - 1);
                const double scale = std::pow(10.0, -delta);
                const auto offset = offsetConversion(attr, x, y);
                attr.fractal.logZoom = newZoom;
                attr.fractal.center = attr.fractal.center.addCenterDouble(offset[0] * (1.0 - scale), offset[1] * (1.0 - scale),
                    Perturbator::logZoomToExp10(newZoom));
                smoothZoomTarget.x += smoothZoomTarget.scale * (x + 0.5) / w * (1.0 - scale);
                smoothZoomTarget.y += smoothZoomTarget.scale * (1.0 - (y + 0.5) / h) * (1.0 - scale);
                smoothZoomTarget.scale *= scale;
                retargetSmoothNavigation();
            }
        }
        if (!smoothZoomActive) return;

        BOOL systemAnimation = TRUE;
        SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &systemAnimation, 0);
        const double seconds = smoothZoomEnabled && systemAnimation ?
            std::chrono::duration<double>(now - smoothZoomStarted).count() : SmoothZoomMotion::duration;
        const auto proposed = SmoothZoomMotion::interpolate(smoothZoomFrom, smoothZoomTarget,
            seconds * (smoothZoomDragging ? 2.0 : 1.0));
        if (SmoothZoomMotion::covered(proposed)) smoothZoomView = proposed;
        if (smoothZoomSourceExact && !smoothZoomDirty && !smoothZoomDragging && seconds >= SmoothZoomMotion::duration) {
            endSmoothZoom();
            return;
        }

        if (smoothZoomGeneration != 0 && idleCompute.load() && !smoothZoomDirty) {
            if (completedComputeGeneration.load() != smoothZoomGeneration) {
                smoothZoomGeneration = 0;
                smoothZoomFailed = true;
                previewUploadPending = false;
                setStatusMessage(Constants::Status::RENDER_STATUS, L"Navigation stopped; move again to retry or press Escape");
            } else if (SmoothZoomMotion::covered(SmoothZoomMotion::relative(smoothZoomView, smoothZoomCandidate))) {
                waitFramesInFlight();
                if (!smoothZoomCandidateFullQuality && smoothZoomPreviewMatrix) {
                    const auto pw = smoothZoomPreviewMatrix->getWidth(), ph = smoothZoomPreviewMatrix->getHeight();
                    const auto fw = iterationMatrix->getWidth(), fh = iterationMatrix->getHeight();
                    for (uint16_t y = 0; y < fh; ++y) {
                        const auto sy = SmoothZoomMotion::previewPixel(y, ph, fh);
                        for (uint16_t x = 0; x < fw; ++x) {
                            const auto sx = SmoothZoomMotion::previewPixel(x, pw, fw);
                            iterationMatrix->storeRelaxed(x, y, (*smoothZoomPreviewMatrix)(sx, sy));
                        }
                    }
                }
                snapshotComputePreview(true);
                previewUploadPending = false;
                ++previewRevision;
                renderer->rendererPresent->heldFrame = -1;
                const auto view = SmoothZoomMotion::relative(smoothZoomView, smoothZoomCandidate);
                renderer->rendererPresent->setZoomTransform(static_cast<float>(view.scale), static_cast<float>(view.x), static_cast<float>(view.y));
                smoothZoomCaptureWider = true;
                smoothZoomCandidateRevision = renderer->shadedRevision;
                return;
            }
        }

        if (!smoothZoomDragging && !smoothZoomSourceExact && !smoothZoomFailed && smoothZoomGeneration == 0 &&
            now - smoothZoomInputAt >= std::chrono::milliseconds(80) && seconds >= SmoothZoomMotion::duration)
            smoothZoomDirty = true;
        if (smoothZoomDirty &&
            (idleCompute.load() || now - smoothZoomComputeAt >= std::chrono::milliseconds(100))) {
            smoothZoomExactCandidate = SmoothZoomMotion::covered(smoothZoomTarget) && !smoothZoomDragging;
            smoothZoomCandidateFullQuality = smoothZoomExactCandidate && !smoothZoomNeedsPreview &&
                now - smoothZoomInputAt >= std::chrono::milliseconds(200);
            smoothZoomCandidate = smoothZoomExactCandidate ? smoothZoomTarget :
                SmoothZoomMotion::enclosing(smoothZoomView, smoothZoomTarget);
            Attribute settings = attr;
            settings.fractal = smoothNavigationCamera(smoothZoomCandidate);
            if (!smoothZoomExactCandidate) {
                const double wanted = smoothZoomCandidate.scale;
                double actual = std::pow(10.0, static_cast<double>(smoothZoomOriginal->logZoom) - settings.fractal.logZoom);
                if (actual < wanted) {
                    settings.fractal.logZoom = std::nextafter(settings.fractal.logZoom, -std::numeric_limits<float>::infinity());
                    actual = std::pow(10.0, static_cast<double>(smoothZoomOriginal->logZoom) - settings.fractal.logZoom);
                }
                const double cx = 0.5 + 0.5 / getIterationBufferWidth(attr), cy = 0.5 - 0.5 / getIterationBufferHeight(attr);
                smoothZoomCandidate.x += (wanted - actual) * cx;
                smoothZoomCandidate.y += (wanted - actual) * cy;
                smoothZoomCandidate.scale = actual;
            } else settings.fractal = attr.fractal;
            smoothZoomCandidateFractal = settings.fractal;
            smoothZoomDirty = false;
            smoothZoomComputeAt = now;
            idleCompute = false;
            previewUploadPending = true;
            ++previewRevision;
            recomputeThreaded(&settings, !smoothZoomCandidateFullQuality);
            smoothZoomGeneration = computeGeneration.load();
        }
        renderer->rendererPresent->setZoomTransform(static_cast<float>(smoothZoomView.scale),
            static_cast<float>(smoothZoomView.x), static_cast<float>(smoothZoomView.y));
    }

    void RenderScene::render(const bool present) {
        if (requests.defaultAttrRequested || requests.shaderRequested || requests.resizeRequested || requests.recomputeRequested) {
            if (requests.shaderRequested && !requests.defaultAttrRequested && !requests.resizeRequested && !requests.recomputeRequested)
                restoreSmoothZoomSource();
            endSmoothZoom();
            smoothZoomPending = 0;
        }
        if (requests.defaultAttrRequested) {
            applyDefaultAttr();
            requests.defaultAttrRequested.exchange(false);
            backgroundThreads.notifyAll();
        }
        if (requests.shaderRequested) {
            ++previewRevision;
            if (attr.shader.slope.lustreRelief && attr.shader.slope.reliefZoomReference < 0.0f)
                attr.shader.slope.reliefZoomReference = attr.fractal.logZoom;
            ensureShaderFormat(attr.shader.sceneLinear());
            applyShaderAttr(attr);
            // The shader is where a change lands that never recomputes, so the snapshot has to
            // follow it here as well as at a compute - throttled, because a dragged slider asks
            // for this every frame.
            writeRecoverySnapshot(false);
            requests.shaderRequested.exchange(false);
            backgroundThreads.notifyAll();
        }

        if (requests.resizeRequested) {
            ++previewRevision;
            state.cancel();
            applyResize();
            requests.resizeRequested.exchange(false);
            backgroundThreads.notifyAll();
        }

        if (requests.recomputeRequested && !computeHold) {
            ++previewRevision;
            if (attr.shader.slope.lustreRelief && attr.shader.slope.reliefZoomReference < 0.0f)
                attr.shader.slope.reliefZoomReference = attr.fractal.logZoom;
            idleCompute = false;
            previewUploadPending = true;
            // Zeroed rather than back-dated, so the first snapshot lands on the very next frame and
            // the view blanks as promptly as it did when the compute threads wrote the buffer itself.
            lastPreviewSnapshot = {};
            requests.recomputeRequested.exchange(false);
            recomputeThreaded();
            //it is threaded, not idle
        }

        // Ahead of the image request below, which renders one offscreen frame off this buffer: the
        // exact map has to be in it before that frame is recorded, or the file keeps carried-down rows.
        if (present && !wc.getWindow().isUnrenderable()) {
            tickSmoothZoom();
        }
        if (!smoothZoomActive && idleCompute.load()) {
            if (previewUploadPending.exchange(false)) {
                ++previewRevision;
                snapshotComputePreview(true);
            }
        } else if (!smoothZoomActive) {
            snapshotComputePreview(false);
        }

        renderer->rendererSlope->setReliefZoom(attr.shader.slope,
            smoothZoomCaptureWider ? smoothZoomCandidateFractal->logZoom : attr.fractal.logZoom);
        if (idleCompute.load() && !smoothZoomActive) {
            if (auto imageRequest = requests.takeCreateImageRequest()) {
                applyCreateImage(std::move(*imageRequest));
                requests.completeCreateImageRequest();
                backgroundThreads.notifyAll();
            }
        }

        if (!present) {
            return;
        }

        renderer->passTimingEnabled = passTimingEnabled;
        if (renderer->execute()) {
            // A stale swapchain, not a resize: routing this through applyResize would cancel the
            // compute and hand back a freshly allocated iteration buffer, so a driver that keeps
            // reporting SUBOPTIMAL would restart the picture from nothing on every frame.
            recoverStaleSwapchain();
        }
    }


    Attribute RenderScene::genDefaultAttr() {
        return Attribute{
            .fractal = FractalAttribute{
                .center = fp_complex("-0.85",
                                     "0",
                                     //"-1.29255707077531686131098415679305324693162987219277534742408945445699102528813182208390942132824552642640105852802031375797639923173781472397893283277669022615909880587638643429120957543820179919830492623879949932",
                                     //"-1.7433380976879299408417853435676017785972000052524291128107561584529660103218876836645852866195456038569337053542405",
                                     // "0.438169590583770312890168860021043433478705507119371935117854030759551072299659171256225012539071884716681573917133522314360175105572598172732723792994562397110248396170036793222839041625954944698185617470725880129",
                                     //"-0.00000180836819716880795128873613161993554089471597685393367018109950768833467685704762711890797154859214327088989719746641",
                                     Perturbator::logZoomToExp10(2)),
                .logZoom = 2, //186.47, //85.190033f,
                .maxIteration = 300,
                .bailout = 1e30f,
                .decimalizeIterationMethod = FrtDecimalizeIterationMethod::LOG_LOG,
                .mpaAttribute = CalculationPresets::UltraFast().genMPA(),
                .referenceCompAttribute = CalculationPresets::UltraFast().genReferenceCompression(),
                .reuseReferenceMethod = FrtReuseReferenceMethod::DISABLED,
                .autoMaxIteration = true,
                .autoIterationMultiplier = 150,
                .absoluteIterationMode = false,
                .rotation = 0.0f
            },
            .render = [] {
                auto r = RenderPresets::High().genRender();
                r.linearInterpolation = false;
                r.clarityMultiplier = 1.5f;
                return r;
            }(),
            .shader = {
                .palette = ShdPalettePresets::FromColors(KFRColorLoader::generateRandomPalette(10)).genPalette(),
                .stripe = ShdStripePresets::Disabled().genStripe(),
                .slope = ShdSlopePresets::Disabled().genSlope(),
                .color = ShdColorPresets::Disabled().genColor(),
                .fog = ShdFogPresets::Disabled().genFog(),
                .bloom = BloomPresets::Disabled().genBloom()
            },
            .video = {
                .data = {
                    .defaultZoomIncrement = 2,
                    .isStatic = false
                },
                .animation = {
                    .overZoom = 2,
                    .showText = true,
                    .mps = 1
                },
                .exportation = {
                    .fps = 60,
                    .bitrate = 65535,
                    .lossless = false,
                    .keyframeAA = 1,
                    .colorAA = 1,
                    .autoCreateVideo = false,
                    .pauseMainPreview = true,
                    .pauseKeyframePreview = false,
                    .compressKeyframes = true
                }
            }
        };
    }

    LRESULT RenderScene::renderSceneProc(const HWND hwnd, const UINT msg, const WPARAM wparam, const LPARAM lparam) {
        auto* scene = reinterpret_cast<RenderScene *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        
        if (scene != nullptr) {
            scene->runAction(msg, wparam, lparam);
        }
        
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    void RenderScene::runAction(const UINT msg, const WPARAM wparam, const LPARAM) {

        if (isVideoGenerationActive) {
            switch (msg) {
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                case WM_MOUSEMOVE:
                case WM_MOUSEWHEEL:
                    return;
                default:
                    break;
            }
        }

        switch (msg) {
            case WM_LBUTTONDOWN: {
                if (smoothZoomActive) {
                    settleSmoothNavigation();
                }
                if (isColorFreezePickActive()) {
                    const auto onPicked = std::move(colorFreezePickCallback);
                    cancelColorFreezePick();
                    SetCursor(LoadCursor(nullptr, IDC_ARROW));
                    if (idleCompute.load() && renderer->iterationStagingBufferContext != nullptr) {
                        const uint16_t px = getMouseXOnIterationBuffer();
                        const uint16_t py = getMouseYOnIterationBuffer();
                        const double it = (*renderer->iterationStagingBufferContext)(px, py);
                        auto &iters = attr.shader.palette.staticColorIterations;
                        // Skip the Mandelbrot interior (iteration 0) which never animates anyway.
                        if (it != 0 && iters.size() < ShdPaletteAttribute::MAX_STATIC_COLORS) {
                            iters.push_back(it);
                            requests.requestShader();
                        }
                    }
                    if (onPicked) {
                        onPicked();
                    }
                    break;
                }
                if (wparam & MK_SHIFT) {
                    // Begin Shift+drag box-zoom: anchor the rubber-band rectangle.
                    boxZooming = true;
                    boxStartMX = getMouseXOnIterationBuffer();
                    boxStartMY = getMouseYOnIterationBuffer();
                    GetCursorPos(&boxAnchorScreen);
                    ensureBoxZoomOverlay();
                    updateBoxZoomOverlay(boxAnchorScreen, boxAnchorScreen);
                    SetCursor(LoadCursor(nullptr, IDC_CROSS));
                    break;
                }
                SetCursor(LoadCursor(nullptr, IDC_SIZEALL));
                interactedMX = getMouseXOnIterationBuffer();
                interactedMY = getMouseYOnIterationBuffer();
                canvasDragging = true;
                smoothZoomDragging = smoothZoomActive;
                SetCapture(wc.getWindow().getWindowHandle());
                break;
            }
            case WM_LBUTTONUP: {
                if (boxZooming) {
                    boxZooming = false;
                    hideBoxZoomOverlay();
                    SetCursor(LoadCursor(nullptr, IDC_CROSS));
                    POINT cur;
                    GetCursorPos(&cur);
                    if (std::abs(cur.x - boxAnchorScreen.x) >= Constants::Win32::BOX_ZOOM_MIN_DRAG_PIXELS &&
                        std::abs(cur.y - boxAnchorScreen.y) >= Constants::Win32::BOX_ZOOM_MIN_DRAG_PIXELS) {
                        applyBoxZoom(boxStartMX, boxStartMY,
                                     getMouseXOnIterationBuffer(), getMouseYOnIterationBuffer());
                    }
                    break;
                }
                SetCursor(LoadCursor(nullptr, IDC_CROSS));
                canvasDragging = false;
                smoothZoomDragging = false;
                if (GetCapture() == wc.getWindow().getWindowHandle()) {
                    ReleaseCapture();
                }
                interactedMX = 0;
                interactedMY = 0;
                break;
            }
            case WM_MOUSEMOVE: {
                if (boxZooming) {
                    if (wparam & MK_LBUTTON) {
                        SetCursor(LoadCursor(nullptr, IDC_CROSS));
                        POINT cur;
                        GetCursorPos(&cur);
                        updateBoxZoomOverlay(boxAnchorScreen, cur);
                    } else {
                        // Button was released without a WM_LBUTTONUP reaching us: cancel.
                        boxZooming = false;
                        hideBoxZoomOverlay();
                    }
                    break;
                }
                const uint16_t x = getMouseXOnIterationBuffer();
                const uint16_t y = getMouseYOnIterationBuffer();
                if ((wparam & MK_LBUTTON) && canvasDragging) {
                    SetCursor(LoadCursor(nullptr, IDC_SIZEALL));
                    const auto dx = static_cast<int16_t>(interactedMX - x);
                    const auto dy = static_cast<int16_t>(interactedMY - y);
                    if (dx == 0 && dy == 0) {
                        break;
                    }
                    if ((dx != 0 || dy != 0) && beginSmoothNavigation()) {
                        const auto w = getIterationBufferWidth(attr), h = getIterationBufferHeight(attr);
                        smoothZoomTarget.x += smoothZoomTarget.scale * static_cast<double>(dx) / w;
                        smoothZoomTarget.y -= smoothZoomTarget.scale * static_cast<double>(dy) / h;
                        const double radians = attr.fractal.rotation * std::numbers::pi / 180.0;
                        const double resolution = static_cast<double>(attr.render.clarityMultiplier) * attr.render.ssaa;
                        attr.fractal.center = attr.fractal.center.addCenterDouble(
                            dex::value((dx * std::cos(radians) - dy * std::sin(radians)) / resolution) / getDivisor(attr),
                            dex::value((dx * std::sin(radians) + dy * std::cos(radians)) / resolution) / getDivisor(attr),
                            Perturbator::logZoomToExp10(attr.fractal.logZoom));
                        smoothZoomDragging = true;
                        retargetSmoothNavigation();
                        interactedMX = x;
                        interactedMY = y;
                        break;
                    }
                    // Dragging a 360 view turns the viewer rather than sliding a flat image, so it moves the heading and leaves the center alone.
                    if (auto &frt = attr.fractal;
                        effectiveProjection(frt.projectionMethod) != FrtProjectionMethod::PLANAR) {
                        const auto bw = static_cast<float>(std::max<uint16_t>(getIterationBufferWidth(attr), 1));
                        // A drag turns by what the pixels under it cover, so the picture follows the pointer.
                        const float degPerPixel = effectiveProjection(frt.projectionMethod) ==
                                                  FrtProjectionMethod::PERSPECTIVE_360
                                                      ? frt.panoramaFov / bw
                                                      : 360.0f / bw;
                        frt.rotation = std::fmod(frt.rotation + static_cast<float>(dx) * degPerPixel, 360.0f);
                        // The equirectangular layout keeps its nadir at the bottom, as a 360 player expects, so only the camera pitches.
                        if (effectiveProjection(frt.projectionMethod) == FrtProjectionMethod::PERSPECTIVE_360) {
                            frt.panoramaPitch = std::clamp(
                                frt.panoramaPitch + static_cast<float>(dy) * degPerPixel, -90.0f, 90.0f);
                        }
                        interactedMX = x;
                        interactedMY = y;
                        requests.requestRecompute();
                        break;
                    }
                    // iteration-buffer pixel scale is clarity * ssaa.
                    const float m = attr.render.clarityMultiplier * static_cast<float>(attr.render.ssaa);
                    const float logZoom = attr.fractal.logZoom;

                    const double radians = static_cast<double>(attr.fractal.rotation) * std::numbers::pi / 180.0;
                    const double s = std::sin(radians);
                    const double c = std::cos(radians);

                    const double rdx = static_cast<double>(dx) * c - static_cast<double>(dy) * s;
                    const double rdy = static_cast<double>(dx) * s + static_cast<double>(dy) * c;


                    fp_complex &center = attr.fractal.center;
                    center = center.addCenterDouble(dex::value(static_cast<float>(rdx) / m) / getDivisor(attr),
                                                    dex::value(static_cast<float>(rdy) / m) / getDivisor(attr),
                                                    Perturbator::logZoomToExp10(logZoom));
                    interactedMX = x;
                    interactedMY = y;
                    requests.requestRecompute();
                } else {
                    SetCursor(LoadCursor(nullptr, IDC_CROSS));
                    if (!idleCompute.load() || renderer->iterationStagingBufferContext == nullptr) {
                        return;
                    }

                    if (auto it = static_cast<uint64_t>((*renderer->iterationStagingBufferContext)(x, y)); it != 0) {
                        setStatusMessage(Constants::Status::ITERATION_STATUS,
                                         std::format(L"I : {} ({}, {})", it, x, y));
                    }
                }
                break;
            }
            case WM_CAPTURECHANGED: {
                canvasDragging = false;
                smoothZoomDragging = false;
                break;
            }
            case WM_MOUSEWHEEL: {
                if (smoothZoomEnabled && !computeHold && !longJobBusy.load() && !isVideoExportActive &&
                    effectiveProjection(attr.fractal.projectionMethod) == FrtProjectionMethod::PLANAR) {
                    smoothZoomPending += static_cast<double>(GET_WHEEL_DELTA_WPARAM(wparam)) / WHEEL_DELTA;
                    smoothZoomMouseX = getMouseXOnIterationBuffer();
                    smoothZoomMouseY = getMouseYOnIterationBuffer();
                    backgroundThreads.notifyAll();
                    break;
                }
                const int value = GET_WHEEL_DELTA_WPARAM(wparam) > 0 ? 1 : -1;
                constexpr float increment = Constants::Fractal::ZOOM_INTERVAL;

                attr.fractal.logZoom = std::max(Constants::Fractal::ZOOM_MIN,
                                                attr.fractal.logZoom);
                if (value == 1) {
                    const std::array<dex, 2> offset = offsetConversion(attr, getMouseXOnIterationBuffer(),
                                                                       getMouseYOnIterationBuffer());
                    const double mzi = 1.0 / pow(10, Constants::Fractal::ZOOM_INTERVAL);
                    float &logZoom = attr.fractal.logZoom;
                    logZoom += increment;
                    attr.fractal.center = attr.fractal.center.addCenterDouble(
                        offset[0] * (1 - mzi),
                        offset[1] * (1 - mzi),
                        Perturbator::logZoomToExp10(logZoom));
                }
                if (value == -1) {
                    const std::array<dex, 2> offset = offsetConversion(attr, getMouseXOnIterationBuffer(),
                                                                       getMouseYOnIterationBuffer());
                    const double mzo = 1.0 / pow(10, -Constants::Fractal::ZOOM_INTERVAL);
                    float &logZoom = attr.fractal.logZoom;
                    logZoom -= increment;
                    attr.fractal.center = attr.fractal.center.addCenterDouble(
                        offset[0] * (1 - mzo),
                        offset[1] * (1 - mzo),
                        Perturbator::logZoomToExp10(logZoom));
                }


                requests.requestRecompute();
                break;
            }
            default: {
                //noop
            }
        }
    }

    namespace {
        // A pixel sitting exactly on the reference has dc = 0, so it just replays the reference orbit
        // and never escapes. It is pushed off the reference by a fraction of a pixel to avoid that.
        // The test is on the pair: clamping each component on its own also caught every pixel of the
        // center row, whose dc.imag is zero but whose dc is perfectly usable, and every pixel of the
        // center column, drawing a one-pixel cross through the middle of the view. Sign is kept so
        // the nudge stays on the pixel's own side of the reference.
        std::array<double, 2> nudgeOffReference(const double rox, const double roy, const FractalAttribute &calc) {
            using namespace Constants::Fractal;
            if (std::abs(rox) < INTENTIONAL_ERROR_OFFSET_MIN_PIX &&
                std::abs(roy) < INTENTIONAL_ERROR_OFFSET_MIN_PIX) {
                return {
                    std::signbit(rox) ? -INTENTIONAL_ERROR_OFFSET_MIN_PIX : INTENTIONAL_ERROR_OFFSET_MIN_PIX,
                    std::signbit(roy) ? -INTENTIONAL_ERROR_OFFSET_MIN_PIX : INTENTIONAL_ERROR_OFFSET_MIN_PIX
                };
            }
            // A center coordinate of exactly zero holds the middle row (or column) on the axis its orbit never leaves, so the whole line reports the set's measure-zero slice as one hard black stroke, and the view is mirror-symmetric about that axis anyway.
            return {
                rox == 0.0 && calc.center.real.is_zero() ? INTENTIONAL_ERROR_OFFSET_MIN_PIX : rox,
                roy == 0.0 && calc.center.imag.is_zero() ? INTENTIONAL_ERROR_OFFSET_MIN_PIX : roy
            };
        }

        // A canvas pixel's landing place on the plane, in canvas pixels, or a mark that it sees no plane at all.
        struct ProjectedPoint {
            double x;
            double y;
            bool sky;
        };

        // Both 360 layouts run the plane out to infinity somewhere, so the projection has to fold at some finite
        // radius. The fold is held below what one pixel can tell apart, which keeps it from spreading one ring of
        // the plane across a visible patch of sky; Panorama Range caps it for anyone who would rather have the
        // speed, since the reference has to stay valid out to whatever radius the fold lands on.
        double panoramaLimit(const FractalAttribute &calc, const double w, const double h) {
            constexpr double toRadians = std::numbers::pi / 180.0;
            const double pixelAngle = effectiveProjection(calc.projectionMethod) == FrtProjectionMethod::PERSPECTIVE_360
                                          ? std::clamp(static_cast<double>(calc.panoramaFov), 1.0, 179.0) * toRadians / w
                                          : std::min(std::numbers::pi / h, 2.0 * std::numbers::pi / w);
            const double resolved = 4.0 / std::max(pixelAngle, 1e-12);
            return std::min(std::pow(10.0, std::max(static_cast<double>(calc.panoramaRange), 0.0)), resolved);
        }

        // The plane a direction lands on. Ground stands the viewer on the plane, so the radius is the tangent of the
        // angle below the horizon and everything at or above the horizon is sky. Full Sphere wraps the whole plane
        // onto the whole sphere by stereographic projection, whose half-angle tangent is conformal and so keeps the
        // fractal's shape everywhere, at the price of showing the plane beyond the horizon radius a second time,
        // turned inside out, over the upper half of the view.
        ProjectedPoint projectDirection(const double dx, const double dy, const double dz, const double h,
                                        const double limit, const FrtPanoramaLayout layout) {
            const double horiz = std::hypot(dx, dz);
            const double denom = layout == FrtPanoramaLayout::GROUND ? -dy : 1.0 - dy;
            if (layout == FrtPanoramaLayout::GROUND && denom <= 0.0) {
                return {0.0, 0.0, true};
            }
            const double t = denom > 0.0 && horiz < denom * limit ? horiz / denom : limit;
            const double r = h * 0.5 * t;
            // Looking exactly at a pole leaves no heading to keep, and the nadir is the view center anyway.
            return horiz > 0.0 ? ProjectedPoint{r * dx / horiz, r * dz / horiz, false} : ProjectedPoint{0.0, 0.0, false};
        }

        // Equirectangular: the canvas width is one turn of longitude, its height runs nadir to zenith, and its center
        // column faces the same way the camera below does at the same yaw. Buffer row 0 is the image's bottom row.
        ProjectedPoint equirectOffsetPixels(const double px, const double py, const double w, const double h,
                                            const double yaw, const double limit, const FrtPanoramaLayout layout) {
            const double lambda = 2.0 * std::numbers::pi * ((px + 0.5) / w - 0.5) + yaw;
            const double alpha = std::numbers::pi * ((py + 0.5) / h);
            const double sa = std::sin(alpha);
            return projectDirection(sa * std::sin(lambda), -std::cos(alpha), sa * std::cos(lambda), h, limit, layout);
        }

        // A camera inside the sphere, looking along yaw and pitch through a horizontal field of view. The right axis is
        // built from the yaw alone, so it is horizontal at every pitch and the basis holds together looking straight down.
        ProjectedPoint cameraOffsetPixels(const double px, const double py, const double w, const double h,
                                          const double yaw, const double pitch, const double fov, const double limit,
                                          const FrtPanoramaLayout layout) {
            const double cp = std::cos(pitch);
            const double sp = std::sin(pitch);
            const double sy = std::sin(yaw);
            const double cy = std::cos(yaw);
            const double fwd[3] = {cp * sy, sp, cp * cy};
            const double right[3] = {cy, 0.0, -sy};
            const double up[3] = {-sp * sy, cp, -sp * cy};
            const double ta = std::tan(fov * 0.5);
            const double a = (2.0 * ((px + 0.5) / w) - 1.0) * ta;
            const double b = (2.0 * ((py + 0.5) / h) - 1.0) * ta * h / w;
            const double dx = fwd[0] + a * right[0] + b * up[0];
            const double dy = fwd[1] + b * up[1];
            const double dz = fwd[2] + a * right[2] + b * up[2];
            const double len = std::sqrt(dx * dx + dy * dy + dz * dz);
            return projectDirection(dx / len, dy / len, dz / len, h, limit, layout);
        }

        // The plane offset a canvas pixel lands on, in canvas pixels, for whichever 360 mode is on.
        ProjectedPoint projectedOffsetPixels(const FractalAttribute &calc, const double px, const double py,
                                             const double w, const double h, const double yaw) {
            const double limit = panoramaLimit(calc, w, h);
            if (effectiveProjection(calc.projectionMethod) == FrtProjectionMethod::PERSPECTIVE_360) {
                constexpr double toRadians = std::numbers::pi / 180.0;
                return cameraOffsetPixels(px, py, w, h, yaw,
                                          std::clamp(static_cast<double>(calc.panoramaPitch), -90.0, 90.0) * toRadians,
                                          std::clamp(static_cast<double>(calc.panoramaFov), 1.0, 179.0) * toRadians,
                                          limit, calc.panoramaLayout);
            }
            return equirectOffsetPixels(px, py, w, h, yaw, limit, calc.panoramaLayout);
        }
    }

    std::array<dex, 2> RenderScene::offsetConversion(const Attribute &settings, const double mx, const double my,
                                                     bool *sky) const {
        using namespace Constants::Fractal;
        const double w = static_cast<double>(getIterationBufferWidth(settings));
        const double h = static_cast<double>(getIterationBufferHeight(settings));
        const double ox = static_cast<double>(mx) - w / 2.0;
        const double oy = static_cast<double>(my) - h / 2.0;

        const double radians = static_cast<double>(settings.fractal.rotation) * std::numbers::pi / 180.0;
        const double s = std::sin(radians);
        const double c = std::cos(radians);

        // Under a 360 mode the rotation is a yaw, turning which way the viewer faces rather than the flat image.
        const bool panorama = effectiveProjection(settings.fractal.projectionMethod) != FrtProjectionMethod::PLANAR;
        const ProjectedPoint pano = panorama
                                        ? projectedOffsetPixels(settings.fractal, static_cast<double>(mx),
                                                                static_cast<double>(my), w, h, radians)
                                        : ProjectedPoint{0.0, 0.0, false};
        if (sky != nullptr) {
            *sky = pano.sky;
        }
        const double rox = panorama ? pano.x : ox * c - oy * s;
        const double roy = panorama ? pano.y : ox * s + oy * c;

        // the iteration buffer spans client * clarity * ssaa
        // pixels, so divide the pixel offset by the same factor to reach fractal units.
        const auto pixelScale = static_cast<double>(settings.render.clarityMultiplier) *
                                static_cast<double>(settings.render.ssaa);
        const auto [nox, noy] = nudgeOffReference(rox, roy, settings.fractal);
        return {
            dex::value(nox) / getDivisor(settings) / pixelScale,
            dex::value(noy) / getDivisor(settings) / pixelScale
        };
    }

    std::array<dex, 2> RenderScene::offsetConversionFullGrid(const Attribute &settings, const int fx, const int fy,
                                                             const int fullW, const int fullH,
                                                             const int scale, bool *sky) const {
        const double ox = static_cast<double>(fx) - static_cast<double>(fullW) / 2.0;
        const double oy = static_cast<double>(fy) - static_cast<double>(fullH) / 2.0;
        const double radians = static_cast<double>(settings.fractal.rotation) * std::numbers::pi / 180.0;
        const double s = std::sin(radians);
        const double c = std::cos(radians);
        const bool panorama = effectiveProjection(settings.fractal.projectionMethod) != FrtProjectionMethod::PLANAR;
        const ProjectedPoint pano = panorama
                                        ? projectedOffsetPixels(settings.fractal, static_cast<double>(fx),
                                                                static_cast<double>(fy), static_cast<double>(fullW),
                                                                static_cast<double>(fullH), radians)
                                        : ProjectedPoint{0.0, 0.0, false};
        if (sky != nullptr) {
            *sky = pano.sky;
        }
        const double rox = panorama ? pano.x : ox * c - oy * s;
        const double roy = panorama ? pano.y : ox * s + oy * c;
        const double pixelScale = static_cast<double>(settings.render.clarityMultiplier) *
                                  static_cast<double>(settings.render.ssaa) * static_cast<double>(scale);
        const auto [nox, noy] = nudgeOffReference(rox, roy, settings.fractal);
        return {
            dex::value(nox) / getDivisor(settings) / pixelScale,
            dex::value(noy) / getDivisor(settings) / pixelScale
        };
    }

    dex RenderScene::dcMaxOf(const Attribute &settings, const int fullW, const int fullH, const int scale) const {
        if (effectiveProjection(settings.fractal.projectionMethod) == FrtProjectionMethod::PLANAR) {
            const auto o = offsetConversionFullGrid(settings, 0, 0, fullW, fullH, scale);
            dex r = dex::ZERO;
            dex_trigonometric::hypot_approx(&r, o[0], o[1]);
            return r;
        }
        // The camera's right axis is horizontal at every pitch and the equirectangular radius does not depend on the
        // column at all, so the pixel reaching furthest is on the middle column where the view faces the sky, and on a
        // side column where it faces the ground and the widest angle away from the center wins. The winner is picked in
        // pixel space, where the whole frame shares one scale, so the ordering is the one the fractal offsets are in.
        const double yaw = static_cast<double>(settings.fractal.rotation) * std::numbers::pi / 180.0;
        int bestX = 0;
        int bestY = 0;
        double bestR = -1.0;
        for (const int cx : {0, fullW / 2}) {
            for (int y = 0; y < fullH; ++y) {
                const auto p = projectedOffsetPixels(settings.fractal, static_cast<double>(cx), static_cast<double>(y),
                                                     static_cast<double>(fullW), static_cast<double>(fullH), yaw);
                if (p.sky) {
                    continue;
                }
                if (const double r = std::hypot(p.x, p.y); r > bestR) {
                    bestR = r;
                    bestX = cx;
                    bestY = y;
                }
            }
        }
        if (bestR < 0.0) {
            // Every pixel is sky, so nothing is iterated; the reference is still built, at the horizon radius.
            const double pixelScale = static_cast<double>(settings.render.clarityMultiplier) *
                                      static_cast<double>(settings.render.ssaa) * static_cast<double>(scale);
            return dex::value(static_cast<double>(fullH) * 0.5) / getDivisor(settings) / pixelScale;
        }
        const auto o = offsetConversionFullGrid(settings, bestX, bestY, fullW, fullH, scale);
        dex r = dex::ZERO;
        dex_trigonometric::hypot_approx(&r, o[0], o[1]);
        return r;
    }

    dex RenderScene::getDivisor(const Attribute &settings) {
        dex v = dex::ZERO;
        dex_exp::exp10(&v, settings.fractal.logZoom);
        return v;
    }

    uint16_t RenderScene::getClientWidth() const {
        return static_cast<uint16_t>(canvasExtent.width);
    }

    uint16_t RenderScene::getClientHeight() const {
        return static_cast<uint16_t>(canvasExtent.height);
    }

    uint16_t RenderScene::getIterationBufferWidth(const Attribute &settings) const {
        // clarity * ssaa. The ssaa factor is downsampled
        // away at export so keyframe maps / images carry true supersampled detail.
        const float multiplier = settings.render.clarityMultiplier * static_cast<float>(settings.render.ssaa);
        return static_cast<uint16_t>(scaledRenderAxis(getClientWidth(), multiplier, settings.video.data.sourceScale));
    }

    uint16_t RenderScene::getIterationBufferHeight(const Attribute &settings) const {
        const float multiplier = settings.render.clarityMultiplier * static_cast<float>(settings.render.ssaa);
        return static_cast<uint16_t>(scaledRenderAxis(getClientHeight(), multiplier, settings.video.data.sourceScale));
    }

    // Human-readable byte size for the memory-budget warnings.
    static std::wstring formatBytes(const uint64_t bytes) {
        constexpr double GB = 1024.0 * 1024.0 * 1024.0;
        constexpr double MB = 1024.0 * 1024.0;
        const double b = static_cast<double>(bytes);
        if (b >= GB) return std::format(L"{:.2f} GB", b / GB);
        return std::format(L"{:.0f} MB", b / MB);
    }

    uint64_t RenderScene::getDeviceLocalMemoryTotal() const {
        const auto &mem = wc.core.getPhysicalDevice().getPhysicalDeviceMemoryProperties();
        uint64_t total = 0;
        for (uint32_t i = 0; i < mem.memoryHeapCount; ++i) {
            if (mem.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                total += mem.memoryHeaps[i].size;
            }
        }
        return total;
    }

    uint64_t RenderScene::getSystemMemoryTotal() {
        MEMORYSTATUSEX status{};
        status.dwLength = sizeof(status);
        if (GlobalMemoryStatusEx(&status)) {
            return status.ullTotalPhys;
        }
        return 0;
    }

    float RenderScene::getMaxInternalScale() const {
        return getMaxInternalScale(canvasExtent.width,canvasExtent.height);
    }

    float RenderScene::getMaxInternalScale(uint32_t width,uint32_t height) const {
        // Internal render extent is client * clarity * ssaa; the larger client axis is what hits maxImageDimension2D first.
        const uint32_t largest = std::max(width, height);
        if (largest == 0) return 0.0f;
        const auto &limits = wc.core.getPhysicalDevice().getPhysicalDeviceProperties().limits;
        const double dimScale = static_cast<double>(limits.maxImageDimension2D) / static_cast<double>(largest);
        // The iteration matrix is one storage buffer of width * height doubles, so its own limit is on area.
        const double px = static_cast<double>(width) * static_cast<double>(height) * sizeof(double);
        if (px <= 0.0) return static_cast<float>(dimScale);
        const double bufferScale = std::sqrt(static_cast<double>(limits.maxStorageBufferRange) / px);
        const double exactScale = std::min(dimScale, bufferScale);
        float roundedScale = static_cast<float>(exactScale);
        if (static_cast<double>(roundedScale) > exactScale) {
            roundedScale = std::nextafter(roundedScale, 0.0f);
        }
        return roundedScale;
    }

    std::wstring RenderScene::checkRenderMemoryBudget(const Attribute &settings) const {
        return checkRenderMemoryBudget(settings,getClientWidth(),getClientHeight());
    }

    std::wstring RenderScene::checkRenderMemoryBudget(const Attribute &settings,uint32_t width,uint32_t height) const {
        // Internal render extent = client * clarity * ssaa (matches getInternalImageExtent / iteration buffer).
        const double multiplier = static_cast<double>(settings.render.clarityMultiplier) *
                                  static_cast<double>(settings.render.ssaa);
        const double iw = static_cast<double>(width) * multiplier * settings.video.data.sourceScale;
        const double ih = static_cast<double>(height) * multiplier * settings.video.data.sourceScale;
        const double internalPx = iw * ih;

        // The downsample-for-blur images are capped to GAUSSIAN_MAX_WIDTH wide (negligible, but counted).
        double blurredPx = internalPx;
        if (iw > 0.0) {
            if (const double rat = static_cast<double>(Constants::Fractal::GAUSSIAN_MAX_WIDTH) / iw; rat < 1.0) {
                blurredPx = static_cast<double>(Constants::Fractal::GAUSSIAN_MAX_WIDTH) * (ih * rat);
            }
        }

        const auto mff = static_cast<double>(wc.core.getPhysicalDevice().getMaxFramesInFlight());
        // Two R16G16B16A16 internal images and two R16G16B16A16_SFLOAT blurred images (16 B/px each pair), per frame in flight,
        // plus ~30% for framebuffers/descriptors/driver reserve.
        const double vramBytes = mff * (internalPx * 16.0 + blurredPx * 16.0) * 1.3;
        // Host side: iteration matrix (double) + host-visible staging buffer, both width*height*8 B.
        const double ramBytes = internalPx * 16.0;

        const uint64_t vramTotal = getDeviceLocalMemoryTotal();
        const uint64_t ramTotal = getSystemMemoryTotal();

        std::wstring msg;
        if (vramTotal > 0 && vramBytes > 0.8 * static_cast<double>(vramTotal)) {
            msg += std::format(L"- VRAM: this view needs about {}, but the GPU only has {}.\n",
                               formatBytes(static_cast<uint64_t>(vramBytes)), formatBytes(vramTotal));
        }
        if (ramTotal > 0 && ramBytes > 0.8 * static_cast<double>(ramTotal)) {
            msg += std::format(L"- RAM: this view needs about {}, but the system only has {}.\n",
                               formatBytes(static_cast<uint64_t>(ramBytes)), formatBytes(ramTotal));
        }
        return msg;
    }

    std::wstring RenderScene::getPassTimingReport() const {
        if (renderer == nullptr) {
            return L"  (no renderer)\n";
        }
        if (!passTimingEnabled) {
            return L"  (off - turn on \"Measure GPU Pass Times\" first)\n";
        }
        return renderer->passTimer.report();
    }

    void RenderScene::clearPassTiming() const {
        if (renderer != nullptr) {
            renderer->passTimer.clear();
        }
    }

    std::wstring RenderScene::dumpState() const {
        // Every number here is read where it lies rather than through a lock: this is a snapshot of a
        // moving thing, taken to be looked at, and a torn digit costs nothing next to stopping the
        // compute to read it.
        const auto toW = [](const std::string &s) { return std::wstring(s.begin(), s.end()); };
        const auto &calc = attr.fractal;
        const auto &props = wc.core.getPhysicalDevice().getPhysicalDeviceProperties();

        std::wstring out = L"=== RFF_Super scene state ===\n\n[view]\n";
        out += std::format(L"  real                 {}\n", toW(calc.center.real.to_string()));
        out += std::format(L"  imag                 {}\n", toW(calc.center.imag.to_string()));
        out += std::format(L"  log zoom (e)         {:.3f}\n", calc.logZoom);
        out += std::format(L"  rotation             {:.1f}\n", calc.rotation);
        out += std::format(L"  formula              {}\n", Selectable::toString(calc.formulaType));
        out += std::format(L"  reuse reference      {}\n", Selectable::toString(calc.reuseReferenceMethod));
        out += std::format(std::locale(), L"  max iteration        {:L}\n", calc.maxIteration);

        out += L"\n[canvas]\n";
        out += std::format(L"  client               {} x {}\n", getClientWidth(), getClientHeight());
        out += std::format(L"  canvas extent        {} x {}\n", canvasExtent.width, canvasExtent.height);
        out += std::format(L"  iteration buffer     {} x {}\n", getIterationBufferWidth(attr),
                           getIterationBufferHeight(attr));
        out += std::format(L"  clarity x ssaa       {} x {}\n", attr.render.clarityMultiplier, attr.render.ssaa);
        out += std::format(L"  max internal scale   {:.3f}\n", getMaxInternalScale());

        out += L"\n[last compute]\n";
        out += std::format(L"  log zoom             {:.3f}\n", lastLogZoom);
        out += std::format(std::locale(), L"  max iteration        {:L}\n", lastMaxIteration);
        out += std::format(std::locale(), L"  longest period       {:L}\n", lastPeriod);
        out += std::format(L"  idle                 {}\n", idleCompute.load() ? L"yes" : L"no");
        out += std::format(L"  interrupt requested  {}\n", state.interruptRequested() ? L"yes" : L"no");
        out += std::format(L"  preview upload       {}\n", previewUploadPending.load() ? L"pending" : L"-");
        out += std::format(std::locale(), L"  preview generation   {:L}\n", previewSeedGeneration);

        out += L"\n[reference]\n";
        if (currentPerturbator == nullptr) {
            out += L"  (none built yet)\n";
        } else {
            const wchar_t *kind = L"unknown";
            size_t mpaLength = 0;
            if (const auto *t = dynamic_cast<LightMandelbrotPerturbator *>(currentPerturbator.get())) {
                kind = L"Light";
                mpaLength = t->getTable().getLength();
            } else if (const auto *t = dynamic_cast<DeepMandelbrotPerturbator *>(currentPerturbator.get())) {
                kind = L"Deep";
                mpaLength = t->getTable().getLength();
            } else if (dynamic_cast<CustomFormulaPerturbator *>(currentPerturbator.get()) != nullptr) {
                kind = L"Custom";
            }
            out += std::format(L"  perturbator          {}\n", kind);
            const MandelbrotReference *reference = currentPerturbator->getReference();
            if (reference == nullptr || reference == Constants::NullPointer::PROCESS_TERMINATED_REFERENCE) {
                out += L"  reference            (terminated)\n";
            } else {
                out += std::format(std::locale(), L"  reference length     {:L}\n", reference->length());
                out += std::format(std::locale(), L"  longest period       {:L}\n", reference->longestPeriod());
            }
            out += std::format(std::locale(), L"  MPA table length     {:L}\n", mpaLength);
        }

        out += L"\n[approximation cache]\n";
        size_t lightEntries = 0;
        for (const auto &row: approxTableCache.lightTable) {
            lightEntries += row.size();
        }
        size_t deepEntries = 0;
        for (const auto &row: approxTableCache.deepTable) {
            deepEntries += row.size();
        }
        out += std::format(std::locale(), L"  light                {:L} rows, {:L} entries, {}\n",
                           approxTableCache.lightTable.size(), lightEntries,
                           formatBytes(static_cast<uint64_t>(lightEntries) * sizeof(LightPA)));
        out += std::format(std::locale(), L"  deep                 {:L} rows, {:L} entries, {}\n",
                           approxTableCache.deepTable.size(), deepEntries,
                           formatBytes(static_cast<uint64_t>(deepEntries) * sizeof(DeepPA)));

        out += L"\n[device]\n";
        out += std::format(L"  name                 {}\n", toW(std::string(props.deviceName)));
        out += std::format(L"  frames in flight     {}\n", wc.core.getPhysicalDevice().getMaxFramesInFlight());
        out += std::format(L"  VRAM (device local)  {}\n", formatBytes(getDeviceLocalMemoryTotal()));
        out += std::format(L"  RAM                  {}\n", formatBytes(getSystemMemoryTotal()));
        if (const std::wstring budget = checkRenderMemoryBudget(attr); !budget.empty()) {
            out += L"  over budget:\n" + budget;
        }

        out += L"\n[running]\n";
        out += std::format(L"  background threads   {}\n", backgroundThreads.runningCount());
        out += std::format(L"  browsed maps         {} (index {})\n", browsedMaps.size(), browsedMapIndex);
        out += std::format(L"  pass timing          {}\n", passTimingEnabled ? L"on" : L"off");

        if (passTimingEnabled) {
            out += L"\n[gpu passes]\n";
            out += getPassTimingReport();
        }
        return out;
    }

    void RenderScene::applyDefaultAttr() {
        wc.core.getLogicalDevice().waitDeviceIdle();
        // Preserve color settings, render settings, and formula across Reset.
        auto preservedShader = attr.shader;
        auto preservedRender = attr.render;
        auto preservedFormulaType = attr.fractal.formulaType;
        auto preservedCustomFormula = attr.fractal.customFormula;
        const bool preservedAutoMaxIteration = attr.fractal.autoMaxIteration;
        attr = genDefaultAttr();
        attr.shader = std::move(preservedShader);
        attr.render = std::move(preservedRender);
        attr.fractal.formulaType = preservedFormulaType;
        attr.fractal.customFormula = std::move(preservedCustomFormula);
        // The default center (-0.85) is tuned for the Mandelbrot set; custom
        // formulas are generally centered around the origin instead.
        if (preservedFormulaType == FractalFormulaType::CUSTOM) {
            attr.fractal.center = fp_complex("0", "0",
                                             Perturbator::logZoomToExp10(attr.fractal.logZoom));
        }
        if (preservedFormulaType == FractalFormulaType::CUSTOM) {
            if (!autoIterationCustomActive) {
                autoIterationBackup = preservedAutoMaxIteration;
                autoIterationCustomActive = true;
            }
            attr.fractal.autoMaxIteration = false;
        } else if (autoIterationCustomActive) {
            attr.fractal.autoMaxIteration = autoIterationBackup;
            autoIterationCustomActive = false;
        }
    }


    void RenderScene::applyLoadedConfig() {
        // The config restores the location, max-iteration and auto-iteration directly.
        if (attr.fractal.formulaType == FractalFormulaType::CUSTOM) {
            attr.fractal.autoMaxIteration = false;
            autoIterationCustomActive = true;
        } else {
            autoIterationCustomActive = false;
        }
    }

    void RenderScene::applyRecoveredLocation(const Attribute &loaded) {
        // The shader is read by the renderer, which is replaced here along with everything else.
        wc.core.getLogicalDevice().waitDeviceIdle();
        attr = genDefaultAttr();
        auto &fr = attr.fractal;
        const auto &lf = loaded.fractal;
        fr.center = lf.center;
        fr.logZoom = lf.logZoom;
        fr.maxIteration = lf.maxIteration;
        fr.autoMaxIteration = lf.autoMaxIteration;
        fr.autoIterationMultiplier = lf.autoIterationMultiplier;
        fr.formulaType = lf.formulaType;
        fr.customFormula = lf.customFormula;
        applyLoadedConfig();
    }

    void RenderScene::setComputeHold(const bool hold) {
        computeHold = hold;
        if (hold) {
            // The canvas stays empty for as long as this lasts, so the status bar has to say why.
            setStatusMessage(Constants::Status::RENDER_STATUS, L"Waiting to generate");
        }
    }

    bool RenderScene::isComputeUnfinished() const {
        // Long enough that the view was worth waiting for and did not come. Shorter than the wait
        // anyone would sit through, or every ordinary exit taken mid-recompute would be kept.
        constexpr auto GIVEN_UP_ON_AFTER = std::chrono::seconds(15);
        if (idleCompute) {
            return false;
        }
        return std::chrono::steady_clock::now() - computeStartedAt >= GIVEN_UP_ON_AFTER;
    }

    void RenderScene::writeRecoverySnapshot(const bool force) {
        const auto now = std::chrono::steady_clock::now();
        if (!force && now - lastRecoverySnapshot < std::chrono::seconds(3)) {
            return;
        }
        lastRecoverySnapshot = now;
        const auto dimensions=documentCanvasSize();
        RecoveryIO::writeSnapshot(attr,dimensions.cx,dimensions.cy);
    }


    namespace {
        // Copies an internal render image (R16G16B16A16) to a host-visible buffer and returns it as a
        // cloned RGBA cv::Mat. The caller must have waited the render fence before calling.
        template<typename ImageCtx>
        cv::Mat readbackImageContextToMat(vkh::WindowContextRef wc, const ImageCtx &imgCtx) {
            // The Mat below reads 8 bytes per texel, so any other format (a swapchain image, say) would be misread.
            if (imgCtx.imageFormat != VK_FORMAT_R16G16B16A16_SFLOAT &&
                imgCtx.imageFormat != VK_FORMAT_R16G16B16A16_UNORM) {
                throw std::runtime_error("Image readback expects an R16G16B16A16 render image");
            }
            // Sized from the extent the copy writes and the Mat reads, not from the image's memory requirement.
            const size_t rowBytes = static_cast<size_t>(imgCtx.extent.width) * 4 * sizeof(uint16_t);
            const VkDeviceSize packedSize = static_cast<VkDeviceSize>(rowBytes) * imgCtx.extent.height;
            if (packedSize == 0) {
                throw std::runtime_error("Image readback of an empty image");
            }
            vkh::BufferContext bufCtx = vkh::BufferContext::createContext(wc.core, {
                .size = packedSize,
                .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            });
            bool mapped = false;
            cv::Mat result;
            try {
                vkh::BufferContext::mapMemory(wc.core, bufCtx);
                mapped = true;
                auto executor = vkh::ScopedNewCommandBufferExecutor(wc.core, wc.getCommandPool());
                vkh::BarrierUtils::cmdImageMemoryBarrier(executor.getCommandBufferHandle(), imgCtx.image,
                                                         VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 1,
                                                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                                         VK_PIPELINE_STAGE_TRANSFER_BIT);
                vkh::BufferImageContextUtils::cmdCopyImageToBuffer(executor.getCommandBufferHandle(), imgCtx, bufCtx);
                vkh::BarrierUtils::cmdBufferMemoryBarrier(
                    executor.getCommandBufferHandle(), VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT,
                    bufCtx.buffer, 0, bufCtx.bufferSize, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT);
                executor.finish();
                const bool floating = imgCtx.imageFormat == VK_FORMAT_R16G16B16A16_SFLOAT;
                cv::Mat view(static_cast<int>(imgCtx.extent.height), static_cast<int>(imgCtx.extent.width), floating ? CV_16FC4 : CV_16UC4,
                             bufCtx.mappedMemory, rowBytes);
                if (floating) {
                    view.convertTo(result, CV_16UC4, 65535.0);
                } else {
                    result = view.clone();
                }
            } catch (...) {
                if (mapped) {
                    vkh::BufferContext::unmapMemory(wc.core, bufCtx);
                }
                vkh::BufferContext::destroyContext(wc.core, bufCtx);
                throw;
            }
            vkh::BufferContext::unmapMemory(wc.core, bufCtx);
            vkh::BufferContext::destroyContext(wc.core, bufCtx);
            return result;
        }

        void reportImageSaveProgress(const std::shared_ptr<ExportProgress> &progress,
                                     ExportCompletion &completion, const std::filesystem::path &filename,
                                     const wchar_t *successPrefix, bool saved) {
            if (!progress) {
                return;
            }
            if (!saved) {
                progress->report(ExportProgress::Phase::FAILED, L"Image save failed. Check the destination.");
                return;
            }
            completion.progress.reset();
            try {
                progress->report(ExportProgress::Phase::COMPLETED, successPrefix + filename.wstring(), 1.0f);
            } catch (...) {
                try {
                    progress->report(ExportProgress::Phase::COMPLETED, {}, 1.0f);
                } catch (...) {
                }
            }
        }
    }

    std::pair<cv::Mat, cv::Mat> RenderScene::renderComparison(const ShaderAttribute &reference, float seconds) {
        if (smoothZoomActive || !idleCompute || previewUploadPending || requests.recomputeRequested ||
            requests.resizeRequested || requests.shaderRequested || isLongJobBusy()) {
            return {};
        }

        const auto phases = renderer->rendererIteration->phases;
        const auto clock = renderer->rendererIteration->previewClock;
        const bool pinned = renderer->rendererIteration->animationTimePinned;
        const float pinnedTime = renderer->rendererIteration->pinnedTime;
        const auto restore = [&] {
            ensureShaderFormat(attr.shader.sceneLinear());
            renderer->rendererIteration->animationTimePinned = true;
            renderer->rendererIteration->pinnedTime = seconds;
            applyShaderAttr(attr);
            renderer->rendererIteration->phases = phases;
            renderer->rendererIteration->previewClock = clock;
            renderer->rendererIteration->animationTimePinned = pinned;
            renderer->rendererIteration->pinnedTime = pinnedTime;
        };
        try {
            const auto capture = [&](const ShaderAttribute &shader) {
                auto settings = attr;
                settings.shader = shader;
                if (settings.shader.slope.lustreRelief && settings.shader.slope.reliefZoomReference < 0) {
                    settings.shader.slope.reliefZoomReference = attr.fractal.logZoom;
                }
                ensureShaderFormat(shader.sceneLinear());
                renderer->rendererIteration->animationTimePinned = true;
                renderer->rendererIteration->pinnedTime = seconds;
                applyShaderAttr(settings);
                renderer->rendererIteration->phases.seekTo(seconds);
                renderer->executeOffscreen();
                const auto frame = renderer->getFrameIndex();
                wc.getSyncObject().getFence(frame).wait();
                const auto &image = wc.getSharedImageContext().getImageContextMF(
                    SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_SECONDARY)[frame];
                auto pixels = readbackImageContextToMat(wc, image);
                cv::cvtColor(pixels, pixels, cv::COLOR_RGBA2BGRA);
                cv::resize(pixels, pixels, cv::Size(getClientWidth(), getClientHeight()), 0, 0, cv::INTER_AREA);
                pixels.convertTo(pixels, CV_8UC4, 1.0 / 257.0);
                return pixels;
            };
            auto referenceImage = capture(reference);
            auto currentImage = &reference == &attr.shader ? referenceImage : capture(attr.shader);
            restore();
            return {std::move(referenceImage), std::move(currentImage)};
        } catch (...) {
            restore();
            throw;
        }
    }

    void RenderScene::applyCreateImage(RenderSceneRequests::CreateImageRequest request) {
        ExportCompletion completion{request.progress};
        if (request.progress) {
            if (request.progress->cancelRequested) {
                return;
            }
            request.progress->report(ExportProgress::Phase::RENDERING, L"Rendering image...");
        }
        // The dialog runs before the fence wait so cancelling costs nothing, and returns nullptr when closed.
        if (request.filename.empty()) {
            const auto path = IOUtilities::ioFileDialog(L"Save Image", Constants::Extension::DESC_IMAGE,
                                                        IOUtilities::SAVE_FILE, Constants::Extension::IMAGE);
            if (path == nullptr) {
                return;
            }
            request.filename = *path;
        }

        // The compute thread writes the map straight into the persistently mapped staging buffer the GPU
        // samples, and this runs before execute() in the render loop, so the newest presented frame can
        // predate the end of the render. Reading it back saved whatever rows the progressive renderer had
        // not reached yet, which showed up as horizontal bands on the render-priority row grid. One
        // offscreen frame off the finalized map removes the race, the same way the tiled export does.
        renderer->executeOffscreen();

        const uint32_t frameIndex = renderer->getFrameIndex();
        wc.getSyncObject().getFence(frameIndex).wait();
        const auto &imgCtx = wc.getSharedImageContext().getImageContextMF(
            SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_SECONDARY)[frameIndex];

        // readbackImageContextToMat clones before unmapping; the inline copy here read mappedMemory after vkUnmapMemory.
        cv::Mat img = readbackImageContextToMat(wc, imgCtx);
        cv::cvtColor(img, img, cv::COLOR_RGBA2BGRA);
        bool saved = false;
        if (const uint32_t ssaa = attr.render.ssaa; ssaa > 1 && request.downsample) {
            cv::Mat resized;
            cv::resize(img, resized,
                       // Clamped like the same divide in CPCImageRGBA2BGR and VideoWindow: a zero size made cv::resize throw.
                       cv::Size(std::max(1, static_cast<int>(imgCtx.extent.width) / static_cast<int>(ssaa)),
                                std::max(1, static_cast<int>(imgCtx.extent.height) / static_cast<int>(ssaa))),
                       0, 0, cv::INTER_AREA);
            saved = IOUtilities::writeImage(request.filename, resized);
        } else {
            saved = IOUtilities::writeImage(request.filename, img);
        }
        reportImageSaveProgress(request.progress, completion, request.filename, L"Image saved: ", saved);
        if (!saved) {
            setStatusMessage(Constants::Status::RENDER_STATUS, L"Image save failed");
        }
    }

    void RenderScene::snapshotComputePreview(const bool complete) {
        if (iterationMatrix == nullptr || renderer == nullptr ||
            renderer->iterationStagingBufferContext == nullptr) {
            return;
        }
        // 20 uploads a second: fast enough to read as the map filling in, rare enough that the
        // buffer is rewritten between submissions rather than under a run of them.
        constexpr auto SNAPSHOT_INTERVAL = std::chrono::milliseconds(50);
        const auto now = std::chrono::steady_clock::now();
        if (!complete && now - lastPreviewSnapshot < SNAPSHOT_INTERVAL) {
            return;
        }
        lastPreviewSnapshot = now;

        auto &staging = *renderer->iterationStagingBufferContext;
        const uint16_t w = iterationMatrix->getWidth();
        const uint16_t h = iterationMatrix->getHeight();
        if (staging.getWidth() != w || staging.getHeight() != h) {
            // A resize is mid-flight and applyResize owns the buffer; the next frame lands on the pair.
            return;
        }

        // One buffer serves every frame, and the copy out of it is recorded once per frame, so a
        // frame submitted earlier can still be reading it while this writes. Waiting the frames in
        // flight out first is what makes the rewrite safe; it costs nothing on the frames that do
        // not snapshot, which is all but twenty a second.
        waitFramesInFlight();

        // Read one element at a time rather than by block: the compute threads are still writing
        // this matrix, and only the relaxed accessor makes reading it as it fills defined.
        const Matrix<double> &src = *iterationMatrix;
        auto *dst = reinterpret_cast<double *>(staging.getContext().mappedMemory);

        if (complete) {
            // The map is final, so what is left of the picture the resize was seeded from goes with it.
            previewSeedGeneration = 0;
            for (uint32_t i = 0, length = static_cast<uint32_t>(w) * h; i < length; ++i) {
                dst[i] = src.loadRelaxed(i);
            }
            return;
        }

        // The buffer still holds the view the resize was asked over, so only the pixels this compute has reached are laid onto it: a zero would paint the interior color across the picture instead.
        if (previewSeedGeneration != 0 && previewSeedGeneration == computeGeneration.load()) {
            for (uint32_t i = 0, length = static_cast<uint32_t>(w) * h; i < length; ++i) {
                if (const double v = src.loadRelaxed(i); v != 0) {
                    dst[i] = v;
                }
            }
            return;
        }

        if (!previewFillDown.load()) {
            for (uint32_t i = 0, length = static_cast<uint32_t>(w) * h; i < length; ++i) {
                dst[i] = src.loadRelaxed(i);
            }
            return;
        }

        // compute() zeroed the matrix, so a zero is a pixel the render front has not reached: it takes
        // the nearest computed value above it, which is the continuous fill the preview always had.
        std::vector<double> above(w, 0.0);
        for (uint16_t y = 0; y < h; ++y) {
            const uint32_t rowStart = static_cast<uint32_t>(w) * y;
            double *dstRow = dst + static_cast<size_t>(w) * y;
            for (uint16_t x = 0; x < w; ++x) {
                if (const double v = src.loadRelaxed(rowStart + x); v != 0) {
                    above[x] = v;
                    dstRow[x] = v;
                } else {
                    dstRow[x] = above[x];
                }
            }
        }
    }

    void RenderScene::ensureShaderFormat(bool studio) {
            const VkFormat desired = studio ? VK_FORMAT_R16G16B16A16_SFLOAT : VK_FORMAT_R16G16B16A16_UNORM;
            if (renderer == nullptr) {
                wc.core.getLogicalDevice().waitDeviceIdle();
                refreshSharedImgContext(desired);
                for (const auto &context : wc.getRenderContexts()) {
                    context->recreate();
                }
                try {
                    initRenderer();
                    for (const auto &configurator : renderer->configurators) {
                        configurator->renderContextRefreshed();
                    }
                    const auto width = getIterationBufferWidth(attr);
                    const auto height = getIterationBufferHeight(attr);
                    auto staging = std::make_unique<GraphicsMatrixBuffer<double>>(
                        wc.core, width, height, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
                    if (iterationMatrix != nullptr && iterationMatrix->getWidth() == width &&
                        iterationMatrix->getHeight() == height) {
                        staging->fill(iterationMatrix->getCanvas());
                    }
                    renderer->iterationStagingBufferContext = std::move(staging);
                    renderer->rendererIteration->resetIterationBuffer(width, height);
                    renderer->rendererIteration->setMaxIteration(static_cast<double>(lastMaxIteration));
                    const auto blurExtent = getBlurredImageExtent();
                    const auto presentExtent = getSwapchainRenderContextExtent();
                    renderer->rendererDownsampleForBlur->setRescaledResolution(0, {blurExtent.width, blurExtent.height});
                    renderer->rendererDownsampleForBlur->setRescaledResolution(1, {blurExtent.width, blurExtent.height});
                    renderer->rendererPresent->setRescaledResolution({presentExtent.width, presentExtent.height});
                } catch (...) {
                    renderer.reset();
                    throw;
                }
                return;
            }
            if (wc.getSharedImageContext().getImageContextMF(SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_PRIMARY)[0].imageFormat != desired) {
                wc.core.getLogicalDevice().waitDeviceIdle();
                using namespace SharedDescriptorTemplate;
                const auto &iterationInfo = renderer->rendererIteration->getDescriptor(GPCIterationPalette::SET_ITERATION)
                    .get<vkh::Uniform>(0, DescIteration::BINDING_UBO_ITERATION_INFO)->getHostObject();
                const double maximum = iterationInfo.get<double>(DescIteration::TARGET_UBO_ITERATION_MAX);
                const auto phases = renderer->rendererIteration->phases;
                const auto previewClock = renderer->rendererIteration->previewClock;
                const auto pinned = renderer->rendererIteration->animationTimePinned;
                const auto pinnedTime = renderer->rendererIteration->pinnedTime;
                auto staging = std::move(renderer->iterationStagingBufferContext);
                renderer.reset();
                refreshSharedImgContext(desired);
                for (const auto &context : wc.getRenderContexts()) {
                    context->recreate();
                }
                try {
                    initRenderer();
                    // Rebind the replacement images and blur resources before the first frame uses the new pipelines.
                    for (const auto &configurator : renderer->configurators) {
                        configurator->renderContextRefreshed();
                    }
                    renderer->iterationStagingBufferContext = std::move(staging);
                    renderer->rendererIteration->resetIterationBuffer(getIterationBufferWidth(attr), getIterationBufferHeight(attr));
                    renderer->rendererIteration->setMaxIteration(maximum);
                    renderer->rendererIteration->phases = phases;
                    renderer->rendererIteration->previewClock = previewClock;
                    renderer->rendererIteration->animationTimePinned = pinned;
                    renderer->rendererIteration->pinnedTime = pinnedTime;
                    const auto blurExtent = getBlurredImageExtent();
                    const auto presentExtent = getSwapchainRenderContextExtent();
                    renderer->rendererDownsampleForBlur->setRescaledResolution(0, {blurExtent.width, blurExtent.height});
                    renderer->rendererDownsampleForBlur->setRescaledResolution(1, {blurExtent.width, blurExtent.height});
                    renderer->rendererPresent->setRescaledResolution({presentExtent.width, presentExtent.height});
                } catch (...) {
                    renderer.reset();
                    throw;
                }
            }
    }

    void RenderScene::applyShaderAttr(const Attribute &attr) const {
        wc.core.getLogicalDevice().waitDeviceIdle();
        renderer->rendererIteration->setPalette(attr.shader.palette);
        renderer->rendererIteration->setTextures(attr.shader.textures, warpSourceLayer(attr.shader.warp));
        renderer->rendererIteration->setPattern(attr.shader.patterns);
        renderer->rendererIteration->setWarp(attr.shader.warp);
        renderer->rendererStripe->setStripe(attr.shader.stripe);
        // The stripe's own uniform holds its look; its phase rides the clock the iteration pass
        // publishes, so the speed goes there as well.
        renderer->rendererIteration->setStripeSpeed(attr.shader.stripe);
        renderer->layerShader = attr.shader;
        renderer->rendererSlope->setSlope(attr.shader.slope);
        renderer->rendererLinearInterpolation->setSurface(attr.shader.slope);
        renderer->rendererSlope->setGroove(attr.shader.palette);
        renderer->rendererSlope->setReliefZoom(attr.shader.slope, attr.fractal.logZoom);
        renderer->rendererColor->setColor(attr.shader.color, attr.shader.sceneLinear());
        renderer->rendererFog->setFog(attr.shader.fog);
        renderer->rendererSlope->setEffects(attr.shader.effects, attr.shader.sceneLinear());
        renderer->rendererIteration->setEffects(attr.shader.effects);
        renderer->rendererBloom->setBloom(attr.shader.bloom, attr.shader.hdr, attr.shader.sceneLinear());
        renderer->rendererLinearInterpolation->setLinearInterpolation(attr.render.linearInterpolation);
        renderer->rendererLinearInterpolation->setDither(attr.render.dither);
        // The canvas is not a display, so the preview always takes the tone-mapped SDR end of the transform.
        renderer->rendererLinearInterpolation->setToneMap(attr.shader.hdr, VidHdrTransfer::SDR, 0.0f, attr.shader.sceneLinear());
        renderer->rendererBoxBlur->setBlurInfo(CPCBoxBlur::DESC_INDEX_BLUR_TARGET_FOG, attr.shader.fog.radius);
        renderer->rendererBoxBlur->
                setBlurInfo(CPCBoxBlur::DESC_INDEX_BLUR_TARGET_BLOOM, attr.shader.bloom.radius);
    }

    void RenderScene::refreshResizeParams() {
        const uint16_t iw = getIterationBufferWidth(attr);
        const uint16_t ih = getIterationBufferHeight(attr);
        const auto &[dWidth, dHeight] = getBlurredImageExtent();
        const auto &[sWidth, sHeight] = getSwapchainRenderContextExtent();

        renderer->rendererDownsampleForBlur->setRescaledResolution(0, {dWidth, dHeight});
        renderer->rendererDownsampleForBlur->setRescaledResolution(1, {dWidth, dHeight});
        renderer->rendererPresent->setRescaledResolution({sWidth, sHeight});
        renderer->rendererIteration->resetIterationBuffer(iw, ih);
        iterationMatrix = std::make_unique<Matrix<double> >(iw, ih);
        renderer->iterationStagingBufferContext = std::make_unique<GraphicsMatrixBuffer<double> >(
            wc.core, iw, ih, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }

    void RenderScene::initRenderer(bool prepareAll) {
        wc.core.getLogicalDevice().waitDeviceIdle();
        const std::function<void()> prepare = prepareAll ? std::function<void()>{[&] { prepareVideoPipelines(engine, attr); }} : std::function<void()>{};
        renderer = std::make_unique<RenderSceneRenderer>(engine, wc.getAttachmentIndex(), prepare);
        wc.core.getLogicalDevice().savePipelineCache();
    }


    void RenderScene::applyResize() {
        wc.core.getLogicalDevice().waitDeviceIdle();
        // A resize asked for by a config load or a recovery rather than by the window moves the canvas with no swapchain recreate, leaving its images smaller than the framebuffer built over them.
        const auto [surfaceWidth, surfaceHeight] = wc.getSwapchain().populateSwapchainExtent();
        if (const auto [swapchainWidth, swapchainHeight] = wc.getSwapchain().getCurrentExtent();
            (swapchainWidth != surfaceWidth || swapchainHeight != surfaceHeight) &&
            !wc.getWindow().isUnrenderable()) {
            wc.getSwapchain().recreate();
        }
        // The map on screen belongs to the canvas being replaced, so it is taken before the size moves and put back on the new buffer below.
        const std::unique_ptr<Matrix<double> > previous = std::move(iterationMatrix);
        refreshCanvasExtent();
        refreshSharedImgContext();
        refreshRenderContext();
        refreshResizeParams();
        if (previous != nullptr) {
            seedPreviewFromMatrix(*previous);
        }
    }

    void RenderScene::refreshCanvasExtent() {
        canvasExtent = documentCanvasExtent.value_or(wc.getSwapchain().getCurrentExtent());
    }

    void RenderScene::waitFramesInFlight() const {
        for (uint32_t i = 0, framesInFlight = wc.core.getPhysicalDevice().getMaxFramesInFlight();
             i < framesInFlight; ++i) {
            wc.getSyncObject().getFence(i).wait();
        }
    }

    void RenderScene::seedPreviewFromZoom(const double srcCenterX, const double srcCenterY,
                                          const double magnification) {
        if (renderer == nullptr || renderer->iterationStagingBufferContext == nullptr ||
            !(magnification > 0.0)) {
            return;
        }
        auto &staging = *renderer->iterationStagingBufferContext;
        const uint16_t w = staging.getWidth();
        const uint16_t h = staging.getHeight();
        if (w == 0 || h == 0) {
            return;
        }
        // The frame recorded before this one copies out of the same buffer, so it has to be done with it before the picture below is rewritten in place.
        waitFramesInFlight();

        auto *dst = reinterpret_cast<double *>(staging.getContext().mappedMemory);
        // The magnified picture reads pixels the same pass is overwriting, so the source is taken aside first.
        const std::vector<double> src(dst, dst + static_cast<size_t>(w) * h);

        const double halfW = static_cast<double>(w) / 2.0;
        const double halfH = static_cast<double>(h) / 2.0;
        for (uint16_t y = 0; y < h; ++y) {
            const double sy = srcCenterY + (static_cast<double>(y) - halfH) / magnification;
            const auto ry = static_cast<int>(std::floor(sy + 0.5));
            double *dstRow = dst + static_cast<size_t>(w) * y;
            for (uint16_t x = 0; x < w; ++x) {
                const double sx = srcCenterX + (static_cast<double>(x) - halfW) / magnification;
                const auto rx = static_cast<int>(std::floor(sx + 0.5));
                // Nothing outside the old picture was ever computed, so it starts at the interior color the compute would have left there anyway.
                dstRow[x] = rx < 0 || ry < 0 || rx >= w || ry >= h
                                ? 0.0
                                : src[static_cast<size_t>(w) * ry + rx];
            }
        }
        // The compute started over this zoom is the next one, and only it draws onto this picture.
        previewSeedGeneration = computeGeneration.load() + 1;
    }

    void RenderScene::seedPreviewFromMatrix(const Matrix<double> &previous) {
        if (iterationMatrix == nullptr || renderer == nullptr ||
            renderer->iterationStagingBufferContext == nullptr) {
            return;
        }
        auto &staging = *renderer->iterationStagingBufferContext;
        const uint16_t w = iterationMatrix->getWidth();
        const uint16_t h = iterationMatrix->getHeight();
        const uint16_t pw = previous.getWidth();
        const uint16_t ph = previous.getHeight();
        if (staging.getWidth() != w || staging.getHeight() != h || w == 0 || h == 0 || pw == 0 || ph == 0) {
            return;
        }
        // The view is centred on the buffer and measured in its pixels, so the offset between the two centres is the whole of the move and every shared pixel lands on the value it was computed for.
        const int offsetX = (static_cast<int>(w) - static_cast<int>(pw)) / 2;
        const int offsetY = (static_cast<int>(h) - static_cast<int>(ph)) / 2;
        auto *dst = reinterpret_cast<double *>(staging.getContext().mappedMemory);
        std::fill_n(dst, static_cast<size_t>(w) * h, 0.0);
        for (int y = std::max(0, offsetY), yEnd = std::min<int>(h, ph + offsetY); y < yEnd; ++y) {
            const uint32_t srcRow = static_cast<uint32_t>(pw) * static_cast<uint32_t>(y - offsetY);
            double *dstRow = dst + static_cast<size_t>(w) * y;
            for (int x = std::max(0, offsetX), xEnd = std::min<int>(w, pw + offsetX); x < xEnd; ++x) {
                dstRow[x] = previous.loadRelaxed(srcRow + static_cast<uint32_t>(x - offsetX));
            }
        }
        // The compute started over this resize is the next one, and only it draws onto this picture.
        previewSeedGeneration = computeGeneration.load() + 1;
    }

    void RenderScene::refreshRenderContext() const {
        for (auto &context: wc.getRenderContexts()) {
            context->recreate();
        }

        for (const auto &sp: renderer->configurators) {
            sp->renderContextRefreshed();
        }
    }


    void RenderScene::refreshSharedImgContext(VkFormat format) const {
        if (renderer) {
            renderer->lastShadedFrame = -1;
        }
        if (format == VK_FORMAT_UNDEFINED) {
            format = attr.shader.sceneLinear() ? VK_FORMAT_R16G16B16A16_SFLOAT : VK_FORMAT_R16G16B16A16_UNORM;
        }
        using namespace SharedImageContextIndices;
        auto &sharedImg = wc.getSharedImageContext();
        sharedImg.cleanupContexts();
        auto iiiGetter = [](const VkExtent2D extent, const VkFormat format, const VkImageUsageFlags usage) {
            return vkh::ImageInitInfo{
                .imageType = VK_IMAGE_TYPE_2D,
                .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
                .imageFormat = format,
                .extent = {extent.width, extent.height, 1},
                .useMipmap = VK_FALSE,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .imageTiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = usage,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            };
        };

        const auto internalImageExtent = getInternalImageExtent();
        const auto blurredImageExtent = getBlurredImageExtent();

        sharedImg.appendMultiframeImageContext(MF_MAIN_RENDER_IMAGE_PRIMARY,
                                               iiiGetter(internalImageExtent, format,
                                                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                         VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                                                         VK_IMAGE_USAGE_SAMPLED_BIT));
        sharedImg.appendMultiframeImageContext(MF_MAIN_RENDER_IMAGE_SECONDARY,
                                               iiiGetter(internalImageExtent, format,
                                                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                                         VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                                                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                         VK_IMAGE_USAGE_SAMPLED_BIT));
        sharedImg.appendMultiframeImageContext(MF_MAIN_RENDER_DOWNSAMPLED_IMAGE_PRIMARY,
                                               iiiGetter(blurredImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT,
                                                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                         VK_IMAGE_USAGE_SAMPLED_BIT |
                                                         VK_IMAGE_USAGE_STORAGE_BIT));
        sharedImg.appendMultiframeImageContext(MF_MAIN_RENDER_DOWNSAMPLED_IMAGE_SECONDARY,
                                               iiiGetter(blurredImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT,
                                                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                         VK_IMAGE_USAGE_SAMPLED_BIT |
                                                         VK_IMAGE_USAGE_STORAGE_BIT));
    }

    namespace {
        // The zoom ratio as the status bar states it, mantissa and power of ten. A map file carries
        // the zoom it was computed at, the same value a keyframe's `.rfsm` holds, so a map put on the
        // canvas can say where it is even though nothing has been recomputed.
        std::wstring zoomStatus(const float logZoom) {
            return std::format(L"Z : {:.06f}E{:d}", pow(10, fmod(logZoom, 1)), static_cast<int>(logZoom));
        }
    }

    void RenderScene::cancelRunningCompute() {
        endSmoothZoom();
        smoothZoomPending = 0;
        // The pending request goes with the run: left standing it would start the same compute on
        // the next frame and put it right back over whatever was just loaded.
        requests.recomputeRequested.exchange(false);
        state.cancel();
        previewUploadPending.exchange(false);
        idleCompute = true;
        backgroundThreads.notifyAll();
    }

    bool RenderScene::overwriteMatrixFromMap(const RFFDynamicMapBinary &map) {
        ++previewRevision;
        const uint32_t iw = getIterationBufferWidth(attr);
        const uint32_t ih = getIterationBufferHeight(attr);
        if (iw != map.getMatrix().getWidth() || ih != map.getMatrix().getHeight()) {
            vkh::logger::log_err("Map size mismatch, {}x{} required but provided {}x{}", iw, ih,
                                 map.getMatrix().getWidth(), map.getMatrix().getHeight());
            return false;
        }
        auto loadedMatrix = std::make_unique<Matrix<double>>(map.getMatrix());
        // Both walks put something of their own on the canvas, so only one may hold it. Taken away
        // before the zoom below is written, because that is the line this would otherwise restore
        // over: the map's zoom is what belongs on the bar once the map is what is being shown.
        endImageBrowse();
        // A compute still running owns this buffer through its preview snapshots: the map opened
        // here would be back under the half-finished view within the next frame or two, so the run
        // it belongs to is stopped rather than raced with.
        cancelRunningCompute();
        wc.core.getLogicalDevice().waitDeviceIdle();

        renderer->rendererIteration->setMaxIteration(static_cast<double>(map.getMaxIteration()));
        renderer->iterationStagingBufferContext->fill(map.getMatrix().getCanvas());
        iterationMatrix = std::move(loadedMatrix);
        lastLogZoom = map.getLogZoom();
        lastPeriod = map.getPeriod();
        lastMaxIteration = map.getMaxIteration();
        previewSeedGeneration = 0;
        // The canvas is now the map's view, not the one last computed, so the zoom shown follows it.
        setStatusMessage(Constants::Status::ZOOM_STATUS, zoomStatus(map.getLogZoom()));
        return true;
    }

    void RenderScene::beginMapBrowse(const std::filesystem::path &loaded) {
        browsedMaps.clear();
        browsedMapIndex = -1;

        const std::wstring dynamicExt = std::format(L".{}", Constants::Extension::DYNAMIC_MAP);
        const std::wstring compressedExt = std::format(L".{}", Constants::Extension::COMPRESSED_MAP);
        std::error_code ec;
        for (const auto &entry: std::filesystem::directory_iterator(loaded.parent_path(), ec)) {
            if (!entry.is_regular_file(ec)) {
                continue;
            }
            std::wstring ext = entry.path().extension().wstring();
            std::ranges::transform(ext, ext.begin(), [](const wchar_t c) { return std::towlower(c); });
            if (ext == dynamicExt || ext == compressedExt) {
                browsedMaps.push_back(entry.path());
            }
        }
        std::ranges::sort(browsedMaps, [](const std::filesystem::path &x, const std::filesystem::path &y) {
            return Utilities::naturalLess(x.filename().wstring(), y.filename().wstring());
        });

        // Case-folded: the dialog may hand back a name spelled differently from the one on disk.
        const auto folded = [](const std::filesystem::path &p) {
            std::wstring name = p.filename().wstring();
            std::ranges::transform(name, name.begin(), [](const wchar_t c) { return std::towlower(c); });
            return name;
        };
        const std::wstring target = folded(loaded);
        for (size_t i = 0; i < browsedMaps.size(); ++i) {
            if (folded(browsedMaps[i]) == target) {
                browsedMapIndex = static_cast<int>(i);
                break;
            }
        }
        browsedMapTyping = false;
        browsedMapTyped.clear();
        if (browsedMapIndex >= 0) {
            setStatusMessage(Constants::Status::RENDER_STATUS, browsedMapStatus(browsedMapIndex));
        }
    }

    void RenderScene::beginImageBrowse(const std::filesystem::path &loaded) {
        // Both walks put something of their own on the canvas, so only one of them may hold it.
        endMapBrowse();
        browsedImages.clear();
        browsedImageIndex = -1;

        const std::wstring imageExt = std::format(L".{}", Constants::Extension::IMAGE);
        std::error_code ec;
        for (const auto &entry: std::filesystem::directory_iterator(loaded.parent_path(), ec)) {
            if (entry.is_regular_file(ec) && Utilities::lowerExtension(entry.path()) == imageExt) {
                browsedImages.push_back(entry.path());
            }
        }
        std::ranges::sort(browsedImages, [](const std::filesystem::path &x, const std::filesystem::path &y) {
            return Utilities::naturalLess(x.filename().wstring(), y.filename().wstring());
        });

        // Case-folded: the dialog may hand back a name spelled differently from the one on disk.
        const auto folded = [](const std::filesystem::path &p) {
            std::wstring name = p.filename().wstring();
            std::ranges::transform(name, name.begin(), [](const wchar_t c) { return std::towlower(c); });
            return name;
        };
        const std::wstring target = folded(loaded);
        for (size_t i = 0; i < browsedImages.size(); ++i) {
            if (folded(browsedImages[i]) == target) {
                browsedImageIndex = static_cast<int>(i);
                break;
            }
        }
        // A folder that cannot be walked still shows the one file the dialog was answered with.
        if (browsedImageIndex < 0) {
            browsedImages.clear();
            browsedImages.push_back(loaded);
            browsedImageIndex = 0;
        }
        applyBrowsedImage(browsedImageIndex);
    }

    void RenderScene::endImageBrowse() {
        if (browsedImageIndex < 0) {
            return;
        }
        browsedImages.clear();
        browsedImageIndex = -1;
        if (imageCanvas != nullptr) {
            imageCanvas->hide();
            // Hiding it hands the uncovered strip back to the canvas as an area to repaint, and the
            // canvas answers a repaint with the black brush of its class - one black frame, in the
            // gap before the next present. Nothing has to be painted there at all: the canvas holds
            // the finished view and presents it again within the frame, so the area is marked
            // painted and the pixels are left to that present. No compute is asked for here: the
            // fractal underneath was never taken down, only covered.
            RedrawWindow(wc.getWindow().getWindowHandle(), nullptr, nullptr,
                         RDW_VALIDATE | RDW_NOERASE | RDW_NOCHILDREN);
        }
        setStatusMessage(Constants::Status::RENDER_STATUS, L"");
        // The zoom on the bar belonged to the picture, not to what is on the canvas again now.
        setStatusMessage(Constants::Status::ZOOM_STATUS, zoomStatus(attr.fractal.logZoom));
    }

    std::wstring RenderScene::browsedImageStatus(const int index) const {
        return std::format(L"I : {}/{}", index + 1, browsedImages.size());
    }

    RECT RenderScene::imageCanvasArea() const {
        RECT area = {};
        GetClientRect(wc.getWindow().getWindowHandle(), &area);
        return area;
    }

    void RenderScene::layoutImageCanvas(const RECT &area) const {
        if (imageCanvas != nullptr) {
            imageCanvas->layout(area);
        }
    }

    void RenderScene::applyBrowsedImage(const int index) {
        browsedImageIndex = index;
        if (imageCanvas == nullptr) {
            imageCanvas = std::make_unique<ImageCanvas>();
            // A picture is left the way a loaded map is: by reaching for the fractal with the mouse.
            // The press lands on the window holding the picture, never on the canvas under it, so
            // that window is what hands the canvas back.
            imageCanvas->setDismissCallback([this] { endImageBrowse(); });
        }
        const HWND canvas = wc.getWindow().getWindowHandle();
        const HWND host = GetParent(canvas);
        RECT area = imageCanvasArea();
        // The picture sits over the canvas, so it is placed in the coordinates of what holds both.
        MapWindowPoints(canvas, host, reinterpret_cast<POINT *>(&area), 2);
        imageCanvas->show(host, area, browsedImages[index]);
        setStatusMessage(Constants::Status::RENDER_STATUS, browsedImageStatus(index));

        // A keyframe run writes 0002.rfsm beside 0002.png and that header carries the zoom, so a
        // picture sitting next to its keyframe can say where it stands. One saved on its own has no
        // such file, and the bar keeps naming the view the canvas holds underneath.
        std::filesystem::path keyframe = browsedImages[index];
        keyframe.replace_extension(std::format(L".{}", Constants::Extension::STATIC_MAP));
        if (const RFFStaticMapBinary map = RFFStaticMapBinary::read(keyframe); map.hasData()) {
            setStatusMessage(Constants::Status::ZOOM_STATUS, zoomStatus(map.getLogZoom()));
        } else {
            setStatusMessage(Constants::Status::ZOOM_STATUS, zoomStatus(attr.fractal.logZoom));
        }
    }

    void RenderScene::stepBrowsedImage(const int delta) {
        if (browsedImageIndex < 0) {
            return;
        }
        const int last = static_cast<int>(browsedImages.size()) - 1;
        const int target = std::clamp(browsedImageIndex + delta, 0, last);
        if (target == browsedImageIndex) {
            setStatusMessage(Constants::Status::RENDER_STATUS, browsedImageStatus(browsedImageIndex));
            return;
        }
        applyBrowsedImage(target);
    }

    void RenderScene::jumpBrowsedImage(const bool last) {
        if (browsedImageIndex < 0) {
            return;
        }
        const int target = last ? static_cast<int>(browsedImages.size()) - 1 : 0;
        if (target == browsedImageIndex) {
            setStatusMessage(Constants::Status::RENDER_STATUS, browsedImageStatus(browsedImageIndex));
            return;
        }
        applyBrowsedImage(target);
    }

    void RenderScene::endMapBrowse() {
        browsedMaps.clear();
        browsedMapIndex = -1;
        browsedMapTyping = false;
        browsedMapTyped.clear();
    }

    std::wstring RenderScene::browsedMapStatus(const int index) const {
        return std::format(L"M : {}/{}", index + 1, browsedMaps.size());
    }

    void RenderScene::applyBrowsedMap(const int index) {
        browsedMapIndex = index;
        const RFFDynamicMapBinary map = RFFDynamicMapBinary::readAny(browsedMaps[index]);
        if (!map.hasData() || !overwriteMatrixFromMap(map)) {
            setStatusMessage(Constants::Status::RENDER_STATUS,
                             std::format(L"{} (skipped)", browsedMapStatus(index)));
            return;
        }
        setStatusMessage(Constants::Status::RENDER_STATUS, browsedMapStatus(index));
    }

    void RenderScene::stepBrowsedMap(const int delta) {
        if (browsedMapIndex < 0) {
            return;
        }
        browsedMapTyping = false;
        browsedMapTyped.clear();
        const int last = static_cast<int>(browsedMaps.size()) - 1;
        const int target = std::clamp(browsedMapIndex + delta, 0, last);
        if (target == browsedMapIndex) {
            setStatusMessage(Constants::Status::RENDER_STATUS, browsedMapStatus(browsedMapIndex));
            return;
        }
        applyBrowsedMap(target);
    }

    void RenderScene::jumpBrowsedMap(const bool last) {
        if (browsedMapIndex < 0) {
            return;
        }
        browsedMapTyping = false;
        browsedMapTyped.clear();
        const int target = last ? static_cast<int>(browsedMaps.size()) - 1 : 0;
        if (target == browsedMapIndex) {
            setStatusMessage(Constants::Status::RENDER_STATUS, browsedMapStatus(browsedMapIndex));
            return;
        }
        applyBrowsedMap(target);
    }

    bool RenderScene::armBrowsedMapTyping() {
        if (browsedMapIndex < 0) {
            return false;
        }
        browsedMapTyping = true;
        browsedMapTyped.clear();
        showTypedBrowsedMapPosition();
        return true;
    }

    void RenderScene::showTypedBrowsedMapPosition() const {
        // The trailing underscore is the caret: it says the number is still being typed.
        setStatusMessage(Constants::Status::RENDER_STATUS,
                         std::format(L"M : [{}_]/{}", browsedMapTyped, browsedMaps.size()));
    }

    bool RenderScene::typeBrowsedMapPosition(const WPARAM key) {
        if (browsedMapIndex < 0) {
            return false;
        }
        // The digit row and the numeric keypad both count.
        int digit = -1;
        if (key >= L'0' && key <= L'9') {
            digit = static_cast<int>(key - L'0');
        }
        if (key >= VK_NUMPAD0 && key <= VK_NUMPAD9) {
            digit = static_cast<int>(key - VK_NUMPAD0);
        }
        if (digit >= 0) {
            browsedMapTyping = true;
            // Nine digits address more maps than a folder can hold, and keep the number in range.
            if (browsedMapTyped.size() < 9) {
                browsedMapTyped.push_back(static_cast<wchar_t>(L'0' + digit));
            }
            showTypedBrowsedMapPosition();
            return true;
        }
        if (!browsedMapTyping) {
            // No number is being typed, so these keys belong to whoever else wants them.
            return false;
        }
        switch (key) {
            case VK_BACK: {
                if (!browsedMapTyped.empty()) {
                    browsedMapTyped.pop_back();
                }
                showTypedBrowsedMapPosition();
                return true;
            }
            case VK_RETURN: {
                const std::wstring typed = std::exchange(browsedMapTyped, {});
                browsedMapTyping = false;
                if (typed.empty()) {
                    setStatusMessage(Constants::Status::RENDER_STATUS, browsedMapStatus(browsedMapIndex));
                    return true;
                }
                applyBrowsedMap(std::clamp(std::stoi(typed) - 1, 0, static_cast<int>(browsedMaps.size()) - 1));
                return true;
            }
            case VK_ESCAPE: {
                browsedMapTyping = false;
                browsedMapTyped.clear();
                setStatusMessage(Constants::Status::RENDER_STATUS, browsedMapStatus(browsedMapIndex));
                return true;
            }
            default: return false;
        }
    }

    bool RenderScene::runKeyAction(const WPARAM key) {
        if (key == VK_ESCAPE && (smoothZoomActive || smoothZoomPending != 0)) {
            settleSmoothNavigation();
            smoothZoomDragging = false;
            canvasDragging = false;
            smoothZoomNeedsPreview = false;
            smoothZoomInputAt = std::chrono::steady_clock::now() - std::chrono::seconds(1);
            smoothZoomPending = 0;
            return true;
        }
        // The iteration buffer is being written by the job in either case; browsing would fight it.
        if (isVideoGenerationActive || isVideoExportActive || longJobBusy.load()) {
            return false;
        }
        // A picture over the canvas takes the same keys first, because it is what is being looked at.
        if (browsedImageIndex >= 0) {
            switch (key) {
                case VK_LEFT: stepBrowsedImage(-1);
                    return true;
                case VK_RIGHT: stepBrowsedImage(1);
                    return true;
                case VK_UP: stepBrowsedImage(-Constants::Win32::MAP_BROWSE_COARSE_STEP);
                    return true;
                case VK_DOWN: stepBrowsedImage(Constants::Win32::MAP_BROWSE_COARSE_STEP);
                    return true;
                case VK_HOME: jumpBrowsedImage(false);
                    return true;
                case VK_END: jumpBrowsedImage(true);
                    return true;
                case VK_ESCAPE: endImageBrowse();
                    return true;
                default: return false;
            }
        }
        // Every key left walks a folder of maps, so without one they all pass through.
        if (browsedMapIndex < 0) {
            return false;
        }
        if (typeBrowsedMapPosition(key)) {
            return true;
        }
        switch (key) {
            case VK_LEFT: stepBrowsedMap(-1);
                return true;
            case VK_RIGHT: stepBrowsedMap(1);
                return true;
            case VK_UP: stepBrowsedMap(-Constants::Win32::MAP_BROWSE_COARSE_STEP);
                return true;
            case VK_DOWN: stepBrowsedMap(Constants::Win32::MAP_BROWSE_COARSE_STEP);
                return true;
            case VK_HOME: jumpBrowsedMap(false);
                return true;
            case VK_END: jumpBrowsedMap(true);
                return true;
            default: return false;
        }
    }

    uint16_t RenderScene::getMouseXOnIterationBuffer() const {
        POINT cursor;
        GetCursorPos(&cursor);
        ScreenToClient(wc.getWindow().getWindowHandle(), &cursor);
        // iteration-buffer pixel scale is clarity * ssaa.
        RECT bounds;
        GetClientRect(wc.getWindow().getWindowHandle(), &bounds);
        return workspace::PreviewGeometry::sample(cursor.x, bounds.right, getIterationBufferWidth(attr));
    }

    uint16_t RenderScene::getMouseYOnIterationBuffer() const {
        POINT cursor;
        GetCursorPos(&cursor);
        ScreenToClient(wc.getWindow().getWindowHandle(), &cursor);
        // The -1 mirrors the shaders' row flip (height - 1 - y); without it the top row maps past the buffer end.
        RECT bounds;
        GetClientRect(wc.getWindow().getWindowHandle(), &bounds);
        return workspace::PreviewGeometry::sample(cursor.y, bounds.bottom, getIterationBufferHeight(attr), true);
    }

    namespace {
        // A layered window blends from premultiplied BGRA, so each channel carries the alpha already.
        uint32_t premultipliedPixel(const COLORREF color, const BYTE alpha) {
            const uint32_t a = alpha;
            const uint32_t b = GetBValue(color) * a / 255;
            const uint32_t g = GetGValue(color) * a / 255;
            const uint32_t r = GetRValue(color) * a / 255;
            return b | g << 8 | r << 16 | a << 24;
        }

        // Writes the box-zoom frame - white halo, cyan line, white halo - onto the outermost pixels
        // of a width x height picture, or clears those same pixels when clear is set. Only the band
        // is touched: the interior is transparent and stays that way.
        void paintBoxZoomBand(uint32_t *pixels, const int stride, const int width, const int height,
                              const bool clear) {
            using namespace Constants::Win32;
            if (pixels == nullptr || width <= 0 || height <= 0) {
                return;
            }
            constexpr int outline = BOX_ZOOM_OUTLINE_THICKNESS;
            constexpr int line = BOX_ZOOM_BORDER_THICKNESS;
            constexpr int total = line + 2 * outline;
            const uint32_t white = premultipliedPixel(COLOR_BOX_ZOOM_OUTLINE, BOX_ZOOM_OVERLAY_ALPHA);
            const uint32_t cyan = premultipliedPixel(COLOR_BOX_ZOOM_OVERLAY, BOX_ZOOM_OVERLAY_ALPHA);
            for (int y = 0; y < height; ++y) {
                const bool wholeRow = y < total || y >= height - total;
                uint32_t *row = pixels + static_cast<size_t>(stride) * y;
                for (int x = 0; x < width; ++x) {
                    if (!wholeRow && x >= total && x < width - total) {
                        // Inside the box: no part of the frame reaches here, and the next column that does is the far band.
                        x = width - total - 1;
                        continue;
                    }
                    if (clear) {
                        row[x] = 0;
                        continue;
                    }
                    // How deep the pixel sits under the nearest edge is the whole of which band it belongs to.
                    const int depth = std::min({x, y, width - 1 - x, height - 1 - y});
                    row[x] = depth < outline || depth >= outline + line ? white : cyan;
                }
            }
        }
    }

    void RenderScene::ensureBoxZoomOverlay() {
        if (boxZoomOverlay != nullptr) {
            return;
        }
        boxZoomOverlay = CreateWindowExW(
            Constants::Win32::STYLE_EX_BOX_ZOOM_OVERLAY,
            Constants::Win32::CLASS_BOX_ZOOM_OVERLAY,
            L"",
            Constants::Win32::STYLE_BOX_ZOOM_OVERLAY,
            0, 0, 0, 0, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        // No SetLayeredWindowAttributes: the alpha is premultiplied into the picture handed over below.
    }

    bool RenderScene::ensureBoxZoomOverlayBitmap(const int width, const int height) {
        if (boxZoomOverlayPixels != nullptr && width <= boxZoomOverlayCapacityW &&
            height <= boxZoomOverlayCapacityH) {
            return true;
        }
        // Grown to cover both what is asked for and what it already held, so it is only ever built up.
        const int capacityW = std::max(width, boxZoomOverlayCapacityW);
        const int capacityH = std::max(height, boxZoomOverlayCapacityH);
        destroyBoxZoomOverlayBitmap();

        BITMAPINFO info = {};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = capacityW;
        // Top-down, so the first row of the picture is the top edge of the box.
        info.bmiHeader.biHeight = -capacityH;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;

        void *bits = nullptr;
        const HDC screen = GetDC(nullptr);
        boxZoomOverlayDC = CreateCompatibleDC(screen);
        ReleaseDC(nullptr, screen);
        if (boxZoomOverlayDC == nullptr) {
            return false;
        }
        boxZoomOverlayBitmap = CreateDIBSection(boxZoomOverlayDC, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (boxZoomOverlayBitmap == nullptr || bits == nullptr) {
            destroyBoxZoomOverlayBitmap();
            return false;
        }
        boxZoomOverlayPreviousBitmap = SelectObject(boxZoomOverlayDC, boxZoomOverlayBitmap);
        boxZoomOverlayPixels = static_cast<uint32_t *>(bits);
        boxZoomOverlayCapacityW = capacityW;
        boxZoomOverlayCapacityH = capacityH;
        // A fresh section comes zeroed, so nothing of the frame it replaces is left to clear.
        boxZoomOverlayDrawnW = 0;
        boxZoomOverlayDrawnH = 0;
        return true;
    }

    void RenderScene::destroyBoxZoomOverlayBitmap() {
        if (boxZoomOverlayDC != nullptr) {
            if (boxZoomOverlayPreviousBitmap != nullptr) {
                SelectObject(boxZoomOverlayDC, boxZoomOverlayPreviousBitmap);
                boxZoomOverlayPreviousBitmap = nullptr;
            }
            DeleteDC(boxZoomOverlayDC);
            boxZoomOverlayDC = nullptr;
        }
        if (boxZoomOverlayBitmap != nullptr) {
            DeleteObject(boxZoomOverlayBitmap);
            boxZoomOverlayBitmap = nullptr;
        }
        boxZoomOverlayPixels = nullptr;
        boxZoomOverlayCapacityW = 0;
        boxZoomOverlayCapacityH = 0;
        boxZoomOverlayDrawnW = 0;
        boxZoomOverlayDrawnH = 0;
    }

    void RenderScene::updateBoxZoomOverlay(const POINT anchorScreen, const POINT currentScreen) {
        if (boxZoomOverlay == nullptr) {
            return;
        }
        const int left = std::min(anchorScreen.x, currentScreen.x);
        const int top = std::min(anchorScreen.y, currentScreen.y);
        const int width = std::abs(currentScreen.x - anchorScreen.x);
        const int height = std::abs(currentScreen.y - anchorScreen.y);

        if (width <= 0 || height <= 0) {
            ShowWindow(boxZoomOverlay, SW_HIDE);
            return;
        }
        if (!ensureBoxZoomOverlayBitmap(width, height)) {
            return;
        }

        // The frame sits on the edges of the box, so a box that only moves already has its picture.
        if (width != boxZoomOverlayDrawnW || height != boxZoomOverlayDrawnH) {
            paintBoxZoomBand(boxZoomOverlayPixels, boxZoomOverlayCapacityW,
                             boxZoomOverlayDrawnW, boxZoomOverlayDrawnH, true);
            paintBoxZoomBand(boxZoomOverlayPixels, boxZoomOverlayCapacityW, width, height, false);
            boxZoomOverlayDrawnW = width;
            boxZoomOverlayDrawnH = height;
        }

        // Position, size and every pixel in one call, so the window is never on screen holding a
        // surface no picture has been put in yet.
        POINT destination = {left, top};
        POINT source = {0, 0};
        SIZE size = {width, height};
        BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
        UpdateLayeredWindow(boxZoomOverlay, nullptr, &destination, &size, boxZoomOverlayDC, &source,
                            0, &blend, ULW_ALPHA);
        // Shown after the picture is in, and already moved by the call above, so this only raises it.
        SetWindowPos(boxZoomOverlay, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }

    void RenderScene::hideBoxZoomOverlay() const {
        if (boxZoomOverlay != nullptr) {
            ShowWindow(boxZoomOverlay, SW_HIDE);
        }
    }

    LRESULT CALLBACK RenderScene::boxZoomOverlayProc(const HWND hwnd, const UINT msg, const WPARAM wparam,
                                                     const LPARAM lparam) {
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    std::pair<Matrix<double>, uint64_t> RenderScene::sampleIterationsForAi() const {
        if (!iterationMatrix || !idleCompute || previewUploadPending || smoothZoomActive ||
            requests.recomputeRequested || requests.resizeRequested || isLongJobBusy()) {
            throw std::runtime_error("Wait for completed iteration data before AI analysis.");
        }

        const auto width = iterationMatrix->getWidth();
        const auto height = iterationMatrix->getHeight();
        if (width < 3 || height < 3) {
            throw std::runtime_error("Iteration image is too small for AI analysis.");
        }

        const double scale = std::min(1.0, 192.0 / std::max(width, height));
        const auto sampleWidth = uint16_t(std::max(3, int(width * scale)));
        const auto sampleHeight = uint16_t(std::max(3, int(height * scale)));
        Matrix<double> sample(sampleWidth, sampleHeight);

        for (uint16_t y = 0; y < sampleHeight; ++y) {
            for (uint16_t x = 0; x < sampleWidth; ++x) {
                const auto sourceX = uint16_t(uint32_t(x) * (width - 1) / (sampleWidth - 1));
                const auto sourceY = uint16_t(uint32_t(y) * (height - 1) / (sampleHeight - 1));
                sample(x, y) = iterationMatrix->loadRelaxed(iterationMatrix->getIndex(sourceX, sourceY));
            }
        }
        return {std::move(sample), lastMaxIteration};
    }

    void RenderScene::zoomToImagePoint(double x, double y, double factor) {
        if (!std::isfinite(x) || !std::isfinite(y) || x < 0 || x > 1 || y < 0 || y > 1 ||
            !std::isfinite(factor) || factor <= 1 || factor > 100) {
            throw std::runtime_error("Zoom requires image coordinates 0-1 and a factor greater than 1 and at most 100.");
        }
        if (!isIdleCompute() || isLongJobBusy() || requests.recomputeRequested ||
            requests.resizeRequested || isImageBrowsing() || isVideoGenerationActive || isVideoExportActive) {
            throw std::runtime_error("Wait for the current fractal render before zooming.");
        }
        if (effectiveProjection(attr.fractal.projectionMethod) != FrtProjectionMethod::PLANAR) {
            throw std::runtime_error("AI zoom currently requires planar projection.");
        }

        const double pixelX = x * (getIterationBufferWidth(attr) - 1);
        const double pixelY = y * (getIterationBufferHeight(attr) - 1);
        const auto offset = offsetConversion(attr, pixelX, pixelY);
        const float previousLogZoom = attr.fractal.logZoom;
        const float nextLogZoom = previousLogZoom + std::log10(factor);
        if (!std::isfinite(nextLogZoom) || nextLogZoom <= previousLogZoom) {
            throw std::runtime_error("Zoom increment cannot be represented at this depth.");
        }

        auto center = attr.fractal.center.addCenterDouble(
            offset[0], offset[1], Perturbator::logZoomToExp10(nextLogZoom));
        attr.fractal.center = std::move(center);
        attr.fractal.logZoom = nextLogZoom;
        seedPreviewFromZoom(pixelX, pixelY,
                            std::pow(10.0, double(nextLogZoom - previousLogZoom)));
        requests.requestRecompute();
    }

    void RenderScene::applyBoxZoom(const int startMX, const int startMY, const int endMX, const int endMY) {
        const int bw = std::abs(endMX - startMX);
        const int bh = std::abs(endMY - startMY);
        if (bw == 0 || bh == 0) {
            return;
        }

        // Re-center on the box center (offset captured at the current zoom level).
        const int bcx = (startMX + endMX) / 2;
        const int bcy = (startMY + endMY) / 2;
        const std::array<dex, 2> offset = offsetConversion(attr, bcx, bcy);

        // Zoom so the selected box fits entirely inside the viewport.
        const float bufW = static_cast<float>(getIterationBufferWidth(attr));
        const float bufH = static_cast<float>(getIterationBufferHeight(attr));
        const float factor = std::min(bufW / static_cast<float>(bw), bufH / static_cast<float>(bh));

        float &logZoom = attr.fractal.logZoom;
        const float previousLogZoom = logZoom;
        logZoom = std::max(Constants::Fractal::ZOOM_MIN, logZoom + std::log10(factor));
        attr.fractal.center = attr.fractal.center.addCenterDouble(
            offset[0], offset[1], Perturbator::logZoomToExp10(logZoom));

        // The box that was drawn is exactly what the canvas is about to hold, so the picture on
        // screen is magnified into it before the compute starts. Without this the canvas blanks to
        // the interior color the moment the drag ends and only fills back in as the map arrives,
        // which reads as the view blinking. The magnification is read off the zoom that was really
        // applied, not off the box, so a zoom the lower limit cut short still lands on its picture.
        seedPreviewFromZoom(static_cast<double>(bcx), static_cast<double>(bcy),
                            std::pow(10.0, static_cast<double>(logZoom - previousLogZoom)));

        requests.requestRecompute();
    }

    void RenderScene::recomputeThreaded(const Attribute* overrideSettings, const bool lowResolution) {
        // Written before the compute starts, not after it ends: this is the view a run that never
        // comes back from here was working on.
        writeRecoverySnapshot(true);
        computeStartedAt = std::chrono::steady_clock::now();
        // A recompute puts a freshly computed view where the loaded map was, so the folder it came
        // from is no longer what the canvas holds: the keys and the status bar stop answering for it.
        endMapBrowse();
        // A picture standing over the canvas would hide the very view being computed.
        endImageBrowse();
        // createThread cancels and joins the running compute, so its afterCompute lands after the render
        // loop has already cleared idleCompute for this one. Each compute carries the generation it started
        // with and only the newest may declare the scene idle, or a waiter (the keyframe writer) would be
        // released while this compute is still filling the map.
        const uint64_t generation = ++computeGeneration;
        // The previous run is joined here rather than inside createThread, so it is finished with
        // the period beforeCompute is about to read and with the device the uniform below is in.
        state.cancel();
        // The joined worker may have published completion after the caller marked the next run busy.
        idleCompute = false;
        wc.core.getLogicalDevice().waitDeviceIdle();
        // Cloned and prepared on this thread, not on the worker: beforeCompute writes the iteration
        // uniform buffer the drawing reads, and a worker writing it alongside a frame in flight is
        // rewriting what the GPU is already fetching. Here the device is idle and nothing is.
        Attribute settings = overrideSettings ? *overrideSettings : attr; //clone the attr
        beforeCompute(settings);
        Attribute geometry = settings;
        smoothZoomPreviewMatrix.reset();
        if (lowResolution) {
            settings.render.ssaa = 1;
            settings.render.clarityMultiplier = SmoothZoomMotion::previewClarity(settings.render.clarityMultiplier,
                getClientWidth(), getClientHeight(), settings.video.data.sourceScale);
            if (getIterationBufferWidth(settings) == 0 || getIterationBufferHeight(settings) == 0)
                settings.render.clarityMultiplier = geometry.render.clarityMultiplier * geometry.render.ssaa;
            settings.render.coarsePreview = false;
            smoothZoomPreviewMatrix = std::make_unique<Matrix<double>>(getIterationBufferWidth(settings), getIterationBufferHeight(settings));
        }
        auto* output = lowResolution ? smoothZoomPreviewMatrix.get() : iterationMatrix.get();
        state.createThread([this, generation, output, geometry = std::move(geometry), settings = std::move(settings)](const std::stop_token &) {
            try {
                const bool success = compute(settings, output, &geometry);
                afterCompute(success, generation);
            } catch (const std::exception &error) {
                afterCompute(false, generation);
                try {
                    vkh::logger::log_err_silent("Recompute failed: {}", error.what());
                } catch (...) {
                }
            } catch (...) {
                afterCompute(false, generation);
                try {
                    vkh::logger::log_err_silent("Recompute failed with an unknown exception");
                } catch (...) {
                }
            }
        });
    }

    void RenderScene::beforeCompute(Attribute &attr) const {
        attr.fractal.maxIteration = attr.fractal.autoMaxIteration
                                        ? NumericSettingLimits::automaticIterationLimit(lastPeriod, attr.fractal.autoIterationMultiplier)
                                        : this->attr.fractal.maxIteration;
        renderer->rendererIteration->setMaxIteration(static_cast<double>(attr.fractal.maxIteration));
    }

    bool RenderScene::buildPerturbator(const Attribute &attr, const dex &dcMax,
                                       std::chrono::high_resolution_clock::time_point start) {
        auto &calc = attr.fractal;
        const float logZoom = calc.logZoom;
        const auto refreshInterval = Utilities::getRefreshInterval(logZoom);
        std::function actionPerRefCalcIteration = [refreshInterval, this, &start](const uint64_t p) {
            if (p % refreshInterval == 0) {
                setStatusMessage(Constants::Status::RENDER_STATUS, std::format(std::locale(), L"P : {:L}", p));
                setStatusMessage(Constants::Status::TIME_STATUS, Utilities::elapsed_time(start));
            }
        };
        std::function actionPerCreatingTableIteration = [refreshInterval, this, &start
                ](const uint64_t p, const double i) {
            if (p % refreshInterval == 0) {
                setStatusMessage(Constants::Status::RENDER_STATUS, std::format(L"A : {:.3f}%", i * 100));
                setStatusMessage(Constants::Status::TIME_STATUS, Utilities::elapsed_time(start));
            }
        };



        if (state.interruptRequested()) return false;

        if (calc.formulaType == FractalFormulaType::CUSTOM) {
             currentPerturbator = std::make_unique<CustomFormulaPerturbator>(
                 state, calc, static_cast<double>(dcMax));
        } else {
            switch (calc.reuseReferenceMethod) {
                using enum FrtReuseReferenceMethod;
            case CURRENT_REFERENCE: {
                if (auto p = dynamic_cast<DeepMandelbrotPerturbator *>(currentPerturbator.get())) {
                    currentPerturbator = p->reuse(calc, currentPerturbator->getDcMaxAsDoubleExp(), approxTableCache);
                }
                if (auto p = dynamic_cast<LightMandelbrotPerturbator *>(currentPerturbator.get())) {
                    currentPerturbator = p->reuse(calc, static_cast<double>(currentPerturbator->getDcMaxAsDoubleExp()),
                                                  approxTableCache);
                }
                break;
            }
            case CENTERED_REFERENCE: {
                uint64_t period = currentPerturbator->getReference()->longestPeriod();
                auto center = MandelbrotLocator::locateMinibrot(state, currentPerturbator.get(), approxTableCache,
                                                                CallbackExplore::getActionWhileFindingMinibrotCenter(
                                                                    *this, logZoom, period),
                                                                CallbackExplore::getActionWhileCreatingTable(
                                                                    *this, logZoom),
                                                                CallbackExplore::getActionWhileFindingZoom(*this)
                );
                if (center == nullptr) return false;

                FractalAttribute refCalc = calc;
                refCalc.center = center->perturbator->calc.center;
                refCalc.logZoom = center->perturbator->calc.logZoom;
                int refExp10 = Perturbator::logZoomToExp10(refCalc.logZoom);

                if (refCalc.logZoom > Constants::Fractal::ZOOM_DEADLINE) {
                    currentPerturbator = std::make_unique<DeepMandelbrotPerturbator>(
                                state, refCalc, center->perturbator->getDcMaxAsDoubleExp(),
                                refExp10,
                                period, approxTableCache, std::move(actionPerRefCalcIteration),
                                std::move(actionPerCreatingTableIteration))
                            ->reuse(calc, dcMax, approxTableCache);
                } else {
                    currentPerturbator = std::make_unique<LightMandelbrotPerturbator>(state, refCalc,
                                static_cast<double>(center->perturbator->getDcMaxAsDoubleExp()),
                                refExp10, period, approxTableCache, std::move(actionPerRefCalcIteration),
                                std::move(actionPerCreatingTableIteration))
                            ->reuse(calc, static_cast<double>(dcMax), approxTableCache);
                }
                break;
            }
            case DISABLED: {
                int exp10 = Perturbator::logZoomToExp10(logZoom);
                if (logZoom > Constants::Fractal::ZOOM_DEADLINE) {
                    currentPerturbator = std::make_unique<DeepMandelbrotPerturbator>(
                        state, calc, dcMax, exp10,
                        0, approxTableCache, std::move(actionPerRefCalcIteration),
                        std::move(actionPerCreatingTableIteration));
                } else {
                    currentPerturbator = std::make_unique<LightMandelbrotPerturbator>(
                        state, calc, static_cast<double>(dcMax), exp10,
                        0, approxTableCache, std::move(actionPerRefCalcIteration),
                        std::move(actionPerCreatingTableIteration));
                }
                break;
            }
            default: {
                //noop
            }
        }
        }

        const MandelbrotReference *reference = currentPerturbator->getReference();
        if (reference == Constants::NullPointer::PROCESS_TERMINATED_REFERENCE || state.interruptRequested())
            return false;

        lastLogZoom = calc.logZoom;
        lastMaxIteration = calc.maxIteration;
        lastPeriod = reference->longestPeriod();
        size_t refLength = reference->length();
        size_t mpaLen = 0;
        if (const auto t = dynamic_cast<LightMandelbrotPerturbator *>(currentPerturbator.get())) {
            mpaLen = t->getTable().getLength();
        }
        if (const auto t = dynamic_cast<DeepMandelbrotPerturbator *>(currentPerturbator.get())) {
            mpaLen = t->getTable().getLength();
        }

        setStatusMessage(Constants::Status::PERIOD_STATUS,
                         std::format(L"P : {:L} ({:L}, {:L})", lastPeriod, refLength, mpaLen));
        if (state.interruptRequested()) return false;
        return true;
    }

    bool RenderScene::compute(const Attribute &requestedAttr, Matrix<double>* output, const Attribute* samplingGeometry) {
        const Attribute& attr = requestedAttr;
        auto& matrix = output ? *output : *iterationMatrix;
        const auto& geometry = samplingGeometry ? *samplingGeometry : attr;
        auto start = std::chrono::high_resolution_clock::now();
        const uint16_t w = getIterationBufferWidth(attr);
        const uint16_t h = getIterationBufferHeight(attr);
        uint32_t len = uint32_t(w) * h;
        const double fullW = getIterationBufferWidth(geometry), fullH = getIterationBufferHeight(geometry);
        const auto coordinate = [this, &geometry, w, h, fullW, fullH](uint16_t x, uint16_t y, bool* sky) {
            return offsetConversion(geometry, SmoothZoomMotion::sourcePixel(x, static_cast<uint32_t>(fullW), w),
                SmoothZoomMotion::sourcePixel(y, static_cast<uint32_t>(fullH), h), sky);
        };

        if (state.interruptRequested()) return false;

        auto &calc = attr.fractal;

        const float logZoom = calc.logZoom;

        if (state.interruptRequested()) return false;

        setStatusMessage(Constants::Status::ZOOM_STATUS, zoomStatus(logZoom));

        const dex dcMax = dcMaxOf(geometry, static_cast<int>(fullW), static_cast<int>(fullH), 1);

        if (!buildPerturbator(attr, dcMax, start)) return false;


        std::atomic renderPixelsCount = 0;

        // Zeroed here instead of in the staging buffer: a zero is what marks a pixel this compute
        // has not reached, and the snapshot on the render thread reads that mark.
        for (uint32_t i = 0, matrixLength = matrix.getLength(); i < matrixLength; ++i) {
            matrix.storeRelaxed(i, 0);
        }

        auto statusThread = std::jthread([&renderPixelsCount, len, this, &start](const std::stop_token &stop) {
            while (!stop.stop_requested()) {
                float ratio = static_cast<float>(renderPixelsCount.load()) / static_cast<float>(len) * 100;
                setStatusMessage(Constants::Status::TIME_STATUS, Utilities::elapsed_time(start));
                setStatusMessage(Constants::Status::RENDER_STATUS, std::format(L"C : {:.3f}%", ratio));

                Sleep(Constants::Status::SET_PROCESS_INTERVAL_MS);
            }
        });

        // Boundary trace is only safe when "did not escape" is encoded as the
        // canonical maxIteration return value, i.e. the non-absolute mode.
        const bool useBoundaryTrace = attr.render.boundaryTraceFill && !attr.fractal.absoluteIterationMode;
        // White-tile fill assumes a binary in/out classification (no gradient
        // detail in the exterior), so it is gated on the 2-color preview mode.
        const bool useWhiteFill = useBoundaryTrace && attr.render.preview2Color;
        previewFillDown = !useBoundaryTrace;

        if (useBoundaryTrace) {
            const uint16_t tileSize = std::clamp<uint16_t>(
                static_cast<uint16_t>(std::min(w, h) / 8), 8, 32);
            constexpr double whiteFillValue = 1.0;
            const auto maxItValue = static_cast<double>(attr.fractal.maxIteration);
            const uint16_t tilesX = (w + tileSize - 1) / tileSize;
            const uint16_t tilesY = (h + tileSize - 1) / tileSize;
            const uint32_t numTiles = static_cast<uint32_t>(tilesX) * tilesY;

            std::atomic<uint32_t> tileIndex = 0;
            const auto computePixel = [this, &coordinate, &matrix, &renderPixelsCount, useWhiteFill, maxItValue](
                                          const uint16_t x, const uint16_t y) {
                bool sky = false;
                const auto dc = coordinate(x, y, &sky);
                const double it = sky ? maxItValue : currentPerturbator->iterate(dc[0], dc[1]);
                const double stored = (useWhiteFill && it != maxItValue) ? whiteFillValue : it;
                matrix.storeRelaxed(x, y, stored);
                ++renderPixelsCount;
                return it;
            };

            const auto fillPixel = [&matrix, &renderPixelsCount](const uint16_t x, const uint16_t y, const double v) {
                matrix.storeRelaxed(x, y, v);
                ++renderPixelsCount;
            };

            auto worker = [&] {
                while (true) {
                    const uint32_t ti = tileIndex.fetch_add(1);
                    if (ti >= numTiles) return;
                    if (state.interruptRequested()) return;

                    const uint16_t tx = static_cast<uint16_t>(ti % tilesX);
                    const uint16_t ty = static_cast<uint16_t>(ti / tilesX);
                    const uint16_t x0 = tx * tileSize;
                    const uint16_t y0 = ty * tileSize;
                    const uint16_t x1 = std::min<uint16_t>(x0 + tileSize, w);
                    const uint16_t y1 = std::min<uint16_t>(y0 + tileSize, h);

                    bool allBlack = true;
                    bool allWhite = true;

                    const auto classify = [&](const double it) {
                        if (it == maxItValue) {
                            allWhite = false;
                        } else {
                            allBlack = false;
                        }
                    };

                    for (uint16_t x = x0; x < x1; ++x) {
                        classify(computePixel(x, y0));
                        if (y1 > y0 + 1) {
                            classify(computePixel(x, static_cast<uint16_t>(y1 - 1)));
                        }
                    }
                    for (uint16_t y = static_cast<uint16_t>(y0 + 1); y + 1 < y1; ++y) {
                        classify(computePixel(x0, y));
                        if (x1 > x0 + 1) {
                            classify(computePixel(static_cast<uint16_t>(x1 - 1), y));
                        }
                    }

                    if (state.interruptRequested()) return;

                    if (allBlack) {
                        for (uint16_t y = static_cast<uint16_t>(y0 + 1); y + 1 < y1; ++y) {
                            for (uint16_t x = static_cast<uint16_t>(x0 + 1); x + 1 < x1; ++x) {
                                fillPixel(x, y, maxItValue);
                            }
                        }
                    } else if (useWhiteFill && allWhite) {
                        for (uint16_t y = static_cast<uint16_t>(y0 + 1); y + 1 < y1; ++y) {
                            for (uint16_t x = static_cast<uint16_t>(x0 + 1); x + 1 < x1; ++x) {
                                fillPixel(x, y, whiteFillValue);
                            }
                        }
                    } else {
                        for (uint16_t y = static_cast<uint16_t>(y0 + 1); y + 1 < y1; ++y) {
                            for (uint16_t x = static_cast<uint16_t>(x0 + 1); x + 1 < x1; ++x) {
                                computePixel(x, y);
                            }
                            if (state.interruptRequested()) return;
                        }
                    }
                }
            };

            const uint32_t workerCount = std::max<uint32_t>(1u, attr.render.threads);

            if (attr.render.coarsePreview && !getVideoGenerationActive()) {
                const uint16_t coarseStep = std::clamp<uint16_t>(tileSize, 8, 16);
                const uint16_t coarseCols = (w + coarseStep - 1) / coarseStep;
                const uint16_t coarseRows = (h + coarseStep - 1) / coarseStep;
                const uint32_t coarseCount = static_cast<uint32_t>(coarseCols) * coarseRows;
                std::atomic<uint32_t> coarseIndex = 0;

                auto coarseWorker = [&] {
                    while (true) {
                        const uint32_t ci = coarseIndex.fetch_add(1);
                        if (ci >= coarseCount) return;
                        if (state.interruptRequested()) return;

                        const uint16_t cx = static_cast<uint16_t>(ci % coarseCols);
                        const uint16_t cy = static_cast<uint16_t>(ci / coarseCols);
                        const uint16_t sx = cx * coarseStep;
                        const uint16_t sy = cy * coarseStep;

                        bool sky = false;
                        const auto dc = coordinate(sx, sy, &sky);
                        const double it = sky ? maxItValue : currentPerturbator->iterate(dc[0], dc[1]);
                        const double stored = (useWhiteFill && it != maxItValue) ? whiteFillValue : it;

                        const uint16_t bx1 = std::min<uint16_t>(sx + coarseStep, w);
                        const uint16_t by1 = std::min<uint16_t>(sy + coarseStep, h);
                        for (uint16_t y = sy; y < by1; ++y) {
                            for (uint16_t x = sx; x < bx1; ++x) {
                                matrix.storeRelaxed(x, y, stored);
                            }
                        }
                    }
                };

                std::vector<std::jthread> coarsePool;
                coarsePool.reserve(workerCount);
                for (uint32_t t = 0; t < workerCount; ++t) {
                    coarsePool.emplace_back(coarseWorker);
                }
                for (auto &t : coarsePool) {
                    if (t.joinable()) t.join();
                }
            }

            std::vector<std::jthread> tilePool;
            tilePool.reserve(workerCount);
            for (uint32_t t = 0; t < workerCount; ++t) {
                tilePool.emplace_back(worker);
            }
            for (auto &t : tilePool) {
                if (t.joinable()) t.join();
            }
        } else {
            // The rows under the front are no longer painted from here: the snapshot carries the
            // front down as it uploads, which is what leaves this buffer to a single writer.
            auto previewer = ParallelArrayDispatcher<double>(
                state, matrix, attr.render.threads,
                [attr, this, &coordinate, &renderPixelsCount](const uint16_t x, const uint16_t y, uint16_t, uint16_t, float,
                                                 float, uint32_t, double) {
                    bool sky = false;
                    const auto dc = coordinate(x, y, &sky);
                    const double iteration = sky
                                                 ? static_cast<double>(attr.fractal.maxIteration)
                                                 : currentPerturbator->iterate(dc[0], dc[1]);
                    ++renderPixelsCount;
                    return iteration;
                });

            previewer.dispatch();
        }

        statusThread.request_stop();
        statusThread.join();

        if (state.interruptRequested()) return false;

        // The exact map reaches the staging buffer through the render thread's completing snapshot,
        // so nothing here writes the buffer a transfer could be reading.
        setStatusMessage(Constants::Status::RENDER_STATUS, L"Done");

        return true;
    }

    void RenderScene::afterCompute(const bool success, const uint64_t generation) {
        if (!success) {
            vkh::logger::log("Recompute cancelled.");
        }
        if (generation != computeGeneration.load()) {
            // Superseded: the compute that replaced this one owns idleCompute and will notify on its own.
            return;
        }
        if (success && attr.fractal.reuseReferenceMethod == FrtReuseReferenceMethod::CENTERED_REFERENCE) {
            attr.fractal.reuseReferenceMethod = FrtReuseReferenceMethod::CURRENT_REFERENCE;
        }
        if (success) completedComputeGeneration.store(generation);
        idleCompute = true;
        backgroundThreads.notifyAll();
    }


    void RenderScene::destroy() {
        state.cancel();
        if (boxZoomOverlay != nullptr) {
            DestroyWindow(boxZoomOverlay);
            boxZoomOverlay = nullptr;
        }
        destroyBoxZoomOverlayBitmap();
        engine.getCore().getLogicalDevice().waitDeviceIdle();
        renderer = nullptr;
    }
}
