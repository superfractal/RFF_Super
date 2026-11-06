//
// Modified by GPT-6 on 2026-10-01
// Modified by Opus 5.5 on 2026-10-08, 2026-10-09, 2026-10-16, 2026-10-20, 2026-10-23, 2026-10-24, 2026-10-26, 2026-10-29, 2026-11-04, 2026-11-05
//

#pragma once
#include "FormValues.hpp"

namespace merutilm::rff2::workspace {
    // Studio fields only OpenPBR Surface reads.
    inline bool openPbrField(const std::string &id) {
        return id == "studio.coatIor" || id == "studio.coatDarkening" || id == "studio.coatColor" || id == "studio.edgeTint" ||
               id == "studio.fuzz" || id == "studio.fuzzRoughness" || id == "studio.fuzzColor" || id == "studio.filmIor";
    }

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
                if (id.starts_with("effect.") && (member == "dropSize" || member == "direction" || member == "length" || member == "width"))
                    field.dependencies.push_back(prefix + "type");
            }
            if (id.starts_with("render.computeShader.") && id != "render.computeShader.use")
                field.dependencies.push_back("render.computeShader.use");
            if (id == "render.computeShader.stopMpaAfterBatches") field.dependencies.push_back("render.computeShader.ignoreMpa");
            if (id == "bloom.threshold" || id == "bloom.radius" || id == "bloom.softness" || id == "bloom.linearAdd")
                field.dependencies.push_back("bloom.intensity");
            if (id == "color.vignetteShape" || id == "color.vignetteRadius" || id == "color.vignetteRichness") field.dependencies.push_back("color.vignette");
            if (id == "fog.radius" || id == "fog.centerStart" || id == "fog.centerInvert" || id == "fog.rimMask" ||
                id == "fog.rimMaskBoost" || id == "fog.rimBlur")
                field.dependencies.push_back("fog.opacity");
            if (id == "fog.centerInvert") field.dependencies.push_back("fog.centerStart");
            if (id == "fog.rimMaskBoost" || id == "fog.rimBlur") field.dependencies.push_back("fog.rimMask");
            if (id.starts_with("stripe.") && id != "stripe.stripeType") field.dependencies.push_back("stripe.stripeType");
            if (id == "palette.glossColor") field.dependencies.push_back("palette.enableGloss");
            if (id == "palette.escapeAngleAmount") field.dependencies.push_back("palette.escapeAngleColoring");
            if (id == "slope.macroRadius") field.dependencies.push_back("slope.macroRelief");
            if (id == "slope.iridescence") field.dependencies.insert(field.dependencies.end(), {"slope.depth", "slope.opacity", "studio.use"});
            if (id.starts_with("warp.") && id != "warp.enabled") field.dependencies.push_back("warp.enabled");
            if (id == "warp.octaves") field.dependencies.push_back("warp.source");
            if (id == "animation.flowAmount" || id == "animation.flowScale" || id == "animation.flowSpeed" || id == "animation.swirl") field.dependencies.push_back("animation.mode");
            if (id == "slope.reliefAutoZoom" || id == "slope.invertRelief" || id == "slope.reliefDepth" || id == "slope.normalSmooth" || id == "slope.aoRadius" || id == "slope.reliefWaves" || id == "slope.waveFrequency" || id == "slope.reliefWavesCoatOnly" || id == "slope.reliefCap") field.dependencies.push_back("slope.lustreRelief");
            if (id.starts_with("studio.") && id != "studio.use") field.dependencies.push_back("studio.use");
            if (openPbrField(id)) field.dependencies.push_back("studio.model");
            if (id == "studio.coatIor" || id == "studio.coatDarkening" || id == "studio.coatColor") field.dependencies.push_back("studio.clearcoat");
            if (id == "studio.edgeTint") field.dependencies.push_back("studio.metalness");
            if (id == "studio.fuzzRoughness" || id == "studio.fuzzColor") field.dependencies.push_back("studio.fuzz");
            if (id == "studio.filmIor") field.dependencies.push_back("slope.iridescence");
            if (id == "slope.coatSoftbox") field.dependencies.insert(field.dependencies.end(), {"studio.use", "studio.clearcoat"});
            if (id == "slope.studioEnvironmentSource") field.dependencies.push_back("studio.use");
            if (id == "slope.environmentBrightness") field.dependencies.insert(field.dependencies.end(), {"studio.use", "slope.studioEnvironmentSource"});
            if (id.starts_with("slope.environmentStop")) field.dependencies.insert(field.dependencies.end(), {"studio.use", "slope.studioEnvironmentSource"});
            if (id == "palette.bandLineGroove" || id == "palette.bandLineCount" ||
                id == "palette.bandLineWidth" || id == "palette.bandLineOpacity" ||
                id == "palette.bandLineSoftness" || id == "palette.bandLineColor" ||
                id == "palette.grooveAuto" || id == "palette.grooveSmooth" || id == "palette.grooveDepth" || id == "palette.grooveWidth" ||
                id == "palette.grooveRounded" || id == "palette.grooveCreaseBlur")
                field.dependencies.push_back("palette.bandLineEnabled");
            if (id == "palette.bandLineWidth" || id == "palette.bandLineOpacity" ||
                id == "palette.bandLineSoftness" || id == "palette.bandLineColor" ||
                id == "palette.grooveAuto" || id == "palette.grooveSmooth" || id == "palette.grooveDepth" || id == "palette.grooveWidth" ||
                id == "palette.grooveRounded" || id == "palette.grooveCreaseBlur")
                field.dependencies.push_back("palette.bandLineGroove");
            if (id == "palette.grooveCreaseBlur") field.dependencies.push_back("palette.grooveRounded");
            if (id == "palette.grooveAuto" || id == "palette.grooveSmooth" || id == "palette.grooveWidth")
                field.dependencies.push_back("palette.grooveRounded");
            if (id == "slope.specularPower" || id == "slope.specularColor" ||
                id == "slope.specularIndependent" || id == "slope.specularZenith" ||
                id == "slope.specularAzimuth" || id == "slope.specularAnisotropy" ||
                id == "slope.specularAnisotropyAngle" || id == "slope.specularAnisotropyDirection" || id == "slope.reliefResponse") {
                field.dependencies.insert(field.dependencies.end(), {"slope.depth", "slope.opacity", "slope.specularIntensity"});
            }
            if (id == "slope.specularAnisotropyDirection") field.dependencies.insert(field.dependencies.end(), {"studio.use", "slope.specularAnisotropy"});
            if (id == "slope.specularPower" || id == "slope.specularColor" ||
                id == "slope.specularIndependent" || id == "slope.specularZenith" || id == "slope.specularAzimuth")
                field.dependencies.push_back("studio.use");
            if (id == "slope.specularColor" || id == "slope.specularIndependent" ||
                id == "slope.specularZenith" || id == "slope.specularAzimuth")
                field.dependencies.push_back("studio.clearcoat");
            if (id == "slope.specularZenith" || id == "slope.specularAzimuth")
                field.dependencies.push_back("slope.specularIndependent");
            if (id == "slope.specularAnisotropyAngle") field.dependencies.push_back("slope.specularAnisotropy");
            if (id.starts_with("palette.orbitTrap") && id != "palette.orbitTrapColoring")
                field.dependencies.push_back("palette.orbitTrapColoring");
            if (id == "palette.orbitTrapRadius" || id == "palette.orbitTrapAngle" || id == "palette.orbitTrapSides")
                field.dependencies.push_back("palette.orbitTrapShape");
            if (id == "palette.orbitTrapArms" || id == "palette.orbitTrapDensity") field.dependencies.push_back("palette.orbitTrapMapping");
            if (id == "palette.orbitTrapGlowSharpness") field.dependencies.push_back("palette.orbitTrapRelief");
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
                    if (id.starts_with("effect.")) {
                        // The Effect Type entry packs the type with its Rain Shape or style as type + 8 * shape-or-style.
                        const int variant = values.number<int>(prefix + "type").value_or(0);
                        const auto type = ShdEffectType(variant % 8);
                        const bool raindrops = type == ShdEffectType::RAIN && variant / 8 == int(ShdRainShape::DROPS);
                        if (member == "dropSize") return raindrops;
                        if (member == "direction") return type == ShdEffectType::RAIN || type == ShdEffectType::EMBERS;
                        if (member == "length" || member == "width") return !raindrops;
                    }
                }
                if (id.starts_with("render.computeShader.") && id != "render.computeShader.use") {
                    if (!on("render.computeShader.use")) return false;
                    return id != "render.computeShader.stopMpaAfterBatches" || !on("render.computeShader.ignoreMpa");
                }
                if (id == "bloom.threshold" || id == "bloom.radius" || id == "bloom.softness" || id == "bloom.linearAdd")
                    return number("bloom.intensity") > 0;
                if (id == "color.vignetteShape" || id == "color.vignetteRadius" || id == "color.vignetteRichness") return number("color.vignette") > 0;
                if (id == "fog.radius" || id == "fog.centerStart" || id == "fog.centerInvert" || id == "fog.rimMask" ||
                    id == "fog.rimMaskBoost" || id == "fog.rimBlur") {
                    if (number("fog.opacity") <= 0) return false;
                    if (id == "fog.centerInvert") return number("fog.centerStart") > 0;
                    if (id == "fog.rimMaskBoost" || id == "fog.rimBlur") return number("fog.rimMask") > 0;
                    return true;
                }
                if (id.starts_with("stripe.") && id != "stripe.stripeType")
                    return values.number<int>("stripe.stripeType").value_or(0) != int(ShdStripeType::NONE);
                if (id == "palette.glossColor") return on("palette.enableGloss");
                if (id == "palette.escapeAngleAmount")
                    return values.number<int>("palette.escapeAngleColoring").value_or(0) != int(ShdPalEscapeAngleColoring::NONE);
                if (id == "slope.macroRadius") return number("slope.macroRelief") > 0;
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
                if (id == "palette.grooveAuto" || id == "palette.grooveSmooth" || id == "palette.grooveWidth")
                    return on("palette.bandLineEnabled") && on("palette.bandLineGroove") && !on("palette.grooveRounded");
                if (id == "palette.grooveDepth" || id == "palette.grooveRounded")
                    return on("palette.bandLineEnabled") && on("palette.bandLineGroove");
                if (id == "palette.grooveCreaseBlur")
                    return on("palette.bandLineEnabled") && on("palette.bandLineGroove") && on("palette.grooveRounded");
                if (id.starts_with("palette.orbitTrap") && id != "palette.orbitTrapColoring") {
                    const int coloring = values.number<int>("palette.orbitTrapColoring").value_or(0);
                    if (coloring == int(ShdPalOrbitTrapColoring::NONE)) return false;
                    const bool shapeColoring = coloring == int(ShdPalOrbitTrapColoring::SHAPE);
                    const bool angleMapping = shapeColoring &&
                        values.number<int>("palette.orbitTrapMapping").value_or(0) == int(ShdPalOrbitTrapMapping::ANGLE);
                    if (id == "palette.orbitTrapShape" || id == "palette.orbitTrapCenterReal" ||
                        id == "palette.orbitTrapCenterImag" || id == "palette.orbitTrapMapping") return shapeColoring;
                    if (id == "palette.orbitTrapRadius" || id == "palette.orbitTrapAngle" || id == "palette.orbitTrapSides") {
                        if (!shapeColoring) return false;
                        const auto shape = static_cast<ShdPalOrbitTrapShape>(
                            values.number<int>("palette.orbitTrapShape").value_or(int(ShdPalOrbitTrapShape::POINT)));
                        if (id == "palette.orbitTrapRadius") return orbitTrapShapeUsesRadius(shape);
                        if (id == "palette.orbitTrapSides") return orbitTrapShapeUsesSides(shape);
                        return orbitTrapShapeUsesAngle(shape);
                    }
                    if (id == "palette.orbitTrapArms") return angleMapping;
                    if (id == "palette.orbitTrapDensity") return !angleMapping;
                    const bool cellular = coloring == int(ShdPalOrbitTrapColoring::CELLULAR);
                    if (id == "palette.orbitTrapRelief") return !cellular;
                    if (id == "palette.orbitTrapGlowSharpness") return cellular || !on("palette.orbitTrapRelief");
                    if (id == "palette.orbitTrapCellBorderWidth" || id == "palette.orbitTrapCellBorderDarkness") return cellular;
                    return true;
                }
                if (id == "fog.focusRatio" || id == "fog.focusRange" || id == "fog.focusFalloff" || id == "fog.focusBlur" ||
                    id == "fog.focusSmoothKeyframes")
                    return number("fog.focusAmount") > 0;
                const bool slope = number("slope.depth", 1) > 0 && number("slope.opacity", 1) > 0;
                const bool studio = slope && on("studio.use");
                const bool specular = slope && number("slope.specularIntensity") > 0;
                const bool reflection = specular || (studio && number("studio.clearcoat") > 0);
                if (openPbrField(id)) {
                    if (!studio || values.number<int>("studio.model").value_or(0) != int(ShdStudioMaterialModel::OPENPBR_SURFACE)) return false;
                    if (id == "studio.coatIor" || id == "studio.coatDarkening" || id == "studio.coatColor") return number("studio.clearcoat") > 0;
                    if (id == "studio.edgeTint") return number("studio.metalness") > 0;
                    if (id == "studio.fuzzRoughness" || id == "studio.fuzzColor") return number("studio.fuzz") > 0;
                    if (id == "studio.filmIor") return number("slope.iridescence") > 0;
                    return true;
                }
                if (id.starts_with("studio.") && id != "studio.use") return studio;
                if (id == "slope.coatSoftbox") return studio && number("studio.clearcoat") > 0;
                if (id == "slope.studioEnvironmentSource" || id == "slope.iridescence") return studio;
                if (id == "slope.environmentBrightness")
                    return studio && values.number<int>("slope.studioEnvironmentSource").value_or(0) != int(ShdStudioEnvironmentSource::TWO_COLOR);
                if (id.starts_with("slope.environmentStop"))
                    return studio && values.number<int>("slope.studioEnvironmentSource").value_or(0) == int(ShdStudioEnvironmentSource::GRADIENT);
                if (id == "slope.specularPower") return specular && !on("studio.use");
                if (id == "slope.specularColor" || id == "slope.specularIndependent") return reflection;
                if (id == "slope.specularZenith" || id == "slope.specularAzimuth") return reflection && on("slope.specularIndependent");
                if (id == "slope.specularAnisotropy" || id == "slope.reliefResponse") return specular;
                if (id == "slope.specularAnisotropyAngle") return specular && number("slope.specularAnisotropy") > 0;
                if (id == "slope.specularAnisotropyDirection") return specular && studio && number("slope.specularAnisotropy") > 0;
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
                    id == "slope.reliefWaves" || id == "slope.waveFrequency" || id == "slope.reliefWavesCoatOnly" || id == "slope.reliefCap") return slope && on("slope.lustreRelief");
                return true;
            };
        }
    }
}
