//
// Created by Opus 5.5 on 2026-10-13
// Modified by Opus 5.5 on 2026-10-16, 2026-10-29
//

#include "RFFOrbitTrapBinary.h"

#include <cstring>
#include <fstream>
#include <new>

#include "MapLimits.hpp"
#include "RFFMapCompression.h"
#include "../formula/OrbitTrap.h"
#include "../constants/Constants.hpp"
#include "../ui/IOUtilities.h"
#include "../../vulkan_helper/core/logger.hpp"

namespace merutilm::rff2 {
    namespace {
        constexpr uint32_t BYTES_PER_VALUE = sizeof(float);

        // Each float's four bytes go to four planes of equal significance, which the entropy coder packs far better.
        std::vector<char> splitBytes(const std::vector<float> &values) {
            const size_t count = values.size();
            std::vector<char> raw(count * BYTES_PER_VALUE);
            const auto *bytes = reinterpret_cast<const unsigned char *>(values.data());
            for (size_t i = 0; i < count; ++i) {
                for (uint32_t b = 0; b < BYTES_PER_VALUE; ++b) {
                    raw[b * count + i] = static_cast<char>(bytes[i * BYTES_PER_VALUE + b]);
                }
            }
            return raw;
        }

        std::vector<float> joinBytes(const std::vector<char> &raw, const size_t count) {
            std::vector<float> values(count);
            auto *bytes = reinterpret_cast<unsigned char *>(values.data());
            for (size_t i = 0; i < count; ++i) {
                for (uint32_t b = 0; b < BYTES_PER_VALUE; ++b) {
                    bytes[i * BYTES_PER_VALUE + b] = static_cast<unsigned char>(raw[b * count + i]);
                }
            }
            return values;
        }

        // The fields of the shape trap's later settings, fixed until those settings exist: Fast, Minimum, no
        // observation transform, and a capture window of every iteration.
        constexpr uint8_t SHAPE_ACCURACY = 0;
        constexpr uint8_t SHAPE_STATISTIC = 0;
        constexpr uint8_t SHAPE_OBSERVATION = 0;
        constexpr uint64_t SHAPE_CAPTURE_START = 1;
        constexpr uint64_t SHAPE_CAPTURE_END = 0;
    }

    std::filesystem::path RFFOrbitTrapBinary::keyframePath(const std::filesystem::path &dir, const uint32_t id) {
        return dir / IOUtilities::fileNameFormat(id, Constants::Extension::ORBIT_TRAP);
    }

    bool RFFOrbitTrapBinary::write(const std::filesystem::path &path, const std::vector<float> &planes,
                                   const uint16_t width, const uint16_t height, const TrapShapeParams &shape) {
        const uint64_t count = static_cast<uint64_t>(width) * height;
        if (!MapLimits::valid(width, height) || (planes.size() != count * OrbitTrap::PLANE_COUNT &&
                                                 planes.size() != count * OrbitTrap::SHAPE_PLANE_COUNT)) {
            vkh::logger::w_log(L"ERROR : Orbit trap planes do not match the map size");
            return false;
        }
        const bool withShape = shape.enabled && planes.size() == count * OrbitTrap::SHAPE_PLANE_COUNT;
        const uint32_t planeCount = withShape ? OrbitTrap::SHAPE_PLANE_COUNT : OrbitTrap::PLANE_COUNT;
        const uint64_t kept = count * planeCount;
        const std::vector<char> raw = kept == planes.size()
                                          ? splitBytes(planes)
                                          : splitBytes(std::vector<float>(planes.begin(), planes.begin() + static_cast<std::ptrdiff_t>(kept)));
        const std::vector<char> compressed = RFFMapCompression::compress(raw, RFFMapCompression::DEFAULT_LEVEL);
        if (compressed.empty()) {
            vkh::logger::w_log(L"ERROR : Cannot compress orbit traps");
            return false;
        }
        const std::filesystem::path temporary = IOUtilities::temporaryFilePath(path);
        std::ofstream out(temporary, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            vkh::logger::w_log(L"ERROR : Cannot save file");
            return false;
        }
        IOUtilities::encodeAndWrite(out, MAGIC, sizeof(MAGIC));
        const bool withSides = withShape && shape.sides != 0;
        IOUtilities::encodeAndWrite(out, withSides ? SIDES_VERSION : withShape ? SHAPE_VERSION : VERSION);
        IOUtilities::encodeAndWrite(out, width);
        IOUtilities::encodeAndWrite(out, height);
        IOUtilities::encodeAndWrite(out, planeCount);
        if (withShape) {
            IOUtilities::encodeAndWrite(out, static_cast<uint8_t>(shape.shape));
            IOUtilities::encodeAndWrite(out, SHAPE_ACCURACY);
            IOUtilities::encodeAndWrite(out, SHAPE_STATISTIC);
            IOUtilities::encodeAndWrite(out, SHAPE_OBSERVATION);
            IOUtilities::encodeAndWrite(out, shape.centerReal);
            IOUtilities::encodeAndWrite(out, shape.centerImag);
            IOUtilities::encodeAndWrite(out, shape.radius);
            IOUtilities::encodeAndWrite(out, shape.angleDegrees);
            IOUtilities::encodeAndWrite(out, SHAPE_CAPTURE_START);
            IOUtilities::encodeAndWrite(out, SHAPE_CAPTURE_END);
            if (withSides) {
                IOUtilities::encodeAndWrite(out, shape.sides);
            }
        }
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

    std::vector<float> RFFOrbitTrapBinary::read(const std::filesystem::path &path, const uint16_t width,
                                                const uint16_t height, const TrapShapeParams &expected) {
        std::error_code existsError;
        if (!std::filesystem::is_regular_file(path, existsError)) {
            return {};
        }
        std::ifstream in(path, std::ios::in | std::ios::binary);
        if (!in.is_open()) {
            return {};
        }
        char magic[sizeof(MAGIC)] = {};
        IOUtilities::readAndDecode(in, sizeof(magic), magic);
        uint16_t version = 0, w = 0, h = 0;
        uint32_t planeCount = 0;
        uint64_t rawSize = 0, compressedSize = 0;
        IOUtilities::readAndDecode(in, &version);
        IOUtilities::readAndDecode(in, &w);
        IOUtilities::readAndDecode(in, &h);
        IOUtilities::readAndDecode(in, &planeCount);
        uint8_t shapeKind = 0, accuracy = 0, statistic = 0, observation = 0;
        uint64_t captureStart = 0, captureEnd = 0;
        TrapShapeParams stored;
        const bool shapeVersion = version == SHAPE_VERSION || version == SIDES_VERSION;
        if (shapeVersion) {
            IOUtilities::readAndDecode(in, &shapeKind);
            IOUtilities::readAndDecode(in, &accuracy);
            IOUtilities::readAndDecode(in, &statistic);
            IOUtilities::readAndDecode(in, &observation);
            IOUtilities::readAndDecode(in, &stored.centerReal);
            IOUtilities::readAndDecode(in, &stored.centerImag);
            IOUtilities::readAndDecode(in, &stored.radius);
            IOUtilities::readAndDecode(in, &stored.angleDegrees);
            IOUtilities::readAndDecode(in, &captureStart);
            IOUtilities::readAndDecode(in, &captureEnd);
            if (version == SIDES_VERSION) {
                IOUtilities::readAndDecode(in, &stored.sides);
            }
            stored.enabled = true;
            stored.shape = static_cast<ShdPalOrbitTrapShape>(shapeKind);
        }
        IOUtilities::readAndDecode(in, &rawSize);
        IOUtilities::readAndDecode(in, &compressedSize);

        // The header is checked against the map it goes with before anything the size asks for is allocated.
        const uint64_t pixels = static_cast<uint64_t>(w) * h;
        const uint64_t count = pixels * planeCount;
        const bool knownVersion = (version == VERSION && planeCount == OrbitTrap::PLANE_COUNT) ||
                                  (shapeVersion && planeCount == OrbitTrap::SHAPE_PLANE_COUNT &&
                                   shapeKind >= static_cast<uint8_t>(ShdPalOrbitTrapShape::POINT) &&
                                   shapeKind <= ORBIT_TRAP_SHAPE_LAST);
        std::error_code sizeError;
        const uint64_t fileSize = std::filesystem::file_size(path, sizeError);
        const auto headerEnd = static_cast<uint64_t>(in.tellg());
        if (!in || sizeError || std::memcmp(magic, MAGIC, sizeof(MAGIC)) != 0 || !knownVersion ||
            w != width || h != height || !MapLimits::valid(w, h) ||
            rawSize != count * BYTES_PER_VALUE || headerEnd > fileSize || compressedSize > fileSize - headerEnd) {
            return {};
        }
        // Shape planes measured under other settings than the ones drawing them are dropped, leaving the first four.
        const bool shapeMatches = shapeVersion && expected.enabled && stored == expected &&
                                  accuracy == SHAPE_ACCURACY && statistic == SHAPE_STATISTIC &&
                                  observation == SHAPE_OBSERVATION && captureStart == SHAPE_CAPTURE_START &&
                                  captureEnd == SHAPE_CAPTURE_END;
        try {
            std::vector<char> compressed(static_cast<size_t>(compressedSize));
            IOUtilities::readAndDecode(in, compressedSize, compressed.data());
            if (!in) {
                return {};
            }
            std::vector<char> raw;
            if (!RFFMapCompression::decompress(compressed, rawSize, &raw)) {
                return {};
            }
            std::vector<float> planes = joinBytes(raw, static_cast<size_t>(count));
            if (planeCount == OrbitTrap::SHAPE_PLANE_COUNT && !shapeMatches) {
                planes.resize(static_cast<size_t>(pixels * OrbitTrap::PLANE_COUNT));
            }
            return planes;
        } catch (const std::bad_alloc &) {
            return {};
        }
    }

    std::vector<float> RFFOrbitTrapBinary::readByID(const std::filesystem::path &dir, const uint32_t id,
                                                    const uint16_t width, const uint16_t height,
                                                    const TrapShapeParams &expected) {
        return read(keyframePath(dir, id), width, height, expected);
    }
}
