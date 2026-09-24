//
// Created by Merutilm on 2025-05-31.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21
// Modified by GPT-6 on 2026-09-23
//

#include "RenderPresets.h"

#include <thread>

namespace merutilm::rff2 {
    namespace {
        RenderAttribute renderWithClarity(float clarityMultiplier) {
            return RenderAttribute{
                .clarityMultiplier = clarityMultiplier,
                .ssaa = 1,
                .fps = 60,
                .linearInterpolation = true,
                .threads = std::thread::hardware_concurrency(),
                .boundaryTraceFill = false,
                .preview2Color = false,
                .coarsePreview = true,
            };
        }
    }

    std::string RenderPresets::Potato::getName() const {
        return "Potato";
    }

    RenderAttribute RenderPresets::Potato::genRender() const {
        return renderWithClarity(0.1f);
    }

    std::string RenderPresets::Low::getName() const {
        return "Low";
    }

    RenderAttribute RenderPresets::Low::genRender() const {
        return renderWithClarity(0.3f);
    }

    std::string RenderPresets::Medium::getName() const {
        return "Medium";
    }

    RenderAttribute RenderPresets::Medium::genRender() const {
        return renderWithClarity(0.5f);
    }

    std::string RenderPresets::High::getName() const {
        return "High";
    }

    RenderAttribute RenderPresets::High::genRender() const {
        return renderWithClarity(1.0f);
    }

    std::string RenderPresets::Ultra::getName() const {
        return "Ultra";
    }

    RenderAttribute RenderPresets::Ultra::genRender() const {
        return renderWithClarity(2.0f);
    }

    std::string RenderPresets::Extreme::getName() const {
        return "Extreme [DANGER]";
    }

    RenderAttribute RenderPresets::Extreme::genRender() const {
        return renderWithClarity(4.0f);
    }
}
