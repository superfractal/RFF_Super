//
// Created by Opus 5 on 2026-08-14.
// Modified by GPT-5 on 2026-08-23.
//

#include "RFFMapCompression.h"

#include <algorithm>
#include <cstring>
#include <new>

#include <zstd.h>

namespace merutilm::rff2 {
    namespace {
        constexpr size_t VALUE_BYTES = sizeof(double);
        constexpr int MIN_LEVEL = 1;
        constexpr int MAX_LEVEL = 19;

        uint64_t toBits(const double v) {
            uint64_t u;
            std::memcpy(&u, &v, VALUE_BYTES);
            return u;
        }

        void fromBits(const uint64_t u, double *v) {
            std::memcpy(v, &u, VALUE_BYTES);
        }

        // Lorenzo prediction: left + up - upleft, the plane through the three neighbours already
        // seen. An iteration table is close to a smooth surface, so the prediction lands within a
        // few ulp of the value and the difference is a small number instead of a fresh 64-bit one.
        // Taken over the raw bit patterns, which for values of one sign rise with the value itself.
        template<typename Get>
        uint64_t lorenzo(const Get &get, const size_t i, const size_t x, const size_t width) {
            const bool hasLeft = x > 0;
            if (const bool hasUp = i >= width; hasLeft && hasUp) {
                return get(i - 1) + get(i - width) - get(i - width - 1);
            } else if (hasUp) {
                return get(i - width);
            }
            return hasLeft ? get(i - 1) : 0;
        }

        // Signed to unsigned, keeping small magnitudes small: a difference of -1 must not become a
        // 64-bit pattern of ones, or the planes it lands in would be noise again.
        uint64_t zigzag(const uint64_t v) {
            return v << 1 ^ static_cast<uint64_t>(static_cast<int64_t>(v) >> 63);
        }

        uint64_t unzigzag(const uint64_t v) {
            return v >> 1 ^ -(v & 1);
        }

        // Byte k of every value lands in plane k, so bytes of equal significance sit together. The
        // top planes of a difference stream are almost all zero, and zstd codes a run of zeros for
        // nothing; interleaved, those same bytes would break up every run around them.
        std::vector<char> splitBytes(const std::vector<uint64_t> &values) {
            const size_t count = values.size();
            std::vector<char> out(count * VALUE_BYTES);
            for (size_t i = 0; i < count; ++i) {
                for (size_t k = 0; k < VALUE_BYTES; ++k) {
                    out[k * count + i] = static_cast<char>(values[i] >> k * 8 & 0xff);
                }
            }
            return out;
        }

        std::vector<uint64_t> joinBytes(const std::vector<char> &stream, const size_t count) {
            std::vector<uint64_t> out(count);
            for (size_t i = 0; i < count; ++i) {
                uint64_t v = 0;
                for (size_t k = 0; k < VALUE_BYTES; ++k) {
                    v |= static_cast<uint64_t>(static_cast<uint8_t>(stream[k * count + i])) << k * 8;
                }
                out[i] = v;
            }
            return out;
        }
    }

    uint64_t RFFMapCompression::streamSize(const uint64_t count) {
        return count * VALUE_BYTES;
    }

    std::vector<char> RFFMapCompression::preprocess(const std::vector<double> &values, const uint16_t width) {
        const size_t count = values.size();
        if (count == 0 || width == 0) {
            return {};
        }
        const auto get = [&values](const size_t index) { return toBits(values[index]); };
        std::vector<uint64_t> residual(count);
        for (size_t i = 0, x = 0; i < count; ++i, x = x + 1 == width ? 0 : x + 1) {
            residual[i] = zigzag(get(i) - lorenzo(get, i, x, width));
        }
        return splitBytes(residual);
    }

    std::vector<double> RFFMapCompression::postprocess(const std::vector<char> &stream, const uint16_t width,
                                                       const uint32_t count, const uint8_t mode) {
        if (stream.size() != static_cast<size_t>(count) * VALUE_BYTES || count == 0 || width == 0) {
            return {};
        }
        const std::vector<uint64_t> joined = joinBytes(stream, count);
        std::vector<double> values(count);

        if (mode == MODE_STREAM_SPLIT) {
            // The first form kept the values themselves, with no prediction in front of the split.
            for (size_t i = 0; i < count; ++i) {
                fromBits(joined[i], &values[i]);
            }
            return values;
        }
        if (mode != MODE_LORENZO_SPLIT) {
            return {};
        }
        // Every neighbour the prediction reads has already been written on an earlier pass of this
        // loop, so it reproduces exactly the prediction the writer subtracted.
        const auto get = [&values](const size_t index) { return toBits(values[index]); };
        for (size_t i = 0, x = 0; i < count; ++i, x = x + 1 == width ? 0 : x + 1) {
            fromBits(unzigzag(joined[i]) + lorenzo(get, i, x, width), &values[i]);
        }
        return values;
    }

    std::vector<char> RFFMapCompression::compress(const std::vector<char> &raw, const int level) {
        const size_t bound = ZSTD_compressBound(raw.size());
        std::vector<char> out(bound);
        const size_t written = ZSTD_compress(out.data(), bound, raw.data(), raw.size(),
                                             std::clamp(level, MIN_LEVEL, MAX_LEVEL));
        if (ZSTD_isError(written)) {
            return {};
        }
        out.resize(written);
        return out;
    }

    bool RFFMapCompression::decompress(const std::vector<char> &compressed, const uint64_t rawSize,
                                       std::vector<char> *out) {
        if (rawSize == 0) {
            out->clear();
            return true;
        }
        const unsigned long long frameSize = ZSTD_getFrameContentSize(compressed.data(), compressed.size());
        if (frameSize == ZSTD_CONTENTSIZE_ERROR || frameSize == ZSTD_CONTENTSIZE_UNKNOWN || frameSize != rawSize) {
            return false;
        }
        try {
            out->assign(static_cast<size_t>(rawSize), 0);
        } catch (const std::bad_alloc &) {
            return false;
        }
        const size_t read = ZSTD_decompress(out->data(), rawSize, compressed.data(), compressed.size());
        return !ZSTD_isError(read) && read == rawSize;
    }
}
