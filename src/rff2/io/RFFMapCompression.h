//
// Created by Opus 5 on 2026-08-14.
//

#pragma once
#include <cstdint>
#include <vector>

namespace merutilm::rff2 {
    // Lossless packing of an iteration table. Every value comes back bit for bit, so a compressed
    // map is the map that was saved and nothing about the picture it draws can change.
    struct RFFMapCompression {
        RFFMapCompression() = delete;

        // Byte stream layouts, stored in the file header so a reader knows how to undo one.
        // SPLIT is the first form and is still read; LORENZO is what is written now.
        static constexpr uint8_t MODE_STREAM_SPLIT = 0;
        static constexpr uint8_t MODE_LORENZO_SPLIT = 1;
        static constexpr uint8_t CURRENT_MODE = MODE_LORENZO_SPLIT;

        // Effort spent by the entropy coder. Past this the ratio barely moves while the time keeps
        // climbing, and a keyframe run pays this cost once per keyframe.
        static constexpr int DEFAULT_LEVEL = 6;

        // Exactly how many bytes `count` values preprocess into, in either mode. A reader validates
        // the header against this before it allocates anything.
        [[nodiscard]] static uint64_t streamSize(uint64_t count);

        // Turns the table into something the entropy coder can work on. Each pixel is predicted from
        // the three already-seen neighbours around it and only the difference is kept, then the eight
        // bytes of every difference are split into planes of equal significance.
        [[nodiscard]] static std::vector<char> preprocess(const std::vector<double> &values, uint16_t width);

        // Undoes preprocess for the given mode. Empty when the stream does not hold `count` values,
        // or when the mode is one this build does not know.
        [[nodiscard]] static std::vector<double> postprocess(const std::vector<char> &stream, uint16_t width,
                                                             uint32_t count, uint8_t mode);

        // Empty on failure, which for a non-empty input is always an error.
        [[nodiscard]] static std::vector<char> compress(const std::vector<char> &raw, int level);

        [[nodiscard]] static bool decompress(const std::vector<char> &compressed, uint64_t rawSize,
                                             std::vector<char> *out);
    };
}
