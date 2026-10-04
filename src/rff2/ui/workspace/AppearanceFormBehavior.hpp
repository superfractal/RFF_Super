//
// Modified by GPT-6 on 2026-10-01
//

#pragma once
#include "FormValues.hpp"

namespace merutilm::rff2::workspace {
    inline void appearanceFormBehavior(WorkspaceForm &form) {
        const auto fields = std::make_shared<std::vector<FormField>>(form.fields);
        for (auto &field : form.fields) {
            const auto &id = field.id;
            if (id.starts_with("texture.") || id.starts_with("pattern.") || id.starts_with("effect.")) {
                const auto split = id.find('.', id.find('.') + 1);
                const auto prefix = id.substr(0, split + 1), member = id.substr(split + 1);
                if (member != "enabled" && member != "path") field.dependencies.push_back(prefix + "enabled");
                if (id.starts_with("pattern.") && member.starts_with("edge") && member != "edgeEnabled") field.dependencies.push_back(prefix + "edgeEnabled");
                if (id.starts_with("pattern.") && (member == "color" || member == "paletteShift")) field.dependencies.push_back(prefix + "inkMode");
            }
            if (id.starts_with("warp.") && id != "warp.enabled") field.dependencies.push_back("warp.enabled");
            if (id == "warp.octaves") field.dependencies.push_back("warp.source");
            if (id == "animation.flowAmount" || id == "animation.flowScale" || id == "animation.flowSpeed" || id == "animation.swirl") field.dependencies.push_back("animation.mode");
            if (id == "slope.reliefAutoZoom" || id == "slope.invertRelief" || id == "slope.reliefDepth" || id == "slope.normalSmooth" || id == "slope.aoRadius" || id == "slope.reliefWaves" || id == "slope.waveFrequency") field.dependencies.push_back("slope.lustreRelief");
            if (id.starts_with("studio.") && id != "studio.use") field.dependencies.push_back("studio.use");
            if (id == "palette.bandLineGroove" || id == "palette.bandLineCount" ||
                id == "palette.bandLineWidth" || id == "palette.bandLineOpacity" ||
                id == "palette.bandLineSoftness" || id == "palette.bandLineColor" ||
                id == "palette.grooveAuto" || id == "palette.grooveDepth" || id == "palette.grooveWidth")
                field.dependencies.push_back("palette.bandLineEnabled");
            if (id == "palette.bandLineWidth" || id == "palette.bandLineOpacity" ||
                id == "palette.bandLineSoftness" || id == "palette.bandLineColor" ||
                id == "palette.grooveAuto" || id == "palette.grooveDepth" || id == "palette.grooveWidth")
                field.dependencies.push_back("palette.bandLineGroove");
            if (id == "slope.specularPower" || id == "slope.specularColor" ||
                id == "slope.specularIndependent" || id == "slope.specularZenith" ||
                id == "slope.specularAzimuth" || id == "slope.specularAnisotropy" ||
                id == "slope.specularAnisotropyAngle" || id == "slope.reliefResponse") {
                field.dependencies.insert(field.dependencies.end(), {"slope.depth", "slope.opacity", "slope.specularIntensity"});
            }
            if (id == "slope.specularPower" || id == "slope.specularColor" ||
                id == "slope.specularIndependent" || id == "slope.specularZenith" || id == "slope.specularAzimuth")
                field.dependencies.push_back("studio.use");
            if (id == "slope.specularColor" || id == "slope.specularIndependent" ||
                id == "slope.specularZenith" || id == "slope.specularAzimuth")
                field.dependencies.push_back("studio.clearcoat");
            if (id == "slope.specularZenith" || id == "slope.specularAzimuth")
                field.dependencies.push_back("slope.specularIndependent");
            if (id == "slope.specularAnisotropyAngle") field.dependencies.push_back("slope.specularAnisotropy");
            const auto previous = field.enabled;
            field.enabled = [fields, id = field.id, previous](const FormDraft &draft) {
                if (previous && !previous(draft)) return false;
                const FormValues values(*fields, draft);
                const auto on = [&](const std::string &key) { return values.number<int>(key).value_or(1) != 0; };
                const auto number = [&](const std::string &key, float fallback = 0) { return values.number<float>(key).value_or(fallback); };
                if (id.starts_with("texture.") || id.starts_with("pattern.") || id.starts_with("effect.")) {
                    const auto split = id.find('.', id.find('.') + 1);
                    const auto prefix = id.substr(0, split + 1);
                    const auto member = id.substr(split + 1);
                    if (member != "enabled" && member != "path" && !on(prefix + "enabled")) return false;
                    if (id.starts_with("pattern.") && member.starts_with("edge") && member != "edgeEnabled" &&
                        !on(prefix + "edgeEnabled")) return false;
                    if (id.starts_with("pattern.") && (member == "paletteShift" || member == "color")) {
                        const bool paletteInk = values.number<int>(prefix + "inkMode").value_or(0) == int(ShdPatternInkMode::PALETTE_SHIFT);
                        return member == "paletteShift" ? paletteInk : !paletteInk;
                    }
                }
                if (id.starts_with("warp.") && id != "warp.enabled") {
                    if (!on("warp.enabled")) return false;
                    if (id == "warp.octaves" && values.number<int>("warp.source").value_or(0) != int(ShdWarpSource::NOISE)) return false;
                }
                if (id == "animation.flowAmount" || id == "animation.flowScale" ||
                    id == "animation.flowSpeed" || id == "animation.swirl") {
                    const auto mode = ShdPaletteAnimationMode(values.number<int>("animation.mode").value_or(0));
                    if (mode == ShdPaletteAnimationMode::LINEAR) return false;
                    if (id == "animation.swirl") return mode == ShdPaletteAnimationMode::PSYCHEDELIC;
                    if (id == "animation.flowScale") return mode == ShdPaletteAnimationMode::TURBULENCE || mode == ShdPaletteAnimationMode::PSYCHEDELIC;
                }
                if (id == "palette.bandLineGroove" || id == "palette.bandLineCount")
                    return on("palette.bandLineEnabled");
                if (id == "palette.bandLineWidth" || id == "palette.bandLineOpacity" || id == "palette.bandLineSoftness" || id == "palette.bandLineColor")
                    return on("palette.bandLineEnabled") && !on("palette.bandLineGroove");
                if (id == "palette.grooveAuto" || id == "palette.grooveDepth" || id == "palette.grooveWidth")
                    return on("palette.bandLineEnabled") && on("palette.bandLineGroove");
                if (id == "fog.focusRatio" || id == "fog.focusRange" || id == "fog.focusFalloff" || id == "fog.focusBlur")
                    return number("fog.focusAmount") > 0;
                const bool slope = number("slope.depth", 1) > 0 && number("slope.opacity", 1) > 0;
                const bool studio = slope && on("studio.use");
                const bool specular = slope && number("slope.specularIntensity") > 0;
                const bool reflection = specular || (studio && number("studio.clearcoat") > 0);
                if (id.starts_with("studio.") && id != "studio.use") return studio;
                if (id == "slope.specularPower") return specular && !on("studio.use");
                if (id == "slope.specularColor" || id == "slope.specularIndependent") return reflection;
                if (id == "slope.specularZenith" || id == "slope.specularAzimuth") return reflection && on("slope.specularIndependent");
                if (id == "slope.specularAnisotropy" || id == "slope.reliefResponse") return specular;
                if (id == "slope.specularAnisotropyAngle") return specular && number("slope.specularAnisotropy") > 0;
                if (id == "slope.fillZenith" || id == "slope.fillAzimuth") return slope && number("slope.fillIntensity") > 0;
                if (id == "slope.zenith" || id == "slope.azimuth" || id == "slope.fillIntensity" || id == "slope.shadingBlend") return slope;
                if (id == "slope.gamma" || id == "slope.reflectionRatio" || id == "slope.terminatorSoftness") return slope && number("slope.lumaAmount") > 0;
                if (id == "slope.skyColor" || id == "slope.groundColor") return studio || (slope && number("slope.ambientIntensity") > 0);
                if (id == "slope.tintResponse" || id == "slope.tintBlend" || id == "slope.shadowChroma")
                    return slope && number("slope.ambientIntensity") > 0 && (id != "slope.shadowChroma" || values.number<int>("slope.tintBlend").value_or(0) == int(ShdSlopeTintBlend::OKLAB));
                if (id == "slope.rimPower" || id == "slope.rimColor") return slope && number("slope.rimIntensity") > 0;
                if (id.starts_with("slope.gloss") && id != "slope.glossIntensity")
                    return slope && number("slope.glossIntensity") > 0 && (id != "slope.glossRelief" || values.number<int>("slope.glossSource").value_or(0) == int(ShdSlopeGlossSource::SHADING_FINE));
                if (id == "slope.lightBlend" || id == "slope.highlightKnee") return slope && !on("studio.use");
                if (id == "slope.reliefAutoZoom" || id == "slope.invertRelief" || id == "slope.reliefDepth" || id == "slope.normalSmooth" || id == "slope.aoRadius" ||
                    id == "slope.reliefWaves" || id == "slope.waveFrequency") return slope && on("slope.lustreRelief");
                return true;
            };
        }
    }
}
