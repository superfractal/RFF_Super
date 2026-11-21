//
// Created by Opus 5.5 on 2026-11-16
//

#pragma once

namespace merutilm::rff2 {
    // Which orbit point the Shape trap keeps; the numbers are the .rftz statistic byte, Nearest being how every earlier version measured.
    enum class ShdPalOrbitTrapStatistic {
        // The orbit point nearest the trap.
        NEAREST = 0,
        // The first orbit point within 2^Orbit Trap Width of the trap.
        FIRST_TOUCH = 1,
        // The last orbit point within 2^Orbit Trap Width of the trap.
        LAST_TOUCH = 2
    };

    constexpr bool orbitTrapStatisticTouches(const ShdPalOrbitTrapStatistic statistic) {
        return statistic != ShdPalOrbitTrapStatistic::NEAREST;
    }
}
