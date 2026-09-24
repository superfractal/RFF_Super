//
// Modified by GPT-6 on 2026-09-14, 2026-09-15, 2026-09-18, 2026-09-21, 2026-09-23, 2026-09-24
//

#pragma once
#include "AttributeFormModel.hpp"
#include "FormValues.hpp"
#include "../../attr/NumericSettingLimits.hpp"

namespace merutilm::rff2::workspace {
    inline std::wstring frozenIterationsText(const Attribute &attributes) {
        std::wstring text;
        for (double value : attributes.shader.palette.staticColorIterations) {
            if (!text.empty()) {
                text += L", ";
            }
            text += AttributeFormModel::number(value);
        }
        return text;
    }

    inline bool setFrozenIterations(Attribute &attributes, const std::wstring &text) {
        std::vector<double> values;
        if (text.find_first_not_of(L" \t\r\n") != std::wstring::npos) {
            size_t start = 0;
            while (start < text.size()) {
                const auto end = text.find(L',', start);
                const auto token = std::wstring_view(text).substr(
                    start, end == std::wstring::npos ? text.size() - start : end - start);
                double value;
                if (!AttributeFormModel::parse(token, value) || value < 0 ||
                    values.size() >= ShdPaletteAttribute::MAX_STATIC_COLORS) {
                    return false;
                }
                values.push_back(value);
                if (end == std::wstring::npos) {
                    break;
                }
                start = end + 1;
                if (start == text.size()) {
                    return false;
                }
            }
        }
        attributes.shader.palette.staticColorIterations = std::move(values);
        return true;
    }

    inline void addAnimationGuidance(WorkspaceForm &form) {
        form.inspect = [fields = form.fields](const FormDraft &draft) {
            FormFeedback feedback;
            const FormValues values(fields, draft);
            const auto mode = values.number<int>("animation.mode");
            if (mode && *mode == int(ShdPaletteAnimationMode::LINEAR)) {
                feedback.hints["animation.flowAmount"] =
                    L"Linear motion uses Color Animation Speed. Choose Breathing, Turbulence or Psychedelic "
                    L"to use Flow Amount.";
            }
            if (mode && *mode != int(ShdPaletteAnimationMode::PSYCHEDELIC)) {
                feedback.hints["animation.swirl"] = L"Swirl is used by Psychedelic motion. This value is "
                                                    L"kept while another Animation Mode is selected.";
            }
            const auto padding = values.number<int>("animation.cameraPadding");
            if (padding && !*padding) {
                feedback.hints["animation.paddingScale"] = L"Rotation / 360 Padding is Off. Enable it to use "
                                                           L"this scale when generating new keyframes.";
            }
            return feedback;
        };
    }
    inline std::shared_ptr<AttributeFormModel> animationModel(std::function<Attribute &()> getter,
                                                              std::function<void()> changed) {
        auto model = std::make_shared<AttributeFormModel>(std::move(getter), std::move(changed));
        const float limit = std::numeric_limits<float>::max();
        model->numeric(
            "animation.speed", 0, L"Color Animation Speed",
            L"Palette iterations per second. Negative reverses direction; 0 stops drift.",
            [](auto &attributes) -> auto & { return attributes.shader.palette.animationSpeed; }, -limit,
            limit);
        model->choice("animation.mode", 0, L"Animation Mode",
                      L"Linear, Breathing, Turbulence or Psychedelic motion.",
                      [](auto &attributes) -> auto & { return attributes.shader.palette.animationMode; });
        model->numeric(
            "animation.flowAmount", 0, L"Flow Amount",
            L"Displacement of the color cycle in iterations; 0 or greater.",
            [](auto &attributes) -> auto & { return attributes.shader.palette.animationFlowAmount; }, 0.f,
            limit);
        model->numeric(
            "animation.flowScale", 0, L"Flow Scale", L"Density of fluid bands, 0 to 12.",
            [](auto &attributes) -> auto & { return attributes.shader.palette.animationFlowScale; }, 0.f,
            12.f);
        model->numeric(
            "animation.flowSpeed", 0, L"Flow Speed", L"-2 to 2. Negative values reverse the flow.",
            [](auto &attributes) -> auto & { return attributes.shader.palette.animationFlowSpeed; }, -2.f,
            2.f);
        model->numeric(
            "animation.swirl", 0, L"Swirl", L"Twist around the center, -2 to 2. Used by Psychedelic motion.",
            [](auto &attributes) -> auto & { return attributes.shader.palette.animationFlowSwirl; }, -2.f,
            2.f);
        model->choice("animation.smoothing", 0, L"Color Smoothing",
                      L"Animation edits enable Normal smoothing when currently None.",
                      [](auto &attributes) -> auto & { return attributes.shader.palette.colorSmoothing; });
        model->numeric(
            "animation.offset", 0, L"Palette Start Offset",
            L"Starting position in the palette cycle, 0 to 1.",
            [](auto &attributes) -> auto & { return attributes.shader.palette.offsetRatio; }, 0.f, 1.f);
        model->numeric(
            "animation.freezeTolerance", 1, L"Freeze Match Tolerance",
            L"Fraction of one color cycle, 0 to 1. Lower values freeze a narrower band.",
            [](auto &attributes) -> auto & { return attributes.shader.palette.staticColorTolerance; }, 0.f,
            1.f);
        model->text(
            "animation.frozenIterations", 1, L"Frozen Iteration Values",
            L"Up to 16 comma-separated iteration values. Empty removes all frozen colors.",
            frozenIterationsText, setFrozenIterations);
        model->numeric(
            "animation.zoomSpeed", 2, L"Zoom Speed", L"Keyframes per second; greater than 0.",
            [](auto &attributes) -> auto & { return attributes.video.animation.mps; },
            std::numeric_limits<float>::min(), limit);
        model->numeric(
            "animation.overZoom", 2, L"Extra Final Zoom-in",
            L"Additional zoom at the end of the video; 0 to 8.",
            [](auto &attributes) -> auto & { return attributes.video.animation.overZoom; },
            NumericSettingLimits::minimumOverZoom, NumericSettingLimits::maximumOverZoom);
        model->choice(
            "animation.showZoom", 2, L"Show Zoom Ratio", L"Adds the zoom readout to video output.",
            [](auto &attributes) -> auto & { return attributes.video.timeline.zoomOverlay.visible; });
        model->choice("animation.timelineEnabled", 3, L"Timeline Enabled",
                      L"Uses the existing keyframe tracks during video playback and export.",
                      [](auto &attributes) -> auto & { return attributes.video.timeline.enabled; });
        model->numeric(
            "animation.estimatedKeyframes", 3, L"Estimated Keyframes",
            L"Preview length before loading keyframes: 1 to 100000.",
            [](auto &attributes) -> auto & { return attributes.video.timeline.estimateKeyframes; },
            NumericSettingLimits::estimatedKeyframes.minimum, NumericSettingLimits::estimatedKeyframes.maximum);
        model->numeric(
            "animation.keyframeStep", 4, L"Zoom Step per Keyframe", L"Logarithmic zoom step; greater than 1.",
            [](auto &attributes) -> auto & { return attributes.video.data.defaultZoomIncrement; },
            std::nextafter(1.f, 2.f), limit);
        model->choice("animation.cameraPadding", 4, L"Rotation / 360 Padding",
                      L"Generates larger planar keyframes for camera movement.",
                      [](auto &attributes) -> auto & { return attributes.video.data.cameraPadding; });
        model->numeric(
            "animation.paddingScale", 4, L"Camera Padding Scale",
            L"2 to 64. Memory and disk use grow with the square of this value.",
            [](auto &attributes) -> auto & { return attributes.video.data.cameraScale; }, uint32_t(2),
            uint32_t(64));
        model->choice("animation.pngSource", 4, L"Render from PNG Images",
                      L"PNG sources support finishing effects; iteration-based effects require maps.",
                      [](auto &attributes) -> auto & { return attributes.video.data.isStatic; });
        model->choice("animation.rotationMode", 5, L"Rotation Mode",
                      L"Constant Period replaces rotation keys and keeps turning during zoom holds. Other "
                      L"camera tracks still apply.",
                      [](auto &attributes) -> auto & { return attributes.video.timeline.rotationMode; });
        model->numeric(
            "animation.rotationPeriod", 5, L"Seconds per Turn",
            L"Duration of one complete turn, 0.01 to 86400 seconds. Used by Constant Period.",
            [](auto &attributes) -> auto & { return attributes.video.timeline.rotationPeriod; }, 0.01f,
            86400.f);
        model->choice("animation.rotationDirection", 5, L"Rotation Direction",
                      L"Direction of continuous rotation. Used by Constant Period.",
                      [](auto &attributes) -> auto & { return attributes.video.timeline.rotationDirection; });
        model->numeric(
            "animation.rotationStartAngle", 5, L"Rotation Start Angle",
            L"Angle at video time zero, in degrees. Used by Constant Period.",
            [](auto &attributes) -> auto & { return attributes.video.timeline.rotationStartAngle; },
            -360000.f, 360000.f);
        model->numeric(
            "animation.cameraRotation", 5, L"Camera Rotation",
            L"Degrees, -360000 to 360000. Values beyond 360 allow multiple turns.",
            [](auto &attributes) -> auto & { return attributes.shader.camera.rotation; }, -360000.f,
            360000.f);
        model->choice("animation.cameraProjection", 5, L"Camera Projection",
                      L"Video projection; rotation and 360 modes require padded keyframes.",
                      [](auto &attributes) -> auto & { return attributes.shader.camera.projection; });
        model->numeric(
            "animation.cameraPitch", 5, L"Camera Pitch", L"-90 to 90 degrees.",
            [](auto &attributes) -> auto & { return attributes.shader.camera.pitch; }, -90.f, 90.f);
        model->numeric(
            "animation.cameraFov", 5, L"Camera Field of View", L"1 to 179 degrees.",
            [](auto &attributes) -> auto & { return attributes.shader.camera.fov; }, 1.f, 179.f);
        model->numeric(
            "animation.cameraRange", 5, L"Camera Panorama Range",
            L"Log10 radius limit, 0 to 6. Available detail depends on saved padding.",
            [](auto &attributes) -> auto & { return attributes.shader.camera.range; }, 0.f, 6.f);
        model->choice("animation.cameraLayout", 5, L"Camera Layout", L"Ground and Sky or Full Sphere.",
                      [](auto &attributes) -> auto & { return attributes.shader.camera.layout; });
        model->setNormalizer([](const Attribute &before, Attribute &after, const FormDraft &draft) {
            auto &palette = after.shader.palette;
            if (palette.animationMode != before.shader.palette.animationMode) {
                const auto setDefaultUnlessProvided = [&](const char *id, float &field, float value) {
                    if (!draft.contains(id)) {
                        field = value;
                    }
                };
                if (palette.animationMode == ShdPaletteAnimationMode::BREATHING) {
                    setDefaultUnlessProvided("animation.flowAmount", palette.animationFlowAmount, 1);
                    setDefaultUnlessProvided("animation.flowSpeed", palette.animationFlowSpeed, .5f);
                }
                if (palette.animationMode == ShdPaletteAnimationMode::TURBULENCE ||
                    palette.animationMode == ShdPaletteAnimationMode::PSYCHEDELIC) {
                    setDefaultUnlessProvided("animation.flowAmount", palette.animationFlowAmount, 3);
                    setDefaultUnlessProvided("animation.flowScale", palette.animationFlowScale, 1);
                    setDefaultUnlessProvided("animation.flowSpeed", palette.animationFlowSpeed, 0);
                    setDefaultUnlessProvided("animation.swirl", palette.animationFlowSwirl, 0);
                }
            }
            const bool changesAnimationShape =
                draft.contains("animation.mode") || draft.contains("animation.flowAmount") ||
                draft.contains("animation.flowScale") || draft.contains("animation.flowSpeed") ||
                draft.contains("animation.swirl");
            if (!draft.contains("animation.smoothing") &&
                (draft.contains("animation.speed") ||
                 (changesAnimationShape && palette.animationMode != ShdPaletteAnimationMode::LINEAR)) &&
                palette.colorSmoothing == ShdPalColorSmoothingMethod::NONE) {
                palette.colorSmoothing = ShdPalColorSmoothingMethod::NORMAL;
            }
        });
        return model;
    }
} // namespace merutilm::rff2::workspace
