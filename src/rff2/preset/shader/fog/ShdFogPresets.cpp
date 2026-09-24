//
// Created by Merutilm on 2025-05-28.
// Modified by GPT-6 on 2026-09-23
//

#include "ShdFogPresets.h"

namespace merutilm::rff2 {

    std::string ShdFogPresets::Disabled::getName() const {
        return "Disabled";
    }

    ShdFogAttribute ShdFogPresets::Disabled::genFog() const {
        return ShdFogAttribute{.radius = 0.0, .opacity = 0.0};
    }

    std::string ShdFogPresets::Low::getName() const {
        return "Low";
    }

    ShdFogAttribute ShdFogPresets::Low::genFog() const {
        return ShdFogAttribute{.radius = 0.1f, .opacity = 0.2f};
    }

    std::string ShdFogPresets::Medium::getName() const {
        return "Medium";
    }

    ShdFogAttribute ShdFogPresets::Medium::genFog() const {
        return ShdFogAttribute{.radius = 0.1f, .opacity = 0.5f};
    }

    std::string ShdFogPresets::High::getName() const {
        return "High";
    }

    ShdFogAttribute ShdFogPresets::High::genFog() const {
        return ShdFogAttribute{.radius = 0.15f, .opacity = 0.8f};
    }

    std::string ShdFogPresets::Ultra::getName() const {
        return "Ultra";
    }

    ShdFogAttribute ShdFogPresets::Ultra::genFog() const {
        return ShdFogAttribute{.radius = 0.15f, .opacity = 1};
    }
}
