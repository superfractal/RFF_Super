//
// Created by Merutilm on 2025-08-08.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21, 2026-08-23, 2026-08-27, 2026-08-31, 2026-09-01
// Modified by Opus 5 on 2026-08-05, 2026-08-06, 2026-08-07, 2026-08-08, 2026-08-10, 2026-08-12, 2026-08-13, 2026-08-14, 2026-08-15, 2026-08-17, 2026-08-19, 2026-08-23, 2026-08-24, 2026-08-26, 2026-08-27, 2026-08-31, 2026-09-01
//

#include "RenderScene.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cwctype>

#include "CallbackExplore.hpp"
#include "IOUtilities.h"
#include "../../vulkan_helper/executor/RenderPassFullscreenRecorder.hpp"
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
        initRenderer();
        refreshRenderContext();
        refreshResizeParams();
        applyShaderAttr(attr);
        wndRequestFPS();
        requests.requestRecompute();
    }


    void RenderScene::attachRenderContext() const {
        const auto swapchainImageContextGetter = [this] {
            auto &swapchain = wc.getSwapchain();
            return vkh::ImageContext::fromSwapchain(wc.core, swapchain);
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
        refreshRenderContext();
        const auto [presentWidth, presentHeight] = getSwapchainRenderContextExtent();
        renderer->rendererPresent->setRescaledResolution({presentWidth, presentHeight});
    }


    void RenderScene::render(const bool present) {
        if (requests.defaultAttrRequested) {
            applyDefaultAttr();
            requests.defaultAttrRequested.exchange(false);
            backgroundThreads.notifyAll();
        }
        if (requests.shaderRequested) {
            applyShaderAttr(attr);
            // The shader is where a change lands that never recomputes, so the snapshot has to
            // follow it here as well as at a compute - throttled, because a dragged slider asks
            // for this every frame.
            writeRecoverySnapshot(false);
            requests.shaderRequested.exchange(false);
            backgroundThreads.notifyAll();
        }

        if (requests.resizeRequested) {
            state.cancel();
            applyResize();
            requests.resizeRequested.exchange(false);
            backgroundThreads.notifyAll();
        }

        if (requests.recomputeRequested && !computeHold) {
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
        if (idleCompute.load()) {
            if (previewUploadPending.exchange(false)) {
                snapshotComputePreview(true);
            }
        } else {
            snapshotComputePreview(false);
        }

        if (idleCompute.load()) {
            if (auto imageRequest = requests.takeCreateImageRequest()) {
                applyCreateImage(std::move(*imageRequest));
                requests.completeCreateImageRequest();
                backgroundThreads.notifyAll();
            }
        }

        if (requests.exportHighResRequested) {
            applyExportHighRes(requests.exportTilesX, requests.exportTilesY);
            requests.exportHighResRequested.exchange(false);
            backgroundThreads.notifyAll();
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
                .bailout = 1000000,
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
                    if (onPicked) onPicked();
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
                if (wparam == MK_LBUTTON && interactedMX > 0 && interactedMY > 0) {
                    SetCursor(LoadCursor(nullptr, IDC_SIZEALL));
                    const auto dx = static_cast<int16_t>(interactedMX - x);
                    const auto dy = static_cast<int16_t>(interactedMY - y);
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
            case WM_MOUSEWHEEL: {
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

    std::array<dex, 2> RenderScene::offsetConversion(const Attribute &settings, const int mx, const int my,
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

    std::array<dex, 2> RenderScene::offsetConversionTiled(const Attribute &settings, const int fx, const int fy,
                                                          const int fullW, const int fullH,
                                                          const int scale, bool *sky) const {
        using namespace Constants::Fractal;
        // Full-grid pixel offset scaled by clarity*ssaa*scale, uniform on both axes.
        const double ox = static_cast<double>(fx) - static_cast<double>(fullW) / 2.0;
        const double oy = static_cast<double>(fy) - static_cast<double>(fullH) / 2.0;

        const double radians = static_cast<double>(settings.fractal.rotation) * std::numbers::pi / 180.0;
        const double s = std::sin(radians);
        const double c = std::cos(radians);

        // A 360 view spans the whole exported grid, so it is read off the full-grid pixel rather than off this tile's own.
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
            const auto o = offsetConversionTiled(settings, 0, 0, fullW, fullH, scale);
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
        const auto o = offsetConversionTiled(settings, bestX, bestY, fullW, fullH, scale);
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
        return static_cast<uint16_t>(static_cast<float>(getClientWidth()) * multiplier);
    }

    uint16_t RenderScene::getIterationBufferHeight(const Attribute &settings) const {
        const float multiplier = settings.render.clarityMultiplier * static_cast<float>(settings.render.ssaa);
        return static_cast<uint16_t>(static_cast<float>(getClientHeight()) * multiplier);
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
        // Internal render extent is client * clarity * ssaa; the larger client axis is what hits maxImageDimension2D first.
        const auto [width, height] = canvasExtent;
        const uint32_t largest = std::max(width, height);
        if (largest == 0) return 0.0f;
        const auto &limits = wc.core.getPhysicalDevice().getPhysicalDeviceProperties().limits;
        const float dimScale = static_cast<float>(limits.maxImageDimension2D) / static_cast<float>(largest);
        // The iteration matrix is one storage buffer of width * height doubles, so its own limit is on area.
        const double px = static_cast<double>(width) * static_cast<double>(height) * sizeof(double);
        if (px <= 0.0) return dimScale;
        const auto bufferScale = static_cast<float>(
            std::sqrt(static_cast<double>(limits.maxStorageBufferRange) / px));
        return std::min(dimScale, bufferScale);
    }

    std::wstring RenderScene::checkRenderMemoryBudget(const Attribute &settings) const {
        // Internal render extent = client * clarity * ssaa (matches getInternalImageExtent / iteration buffer).
        const double multiplier = static_cast<double>(settings.render.clarityMultiplier) *
                                  static_cast<double>(settings.render.ssaa);
        const double iw = static_cast<double>(getClientWidth()) * multiplier;
        const double ih = static_cast<double>(getClientHeight()) * multiplier;
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

    int RenderScene::exportTileMargin(const Attribute &settings, const uint32_t tilesX,
                                      const uint32_t tilesY) const {
        if (tilesX <= 1 && tilesY <= 1) {
            return 0;
        }
        const int tw = getIterationBufferWidth(settings);
        const int th = getIterationBufferHeight(settings);
        const auto &sl = settings.shader.slope;
        const bool slopeOn = sl.depth != 0.0f && sl.opacity > 0.0f;
        // Mirrors the macro radius the slope shader derives, whose multiplier is buffer width over 1280.
        const float multiplier = static_cast<float>(tw) / 1280.0f;
        const int macro = slopeOn && sl.macroRelief > 0.0f
                              ? std::max(static_cast<int>(sl.macroRadius * multiplier + 0.5f), 1)
                              : 0;
        // The fog pass still runs on the tile, and its focus band blurs the tile's own pixels with a
        // radius measured against the finished image. A tap reaching past the overlap reads this tile's
        // own edge instead of the neighbour's pixels and prints a seam along every tile boundary, so the
        // overlap has to cover the radius the band can ask for. The rim band is not counted: the tiles
        // render with the haze, and with it the band, deferred to the stitched image. The image is at
        // most tw * tilesX across - the overlap sized here only makes it smaller - so the estimate errs
        // on the wide side.
        const auto &fog = settings.shader.fog;
        int focus = 0;
        if (fog.focusAmount > 0.0f && fog.focusBlur > 0.0f) {
            const float canvasMultiplier = static_cast<float>(tw * static_cast<int>(tilesX)) / 1280.0f;
            const float requested = fog.focusBlur * canvasMultiplier;
            const float radius = fog.blurQuality == ShdFogBlurQuality::APPEARANCE
                                     ? requested
                                     : std::min(requested, 16.0f);
            focus = static_cast<int>(std::ceil(radius));
        }
        // +1 covers the 3x3 Sobel and the interpolation tent; the cap keeps a usable tile regardless.
        return std::min(macro + focus + 1, std::min(tw, th) / 4);
    }

    std::wstring RenderScene::checkExportMemoryBudget(const Attribute &settings, const uint32_t tilesX,
                                                      const uint32_t tilesY) const {
        const uint32_t gx = std::max<uint32_t>(tilesX, 1);
        const uint32_t gy = std::max<uint32_t>(tilesY, 1);
        const int margin = exportTileMargin(settings, gx, gy);
        const double tw = static_cast<double>(getIterationBufferWidth(settings) - 2 * margin);
        const double th = static_cast<double>(getIterationBufferHeight(settings) - 2 * margin);
        const double fullW = tw * static_cast<double>(gx);
        const double fullH = th * static_cast<double>(gy);
        const double fullPx = fullW * fullH;

        // The full image is held as one CV_16UC4 buffer (8 B/px); with SSAA>1 a downsampled copy
        // (8 B/px / ssaa^2) briefly coexists. ~20% margin for cvtColor/imwrite scratch.
        const double ssaa = std::max<double>(1.0, static_cast<double>(settings.render.ssaa));
        const double ramBytes = (fullPx * 8.0 + fullPx * 8.0 / (ssaa * ssaa)) * 1.2;

        // Tiles render one at a time at the internal extent, so per-tile VRAM matches the live view.
        std::wstring msg = checkRenderMemoryBudget(settings);

        if (const uint64_t ramTotal = getSystemMemoryTotal();
            ramTotal > 0 && ramBytes > 0.8 * static_cast<double>(ramTotal)) {
            msg += std::format(L"- RAM: the {}x{} image needs about {}, but the system only has {}.\n",
                               static_cast<uint64_t>(fullW / ssaa), static_cast<uint64_t>(fullH / ssaa),
                               formatBytes(static_cast<uint64_t>(ramBytes)), formatBytes(ramTotal));
        }
        return msg;
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
        RecoveryIO::writeSnapshot(attr, getClientWidth(), getClientHeight());
    }


    namespace {
        // Copies an internal render image (R16G16B16A16) to a host-visible buffer and returns it as a
        // cloned RGBA cv::Mat. The caller must have waited the render fence before calling.
        template<typename ImageCtx>
        cv::Mat readbackImageContextToMat(vkh::WindowContextRef wc, const ImageCtx &imgCtx) {
            vkh::BufferContext bufCtx = vkh::BufferContext::createContext(wc.core, {
                .size = imgCtx.capacity,
                .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            });
            vkh::BufferContext::mapMemory(wc.core, bufCtx); {
                const auto executor = vkh::ScopedNewCommandBufferExecutor(wc.core, wc.getCommandPool());
                vkh::BarrierUtils::cmdImageMemoryBarrier(executor.getCommandBufferHandle(), imgCtx.image,
                                                         VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 1,
                                                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                                         VK_PIPELINE_STAGE_TRANSFER_BIT);
                vkh::BufferImageContextUtils::cmdCopyImageToBuffer(executor.getCommandBufferHandle(), imgCtx, bufCtx);
            }
            cv::Mat view(static_cast<int>(imgCtx.extent.height), static_cast<int>(imgCtx.extent.width), CV_16UC4,
                         bufCtx.mappedMemory);
            cv::Mat result = view.clone();
            vkh::BufferContext::unmapMemory(wc.core, bufCtx);
            vkh::BufferContext::destroyContext(wc.core, bufCtx);
            return result;
        }

        // One box blur matching vk_box_blur.comp: a row span, then a column span over its result.
        void separableBoxBlur(cv::Mat &img, const int radius) {
            if (radius <= 0) {
                return;
            }
            cv::Mat mid = img.clone();
            cv::parallel_for_(cv::Range(0, img.rows), [&](const cv::Range &rows) {
                for (int y = rows.start; y < rows.end; ++y) {
                    const auto *srcRow = img.ptr<cv::Vec3f>(y);
                    auto *dstRow = mid.ptr<cv::Vec3f>(y);
                    for (int x = 0; x < img.cols; ++x) {
                        cv::Vec3f sum(0, 0, 0);
                        int count = 0;
                        for (int i = std::max(0, x - radius); i <= std::min(img.cols - 1, x + radius); ++i) {
                            sum += srcRow[i];
                            ++count;
                        }
                        dstRow[x] = sum / static_cast<float>(count);
                    }
                }
            });
            cv::parallel_for_(cv::Range(0, img.rows), [&](const cv::Range &rows) {
                for (int y = rows.start; y < rows.end; ++y) {
                    auto *dstRow = img.ptr<cv::Vec3f>(y);
                    for (int x = 0; x < img.cols; ++x) {
                        cv::Vec3f sum(0, 0, 0);
                        int count = 0;
                        for (int i = std::max(0, y - radius); i <= std::min(img.rows - 1, y + radius); ++i) {
                            sum += mid.at<cv::Vec3f>(i, x);
                            ++count;
                        }
                        dstRow[x] = sum / static_cast<float>(count);
                    }
                }
            });
        }

        float smoothStep(const float edge0, const float edge1, const float x) {
            const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
            return t * t * (3.0f - 2.0f * t);
        }

        // Full-image fog for the tiled export, mirroring vk_fog.frag. The GPU pass blurs the whole canvas
        // and measures the falloff from the frame centre; a tile knows neither, so the tiles are rendered
        // without fog and the haze is laid over the stitched image here instead.
        void applyFogToStitched(cv::Mat &rgba16, const ShdFogAttribute &fog) {
            const int fullW = rgba16.cols;
            const int fullH = rgba16.rows;

            cv::Mat rgb;
            cv::cvtColor(rgba16, rgb, cv::COLOR_RGBA2RGB);
            rgb.convertTo(rgb, CV_32FC3, 1.0 / 65535.0);

            // The GPU blurs a copy capped to GAUSSIAN_MAX_WIDTH wide, so the same cap sets the radius here.
            int bw = fullW;
            int bh = fullH;
            if (const double rat = static_cast<double>(Constants::Fractal::GAUSSIAN_MAX_WIDTH) / fullW; rat < 1.0) {
                bw = Constants::Fractal::GAUSSIAN_MAX_WIDTH;
                bh = std::max(1, static_cast<int>(static_cast<double>(fullH) * rat));
            }
            cv::Mat small;
            cv::resize(rgb, small, cv::Size(bw, bh), 0, 0, cv::INTER_AREA);
            const int radius = static_cast<int>(static_cast<float>(bh) * fog.radius);
            for (int pass = 0; pass < 3; ++pass) {
                separableBoxBlur(small, radius);
            }
            cv::Mat blurred;
            cv::resize(small, blurred, cv::Size(fullW, fullH), 0, 0, cv::INTER_LINEAR);

            const float opacity = fog.opacity;
            const float centerStart = fog.centerStart;
            const bool invert = fog.centerInvert;
            const auto gray = [](const cv::Vec3f &c) { return c[0] * 0.3f + c[1] * 0.59f + c[2] * 0.11f; };

            cv::parallel_for_(cv::Range(0, fullH), [&](const cv::Range &rows) {
                for (int y = rows.start; y < rows.end; ++y) {
                    auto *dst = rgba16.ptr<cv::Vec4w>(y);
                    const auto *srcRow = rgb.ptr<cv::Vec3f>(y);
                    const auto *blurRow = blurred.ptr<cv::Vec3f>(y);
                    // The shader's coord is the fragment over the canvas size, which is the full image here.
                    const float ny = (static_cast<float>(y) + 0.5f) / static_cast<float>(fullH);
                    for (int x = 0; x < fullW; ++x) {
                        const float nx = (static_cast<float>(x) + 0.5f) / static_cast<float>(fullW);
                        float amount = opacity;
                        if (centerStart > 0.0f) {
                            const float dist = std::min(std::hypot((nx - 0.5f) * 2.0f, (ny - 0.5f) * 2.0f), 1.0f);
                            const float ramp = smoothStep(std::min(centerStart, 0.99f), 1.0f, dist);
                            amount *= invert ? 1.0f - ramp : ramp;
                        }
                        const cv::Vec3f color = srcRow[x];
                        const cv::Vec3f cf = color - (color - blurRow[x]) * amount;
                        // Away from a masked band the fog stays the brighten-only haze it has always been.
                        const cv::Vec3f lifted = gray(color) < gray(cf) ? cf : color;
                        for (int c = 0; c < 3; ++c) {
                            dst[x][c] = static_cast<uint16_t>(std::lround(
                                std::clamp(lifted[c], 0.0f, 1.0f) * 65535.0f));
                        }
                    }
                }
            });
        }

        // The GPU runs this after the fog pass (RCC5), so deferring the fog has to defer it too or the
        // two swap places. Weights match vk_linear_interpolation.frag; its fetch clamps, so does this.
        void applyLinearInterpolationTent(cv::Mat &rgba16) {
            const cv::Mat kernel = (cv::Mat_<float>(3, 3) << 1, 3, 1, 3, 9, 3, 1, 3, 1) / 25.0f;
            cv::filter2D(rgba16, rgba16, -1, kernel, cv::Point(-1, -1), 0, cv::BORDER_REPLICATE);
        }
    }

    void RenderScene::applyCreateImage(RenderSceneRequests::CreateImageRequest request) {
        // The dialog runs before the fence wait so cancelling costs nothing, and returns nullptr when closed.
        if (request.filename.empty()) {
            const auto path = IOUtilities::ioFileDialog(L"Save image", Constants::Extension::DESC_IMAGE,
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
                       cv::Size(static_cast<int>(imgCtx.extent.width) / static_cast<int>(ssaa),
                                static_cast<int>(imgCtx.extent.height) / static_cast<int>(ssaa)),
                       0, 0, cv::INTER_AREA);
            saved = IOUtilities::writeImage(request.filename, resized);
        } else {
            saved = IOUtilities::writeImage(request.filename, img);
        }
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

    void RenderScene::applyExportHighRes(const uint32_t tilesX, const uint32_t tilesY) {
        using namespace SharedImageContextIndices;
        const auto path = IOUtilities::ioFileDialog(L"Save image", Constants::Extension::DESC_IMAGE,
                                                    IOUtilities::SAVE_FILE, Constants::Extension::IMAGE);
        if (path == nullptr) {
            return;
        }
        const std::filesystem::path filename = *path;

        // Stop any in-flight compute and install a fresh stop token (cancel() alone leaves the token
        // in the requested state, which would make the export loop below look interrupted).
        state.createThread([](const std::stop_token &) {});
        wc.core.getLogicalDevice().waitDeviceIdle();

        Attribute settings = attr;
        beforeCompute(settings);

        // Fog reads the whole canvas (a full-frame blur) and the frame centre, neither of which a single
        // tile carries. The tiles render without it, and without the linear-interpolation tent the GPU
        // applies after it, so both can be laid over the stitched image below in the same order.
        const ShdFogAttribute exportFog = settings.shader.fog;
        const bool deferFog = exportFog.opacity > 0.0f;
        const bool deferLinearInterpolation = deferFog && settings.render.linearInterpolation;

        // One frame is submitted per tile, and the palette's phases are read from the clock on every
        // frame, so without pinning the animation each tile would be coloured a moment further along.
        renderer->rendererIteration->pinAnimationTime(true);
        // Puts everything the export borrows back on every exit, including the interrupted ones.
        struct ExportStateRestore {
            const RenderScene *scene;
            glm::uvec2 liveExtent;
            bool restoreShader;

            ~ExportStateRestore() {
                scene->renderer->rendererIteration->pinAnimationTime(false);
                scene->renderer->rendererIteration->setCanvasGeometry(liveExtent, {0, 0});
                if (restoreShader) {
                    scene->applyShaderAttr(scene->attr);
                }
            }
        } exportRestore{
            this,
            {getIterationBufferWidth(settings), getIterationBufferHeight(settings)},
            deferFog
        };

        if (deferFog) {
            Attribute tileSettings = settings;
            tileSettings.shader.fog.opacity = 0.0f;
            tileSettings.render.linearInterpolation = false;
            applyShaderAttr(tileSettings);
        }

        const uint16_t tw = getIterationBufferWidth(settings);
        const uint16_t th = getIterationBufferHeight(settings);
        const uint32_t gx = std::max<uint32_t>(tilesX, 1);
        const uint32_t gy = std::max<uint32_t>(tilesY, 1);
        const int scale = static_cast<int>(std::min(gx, gy));

        const int margin = exportTileMargin(settings, gx, gy);
        // Kept region of one tile, and therefore the tile pitch across the full grid.
        const int ow = static_cast<int>(tw) - 2 * margin;
        const int oh = static_cast<int>(th) - 2 * margin;
        const int fullW = ow * static_cast<int>(gx);
        const int fullH = oh * static_cast<int>(gy);

        // Boundary trace: skip computing the interior of uniform sub-tiles (same gating/skip logic as
        // compute()), so the export honors the optimization instead of computing every pixel.
        const bool useBoundaryTrace = settings.render.boundaryTraceFill && !settings.fractal.absoluteIterationMode;
        // 2-Color preview: escaped pixels collapse to a single fill value, matching the live view's white-fill.
        const bool useWhiteFill = useBoundaryTrace && settings.render.preview2Color;
        const double maxItValue = static_cast<double>(settings.fractal.maxIteration);
        constexpr double whiteFillValue = 1.0;

        // One reference for the whole image: dcMax from the full-grid corner (largest |dc|).
        auto start = std::chrono::high_resolution_clock::now();
        const dex dcMax = dcMaxOf(settings, fullW, fullH, scale);

        if (!buildPerturbator(settings, dcMax, start)) {
            requests.requestRecompute();
            return;
        }

        cv::Mat big(fullH, fullW, CV_16UC4);
        const uint32_t totalTiles = gx * gy;

        // The export owns the UI thread until it finishes, so the pump below is the only thing that
        // keeps the window painting and answering; longJobBusy tells the handlers it dispatches to
        // leave the scene alone. Cleared on every exit path, including the interrupted ones.
        longJobBusy.store(true);
        struct LongJobScope {
            std::atomic<bool> &flag;
            ~LongJobScope() { flag.store(false); }
        } longJobScope{longJobBusy};

        for (uint32_t tj = 0; tj < gy; ++tj) {
            for (uint32_t ti = 0; ti < gx; ++ti) {
                if (state.interruptRequested()) {
                    requests.requestRecompute();
                    return;
                }

                // Full-grid coordinate of this tile's buffer origin, pulled back by the overlap.
                const int x0 = static_cast<int>(ti) * ow - margin;
                const int y0 = static_cast<int>(tj) * oh - margin;

                // The screen-space animation fields and decor UVs need where the tile sits in the final
                // image, which is flipped against the buffer (image top = buffer bottom).
                renderer->rendererIteration->setCanvasGeometry(
                    {static_cast<uint32_t>(fullW), static_cast<uint32_t>(fullH)},
                    {x0, fullH - y0 - static_cast<int>(th)});

                // Fill the internal-sized iteration buffer with this tile's full-grid pixels.
                if (useBoundaryTrace) {
                    // computePixel stores the (possibly white-filled) value but returns raw iteration for classification.
                    const auto computeBufferPixel = [this, &settings, x0, y0, fullW, fullH, scale,
                                                     useWhiteFill, maxItValue](
                                                        const uint16_t x, const uint16_t y) -> double {
                        bool sky = false;
                        const auto dc = offsetConversionTiled(settings, x0 + static_cast<int>(x),
                                                              y0 + static_cast<int>(y), fullW, fullH, scale, &sky);
                        const double it = sky ? maxItValue : currentPerturbator->iterate(dc[0], dc[1]);
                        const double stored = (useWhiteFill && it != maxItValue) ? whiteFillValue : it;
                        (*iterationMatrix)(x, y) = stored;
                        renderer->iterationStagingBufferContext->set(x, y, stored);
                        return it;
                    };
                    const auto fillBufferPixel = [this](const uint16_t x, const uint16_t y, const double v) {
                        (*iterationMatrix)(x, y) = v;
                        renderer->iterationStagingBufferContext->set(x, y, v);
                    };

                    // Sub-tile this internal buffer; trace each perimeter and flood-fill uniform interiors.
                    const uint16_t tileSize = std::clamp<uint16_t>(
                        static_cast<uint16_t>(std::min(tw, th) / 8), 8, 32);
                    const uint16_t subTilesX = (tw + tileSize - 1) / tileSize;
                    const uint16_t subTilesY = (th + tileSize - 1) / tileSize;
                    const uint32_t numSubTiles = static_cast<uint32_t>(subTilesX) * subTilesY;
                    std::atomic<uint32_t> subTileIndex = 0;

                    auto worker = [&] {
                        while (true) {
                            const uint32_t st = subTileIndex.fetch_add(1);
                            if (st >= numSubTiles) return;
                            if (state.interruptRequested()) return;

                            const uint16_t stx = static_cast<uint16_t>(st % subTilesX);
                            const uint16_t sty = static_cast<uint16_t>(st / subTilesX);
                            const uint16_t bx0 = stx * tileSize;
                            const uint16_t by0 = sty * tileSize;
                            const uint16_t bx1 = std::min<uint16_t>(bx0 + tileSize, tw);
                            const uint16_t by1 = std::min<uint16_t>(by0 + tileSize, th);

                            bool allBlack = true;
                            bool allWhite = true;
                            const auto classify = [&](const double it) {
                                if (it == maxItValue) allWhite = false; else allBlack = false;
                            };

                            for (uint16_t x = bx0; x < bx1; ++x) {
                                classify(computeBufferPixel(x, by0));
                                if (by1 > by0 + 1) {
                                    classify(computeBufferPixel(x, static_cast<uint16_t>(by1 - 1)));
                                }
                            }
                            for (uint16_t y = static_cast<uint16_t>(by0 + 1); y + 1 < by1; ++y) {
                                classify(computeBufferPixel(bx0, y));
                                if (bx1 > bx0 + 1) {
                                    classify(computeBufferPixel(static_cast<uint16_t>(bx1 - 1), y));
                                }
                            }

                            if (state.interruptRequested()) return;

                            if (allBlack) {
                                for (uint16_t y = static_cast<uint16_t>(by0 + 1); y + 1 < by1; ++y)
                                    for (uint16_t x = static_cast<uint16_t>(bx0 + 1); x + 1 < bx1; ++x)
                                        fillBufferPixel(x, y, maxItValue);
                            } else if (useWhiteFill && allWhite) {
                                for (uint16_t y = static_cast<uint16_t>(by0 + 1); y + 1 < by1; ++y)
                                    for (uint16_t x = static_cast<uint16_t>(bx0 + 1); x + 1 < bx1; ++x)
                                        fillBufferPixel(x, y, whiteFillValue);
                            } else {
                                for (uint16_t y = static_cast<uint16_t>(by0 + 1); y + 1 < by1; ++y) {
                                    for (uint16_t x = static_cast<uint16_t>(bx0 + 1); x + 1 < bx1; ++x)
                                        computeBufferPixel(x, y);
                                    if (state.interruptRequested()) return;
                                }
                            }
                        }
                    };

                    const uint32_t workerCount = std::max<uint32_t>(1u, settings.render.threads);
                    std::vector<std::jthread> pool;
                    pool.reserve(workerCount);
                    for (uint32_t t = 0; t < workerCount; ++t) pool.emplace_back(worker);
                    for (auto &t : pool) if (t.joinable()) t.join();
                } else {
                    auto dispatcher = ParallelArrayDispatcher<double>(
                        state, *iterationMatrix, settings.render.threads,
                        [this, &settings, x0, y0, fullW, fullH, scale, useWhiteFill, maxItValue](
                            const uint16_t x, const uint16_t y, uint16_t, uint16_t, float, float, uint32_t, double) {
                            bool sky = false;
                            const auto dc = offsetConversionTiled(settings, x0 + static_cast<int>(x),
                                                                  y0 + static_cast<int>(y), fullW, fullH, scale, &sky);
                            const double iteration = sky ? maxItValue : currentPerturbator->iterate(dc[0], dc[1]);
                            const double stored = (useWhiteFill && iteration != maxItValue) ? whiteFillValue : iteration;
                            renderer->iterationStagingBufferContext->set(x, y, stored);
                            return stored;
                        });
                    dispatcher.dispatch();
                }

                // Offscreen: the tile never reaches the swapchain, so no per-tile present stalls on a
                // vblank and a minimized window can no longer make the render silently do nothing.
                renderer->executeOffscreen();

                const uint32_t fi = renderer->getFrameIndex();
                wc.getSyncObject().getFence(fi).wait();
                const auto &imgCtx = wc.getSharedImageContext().getImageContextMF(
                    MF_MAIN_RENDER_IMAGE_SECONDARY)[fi];
                cv::Mat tile = readbackImageContextToMat(wc, imgCtx);
                // Keep only the inner region: the margin ring exists so these pixels had real
                // neighbours, but it duplicates the adjacent tiles and is thrown away. The image is
                // vertically flipped against the buffer, so the kept rows count from the bottom.
                tile(cv::Rect(margin, static_cast<int>(th) - margin - oh, ow, oh))
                        .copyTo(big(cv::Rect(static_cast<int>(ti) * ow,
                                             fullH - static_cast<int>(tj) * oh - oh, ow, oh)));

                setStatusMessage(Constants::Status::RENDER_STATUS,
                                 std::format(L"Export tile {} / {}", tj * gx + ti + 1, totalTiles));
                if (longJobPump) {
                    longJobPump();
                }
            }
        }

        if (deferFog) {
            setStatusMessage(Constants::Status::RENDER_STATUS, L"Export: fog");
            if (longJobPump) {
                longJobPump();
            }
            applyFogToStitched(big, exportFog);
            if (deferLinearInterpolation) {
                applyLinearInterpolationTent(big);
            }
        }

        cv::cvtColor(big, big, cv::COLOR_RGBA2BGRA);
        bool saved = false;
        if (const uint32_t ssaa = settings.render.ssaa; ssaa > 1) {
            cv::Mat resized;
            cv::resize(big, resized,
                       cv::Size(big.cols / static_cast<int>(ssaa), big.rows / static_cast<int>(ssaa)),
                       0, 0, cv::INTER_AREA);
            saved = IOUtilities::writeImage(filename, resized);
        } else {
            saved = IOUtilities::writeImage(filename, big);
        }

        setStatusMessage(Constants::Status::RENDER_STATUS, saved ? L"Export done" : L"Export failed");
        // Restore the live view (the export rebuilt the perturbator for the full grid).
        requests.requestRecompute();
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
        renderer->rendererSlope->setSlope(attr.shader.slope);
        renderer->rendererColor->setColor(attr.shader.color);
        renderer->rendererFog->setFog(attr.shader.fog);
        renderer->rendererBloom->setBloom(attr.shader.bloom, attr.shader.hdr);
        renderer->rendererLinearInterpolation->setLinearInterpolation(attr.render.linearInterpolation);
        renderer->rendererLinearInterpolation->setDither(attr.render.dither);
        // The canvas is not a display, so the preview always takes the tone-mapped SDR end of the transform.
        renderer->rendererLinearInterpolation->setToneMap(attr.shader.hdr, VidHdrTransfer::SDR, 0.0f);
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

    void RenderScene::initRenderer() {
        wc.core.getLogicalDevice().waitDeviceIdle();
        renderer = std::make_unique<RenderSceneRenderer>(engine, wc.getAttachmentIndex());
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
        canvasExtent = wc.getSwapchain().getCurrentExtent();
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


    void RenderScene::refreshSharedImgContext() const {
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
                                               iiiGetter(internalImageExtent, VK_FORMAT_R16G16B16A16_UNORM,
                                                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                         VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                                                         VK_IMAGE_USAGE_SAMPLED_BIT));
        sharedImg.appendMultiframeImageContext(MF_MAIN_RENDER_IMAGE_SECONDARY,
                                               iiiGetter(internalImageExtent, VK_FORMAT_R16G16B16A16_UNORM,
                                                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
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

        // Orders names the way a file manager does: a run of digits counts as its value, so 0009
        // comes before 0010 and map2 before map10, whatever padding the names carry.
        bool naturalLess(const std::wstring &a, const std::wstring &b) {
            size_t i = 0;
            size_t j = 0;
            while (i < a.size() && j < b.size()) {
                if (std::iswdigit(a[i]) && std::iswdigit(b[j])) {
                    size_t ea = i;
                    size_t eb = j;
                    while (ea < a.size() && std::iswdigit(a[ea])) ++ea;
                    while (eb < b.size() && std::iswdigit(b[eb])) ++eb;
                    // Leading zeros carry no value, so they are dropped before the digits are compared.
                    std::wstring_view na(a.data() + i, ea - i);
                    std::wstring_view nb(b.data() + j, eb - j);
                    na.remove_prefix(std::min(na.find_first_not_of(L'0'), na.size() - 1));
                    nb.remove_prefix(std::min(nb.find_first_not_of(L'0'), nb.size() - 1));
                    if (na.size() != nb.size()) {
                        return na.size() < nb.size();
                    }
                    if (na != nb) {
                        return na < nb;
                    }
                    i = ea;
                    j = eb;
                    continue;
                }
                const wchar_t ca = std::towlower(a[i]);
                if (const wchar_t cb = std::towlower(b[j]); ca != cb) {
                    return ca < cb;
                }
                ++i;
                ++j;
            }
            return a.size() - i < b.size() - j;
        }
    }

    void RenderScene::cancelRunningCompute() {
        // The pending request goes with the run: left standing it would start the same compute on
        // the next frame and put it right back over whatever was just loaded.
        requests.recomputeRequested.exchange(false);
        state.cancel();
        previewUploadPending.exchange(false);
        idleCompute = true;
        backgroundThreads.notifyAll();
    }

    bool RenderScene::overwriteMatrixFromMap(const RFFDynamicMapBinary &map) {
        const uint32_t iw = getIterationBufferWidth(attr);
        const uint32_t ih = getIterationBufferHeight(attr);
        if (iw != map.getMatrix().getWidth() || ih != map.getMatrix().getHeight()) {
            vkh::logger::log_err("Map size mismatch, {}x{} required but provided {}x{}", iw, ih,
                                 map.getMatrix().getWidth(), map.getMatrix().getHeight());
            return false;
        }
        // A compute still running owns this buffer through its preview snapshots: the map opened
        // here would be back under the half-finished view within the next frame or two, so the run
        // it belongs to is stopped rather than raced with.
        cancelRunningCompute();
        wc.core.getLogicalDevice().waitDeviceIdle();

        renderer->rendererIteration->setMaxIteration(static_cast<double>(map.getMaxIteration()));
        renderer->iterationStagingBufferContext->fill(map.getMatrix().getCanvas());
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
            return naturalLess(x.filename().wstring(), y.filename().wstring());
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
        // The iteration buffer is being written by the job in either case; browsing would fight it.
        if (isVideoGenerationActive || isVideoExportActive || longJobBusy.load()) {
            return false;
        }
        // Every key handled here walks a folder of maps, so without one they all pass through.
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
        const float multiplier = attr.render.clarityMultiplier * static_cast<float>(attr.render.ssaa);
        return static_cast<uint16_t>(static_cast<float>(cursor.x) * multiplier);
    }

    uint16_t RenderScene::getMouseYOnIterationBuffer() const {
        POINT cursor;
        GetCursorPos(&cursor);
        ScreenToClient(wc.getWindow().getWindowHandle(), &cursor);
        const float multiplier = attr.render.clarityMultiplier * static_cast<float>(attr.render.ssaa);
        // The -1 mirrors the shaders' row flip (height - 1 - y); without it the top row maps past the buffer end.
        return static_cast<uint16_t>(static_cast<float>(getIterationBufferHeight(attr)) - 1.0f -
                                     static_cast<float>(cursor.y) * multiplier);
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

    void RenderScene::recomputeThreaded() {
        // Written before the compute starts, not after it ends: this is the view a run that never
        // comes back from here was working on.
        writeRecoverySnapshot(true);
        computeStartedAt = std::chrono::steady_clock::now();
        // A recompute puts a freshly computed view where the loaded map was, so the folder it came
        // from is no longer what the canvas holds: the keys and the status bar stop answering for it.
        endMapBrowse();
        // createThread cancels and joins the running compute, so its afterCompute lands after the render
        // loop has already cleared idleCompute for this one. Each compute carries the generation it started
        // with and only the newest may declare the scene idle, or a waiter (the keyframe writer) would be
        // released while this compute is still filling the map.
        const uint64_t generation = ++computeGeneration;
        // The previous run is joined here rather than inside createThread, so it is finished with
        // the period beforeCompute is about to read and with the device the uniform below is in.
        state.cancel();
        wc.core.getLogicalDevice().waitDeviceIdle();
        // Cloned and prepared on this thread, not on the worker: beforeCompute writes the iteration
        // uniform buffer the drawing reads, and a worker writing it alongside a frame in flight is
        // rewriting what the GPU is already fetching. Here the device is idle and nothing is.
        Attribute settings = attr; //clone the attr
        beforeCompute(settings);
        state.createThread([this, generation, settings = std::move(settings)](const std::stop_token &) {
            try {
                const bool success = compute(settings);
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
        uint64_t multiplier = lastPeriod == 0 ? 1 : lastPeriod;
        attr.fractal.maxIteration = attr.fractal.autoMaxIteration
                                        ? multiplier * attr.fractal.
                                          autoIterationMultiplier
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

    bool RenderScene::compute(const Attribute &attr) {
        auto start = std::chrono::high_resolution_clock::now();
        const uint16_t w = getIterationBufferWidth(attr);
        const uint16_t h = getIterationBufferHeight(attr);
        uint32_t len = uint32_t(w) * h;

        if (state.interruptRequested()) return false;

        auto &calc = attr.fractal;

        const float logZoom = calc.logZoom;

        if (state.interruptRequested()) return false;

        setStatusMessage(Constants::Status::ZOOM_STATUS, zoomStatus(logZoom));

        const dex dcMax = dcMaxOf(attr, w, h, 1);

        if (!buildPerturbator(attr, dcMax, start)) return false;


        std::atomic renderPixelsCount = 0;

        // Zeroed here instead of in the staging buffer: a zero is what marks a pixel this compute
        // has not reached, and the snapshot on the render thread reads that mark.
        for (uint32_t i = 0, matrixLength = iterationMatrix->getLength(); i < matrixLength; ++i) {
            iterationMatrix->storeRelaxed(i, 0);
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
            const auto computePixel = [this, &attr, &renderPixelsCount, useWhiteFill, maxItValue](
                                          const uint16_t x, const uint16_t y) {
                bool sky = false;
                const auto dc = offsetConversion(attr, x, y, &sky);
                const double it = sky ? maxItValue : currentPerturbator->iterate(dc[0], dc[1]);
                const double stored = (useWhiteFill && it != maxItValue) ? whiteFillValue : it;
                iterationMatrix->storeRelaxed(x, y, stored);
                ++renderPixelsCount;
                return it;
            };

            const auto fillPixel = [this, &renderPixelsCount](const uint16_t x, const uint16_t y, const double v) {
                iterationMatrix->storeRelaxed(x, y, v);
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
                        const auto dc = offsetConversion(attr, sx, sy, &sky);
                        const double it = sky ? maxItValue : currentPerturbator->iterate(dc[0], dc[1]);
                        const double stored = (useWhiteFill && it != maxItValue) ? whiteFillValue : it;

                        const uint16_t bx1 = std::min<uint16_t>(sx + coarseStep, w);
                        const uint16_t by1 = std::min<uint16_t>(sy + coarseStep, h);
                        for (uint16_t y = sy; y < by1; ++y) {
                            for (uint16_t x = sx; x < bx1; ++x) {
                                iterationMatrix->storeRelaxed(x, y, stored);
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
                state, *iterationMatrix, attr.render.threads,
                [attr, this, &renderPixelsCount](const uint16_t x, const uint16_t y, uint16_t, uint16_t, float,
                                                 float, uint32_t, double) {
                    bool sky = false;
                    const auto dc = offsetConversion(attr, x, y, &sky);
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
