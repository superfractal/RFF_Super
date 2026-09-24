//
// Created by Merutilm on 2025-05-28.
// Modified by GPT-6 on 2026-09-23
//

#include "ShdBloomPresets.h"

namespace merutilm::rff2 {

    std::string BloomPresets::Disabled::getName() const {
        return "Disabled";
    }

    ShdBloomAttribute BloomPresets::Disabled::genBloom() const {
        return ShdBloomAttribute{
            .threshold = 0,
            .radius = 0.0f,
            .softness = 0,
            .intensity = 0
        };
    }

    std::string BloomPresets::Highlighted::getName() const {
        return "Highlighted";
    }

    ShdBloomAttribute BloomPresets::Highlighted::genBloom() const {
        return ShdBloomAttribute{
            .threshold = 0,
            .radius = 0.05f,
            .softness = 0.2f,
            .intensity = 1
        };
    }

    std::string BloomPresets::HighlightedStrong::getName() const {
        return "Highlighted Strong";
    }

    ShdBloomAttribute BloomPresets::HighlightedStrong::genBloom() const {
        return ShdBloomAttribute{
            .threshold = 0,
            .radius = 0.08f,
            .softness = 0.4f,
            .intensity = 1.5f
        };
    }

    std::string BloomPresets::Weak::getName() const {
        return "Weak";
    }

    ShdBloomAttribute BloomPresets::Weak::genBloom() const {
        return ShdBloomAttribute{
            .threshold = 0,
            .radius = 0.1f,
            .softness = 0,
            .intensity = 0.5f
        };
    }

    std::string BloomPresets::Normal::getName() const {
        return "Normal";
    }

    ShdBloomAttribute BloomPresets::Normal::genBloom() const {
        return ShdBloomAttribute{
            .threshold = 0,
            .radius = 0.1f,
            .softness = 0,
            .intensity = 1
        };
    }

    std::string BloomPresets::Strong::getName() const {
        return "Strong";
    }

    ShdBloomAttribute BloomPresets::Strong::genBloom() const {
        return ShdBloomAttribute{
            .threshold = 0,
            .radius = 0.1f,
            .softness = 0,
            .intensity = 1.5f
        };
    }
}
