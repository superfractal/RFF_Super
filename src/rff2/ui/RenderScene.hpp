//
// Created by Merutilm on 2025-08-08.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21, 2026-08-23, 2026-09-01.
// Modified by Opus 5 on 2026-08-10, 2026-08-13, 2026-08-14, 2026-08-15, 2026-08-23, 2026-08-24, 2026-08-26, 2026-08-27, 2026-08-31, 2026-09-01
//

#pragma once
#include <vector>
#include <windows.h>
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>

#include "RenderSceneRequests.hpp"
#include "RenderSceneRenderer.hpp"
#include "../../vulkan_helper/handle/EngineHandler.hpp"
#include "../data/ApproxTableCache.h"
#include "../formula/MandelbrotPerturbator.h"
#include "../io/RFFDynamicMapBinary.h"
#include "../parallel/BackgroundThreads.h"
#include "../preset/Presets.h"
#include "../attr/Attribute.h"
#include "../calc/rff_math.h"

namespace merutilm::rff2 {
    class RenderScene final : public vkh::EngineHandler {

        vkh::WindowContextRef wc;
        ParallelRenderState state;
        Attribute attr;

        uint16_t interactedMX = 0;
        uint16_t interactedMY = 0;

        // Eyedropper: when active, the next canvas left-click freezes the picked color's animation.
        bool colorFreezePickActive = false;
        // The panel that armed the pick. The arming belongs to that panel: once it is gone there is
        // nothing left to add a color to, and the canvas must go back to panning on a click.
        HWND colorFreezePickOwner = nullptr;
        std::function<void()> colorFreezePickCallback;

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
        std::atomic<bool> longJobBusy{false};
        std::function<void()> longJobPump;


        ApproxTableCache approxTableCache = ApproxTableCache();

        std::array<std::wstring, Constants::Status::LENGTH> *statusMessageRef = nullptr;
        std::mutex *statusMessageMutexRef = nullptr;
        std::unique_ptr<Matrix<double>> iterationMatrix = nullptr;

        // The compute threads own iterationMatrix and never touch the staging buffer the GPU copies
        // from; the render thread lifts snapshots of it across, so that buffer has a single writer.
        std::chrono::steady_clock::time_point lastPreviewSnapshot = {};
        // Set when a compute starts, cleared by the exact upload that follows the last of its pixels.
        std::atomic<bool> previewUploadPending = false;
        // Row-ordered renders leave the rows below the front untouched, and carrying the front down
        // is what made the partial map read as one picture. A tiled render fills no such front.
        std::atomic<bool> previewFillDown = true;
        // The compute the picture already on the staging buffer was laid down for, so the pixels it has not reached yet keep showing that picture instead of the interior color, and a run started after that one does not inherit it.
        uint64_t previewSeedGeneration = 0;

        // The one size the internal images, the render contexts over them and the iteration buffer the shader indexes with its own extent are all built for, taken from the swapchain the images were really created at and moved only by applyResize.
        VkExtent2D canvasExtent = {};

        std::unique_ptr<MandelbrotPerturbator> currentPerturbator = nullptr;

        std::unique_ptr<RenderSceneRenderer> renderer = nullptr;

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

        BackgroundThreads backgroundThreads = BackgroundThreads();

    public:
        explicit RenderScene(vkh::EngineRef engine, vkh::WindowContextRef wc,
                             std::array<std::wstring, Constants::Status::LENGTH> *statusMessageRef,
                             std::mutex *statusMessageMutexRef);

        ~RenderScene() override;

        RenderScene(const RenderScene &) = delete;

        RenderScene &operator=(const RenderScene &) = delete;

        RenderScene(RenderScene &&) = delete;

        RenderScene &operator=(RenderScene &&) = delete;



        void resolveWindowResizeEnd() const;

        // Rebuilds only the swapchain and the framebuffers over it, leaving the compute and the
        // iteration buffer alone. A present that reports the swapchain stale lands here, not in the
        // full resize, which would restart the render.
        void recoverStaleSwapchain() const;

        // present=false serves the pending requests (shader, resize, recompute, image export) but
        // skips the pass that draws them to the window. Keyframe generation is driven by those very
        // requests, so its preview can only be held this way - not by skipping the call.
        void render(bool present = true);

        [[nodiscard]] VkExtent2D getInternalImageExtent() const {
            const auto [width, height] = canvasExtent;
            // render internally at clarity * ssaa; the ssaa
            // factor is resolved away by the export downsample (still + video).
            const float multiplier = attr.render.clarityMultiplier * static_cast<float>(attr.render.ssaa);
            return {
                static_cast<uint32_t>(static_cast<float>(width) * multiplier),
                static_cast<uint32_t>(static_cast<float>(height) * multiplier)
            };
        }

        [[nodiscard]] VkExtent2D getBlurredImageExtent() const {
            const VkExtent2D blurredExtent = getInternalImageExtent();
            if (const float rat = Constants::Fractal::GAUSSIAN_MAX_WIDTH / static_cast<float>(blurredExtent.width);
                rat < 1) {
                return {
                    Constants::Fractal::GAUSSIAN_MAX_WIDTH,
                    static_cast<uint32_t>(static_cast<float>(blurredExtent.height) * rat)
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
        [[nodiscard]] std::array<dex, 2> offsetConversion(const Attribute &settings, int mx, int my,
                                                          bool *sky = nullptr) const;

        // Full-grid variant of offsetConversion for tiled high-res export: maps a pixel of the
        // (fullW x fullH) full image to its fractal offset from center.
        [[nodiscard]] std::array<dex, 2> offsetConversionTiled(const Attribute &settings, int fx, int fy,
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

        // Total device-local (VRAM) bytes summed across DEVICE_LOCAL heaps.
        [[nodiscard]] uint64_t getDeviceLocalMemoryTotal() const;

        // Total physical system RAM in bytes.
        [[nodiscard]] static uint64_t getSystemMemoryTotal();

        // Largest clarity*ssaa product whose internal render extent still fits the GPU's maxImageDimension2D (0 = unknown).
        [[nodiscard]] float getMaxInternalScale() const;

        // Warning text if the given clarity/SSAA settings risk exhausting VRAM or RAM (empty = within budget).
        [[nodiscard]] std::wstring checkRenderMemoryBudget(const Attribute &settings) const;

        // Pixels a tiled export renders past each edge of the region it keeps, so every kept pixel has
        // the real neighbourhood the slope Sobel, its macro ring and the interpolation tent read.
        [[nodiscard]] int exportTileMargin(const Attribute &settings, uint32_t tilesX, uint32_t tilesY) const;

        // Warning text if a tilesX*tilesY high-res export risks exhausting RAM or VRAM (empty = within budget).
        [[nodiscard]] std::wstring checkExportMemoryBudget(const Attribute &settings, uint32_t tilesX,
                                                           uint32_t tilesY) const;

        void applyDefaultAttr();

        void applyCreateImage(RenderSceneRequests::CreateImageRequest request);

        // Lifts what the compute has produced so far into the staging buffer. complete asks for the
        // finished map exactly, with no carried-down rows, and ignores the throttle.
        void snapshotComputePreview(bool complete);

        void applyExportHighRes(uint32_t tilesX, uint32_t tilesY);

        void applyShaderAttr(const Attribute &attr) const;

        void refreshResizeParams();

        // Takes the canvas size from the swapchain images as they now stand.
        void refreshCanvasExtent();

        // Puts the map the previous canvas held back on the new one, centred and pixel for pixel, which is where a resize leaves it - only the strip a larger canvas uncovers is left for the compute.
        void seedPreviewFromMatrix(const Matrix<double> &previous);

        // Magnifies the picture already on the staging buffer so the region around (srcCenterX, srcCenterY) fills the canvas, which is the view the compute starting next is about to draw: the zoom carries the old image instead of blanking to the interior color until the new map reaches each pixel.
        void seedPreviewFromZoom(double srcCenterX, double srcCenterY, double magnification);

        // Waits every frame in flight out, which is what makes rewriting the one staging buffer they all copy from safe.
        void waitFramesInFlight() const;

        void initRenderer();

        void refreshRenderContext() const;

        void applyResize();

        void refreshSharedImgContext() const;

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

        void recomputeThreaded();

        void beforeCompute(Attribute &attr) const;

        bool compute(const Attribute &attr);

        // Builds currentPerturbator for the given settings/dcMax honoring reuseReferenceMethod.
        // Extracted from compute() so the tiled export can share the exact same reference setup.
        bool buildPerturbator(const Attribute &attr, const dex &dcMax,
                              std::chrono::high_resolution_clock::time_point start);

        void afterCompute(bool success, uint64_t generation);

        void setStatusMessage(const int index, const std::wstring_view &message) const {
            std::scoped_lock lock(*statusMessageMutexRef);
            (*statusMessageRef)[index] = std::wstring(L"  ").append(message);
        }

        // Installed by Application. A job that owns the UI thread for minutes calls this between
        // work units so the status bar still paints, the window keeps answering, and a cancel
        // gesture can reach state.interrupt() -- none of which happen while render() has not returned.
        void setLongJobPump(std::function<void()> pump) {
            longJobPump = std::move(pump);
        }

        // True while such a job is running. Message handlers that mutate the scene or the swapchain
        // must do nothing while it is set, because the pump dispatches them from inside the job.
        [[nodiscard]] bool isLongJobBusy() const {
            return longJobBusy.load();
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
            colorFreezePickActive = true;
            colorFreezePickOwner = owner;
            colorFreezePickCallback = std::move(onPicked);
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

        // Puts the view a settings file holds - the fields a location file carries - and its formula
        // onto otherwise default settings. What recovery generates from when the settings themselves
        // are what is suspected of ending the last run.
        void applyRecoveredLocation(const Attribute &loaded);

        // Holds every compute back, or lets them run again. See computeHold.
        void setComputeHold(bool hold);

        [[nodiscard]] bool isComputeHold() const {
            return computeHold;
        }

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

        void setCurrentPerturbator(std::unique_ptr<MandelbrotPerturbator> perturbator) {
            currentPerturbator = std::move(perturbator);
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
        [[nodiscard]] std::wstring dumpState() const;

        [[nodiscard]] RFFDynamicMapBinary generateMap() const {
            return RFFDynamicMapBinary(lastLogZoom, lastPeriod, lastMaxIteration, *iterationMatrix);
        }

        [[nodiscard]] RenderSceneRequests &getRequests() {
            return requests;
        }

        [[nodiscard]] bool isFPSRequested() const {
            return wndFPSRequest;
        }

        [[nodiscard]] bool isIdleCompute() const {
            return idleCompute;
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
            attr.render = preset.genRender();
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
                attr.shader = preset.genShader();
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
