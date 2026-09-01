// Modified by GPT-5 on 2026-08-18, 2026-09-01

#include "VideoFrameSource.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "../constants/Constants.hpp"
#include "../ui/IOUtilities.h"

namespace merutilm::rff2 {
    VideoFrameSource::VideoFrameSource(std::filesystem::path directory, const bool staticImages,
                                       const uint32_t frameCount, const uint32_t width, const uint32_t height) :
        directory(std::move(directory)), staticImages(staticImages), frameCount(frameCount), width(width), height(height) {
    }

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

    bool VideoFrameSource::load(const float depth, std::wstring &error) {
        const float top = static_cast<float>(frameCount);
        sampledDepth = std::min(depth, top);
        if (sampledDepth >= top) {
            sampledDepth = std::nextafter(top, -std::numeric_limits<float>::infinity());
        }
        const int pair = sampledDepth < 1.0f ? 0 : static_cast<int>(std::floor(sampledDepth));
        if (pair == loadedPair) {
            return true;
        }

        if (staticImages) {
            if (pair == 0) {
                normalStatic = RFFStaticMapBinary::readByID(directory, 1);
                zoomedStatic = RFFStaticMapBinary::DEFAULT;
                normalImage = RFFStaticMapBinary::loadImageByID(directory, 1);
                zoomedImage = cv::Mat::zeros(static_cast<int>(height), static_cast<int>(width), CV_16UC4);
            } else {
                zoomedStatic = RFFStaticMapBinary::readByID(directory, static_cast<uint32_t>(pair));
                normalStatic = RFFStaticMapBinary::readByID(directory, static_cast<uint32_t>(pair + 1));
                zoomedImage = RFFStaticMapBinary::loadImageByID(directory, static_cast<uint32_t>(pair));
                normalImage = RFFStaticMapBinary::loadImageByID(directory, static_cast<uint32_t>(pair + 1));
            }
            if (!normalStatic.hasData() || normalImage.empty() || (pair > 0 &&
                (!zoomedStatic.hasData() || zoomedImage.empty()))) {
                error = L"A required RFSM/PNG keyframe pair could not be read.";
                return false;
            }
            const auto hasExpectedSize = [this](const RFFStaticMapBinary &map, const cv::Mat &image) {
                return map.getWidth() == width && map.getHeight() == height &&
                       image.cols == static_cast<int>(width) && image.rows == static_cast<int>(height);
            };
            if (!hasExpectedSize(normalStatic, normalImage) ||
                (pair > 0 && !hasExpectedSize(zoomedStatic, zoomedImage))) {
                error = L"All RFSM/PNG keyframes must have the same dimensions.";
                return false;
            }
        } else {
            if (pair == 0) {
                normalDynamic = RFFDynamicMapBinary::readByID(directory, 1);
                zoomedDynamic = RFFDynamicMapBinary::DEFAULT;
            } else {
                zoomedDynamic = RFFDynamicMapBinary::readByID(directory, static_cast<uint32_t>(pair));
                normalDynamic = RFFDynamicMapBinary::readByID(directory, static_cast<uint32_t>(pair + 1));
            }
            if (!normalDynamic.hasData() || (pair > 0 && !zoomedDynamic.hasData())) {
                error = L"A required RFM/RFMZ keyframe pair could not be read.";
                return false;
            }
            const auto hasExpectedSize = [this](const RFFDynamicMapBinary &map) {
                return map.getMatrix().getWidth() == width && map.getMatrix().getHeight() == height;
            };
            if (!hasExpectedSize(normalDynamic) || (pair > 0 && !hasExpectedSize(zoomedDynamic))) {
                error = L"All RFM/RFMZ keyframes must have the same dimensions.";
                return false;
            }
        }
        loadedPair = pair;
        return true;
    }
}
