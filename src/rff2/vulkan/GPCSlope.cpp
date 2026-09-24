//
// Created by Merutilm on 2025-08-15.
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 4.8 on 2026-07-05
// Modified by Opus 5 on 2026-08-08, 2026-08-15, 2026-08-16, 2026-08-17, 2026-08-20, 2026-08-22, 2026-08-29
// Modified by GPT-5 on 2026-08-16, 2026-08-21
// Modified by ox-alpha on 2026-08-22
// Modified by Fable 5.1 on 2026-09-02
// Modified by GPT-6 on 2026-09-10, 2026-09-11, 2026-09-12, 2026-09-13, 2026-09-16, 2026-09-18, 2026-09-20, 2026-09-23, 2026-09-24
//

#include "GPCSlope.hpp"

#include "RCC1.hpp"
#include "SharedDescriptorTemplate.hpp"
#include "../../vulkan_helper/repo/GlobalSamplerRepo.hpp"
#include "../constants/VulkanWindowConstants.hpp"

namespace merutilm::rff2 {
    void GPCSlope::updateQueue(vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {

        //no operation
    }


    void GPCSlope::setEffects(const ShdEffectsAttribute &effects, const bool sceneLinear, const bool hasIterations) const {
        constexpr uint32_t VECTORS_PER_EFFECT_LAYER = 6;
        constexpr uint32_t CONTEXT_VECTOR = EFFECT_LAYER_COUNT * VECTORS_PER_EFFECT_LAYER;
        constexpr uint32_t FIRST_RAIN_VECTOR = CONTEXT_VECTOR + 1;
        const auto &ubo = *getDescriptor(SET_EFFECTS).get<vkh::Uniform>(0, 0);
        auto &host = ubo.getHostObject();
        for (uint32_t i = 0; i < EFFECT_LAYER_COUNT; ++i) {
            const auto &e = effects[i];
            const uint32_t layerVector = i * VECTORS_PER_EFFECT_LAYER;
            host.set<glm::vec4>(layerVector, {e.enabled ? 1.0f : 0.0f, float(e.type), float(e.blend), float(e.mask)});
            host.set<glm::vec4>(layerVector + 1, {e.opacity, e.scale, e.speed, e.evolution});
            host.set<glm::vec4>(layerVector + 2, {e.density, e.length, e.width, e.direction});
            host.set<glm::vec4>(layerVector + 3, {e.distortion, e.glow, e.seed, e.period});
            host.set<glm::vec4>(layerVector + 4, e.color);
            host.set<glm::vec4>(layerVector + 5, e.secondary);
            host.set<glm::vec4>(FIRST_RAIN_VECTOR + i, {float(e.rainShape), e.dropSize, 0.0f, 0.0f});
        }
        host.set<glm::vec4>(CONTEXT_VECTOR, {sceneLinear ? 1.0f : 0.0f, hasIterations ? 1.0f : 0.0f, 0.0f, 0.0f});
        ubo.update();
    }

    void GPCSlope::setSlope(const ShdSlopeAttribute &slope) const {
        using namespace SharedDescriptorTemplate;
        auto &slopeDesc = getDescriptor(SET_SLOPE);
        const auto &slopeUBO = slopeDesc.get<vkh::Uniform>(0, DescSlope::BINDING_UBO_SLOPE);
        auto &slopeUBOHost = slopeUBO->getHostObject();
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_DEPTH, slope.depth);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_REFLECTION_RATIO, slope.reflectionRatio);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_OPACITY, slope.opacity);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_ZENITH, slope.zenith);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_AZIMUTH, slope.azimuth);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_INTENSITY, slope.specularIntensity);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_POWER, slope.specularPower);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_RIM_INTENSITY, slope.rimIntensity);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_RIM_POWER, slope.rimPower);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_BRIGHTNESS, slope.brightness);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GAMMA, slope.gamma);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_RIM_COLOR_R, slope.rimColor.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_RIM_COLOR_G, slope.rimColor.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_RIM_COLOR_B, slope.rimColor.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_COLOR_R, slope.specularColor.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_COLOR_G, slope.specularColor.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_COLOR_B, slope.specularColor.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_AO_INTENSITY, slope.aoIntensity);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_AMBIENT_INTENSITY, slope.ambientIntensity);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SKY_COLOR_R, slope.skyColor.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SKY_COLOR_G, slope.skyColor.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SKY_COLOR_B, slope.skyColor.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GROUND_COLOR_R, slope.groundColor.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GROUND_COLOR_G, slope.groundColor.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GROUND_COLOR_B, slope.groundColor.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_LINK, slope.specularIndependent ? 0.0f : 1.0f);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_ZENITH, slope.specularZenith);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_AZIMUTH, slope.specularAzimuth);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_ANISOTROPY, slope.specularAnisotropy);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SPECULAR_ANISOTROPY_ANGLE, slope.specularAnisotropyAngle);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_MACRO_RELIEF, slope.macroRelief);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_MACRO_RADIUS, slope.macroRadius);
        // Carried as a float like specular_link; the shader rounds it back.
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SHADING_BLEND,
                                static_cast<float>(static_cast<int32_t>(slope.shadingBlend)));
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_RELIEF_RESPONSE, slope.reliefResponse);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_TERMINATOR_SOFTNESS, slope.terminatorSoftness);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_HIGHLIGHT_KNEE, slope.highlightKnee);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_LIGHT_BLEND,
                                static_cast<float>(static_cast<int32_t>(slope.lightBlend)));
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_LUMA_AMOUNT, slope.lumaAmount);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_TINT_RESPONSE, slope.tintResponse);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_SHADOW_CHROMA, slope.shadowChroma);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_TINT_BLEND,
                                static_cast<float>(static_cast<int32_t>(slope.tintBlend)));
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_FILL_INTENSITY, slope.fillIntensity);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_FILL_ZENITH, slope.fillZenith);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_FILL_AZIMUTH, slope.fillAzimuth);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GLOSS_INTENSITY, slope.glossIntensity);
        // Carried as a float like specular_link; the shader rounds it back.
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GLOSS_SOURCE,
                                static_cast<float>(static_cast<int32_t>(slope.glossSource)));
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GLOSS_BANDS, slope.glossBands);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GLOSS_SHARPNESS, slope.glossSharpness);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GLOSS_PHASE, slope.glossPhase);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GLOSS_COLOR_R, slope.glossColor.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GLOSS_COLOR_G, slope.glossColor.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GLOSS_COLOR_B, slope.glossColor.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_SLOPE_GLOSS_RELIEF, slope.glossRelief);
        slopeUBOHost.set<float>(DescSlope::TARGET_STUDIO_USE, slope.studio.use ? 1.0f : 0.0f);
        slopeUBOHost.set<float>(DescSlope::TARGET_STUDIO_ROUGHNESS, slope.studio.roughness);
        slopeUBOHost.set<float>(DescSlope::TARGET_STUDIO_METALNESS, slope.studio.metalness);
        slopeUBOHost.set<float>(DescSlope::TARGET_STUDIO_IOR, slope.studio.ior);
        slopeUBOHost.set<float>(DescSlope::TARGET_STUDIO_DIRECT_INTENSITY, slope.studio.directIntensity);
        slopeUBOHost.set<float>(DescSlope::TARGET_STUDIO_ENVIRONMENT_INTENSITY, slope.studio.environmentIntensity);
        slopeUBOHost.set<float>(DescSlope::TARGET_STUDIO_CLEARCOAT, slope.studio.clearcoat);
        slopeUBOHost.set<float>(DescSlope::TARGET_STUDIO_CLEARCOAT_ROUGHNESS, slope.studio.clearcoatRoughness);
        slopeUBOHost.set<float>(DescSlope::TARGET_SURFACE_IRIDESCENCE, slope.iridescence);
        slopeUBOHost.set<float>(DescSlope::TARGET_SURFACE_FILM_THICKNESS, slope.filmThickness);
        slopeUBOHost.set<float>(DescSlope::TARGET_SURFACE_SPECULAR_AA, slope.specularAA);
        slopeUBOHost.set<float>(DescSlope::TARGET_SURFACE_LUSTRE_RELIEF, slope.lustreRelief ? 1.0f : 0.0f);
        slopeUBOHost.set<float>(DescSlope::TARGET_SURFACE_RELIEF_DEPTH, slope.reliefDepth);
        slopeUBOHost.set<float>(DescSlope::TARGET_SURFACE_NORMAL_SMOOTH, slope.normalSmooth);
        slopeUBOHost.set<float>(DescSlope::TARGET_SURFACE_AO_RADIUS, slope.aoRadius);
        slopeUBOHost.set<float>(DescSlope::TARGET_SURFACE_RELIEF_WAVES, slope.reliefWaves);
        slopeUBOHost.set<float>(DescSlope::TARGET_SURFACE_WAVE_FREQUENCY, slope.waveFrequency);
        slopeUBOHost.set<float>(DescSlope::TARGET_SURFACE_INVERT_RELIEF, slope.invertRelief ? 1.0f : 0.0f);
        slopeUBOHost.set<float>(DescSlope::TARGET_SURFACE_BOUNDARY_GUARD, slope.boundaryGuard);

        slopeUBOHost.set<float>(DescSlope::TARGET_SURFACE_STYLE, float(slope.surfaceStyle));
        slopeUBOHost.set<float>(DescSlope::TARGET_REPLACE_SURFACE_STYLE, slope.replaceSurfaceStyle ? 1.0f : 0.0f);
        slopeUBOHost.set<float>(DescSlope::TARGET_CHROME_STRENGTH, slope.chromeStrength);
        slopeUBOHost.set<float>(DescSlope::TARGET_FILM_STRENGTH, slope.filmStrength);
        slopeUBOHost.set<float>(DescSlope::TARGET_REFLECTION_DETAIL, slope.reflectionDetail);
        slopeUBOHost.set<float>(DescSlope::TARGET_REFLECTION_CONTRAST, slope.reflectionContrast);
        slopeUBOHost.set<float>(DescSlope::TARGET_REFLECTION_BRIGHTNESS, slope.reflectionBrightness);
        slopeUBOHost.set<float>(DescSlope::TARGET_REFLECTION_CURVE, slope.reflectionCurve);
        slopeUBOHost.set<float>(DescSlope::TARGET_SURFACE_PHASE, slope.surfacePhase);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRISM_WIDTH, slope.prismWidth);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRISM_SPREAD, slope.prismSpread);
        slopeUBOHost.set<float>(DescSlope::TARGET_FILM_HUE, slope.filmHue);
        slopeUBOHost.set<float>(DescSlope::TARGET_SPARKLE_STRENGTH, slope.sparkleStrength);
        slopeUBOHost.set<float>(DescSlope::TARGET_BACKGROUND_BRIGHTNESS, slope.backgroundBrightness);
        slopeUBOHost.set<float>(DescSlope::TARGET_SHADOW_CRUSH, slope.shadowCrush);
        slopeUBOHost.set<float>(DescSlope::TARGET_FLAME_STRENGTH, slope.flameStrength);
        slopeUBOHost.set<float>(DescSlope::TARGET_PHONK_RED, slope.phonkRed);
        slopeUBOHost.set<float>(DescSlope::TARGET_FROST_STRENGTH, slope.frostStrength);
        slopeUBOHost.set<float>(DescSlope::TARGET_FROST_THRESHOLD, slope.frostThreshold);
        slopeUBOHost.set<float>(DescSlope::TARGET_GRUNGE_SCALE, slope.grungeScale);
        slopeUBOHost.set<float>(DescSlope::TARGET_PALETTE_COLOR_MIX, slope.paletteColorMix);
        slopeUBOHost.set<float>(DescSlope::TARGET_STYLE_COLOR_MIX, slope.styleColorMix);
        slopeUBOHost.set<float>(DescSlope::TARGET_STYLE_HIGHLIGHT_MIX, slope.styleHighlightMix);
        slopeUBOHost.set<float>(DescSlope::TARGET_STYLE_BACKGROUND_MIX, slope.styleBackgroundMix);
        slopeUBOHost.set<float>(DescSlope::TARGET_STYLE_INK_PRESERVE, slope.styleInkPreserve);
        slopeUBOHost.set<float>(DescSlope::TARGET_STYLE_GLINT_SIZE, slope.styleGlintSize);
        slopeUBOHost.set<float>(DescSlope::TARGET_STYLE_DETAIL_LIGHT, slope.styleDetailLight);
        slopeUBOHost.set<float>(DescSlope::TARGET_STYLE_DETAIL_SUPPRESS, slope.styleDetailSuppress);
        slopeUBOHost.set<float>(DescSlope::TARGET_STYLE_DETAIL_THRESHOLD, slope.styleDetailThreshold);
        slopeUBOHost.set<float>(DescSlope::TARGET_STYLE_DAMAGE, slope.styleDamage);
        slopeUBOHost.set<float>(DescSlope::TARGET_SURFACE_BLEND, float(slope.surfaceBlend));
        slopeUBOHost.set<float>(DescSlope::TARGET_STYLE_COLOR_R, slope.styleColor.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_STYLE_COLOR_G, slope.styleColor.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_STYLE_COLOR_B, slope.styleColor.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_STYLE_HIGHLIGHT_COLOR_R, slope.styleHighlightColor.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_STYLE_HIGHLIGHT_COLOR_G, slope.styleHighlightColor.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_STYLE_HIGHLIGHT_COLOR_B, slope.styleHighlightColor.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_STYLE_BACKGROUND_COLOR_R, slope.styleBackgroundColor.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_STYLE_BACKGROUND_COLOR_G, slope.styleBackgroundColor.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_STYLE_BACKGROUND_COLOR_B, slope.styleBackgroundColor.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_SEA_GLOW, slope.seaGlow);
        slopeUBOHost.set<float>(DescSlope::TARGET_SEA_THRESHOLD, slope.seaThreshold);
        slopeUBOHost.set<float>(DescSlope::TARGET_SEA_BODY, slope.seaBody);
        slopeUBOHost.set<float>(DescSlope::TARGET_SEA_PARTICLES, slope.seaParticles);
        slopeUBOHost.set<float>(DescSlope::TARGET_SEA_PARTICLE_SIZE, slope.seaParticleSize);
        slopeUBOHost.set<float>(DescSlope::TARGET_SEA_BALANCE, slope.seaBalance);
        slopeUBOHost.set<float>(DescSlope::TARGET_UKIYO_COLORS, slope.ukiyoColors);
        slopeUBOHost.set<float>(DescSlope::TARGET_UKIYO_FOAM, slope.ukiyoFoam);
        slopeUBOHost.set<float>(DescSlope::TARGET_UKIYO_GRAIN, slope.ukiyoGrain);
        slopeUBOHost.set<float>(DescSlope::TARGET_UKIYO_FLATNESS, slope.ukiyoFlatness);
        slopeUBOHost.set<float>(DescSlope::TARGET_UKIYO_BALANCE, slope.ukiyoBalance);
        slopeUBOHost.set<float>(DescSlope::TARGET_SEA_BODY_COLOR_R, slope.seaBodyColor.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_SEA_BODY_COLOR_G, slope.seaBodyColor.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_SEA_BODY_COLOR_B, slope.seaBodyColor.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_SEA_GLOW_COLOR_R, slope.seaGlowColor.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_SEA_GLOW_COLOR_G, slope.seaGlowColor.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_SEA_GLOW_COLOR_B, slope.seaGlowColor.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_SEA_ACCENT_COLOR_R, slope.seaAccentColor.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_SEA_ACCENT_COLOR_G, slope.seaAccentColor.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_SEA_ACCENT_COLOR_B, slope.seaAccentColor.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRINT_INK_R, slope.printInk.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRINT_INK_G, slope.printInk.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRINT_INK_B, slope.printInk.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRINT_INDIGO_R, slope.printIndigo.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRINT_INDIGO_G, slope.printIndigo.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRINT_INDIGO_B, slope.printIndigo.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRINT_ASAGI_R, slope.printAsagi.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRINT_ASAGI_G, slope.printAsagi.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRINT_ASAGI_B, slope.printAsagi.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRINT_BLUE_R, slope.printBlue.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRINT_BLUE_G, slope.printBlue.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRINT_BLUE_B, slope.printBlue.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRINT_FOAM_R, slope.printFoam.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRINT_FOAM_G, slope.printFoam.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRINT_FOAM_B, slope.printFoam.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRINT_PAPER_R, slope.printPaper.r);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRINT_PAPER_G, slope.printPaper.g);
        slopeUBOHost.set<float>(DescSlope::TARGET_PRINT_PAPER_B, slope.printPaper.b);
        slopeUBOHost.set<float>(DescSlope::TARGET_LAYER_METAL, slope.layerMetal);
        slopeUBOHost.set<float>(DescSlope::TARGET_LAYER_SIGIL, slope.layerSigil);
        slopeUBOHost.set<float>(DescSlope::TARGET_LAYER_PHONK, slope.layerPhonk);
        slopeUBOHost.set<float>(DescSlope::TARGET_LAYER_FROST, slope.layerFrost);
        slopeUBOHost.set<float>(DescSlope::TARGET_LAYER_SEA, slope.layerSea);
        slopeUBOHost.set<float>(DescSlope::TARGET_LAYER_PRINT, slope.layerPrint);
        slopeUBOHost.set<float>(DescSlope::TARGET_LAYER_COMMON, slope.layerCommon);
        slopeUBOHost.set<float>(DescSlope::TARGET_STUDIO_ENVIRONMENT_ROTATION, slope.studioEnvironmentRotation);
        slopeUBOHost.set<float>(DescSlope::TARGET_STUDIO_ENVIRONMENT_FOLLOW, slope.studioEnvironmentFollow);

        slopeUBO->update();
    }

    void GPCSlope::setGroove(const ShdPaletteAttribute &palette) const {
        using namespace SharedDescriptorTemplate;
        const auto &ubo = getDescriptor(SET_SLOPE).get<vkh::Uniform>(0, DescSlope::BINDING_UBO_SLOPE);
        auto &host = ubo->getHostObject();
        host.set<float>(DescSlope::TARGET_GROOVE_ENABLED, palette.bandLineEnabled && palette.bandLineGroove ? 1.0f : 0.0f);
        host.set<float>(DescSlope::TARGET_GROOVE_DEPTH, palette.grooveDepth);
        host.set<float>(DescSlope::TARGET_GROOVE_WIDTH, palette.grooveWidth);
        host.set<float>(DescSlope::TARGET_GROOVE_AUTO, palette.grooveAuto ? 1.0f : 0.0f);
        host.set<float>(DescSlope::TARGET_GROOVE_COUNT, float(palette.bandLineCount));
        ubo->update();
    }

    void GPCSlope::setReliefZoom(const ShdSlopeAttribute &slope, float logZoom) const {
        using namespace SharedDescriptorTemplate;
        const auto &ubo = getDescriptor(SET_SLOPE).get<vkh::Uniform>(0, DescSlope::BINDING_UBO_SLOPE);
        auto &host = ubo->getHostObject();
        const float value = slope.reliefZoomLog2(logZoom);
        if (host.get<float>(DescSlope::TARGET_SURFACE_ZOOM_LOG2) == value) return;
        wc.core.getLogicalDevice().waitDeviceIdle();
        host.set<float>(DescSlope::TARGET_SURFACE_ZOOM_LOG2, value);
        ubo->update();
    }

    void GPCSlope::pipelineInitialized() {
        using namespace SharedDescriptorTemplate;
        writeDescriptorMF([this](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
            getDescriptor(SET_SLOPE).queue(queue, frameIndex, {}, {DescSlope::BINDING_UBO_SLOPE});
            getDescriptor(SET_EFFECTS).queue(queue, frameIndex, {}, {0});
        });
    }

    void GPCSlope::renderContextRefreshed() {
        auto &sic = wc.getSharedImageContext();
        auto &inputDesc = getDescriptor(SET_PREV_RESULT);

        switch (wc.getAttachmentIndex()) {
            case Constants::VulkanWindow::MAIN_WINDOW_ATTACHMENT_INDEX: {
                const auto &input = sic.getImageContextMF(SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_PRIMARY);
                inputDesc.get<vkh::CombinedImageSampler>(0, BINDING_PREV_RESULT_SAMPLER)->setImageContextMF(input);
                break;
            }
            case Constants::VulkanWindow::VIDEO_PREPARATION_WINDOW_ATTACHMENT_INDEX:
            case Constants::VulkanWindow::VIDEO_WINDOW_ATTACHMENT_INDEX: {
                const auto &input = sic.getImageContextMF(SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_PRIMARY);
                inputDesc.get<vkh::CombinedImageSampler>(0, BINDING_PREV_RESULT_SAMPLER)->setImageContextMF(input);
                break;
            }
            default: {
                //noop
            }
        }

        writeDescriptorMF([&inputDesc](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
            inputDesc.queue(queue, frameIndex, {}, {BINDING_PREV_RESULT_SAMPLER});
        });

    }

    void GPCSlope::configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) {
        ShaderLayerControl::configure(layerPush, pipelineLayoutManager);
        //noop
    }

    void GPCSlope::configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) {
        using namespace SharedDescriptorTemplate;
        auto descManager = vkh::factory::create<vkh::DescriptorManager>();
        vkh::SamplerRef sampler = pickFromGlobalRepository<vkh::GlobalSamplerRepo, vkh::SamplerRef>(
            VkSamplerCreateInfo{
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .magFilter = VK_FILTER_NEAREST,
                .minFilter = VK_FILTER_NEAREST,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                .mipLodBias = 0,
                .anisotropyEnable = VK_FALSE,
                .maxAnisotropy = 0,
                .compareEnable = VK_FALSE,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .minLod = 0,
                .maxLod = 0,
                .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
                .unnormalizedCoordinates = VK_TRUE
            });
        descManager->appendCombinedImgSampler(BINDING_PREV_RESULT_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, vkh::factory::create<vkh::CombinedImageSampler>(wc.core, sampler, true));

        appendUniqueDescriptor(SET_PREV_RESULT, descriptors, std::move(descManager));
        appendDescriptor<DescIteration>(SET_ITERATION, descriptors);
        appendDescriptor<DescSlope>(SET_SLOPE, descriptors);
        appendDescriptor<DescEffects>(SET_EFFECTS, descriptors);
        appendDescriptor<DescTime>(SET_EFFECTS_TIME, descriptors);
        appendDescriptor<DescPalette>(SET_PALETTE, descriptors);
        descriptors.push_back(paletteTextures);
        appendDescriptor<DescTime>(SET_PALETTE_TIME, descriptors);
    }
}
