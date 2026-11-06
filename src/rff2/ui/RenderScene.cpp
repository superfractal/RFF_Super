//
// Created by Merutilm on 2025-08-08.
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-05, 2026-08-06, 2026-08-07, 2026-08-08, 2026-08-10, 2026-08-12, 2026-08-13, 2026-08-14, 2026-08-15, 2026-08-17, 2026-08-19, 2026-08-23, 2026-08-24, 2026-08-26, 2026-08-27, 2026-08-31, 2026-09-01, 2026-09-03, 2026-09-04
// Modified by GPT-5 on 2026-08-21, 2026-08-23, 2026-08-27, 2026-08-31, 2026-09-01
// Modified by GPT-6 on 2026-09-08, 2026-09-10, 2026-09-11, 2026-09-13, 2026-09-14, 2026-09-15, 2026-09-16, 2026-09-17, 2026-09-18, 2026-09-20, 2026-09-21, 2026-09-23, 2026-09-24, 2026-09-25, 2026-09-26, 2026-09-27, 2026-09-30, 2026-10-01
// Modified by Opus 5.5 on 2026-09-23, 2026-09-30, 2026-10-04, 2026-10-06, 2026-10-07, 2026-10-08, 2026-10-09, 2026-10-10, 2026-10-12, 2026-10-13, 2026-10-16, 2026-10-21, 2026-10-24, 2026-10-25, 2026-10-26, 2026-11-01, 2026-11-02, 2026-11-05
//

#include "NativeDialogs.hpp"
#include "RenderScene.hpp"
#include "../attr/NumericSettingLimits.hpp"
#include "VideoPipelineWarmup.hpp"
#include "VideoRenderScene.hpp"
#include "SsaaResolve.hpp"

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
#include "../formula/BuddhabrotPerturbator.h"
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
#include "../io/ConfigIO.h"
#include "../io/MapLimits.hpp"
#include <sstream>


namespace merutilm::rff2 {
    namespace {
        void prepareVideoPipelines(vkh::EngineRef engine, const Attribute &source, bool hdr = false) {
            HWND window = CreateWindowExW(0, L"STATIC", L"", WS_POPUP, 0, 0, 64, 64,
                                          nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
            if (!window) {
                throw std::runtime_error("Failed to create video preparation window");
            }
            auto scope = std::make_unique<VideoPipelineWarmup::Window>(
                engine, window, hdr ? Constants::VulkanWindow::VIDEO_PREPARATION_WINDOW_ATTACHMENT_INDEX
                                    : Constants::VulkanWindow::VIDEO_WARMUP_WINDOW_ATTACHMENT_INDEX);
            auto attribute = source;
            attribute.render.ssaa = 1;
            attribute.video.data.sourceScale = 1;
            attribute.shader.slope.studio.use = false;
            attribute.shader.layerOrder.enabled = false;
            auto *context = engine.attachWindowContext(window, scope->index);
            scope->attached = true;
            attribute.shader.hdr.use = hdr;
            const std::function<void()> prepareNext = hdr ? std::function<void()>{}
                                                          : [&] { prepareVideoPipelines(engine, source, true); };
            auto scene = std::make_unique<VideoRenderScene>(engine, *context, VkExtent2D{32, 32}, attribute,
                                                            prepareNext);
            if (scene->isSlopeDeferred()) {
                VideoPipelineWarmup::hold(std::move(scope), std::move(scene));
            }
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
        vkh::PipelinePreparation::trace("scene init begins");
        refreshCanvasExtent();
        refreshSharedImgContext();
        attachRenderContext();
        initRenderer(true);
        refreshRenderContext();
        refreshResizeParams();
        applyShaderAttr(attr);
        wndRequestFPS();
        requests.requestRecompute();
        vkh::PipelinePreparation::trace("scene init ends");
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
        // A draft whose full pass never came would leave its part-way percentage standing for good.
        if (smoothZoomDraftPixels != 0) {
            smoothZoomDraftPixels = 0;
            if (idleCompute.load()) setStatusMessage(Constants::Status::RENDER_STATUS, L"Done");
        }
    }

    void RenderScene::restoreSmoothZoomSource() {
        if (!smoothZoomActive || !smoothZoomOriginal) return;
        // A draft is too coarse to settle on, so the view on screen is calculated in full instead.
        if (smoothZoomCaptureWider ? !smoothZoomCandidateFullQuality : smoothZoomSourceDraft) {
            if (effectiveProjection(attr.fractal.projectionMethod) == FrtProjectionMethod::PLANAR)
                attr.fractal = smoothNavigationCamera(smoothZoomView);
            requests.requestRecompute();
            return;
        }
        if (smoothZoomCaptureWider) return;
        state.cancel();
        pendingIterationLimit = 0;
        waitFramesInFlight();
        attr.fractal = *smoothZoomOriginal;
        lastMaxIteration = smoothZoomOriginalMax;
        lastPeriod = smoothZoomOriginalPeriod;
        lastLogZoom = smoothZoomOriginalLog;
        const auto* source = reinterpret_cast<const double*>(renderer->iterationStagingBufferContext->getContext().mappedMemory);
        for (uint32_t i = 0; i < iterationMatrix->getLength(); ++i) iterationMatrix->storeRelaxed(i, source[i]);
        // The angles staged with that picture are its own as well, so they come back with it.
        const auto* angles = reinterpret_cast<const float*>(renderer->escapeAngleStagingBufferContext->getContext().mappedMemory);
        for (uint32_t i = 0; i < escapeAngleMatrix->getLength(); ++i) escapeAngleMatrix->storeRelaxed(i, angles[i]);
        orbitTrapMatrix->copyFrom(reinterpret_cast<const float*>(renderer->orbitTrapStagingBufferContext->getContext().mappedMemory));
        renderer->rendererIteration->setMaxIteration(static_cast<double>(lastMaxIteration));
        previewUploadPending = false;
        idleCompute = true;
        ++previewRevision;
    }

    bool RenderScene::beginSmoothNavigation() {
        if (smoothZoomActive) return true;
        if (!smoothZoomEnabled || renderer->lastShadedFrame < 0 || computeHold || longJobBusy.load() ||
            isVideoGenerationActive || isVideoExportActive) return false;
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
        smoothZoomSourceDraft = false;
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
        // A 360 view has no flat camera to stop part-way on, so it settles on the zoom it was heading for.
        if (effectiveProjection(attr.fractal.projectionMethod) != FrtProjectionMethod::PLANAR) smoothZoomView = smoothZoomTarget;
        else if (smoothZoomView.scale != smoothZoomTarget.scale || smoothZoomView.x != smoothZoomTarget.x || smoothZoomView.y != smoothZoomTarget.y)
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
            smoothZoomSourceDraft = !smoothZoomCandidateFullQuality;
            if (!smoothZoomDirty && smoothZoomPending == 0) smoothZoomNeedsPreview = false;
        }

        if (smoothZoomPending != 0 && beginSmoothNavigation()) {
            const double steps = std::clamp(smoothZoomPending, -4.0, 4.0);
            smoothZoomPending -= steps;
            const float oldZoom = attr.fractal.logZoom;
            // ZOOM_DEADLINE selects deep perturbation; it is not a navigation limit.
            const float newZoom = std::max(Constants::Fractal::ZOOM_MIN,
                                          static_cast<float>(oldZoom + steps * Constants::Fractal::ZOOM_INTERVAL));
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

        // A drag takes each finished preview that still holds the view, instead of waiting at the edge until the pointer rests.
        if (smoothZoomGeneration != 0 && idleCompute.load() && (!smoothZoomDirty || smoothZoomDragging)) {
            if (completedComputeGeneration.load() != smoothZoomGeneration) {
                if (!smoothZoomDirty) {
                    smoothZoomGeneration = 0;
                    smoothZoomFailed = true;
                    previewUploadPending = false;
                    setStatusMessage(Constants::Status::RENDER_STATUS, L"Navigation stopped; move again to retry or press Escape");
                }
            } else if (SmoothZoomMotion::covered(SmoothZoomMotion::relative(smoothZoomView, smoothZoomCandidate))) {
                waitFramesInFlight();
                if (!smoothZoomCandidateFullQuality && smoothZoomPreviewMatrix) {
                    const auto pw = smoothZoomPreviewMatrix->getWidth(), ph = smoothZoomPreviewMatrix->getHeight();
                    const auto fw = iterationMatrix->getWidth(), fh = iterationMatrix->getHeight();
                    const bool angles = escapeAngleMatrix != nullptr && smoothZoomPreviewAngles != nullptr;
                    // A shape setting changed since the draft started leaves its traps a plane count apart, so none are shown.
                    const bool traps = orbitTrapMatrix != nullptr && smoothZoomPreviewTraps != nullptr &&
                        orbitTrapMatrix->getPlaneCount() == smoothZoomPreviewTraps->getPlaneCount();
                    if (!angles) clearEscapeAngles();
                    else if (!traps && orbitTrapMatrix != nullptr) orbitTrapMatrix->clear();
                    for (uint16_t y = 0; y < fh; ++y) {
                        const auto sy = SmoothZoomMotion::previewPixel(y, ph, fh);
                        for (uint16_t x = 0; x < fw; ++x) {
                            const auto sx = SmoothZoomMotion::previewPixel(x, pw, fw);
                            const auto i = smoothZoomPreviewMatrix->getIndex(sx, sy);
                            iterationMatrix->storeRelaxed(x, y, smoothZoomPreviewMatrix->loadRelaxed(i));
                            if (angles) escapeAngleMatrix->storeRelaxed(x, y, smoothZoomPreviewAngles->loadRelaxed(i));
                            if (traps) orbitTrapMatrix->storeRelaxed(x, y, smoothZoomPreviewTraps->loadRelaxed(i));
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
        // A drag lets its preview finish, since one restarted every 100 ms may never arrive while the pointer moves.
        if (smoothZoomDirty &&
            (idleCompute.load() || (!smoothZoomDragging && now - smoothZoomComputeAt >= std::chrono::milliseconds(100)))) {
            // A 360 view always calculates the target itself, since a wider flat frame is not a wider view of the sphere.
            smoothZoomExactCandidate = (SmoothZoomMotion::covered(smoothZoomTarget) && !smoothZoomDragging) ||
                effectiveProjection(attr.fractal.projectionMethod) != FrtProjectionMethod::PLANAR;
            smoothZoomCandidateFullQuality = smoothZoomExactCandidate && !smoothZoomNeedsPreview &&
                now - smoothZoomInputAt >= std::chrono::milliseconds(200);
            smoothZoomCandidate = smoothZoomExactCandidate ? smoothZoomTarget :
                SmoothZoomMotion::enclosing(smoothZoomView, smoothZoomTarget, smoothZoomDragging ? 2.0 : 1.2);
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

    void RenderScene::collectDeferredPipelines() {
        bool collected = renderer != nullptr && renderer->rendererSlope->collectDeferred();
        collected = VideoPipelineWarmup::poll() || collected;
        vkh::PipelinePreparation::pollDeferred();
        // Kept as soon as it exists, so a session that ends any other way does not compile it again.
        if (collected) {
            vkh::PipelinePreparation::trace("surface shader swapped in");
            wc.core.getLogicalDevice().savePipelineCache();
        }
    }

    void RenderScene::awaitDeferredSlope() const {
        if (renderer != nullptr) {
            renderer->rendererSlope->awaitDeferred();
        }
    }

    void RenderScene::render(const bool present) {
        if (static bool traced = false; !traced) {
            traced = true;
            vkh::PipelinePreparation::trace("first scene render");
        }
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
            // A shape the live planes were not measured with needs the orbits again, whichever control changed it.
            if (const TrapShapeParams shape = TrapShapeParams::of(attr.shader.palette);
                shape.enabled && shape != liveTrapShape) {
                requests.requestRecompute();
            }
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

        // A padded 360 view whose source still holds reprojects the camera move; one whose source is still computing waits for it.
        bool awaitingPanoramaSource = false;
        if (requests.recomputeRequested && !computeHold && panoramaPreviewApplies(attr)) {
            const auto key = panoramaSourceKeyOf(attr);
            if (idleCompute.load() && !panoramaSourceKey.empty() && key == panoramaSourceKey) {
                requests.recomputeRequested.exchange(false);
                reprojectPanoramaView();
            } else if (!idleCompute.load() && key == panoramaPendingKey &&
                       panoramaPendingGeneration == computeGeneration.load()) {
                awaitingPanoramaSource = true;
            }
        }

        if (requests.recomputeRequested && !computeHold && !awaitingPanoramaSource) {
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

        if (const auto limit = pendingIterationLimit.exchange(0); limit != 0) {
            renderer->rendererIteration->setMaxIteration(static_cast<double>(limit));
        }

        renderer->rendererSlope->setReliefZoom(attr.shader.slope,
            smoothZoomCaptureWider ? smoothZoomCandidateFractal->logZoom : attr.fractal.logZoom);
        renderer->rendererSlope->setReliefCap(attr.shader.slope.reliefCapIteration(lastLowestEscaped, lastPeriod));
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
        renderer->rendererPresent->setSsaaFilter(attr.render.ssaaFilter);
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
                r.clarityMultiplier = 1.0f;
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
                case WM_MOUSEMOVE:
                    if (idleCompute.load()) {
                        updateIterationStatus();
                    }
                    return;
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
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
                            POINT point{};
                            COLORREF picked = CLR_INVALID;
                            if (GetCursorPos(&point)) {
                                const HDC screen = GetDC(nullptr);
                                if (screen) {
                                    picked = GetPixel(screen, point.x, point.y);
                                    ReleaseDC(nullptr, screen);
                                }
                            }
                            pickedFreezeColors[it] = picked;
                            iters.push_back(it);
                            requests.requestShader();
                        }
                    }
                    if (onPicked) {
                        onPicked();
                    }
                    break;
                }
                if (isFocusPickActive()) {
                    const auto onPicked = std::move(focusPickCallback);
                    cancelFocusPick();
                    SetCursor(LoadCursor(nullptr, IDC_ARROW));
                    if (idleCompute.load() && renderer->iterationStagingBufferContext != nullptr) {
                        const double it = (*renderer->iterationStagingBufferContext)(getMouseXOnIterationBuffer(),
                                                                                     getMouseYOnIterationBuffer());
                        using namespace SharedDescriptorTemplate;
                        // The same maximum the fog pass divides by, so the band centres on exactly this count.
                        const double maximum = renderer->rendererIteration->getDescriptor(GPCIterationPalette::SET_ITERATION)
                            .get<vkh::Uniform>(0, DescIteration::BINDING_UBO_ITERATION_INFO)->getHostObject()
                            .get<double>(DescIteration::TARGET_UBO_ITERATION_MAX);
                        // The interior and unfinished pixels carry no count the band could sit on.
                        if (it > 0.0 && maximum > 0.0 && it < maximum) {
                            attr.shader.fog.focusRatio = static_cast<float>(std::clamp(it / maximum, 0.0, 1.0));
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
                smoothZoomDragging = smoothZoomActive &&
                    effectiveProjection(attr.fractal.projectionMethod) == FrtProjectionMethod::PLANAR;
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
                    if (effectiveProjection(attr.fractal.projectionMethod) == FrtProjectionMethod::PLANAR &&
                        beginSmoothNavigation()) {
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
                        // A drag follows the pointer one to one instead of easing after it, held at the picture's edge until a wider one is calculated.
                        if (const auto followed = SmoothZoomMotion::clampedInside(smoothZoomTarget); SmoothZoomMotion::covered(followed))
                            smoothZoomView = followed;
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
                                         std::format(L"I : {} ({}, {})", StatusText::grouped(it),
                                                     StatusText::grouped(x), StatusText::grouped(y)));
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
                if (smoothZoomEnabled && !computeHold && !longJobBusy.load() && !isVideoExportActive) {
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

        // Catmull-Rom weights for the four source pixels around fraction t (Catmull & Rom 1974; see NOTICE).
        std::array<double, 4> catmullRomWeights(const double t) {
            const double t2 = t * t, t3 = t2 * t;
            return {-0.5 * t3 + t2 - 0.5 * t, 1.5 * t3 - 2.5 * t2 + 1.0, -1.5 * t3 + 2.0 * t2 + 0.5 * t, 0.5 * t3 - 0.5 * t2};
        }

        // The source's value at (x0 + fx, y0 + fy) with a continuous slope, so Slope shows no source pixel grid where a 360
        // view enlarges it; -1 where any of the 16 pixels is off the source or not an escaping count. Held inside the middle
        // four pixels so the cubic never overshoots past a steep edge, as the keyframe relief does.
        double cubicSourceValue(const Matrix<double> &source, const uint32_t x0, const uint32_t y0, const double fx,
                                const double fy, const double maxIt) {
            const uint32_t sw = source.getWidth(), sh = source.getHeight();
            if (x0 < 1 || y0 < 1 || x0 + 2 >= sw || y0 + 2 >= sh) return -1.0;
            const auto wx = catmullRomWeights(fx), wy = catmullRomWeights(fy);
            double sum = 0.0, lo = maxIt, hi = 0.0;
            for (uint32_t j = 0; j < 4; ++j) {
                double row = 0.0;
                for (uint32_t i = 0; i < 4; ++i) {
                    const double v = source.loadRelaxed((y0 - 1 + j) * sw + x0 - 1 + i);
                    if (v <= 0.0 || v >= maxIt) return -1.0;
                    row += wx[i] * v;
                    if ((i == 1 || i == 2) && (j == 1 || j == 2)) {
                        lo = std::min(lo, v);
                        hi = std::max(hi, v);
                    }
                }
                sum += wy[j] * row;
            }
            return std::clamp(sum, lo, hi);
        }

        // The trap's distances at (x0 + fx, y0 + fy) with a continuous slope, as cubicSourceValue gives the count, so Orbit
        // Trap Relief and glow show no source pixel grid; iterations, angle and status keep the nearest pixel's, having no mean.
        void cubicTrapDistances(const OrbitTrapMatrix &traps, const uint32_t sw, const uint32_t sh, const uint32_t x0,
                                const uint32_t y0, const double fx, const double fy, OrbitTrap &trap) {
            if (x0 < 1 || y0 < 1 || x0 + 2 >= sw || y0 + 2 >= sh) return;
            std::array<OrbitTrap, 16> taps;
            for (uint32_t j = 0; j < 4; ++j)
                for (uint32_t i = 0; i < 4; ++i) taps[j * 4 + i] = traps.loadRelaxed((y0 - 1 + j) * sw + x0 - 1 + i);
            const auto wx = catmullRomWeights(fx), wy = catmullRomWeights(fy);
            const auto blend = [&](float OrbitTrap::*field, const bool shape) {
                double sum = 0.0, lo = OrbitTrap::FAR_LOG2, hi = OrbitTrap::HIT_LOG2;
                for (uint32_t j = 0; j < 4; ++j) {
                    for (uint32_t i = 0; i < 4; ++i) {
                        const auto &tap = taps[j * 4 + i];
                        const double v = tap.*field;
                        // A distance never sampled, or floored at an exact hit, is a stand-in with no slope to follow.
                        if (v >= OrbitTrap::FAR_LOG2 || v <= OrbitTrap::HIT_LOG2 * 0.5 ||
                            (shape && tap.shapeStatus != OrbitTrap::STATUS_VALID)) return;
                        sum += wx[i] * wy[j] * v;
                        if ((i == 1 || i == 2) && (j == 1 || j == 2)) {
                            lo = std::min(lo, v);
                            hi = std::max(hi, v);
                        }
                    }
                }
                trap.*field = static_cast<float>(std::clamp(sum, lo, hi));
            };
            blend(&OrbitTrap::crossLog2, false);
            blend(&OrbitTrap::pointLog2, false);
            blend(&OrbitTrap::secondLog2, false);
            if (traps.hasShapePlanes()) blend(&OrbitTrap::shapeLog2, true);
        }

        // The plane offset a canvas pixel lands on, in canvas pixels, for whichever 360 mode is on.
        ProjectedPoint projectedOffsetPixels(const FractalAttribute &calc, const double px, const double py,
                                             const double w, const double h, const double yaw,
                                             const double coverage = std::numeric_limits<double>::infinity()) {
            const double limit = std::min(panoramaLimit(calc, w, h), coverage);
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

    std::wstring RenderScene::dumpState() {
        if (backgroundThreads.runningCount() != 0 || longJobBusy.load()) {
            return L"Scene state is unavailable while a background operation is running. Try again after it finishes.";
        }
        // Join the compute worker before reading its perturbator, reference, and approximation cache.
        cancelRunningCompute();
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
        if (preservedFormulaType != FractalFormulaType::MANDELBROT) {
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
        pickedFreezeColors.clear();
        // The config restores the location, max-iteration and auto-iteration directly.
        if (attr.fractal.formulaType != FractalFormulaType::MANDELBROT) {
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
        fr.buddhabrotSamples = lf.buddhabrotSamples;
        fr.buddhabrotMinIteration = lf.buddhabrotMinIteration;
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
                awaitDeferredSlope();
                renderer->executeOffscreen();
                const auto frame = renderer->getFrameIndex();
                wc.getSyncObject().getFence(frame).wait();
                const auto &image = wc.getSharedImageContext().getImageContextMF(
                    SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_SECONDARY)[frame];
                auto pixels = readbackImageContextToMat(wc, image);
                cv::cvtColor(pixels, pixels, cv::COLOR_RGBA2BGRA);
                pixels = SsaaResolve::resize(pixels, cv::Size(getClientWidth(), getClientHeight()), attr.render.ssaaFilter);
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
        // A saved image always carries the full surface shader, never the stand-in a skip draws.
        awaitDeferredSlope();
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
            const cv::Mat resized = SsaaResolve::resize(
                img,
                // Clamped like the same divide in CPCImageRGBA2BGR and VideoWindow: a zero size made cv::resize throw.
                cv::Size(std::max(1, static_cast<int>(imgCtx.extent.width) / static_cast<int>(ssaa)),
                         std::max(1, static_cast<int>(imgCtx.extent.height) / static_cast<int>(ssaa))),
                attr.render.ssaaFilter);
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

        // The angles go across whole: a pixel the compute has not reached still holds 0, which draws no angle coloring.
        if (escapeAngleMatrix != nullptr && renderer->escapeAngleStagingBufferContext != nullptr &&
            escapeAngleMatrix->getLength() == renderer->escapeAngleStagingBufferContext->getLength()) {
            auto *angles = reinterpret_cast<float *>(renderer->escapeAngleStagingBufferContext->getContext().mappedMemory);
            for (uint32_t i = 0, length = escapeAngleMatrix->getLength(); i < length; ++i) {
                angles[i] = escapeAngleMatrix->loadRelaxed(i);
            }
        }
        // The traps go across whole as well; an unreached pixel holds iteration 0, which draws no trap coloring.
        if (orbitTrapMatrix != nullptr && renderer->orbitTrapStagingBufferContext != nullptr &&
            static_cast<uint64_t>(orbitTrapMatrix->getLength()) * orbitTrapMatrix->getPlaneCount() ==
            renderer->orbitTrapStagingBufferContext->getLength()) {
            orbitTrapMatrix->copyTo(reinterpret_cast<float *>(renderer->orbitTrapStagingBufferContext->getContext().mappedMemory));
        }

        // Read one element at a time rather than by block: the compute threads are still writing
        // this matrix, and only the relaxed accessor makes reading it as it fills defined.
        const Matrix<double> &src = *iterationMatrix;
        auto *dst = reinterpret_cast<double *>(staging.getContext().mappedMemory);

        if (complete) {
            // The map is final, so what is left of the picture the resize was seeded from goes with it.
            previewSeedGeneration = 0;
            const auto maximum = static_cast<double>(lastMaxIteration);
            double lowest = maximum;
            for (uint32_t i = 0, length = static_cast<uint32_t>(w) * h; i < length; ++i) {
                const double v = src.loadRelaxed(i);
                dst[i] = v;
                if (v > 0.0 && v < lowest) lowest = v;
            }
            lastLowestEscaped = lowest < maximum ? lowest : 0.0;
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
                    auto angleStaging = std::make_unique<GraphicsMatrixBuffer<float>>(
                        wc.core, width, height, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
                    angleStaging->fillZero();
                    const uint32_t trapPlanes = orbitTrapMatrix != nullptr ? orbitTrapMatrix->getPlaneCount()
                                                                           : orbitTrapPlaneCountOf(attr);
                    auto trapStaging = std::make_unique<GraphicsMatrixBuffer<float>>(
                        wc.core, width, static_cast<uint32_t>(height) * trapPlanes,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
                    trapStaging->fillZero();
                    if (iterationMatrix != nullptr && iterationMatrix->getWidth() == width &&
                        iterationMatrix->getHeight() == height) {
                        staging->fill(iterationMatrix->getCanvas());
                        if (escapeAngleMatrix != nullptr && escapeAngleMatrix->getLength() == angleStaging->getLength()) {
                            angleStaging->fill(escapeAngleMatrix->getCanvas());
                        }
                        if (orbitTrapMatrix != nullptr && orbitTrapMatrix->getLength() == iterationMatrix->getLength()) {
                            orbitTrapMatrix->copyTo(reinterpret_cast<float *>(trapStaging->getContext().mappedMemory));
                        }
                    }
                    renderer->iterationStagingBufferContext = std::move(staging);
                    renderer->escapeAngleStagingBufferContext = std::move(angleStaging);
                    renderer->orbitTrapStagingBufferContext = std::move(trapStaging);
                    renderer->rendererIteration->resetIterationBuffer(width, height, trapPlanes);
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
                auto angleStaging = std::move(renderer->escapeAngleStagingBufferContext);
                auto trapStaging = std::move(renderer->orbitTrapStagingBufferContext);
                // The old renderer cannot go while its surface shader is still compiling.
                awaitDeferredSlope();
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
                    renderer->escapeAngleStagingBufferContext = std::move(angleStaging);
                    renderer->orbitTrapStagingBufferContext = std::move(trapStaging);
                    const uint32_t iterationWidth = getIterationBufferWidth(attr);
                    const uint32_t iterationHeight = getIterationBufferHeight(attr);
                    // The kept staging buffer decides the plane count, since every frame copies it whole.
                    const uint64_t trapPixels = static_cast<uint64_t>(iterationWidth) * iterationHeight;
                    const auto trapPlanes = static_cast<uint32_t>(
                        trapPixels == 0 ? OrbitTrap::PLANE_COUNT
                                        : renderer->orbitTrapStagingBufferContext->getLength() / trapPixels);
                    renderer->rendererIteration->resetIterationBuffer(iterationWidth, iterationHeight, trapPlanes);
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
        renderer->rendererIteration->setPalette(attr.shader.palette, attr.shader.slope);
        renderer->rendererIteration->setTextures(attr.shader.textures, warpSourceLayer(attr.shader.warp),
                                                 environmentSourceLayerMask(attr.shader.slope.studioEnvironmentSource));
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
        renderer->rendererSlope->setReliefCap(attr.shader.slope.reliefCapIteration(lastLowestEscaped, lastPeriod));
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
        const uint32_t trapPlanes = orbitTrapPlaneCountOf(attr);
        renderer->rendererIteration->resetIterationBuffer(iw, ih, trapPlanes);
        iterationMatrix = std::make_unique<Matrix<double> >(iw, ih);
        renderer->iterationStagingBufferContext = std::make_unique<GraphicsMatrixBuffer<double> >(
            wc.core, iw, ih, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        // The picture a resize seeds carries no angles, so both start at 0.
        escapeAngleMatrix = std::make_unique<Matrix<float> >(iw, ih);
        renderer->escapeAngleStagingBufferContext = std::make_unique<GraphicsMatrixBuffer<float> >(
            wc.core, iw, ih, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        renderer->escapeAngleStagingBufferContext->fillZero();
        orbitTrapMatrix = std::make_unique<OrbitTrapMatrix>(iw, ih, trapPlanes);
        renderer->orbitTrapStagingBufferContext = std::make_unique<GraphicsMatrixBuffer<float> >(
            wc.core, iw, static_cast<uint32_t>(ih) * trapPlanes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        renderer->orbitTrapStagingBufferContext->fillZero();
    }

    uint32_t RenderScene::orbitTrapPlaneCountOf(const Attribute &settings) {
        return OrbitTrap::planeCountFor(TrapShapeParams::of(settings.shader.palette).enabled);
    }

    void RenderScene::matchOrbitTrapPlanes(const Attribute &settings) {
        const uint32_t planes = orbitTrapPlaneCountOf(settings);
        if (orbitTrapMatrix == nullptr || renderer == nullptr || renderer->orbitTrapStagingBufferContext == nullptr ||
            orbitTrapMatrix->getPlaneCount() == planes) {
            return;
        }
        // The new planes start empty: the compute about to run fills them, and an empty pixel draws as with the trap off.
        const uint16_t w = orbitTrapMatrix->getWidth();
        const uint16_t h = orbitTrapMatrix->getHeight();
        orbitTrapMatrix = std::make_unique<OrbitTrapMatrix>(w, h, planes);
        renderer->orbitTrapStagingBufferContext = std::make_unique<GraphicsMatrixBuffer<float> >(
            wc.core, w, static_cast<uint32_t>(h) * planes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        renderer->orbitTrapStagingBufferContext->fillZero();
        renderer->rendererIteration->resizeOrbitTrapBuffer(w, h, planes);
    }

    void RenderScene::initRenderer(bool prepareAll) {
        wc.core.getLogicalDevice().waitDeviceIdle();
        const std::function<void()> prepare = prepareAll ? std::function<void()>{[&] { prepareVideoPipelines(engine, attr); }} : std::function<void()>{};
        // Skip may leave the surface shader compiling; a stand-in draws until render() collects it.
        vkh::PipelinePreparation::DeferralScope deferral;
        vkh::PipelinePreparation::trace("renderer build begins");
        renderer = std::make_unique<RenderSceneRenderer>(engine, wc.getAttachmentIndex(), prepare);
        vkh::PipelinePreparation::trace("renderer build ends");
        // After a skip the surface shader is not in the cache yet; collectDeferredPipelines saves it once it is.
        if (!vkh::PipelinePreparation::deferredPending()) {
            wc.core.getLogicalDevice().savePipelineCache();
        }
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
        // A one-pixel canvas keeps every image the resize builds at clarity x SSAA pixels, which is what frees the VRAM.
        if (previewImagesReleased) {
            canvasExtent = VkExtent2D{1, 1};
            return;
        }
        canvasExtent = documentCanvasExtent.value_or(wc.getSwapchain().getCurrentExtent());
    }

    void RenderScene::setPreviewImagesReleased(const bool released) {
        if (released == previewImagesReleased || renderer == nullptr) {
            return;
        }
        if (released) {
            // Only a finished map is worth keeping aside; a compute still running is left to end first.
            if (!idleCompute || smoothZoomActive || iterationMatrix == nullptr ||
                requests.resizeRequested || requests.recomputeRequested) {
                return;
            }
            ++previewRevision;
            releasedMatrix = std::move(iterationMatrix);
            releasedEscapeAngleMatrix = std::move(escapeAngleMatrix);
            releasedOrbitTrapMatrix = std::move(orbitTrapMatrix);
            previewImagesReleased = true;
            applyResize();
            return;
        }
        ++previewRevision;
        previewImagesReleased = false;
        iterationMatrix.reset();
        applyResize();
        std::unique_ptr<Matrix<double> > kept = std::move(releasedMatrix);
        std::unique_ptr<Matrix<float> > keptAngles = std::move(releasedEscapeAngleMatrix);
        std::unique_ptr<OrbitTrapMatrix> keptTraps = std::move(releasedOrbitTrapMatrix);
        if (kept == nullptr) {
            requests.requestRecompute();
            return;
        }
        auto &staging = *renderer->iterationStagingBufferContext;
        if (kept->getWidth() == iterationMatrix->getWidth() && kept->getHeight() == iterationMatrix->getHeight() &&
            staging.getWidth() == kept->getWidth() && staging.getHeight() == kept->getHeight()) {
            staging.fill(kept->getCanvas());
            iterationMatrix = std::move(kept);
            if (keptAngles != nullptr && keptAngles->getLength() == escapeAngleMatrix->getLength()) {
                renderer->escapeAngleStagingBufferContext->fill(keptAngles->getCanvas());
                escapeAngleMatrix = std::move(keptAngles);
            }
            if (keptTraps != nullptr && keptTraps->getLength() == orbitTrapMatrix->getLength() &&
                keptTraps->getPlaneCount() == orbitTrapMatrix->getPlaneCount()) {
                keptTraps->copyTo(reinterpret_cast<float *>(renderer->orbitTrapStagingBufferContext->getContext().mappedMemory));
                orbitTrapMatrix = std::move(keptTraps);
            }
            previewUploadPending = false;
            previewSeedGeneration = 0;
            return;
        }
        // The window changed size during the export, so the kept map only seeds the view the recompute fills.
        seedPreviewFromMatrix(*kept);
        requests.requestRecompute();
    }

    void RenderScene::clearEscapeAngles() {
        if (escapeAngleMatrix != nullptr) {
            for (uint32_t i = 0, length = escapeAngleMatrix->getLength(); i < length; ++i) {
                escapeAngleMatrix->storeRelaxed(i, 0.0f);
            }
        }
        if (renderer != nullptr && renderer->escapeAngleStagingBufferContext != nullptr) {
            renderer->escapeAngleStagingBufferContext->fillZero();
        }
        if (orbitTrapMatrix != nullptr) {
            orbitTrapMatrix->clear();
        }
        if (renderer != nullptr && renderer->orbitTrapStagingBufferContext != nullptr) {
            renderer->orbitTrapStagingBufferContext->fillZero();
        }
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
        // The magnified picture keeps its iterations but not the angles or traps laid out for the old view.
        renderer->escapeAngleStagingBufferContext->fillZero();
        renderer->orbitTrapStagingBufferContext->fillZero();

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
        pendingIterationLimit = 0;
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
        // A map file stores iterations only, so the opened view has no escape angles.
        clearEscapeAngles();
        lastLogZoom = map.getLogZoom();
        lastPeriod = map.getPeriod();
        lastMaxIteration = map.getMaxIteration();
        lastLowestEscaped = map.getLowestEscapedIteration();
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
        return std::format(L"I : {}/{}", StatusText::grouped(index + 1), StatusText::grouped(browsedImages.size()));
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
        // Nothing reads the trap buffers while the device is idle and no compute runs, so the shape planes come and go here.
        matchOrbitTrapPlanes(settings);
        liveTrapShape = orbitTrapMatrix != nullptr && orbitTrapMatrix->hasShapePlanes()
                            ? TrapShapeParams::of(settings.shader.palette) : TrapShapeParams{};
        beforeCompute(settings);
        Attribute geometry = settings;
        ComputeProgress progress;
        const uint64_t fullPixels = static_cast<uint64_t>(getIterationBufferWidth(settings)) * getIterationBufferHeight(settings);
        smoothZoomPreviewMatrix.reset();
        smoothZoomPreviewAngles.reset();
        smoothZoomPreviewTraps.reset();
        if (lowResolution) {
            settings.render.ssaa = 1;
            settings.render.clarityMultiplier = SmoothZoomMotion::previewClarity(settings.render.clarityMultiplier,
                getClientWidth(), getClientHeight(), settings.video.data.sourceScale);
            if (getIterationBufferWidth(settings) == 0 || getIterationBufferHeight(settings) == 0)
                settings.render.clarityMultiplier = geometry.render.clarityMultiplier * geometry.render.ssaa;
            settings.render.coarsePreview = false;
            smoothZoomPreviewMatrix = std::make_unique<Matrix<double>>(getIterationBufferWidth(settings), getIterationBufferHeight(settings));
            // The draft carries its own angles and traps, so Escape Angle and Orbit Trap Coloring keep drawing while it is shown.
            smoothZoomPreviewAngles = std::make_unique<Matrix<float>>(smoothZoomPreviewMatrix->getWidth(), smoothZoomPreviewMatrix->getHeight());
            smoothZoomPreviewTraps = std::make_unique<OrbitTrapMatrix>(smoothZoomPreviewMatrix->getWidth(), smoothZoomPreviewMatrix->getHeight(),
                orbitTrapPlaneCountOf(settings));
            smoothZoomDraftPixels = smoothZoomPreviewMatrix->getLength();
            smoothZoomDraftTicks = 0;
            progress.total = smoothZoomDraftPixels + fullPixels;
            progress.announceDone = false;
        } else if (smoothZoomActive && smoothZoomDraftPixels != 0) {
            progress.before = smoothZoomDraftPixels;
            progress.total = smoothZoomDraftPixels + fullPixels;
            progress.start -= std::chrono::high_resolution_clock::duration(smoothZoomDraftTicks.load());
            smoothZoomDraftPixels = 0;
        } else smoothZoomDraftPixels = 0;
        auto* output = lowResolution ? smoothZoomPreviewMatrix.get() : iterationMatrix.get();
        auto* outputAngles = smoothZoomPreviewAngles.get();
        auto* outputTraps = smoothZoomPreviewTraps.get();
        Attribute source = settings;
        const bool panorama = !lowResolution && overrideSettings == nullptr && preparePanoramaSource(source, generation);
        if (panorama) {
            progress.total = static_cast<uint64_t>(getIterationBufferWidth(source)) * getIterationBufferHeight(source);
        } else {
            releasePanoramaSource();
        }
        state.createThread([this, generation, output, outputAngles, outputTraps, progress, panorama, source = std::move(source), geometry = std::move(geometry), settings = std::move(settings)](const std::stop_token &) {
            try {
                const bool success = panorama ? computePanoramaSource(source, settings, &progress)
                                              : compute(settings, output, &geometry, &progress, outputAngles, outputTraps);
                afterCompute(success, generation);
            } catch (const std::bad_alloc &) {
                afterCompute(false, generation);
                if (generation == computeGeneration.load()) {
                    reportComputeFailure(L"There is not enough memory. Lower Clarity, Supersampling or the resolution.");
                }
            } catch (const std::exception &error) {
                afterCompute(false, generation);
                try {
                    vkh::logger::log_err_silent("Recompute failed: {}", error.what());
                    if (generation == computeGeneration.load()) {
                        const std::string what = error.what();
                        reportComputeFailure(std::wstring(what.begin(), what.end()));
                    }
                } catch (...) {
                }
            } catch (...) {
                afterCompute(false, generation);
                try {
                    vkh::logger::log_err_silent("Recompute failed with an unknown exception");
                    if (generation == computeGeneration.load()) {
                        reportComputeFailure(L"");
                    }
                } catch (...) {
                }
            }
        });
    }

    void RenderScene::beforeCompute(Attribute &attr) const {
        pendingIterationLimit = 0;
        attr.fractal.maxIteration = attr.fractal.autoMaxIteration
                                        ? NumericSettingLimits::automaticIterationLimit(lastPeriod, attr.fractal.autoIterationMultiplier)
                                        : this->attr.fractal.maxIteration;
        renderer->rendererIteration->setMaxIteration(static_cast<double>(attr.fractal.maxIteration));
    }

    bool RenderScene::buildPerturbator(Attribute &attr, const dex &dcMax,
                                       std::chrono::high_resolution_clock::time_point start) {
        auto &calc = attr.fractal;
        const float logZoom = calc.logZoom;
        const auto refreshInterval = Utilities::getRefreshInterval(logZoom);
        std::function actionPerRefCalcIteration = [refreshInterval, this, &start](const uint64_t p) {
            if (p % refreshInterval == 0) {
                setStatusMessage(Constants::Status::RENDER_STATUS, std::format(L"P : {}", StatusText::grouped(p)));
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
        } else if (calc.formulaType == FractalFormulaType::BUDDHABROT) {
            currentPerturbator = std::make_unique<BuddhabrotPerturbator>(state, calc, static_cast<double>(dcMax));
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

        if (calc.autoMaxIteration && calc.formulaType == FractalFormulaType::MANDELBROT) {
            calc.maxIteration = NumericSettingLimits::automaticIterationLimit(
                reference->longestPeriod(), calc.autoIterationMultiplier);
            if (auto* p = dynamic_cast<LightMandelbrotPerturbator*>(currentPerturbator.get())) {
                currentPerturbator = p->reuse(calc, static_cast<double>(p->getDcMaxAsDoubleExp()), approxTableCache);
            } else if (auto* p = dynamic_cast<DeepMandelbrotPerturbator*>(currentPerturbator.get())) {
                currentPerturbator = p->reuse(calc, p->getDcMaxAsDoubleExp(), approxTableCache);
            }
            pendingIterationLimit = calc.maxIteration;
        }

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
                         std::format(L"P : {} ({}, {})", StatusText::grouped(lastPeriod),
                                     StatusText::grouped(refLength), StatusText::grouped(mpaLen)));
        if (state.interruptRequested()) return false;
        return true;
    }

    GpuLightPerturbator *RenderScene::gpuPerturbatorOrNull() {
        if (gpuPerturbator != nullptr || gpuPerturbatorUnavailable) return gpuPerturbator.get();
        try {
            if (!GpuLightPerturbator::isSupported(wc.core)) {
                gpuPerturbatorUnavailable = true;
                vkh::logger::log_err_silent("GPU perturbation needs 64-bit shader integers; calculating on the CPU.");
                postNotice(L"GPU Perturbation is not available on this GPU, so RFF_Super calculates on the CPU.",
                           L"This GPU has no 64-bit shader integers.");
                return nullptr;
            }
            gpuPerturbator = std::make_unique<GpuLightPerturbator>(wc.core);
        } catch (const std::exception &error) {
            gpuPerturbatorUnavailable = true;
            vkh::logger::log_err_silent("GPU perturbation is unavailable: {}", error.what());
            const std::string what = error.what();
            postNotice(L"GPU Perturbation is not available on this GPU, so RFF_Super calculates on the CPU.",
                       std::wstring(what.begin(), what.end()));
        }
        return gpuPerturbator.get();
    }

    std::vector<double> RenderScene::gpuPixelOffsets(const Attribute &attr, const LightMandelbrotPerturbator &perturbator,
                                                     const PixelCoordinate &coordinate, const uint16_t w,
                                                     const uint16_t h) const {
        // The CPU iterate() adds the reuse offset in double after narrowing dc, so the same is done here.
        const auto [offR, offI] = perturbator.getOffset();
        std::vector<double> dc(static_cast<size_t>(w) * h * 2);
        const uint32_t workerCount = std::max<uint32_t>(1u, attr.render.threads);
        std::atomic<uint32_t> nextRow = 0;
        std::vector<std::jthread> pool;
        pool.reserve(workerCount);
        for (uint32_t t = 0; t < workerCount; ++t) {
            pool.emplace_back([&] {
                for (uint32_t y = nextRow.fetch_add(1); y < h; y = nextRow.fetch_add(1)) {
                    if (state.interruptRequested()) return;
                    for (uint16_t x = 0; x < w; ++x) {
                        bool sky = false;
                        const auto c = coordinate(x, static_cast<uint16_t>(y), &sky);
                        const size_t i = (static_cast<size_t>(y) * w + x) * 2;
                        dc[i] = sky ? std::numeric_limits<double>::quiet_NaN() : static_cast<double>(c[0]) + offR;
                        dc[i + 1] = sky ? 0.0 : static_cast<double>(c[1]) + offI;
                    }
                }
            });
        }
        // Joins the workers before dc is handed back.
        pool.clear();
        return dc;
    }

    bool RenderScene::computeOnGpu(const Attribute &attr, const PixelCoordinate &coordinate, const uint16_t w,
                                   const uint16_t h, Matrix<double> &matrix, Matrix<float> *escapeAngles,
                                   OrbitTrapMatrix *traps, std::atomic<int> &renderPixelsCount) {
        if (!attr.render.computeShader.use) return false;
        const auto *light = dynamic_cast<const LightMandelbrotPerturbator *>(currentPerturbator.get());
        if (light == nullptr || !GpuLightPerturbator::accepts(*light)) return false;
        GpuLightPerturbator *gpu = gpuPerturbatorOrNull();
        if (gpu == nullptr) return false;

        const std::vector<double> dc = gpuPixelOffsets(attr, *light, coordinate, w, h);
        // An interrupted compute is still this path's to finish; compute() sees the interruption and stops.
        if (state.interruptRequested()) return true;

        gpuComputing = true;
        try {
            gpu->render(state, *light, attr.fractal, attr.render.computeShader, w, h, dc, matrix, escapeAngles, traps,
                        renderPixelsCount, {});
            gpuComputing = false;
            return true;
        } catch (const std::exception &error) {
            gpuComputing = false;
            vkh::logger::log_err_silent("GPU perturbation failed, calculating on the CPU: {}", error.what());
            const std::string what = error.what();
            postNotice(L"GPU Perturbation stopped on an error, so this view is calculated on the CPU.",
                       std::wstring(what.begin(), what.end()));
            // The CPU path starts from the same blank matrix the GPU started from.
            renderPixelsCount = 0;
            for (uint32_t i = 0, length = matrix.getLength(); i < length; ++i) {
                matrix.storeRelaxed(i, 0);
            }
            if (escapeAngles != nullptr) {
                for (uint32_t i = 0, length = escapeAngles->getLength(); i < length; ++i) {
                    escapeAngles->storeRelaxed(i, 0.0f);
                }
            }
            if (traps != nullptr) traps->clear();
            return false;
        }
    }

    bool RenderScene::computeBuddhabrot(const Attribute &geometry, const uint16_t w, const uint16_t h,
                                        const double fullW, const double fullH, Matrix<double> &matrix,
                                        std::atomic<int> &renderPixelsCount) {
        const auto *perturbator = dynamic_cast<const BuddhabrotPerturbator *>(currentPerturbator.get());
        if (perturbator == nullptr) return false;
        Attribute planar = geometry;
        planar.fractal.projectionMethod = FrtProjectionMethod::PLANAR;
        // A draft pixel x samples the full grid at (x + 0.5) * fullW / w - 0.5, so the map stays affine.
        const double scaleX = fullW / w, scaleY = fullH / h;
        const double firstX = 0.5 * scaleX - 0.5, firstY = 0.5 * scaleY - 0.5;
        const double spanX = std::max(1, w - 1), spanY = std::max(1, h - 1);
        const auto at = [this, &planar](const double x, const double y) {
            const auto offset = offsetConversion(planar, x, y);
            return std::array{static_cast<double>(offset[0]), static_cast<double>(offset[1])};
        };
        // Corners are sampled because the nudge off the reference only moves points next to the center.
        const auto origin = at(firstX, firstY);
        const auto right = at(firstX + scaleX * spanX, firstY);
        const auto down = at(firstX, firstY + scaleY * spanY);
        const BuddhabrotPlane plane{
            origin[0], origin[1],
            (right[0] - origin[0]) / spanX, (right[1] - origin[1]) / spanX,
            (down[0] - origin[0]) / spanY, (down[1] - origin[1]) / spanY
        };
        return perturbator->render(plane, matrix, geometry.render.threads, renderPixelsCount);
    }

    bool RenderScene::compute(const Attribute &requestedAttr, Matrix<double>* output, const Attribute* samplingGeometry,
                              const ComputeProgress* requestedProgress, Matrix<float>* angleOutput,
                              OrbitTrapMatrix* trapOutput) {
        Attribute attr = requestedAttr;
        auto& matrix = output ? *output : *iterationMatrix;
        // Angles are kept for the live view only; a low-resolution draft has nowhere to put them.
        Matrix<float>* escapeAngles = angleOutput != nullptr ? angleOutput
                                      : &matrix == iterationMatrix.get() ? escapeAngleMatrix.get() : nullptr;
        // Traps are kept for the live view only, as the angles are; the 360 source and drafts carry none.
        OrbitTrapMatrix* traps = angleOutput == nullptr && &matrix == iterationMatrix.get() &&
                                 orbitTrapMatrix != nullptr && orbitTrapMatrix->getLength() == matrix.getLength()
                                     ? orbitTrapMatrix.get() : nullptr;
        // A caller handing its own trap matrix (the 360 source) gets the traps there instead.
        if (trapOutput != nullptr && trapOutput->getLength() == matrix.getLength()) traps = trapOutput;
        const auto& geometry = samplingGeometry ? *samplingGeometry : attr;
        const ComputeProgress progress = requestedProgress ? *requestedProgress : ComputeProgress{};
        auto start = progress.start;
        const uint16_t w = getIterationBufferWidth(attr);
        const uint16_t h = getIterationBufferHeight(attr);
        uint32_t len = uint32_t(w) * h;
        const uint64_t progressTotal = progress.total != 0 ? progress.total : len;
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
        // The shape is measured only where there are planes to keep it.
        currentPerturbator->setTrapShape(traps != nullptr && traps->hasShapePlanes()
                                             ? TrapShapeParams::of(attr.shader.palette) : TrapShapeParams{});


        std::atomic renderPixelsCount = 0;

        // Zeroed here instead of in the staging buffer: a zero is what marks a pixel this compute
        // has not reached, and the snapshot on the render thread reads that mark.
        for (uint32_t i = 0, matrixLength = matrix.getLength(); i < matrixLength; ++i) {
            matrix.storeRelaxed(i, 0);
        }
        if (escapeAngles != nullptr) {
            for (uint32_t i = 0, length = escapeAngles->getLength(); i < length; ++i) {
                escapeAngles->storeRelaxed(i, 0.0f);
            }
        }
        if (traps != nullptr) traps->clear();

        auto statusThread = std::jthread([&renderPixelsCount, &progress, progressTotal, this, &start](const std::stop_token &stop) {
            while (!stop.stop_requested()) {
                float ratio = static_cast<float>(progress.before + renderPixelsCount.load()) / static_cast<float>(progressTotal) * 100;
                setStatusMessage(Constants::Status::TIME_STATUS, Utilities::elapsed_time(start));
                setStatusMessage(Constants::Status::RENDER_STATUS,
                                 std::format(L"{} : {:.3f}%", gpuComputing.load() ? L"G" : L"C", ratio));

                Sleep(Constants::Status::SET_PROCESS_INTERVAL_MS);
            }
        });

        // Boundary trace is only safe when "did not escape" is encoded as the
        // canonical maxIteration return value, i.e. the non-absolute mode.
        // Fixed-tile boundary fill relates to Mariani-Silver, without recursive subdivision; see NOTICE; project license unchanged.
        const bool useBoundaryTrace = attr.render.boundaryTraceFill && !attr.fractal.absoluteIterationMode;
        // White-tile fill assumes a binary in/out classification (no gradient
        // detail in the exterior), so it is gated on the 2-color preview mode.
        const bool useWhiteFill = useBoundaryTrace && attr.render.preview2Color;
        const bool buddhabrot = attr.fractal.formulaType == FractalFormulaType::BUDDHABROT;
        // Each Buddhabrot pass writes the whole map, so its preview is shown as it stands.
        previewFillDown = !useBoundaryTrace && !buddhabrot;

        if (buddhabrot) {
            computeBuddhabrot(geometry, w, h, fullW, fullH, matrix, renderPixelsCount);
        } else if (computeOnGpu(attr, coordinate, w, h, matrix, escapeAngles, traps, renderPixelsCount)) {
            // The GPU filled the matrix, or was interrupted while doing so.
        } else if (useBoundaryTrace) {
            const uint16_t tileSize = std::clamp<uint16_t>(
                static_cast<uint16_t>(std::min(w, h) / 8), 8, 32);
            constexpr double whiteFillValue = 1.0;
            const auto maxItValue = static_cast<double>(attr.fractal.maxIteration);
            const uint16_t tilesX = (w + tileSize - 1) / tileSize;
            const uint16_t tilesY = (h + tileSize - 1) / tileSize;
            const uint32_t numTiles = static_cast<uint32_t>(tilesX) * tilesY;

            std::atomic<uint32_t> tileIndex = 0;
            const auto computePixel = [this, &coordinate, &matrix, escapeAngles, traps, &renderPixelsCount, useWhiteFill,
                                       maxItValue](const uint16_t x, const uint16_t y) {
                bool sky = false;
                const auto dc = coordinate(x, y, &sky);
                float angle = 0.0f;
                OrbitTrap trap{0.0f, 0.0f, 0.0f, 0.0f};
                const double it = sky ? maxItValue : currentPerturbator->iterate(dc[0], dc[1], &angle, traps != nullptr ? &trap : nullptr);
                const double stored = (useWhiteFill && it != maxItValue) ? whiteFillValue : it;
                matrix.storeRelaxed(x, y, stored);
                if (escapeAngles != nullptr) escapeAngles->storeRelaxed(x, y, angle);
                if (traps != nullptr) traps->storeRelaxed(x, y, trap);
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
                        float angle = 0.0f;
                        OrbitTrap trap{0.0f, 0.0f, 0.0f, 0.0f};
                        const double it = sky ? maxItValue : currentPerturbator->iterate(dc[0], dc[1], &angle, traps != nullptr ? &trap : nullptr);
                        const double stored = (useWhiteFill && it != maxItValue) ? whiteFillValue : it;

                        const uint16_t bx1 = std::min<uint16_t>(sx + coarseStep, w);
                        const uint16_t by1 = std::min<uint16_t>(sy + coarseStep, h);
                        for (uint16_t y = sy; y < by1; ++y) {
                            for (uint16_t x = sx; x < bx1; ++x) {
                                matrix.storeRelaxed(x, y, stored);
                                if (escapeAngles != nullptr) escapeAngles->storeRelaxed(x, y, angle);
                                if (traps != nullptr) traps->storeRelaxed(x, y, trap);
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
                [attr, this, &coordinate, escapeAngles, traps, &renderPixelsCount](const uint16_t x, const uint16_t y, uint16_t,
                                                 uint16_t, float, float, uint32_t, double) {
                    bool sky = false;
                    const auto dc = coordinate(x, y, &sky);
                    float angle = 0.0f;
                    OrbitTrap trap{0.0f, 0.0f, 0.0f, 0.0f};
                    const double iteration = sky
                                                 ? static_cast<double>(attr.fractal.maxIteration)
                                                 : currentPerturbator->iterate(dc[0], dc[1], &angle, traps != nullptr ? &trap : nullptr);
                    if (escapeAngles != nullptr) escapeAngles->storeRelaxed(x, y, angle);
                    if (traps != nullptr) traps->storeRelaxed(x, y, trap);
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
        if (progress.announceDone) {
            setStatusMessage(Constants::Status::RENDER_STATUS, L"Done");
        } else {
            smoothZoomDraftTicks = (std::chrono::high_resolution_clock::now() - start).count();
            setStatusMessage(Constants::Status::TIME_STATUS, Utilities::elapsed_time(start));
            setStatusMessage(Constants::Status::RENDER_STATUS, std::format(L"C : {:.3f}%",
                static_cast<float>(progress.before + len) / static_cast<float>(progressTotal) * 100));
        }

        return true;
    }

    void RenderScene::updateIterationStatus() const {
        if (!iterationMatrix || !iterationMatrix->getWidth() || !iterationMatrix->getHeight()) {
            return;
        }
        const auto x = std::min<uint16_t>(getMouseXOnIterationBuffer(), iterationMatrix->getWidth() - 1);
        const auto y = std::min<uint16_t>(getMouseYOnIterationBuffer(), iterationMatrix->getHeight() - 1);
        const auto value = iterationMatrix->loadRelaxed(uint32_t(y) * iterationMatrix->getWidth() + x);
        if (std::isfinite(value) && value >= 0 && value < std::ldexp(1.0, 64)) {
            setStatusMessage(Constants::Status::ITERATION_STATUS,
                             std::format(L"I : {} ({}, {})", StatusText::grouped(static_cast<uint64_t>(value)),
                                         StatusText::grouped(x), StatusText::grouped(y)));
        }
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
        if (success) {
            if (isVideoGenerationActive) {
                updateIterationStatus();
            }
            completedComputeGeneration.store(generation);
        }
        idleCompute = true;
        backgroundThreads.notifyAll();
    }


    void RenderScene::postNotice(std::wstring headline, std::wstring detail) {
        std::scoped_lock lock(noticeMutex);
        if (!postedNotices.insert(headline + L"\n" + detail).second) {
            return;
        }
        pendingNotices.emplace_back(std::move(headline), std::move(detail));
    }

    void RenderScene::showPendingNotice() {
        // A notice opened over another dialog, or over a job pumping messages, would stack on it.
        if (NativeDialogs::isOpen() || isLongJobBusy()) {
            return;
        }
        std::pair<std::wstring, std::wstring> notice;
        {
            std::scoped_lock lock(noticeMutex);
            if (pendingNotices.empty()) {
                return;
            }
            notice = std::move(pendingNotices.front());
            pendingNotices.erase(pendingNotices.begin());
        }
        std::wstring text = UiLanguage::text(notice.first);
        if (!notice.second.empty()) {
            text.append(L"\n\n").append(UiLanguage::text(notice.second));
        }
        NativeDialogs::message(nullptr, text.c_str(), L"RFF_Super", MB_OK | MB_ICONWARNING);
    }

    void RenderScene::reportComputeFailure(std::wstring detail) {
        setStatusMessage(Constants::Status::RENDER_STATUS, UiLanguage::text(L"Calculation failed"));
        postNotice(L"The view could not be calculated. The picture on screen is the last one that finished.",
                   std::move(detail));
    }

    uint32_t RenderScene::panoramaSourceScale(const Attribute &settings) const {
        const uint32_t padding = std::clamp<uint32_t>(settings.video.data.cameraScale, 2, 64);
        if (effectiveProjection(settings.fractal.projectionMethod) != FrtProjectionMethod::PERSPECTIVE_360) return padding;
        const double w = getIterationBufferWidth(settings), h = getIterationBufferHeight(settings);
        if (w <= 0.0 || h <= 0.0) return padding;
        // The source reaches at least the corners of the view looking straight down, which a tall canvas pushes past the padding.
        FractalAttribute nadir = settings.fractal;
        nadir.panoramaPitch = -90.0f;
        const auto corner = projectedOffsetPixels(nadir, -0.5, -0.5, w, h, 0.0);
        const double needed = std::ceil((2.0 * std::hypot(corner.x, corner.y) + 4.0) / std::min(w, h) - 1e-9);
        return std::max<uint32_t>(padding, static_cast<uint32_t>(std::min(needed, 65.0)));
    }

    bool RenderScene::panoramaPreviewApplies(const Attribute &settings) const {
        // Video work reads the exact map of the view it is given, so it never gets the reprojected one.
        // Past four times the view each way the source costs far more than the calculations it saves, so the view is calculated directly.
        return settings.video.data.cameraPadding && settings.video.data.sourceScale == 1 && settings.video.data.cameraScale <= 4 &&
               panoramaSourceScale(settings) <= 4 &&
               effectiveProjection(settings.fractal.projectionMethod) != FrtProjectionMethod::PLANAR &&
               !isVideoGenerationActive.load() && !isVideoExportActive.load();
    }

    std::string RenderScene::panoramaSourceKeyOf(const Attribute &settings) const {
        // Everything but the camera, the shader and the video settings other than the padding scale decides the planar source.
        Attribute key = settings;
        key.fractal.projectionMethod = FrtProjectionMethod::PLANAR;
        key.fractal.rotation = 0.0f;
        key.fractal.panoramaPitch = 0.0f;
        key.fractal.panoramaFov = 0.0f;
        key.fractal.panoramaRange = 0.0f;
        key.fractal.panoramaLayout = FrtPanoramaLayout::GROUND;
        key.fractal.reuseReferenceMethod = FrtReuseReferenceMethod::DISABLED;
        key.shader = {};
        // The shape trap is computed into the source, so its settings decide the source too.
        if (const auto &palette = settings.shader.palette; TrapShapeParams::of(palette).enabled) {
            key.shader.palette.orbitTrapColoring = palette.orbitTrapColoring;
            key.shader.palette.orbitTrapShape = palette.orbitTrapShape;
            key.shader.palette.orbitTrapCenterReal = palette.orbitTrapCenterReal;
            key.shader.palette.orbitTrapCenterImag = palette.orbitTrapCenterImag;
            key.shader.palette.orbitTrapRadius = palette.orbitTrapRadius;
            key.shader.palette.orbitTrapAngle = palette.orbitTrapAngle;
        }
        key.video = {};
        key.video.data.cameraPadding = true;
        key.video.data.cameraScale = panoramaSourceScale(settings);
        std::ostringstream out(std::ios::binary);
        ConfigIO::write(out, key, getClientWidth(), getClientHeight());
        return out.str();
    }

    bool RenderScene::preparePanoramaSource(Attribute &source, const uint64_t generation) {
        if (!panoramaPreviewApplies(source)) return false;
        const uint32_t scale = panoramaSourceScale(source);
        const uint32_t w = getIterationBufferWidth(source), h = getIterationBufferHeight(source);
        source.fractal.projectionMethod = FrtProjectionMethod::PLANAR;
        source.fractal.rotation = 0.0f;
        source.video.data.sourceScale = scale;
        const uint32_t sw = getIterationBufferWidth(source), sh = getIterationBufferHeight(source);
        // An axis clamped to the buffer range would cover less than the scale promises, so the view is computed directly instead.
        if (sw != w * scale || sh != h * scale || !MapLimits::valid(sw, sh)) {
            vkh::logger::log("360 preview padding exceeds the map limits; computing the view directly.");
            return false;
        }
        if (!panoramaSource || panoramaSource->getWidth() != sw || panoramaSource->getHeight() != sh) {
            panoramaSource.reset();
            panoramaSourceAngles.reset();
            panoramaSourceTraps.reset();
            panoramaSource = std::make_unique<Matrix<double>>(static_cast<uint16_t>(sw), static_cast<uint16_t>(sh));
            panoramaSourceAngles = std::make_unique<Matrix<float>>(static_cast<uint16_t>(sw), static_cast<uint16_t>(sh));
            panoramaSourceTraps.reset();
        }
        if (!panoramaSourceTraps || panoramaSourceTraps->getPlaneCount() != orbitTrapPlaneCountOf(source)) {
            panoramaSourceTraps = std::make_unique<OrbitTrapMatrix>(static_cast<uint16_t>(sw), static_cast<uint16_t>(sh),
                                                                    orbitTrapPlaneCountOf(source));
        }
        panoramaSourceKey.clear();
        panoramaPendingKey = panoramaSourceKeyOf(attr);
        panoramaPendingGeneration = generation;
        // The view keeps its last picture until the reprojection replaces it whole.
        previewSeedGeneration = generation;
        return true;
    }

    bool RenderScene::computePanoramaSource(const Attribute &source, const Attribute &settings,
                                            const ComputeProgress *progress) {
        if (!compute(source, panoramaSource.get(), &source, progress, panoramaSourceAngles.get(),
                     panoramaSourceTraps.get())) return false;
        // Auto Max Iteration settles the limit inside the compute, so the source's own setting can be a stale guess.
        panoramaSourceMax = static_cast<double>(lastMaxIteration);
        reprojectPanoramaSource(settings);
        panoramaSourceKey = panoramaPendingKey;
        return true;
    }

    void RenderScene::reprojectPanoramaSource(const Attribute &settings) {
        const auto &source = *panoramaSource;
        const auto &sourceAngles = *panoramaSourceAngles;
        auto &matrix = *iterationMatrix;
        const uint16_t w = matrix.getWidth(), h = matrix.getHeight();
        if (w != getIterationBufferWidth(settings) || h != getIterationBufferHeight(settings)) return;
        const uint32_t sw = source.getWidth(), sh = source.getHeight();
        const double radians = static_cast<double>(settings.fractal.rotation) * std::numbers::pi / 180.0;
        // Held two pixels inside the source edge, in horizon radii, so every bilinear tap is a computed pixel.
        const double coverage = std::max(0.0, (std::min(sw, sh) * 0.5 - 2.0) / (h * 0.5));
        const double maxIt = panoramaSourceMax;
        Matrix<float> *angles = escapeAngleMatrix && escapeAngleMatrix->getLength() == matrix.getLength()
                                    ? escapeAngleMatrix.get() : nullptr;
        const OrbitTrapMatrix *sourceTraps = panoramaSourceTraps && panoramaSourceTraps->getLength() == source.getLength()
                                                 ? panoramaSourceTraps.get() : nullptr;
        OrbitTrapMatrix *traps = orbitTrapMatrix && orbitTrapMatrix->getLength() == matrix.getLength()
                                     ? orbitTrapMatrix.get() : nullptr;
        std::atomic<uint32_t> nextRow = 0;
        const auto worker = [&] {
            for (uint32_t y; (y = nextRow.fetch_add(1)) < h;) {
                for (uint16_t x = 0; x < w; ++x) {
                    const auto p = projectedOffsetPixels(settings.fractal, x, y, w, h, radians, coverage);
                    double value = maxIt;
                    float angle = 0.0f;
                    OrbitTrap trap{0.0f, 0.0f, 0.0f, 0.0f};
                    if (!p.sky) {
                        // The planar source puts offset 0 at its middle, as offsetConversion does.
                        const double sx = std::clamp(p.x + sw / 2.0, 0.0, sw - 1.0);
                        const double sy = std::clamp(p.y + sh / 2.0, 0.0, sh - 1.0);
                        const uint32_t x0 = std::min(static_cast<uint32_t>(sx), sw - 2);
                        const uint32_t y0 = std::min(static_cast<uint32_t>(sy), sh - 2);
                        const double fx = sx - x0, fy = sy - y0;
                        const uint32_t taps[4] = {y0 * sw + x0, y0 * sw + x0 + 1, (y0 + 1) * sw + x0, (y0 + 1) * sw + x0 + 1};
                        const double weights[4] = {(1 - fx) * (1 - fy), fx * (1 - fy), (1 - fx) * fy, fx * fy};
                        // Interior taps carry no escape count to blend, so the pixel follows whichever side covers most of it, as the video camera does.
                        double exterior = 0.0, sum = 0.0;
                        // The heaviest tap on each side; angles and traps have no mean worth taking, so the pixel copies that tap's.
                        int nearestExterior = -1, nearestInterior = -1;
                        for (int i = 0; i < 4; ++i) {
                            const double v = source.loadRelaxed(taps[i]);
                            if (v <= 0.0) continue;
                            int &nearest = v >= maxIt ? nearestInterior : nearestExterior;
                            if (nearest < 0 || weights[i] > weights[nearest]) nearest = i;
                            if (v >= maxIt) continue;
                            exterior += weights[i];
                            sum += weights[i] * v;
                        }
                        const int followed = exterior >= 0.5 ? nearestExterior : nearestInterior;
                        if (exterior >= 0.5) {
                            const double cubic = cubicSourceValue(source, x0, y0, fx, fy, maxIt);
                            value = cubic > 0.0 ? cubic : sum / exterior;
                            angle = sourceAngles.loadRelaxed(taps[followed]);
                        }
                        if (sourceTraps != nullptr && followed >= 0) {
                            trap = sourceTraps->loadRelaxed(taps[followed]);
                            cubicTrapDistances(*sourceTraps, sw, sh, x0, y0, fx, fy, trap);
                        }
                    }
                    matrix.storeRelaxed(x, static_cast<uint16_t>(y), value);
                    if (angles != nullptr) angles->storeRelaxed(x, static_cast<uint16_t>(y), angle);
                    if (traps != nullptr) traps->storeRelaxed(x, static_cast<uint16_t>(y), trap);
                }
            }
        };
        std::vector<std::jthread> pool;
        const uint32_t workerCount = std::max<uint32_t>(1u, settings.render.threads);
        pool.reserve(workerCount);
        for (uint32_t t = 0; t < workerCount; ++t) pool.emplace_back(worker);
    }

    void RenderScene::reprojectPanoramaView() {
        state.cancel();
        endMapBrowse();
        endImageBrowse();
        const uint64_t generation = ++computeGeneration;
        ++previewRevision;
        reprojectPanoramaSource(attr);
        previewSeedGeneration = 0;
        previewUploadPending = true;
        completedComputeGeneration.store(generation);
        idleCompute = true;
        writeRecoverySnapshot(false);
        backgroundThreads.notifyAll();
    }

    void RenderScene::releasePanoramaSource() {
        panoramaSource.reset();
        panoramaSourceAngles.reset();
        panoramaSourceTraps.reset();
        panoramaSourceKey.clear();
        panoramaPendingKey.clear();
        panoramaPendingGeneration = 0;
    }


    void RenderScene::destroy() {
        state.cancel();
        if (boxZoomOverlay != nullptr) {
            DestroyWindow(boxZoomOverlay);
            boxZoomOverlay = nullptr;
        }
        destroyBoxZoomOverlayBitmap();
        // Compiles a skip left running still read the warm-up scenes and this renderer's render passes.
        // The main window is already gone, so they finish unseen; the cache saved at device teardown
        // then holds the surface shader, and the next start needs no preparation for it.
        vkh::PipelinePreparation::quietWaits = true;
        VideoPipelineWarmup::drain();
        awaitDeferredSlope();
        vkh::PipelinePreparation::awaitAll();
        engine.getCore().getLogicalDevice().waitDeviceIdle();
        gpuPerturbator = nullptr;
        renderer = nullptr;
    }
}
