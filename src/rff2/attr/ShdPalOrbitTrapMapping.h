//
// Created by Opus 5.5 on 2026-10-24
// Modified by Gemini 4 Neon on 2026-11-19
//

#pragma once

#include <cstdint>

namespace merutilm::rff2 {
    // What the Shape trap's palette position follows; Distance is how every earlier version drew it.
    enum class ShdPalOrbitTrapMapping {
        // The palette runs along the log2 distance to the trap.
        DISTANCE = 0,
        // The palette turns with arg(z - c) at the nearest approach, Orbit Trap Arms times per turn.
        ANGLE = 1,
        // Contour bands of the log2 distance, a quarter cycle per halving times Density, with a dark seam between bands.
        BANDS = 2,
        // The palette follows the hit iteration stored on plane 5, Density cycles per 12 iterations.
        ITERATION = 3,
        // Combines the hit iteration with the distance to the trap, scaled by Density.
        ITERATION_DISTANCE = 4,
        // Polar petal motif from distance, arg(z - c) and hit iteration, with Arms petals per turn.
        POLAR_PETALS = 5,
        // Polar checkerboard of concentric log2-distance rings and 2 * Arms angular sectors, tinted by hit iteration.
        POLAR_CHECKER = 6
    };

    constexpr uint32_t ORBIT_TRAP_MAPPING_LAST = static_cast<uint32_t>(ShdPalOrbitTrapMapping::POLAR_CHECKER);

    // Mappings that read Orbit Trap Arms.
    constexpr bool orbitTrapMappingUsesArms(const ShdPalOrbitTrapMapping mapping) {
        return mapping == ShdPalOrbitTrapMapping::ANGLE ||
               mapping == ShdPalOrbitTrapMapping::POLAR_PETALS ||
               mapping == ShdPalOrbitTrapMapping::POLAR_CHECKER;
    }

    // Mappings that read Orbit Trap Density.
    constexpr bool orbitTrapMappingUsesDensity(const ShdPalOrbitTrapMapping mapping) {
        return mapping != ShdPalOrbitTrapMapping::ANGLE;
    }
}
