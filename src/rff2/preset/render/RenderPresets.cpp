//
// Created by Merutilm on 2025-05-31.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
//

#include "RenderPresets.h"

#include <thread>


namespace merutilm::rff2 {
    std::string RenderPresets::Potato::getName() const {
        return "Potato";
    }

    RenderAttribute RenderPresets::Potato::genRender() const {
        return RenderAttribute{0.1f, 1, 60, true, std::thread::hardware_concurrency(), false, false, true};
    }


    std::string RenderPresets::Low::getName() const {
        return "Low";
    }

    RenderAttribute RenderPresets::Low::genRender() const {
        return RenderAttribute{0.3f, 1, 60, true, std::thread::hardware_concurrency(), false, false, true};
    }

    std::string RenderPresets::Medium::getName() const {
        return "Medium";
    }

    RenderAttribute RenderPresets::Medium::genRender() const {
        return RenderAttribute{0.5f, 1, 60, true, std::thread::hardware_concurrency(), false, false, true};
    }

    std::string RenderPresets::High::getName() const {
        return "High";
    }

    RenderAttribute RenderPresets::High::genRender() const {
        return RenderAttribute{1.0f, 1, 60, true, std::thread::hardware_concurrency(), false, false, true};
    }

    std::string RenderPresets::Ultra::getName() const {
        return "Ultra";
    }

    RenderAttribute RenderPresets::Ultra::genRender() const {
        return RenderAttribute{2.0f, 1, 60, true, std::thread::hardware_concurrency(), false, false, true};
    }

    std::string RenderPresets::Extreme::getName() const {
        return "Extreme [DANGER]";
    }

    RenderAttribute RenderPresets::Extreme::genRender() const {
        return RenderAttribute{4.0f, 1, 60, true, std::thread::hardware_concurrency(), false, false, true};
    }
}
