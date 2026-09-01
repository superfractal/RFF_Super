//
// Created by Merutilm on 2025-09-12.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-19.
//

#pragma once
#include "../../vulkan_helper/context/BufferContext.hpp"
#include "../../vulkan_helper/handle/CoreHandler.hpp"
#include "opencv2/core/mat.hpp"

namespace merutilm::rff2 {
    struct VideoBufferCache final : vkh::CoreHandler{
        vkh::BufferContext bufferContext;
        int width;
        int height;
        // rgba64le rather than BGR24, which is what the HDR encoder is fed.
        bool hdr;
        float zoom;
        int subsampleCount;
        cv::Mat image;

        explicit VideoBufferCache(vkh::CoreRef core, vkh::BufferContext &&ctx, const int width,
                                  const int height, const bool hdr, const float zoom, const int subsampleCount = 1) : CoreHandler(core), bufferContext(std::move(ctx)), width(width), height(height), hdr(hdr), zoom(zoom), subsampleCount(subsampleCount) {
            VideoBufferCache::init();
        }

        ~VideoBufferCache() override {
            VideoBufferCache::destroy();
        }

        VideoBufferCache(const VideoBufferCache &) = delete;

        VideoBufferCache &operator=(const VideoBufferCache &) = delete;

        VideoBufferCache(VideoBufferCache &&) = delete;

        VideoBufferCache &operator=(VideoBufferCache &&) = delete;


        void init() override {
            image = cv::Mat(height, width, hdr ? CV_16UC4 : CV_8UC3, bufferContext.mappedMemory);
        }

        void destroy() override {
            vkh::BufferContext::destroyContext(core, bufferContext);
        }
    };
}
