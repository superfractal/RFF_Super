// Modified by GPT-5 on 2026-08-18

#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "../io/RFFDynamicMapBinary.h"
#include "../io/RFFStaticMapBinary.h"

namespace merutilm::rff2 {
    class VideoFrameSource final {
        std::filesystem::path directory;
        bool staticImages = false;
        uint32_t frameCount = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        int loadedPair = -1;
        float sampledDepth = 0.0f;
        RFFDynamicMapBinary normalDynamic = RFFDynamicMapBinary::DEFAULT;
        RFFDynamicMapBinary zoomedDynamic = RFFDynamicMapBinary::DEFAULT;
        RFFStaticMapBinary normalStatic = RFFStaticMapBinary::DEFAULT;
        RFFStaticMapBinary zoomedStatic = RFFStaticMapBinary::DEFAULT;
        cv::Mat normalImage;
        cv::Mat zoomedImage;

        VideoFrameSource(std::filesystem::path directory, bool staticImages, uint32_t frameCount,
                         uint32_t width, uint32_t height);

    public:
        static std::unique_ptr<VideoFrameSource> open(const std::filesystem::path &directory,
                                                      bool preferStatic, std::wstring &error);

        [[nodiscard]] bool load(float depth, std::wstring &error);

        [[nodiscard]] bool isStatic() const { return staticImages; }

        [[nodiscard]] uint32_t getFrameCount() const { return frameCount; }

        [[nodiscard]] uint32_t getWidth() const { return width; }

        [[nodiscard]] uint32_t getHeight() const { return height; }

        [[nodiscard]] float getSampledDepth() const { return sampledDepth; }

        [[nodiscard]] const std::filesystem::path &getDirectory() const { return directory; }

        [[nodiscard]] RFFDynamicMapBinary &getNormalDynamic() { return normalDynamic; }

        [[nodiscard]] RFFDynamicMapBinary &getZoomedDynamic() { return zoomedDynamic; }

        [[nodiscard]] RFFStaticMapBinary &getNormalStatic() { return normalStatic; }

        [[nodiscard]] RFFStaticMapBinary &getZoomedStatic() { return zoomedStatic; }

        [[nodiscard]] const cv::Mat &getNormalImage() const { return normalImage; }

        [[nodiscard]] const cv::Mat &getZoomedImage() const { return zoomedImage; }
    };
}
