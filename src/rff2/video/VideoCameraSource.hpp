//
// Modified by GPT-6 on 2026-09-08, 2026-09-23
//

#pragma once
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include "../attr/Attribute.h"

namespace merutilm::rff2 {
    inline uint32_t readCameraSourceScale(const std::filesystem::path &directory) {
        const auto path = directory / "camera-source.txt";
        if (!std::filesystem::exists(path)) return 1;
        std::ifstream in(path);
        std::string magic;
        uint32_t scale = 0;
        if (!(in >> magic >> scale) || magic != "RFF_CAMERA_1" || scale < 2 || scale > 64) {
            throw std::runtime_error("Invalid camera-source.txt; restore the metadata saved with these keyframes.");
        }
        return scale;
    }

    inline Attribute cameraSourceAttribute(const Attribute &attribute, const std::filesystem::path &directory) {
        Attribute result = attribute;
        result.video.data.sourceScale = readCameraSourceScale(directory);
        if (result.render.ssaa > std::numeric_limits<uint32_t>::max() / result.video.data.sourceScale) {
            throw std::runtime_error("Camera source scale exceeds the supported supersampling range.");
        }
        result.render.ssaa *= result.video.data.sourceScale;
        return result;
    }
}
