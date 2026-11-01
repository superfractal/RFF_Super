//
// Created by Opus 5.5 on 2026-10-13
// Modified by Opus 5.5 on 2026-10-16, 2026-10-29
//

#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>

#include "../formula/OrbitTrapShape.h"

namespace merutilm::rff2 {
    // A keyframe's per-pixel orbit traps, saved beside its map as NNNN.rftz so a video can draw Orbit Trap Coloring.
    // The planes are OrbitTrap::PLANE_COUNT float arrays laid out as the map's own table, plane after plane.
    struct RFFOrbitTrapBinary {
        RFFOrbitTrapBinary() = delete;

        static constexpr char MAGIC[4] = {'R', 'F', 'T', 'Z'};
        static constexpr uint16_t VERSION = 1;
        // Version 2 carries all OrbitTrap::SHAPE_PLANE_COUNT planes and the TrapShapeParams they were measured with.
        static constexpr uint16_t SHAPE_VERSION = 2;
        // Version 3 is version 2 with a trailing uint32 sides, written only for the shapes that read Orbit Trap Sides.
        static constexpr uint16_t SIDES_VERSION = 3;

        [[nodiscard]] static std::filesystem::path keyframePath(const std::filesystem::path &dir, uint32_t id);

        // False when the planes do not fit the size or the file cannot be written.
        // Shape planes go out as version 2 only while shape is enabled; otherwise the first four go out as version 1.
        [[nodiscard]] static bool write(const std::filesystem::path &path, const std::vector<float> &planes,
                                        uint16_t width, uint16_t height, const TrapShapeParams &shape);

        // Empty when the file is missing, unreadable, or holds another size than the map it goes with.
        // A version 2 file gives its shape planes only when they were measured with expected; otherwise its first four.
        [[nodiscard]] static std::vector<float> read(const std::filesystem::path &path, uint16_t width, uint16_t height,
                                                     const TrapShapeParams &expected);

        [[nodiscard]] static std::vector<float> readByID(const std::filesystem::path &dir, uint32_t id,
                                                         uint16_t width, uint16_t height,
                                                         const TrapShapeParams &expected);
    };
}
