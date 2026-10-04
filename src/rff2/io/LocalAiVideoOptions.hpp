//
// Modified by GPT-6 on 2026-09-26, 2026-09-27, 2026-09-30
//

#pragma once
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace merutilm::rff2 {
    struct LocalAiVideoOptions {
        bool limitZoom = false;
        float maxLogZoom = 100;
        int explorationSteps = 3;
        double zoomFactor = 2;
        bool locateMinibrot = false;
        bool retryLocate = true;
        float retryDecrease = 0.5f;
        bool improveAppearance = true;
        bool appearanceBeforeZoom = false;
        bool paletteOnly = true;
        bool randomSmooth = true;
        int maxChanges = 3;
        float colorAnimationSpeed = 0;
        int colorAnimationMode = 0;

        static LocalAiVideoOptions read(const nlohmann::json &value) {
            if (!value.is_object()) throw std::runtime_error("Automatic video options must be an object.");
            LocalAiVideoOptions result;
            result.limitZoom = value.value("limit_zoom", false);
            result.maxLogZoom = value.value("max_log_zoom", 100.f);
            result.zoomFactor = value.value("zoom_factor", 2.0);
            result.locateMinibrot = value.value("locate_minibrot", false);
            result.retryLocate = value.value("retry_locate", true);
            result.retryDecrease = value.value("retry_decrease", 0.5f);
            result.improveAppearance = value.value("ai_appearance_enabled", true);
            result.appearanceBeforeZoom = value.value("appearance_before_zoom", false);
            result.paletteOnly = value.value("palette_only", true);
            result.randomSmooth = value.value("random_smooth_palette", true);
            result.colorAnimationSpeed = value.value("color_animation_speed", 0.f);
            if (value.contains("color_animation_mode") && !value.at("color_animation_mode").is_number_integer())
                throw std::runtime_error("Invalid color animation mode.");
            const auto mode = value.value("color_animation_mode", int64_t(-1));
            if (mode != -1 && mode != 0 && mode != 1 && mode != 3 && mode != 4)
                throw std::runtime_error("Invalid color animation mode.");
            result.colorAnimationMode = 0;
            if (!std::isfinite(result.colorAnimationSpeed)) throw std::runtime_error("Color animation speed must be finite.");
            for (const auto key : {"exploration_steps", "max_changes"})
                if (value.contains(key) && !value.at(key).is_number_integer())
                    throw std::runtime_error("Exploration steps and Max changes must be whole numbers.");
            const auto steps = value.value("exploration_steps", int64_t(3));
            const auto changes = value.value("max_changes", int64_t(3));
            if (steps < 1 || steps > 1000 || changes < 0 || changes > 10)
                throw std::runtime_error("Exploration steps must be 1-1000; Max changes must be 0-10.");
            result.explorationSteps = int(steps);
            result.maxChanges = int(changes);
            if (!std::isfinite(result.maxLogZoom) || result.maxLogZoom < 1 ||
                !std::isfinite(result.zoomFactor) || result.zoomFactor <= 1 || result.zoomFactor > 100 ||
                !std::isfinite(result.retryDecrease) || result.retryDecrease <= 0 || result.retryDecrease > 10)
                throw std::runtime_error("Maximum Log Zoom must be at least 1; zoom factor must be >1 to 100; retry decrease must be >0 to 10.");
            return result;
        }
        nlohmann::json json() const {
            return {{"limit_zoom", limitZoom}, {"max_log_zoom", maxLogZoom},
                    {"exploration_steps", explorationSteps}, {"zoom_factor", zoomFactor},
                    {"locate_minibrot", locateMinibrot}, {"retry_locate", retryLocate},
                    {"retry_decrease", retryDecrease}, {"improve_appearance", improveAppearance},
                    {"ai_appearance_enabled", improveAppearance}, {"palette_only", paletteOnly},
                    {"appearance_before_zoom", appearanceBeforeZoom},
                    {"random_smooth_palette", randomSmooth},
                    {"max_changes", maxChanges}, {"color_animation_speed", colorAnimationSpeed}, {"color_animation_mode", 0}};
        }
        bool reachedLimit(float zoom) const { return limitZoom && zoom >= maxLogZoom; }
        float boundedZoom(float zoom) const { return limitZoom ? std::min(zoom, maxLogZoom) : zoom; }
        double boundedFactor(float zoom, double requested) const {
            if (!limitZoom) return requested;
            if (reachedLimit(zoom)) return 1;
            const double delta = std::min(std::log10(requested), double(maxLogZoom) - double(zoom));
            double factor = std::min(requested, std::pow(10.0, delta));
            while (factor > 1 && static_cast<float>(double(zoom) + std::log10(factor)) > maxLogZoom)
                factor = std::nextafter(factor, 1.0);
            return factor;
        }
        int changesPerAttempt() const { return improveAppearance ? std::max(1, maxChanges) : 1; }
    };
}
