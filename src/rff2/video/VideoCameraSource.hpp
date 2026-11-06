//
// Modified by GPT-6 on 2026-09-08, 2026-09-23
// Modified by Opus 5.5 on 2026-11-05
//

#pragma once
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <numbers>
#include <optional>
#include <stdexcept>
#include "../attr/Attribute.h"
#include "../io/ConfigIO.h"

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

    // The lowest-numbered keyframe's settings file, which records the projection every keyframe of the folder was calculated in.
    inline std::optional<std::filesystem::path> firstKeyframeConfig(const std::filesystem::path &directory) {
        std::optional<std::filesystem::path> first;
        unsigned long long lowest = std::numeric_limits<unsigned long long>::max();
        std::error_code error;
        for (const auto &entry : std::filesystem::directory_iterator(directory, error)) {
            const auto &path = entry.path();
            const std::wstring stem = path.stem().wstring();
            if (path.extension() != L".rfc" || stem.empty() || stem.size() > 9 ||
                !std::ranges::all_of(stem, [](const wchar_t c) { return std::iswdigit(c) != 0; })) {
                continue;
            }
            if (const unsigned long long number = std::stoull(stem); number < lowest) {
                lowest = number;
                first = path;
            }
        }
        return first;
    }

    // A folder calculated in a 360 projection keeps it baked into every keyframe, so the zoom between two of them follows it.
    inline VidKeyframeProjection readKeyframeProjection(const std::filesystem::path &directory, const Attribute &base) {
        VidKeyframeProjection result;
        const auto config = firstKeyframeConfig(directory);
        if (!config) return result;
        // Loaded over a copy so a field an older file lacks keeps a defined value.
        Attribute saved = base;
        uint16_t width = 0, height = 0;
        if (!ConfigIO::load(*config, saved, &width, &height)) return result;
        const FractalAttribute &calc = saved.fractal;
        const FrtProjectionMethod projection = effectiveProjection(calc.projectionMethod);
        if (projection == FrtProjectionMethod::PLANAR) return result;
        constexpr double toRadians = std::numbers::pi / 180.0;
        const double fov = std::clamp(static_cast<double>(calc.panoramaFov), 1.0, 179.0) * toRadians;
        const double w = std::max(1.0, static_cast<double>(width) * saved.render.clarityMultiplier * saved.render.ssaa);
        const double h = std::max(1.0, static_cast<double>(height) * saved.render.clarityMultiplier * saved.render.ssaa);
        // The fold radius RenderScene's panoramaLimit gives the same view.
        const double pixelAngle = projection == FrtProjectionMethod::PERSPECTIVE_360
                                      ? fov / w
                                      : std::min(std::numbers::pi / h, 2.0 * std::numbers::pi / w);
        result.projection = projection;
        result.yaw = static_cast<float>(calc.rotation * toRadians);
        result.pitch = static_cast<float>(std::clamp(static_cast<double>(calc.panoramaPitch), -90.0, 90.0) * toRadians);
        result.halfFovTangent = static_cast<float>(std::tan(fov * 0.5));
        result.limit = static_cast<float>(std::min(std::pow(10.0, std::max(static_cast<double>(calc.panoramaRange), 0.0)),
                                                         4.0 / std::max(pixelAngle, 1e-12)));
        result.layout = calc.panoramaLayout;
        return result;
    }

    inline Attribute cameraSourceAttribute(const Attribute &attribute, const std::filesystem::path &directory) {
        Attribute result = attribute;
        result.video.data.sourceScale = readCameraSourceScale(directory);
        result.video.data.keyframeProjection = readKeyframeProjection(directory, attribute);
        if (result.render.ssaa > std::numeric_limits<uint32_t>::max() / result.video.data.sourceScale) {
            throw std::runtime_error("Camera source scale exceeds the supported supersampling range.");
        }
        result.render.ssaa *= result.video.data.sourceScale;
        return result;
    }
}
