//
// Created by Merutilm on 2025-08-08.
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-10, 2026-08-13, 2026-08-14, 2026-08-15, 2026-08-23, 2026-08-24, 2026-08-26, 2026-08-27, 2026-08-31, 2026-09-01, 2026-09-03
// Modified by GPT-5 on 2026-08-21, 2026-08-23, 2026-09-01
// Modified by GPT-6 on 2026-09-08, 2026-09-11, 2026-09-13, 2026-09-14, 2026-09-17, 2026-09-18, 2026-09-20, 2026-09-21, 2026-09-23, 2026-09-24, 2026-09-25, 2026-09-26, 2026-09-27, 2026-09-29, 2026-10-01
// Modified by Opus 5.5 on 2026-09-23, 2026-09-30, 2026-10-04, 2026-10-06, 2026-10-07, 2026-10-09, 2026-10-10, 2026-10-12, 2026-10-13, 2026-10-16, 2026-10-21, 2026-10-25, 2026-10-26, 2026-11-01
//

#pragma once
#include <algorithm>
#include <vector>
#include <windows.h>
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <optional>
#include <map>
#include <set>
#include <string>

#include "ImageCanvas.hpp"
#include "NativeDialogs.hpp"
#include "../io/ShaderPresetIO.h"
#include "SmoothZoomMotion.hpp"
#include "StatusText.hpp"
#include "RenderSceneRequests.hpp"
#include "RenderSceneRenderer.hpp"
#include "../../vulkan_helper/handle/EngineHandler.hpp"
#include "../data/ApproxTableCache.h"
#include "../data/OrbitTrapMatrix.h"
#include "../formula/MandelbrotPerturbator.h"
#include "../io/RFFDynamicMapBinary.h"
#include "../parallel/BackgroundThreads.h"
#include "../preset/Presets.h"
#include "../attr/Attribute.h"
#include "../calc/rff_math.h"
#include "../vulkan/GpuLightPerturbator.hpp"

namespace merutilm::rff2 {
    class RenderScene final : public vkh::EngineHandler {

        vkh::WindowContextRef wc;
        ParallelRenderState state;
        Attribute attr;
        uint64_t previewRevision=0;
        void ensureShaderFormat(bool studio);
        std::function<void(const std::filesystem::path&,bool,uint16_t,uint16_t)> configFileObserver;
        std::function<bool()> configSaveGuard,configReplaceGuard;
        std::function<void()> configReplaceCommit;
        std::filesystem::path configDocumentPath;

        uint16_t interactedMX = 0;
        uint16_t interactedMY = 0;
        bool canvasDragging = false;

        // Eyedropper: when active, the next canvas left-click freezes the picked color's animation.
        bool colorFreezePickActive = false;
        // The panel that armed the pick. The arming belongs to that panel: once it is gone there is
        // nothing left to add a color to, and the canvas must go back to panning on a click.
        HWND colorFreezePickOwner = nullptr;
        std::function<void()> colorFreezePickCallback;
        std::map<double, COLORREF> pickedFreezeColors;
        // Focus pick: when active, the next canvas left-click sets Focus Depth to the clicked pixel's iteration.
        bool focusPickActive = false;
        HWND focusPickOwner = nullptr;
        std::function<void()> focusPickCallback;

        // Shift+drag box-zoom state
        bool boxZooming = false;
        POINT boxAnchorScreen = {0, 0};
        uint16_t boxStartMX = 0;
        uint16_t boxStartMY = 0;
        HWND boxZoomOverlay = nullptr;
        // The frame is handed to the compositor as a finished picture rather than painted into the
        // window after it has been resized: a layered window shows the bytes its surface was given
        // before that paint arrives, and a surface just grown holds none, which is the black frame.
        HDC boxZoomOverlayDC = nullptr;
        HBITMAP boxZoomOverlayBitmap = nullptr;
        HGDIOBJ boxZoomOverlayPreviousBitmap = nullptr;
        uint32_t *boxZoomOverlayPixels = nullptr;
        // Grown to the largest box the drag has reached and kept there, so a moving box reuses it.
        int boxZoomOverlayCapacityW = 0;
        int boxZoomOverlayCapacityH = 0;
        // The box size the picture currently holds. The frame sits on the edges of that size alone,
        // so a box that only moves needs no redraw at all, and one that resizes needs only its
        // predecessor's band cleared.
        int boxZoomOverlayDrawnW = 0;
        int boxZoomOverlayDrawnH = 0;

        uint64_t lastMaxIteration = 0;
        float lastLogZoom = 0;
        uint64_t lastPeriod = 1;

        bool autoIterationBackup = true;
        bool autoIterationCustomActive = false;

        RenderSceneRequests requests;

        std::atomic<bool> idleCompute = true;
        // Nothing is computed while this is set: a recompute asked for meanwhile stays pending until
        // it is cleared. Recovery holds the view this way, so settings that may have ended the last
        // run can be lowered before they are put to work again.
        bool computeHold = false;
        // When the recovery snapshot was last written, so a dragged slider writes one file rather
        // than one per frame.
        std::chrono::steady_clock::time_point lastRecoverySnapshot = {};
        // When the compute now running was asked for. How long it has gone on for is what tells a
        // window closed on a view worth waiting for apart from one closed on a view that never came.
        std::chrono::steady_clock::time_point computeStartedAt = {};
        // Bumped per recompute so a cancelled compute cannot mark the scene idle behind its successor.
        std::atomic<uint64_t> computeGeneration{0};
        std::atomic<bool> isVideoGenerationActive{false};
        std::atomic<bool> isVideoExportActive{false};
        // Set while a paused-preview video export has shrunk the canvas images; the computed map waits in releasedMatrix.
        std::atomic<bool> previewImagesReleased{false};
        std::unique_ptr<Matrix<double>> releasedMatrix = nullptr;
        // The escape angles that belong to releasedMatrix, kept and put back with it.
        std::unique_ptr<Matrix<float>> releasedEscapeAngleMatrix = nullptr;
        // The orbit traps that belong to releasedMatrix, kept and put back with it.
        std::unique_ptr<OrbitTrapMatrix> releasedOrbitTrapMatrix = nullptr;
        std::atomic<bool> longJobBusy{false};


        ApproxTableCache approxTableCache = ApproxTableCache();

        std::array<std::wstring, Constants::Status::LENGTH> *statusMessageRef = nullptr;
        std::mutex *statusMessageMutexRef = nullptr;
        std::unique_ptr<Matrix<double>> iterationMatrix = nullptr;
        // arg z where each pixel of iterationMatrix escaped; 0 (interior, unreached, map or draft) draws as with the coloring off.
        std::unique_ptr<Matrix<float>> escapeAngleMatrix = nullptr;
        // Orbit traps of each pixel of iterationMatrix; iteration 0 (unreached, map or draft) draws as with the trap coloring off.
        std::unique_ptr<OrbitTrapMatrix> orbitTrapMatrix = nullptr;
        // The Orbit Trap Shape the live shape planes are being measured with; disabled while there are none.
        TrapShapeParams liveTrapShape{};

        // The compute threads own iterationMatrix and never touch the staging buffer the GPU copies
        // from; the render thread lifts snapshots of it across, so that buffer has a single writer.
        std::chrono::steady_clock::time_point lastPreviewSnapshot = {};
        // Set when a compute starts, cleared by the exact upload that follows the last of its pixels.
        std::atomic<bool> previewUploadPending = false;
        mutable std::atomic<uint64_t> pendingIterationLimit{0};
        // Row-ordered renders leave the rows below the front untouched, and carrying the front down
        // is what made the partial map read as one picture. A tiled render fills no such front.
        std::atomic<bool> previewFillDown = true;
        // The compute the picture already on the staging buffer was laid down for, so the pixels it has not reached yet keep showing that picture instead of the interior color, and a run started after that one does not inherit it.
        uint64_t previewSeedGeneration = 0;
        bool smoothZoomEnabled = true;
        bool smoothZoomActive = false;
        bool smoothZoomCaptureWider = false;
        bool smoothZoomWasPaused = false;
        bool smoothZoomDirty = false;
        bool smoothZoomExactCandidate = false;
        bool smoothZoomSourceExact = false;
        // The picture navigation holds is a low-resolution draft rather than a full pass.
        bool smoothZoomSourceDraft = false;
        bool smoothZoomNeedsPreview = true;
        bool smoothZoomCandidateFullQuality = false;
        bool smoothZoomDragging = false;
        bool smoothZoomFailed = false;
        double smoothZoomPending = 0.0;
        SmoothZoomMotion::View smoothZoomFrom, smoothZoomTarget, smoothZoomView, smoothZoomCandidate;
        uint16_t smoothZoomMouseX = 0, smoothZoomMouseY = 0;
        uint64_t smoothZoomGeneration = 0;
        uint64_t smoothZoomCandidateRevision = 0;
        std::chrono::steady_clock::time_point smoothZoomStarted{};
        std::chrono::steady_clock::time_point smoothZoomLastTick{};
        std::chrono::steady_clock::time_point smoothZoomInputAt{}, smoothZoomComputeAt{};
        std::optional<FractalAttribute> smoothZoomOriginal;
        std::optional<FractalAttribute> smoothZoomCandidateFractal;
        std::unique_ptr<Matrix<double>> smoothZoomPreviewMatrix;
        std::unique_ptr<Matrix<float>> smoothZoomPreviewAngles;
        std::unique_ptr<OrbitTrapMatrix> smoothZoomPreviewTraps;
        // Pixels of the smooth-zoom draft whose full pass has not started yet, and how long the draft
        // computed for. Only that time is carried over: the wait for the zoom animation between the
        // two passes is no part of the calculation, and the elapsed time left it out before.
        uint64_t smoothZoomDraftPixels = 0;
        std::atomic<std::chrono::high_resolution_clock::rep> smoothZoomDraftTicks{0};
        uint64_t smoothZoomOriginalMax = 0, smoothZoomOriginalPeriod = 0;
        float smoothZoomOriginalLog = 0;
        std::atomic<uint64_t> completedComputeGeneration{0};
        // A padded 360 view is computed once as a planar map Camera Padding Scale times wider, and turning the camera reprojects it instead of recomputing.
        std::unique_ptr<Matrix<double>> panoramaSource;
        std::unique_ptr<Matrix<float>> panoramaSourceAngles;
        // The orbit traps of panoramaSource, reprojected into the view beside the angles.
        std::unique_ptr<OrbitTrapMatrix> panoramaSourceTraps;
        // The settings the finished source was computed for, empty while none is; the camera fields are left out of it.
        std::string panoramaSourceKey;
        // The settings of the source compute in flight and the generation it runs under.
        std::string panoramaPendingKey;
        uint64_t panoramaPendingGeneration = 0;
        double panoramaSourceMax = 0.0;
        [[nodiscard]] bool panoramaPreviewApplies(const Attribute &settings) const;
        [[nodiscard]] std::string panoramaSourceKeyOf(const Attribute &settings) const;
        bool preparePanoramaSource(Attribute &source, uint64_t generation);
        void reprojectPanoramaSource(const Attribute &settings);
        void reprojectPanoramaView();
        void releasePanoramaSource();
        void tickSmoothZoom();
        bool beginSmoothNavigation();
        void retargetSmoothNavigation();
        void settleSmoothNavigation();
        FractalAttribute smoothNavigationCamera(SmoothZoomMotion::View view) const;
        void endSmoothZoom();
        void restoreSmoothZoomSource();

        // The one size the internal images, the render contexts over them and the iteration buffer the shader indexes with its own extent are all built for, taken from the swapchain the images were really created at and moved only by applyResize.
        VkExtent2D canvasExtent = {};
        std::optional<VkExtent2D> documentCanvasExtent;

        std::unique_ptr<MandelbrotPerturbator> currentPerturbator = nullptr;

        std::unique_ptr<RenderSceneRenderer> renderer = nullptr;

        // Built on first use by the compute thread; it stays out of the renderer, which every resize rebuilds.
        std::unique_ptr<GpuLightPerturbator> gpuPerturbator = nullptr;
        bool gpuPerturbatorUnavailable = false;
        std::atomic<bool> gpuComputing = false;

        // Problems met on a worker thread, each a headline and a detail, waiting for the UI thread to show them.
        std::mutex noticeMutex;
        std::vector<std::pair<std::wstring, std::wstring>> pendingNotices;
        std::set<std::wstring> postedNotices;

        // The Debug menu's GPU-pass-timing switch. Kept here rather than on the renderer, which is
        // rebuilt from nothing on every resize and would lose it.
        bool passTimingEnabled = false;

        bool wndFPSRequest = false;
        uint16_t wndCWRequest = 0;
        uint16_t wndCHRequest = 0;

        // The maps sitting in the folder Load Map opened from, in the order the arrow keys walk them,
        // and which of them the canvas is showing. Empty until a map has been loaded.
        std::vector<std::filesystem::path> browsedMaps = {};
        int browsedMapIndex = -1;
        // A place in the folder is being typed toward a jump, and the digits given for it so far.
        bool browsedMapTyping = false;
        std::wstring browsedMapTyped = {};

        // The same for a .png opened by Load File: the pictures in the folder it opened from, which of them is over the
        // canvas, and the window holding it there. The two walks are exclusive - each one owns what
        // the canvas shows - so beginning either ends the other.
        std::vector<std::filesystem::path> browsedImages = {};
        int browsedImageIndex = -1;
        std::unique_ptr<ImageCanvas> imageCanvas = nullptr;

        BackgroundThreads backgroundThreads = BackgroundThreads();

    public:
        SIZE documentCanvasSize() const {
            if (wndCWRequest && wndCHRequest) {
                return {wndCWRequest, wndCHRequest};
            }
            const auto extent = documentCanvasExtent.value_or(canvasExtent);
            return {LONG(extent.width), LONG(extent.height)};
        }
        void configureCanvasSize(uint16_t width, uint16_t height) {
            if (!width || !height) {
                return;
            }
            documentCanvasExtent = VkExtent2D{width, height};
            if (canvasExtent.width != width || canvasExtent.height != height) {
                requests.requestResize();
                requests.requestRecompute();
            }
        }
        explicit RenderScene(vkh::EngineRef engine, vkh::WindowContextRef wc,
                             std::array<std::wstring, Constants::Status::LENGTH> *statusMessageRef,
                             std::mutex *statusMessageMutexRef);

        ~RenderScene() override;

        RenderScene(const RenderScene &) = delete;

        RenderScene &operator=(const RenderScene &) = delete;

        RenderScene(RenderScene &&) = delete;

        RenderScene &operator=(RenderScene &&) = delete;



        // Rebuilds only the swapchain and the framebuffers over it, leaving the compute and the
        // iteration buffer alone. A present that reports the swapchain stale lands here, not in the
        // full resize, which would restart the render.
        void recoverStaleSwapchain() const;

        // present=false serves the pending requests (shader, resize, recompute, image export) but
        // skips the pass that draws them to the window. Keyframe generation is driven by those very
        // requests, so its preview can only be held this way - not by skipping the call.
        void render(bool present = true);

        // canvas * multiplier * sourceScale, clamped in wide types before narrowing to the uint16_t iteration-buffer range.
        [[nodiscard]] static uint32_t scaledRenderAxis(const uint32_t canvas, const float multiplier,
                                                       const uint32_t sourceScale) {
            const double scaled = static_cast<double>(static_cast<float>(canvas) * multiplier);
            const uint64_t base = scaled >= 1.0 ? static_cast<uint64_t>(std::min(scaled, 65535.0)) : 1;
            return static_cast<uint32_t>(std::min<uint64_t>(base * sourceScale, 65535));
        }

        [[nodiscard]] VkExtent2D getInternalImageExtent() const {
            const auto [width, height] = canvasExtent;
            // render internally at clarity * ssaa; the ssaa
            // factor is resolved away by the export downsample (still + video).
            const float multiplier = attr.render.clarityMultiplier * static_cast<float>(attr.render.ssaa);
            return {
                scaledRenderAxis(width, multiplier, attr.video.data.sourceScale),
                scaledRenderAxis(height, multiplier, attr.video.data.sourceScale)
            };
        }

        [[nodiscard]] VkExtent2D getBlurredImageExtent() const {
            const VkExtent2D blurredExtent = getInternalImageExtent();
            if (const float rat = Constants::Fractal::GAUSSIAN_MAX_WIDTH / static_cast<float>(blurredExtent.width);
                rat < 1) {
                return {
                    Constants::Fractal::GAUSSIAN_MAX_WIDTH,
                    std::max(1u, static_cast<uint32_t>(static_cast<float>(blurredExtent.height) * rat))
                };
            }
            return blurredExtent;
        }

        // The size the present images were really created at, which the framebuffer built over them has to name as well - never the surface's own answer, which the window can move out from under.
        [[nodiscard]] VkExtent2D getSwapchainRenderContextExtent() const {
            const auto &swapchain = wc.getSwapchain();
            return swapchain.getCurrentExtent();
        }


        static Attribute genDefaultAttr();

        static LRESULT CALLBACK renderSceneProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        // The overlay draws through UpdateLayeredWindow and is never asked to paint itself.
        static LRESULT CALLBACK boxZoomOverlayProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        // sky, when given, comes back set for a pixel a 360 layout points away from the plane: it sees no
        // fractal at all, so it is filled rather than iterated, and the offset handed back is meaningless.
        [[nodiscard]] std::array<dex, 2> offsetConversion(const Attribute &settings, double mx, double my,
                                                          bool *sky = nullptr) const;

        [[nodiscard]] std::array<dex, 2> offsetConversionFullGrid(const Attribute &settings, int fx, int fy,
                                                                  int fullW, int fullH, int scale,
                                                                  bool *sky = nullptr) const;

        // Largest |dc| over the whole (fullW x fullH) grid, which is the radius the reference has to
        // stay valid out to. Where that pixel sits depends on the projection, so every caller asks here.
        [[nodiscard]] dex dcMaxOf(const Attribute &settings, int fullW, int fullH, int scale) const;

        static dex getDivisor(const Attribute &settings);

        [[nodiscard]] uint16_t getClientWidth() const;

        [[nodiscard]] uint16_t getClientHeight() const;

        [[nodiscard]] uint16_t getIterationBufferWidth(const Attribute &settings) const;

        [[nodiscard]] uint16_t getIterationBufferHeight(const Attribute &settings) const;

        // Shows the oldest problem a worker thread posted; called by the UI thread once per frame.
        void showPendingNotice();

        // Total device-local (VRAM) bytes summed across DEVICE_LOCAL heaps.
        [[nodiscard]] uint64_t getDeviceLocalMemoryTotal() const;

        // Total physical system RAM in bytes.
        [[nodiscard]] static uint64_t getSystemMemoryTotal();

        // Largest clarity*ssaa product whose internal render extent still fits the GPU's maxImageDimension2D (0 = unknown).
        [[nodiscard]] float getMaxInternalScale() const;
        [[nodiscard]] float getMaxInternalScale(uint32_t width,uint32_t height) const;

        // Warning text if the given clarity/SSAA settings risk exhausting VRAM or RAM (empty = within budget).
        [[nodiscard]] std::wstring checkRenderMemoryBudget(const Attribute &settings) const;
        [[nodiscard]] std::wstring checkRenderMemoryBudget(const Attribute &settings,uint32_t width,uint32_t height) const;

        void applyDefaultAttr();

        void applyCreateImage(RenderSceneRequests::CreateImageRequest request);

        // Lifts what the compute has produced so far into the staging buffer. complete asks for the
        // finished map exactly, with no carried-down rows, and ignores the throttle.
        void snapshotComputePreview(bool complete);

        void applyShaderAttr(const Attribute &attr) const;

        void refreshResizeParams();

        // OrbitTrap::SHAPE_PLANE_COUNT while settings color by the Orbit Trap Shape, PLANE_COUNT otherwise.
        [[nodiscard]] static uint32_t orbitTrapPlaneCountOf(const Attribute &settings);

        // Reallocates the live trap planes when Orbit Trap Coloring moved onto or off Shape; called with no compute running.
        void matchOrbitTrapPlanes(const Attribute &settings);

        // Takes the canvas size from the swapchain images as they now stand.
        void refreshCanvasExtent();

        // Puts the map the previous canvas held back on the new one, centred and pixel for pixel, which is where a resize leaves it - only the strip a larger canvas uncovers is left for the compute.
        void seedPreviewFromMatrix(const Matrix<double> &previous);

        // Magnifies the picture already on the staging buffer so the region around (srcCenterX, srcCenterY) fills the canvas, which is the view the compute starting next is about to draw: the zoom carries the old image instead of blanking to the interior color until the new map reaches each pixel.
        void seedPreviewFromZoom(double srcCenterX, double srcCenterY, double magnification);

        // Waits every frame in flight out, which is what makes rewriting the one staging buffer they all copy from safe.
        void waitFramesInFlight() const;

        void initRenderer(bool prepareAll = false);

        // Swaps in a surface shader a skip left compiling once it is ready, and lets go of finished warm-ups.
        void collectDeferredPipelines();

        // Waits, with the preparation window up, for a surface shader a skip left compiling.
        void awaitDeferredSlope() const;

        void refreshRenderContext() const;

        void applyResize();

        void refreshSharedImgContext(VkFormat format=VK_FORMAT_UNDEFINED) const;
        uint64_t getPreviewRevision() const { return previewRevision; }
        std::pair<cv::Mat,cv::Mat> renderComparison(const ShaderAttribute& reference,float seconds);
        void zoomToImagePoint(double x,double y,double factor);
        std::pair<Matrix<double>,uint64_t> sampleIterationsForAi() const;

        // False when the map does not fit the current iteration buffer, which leaves the canvas as it was.
        bool overwriteMatrixFromMap(const RFFDynamicMapBinary &map);

        // Stops a compute in flight, and drops a request for one still waiting to start, so what is
        // put on the canvas next is not overwritten by a run that was already under way.
        void cancelRunningCompute();

        // Lists the maps beside the one just loaded, so the arrow keys can step through the folder
        // without going back through the dialog. Both forms are listed, in one order.
        void beginMapBrowse(const std::filesystem::path &loaded);

        // Forgets that folder, which is what a recompute does: nothing of it is on the canvas any more.
        void endMapBrowse();

        // Puts the picture over the canvas and lists the pictures beside it, so the arrow keys step
        // through the folder as they do for maps. The fractal keeps whatever it had underneath.
        void beginImageBrowse(const std::filesystem::path &loaded);

        // Takes the picture away and forgets that folder, which puts the fractal back on screen.
        void endImageBrowse();

        // True while a picture is over the canvas.
        [[nodiscard]] bool isImageBrowsing() const { return browsedImageIndex >= 0; }

        // Keeps that picture on the canvas as the window is resized. Called from where the canvas
        // itself is sized, because the two must land on the same rectangle.
        void layoutImageCanvas(const RECT &area) const;

        // Moves that selection by delta maps, stopping at the ends.
        void stepBrowsedMap(int delta);

        // Jumps it to the first or the last map of the folder.
        void jumpBrowsedMap(bool last);

        // Collects a typed position: digits build the number, Backspace takes one back, Enter jumps
        // to it and Escape drops it. False when the key is none of those, or nothing is being typed.
        bool typeBrowsedMapPosition(WPARAM key);

        // Opens that typing for a number, as clicking the status bar's map part does. False when no
        // folder is being walked, which leaves the click to mean nothing.
        bool armBrowsedMapTyping();

        // Handles a keystroke from the master window, which is where one lands: the canvas is a child
        // window that never takes focus. True when the key was used up here.
        bool runKeyAction(WPARAM key);

        [[nodiscard]] uint16_t getMouseXOnIterationBuffer() const;

        [[nodiscard]] uint16_t getMouseYOnIterationBuffer() const;

        void updateIterationStatus() const;

        void recomputeThreaded(const Attribute* overrideSettings = nullptr, bool lowResolution = false);

        void beforeCompute(Attribute &attr) const;

        // Where one compute sits inside the progress the status bar shows. A smooth-zoom draft and the
        // full pass after it read as one job: the draft counts its pixels toward the full pass's total
        // and holds back "Done", and the full pass carries on from the draft's count and clock.
        struct ComputeProgress {
            uint64_t before = 0;
            uint64_t total = 0;
            bool announceDone = true;
            std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
        };

        bool compute(const Attribute &attr, Matrix<double>* output = nullptr, const Attribute* samplingGeometry = nullptr,
                     const ComputeProgress* progress = nullptr, Matrix<float>* angleOutput = nullptr,
                     OrbitTrapMatrix* trapOutput = nullptr);

        // Computes the padded planar source on the worker and reprojects it into the view.
        bool computePanoramaSource(const Attribute &source, const Attribute &settings, const ComputeProgress *progress);

        // Builds currentPerturbator for the given settings/dcMax honoring reuseReferenceMethod.
        // Extracted from compute() so the tiled export can share the exact same reference setup.
        bool buildPerturbator(Attribute &attr, const dex &dcMax,
                              std::chrono::high_resolution_clock::time_point start);

        using PixelCoordinate = std::function<std::array<dex, 2>(uint16_t, uint16_t, bool *)>;

        // Runs the iteration on the GPU when the settings and view allow it; false leaves the job to the CPU paths.
        bool computeOnGpu(const Attribute &attr, const PixelCoordinate &coordinate, uint16_t w, uint16_t h,
                          Matrix<double> &matrix, Matrix<float> *escapeAngles, OrbitTrapMatrix *traps,
                          std::atomic<int> &renderPixelsCount);

        // Draws the Buddhabrot density on the flat plane, whatever the projection; false when interrupted.
        bool computeBuddhabrot(const Attribute &geometry, uint16_t w, uint16_t h, double fullW, double fullH,
                               Matrix<double> &matrix, std::atomic<int> &renderPixelsCount);

        // Clears the escape angles and orbit traps on the matrices and staging buffers, for a picture put there by anything but a compute of this view.
        void clearEscapeAngles();

        // The GPU engine, built on first use; nullptr when this device cannot run it.
        GpuLightPerturbator *gpuPerturbatorOrNull();

        // Every pixel's dc as the CPU iterate() forms it, x then y, NaN for a sky pixel.
        std::vector<double> gpuPixelOffsets(const Attribute &attr, const LightMandelbrotPerturbator &perturbator,
                                            const PixelCoordinate &coordinate, uint16_t w, uint16_t h) const;

        void afterCompute(bool success, uint64_t generation);

        // Queues a problem for showPendingNotice; one already posted this session is not posted again.
        void postNotice(std::wstring headline, std::wstring detail);

        // Says on the status line and in a notice that the view's calculation stopped on an error.
        void reportComputeFailure(std::wstring detail);

        void setStatusMessage(const int index, const std::wstring_view &message) const {
            std::scoped_lock lock(*statusMessageMutexRef);
            (*statusMessageRef)[index] = std::wstring(L"  ").append(message);
        }

        // True while such a job is running. Message handlers that mutate the scene or the swapchain
        // must do nothing while it is set, because the pump dispatches them from inside the job.
        [[nodiscard]] bool isLongJobBusy() const {
            return longJobBusy.load() || vkh::GraphicsContextWindowImpl::isPipelineCompilationPending();
        }

        void requestLongJobCancel() {
            state.interrupt();
        }

        [[nodiscard]] Attribute &getAttribute() {
            return attr;
        }

        // Arms the eyedropper: the next canvas left-click appends the picked color to the
        // palette's frozen list. onPicked runs after a successful pick (e.g. to refresh the UI).
        // owner is the panel the pick belongs to; the arming is dropped when that window closes.
        void beginColorFreezePick(const HWND owner, std::function<void()> onPicked) {
            cancelFocusPick();
            colorFreezePickActive = true;
            colorFreezePickOwner = owner;
            colorFreezePickCallback = std::move(onPicked);
        }

        [[nodiscard]] COLORREF pickedFreezeColor(double iteration) const {
            const auto found = pickedFreezeColors.find(iteration);
            return found == pickedFreezeColors.end() ? CLR_INVALID : found->second;
        }

        // Arms the focus pick: the next canvas left-click moves Focus Depth onto that pixel's iteration count.
        // onPicked runs after the click whether or not the pixel had a count; the arming is dropped when owner closes.
        void beginFocusPick(const HWND owner, std::function<void()> onPicked) {
            cancelColorFreezePick();
            focusPickActive = true;
            focusPickOwner = owner;
            focusPickCallback = std::move(onPicked);
        }

        void cancelFocusPick() {
            focusPickActive = false;
            focusPickOwner = nullptr;
            focusPickCallback = nullptr;
        }

        [[nodiscard]] bool isFocusPickActive() {
            if (focusPickActive && focusPickOwner != nullptr && !IsWindow(focusPickOwner)) {
                cancelFocusPick();
            }
            return focusPickActive;
        }

        void cancelColorFreezePick() {
            colorFreezePickActive = false;
            colorFreezePickOwner = nullptr;
            colorFreezePickCallback = nullptr;
        }

        // Not const: a pick whose panel has since closed is dropped here, which is the only place
        // the closing is noticed - a panel is destroyed without passing through this class.
        [[nodiscard]] bool isColorFreezePickActive() {
            if (colorFreezePickActive && colorFreezePickOwner != nullptr && !IsWindow(colorFreezePickOwner)) {
                cancelColorFreezePick();
            }
            return colorFreezePickActive;
        }

        void applyLoadedConfig();
        void setPreviewAnimationPaused(bool paused) { renderer->rendererIteration->setPreviewAnimationPaused(paused); }

        void swapAnimationLayers(AnimatedLayerFamily family, uint32_t from, uint32_t to) {
            if (renderer) {
                renderer->rendererIteration->swapAnimationLayers(family, from, to);
                renderer->lastShadedPhases.swapLayers(family, from, to);
            }
        }
        bool isPreviewAnimationPaused() const { return renderer->rendererIteration->previewClock.paused; }
        bool* smoothZoomOption() { return &smoothZoomEnabled; }
        void setConfigFileObserver(
            std::function<void(const std::filesystem::path&, bool, uint16_t, uint16_t)> observer) {
            configFileObserver = std::move(observer);
        }
        void setConfigGuards(std::function<bool()> save, std::function<bool()> replace,
                             std::function<void()> commit = {}) {
            configSaveGuard = std::move(save);
            configReplaceGuard = std::move(replace);
            configReplaceCommit = std::move(commit);
        }
        bool prepareConfigSave() {
            return !configSaveGuard || configSaveGuard();
        }
        bool prepareConfigReplace() {
            return !configReplaceGuard || configReplaceGuard();
        }
        void commitConfigReplace() {
            if (configReplaceCommit) {
                configReplaceCommit();
            }
        }
        const std::filesystem::path& getConfigDocumentPath() const {
            return configDocumentPath;
        }
        void notifyConfigFile(const std::filesystem::path& path, bool loaded,
                              uint16_t width = 0, uint16_t height = 0) {
            configDocumentPath = path;
            const auto dimensions = documentCanvasSize();
            if (configFileObserver) {
                configFileObserver(path, loaded, width ? width : dimensions.cx,
                                   height ? height : dimensions.cy);
            }
        }

        // Puts the view a settings file holds - the fields a location file carries - and its formula
        // onto otherwise default settings. What recovery generates from when the settings themselves
        // are what is suspected of ending the last run.
        void applyRecoveredLocation(const Attribute &loaded);

        // Holds every compute back, or lets them run again. See computeHold.
        void setComputeHold(bool hold);

        // Puts the settings this session is working on where the next start can pick them up from.
        // force writes at once (the view is about to be computed, so this is the location a crash
        // would be at); otherwise it is written at most once every few seconds.
        void writeRecoverySnapshot(bool force);

        [[nodiscard]] ParallelRenderState &getState() {
            return state;
        }

        [[nodiscard]] MandelbrotPerturbator *getCurrentPerturbator() const {
            return currentPerturbator.get();
        }

        [[nodiscard]] ApproxTableCache &getApproxTableCache() {
            return approxTableCache;
        }

        [[nodiscard]] BackgroundThreads &getBackgroundThreads() {
            return backgroundThreads;
        }

        [[nodiscard]] bool &passTimingFlag() {
            return passTimingEnabled;
        }

        // Per-pass GPU times gathered since the switch was last turned on.
        [[nodiscard]] std::wstring getPassTimingReport() const;

        // Throws those away, so the next measurement is of the frames that follow rather than an
        // average over everything drawn since.
        void clearPassTiming() const;

        // Everything the scene is holding right now in one block: the view, what the canvas costs,
        // what the last compute came to, the reference and its tables, and what is still running.
        [[nodiscard]] std::wstring dumpState();

        [[nodiscard]] uint64_t getMapMaxIteration() const { return lastMaxIteration; }

        [[nodiscard]] RFFDynamicMapBinary generateMap() const {
            return RFFDynamicMapBinary(lastLogZoom, lastPeriod, lastMaxIteration, *iterationMatrix);
        }

        // The orbit traps of the map generateMap returns, plane-major; empty when this view holds none.
        [[nodiscard]] std::vector<float> generateOrbitTrapPlanes() const {
            if (orbitTrapMatrix == nullptr || iterationMatrix == nullptr ||
                orbitTrapMatrix->getLength() != iterationMatrix->getLength()) {
                return {};
            }
            std::vector<float> planes(static_cast<size_t>(orbitTrapMatrix->getLength()) * orbitTrapMatrix->getPlaneCount());
            orbitTrapMatrix->copyTo(planes.data());
            const auto iterations = planes.begin() + static_cast<std::ptrdiff_t>(3u * orbitTrapMatrix->getLength());
            const auto iterationsEnd = iterations + static_cast<std::ptrdiff_t>(orbitTrapMatrix->getLength());
            if (std::all_of(iterations, iterationsEnd, [](const float v) { return v == 0.0f; })) return {};
            return planes;
        }

        [[nodiscard]] RenderSceneRequests &getRequests() {
            return requests;
        }

        [[nodiscard]] bool isFPSRequested() const {
            return wndFPSRequest;
        }

        [[nodiscard]] bool isIdleCompute() const {
            return idleCompute && !smoothZoomActive;
        }

        // True when the shutdown now under way leaves a view that was asked for and never arrived.
        // The compute is threaded, so the window keeps answering however heavy it gets: closing it
        // is how a view too slow to wait for is given up on, and that is the ending whose settings
        // are kept for the next start. Only a compute that has gone on long enough counts - one
        // closed a moment after it started is an ordinary exit, and offering it back would put the
        // panel in front of a user who was only leaving. See RecoveryIO.
        [[nodiscard]] bool isComputeUnfinished() const;

        void setVideoGenerationActive(bool active) {
            isVideoGenerationActive = active;
        }

        [[nodiscard]] bool getVideoGenerationActive() const {
            return isVideoGenerationActive;
        }

        void setVideoExportActive(const bool active) {
            isVideoExportActive = active;
        }

        [[nodiscard]] bool getVideoExportActive() const {
            return isVideoExportActive;
        }

        // Render thread only: shrinks the canvas images to free VRAM for an export, and puts the map back at full size afterwards.
        void setPreviewImagesReleased(bool released);

        [[nodiscard]] bool isPreviewImagesReleased() const {
            return previewImagesReleased;
        }

        [[nodiscard]] int getWndCWRequest() const {
            return wndCWRequest;
        }

        [[nodiscard]] int getWndCHRequest() const {
            return wndCHRequest;
        }

        [[nodiscard]] vkh::WindowContextRef getWindowContext() const {
            return wc;
        }

        void wndRequestFPS() {
            wndFPSRequest = true;
        }

        void wndRequestClientSize(const uint16_t width, const uint16_t height) {
            wndCWRequest = width;
            wndCHRequest = height;
        }

        void wndClientSizeRequestSolved() {
            wndCWRequest = 0;
            wndCHRequest = 0;
        }

        void wndFPSRequestSolved() {
            wndFPSRequest = false;
        }


        template<typename P> requires std::is_base_of_v<Preset, P>
        void changePreset(P &preset);

    private:
        void runAction(UINT msg, WPARAM wparam, LPARAM lparam);

        // Shows the map of that index and reports it in the status bar. The selection moves even when
        // the map cannot be shown, so a map of another size cannot trap the arrow keys on itself.
        void applyBrowsedMap(int index);

        // The status bar's line for the map of that index: its place in the folder.
        [[nodiscard]] std::wstring browsedMapStatus(int index) const;

        // The same three for the pictures a .png from Load File walks. There is no typed jump among them: a
        // picture is put up whole by the arrow keys alone.
        void applyBrowsedImage(int index);

        [[nodiscard]] std::wstring browsedImageStatus(int index) const;

        void stepBrowsedImage(int delta);

        void jumpBrowsedImage(bool last);

        // The canvas rectangle the picture is laid over, in the canvas's own coordinates.
        [[nodiscard]] RECT imageCanvasArea() const;

        // Puts the number being typed in the status bar, in place of the position it will replace.
        void showTypedBrowsedMapPosition() const;

        void ensureBoxZoomOverlay();

        void updateBoxZoomOverlay(POINT anchorScreen, POINT currentScreen);

        void hideBoxZoomOverlay() const;

        // Grows the overlay's picture to hold a box of this size, keeping what it already holds.
        bool ensureBoxZoomOverlayBitmap(int width, int height);

        void destroyBoxZoomOverlayBitmap();

        void applyBoxZoom(int startMX, int startMY, int endMX, int endMY);

        void init() override;

        void attachRenderContext() const;

        void destroy() override;
    };


    template<typename P> requires std::is_base_of_v<Preset, P>
    void RenderScene::changePreset(P &preset) {
        if constexpr (std::is_base_of_v<Presets::CalculationPreset, P>) {
            attr.fractal.mpaAttribute = preset.genMPA();
            attr.fractal.referenceCompAttribute = preset.genReferenceCompression();
            requests.requestRecompute();
        }
        if constexpr (std::is_base_of_v<Presets::RenderPreset, P>) {
            // A quality preset says nothing about which processor calculates, so the GPU choice stays.
            const RndComputeShaderAttribute computeShader = attr.render.computeShader;
            // Nor which filter resolves Supersampling.
            const RndSsaaFilter ssaaFilter = attr.render.ssaaFilter;
            attr.render = preset.genRender();
            attr.render.computeShader = computeShader;
            attr.render.ssaaFilter = ssaaFilter;
            requests.requestResize();
            requests.requestRecompute();
        }
        if constexpr (std::is_base_of_v<Presets::ResolutionPreset, P>) {
            auto r = preset.genResolution();
            wndRequestClientSize(r[0], r[1]);
        }
        if constexpr (std::is_base_of_v<Presets::ShaderPreset, P>) {
            if constexpr (std::is_base_of_v<Presets::ShaderPresets::FullShaderPreset, P>) {
                // Every section at once, so nothing of the shader on screen is carried over.
                auto shader = preset.genShader();
                if (!ShaderPresetIO::validate(shader)) {
                    NativeDialogs::message(nullptr,
                                           "Could not load this example. Check that its file is complete and readable, then try again.",
                                           "Example Not Loaded", MB_OK | MB_ICONWARNING);
                    return;
                }
                attr.shader = std::move(shader);
            }
            if constexpr (std::is_base_of_v<Presets::ShaderPresets::PalettePreset, P>) {
                // Capture the seed before generating so the same color array can be regenerated on load.
                const int32_t recipeId = preset.getPaletteRecipeId();
                uint32_t seed = 0;
                if (recipeId >= 0) {
                    seed = rff_math::randomSeed();
                    rff_math::reseed(seed);
                }
                attr.shader.palette = preset.genPalette();
                attr.shader.palette.recipePresetId = recipeId;
                attr.shader.palette.recipeSeed = seed;
            }
            if constexpr (std::is_base_of_v<Presets::ShaderPresets::StripePreset, P>) {
                attr.shader.stripe = preset.genStripe();
            }
            if constexpr (std::is_base_of_v<Presets::ShaderPresets::SlopePreset, P>) {
                attr.shader.slope = preset.genSlope();
            }
            if constexpr (std::is_base_of_v<Presets::ShaderPresets::ColorPreset, P>) {
                attr.shader.color = preset.genColor();
            }
            if constexpr (std::is_base_of_v<Presets::ShaderPresets::FogPreset, P>) {
                attr.shader.fog = preset.genFog();
            }
            if constexpr (std::is_base_of_v<Presets::ShaderPresets::BloomPreset, P>) {
                attr.shader.bloom = preset.genBloom();
            }
            requests.requestShader();
        }
    }
}
