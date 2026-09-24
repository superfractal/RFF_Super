//
// Created by Merutilm on 2025-09-03.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include "../core/vkh_base.hpp"
#include "ImageContext.hpp"

namespace merutilm::vkh {
    class SharedImageContextImpl final {
        CoreRef core;
        std::vector<MultiframeImageContext> multiframeContexts = {};
        std::vector<ImageContext> imageContexts = {};

    public:
        explicit SharedImageContextImpl(CoreRef core) : core(core) {
        }

        ~SharedImageContextImpl() {
            cleanupContexts();
        }

        SharedImageContextImpl(const SharedImageContextImpl &) = delete;

        SharedImageContextImpl &operator=(const SharedImageContextImpl &) = delete;

        SharedImageContextImpl(SharedImageContextImpl &&) = delete;

        SharedImageContextImpl &operator=(SharedImageContextImpl &&) = delete;

        void appendMultiframeImageContext(const uint32_t expected, ImageInitInfo &&iii) {
            safe_array::check_index_equal(expected, multiframeContexts.size(),
                                          "Shared Multiframe Image Context append");
            multiframeContexts.reserve(multiframeContexts.size() + 1);
            multiframeContexts.emplace_back(ImageContext::createMultiframeContext(core, iii));
        }

        void appendImageContext(const uint32_t expected, ImageInitInfo &&iii) {
            safe_array::check_index_equal(expected, imageContexts.size(), "Shared Image Context append");
            imageContexts.reserve(imageContexts.size() + 1);
            imageContexts.emplace_back(ImageContext::createContext(core, iii));
        }

        MultiframeImageContext &getImageContextMF(const uint32_t index) {
            return multiframeContexts[index];
        }

        ImageContext &getImageContext(const uint32_t index) {
            return imageContexts[index];
        }

        void cleanupContexts() {
            for (const auto &context: multiframeContexts) {
                ImageContext::destroyContext(core, context);
            }
            for (const auto &context: imageContexts) {
                ImageContext::destroyContext(core, context);
            }
            multiframeContexts.clear();
            imageContexts.clear();
        }
    };

    using SharedImageContext = std::unique_ptr<SharedImageContextImpl>;
    using SharedImageContextPtr = SharedImageContextImpl *;
    using SharedImageContextRef = SharedImageContextImpl &;
}
