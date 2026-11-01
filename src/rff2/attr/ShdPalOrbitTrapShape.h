//
// Created by Opus 5.5 on 2026-10-16
// Modified by Opus 5.5 on 2026-10-29
//

#pragma once

#include <cstdint>

namespace merutilm::rff2 {
    // The trap Orbit Trap Coloring's Shape measures each orbit point against; 0 stays reserved for "no shape" in .rftz.
    // From GRID_POINTS on, the radius r is the shape's spacing or size and a radius of 0 measures as Point (Grid Lines as Cross).
    enum class ShdPalOrbitTrapShape {
        // |z - c|.
        POINT = 1,
        // | |z - c| - r |: the distance to the circle's rim, not to a filled disc.
        CIRCLE = 2,
        // min(|u|, |v|) for z - c turned by the angle: two perpendicular lines through the center.
        CROSS = 3,
        // |n . (z - c)| with n = (cos angle, sin angle): one line through the center, normal to n.
        LINE = 4,
        // The nearest point of a square lattice of spacing r through the center, turned by the angle.
        GRID_POINTS = 5,
        // The nearest line of that lattice: a grid of squares.
        GRID_LINES = 6,
        // The nearest of the circles of radius r, 2r, 3r, ... around the center, the center itself included.
        RINGS = 7,
        // The outline of a regular polygon of circumradius r with Orbit Trap Sides sides, a corner on the angle.
        POLYGON = 8,
        // The outline of a star of Orbit Trap Sides points of radius r, its inner corners at r / 2, a point on the angle.
        STAR = 9,
        // An Archimedean spiral moving out by r per turn, measured along the radius, the center included.
        SPIRAL = 10
    };

    constexpr uint32_t ORBIT_TRAP_SHAPE_LAST = static_cast<uint32_t>(ShdPalOrbitTrapShape::SPIRAL);
    // The range of Orbit Trap Sides.
    constexpr uint32_t ORBIT_TRAP_SIDES_MIN = 3;
    constexpr uint32_t ORBIT_TRAP_SIDES_MAX = 64;

    // Shapes that read Orbit Trap Sides.
    constexpr bool orbitTrapShapeUsesSides(const ShdPalOrbitTrapShape shape) {
        return shape == ShdPalOrbitTrapShape::POLYGON || shape == ShdPalOrbitTrapShape::STAR;
    }

    // Shapes that read Orbit Trap Radius.
    constexpr bool orbitTrapShapeUsesRadius(const ShdPalOrbitTrapShape shape) {
        return shape != ShdPalOrbitTrapShape::POINT && shape != ShdPalOrbitTrapShape::CROSS &&
               shape != ShdPalOrbitTrapShape::LINE;
    }

    // Shapes that read Orbit Trap Angle.
    constexpr bool orbitTrapShapeUsesAngle(const ShdPalOrbitTrapShape shape) {
        return shape != ShdPalOrbitTrapShape::POINT && shape != ShdPalOrbitTrapShape::CIRCLE &&
               shape != ShdPalOrbitTrapShape::RINGS;
    }
}
