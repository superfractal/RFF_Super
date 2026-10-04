//
// Created by Merutilm on 2025-06-23.
// Modified by GPT-6 on 2026-09-23, 2026-09-26
//

#pragma once
#include <cstdint>
#include <optional>
#include <filesystem>

#include "RFFBinary.h"
#include "opencv2/core/mat.hpp"

namespace merutilm::rff2 {
    class RFFStaticMapBinary final : public RFFBinary {
        uint32_t width;
        uint32_t height;
        std::optional<uint64_t> maxIteration;

    public:
        static const RFFStaticMapBinary DEFAULT;

        explicit RFFStaticMapBinary(float logZoom, uint32_t width, uint32_t height,
                                    std::optional<uint64_t> maxIteration = {});

        [[nodiscard]] bool hasData() const override;

        [[nodiscard]] static RFFStaticMapBinary read(const std::filesystem::path &path);

        [[nodiscard]] static cv::Mat loadImageByID(const std::filesystem::path &dir, uint32_t id);

        [[nodiscard]] static RFFStaticMapBinary readByID(const std::filesystem::path &dir, uint32_t id);

        [[nodiscard]] uint32_t getWidth() const;

        [[nodiscard]] uint32_t getHeight() const;

        [[nodiscard]] std::optional<uint64_t> getMaxIteration() const { return maxIteration; }

        void exportAsKeyframe(const std::filesystem::path &dir) const override;

        void exportFile(const std::filesystem::path &path) const override;
    };
}
