//
// Created by Merutilm on 2025-09-06.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-07-09, 2026-08-21
// Modified by Opus 5 on 2026-08-10, 2026-08-19
// Modified by GPT-6 on 2026-09-15, 2026-09-20, 2026-09-21, 2026-09-22, 2026-09-23, 2026-09-26
//

#pragma once
#include <functional>
#include <queue>

#include "VideoBufferCache.hpp"
#include "VideoRenderSceneRenderer.hpp"
#include "../../vulkan_helper/handle/EngineHandler.hpp"
#include "../attr/Attribute.h"
#include "../io/RFFDynamicMapBinary.h"
#include "../video/TimelineEvaluator.hpp"
#include "../video/TimelineAnimationPhases.hpp"

namespace merutilm::rff2 {
    class VideoRenderScene final : vkh::EngineHandler {

        vkh::WindowContextRef wc;
        std::function<void()> preparationHook;
        RFFBinary *normal = nullptr;
        RFFBinary *zoomed = nullptr;
        const VkExtent2D videoExtent;
        Attribute baseAttribute;
        // The base the grading chain opens on when the source is PNG: see staticGradeBase.
        ShaderAttribute staticShader;
        ShaderAttribute liveShader;
        TimelineEvaluator timelineEvaluator;
        std::unique_ptr<TimelineAnimationPhases> animationIntegrator;
        std::array<double, 3> animationScheduleKey{};
        bool animationInputsChanged = true;
        std::unique_ptr<VideoRenderSceneRenderer> renderer = nullptr;

        std::mutex bufferCachedMutex;
        std::queue<std::unique_ptr<VideoBufferCache>> queuedVbc = {};
        std::condition_variable bufferCachedCondition;

        // Historical description of the removed, unread CPU timing counters:
        // Export timing, reported next to the video. Backpressure > 0 means the consumer, not the GPU, is the limit.

    public:
        explicit VideoRenderScene(vkh::EngineRef engine, vkh::WindowContextRef wc, const VkExtent2D &videoExtent, const Attribute &targetAttribute, std::function<void()> beforeWait = {});

        ~VideoRenderScene() override;

        VideoRenderScene(const VideoRenderScene &) = delete;

        VideoRenderScene &operator=(const VideoRenderScene &) = delete;

        VideoRenderScene(VideoRenderScene &&) = delete;

        VideoRenderScene &operator=(VideoRenderScene &&) = delete;

        void applyCurrentDynamicMap(const RFFDynamicMapBinary &normal, const RFFDynamicMapBinary &zoomed, float currentFrame) const;

        void setMaxIterationDynamic(double maxIteration, double normalMaxIteration, double zoomedMaxIteration) const;

        void setSampleJitter(float jitterX, float jitterY) const;

        void applyShaderStatic() const;

        // Picks the curve the encoded frames carry, and with it whether they are packed 8-bit or 16-bit.
        void setHdrOutput(VidHdrTransfer transfer, float peakNits) const;

        // False whenever HDR is off, since without the float chain there is nothing above white to carry.
        [[nodiscard]] bool isHdrOutput(VidHdrTransfer transfer) const;


        void applyShaderDynamic(const ShaderAttribute &shader, TimelineDirtyMask dirty) const;

        void applyTimelineShader(float depth, double sec);

        void updateBase(const ShaderAttribute &shader, const VidTimelineAttribute &timeline);

        void setTimelineSchedule(const TimelineSchedule &schedule);

        void setTime(double currentSec) const;

        void setCurrentFrame(float currentFrame) const;

        // The shader the timeline is evaluated on top of. A PNG source grades from a neutral one,
        // so a picture reaches the encoder as it was drawn until a track moves something.
        [[nodiscard]] const ShaderAttribute &gradeBase() const;

        void setStatic(bool isStatic);

        void setMap(RFFBinary *normal, RFFBinary *zoomed);

        void applyCurrentStaticImage(const cv::Mat &normal, const cv::Mat &zoomed) const;

        void initRenderContext() const;

        void initRenderer();

        void applySize() const;

        VkExtent2D getBlurredImageExtent() const;

        void refreshSharedImgContext() const;

        void renderOnce() const;

        void renderOffscreenOnce() const;

        [[nodiscard]] const VideoRenderSceneRenderer &getRenderer() const {
            return *renderer;
        }

        [[nodiscard]] float calculateZoom(float defaultZoomIncrement, float currentFrame) const;

        void queueImage(int subsampleCount = 1, const std::function<bool()> &stopRequested = {});


        [[nodiscard]] std::mutex &getBufferCachedMutex() {
            return bufferCachedMutex;
        }

        [[nodiscard]] std::condition_variable &getBufferCachedCondition() {
            return bufferCachedCondition;
        }

        [[nodiscard]] std::queue<std::unique_ptr<VideoBufferCache>> &getQueuedBuffers() {
            return queuedVbc;
        }

        [[nodiscard]] std::wstring getPassTimingReport() const {
            return renderer->passTimer.report();
        }


        void init() override;

        void destroy() override;

    private:
        void updateReliefZoomForFrame() const;
    };
}
