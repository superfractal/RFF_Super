//
// Created by Merutilm on 2025-09-06.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-07-09, 2026-08-21, 2026-08-23, 2026-08-27
// Modified by Opus 5 on 2026-08-05, 2026-08-07, 2026-08-10, 2026-08-12, 2026-08-13, 2026-08-15, 2026-08-17, 2026-08-19, 2026-08-25, 2026-08-26, 2026-08-31
// Modified by GPT-6 on 2026-09-08, 2026-09-10, 2026-09-11, 2026-09-13, 2026-09-15, 2026-09-16, 2026-09-20, 2026-09-21, 2026-09-22, 2026-09-23
//

#include "VideoRenderScene.hpp"

#include "../../vulkan_helper/util/BufferImageContextUtils.hpp"
#include "../../vulkan_helper/util/BarrierUtils.hpp"
#include "../vulkan/RCCPresentVid.hpp"
#include "opencv2/imgproc.hpp"
#include "../constants/FractalConstants.hpp"
#include "../constants/VideoConstants.hpp"

#include "../attr/VidTimelineTarget.h"
#include "../preset/shader/bloom/ShdBloomPresets.h"
#include "../preset/shader/color/ShdColorPresets.h"
#include "../preset/shader/fog/ShdFogPresets.h"
#include "../preset/shader/slope/ShdSlopePresets.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <stdexcept>

namespace merutilm::rff2 {
    namespace {
        // Escape hatch for A/B timing: RFF_GPU_SSAA_DOWNSAMPLE=0 restores the old CPU-side cv::resize path.
        bool gpuDownsampleEnabled() {
            static const bool enabled = [] {
                const char *v = std::getenv("RFF_GPU_SSAA_DOWNSAMPLE");
                return v == nullptr || (v[0] != '0' || v[1] != '\0');
            }();
            return enabled;
        }

        // Texture layers the timeline can still call for: one a track turns on, and one a track can
        // point Warp at, including the source already set for a Warp a track only has to enable.
        // Images are read off disk once, before the first frame, so a layer that is only reached
        // later has to be uploaded with the rest or it is never there to draw.
        uint32_t timelineResidentTextures(const VidTimelineAttribute &timeline,
                                          const ShdWarpAttribute &warp) {
            if (!timeline.enabled) {
                return 0u;
            }
            constexpr uint16_t ENABLED_IDS[TEXTURE_LAYER_COUNT] = {
                vidTimelineTargetId(VidTimelineTarget::TEXTURE_1_ENABLED),
                vidTimelineTargetId(VidTimelineTarget::TEXTURE_2_ENABLED),
                vidTimelineTargetId(VidTimelineTarget::TEXTURE_3_ENABLED),
                vidTimelineTargetId(VidTimelineTarget::TEXTURE_4_ENABLED),
            };
            uint32_t mask = 0u;
            bool warpTracked = false;
            for (const auto &track: timeline.tracks) {
                if (!track.enabled || track.keys.empty()) {
                    continue;
                }
                for (uint32_t layer = 0; layer < TEXTURE_LAYER_COUNT; ++layer) {
                    if (track.targetId != ENABLED_IDS[layer]) {
                        continue;
                    }
                    for (const auto &key: track.keys) {
                        if (key.value >= 0.5f) {
                            mask |= 1u << layer;
                            break;
                        }
                    }
                }
                if (track.targetId == vidTimelineTargetId(VidTimelineTarget::WARP_ENABLED)) {
                    warpTracked = true;
                }
                if (track.targetId == vidTimelineTargetId(VidTimelineTarget::WARP_SOURCE)) {
                    warpTracked = true;
                    for (const auto &key: track.keys) {
                        const long source = std::lround(key.value);
                        if (source >= 1 && source <= static_cast<long>(TEXTURE_LAYER_COUNT)) {
                            mask |= 1u << (source - 1);
                        }
                    }
                }
            }
            if (warpTracked && warp.source != ShdWarpSource::NOISE) {
                mask |= 1u << (static_cast<uint32_t>(warp.source) - 1u);
            }
            return mask;
        }

        // A PNG keyframe is a finished picture: the stripe and the relief are read from iteration
        // data it does not carry, and the grade it was drawn with is already in it. So the chain
        // opens neutral over a PNG and what the timeline drives is laid on that - a fade, an
        // exposure ramp, a haze that comes and goes over the pictures - rather than a second
        // helping of the settings they were drawn with. With no track at all the picture leaves
        // the chain as it arrived, which is what the source did before the chain ran on it.
        ShaderAttribute staticGradeBase(const ShaderAttribute &shader) {
            ShaderAttribute out = shader;
            out.slope = ShdSlopePresets::Disabled().genSlope();
            out.color = ShdColorPresets::Disabled().genColor();
            out.fog = ShdFogPresets::Disabled().genFog();
            out.bloom = BloomPresets::Disabled().genBloom();
            return out;
        }

        bool sameAnimationTimeline(const VidTimelineAttribute &a, const VidTimelineAttribute &b) {
            if (a.enabled != b.enabled || a.tracks.size() != b.tracks.size() || a.holds.size() != b.holds.size()) return false;
            for (size_t i = 0; i < a.tracks.size(); ++i) {
                const auto &left = a.tracks[i];
                const auto &right = b.tracks[i];
                if (left.targetId != right.targetId || left.enabled != right.enabled ||
                    left.keys.size() != right.keys.size()) return false;
                for (size_t k = 0; k < left.keys.size(); ++k) {
                    const auto &x = left.keys[k];
                    const auto &y = right.keys[k];
                    if (x.depth != y.depth || x.value != y.value || x.out != y.out ||
                        x.color.r != y.color.r || x.color.g != y.color.g ||
                        x.color.b != y.color.b || x.color.a != y.color.a) return false;
                }
            }
            for (size_t i = 0; i < a.holds.size(); ++i) {
                if (a.holds[i].depth != b.holds[i].depth || a.holds[i].seconds != b.holds[i].seconds) return false;
            }
            return true;
        }
    }

    VideoRenderScene::VideoRenderScene(vkh::EngineRef engine, vkh::WindowContextRef wc, const VkExtent2D &videoExtent,
                                       const Attribute &targetAttribute, std::function<void()> beforeWait) : EngineHandler(engine), wc(wc), preparationHook(std::move(beforeWait)),
                                                                           videoExtent(videoExtent),
                                                                           baseAttribute(targetAttribute),
                                                                           staticShader(staticGradeBase(targetAttribute.shader)),
                                                                           liveShader(targetAttribute.shader),
                                                                           timelineEvaluator(targetAttribute.video.timeline) {
        VideoRenderScene::init();
    }

    VideoRenderScene::~VideoRenderScene() {
        VideoRenderScene::destroy();
    }


    void VideoRenderScene::applyCurrentDynamicMap(const RFFDynamicMapBinary &normal,
                                                  const RFFDynamicMapBinary &zoomed, const float currentFrame) const {
        wc.core.getLogicalDevice().waitDeviceIdle();
        auto &normalI = normal.getMatrix();
        if (currentFrame < 1) {
            const std::vector<double> zoomedDefault(normalI.getLength());
            renderer->renderer2MapIterationStripe->setAllIterations(normalI.getCanvas(), zoomedDefault);
        } else {
            auto &zoomedI = zoomed.getMatrix();
            renderer->renderer2MapIterationStripe->setAllIterations(normalI.getCanvas(), zoomedI.getCanvas());
        }
    }

    void VideoRenderScene::setMaxIterationDynamic(const double maxIteration, const double normalMaxIteration,
                                                  const double zoomedMaxIteration) const {
        renderer->renderer2MapIterationStripe->setInfo(maxIteration, normalMaxIteration, zoomedMaxIteration);
    }

    void VideoRenderScene::setSampleJitter(const float jitterX, const float jitterY) const {
        renderer->renderer2MapIterationStripe->setSampleJitter(jitterX, jitterY);
    }

    const ShaderAttribute &VideoRenderScene::gradeBase() const {
        return renderer != nullptr && renderer->isStaticImages ? staticShader : baseAttribute.shader;
    }

    void VideoRenderScene::applyShaderStatic() const {
        engine.getCore().getLogicalDevice().waitDeviceIdle();
        renderer->renderer2MapIterationStripe->setPalette(baseAttribute.shader.palette);
        renderer->renderer2MapIterationStripe->set2MapSize(videoExtent);
        renderer->renderer2MapIterationStripe->setDefaultZoomIncrement(
            baseAttribute.video.data.defaultZoomIncrement);
        renderer->renderer2MapIterationStripe->setSampleJitter(0.0f, 0.0f);
        renderer->renderer2MapIterationStripe->setTextures(baseAttribute.shader.textures,
                                                           warpSourceLayer(baseAttribute.shader.warp),
                                                           timelineResidentTextures(baseAttribute.video.timeline,
                                                               baseAttribute.shader.warp));
        renderer->rendererBloom->setBloom(gradeBase().bloom, baseAttribute.shader.hdr, gradeBase().sceneLinear());
        renderer->layerShader = gradeBase();
        renderer->rendererSlope->setEffects(gradeBase().effects, gradeBase().sceneLinear(), !renderer->isStaticImages);
        renderer->renderer2MapIterationStripe->setEffects(gradeBase().effects);
        applyShaderDynamic(gradeBase(), TimelineDirtyMask::ALL);
        renderer->rendererLinearInterpolation->setLinearInterpolation(baseAttribute.render.linearInterpolation);
        renderer->renderer2MapIterationStripe->setDither(baseAttribute.render.dither);
        renderer->rendererLinearInterpolation->setDither(baseAttribute.render.dither);
        renderer->rendererImageRGBA2BGR->setDownsample(
            gpuDownsampleEnabled() ? baseAttribute.render.ssaa : 1);
        // A preview window is an SDR display; only an export that asks for it moves off this.
        setHdrOutput(VidHdrTransfer::SDR, baseAttribute.video.exportation.hdrPeakNits);
    }

    void VideoRenderScene::setHdrOutput(const VidHdrTransfer transfer, const float peakNits) const {
        renderer->rendererLinearInterpolation->setToneMap(baseAttribute.shader.hdr, transfer, peakNits, gradeBase().sceneLinear());
        renderer->rendererImageRGBA2BGR->setHdr(isHdrOutput(transfer));
    }

    bool VideoRenderScene::isHdrOutput(const VidHdrTransfer transfer) const {
        return baseAttribute.shader.hdr.use && transfer != VidHdrTransfer::SDR;
    }

    void VideoRenderScene::applyShaderDynamic(const ShaderAttribute &shader, const TimelineDirtyMask dirty) const {
        renderer->layerShader = shader;
        if (hasTimelineDirty(dirty, TimelineDirtyMask::CAMERA)) {
            renderer->renderer2MapIterationStripe->setCamera(shader.camera, baseAttribute.video.data.sourceScale);
        }
        if (hasTimelineDirty(dirty, TimelineDirtyMask::PALETTE)) {
            renderer->renderer2MapIterationStripe->setPaletteDynamic(shader.palette);
            renderer->rendererSlope->setGroove(shader.palette);
        }
        if (hasTimelineDirty(dirty, TimelineDirtyMask::STRIPE)) {
            renderer->renderer2MapIterationStripe->setStripe(shader.stripe);
        }
        if (hasTimelineDirty(dirty, TimelineDirtyMask::TEXTURE)) {
            renderer->renderer2MapIterationStripe->setTextureParams(shader.textures);
        }
        if (hasTimelineDirty(dirty, TimelineDirtyMask::PATTERN)) {
            renderer->renderer2MapIterationStripe->setPattern(shader.patterns);
        }
        if (hasTimelineDirty(dirty, TimelineDirtyMask::WARP)) {
            renderer->renderer2MapIterationStripe->setWarp(shader.warp);
        }
        if (hasTimelineDirty(dirty, TimelineDirtyMask::SLOPE)) {
            ShdSlopeAttribute slope = shader.slope;
            // Held off rather than trusted to be off: the relief is read from the iteration buffer,
            // which a PNG source never fills, and the shader only leaves it alone while the layer's
            // own off switches are down. A slope track on a PNG video moves nothing, as before.
            if (renderer->isStaticImages) {
                slope.depth = 0.0f;
                slope.opacity = 0.0f;
            }
            renderer->rendererSlope->setSlope(slope);
            renderer->rendererLinearInterpolation->setSurface(slope);
        }
        if (hasTimelineDirty(dirty, TimelineDirtyMask::COLOR)) {
            renderer->rendererColor->setColor(shader.color, gradeBase().sceneLinear());
        }
        if (hasTimelineDirty(dirty, TimelineDirtyMask::FOG)) {
            ShdFogAttribute fog = shader.fog;
            // The rim mask reads the relief and the focus band reads the iteration count, neither of
            // which a PNG carries. The rest of the fog is laid on the picture itself and stands.
            if (renderer->isStaticImages) {
                fog.rimMask = 0.0f;
                fog.focusAmount = 0.0f;
                fog.chaosAmount = 0.0f;
            }
            renderer->rendererFog->setFog(fog);
            renderer->rendererBoxBlur->setBlurInfo(CPCBoxBlur::DESC_INDEX_BLUR_TARGET_FOG, fog.radius);
        }
        if (hasTimelineDirty(dirty, TimelineDirtyMask::BLOOM)) {
            renderer->rendererBloom->setBloomDynamic(shader.bloom, shader.hdr, gradeBase().sceneLinear());
            renderer->rendererBoxBlur->setBlurInfo(CPCBoxBlur::DESC_INDEX_BLUR_TARGET_BLOOM, shader.bloom.radius);
        }
    }

    void VideoRenderScene::applyTimelineShader(const float depth, const float sec) {
        if (timelineEvaluator.hasActiveShaderTracks()) {
            ShaderAttribute evaluated = {};
            timelineEvaluator.evaluate(depth, sec, gradeBase(), evaluated);
            const TimelineDirtyMask dirty = timelineEvaluator.diff(liveShader, evaluated);
            if (dirty != TimelineDirtyMask::NONE) {
                applyShaderDynamic(evaluated, dirty);
                liveShader = std::move(evaluated);
            }
        }
        if (animationIntegrator) {
            renderer->renderer2MapIterationStripe->setTimelinePhases(animationIntegrator->at(sec), sec);
        } else {
            renderer->renderer2MapIterationStripe->advanceAnimationTo(sec);
        }
    }

    void VideoRenderScene::updateBase(const ShaderAttribute &shader, const VidTimelineAttribute &timeline) {
        // The shader the timeline is evaluated on top of, refreshed from the session so a setting
        // changed while the editor is open reaches its next preview. The 2map size and the sample
        // jitter are left out: they belong to the render, not to the shader, and resizing the
        // iteration buffer here would throw away the keyframe already uploaded into it.
        animationInputsChanged |= !sameAnimationTimeline(baseAttribute.video.timeline, timeline) ||
                                  TimelineAnimationPhases::ratesOf(baseAttribute.shader) !=
                                  TimelineAnimationPhases::ratesOf(shader);
        engine.getCore().getLogicalDevice().waitDeviceIdle();
        const bool rebuildHdrChain = renderer == nullptr || baseAttribute.shader.hdr.use != shader.hdr.use ||
                                     baseAttribute.shader.sceneLinear() != shader.sceneLinear();
        baseAttribute.shader = shader;
        staticShader = staticGradeBase(shader);
        baseAttribute.video.timeline = timeline;
        timelineEvaluator = TimelineEvaluator(timeline);
        liveShader = gradeBase();
        if (rebuildHdrChain) {
            renderer.reset();
            refreshSharedImgContext();
            for (const auto &context: wc.getRenderContexts()) {
                context->recreate();
            }
            initRenderer();
            return;
        }
        renderer->renderer2MapIterationStripe->setPalette(shader.palette);
        renderer->renderer2MapIterationStripe->setTextures(shader.textures, warpSourceLayer(shader.warp),
                                                           timelineResidentTextures(baseAttribute.video.timeline,
                                                               shader.warp));
        renderer->rendererBloom->setBloom(gradeBase().bloom, shader.hdr, gradeBase().sceneLinear());
        renderer->layerShader = gradeBase();
        renderer->rendererSlope->setEffects(gradeBase().effects, gradeBase().sceneLinear(), !renderer->isStaticImages);
        renderer->renderer2MapIterationStripe->setEffects(gradeBase().effects);
        applyShaderDynamic(liveShader, TimelineDirtyMask::ALL);
        setHdrOutput(VidHdrTransfer::SDR, baseAttribute.video.exportation.hdrPeakNits);
    }

    void VideoRenderScene::setTimelineSchedule(const TimelineSchedule &schedule) {
        const std::array<float, 3> key{schedule.getStartDepth(), schedule.getEndDepth(),
                                       schedule.getTotalSeconds()};
        if (!animationIntegrator || animationInputsChanged || animationScheduleKey != key) {
            animationIntegrator = std::make_unique<TimelineAnimationPhases>(schedule,
                baseAttribute.video.timeline, gradeBase());
            animationScheduleKey = key;
            animationInputsChanged = false;
        }
    }

    void VideoRenderScene::setTime(const float currentSec) const {
        renderer->currentSec = currentSec;
    }


    void VideoRenderScene::setCurrentFrame(const float currentFrame) const {
        renderer->currentFrame = currentFrame;
    }

    void VideoRenderScene::setStatic(const bool isStatic) {
        if (renderer->isStaticImages == isStatic) {
            return;
        }
        // Which source it is picks the base the chain grades from, so the values already written
        // into the pipelines are the other source's and are put right here. Reported once per
        // change and not once per frame, which is what makes the device wait below affordable.
        renderer->isStaticImages = isStatic;
        animationInputsChanged = true;
        applyShaderStatic();
    }

    void VideoRenderScene::setMap(RFFBinary *normal, RFFBinary *zoomed) {
        this->normal = normal;
        this->zoomed = zoomed;
    }

    void VideoRenderScene::applyCurrentStaticImage(const cv::Mat &normal, const cv::Mat &zoomed) const {
        wc.core.getLogicalDevice().waitDeviceIdle();
        renderer->rendererStaticImage->setImages(normal, zoomed);
    }

    void VideoRenderScene::initRenderContext() const {
        const auto swapchainImageContextGetter = [this] {
            auto &swapchain = wc.getSwapchain();
            return vkh::ImageContext::fromSwapchain(swapchain);
        };
        wc.attachRenderContext<RCC1Vid>(wc.core,
                                        [this] { return videoExtent; },
                                        swapchainImageContextGetter);
        wc.attachRenderContext<RCCDownsampleForBlurVid>(wc.core,
                                                        [this] { return getBlurredImageExtent(); },
                                                        swapchainImageContextGetter);
        wc.attachRenderContext<RCC2Vid>(wc.core,
                                        [this] { return videoExtent; },
                                        swapchainImageContextGetter);
        wc.attachRenderContext<RCC3Vid>(wc.core,
                                        [this] { return videoExtent; },
                                        swapchainImageContextGetter);
        wc.attachRenderContext<RCC4Vid>(wc.core,
                                        [this] { return videoExtent; },
                                        swapchainImageContextGetter);
        wc.attachRenderContext<RCCPresentVid>(wc.core,
                                              [this] { return wc.getSwapchain().populateSwapchainExtent(); },
                                              swapchainImageContextGetter);
        wc.attachRenderContext<RCCStatic2Image>(wc.core,
                                                [this] { return videoExtent; },
                                                swapchainImageContextGetter);
    }

    void VideoRenderScene::initRenderer() {
        renderer = std::make_unique<VideoRenderSceneRenderer>(engine, wc.getAttachmentIndex(),
                                                             baseAttribute.shader.hdr.use || baseAttribute.shader.sceneLinear(), preparationHook,
                                                             &baseAttribute.shader, baseAttribute.render.dither);
        preparationHook = {};
        applySize();
        applyShaderStatic();
    }

    void VideoRenderScene::applySize() const {
        auto [sWidth, sHeight] = wc.getSwapchain().populateSwapchainExtent();
        auto [bWidth, bHeight] = getBlurredImageExtent();

        for (const auto &sp: renderer->configurators) {
            sp->renderContextRefreshed();
        }

        renderer->rendererDownsampleForBlur->setRescaledResolution(0, {bWidth, bHeight});
        renderer->rendererDownsampleForBlur->setRescaledResolution(1, {bWidth, bHeight});
        renderer->rendererPresent->setRescaledResolution({sWidth, sHeight});
    }

    VkExtent2D VideoRenderScene::getBlurredImageExtent() const {
        if (const float rat = Constants::Fractal::GAUSSIAN_MAX_WIDTH / static_cast<float>(videoExtent.width); rat < 1) {
            return {
                Constants::Fractal::GAUSSIAN_MAX_WIDTH,
                std::max(1u, static_cast<uint32_t>(static_cast<float>(videoExtent.height) * rat))
            };
        }
        return videoExtent;
    }


    void VideoRenderScene::refreshSharedImgContext() const {
        using namespace SharedImageContextIndices;

        auto &sharedImg = wc.getSharedImageContext();
        sharedImg.cleanupContexts();

        auto iiiGetter = [](const VkExtent2D extent, const VkFormat format, const VkImageUsageFlags usage) {
            return vkh::ImageInitInfo{
                .imageType = VK_IMAGE_TYPE_2D,
                .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
                .imageFormat = format,
                .extent = VkExtent3D{extent.width, extent.height, 1},
                .useMipmap = VK_FALSE,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .imageTiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = usage,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            };
        };
        const auto blurredImageExtent = getBlurredImageExtent();
        // An SDR export grades in the 8-bit images it always has, down to the bit. HDR cannot be packed
        // into them, so only that path widens - and the merged-image shader is chosen to match.
        const VkFormat gradingFormat = (baseAttribute.shader.hdr.use || baseAttribute.shader.sceneLinear())
                                           ? VK_FORMAT_R16G16B16A16_SFLOAT
                                           : VK_FORMAT_R8G8B8A8_UNORM;

        sharedImg.appendMultiframeImageContext(MF_VIDEO_RENDER_IMAGE_PRIMARY,
                                               iiiGetter(videoExtent, gradingFormat,
                                                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                         VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                                                         VK_IMAGE_USAGE_SAMPLED_BIT |
                                                         VK_IMAGE_USAGE_STORAGE_BIT));
        sharedImg.appendMultiframeImageContext(MF_VIDEO_RENDER_IMAGE_SECONDARY,
                                               iiiGetter(videoExtent, gradingFormat,
                                                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                                         VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                                                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                         VK_IMAGE_USAGE_SAMPLED_BIT |
                                                         VK_IMAGE_USAGE_STORAGE_BIT));
        sharedImg.appendMultiframeImageContext(MF_VIDEO_RENDER_DOWNSAMPLED_IMAGE_PRIMARY,
                                               iiiGetter(blurredImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT,
                                                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                         VK_IMAGE_USAGE_SAMPLED_BIT |
                                                         VK_IMAGE_USAGE_STORAGE_BIT));
        sharedImg.appendMultiframeImageContext(MF_VIDEO_RENDER_DOWNSAMPLED_IMAGE_SECONDARY,
                                               iiiGetter(blurredImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT,
                                                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                         VK_IMAGE_USAGE_SAMPLED_BIT |
                                                         VK_IMAGE_USAGE_STORAGE_BIT));
    }


    void VideoRenderScene::updateReliefZoomForFrame() const {
        if (!renderer->isStaticImages && normal && zoomed) {
            auto slope = liveShader.slope;
            if (slope.reliefZoomReference < 0.0f) slope.reliefZoomReference = baseAttribute.fractal.logZoom;
            renderer->rendererSlope->setReliefZoom(slope, calculateZoom(baseAttribute.video.data.defaultZoomIncrement, renderer->currentFrame));
        }
    }

    void VideoRenderScene::renderOnce() const {
        updateReliefZoomForFrame();
        bool submitted = false;
        const bool recreate = renderer->execute(&submitted);
        if (!submitted) {
            renderer->executeOffscreen();
        }
        if (recreate && !wc.getWindow().isUnrenderable()) {
            const auto [surfaceWidth, surfaceHeight] = wc.getSwapchain().populateSwapchainExtent();
            const auto [swapchainWidth, swapchainHeight] = wc.getSwapchain().getCurrentExtent();
            if (!submitted || surfaceWidth != swapchainWidth || surfaceHeight != swapchainHeight) {
                wc.core.getLogicalDevice().waitDeviceIdle();
                wc.getSwapchain().recreate();
                wc.getRenderContext(RCCPresentVid::CONTEXT_INDEX).recreate();
                renderer->rendererPresent->renderContextRefreshed();
                renderer->rendererPresent->setRescaledResolution({surfaceWidth, surfaceHeight});
            }
        }
    }

    void VideoRenderScene::renderOffscreenOnce() const {
        updateReliefZoomForFrame();
        renderer->executeOffscreen();
    }

    float VideoRenderScene::calculateZoom(const float defaultZoomIncrement, const float currentFrame) const {
        if (currentFrame < 1) {
            const float r = 1 - currentFrame;

            if (!normal->hasData()) {
                return 0;
            }

            const float z1 = normal->getLogZoom();
            return std::lerp(z1, z1 + std::log10(defaultZoomIncrement), r);
        }
        const auto f1 = static_cast<int>(currentFrame); // it is smaller
        const auto f2 = f1 + 1;
        //frame size : f1 = 1x, f2 = 2x
        const float r = static_cast<float>(f2) - currentFrame;

        if (!zoomed->hasData() || !normal->hasData()) {
            return 0;
        }

        const float z1 = zoomed->getLogZoom();
        const float z2 = normal->getLogZoom();
        return std::lerp(z2, z1, r);
    }


    void VideoRenderScene::queueImage(const int subsampleCount, const std::function<bool()> &stopRequested) {
        // renderOnce() only records and submits, so the whole shader chain is paid for in this fence wait.

        const uint32_t frameIndex = renderer->getFrameIndex();
        wc.getSyncObject().getFence(frameIndex).wait();
        renderer->passTimer.collect();

        const vkh::BufferContext &srcBuffer = renderer->rendererImageRGBA2BGR->getBufferContext(frameIndex);
        vkh::BufferContext dstBuffer = vkh::BufferContext::createContext(wc.core, {
                                                                             .size = srcBuffer.bufferSize,
                                                                             .usage =
                                                                             VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                                             .properties =
                                                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                                         });

        bool copySubmitted = false;
        bool bufferTransferred = false;
        try {
            vkh::BufferContext::mapMemory(wc.core, dstBuffer);

            {
                vkh::ScopedCommandBufferExecutor executor(wc, frameIndex, VK_NULL_HANDLE, VK_NULL_HANDLE);
                vkh::BufferImageContextUtils::cmdCopyBuffer(wc.getCommandBuffer().getCommandBufferHandle(frameIndex),
                                                            srcBuffer, dstBuffer);
                vkh::BarrierUtils::cmdBufferMemoryBarrier(
                    wc.getCommandBuffer().getCommandBufferHandle(frameIndex), VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_ACCESS_HOST_READ_BIT, dstBuffer.buffer, 0, dstBuffer.bufferSize,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT);
                executor.finish();
                copySubmitted = true;
            }

            // Not unmapped here: VideoBufferCache wraps dstBuffer.mappedMemory in the cv::Mat the resolve thread reads, and vkFreeMemory in destroyContext unmaps it once that Mat is gone.
            wc.getSyncObject().getFence(frameIndex).wait();
            copySubmitted = false;

            std::unique_lock queueLock(bufferCachedMutex);
            while (queuedVbc.size() >= Constants::VideoConfig::MAX_VIDEO_QUEUE_SIZE &&
                   !(stopRequested && stopRequested())) {
                bufferCachedCondition.wait_for(queueLock, std::chrono::milliseconds(100));
            }
            if (stopRequested && stopRequested()) {
                throw std::runtime_error("Video export stopped while waiting for the encoder.");
            }

            // Already SSAA-downsampled by the RGBA2BGR pass, so the cached image is output-sized.
            const auto &[outWidth, outHeight] = renderer->rendererImageRGBA2BGR->getOutputExtent();
            auto cachedBuffer = std::make_unique<VideoBufferCache>(wc.core, std::move(dstBuffer),
                                                                   static_cast<int>(outWidth),
                                                                   static_cast<int>(outHeight),
                                                                   renderer->rendererImageRGBA2BGR->isHdr(),
                                                                   calculateZoom(
                                                                       baseAttribute.video.data.defaultZoomIncrement,
                                                                       renderer->currentFrame),
                                                                   subsampleCount);
            bufferTransferred = true;
            queuedVbc.push(std::move(cachedBuffer));
        } catch (...) {
            if (copySubmitted) {
                const VkResult result = wc.core.getLogicalDevice().waitDeviceIdle();
                if (result != VK_SUCCESS && result != VK_ERROR_DEVICE_LOST) {
                    std::terminate();
                }
            }
            if (!bufferTransferred) {
                vkh::BufferContext::destroyContext(wc.core, dstBuffer);
            }
            throw;
        }
    }

    void VideoRenderScene::init() {
        refreshSharedImgContext();
        initRenderContext();
        initRenderer();
    }

    void VideoRenderScene::destroy() {
        engine.getCore().getLogicalDevice().waitDeviceIdle();
    }
}
