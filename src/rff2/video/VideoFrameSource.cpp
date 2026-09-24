//
// Modified by GPT-5 on 2026-08-18, 2026-09-01
// Modified by GPT-6 on 2026-09-15, 2026-09-21, 2026-09-23
//

#include "VideoFrameSource.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <opencv2/imgproc.hpp>

#include "../constants/Constants.hpp"
#include "../ui/IOUtilities.h"

namespace merutilm::rff2 {
    VideoFrameSource::VideoFrameSource(std::filesystem::path directory, const bool staticImages,
                                       const uint32_t frameCount, const uint32_t width, const uint32_t height)
        : directory(std::move(directory)), staticImages(staticImages), frameCount(frameCount), width(width),
          height(height) {}

    std::unique_ptr<VideoFrameSource> VideoFrameSource::open(const std::filesystem::path &directory,
                                                             const bool preferStatic, std::wstring &error) {
        const uint32_t dynamicCount = RFFDynamicMapBinary::keyframeCount(directory);
        const uint32_t staticCount = IOUtilities::fileNameCount(directory, Constants::Extension::STATIC_MAP);
        const bool useStatic = preferStatic ? staticCount > 0 : dynamicCount == 0 && staticCount > 0;
        const uint32_t count = useStatic ? staticCount : dynamicCount;
        if (count == 0) {
            error = L"No contiguous RFM/RFMZ or RFSM/PNG keyframes were found in this folder.";
            return nullptr;
        }

        if (useStatic) {
            const RFFStaticMapBinary first = RFFStaticMapBinary::readByID(directory, 1);
            if (!first.hasData()) {
                error = L"The first RFSM keyframe header could not be read.";
                return nullptr;
            }
            return std::unique_ptr<VideoFrameSource>(
                new VideoFrameSource(directory, true, count, first.getWidth(), first.getHeight()));
        }

        uint16_t width = 0;
        uint16_t height = 0;
        if (!RFFDynamicMapBinary::readSizeByID(directory, 1, width, height)) {
            error = L"The first RFM/RFMZ keyframe header could not be read.";
            return nullptr;
        }
        return std::unique_ptr<VideoFrameSource>(
            new VideoFrameSource(directory, false, count, width, height));
    }

    void VideoFrameSource::setPreviewSize(const uint32_t targetWidth, const uint32_t targetHeight) {
        const auto outputWidth = std::clamp(targetWidth, 1u, width);
        const auto outputHeight = std::clamp(targetHeight, 1u, height);
        if (previewWidth == outputWidth && previewHeight == outputHeight) {
            return;
        }
        previewWidth = outputWidth;
        previewHeight = outputHeight;
        cachedFrames.clear();
        normalFrame = std::make_shared<Frame>();
        zoomedFrame = std::make_shared<Frame>();
        loadedPair.reset();
    }

    uint64_t VideoFrameSource::previewCacheBytes() const {
        if (frameCount == 0) {
            return 0;
        }
        const uint64_t pixels =
            uint64_t(previewWidth ? previewWidth : width) * (previewHeight ? previewHeight : height);
        const uint64_t overhead = sizeof(Frame) + sizeof(std::shared_ptr<Frame>) + 64;
        const uint64_t maximumBytes = std::numeric_limits<uint64_t>::max();
        if (pixels > (maximumBytes - overhead) / 8) {
            return maximumBytes;
        }
        const uint64_t perFrameBytes = pixels * 8 + overhead;
        return frameCount > maximumBytes / perFrameBytes ? maximumBytes : perFrameBytes * frameCount;
    }

    std::shared_ptr<VideoFrameSource::Frame> VideoFrameSource::readFrame(const uint32_t id,
                                                                         std::wstring &error) const {
        auto frame = std::make_shared<Frame>();
        frame->id = id;
        const uint32_t outputWidth = previewWidth ? previewWidth : width;
        const uint32_t outputHeight = previewHeight ? previewHeight : height;
        if (staticImages) {
            frame->header = RFFStaticMapBinary::readByID(directory, id);
            frame->image = RFFStaticMapBinary::loadImageByID(directory, id);
            if (!frame->header.hasData() || frame->image.empty()) {
                error = L"A required RFSM/PNG keyframe pair could not be read.";
                return nullptr;
            }
            if (frame->header.getWidth() != width || frame->header.getHeight() != height ||
                frame->image.cols != static_cast<int>(width) ||
                frame->image.rows != static_cast<int>(height)) {
                error = L"All RFSM/PNG keyframes must have the same dimensions.";
                return nullptr;
            }
            if (outputWidth != width || outputHeight != height) {
                cv::resize(frame->image, frame->image,
                           cv::Size(static_cast<int>(outputWidth), static_cast<int>(outputHeight)), 0, 0,
                           cv::INTER_AREA);
            }
        } else {
            frame->dynamic = RFFDynamicMapBinary::readByID(directory, id);
            if (!frame->dynamic.hasData()) {
                error = L"A required RFM/RFMZ keyframe pair could not be read.";
                return nullptr;
            }
            const auto &source = frame->dynamic.getMatrix();
            if (source.getWidth() != width || source.getHeight() != height) {
                error = L"All RFM/RFMZ keyframes must have the same dimensions.";
                return nullptr;
            }
            if (outputWidth != width || outputHeight != height) {
                Matrix<double> reduced(static_cast<uint16_t>(outputWidth),
                                       static_cast<uint16_t>(outputHeight));
                for (uint32_t y = 0; y < outputHeight; ++y) {
                    const auto sourceY =
                        static_cast<uint16_t>((uint64_t(2 * y + 1) * height) / (2 * outputHeight));
                    for (uint32_t x = 0; x < outputWidth; ++x) {
                        const auto sourceX =
                            static_cast<uint16_t>((uint64_t(2 * x + 1) * width) / (2 * outputWidth));
                        reduced(static_cast<uint16_t>(x), static_cast<uint16_t>(y)) =
                            source(sourceX, sourceY);
                    }
                }
                const auto &map = frame->dynamic;
                frame->dynamic = RFFDynamicMapBinary(map.getLogZoom(), map.getPeriod(), map.getMaxIteration(),
                                                     std::move(reduced));
            }
        }
        return frame;
    }

    std::shared_ptr<VideoFrameSource::Frame> VideoFrameSource::getFrame(const uint32_t id,
                                                                        std::wstring &error) const {
        if (isPreloaded()) {
            return cachedFrames[id - 1];
        }
        if (normalFrame->id == id) {
            return normalFrame;
        }
        if (zoomedFrame->id == id) {
            return zoomedFrame;
        }
        return readFrame(id, error);
    }

    bool VideoFrameSource::preload(const uint64_t byteLimit, const std::stop_token stop,
                                   const std::function<void(uint32_t, uint32_t, uint64_t)> &progress,
                                   std::wstring &error) {
        error.clear();
        const uint64_t requiredBytes = previewCacheBytes();
        if (isPreloaded()) {
            if (progress) {
                progress(frameCount, frameCount, requiredBytes);
            }
            return true;
        }
        if (requiredBytes > byteLimit) {
            error = L"Not enough free RAM for all previews. Using on-demand loading.";
            return false;
        }
        try {
            std::vector<std::shared_ptr<Frame>> pendingFrames;
            pendingFrames.reserve(frameCount);
            if (progress) {
                progress(0, frameCount, requiredBytes);
            }
            for (uint32_t index = 0; index < frameCount; ++index) {
                const uint32_t id = index + 1;
                if (stop.stop_requested()) {
                    error = L"RAM preload canceled. Using on-demand loading.";
                    return false;
                }
                auto frame = getFrame(id, error);
                if (!frame) {
                    return false;
                }
                pendingFrames.push_back(std::move(frame));
                if (progress) {
                    progress(id, frameCount, requiredBytes);
                }
            }
            if (stop.stop_requested()) {
                error = L"RAM preload canceled. Using on-demand loading.";
                return false;
            }
            cachedFrames = std::move(pendingFrames);
            return true;
        } catch (const std::bad_alloc &) {
            error = L"Not enough free RAM for all previews. Using on-demand loading.";
            return false;
        } catch (const std::exception &) {
            error = L"RAM preload failed. Using on-demand loading.";
            return false;
        }
    }

    bool VideoFrameSource::load(const float depth, std::wstring &error) {
        if (!std::isfinite(depth)) {
            error = L"Preview depth must be finite.";
            return false;
        }
        const float maximumDepth = static_cast<float>(frameCount);
        sampledDepth = std::min(depth, maximumDepth);
        if (sampledDepth >= maximumDepth) {
            sampledDepth = std::nextafter(maximumDepth, -std::numeric_limits<float>::infinity());
        }
        const uint32_t pairIndex = sampledDepth < 1.0f ? 0 : static_cast<uint32_t>(std::floor(sampledDepth));
        if (pairIndex == loadedPair) {
            return true;
        }
        auto nextNormal = getFrame(static_cast<uint32_t>(pairIndex + 1), error);
        if (!nextNormal) {
            return false;
        }
        auto nextZoomed =
            pairIndex > 0 ? getFrame(static_cast<uint32_t>(pairIndex), error) : std::make_shared<Frame>();
        if (!nextZoomed) {
            return false;
        }
        if (pairIndex == 0 && staticImages) {
            nextZoomed->image =
                cv::Mat::zeros(static_cast<int>(previewHeight ? previewHeight : height),
                               static_cast<int>(previewWidth ? previewWidth : width), CV_16UC4);
        }
        normalFrame = std::move(nextNormal);
        zoomedFrame = std::move(nextZoomed);
        loadedPair = pairIndex;
        return true;
    }
}
