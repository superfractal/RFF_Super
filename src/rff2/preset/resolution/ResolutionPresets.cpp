//
// Created by Merutilm on 2025-05-31.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
//

#include "ResolutionPresets.h"

#include <array>


namespace merutilm::rff2 {
    std::string ResolutionPresets::L1::getName() const {
        return "640x360";
    }
    std::array<int, 2> ResolutionPresets::L1::genResolution() const {
        return {640, 360};
    }
    std::string ResolutionPresets::L2::getName() const {
        return "960x540";
    }
    std::array<int, 2> ResolutionPresets::L2::genResolution() const {
        return {960, 540};
    }
    std::string ResolutionPresets::L3::getName() const {
        return "1280x720";
    }
    std::array<int, 2> ResolutionPresets::L3::genResolution() const {
        return {1280, 720};
    }
    std::string ResolutionPresets::L4::getName() const {
        return "1600x900";
    }
    std::array<int, 2> ResolutionPresets::L4::genResolution() const {
        return {1600, 900};
    }
    std::string ResolutionPresets::L5::getName() const {
        return "1920x1080";
    }
    std::array<int, 2> ResolutionPresets::L5::genResolution() const {
        return {1920, 1080};
    }
    std::string ResolutionPresets::L6::getName() const {
        return "360x640";
    }
    std::array<int, 2> ResolutionPresets::L6::genResolution() const {
        return {360, 640};
    }
}
