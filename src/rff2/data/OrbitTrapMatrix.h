//
// Created by Opus 5.5 on 2026-10-12
// Modified by Opus 5.5 on 2026-10-16
//

#pragma once
#include <cstdint>
#include <vector>

#include "Matrix.h"
#include "../formula/OrbitTrap.h"

namespace merutilm::rff2 {
    // Per-pixel OrbitTrap kept as one float plane per field, so every element stays a lock-free relaxed atomic.
    class OrbitTrapMatrix {
        std::vector<Matrix<float>> planes;

    public:
        // planeCount is OrbitTrap::PLANE_COUNT, or SHAPE_PLANE_COUNT to keep the shape trap as well.
        OrbitTrapMatrix(const uint16_t width, const uint16_t height,
                        const uint32_t planeCount = OrbitTrap::PLANE_COUNT) {
            const uint32_t count = planeCount == OrbitTrap::SHAPE_PLANE_COUNT ? planeCount : OrbitTrap::PLANE_COUNT;
            planes.reserve(count);
            for (uint32_t k = 0; k < count; ++k) {
                planes.emplace_back(width, height);
            }
        }

        uint16_t getWidth() const { return planes[0].getWidth(); }

        uint16_t getHeight() const { return planes[0].getHeight(); }

        uint32_t getLength() const { return planes[0].getLength(); }

        uint32_t getPlaneCount() const { return static_cast<uint32_t>(planes.size()); }

        bool hasShapePlanes() const { return planes.size() == OrbitTrap::SHAPE_PLANE_COUNT; }

        void storeRelaxed(const uint32_t i, const OrbitTrap &trap) {
            planes[0].storeRelaxed(i, trap.crossLog2);
            planes[1].storeRelaxed(i, trap.pointLog2);
            planes[2].storeRelaxed(i, trap.secondLog2);
            planes[3].storeRelaxed(i, trap.iteration);
            if (hasShapePlanes()) {
                planes[4].storeRelaxed(i, trap.shapeLog2);
                planes[5].storeRelaxed(i, trap.shapeIteration);
                planes[6].storeRelaxed(i, trap.shapeAngle);
                planes[7].storeRelaxed(i, trap.shapeStatus);
            }
        }

        void storeRelaxed(const uint16_t x, const uint16_t y, const OrbitTrap &trap) {
            storeRelaxed(planes[0].getIndex(x, y), trap);
        }

        OrbitTrap loadRelaxed(const uint32_t i) const {
            OrbitTrap trap{planes[0].loadRelaxed(i), planes[1].loadRelaxed(i), planes[2].loadRelaxed(i),
                           planes[3].loadRelaxed(i)};
            if (hasShapePlanes()) {
                trap.shapeLog2 = planes[4].loadRelaxed(i);
                trap.shapeIteration = planes[5].loadRelaxed(i);
                trap.shapeAngle = planes[6].loadRelaxed(i);
                trap.shapeStatus = planes[7].loadRelaxed(i);
            }
            return trap;
        }

        // Every pixel back to "no trap data".
        void clear() {
            for (uint32_t i = 0, length = getLength(); i < length; ++i) {
                storeRelaxed(i, OrbitTrap{0.0f, 0.0f, 0.0f, 0.0f});
            }
        }

        // Plane-major, plane k starting at k * getLength(), as the iteration palette shader reads it.
        void copyTo(float *destination) const {
            const uint32_t length = getLength();
            for (uint32_t k = 0; k < getPlaneCount(); ++k) {
                for (uint32_t i = 0; i < length; ++i) {
                    destination[k * length + i] = planes[k].loadRelaxed(i);
                }
            }
        }

        void copyFrom(const float *source) {
            const uint32_t length = getLength();
            for (uint32_t k = 0; k < getPlaneCount(); ++k) {
                for (uint32_t i = 0; i < length; ++i) {
                    planes[k].storeRelaxed(i, source[k * length + i]);
                }
            }
        }
    };
}
