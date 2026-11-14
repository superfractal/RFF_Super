//
// Created by Opus 5.5 on 2026-11-07
// Modified by Opus 5.5 on 2026-11-10, 2026-11-11
//

#pragma once
#include <cstdint>
#include <filesystem>
#include <opencv2/core/mat.hpp>

namespace merutilm::rff2 {
    // The small pictures the Gallery shows. A saved file's thumbnail is a PNG beside it, named after
    // it with the .rfth extension, so the file itself stays exactly what older releases can read.
    struct ThumbnailIO {
        ThumbnailIO() = delete;

        // The thumbnail beside file: same folder, same name, .rfth in place of its extension.
        [[nodiscard]] static std::filesystem::path pathFor(const std::filesystem::path &file);

        // Shrinks picture (BGR or BGRA, 8 or 16 bit) to width pixels across and writes it as a PNG.
        static bool write(const std::filesystem::path &target, const cv::Mat &picture, uint32_t width);

        // The picture of a .rfth, as 8-bit BGR. Empty when there is none or it cannot be read.
        [[nodiscard]] static cv::Mat read(const std::filesystem::path &thumbnail);

        // A picture file shrunk to width pixels across.
        [[nodiscard]] static cv::Mat fromImage(const std::filesystem::path &image, uint32_t width);

        // The picture to show for file: the .rfth beside it, else - for a picture file - one made from it
        // earlier, or when slow is true one made now and kept for next time. Empty for anything else.
        [[nodiscard]] static cv::Mat lookup(const std::filesystem::path &file, uint32_t width, bool slow);

        // Where a picture made from a picture file is kept, so the Gallery does not make it twice.
        [[nodiscard]] static std::filesystem::path cachePathFor(const std::filesystem::path &source, uint32_t width);

        // Room the kept pictures may take before the ones looked at longest ago are let go.
        static constexpr uint64_t MAX_CACHE_BYTES = 256ull << 20;

        // Lets go of the kept pictures looked at longest ago while they take more than MAX_CACHE_BYTES.
        static void trimCache();
    };
}
