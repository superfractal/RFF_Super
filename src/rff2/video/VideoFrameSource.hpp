//
// Modified by GPT-5 on 2026-08-18
// Modified by GPT-6 on 2026-09-15, 2026-09-23
//

#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <functional>
#include <stop_token>
#include <optional>

#include "../io/RFFDynamicMapBinary.h"
#include "../io/RFFStaticMapBinary.h"

namespace merutilm::rff2 {
    class VideoFrameSource final {
        std::filesystem::path directory;
        bool staticImages = false;
        uint32_t frameCount = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t previewWidth = 0;
        uint32_t previewHeight = 0;
        std::optional<uint32_t> loadedPair;
        float sampledDepth = 0.0f;
        struct Frame {
            uint32_t id = 0;
            RFFDynamicMapBinary dynamic = RFFDynamicMapBinary::DEFAULT;
            RFFStaticMapBinary header = RFFStaticMapBinary::DEFAULT;
            cv::Mat image;
        };
        std::shared_ptr<Frame> normalFrame = std::make_shared<Frame>();
        std::shared_ptr<Frame> zoomedFrame = std::make_shared<Frame>();
        std::vector<std::shared_ptr<Frame>> cachedFrames;
        [[nodiscard]] std::shared_ptr<Frame> readFrame(uint32_t id, std::wstring &error) const;
        [[nodiscard]] std::shared_ptr<Frame> getFrame(uint32_t id, std::wstring &error) const;

        VideoFrameSource(std::filesystem::path directory, bool staticImages, uint32_t frameCount,
                         uint32_t width, uint32_t height);

    public:
        static std::unique_ptr<VideoFrameSource> open(const std::filesystem::path &directory,
                                                      bool preferStatic, std::wstring &error);

        void setPreviewSize(uint32_t width, uint32_t height);

        [[nodiscard]] uint64_t previewCacheBytes() const;
        [[nodiscard]] bool isPreloaded() const { return cachedFrames.size() == frameCount; }
        [[nodiscard]] std::optional<float> cachedLogZoom(uint32_t id) const {
            if (!isPreloaded() || id == 0 || id > frameCount) return {};
            const auto& frame = cachedFrames[id - 1];
            return staticImages ? frame->header.getLogZoom() : frame->dynamic.getLogZoom();
        }
        [[nodiscard]] bool preload(uint64_t byteLimit, std::stop_token stop,
            const std::function<void(uint32_t, uint32_t, uint64_t)> &progress, std::wstring &error);

        [[nodiscard]] bool load(float depth, std::wstring &error);

        [[nodiscard]] bool isStatic() const { return staticImages; }

        [[nodiscard]] uint32_t getFrameCount() const { return frameCount; }

        [[nodiscard]] uint32_t getWidth() const { return width; }

        [[nodiscard]] uint32_t getHeight() const { return height; }

        [[nodiscard]] float getSampledDepth() const { return sampledDepth; }

        [[nodiscard]] const std::filesystem::path &getDirectory() const { return directory; }

        [[nodiscard]] RFFDynamicMapBinary &getNormalDynamic() { return normalFrame->dynamic; }

        [[nodiscard]] RFFDynamicMapBinary &getZoomedDynamic() { return zoomedFrame->dynamic; }

        [[nodiscard]] RFFStaticMapBinary &getNormalStatic() { return normalFrame->header; }

        [[nodiscard]] RFFStaticMapBinary &getZoomedStatic() { return zoomedFrame->header; }

        [[nodiscard]] const cv::Mat &getNormalImage() const { return normalFrame->image; }

        [[nodiscard]] const cv::Mat &getZoomedImage() const { return zoomedFrame->image; }
    };
}
