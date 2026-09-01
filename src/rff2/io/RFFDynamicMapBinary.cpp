//
// Created by Merutilm on 2025-05-08.
// Modified by Opus 5 on 2026-08-14, 2026-08-23, 2026-08-26
// Modified by GPT-5 on 2026-08-18,2026-08-23, 2026-09-01
//

#include "RFFDynamicMapBinary.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <new>

#include "../../vulkan_helper/util/BufferImageUtils.hpp"
#include "../../vulkan_helper/core/logger.hpp"
#include "../ui/IOUtilities.h"
#include "../constants/Constants.hpp"

namespace merutilm::rff2 {
    namespace {
        constexpr uint64_t MAX_MAP_PIXELS = 100'000'000;
    }

    inline const RFFDynamicMapBinary RFFDynamicMapBinary::DEFAULT = RFFDynamicMapBinary(0, 0, 0, Matrix<double>(0, 0));

    RFFDynamicMapBinary::RFFDynamicMapBinary(const float logZoom, const uint64_t period, const uint64_t maxIteration,
                                  Matrix<double> iterations) : RFFBinary(logZoom), period(period), maxIteration(maxIteration),
                                                               iterations(std::move(iterations)) {
    }


    bool RFFDynamicMapBinary::hasData() const {
        return iterations.getWidth() > 0;
    }


    RFFDynamicMapBinary RFFDynamicMapBinary::read(const std::filesystem::path &path) {
        if (!std::filesystem::exists(path)) {
            return DEFAULT;
        }
        std::ifstream in(path, std::ios::in | std::ios::binary);

        if (!in.is_open()) {
            return DEFAULT;
        }

        uint16_t w;
        IOUtilities::readAndDecode(in, &w);
        uint16_t h;
        IOUtilities::readAndDecode(in, &h);
        float z;
        IOUtilities::readAndDecode(in, &z);
        uint64_t p;
        IOUtilities::readAndDecode(in, &p);
        uint64_t m;
        IOUtilities::readAndDecode(in, &m);
        const uint64_t count = static_cast<uint64_t>(w) * h;
        if (count == 0 || !IOUtilities::validateReadCount(in, count, sizeof(double), MAX_MAP_PIXELS)) {
            return DEFAULT;
        }
        std::vector<double> i;
        try {
            i.resize(static_cast<size_t>(count));
        } catch (const std::bad_alloc &) {
            return DEFAULT;
        }
        IOUtilities::readAndDecode(in, &i);
        if (!in) {
            return DEFAULT;
        }
        return RFFDynamicMapBinary(z, p, m, Matrix(w, h, i));
    }

    namespace {
        std::filesystem::path compressedKeyframePath(const std::filesystem::path &dir, const uint32_t id) {
            return dir / IOUtilities::fileNameFormat(id, Constants::Extension::COMPRESSED_MAP);
        }

        std::filesystem::path plainKeyframePath(const std::filesystem::path &dir, const uint32_t id) {
            return dir / IOUtilities::fileNameFormat(id, Constants::Extension::DYNAMIC_MAP);
        }

        // Path of keyframe `id` in the folder, preferring the compressed form. A folder can hold
        // both: keyframes were written uncompressed before, and a run can be resumed into one.
        std::filesystem::path keyframePath(const std::filesystem::path &dir, const uint32_t id) {
            if (std::filesystem::path compressed = compressedKeyframePath(dir, id);
                std::filesystem::exists(compressed)) {
                return compressed;
            }
            return plainKeyframePath(dir, id);
        }

        // Both forms hold width, height and zoom in that order, the compressed one behind its own leading bytes.
        bool readKeyframeHeader(const std::filesystem::path &path, uint16_t &width, uint16_t &height, float &logZoom) {
            std::ifstream in(path, std::ios::in | std::ios::binary);
            if (!in.is_open()) {
                return false;
            }
            if (RFFDynamicMapBinary::isCompressedFile(path)) {
                char magic[sizeof(RFFDynamicMapBinary::COMPRESSED_MAGIC)] = {};
                uint16_t version = 0;
                uint8_t mode = 0;
                IOUtilities::readAndDecode(in, sizeof(magic), magic);
                IOUtilities::readAndDecode(in, &version);
                IOUtilities::readAndDecode(in, &mode);
                (void) mode;
                if (!in || std::memcmp(magic, RFFDynamicMapBinary::COMPRESSED_MAGIC, sizeof(magic)) != 0 ||
                    version != RFFDynamicMapBinary::COMPRESSED_VERSION) {
                    return false;
                }
            }
            IOUtilities::readAndDecode(in, &width);
            IOUtilities::readAndDecode(in, &height);
            IOUtilities::readAndDecode(in, &logZoom);
            return static_cast<bool>(in) && width > 0 && height > 0 &&
                   static_cast<uint64_t>(width) * height <= MAX_MAP_PIXELS;
        }
    }

    RFFDynamicMapBinary RFFDynamicMapBinary::readByID(const std::filesystem::path& dir, const uint32_t id) {
        const std::filesystem::path compressed = compressedKeyframePath(dir, id);
        const std::filesystem::path plain = plainKeyframePath(dir, id);
        if (std::filesystem::exists(compressed)) {
            RFFDynamicMapBinary map = readAny(compressed);
            // Preferring the compressed form is a preference, not a verdict: one left half-written
            // by an interrupted run must not bury the uncompressed keyframe still sitting beside it.
            if (map.hasData() || !std::filesystem::exists(plain)) {
                return map;
            }
            vkh::logger::w_log(L"WARNING : Compressed keyframe unreadable, reading the uncompressed one");
        }
        return readAny(plain);
    }

    uint32_t RFFDynamicMapBinary::keyframeCount(const std::filesystem::path &dir) {
        uint32_t n = 0;
        while (std::filesystem::exists(keyframePath(dir, n + 1))) {
            ++n;
        }
        return n;
    }

    namespace {
        // Same fallback the map read makes: a compressed header that does not hold up leaves the
        // uncompressed keyframe as the one being described.
        bool readKeyframeHeaderByID(const std::filesystem::path &dir, const uint32_t id,
                                    uint16_t &width, uint16_t &height, float &logZoom) {
            if (const std::filesystem::path compressed = compressedKeyframePath(dir, id);
                std::filesystem::exists(compressed) &&
                readKeyframeHeader(compressed, width, height, logZoom)) {
                return true;
            }
            return readKeyframeHeader(plainKeyframePath(dir, id), width, height, logZoom);
        }
    }

    bool RFFDynamicMapBinary::readSizeByID(const std::filesystem::path &dir, const uint32_t id,
                                           uint16_t &width, uint16_t &height) {
        float logZoom = 0;
        return readKeyframeHeaderByID(dir, id, width, height, logZoom);
    }

    bool RFFDynamicMapBinary::readLogZoomByID(const std::filesystem::path &dir, const uint32_t id, float &logZoom) {
        uint16_t width = 0;
        uint16_t height = 0;
        return readKeyframeHeaderByID(dir, id, width, height, logZoom);
    }

    bool RFFDynamicMapBinary::isCompressedFile(const std::filesystem::path &path) {
        if (!std::filesystem::exists(path)) {
            return false;
        }
        std::ifstream in(path, std::ios::in | std::ios::binary);
        if (!in.is_open()) {
            return false;
        }
        char magic[sizeof(COMPRESSED_MAGIC)] = {};
        IOUtilities::readAndDecode(in, sizeof(magic), magic);
        return static_cast<bool>(in) && std::memcmp(magic, COMPRESSED_MAGIC, sizeof(magic)) == 0;
    }

    RFFDynamicMapBinary RFFDynamicMapBinary::readCompressed(const std::filesystem::path &path) {
        if (!std::filesystem::exists(path)) {
            return DEFAULT;
        }
        std::ifstream in(path, std::ios::in | std::ios::binary);
        if (!in.is_open()) {
            return DEFAULT;
        }

        char magic[sizeof(COMPRESSED_MAGIC)] = {};
        IOUtilities::readAndDecode(in, sizeof(magic), magic);
        if (!in || std::memcmp(magic, COMPRESSED_MAGIC, sizeof(magic)) != 0) {
            return DEFAULT;
        }
        uint16_t version;
        IOUtilities::readAndDecode(in, &version);
        if (version != COMPRESSED_VERSION) {
            return DEFAULT;
        }
        uint8_t mode;
        IOUtilities::readAndDecode(in, &mode);
        uint16_t w;
        IOUtilities::readAndDecode(in, &w);
        uint16_t h;
        IOUtilities::readAndDecode(in, &h);
        float z;
        IOUtilities::readAndDecode(in, &z);
        uint64_t p;
        IOUtilities::readAndDecode(in, &p);
        uint64_t m;
        IOUtilities::readAndDecode(in, &m);
        uint64_t rawSize;
        IOUtilities::readAndDecode(in, &rawSize);
        uint64_t compressedSize;
        IOUtilities::readAndDecode(in, &compressedSize);

        // A truncated or corrupt header must not be allowed to ask for an allocation the file could
        // never fill: the table is a known size, and the payload cannot outrun the bytes left.
        const uint64_t count = static_cast<uint64_t>(w) * h;
        std::error_code sizeError;
        const uint64_t fileSize = std::filesystem::file_size(path, sizeError);
        const auto headerEnd = static_cast<uint64_t>(in.tellg());
        if (!in || sizeError || count == 0 || count > MAX_MAP_PIXELS ||
            rawSize != RFFMapCompression::streamSize(count) ||
            headerEnd > fileSize || compressedSize > fileSize - headerEnd) {
            return DEFAULT;
        }

        std::vector<char> compressed;
        try {
            compressed.resize(static_cast<size_t>(compressedSize));
        } catch (const std::bad_alloc &) {
            return DEFAULT;
        }
        IOUtilities::readAndDecode(in, compressedSize, compressed.data());
        if (!in) {
            return DEFAULT;
        }

        std::vector<char> raw;
        if (!RFFMapCompression::decompress(compressed, rawSize, &raw)) {
            vkh::logger::w_log(L"ERROR : Cannot read the compressed map");
            return DEFAULT;
        }
        std::vector<double> i;
        try {
            i = RFFMapCompression::postprocess(raw, w, static_cast<uint32_t>(count), mode);
        } catch (const std::bad_alloc &) {
            return DEFAULT;
        }
        if (i.size() != count) {
            // Also where a map written by a later build lands: its stream mode is one this build
            // has no decoder for, and the map is refused rather than drawn from a misread table.
            vkh::logger::w_log(L"ERROR : Cannot read the compressed map");
            return DEFAULT;
        }
        return RFFDynamicMapBinary(z, p, m, Matrix(w, h, i));
    }

    RFFDynamicMapBinary RFFDynamicMapBinary::readAny(const std::filesystem::path &path) {
        return isCompressedFile(path) ? readCompressed(path) : read(path);
    }


    void RFFDynamicMapBinary::exportAsKeyframe(const std::filesystem::path &dir) const {
        exportAsKeyframe(dir, true);
    }

    void RFFDynamicMapBinary::exportAsKeyframe(const std::filesystem::path &dir, const bool compressed) const {
        const uint32_t id = keyframeCount(dir) + 1;
        if (compressed) {
            exportCompressedFile(dir / IOUtilities::fileNameFormat(id, Constants::Extension::COMPRESSED_MAP));
        } else {
            exportFile(dir / IOUtilities::fileNameFormat(id, Constants::Extension::DYNAMIC_MAP));
        }
    }

    void RFFDynamicMapBinary::exportFile(const std::filesystem::path &path) const {
        const std::filesystem::path temporary = IOUtilities::temporaryFilePath(path);
        if (std::ofstream out(temporary, std::ios::out | std::ios::binary | std::ios::trunc); out.is_open()) {
            IOUtilities::encodeAndWrite(out, iterations.getWidth());
            IOUtilities::encodeAndWrite(out, iterations.getHeight());
            IOUtilities::encodeAndWrite(out, getLogZoom());
            IOUtilities::encodeAndWrite(out, period);
            IOUtilities::encodeAndWrite(out, maxIteration);
            IOUtilities::encodeAndWrite(out, iterations.getCanvas());
            out.close();
            if (out.fail() || !IOUtilities::commitTemporaryFile(temporary, path)) {
                IOUtilities::discardTemporaryFile(temporary);
                vkh::logger::w_log(L"ERROR : Cannot save file");
            }
        } else {
            vkh::logger::w_log(L"ERROR : Cannot save file");
        }
    }

    bool RFFDynamicMapBinary::exportCompressedFile(const std::filesystem::path &path) const {
        const std::vector<char> raw = RFFMapCompression::preprocess(iterations.getCanvas(), iterations.getWidth());
        const std::vector<char> compressed = RFFMapCompression::compress(raw, RFFMapCompression::DEFAULT_LEVEL);
        if (compressed.empty()) {
            vkh::logger::w_log(L"ERROR : Cannot compress map");
            return false;
        }
        const std::filesystem::path temporary = IOUtilities::temporaryFilePath(path);
        std::ofstream out(temporary, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            vkh::logger::w_log(L"ERROR : Cannot save file");
            return false;
        }
        IOUtilities::encodeAndWrite(out, COMPRESSED_MAGIC, sizeof(COMPRESSED_MAGIC));
        IOUtilities::encodeAndWrite(out, COMPRESSED_VERSION);
        IOUtilities::encodeAndWrite(out, RFFMapCompression::CURRENT_MODE);
        IOUtilities::encodeAndWrite(out, iterations.getWidth());
        IOUtilities::encodeAndWrite(out, iterations.getHeight());
        IOUtilities::encodeAndWrite(out, getLogZoom());
        IOUtilities::encodeAndWrite(out, period);
        IOUtilities::encodeAndWrite(out, maxIteration);
        IOUtilities::encodeAndWrite(out, static_cast<uint64_t>(raw.size()));
        IOUtilities::encodeAndWrite(out, static_cast<uint64_t>(compressed.size()));
        IOUtilities::encodeAndWrite(out, compressed.data(), compressed.size());
        out.close();
        if (out.fail() || !IOUtilities::commitTemporaryFile(temporary, path)) {
            IOUtilities::discardTemporaryFile(temporary);
            vkh::logger::w_log(L"ERROR : Cannot save file");
            return false;
        }
        return true;
    }


    uint64_t RFFDynamicMapBinary::getPeriod() const {
        return period;
    }

    uint64_t RFFDynamicMapBinary::getMaxIteration() const {
        return maxIteration;
    }

    const Matrix<double> &RFFDynamicMapBinary::getMatrix() const {
        return iterations;
    }
}
