//
// Modified by GPT-6 on 2026-09-14, 2026-09-19, 2026-09-20, 2026-09-21, 2026-09-23, 2026-09-24, 2026-09-25, 2026-09-29, 2026-10-01, 2026-10-07
// Modified by Opus 5.5 on 2026-10-03, 2026-10-04, 2026-10-06, 2026-10-07, 2026-10-08, 2026-10-09, 2026-10-12, 2026-10-16, 2026-10-17, 2026-10-20, 2026-10-21, 2026-10-22, 2026-10-24, 2026-10-26, 2026-10-29, 2026-11-01, 2026-11-04, 2026-11-05
//

#include "AppearanceForms.hpp"
#include "AttributeFormSection.hpp"
#include "AppearanceFormBehavior.hpp"
#include "../../attr/NumericSettingLimits.hpp"
#include "../../vulkan/OrderedShaderLayers.hpp"
#include <thread>

namespace merutilm::rff2::workspace {
    namespace {
        using InUseCheck = std::function<bool(const Attribute &)>;
        // The groups listed can be switched off, each on while its check holds; the others are never in use.
        std::vector<std::function<bool()>> groupsInUse(const AttributeGetter &attribute, size_t groupCount,
                                                       std::initializer_list<std::pair<int, InUseCheck>> checks) {
            std::vector<std::function<bool()>> result(groupCount);
            for (const auto &[group, check] : checks) {
                result.at(group) = [attribute, check] { return check(attribute()); };
            }
            return result;
        }
        // One check per layer group, true while the renderer draws that layer.
        std::vector<std::function<bool()>> layersInUse(const AttributeGetter &attribute, ShdLayer first) {
            std::vector<std::function<bool()>> result;
            for (uint32_t i = 0; i < 4; ++i) {
                result.push_back([attribute, layer = ShdLayer(uint32_t(first) + i)] {
                    return shaderLayerActive(attribute().shader, layer);
                });
            }
            return result;
        }
    }
    WorkspaceForm previewPerformanceForm(AttributeGetter attribute, std::function<void()> changed) {
        auto model = std::make_shared<AttributeFormModel>(attribute, std::move(changed));
        using Settings = RenderAttribute;
        AttributeFormSection section(
            model, [](auto &attributes) -> auto & { return attributes.render; }, "render.");
        section.number("fps", &Settings::fps, L"Rendering FPS", NumericSettingLimits::minimumRenderFps,
                       NumericSettingLimits::maximumFps,
                       L"Live rendering limit: 0 to 1000 frames per second; 0 pauses the preview. Lower values reduce rendering load. "
                       L"Video export has a separate FPS setting.");
        section.number("threads", &Settings::threads, L"Calculation Threads", uint32_t(1),
                       std::max(uint32_t(1), std::thread::hardware_concurrency()),
                       L"1 up to this computer's logical core count. Applied to the next calculation.");
        section.group = 1;
        section.choice("boundaryTraceFill", &Settings::boundaryTraceFill, L"Boundary Trace Fill",
                       L"Recomputes the view with boundary tracing enabled or disabled.");
        section.choice("preview2Color", &Settings::preview2Color, L"Two-Color Preview",
                       L"Recomputes the view with a two-color preview.");
        section.choice("coarsePreview", &Settings::coarsePreview, L"Coarse Preview",
                       L"Recomputes the view with an initial coarse preview.");
        using Gpu = RndComputeShaderAttribute;
        AttributeFormSection gpu(
            model, [](auto &attributes) -> auto & { return attributes.render.computeShader; },
            "render.computeShader.", 2);
        gpu.choice("use", &Gpu::use, L"Use GPU Perturbation",
                   L"Iterates on the GPU down to zoom 1e290; deeper zooms, custom formulas and compressed references "
                   L"stay on the CPU.");
        gpu.choice("singlePrecision", &Gpu::singlePrecision, L"Single Precision",
                   L"Iterates in float down to zoom 1e30, much faster on most GPUs, but iteration values drift from "
                   L"the double-precision result; deeper zooms use double.");
        gpu.number("batchSeconds", &Gpu::batchSeconds, L"Batch Duration (s)", 0.01f, 1.0f);
        gpu.number("allowedUnfinishedPixels", &Gpu::allowedUnfinishedPixels, L"Allowed Unfinished Pixels",
                   uint32_t(0), uint32_t(100000000),
                   L"Stops once no more than this many pixels are still iterating. 0 finishes every pixel.");
        gpu.number("stopMpaAfterBatches", &Gpu::stopMpaAfterBatches, L"Stop MPA After Batches", uint32_t(0),
                   uint32_t(1000000), L"Iterates without MPA after this many batches. 0 keeps MPA.");
        gpu.choice("ignoreMpa", &Gpu::ignoreMpa, L"Ignore MPA on GPU",
                   L"Iterates every step without approximation skips.");
        gpu.choice("interpolateIsolatedPixels", &Gpu::interpolateIsolatedPixels, L"Interpolate Isolated Pixels",
                   L"Fills an unfinished pixel with four finished neighbors from their mean; faster, but approximated.");
        auto form = model->form(L"Render & Preview",
                                {L"Performance", L"Calculation Preview", L"GPU Perturbation"});
        form.groupInUse = groupsInUse(attribute, form.groups.size(),
                                      {{2, [](const Attribute &a) { return a.render.computeShader.use; }}});
        appearanceFormBehavior(form);
        return form;
    }
    WorkspaceForm lightingForm(AttributeGetter attribute, std::function<void()> changed) {
        auto model = std::make_shared<AttributeFormModel>(attribute, std::move(changed));
        using Settings = ShdSlopeAttribute;
        AttributeFormSection section(
            model, [](auto &attributes) -> auto & { return attributes.shader.slope; }, "slope.");
        section.choice("shadingBlend", &Settings::shadingBlend, L"Shading Blend");
        section.choice("replaceSurfaceStyle", &Settings::replaceSurfaceStyle,
                       L"Replace Surface Style Completely",
                       L"On replaces the palette with the selected style, ignoring palette tint, Surface "
                       L"Blend, Chrome Strength and Slope Opacity. Off preserves legacy compositing. "
                       L"Requires Studio and a non-Original style.");
        section.choice("surfaceBlend", &Settings::surfaceBlend, L"Surface Blend");
        section.number("depth", &Settings::depth, L"Shading Depth", 0.0f, 10000.0f);
        section.number("reflectionRatio", &Settings::reflectionRatio, L"Shadow Floor", 0.0f, 1.0f);
        section.number("opacity", &Settings::opacity, L"Slope Opacity", 0.0f, 1.0f);
        section.number("terminatorSoftness", &Settings::terminatorSoftness, L"Terminator Softness", 0.0f,
                       1.0f);
        section.number("lumaAmount", &Settings::lumaAmount, L"Relief Lightness", 0.0f, 1.0f);
        section.group = 1;
        section.number("zenith", &Settings::zenith, L"Light Zenith", 0.0f, std::nextafter(360.f, 0.f));
        section.number("azimuth", &Settings::azimuth, L"Light Direction", 0.0f, std::nextafter(360.f, 0.f));
        section.number("fillIntensity", &Settings::fillIntensity, L"Fill Intensity", 0.0f, 1.0f);
        section.number("fillZenith", &Settings::fillZenith, L"Fill Zenith", 0.0f, std::nextafter(360.f, 0.f));
        section.number("fillAzimuth", &Settings::fillAzimuth, L"Fill Direction", 0.0f,
                       std::nextafter(360.f, 0.f));
        section.group = 2;
        section.number("specularIntensity", &Settings::specularIntensity, L"Specular Intensity", 0.0f, 1.0f);
        section.number("specularPower", &Settings::specularPower, L"Specular Power", 1.0f, 100000.0f);
        section.number("reliefResponse", &Settings::reliefResponse, L"Relief Response", 0.0f, 1.0f);
        section.colorRgb("specularColor", &Settings::specularColor, L"Specular Color");
        section.choice("specularIndependent", &Settings::specularIndependent, L"Independent Specular Light");
        section.number("specularZenith", &Settings::specularZenith, L"Specular Zenith", 0.0f,
                       std::nextafter(360.f, 0.f));
        section.number("specularAzimuth", &Settings::specularAzimuth, L"Specular Direction", 0.0f,
                       std::nextafter(360.f, 0.f));
        section.number("specularAnisotropy", &Settings::specularAnisotropy, L"Specular Anisotropy", 0.0f,
                       1.0f);
        section.number("specularAnisotropyAngle", &Settings::specularAnisotropyAngle, L"Anisotropy Angle",
                       0.0f, std::nextafter(360.f, 0.f));
        section.choice("specularAnisotropyDirection", &Settings::specularAnisotropyDirection, L"Anisotropy Direction");
        section.group = 3;
        section.number("macroRelief", &Settings::macroRelief, L"Macro Relief", 0.0f, 1.0f);
        section.number("macroRadius", &Settings::macroRadius, L"Macro Radius", 1.0f, 12.0f);
        section.number("aoIntensity", &Settings::aoIntensity, L"Cavity Intensity", 0.0f, 1.0f);
        section.group = 4;
        section.number("ambientIntensity", &Settings::ambientIntensity, L"Tint Intensity", 0.0f, 1.0f);
        section.colorRgb("skyColor", &Settings::skyColor, L"Light-Facing Color");
        section.colorRgb("groundColor", &Settings::groundColor, L"Shadow-Facing Color");
        section.choice("tintBlend", &Settings::tintBlend, L"Tint Blend");
        section.number("tintResponse", &Settings::tintResponse, L"Tint Response", 0.1f, 4.0f);
        section.number("shadowChroma", &Settings::shadowChroma, L"Shadow Chroma", 0.0f, 2.0f);
        section.group = 5;
        section.number("brightness", &Settings::brightness, L"Slope Brightness", NumericSettingLimits::slopeBrightness.minimum, NumericSettingLimits::slopeBrightness.maximum,
                       L"Brightness multiplier, 0 to 100; 0 turns it off.");
        section.number("gamma", &Settings::gamma, L"Slope Gamma", NumericSettingLimits::gamma.minimum, NumericSettingLimits::gamma.maximum,
                       L"Shadow curve, 0.01 to 10; 1 preserves the curve.");
        section.number("highlightKnee", &Settings::highlightKnee, L"Highlight Knee", 0.0f, 1.0f);
        section.choice("lightBlend", &Settings::lightBlend, L"Light Blend");
        section.group = 6;
        section.number("rimIntensity", &Settings::rimIntensity, L"Rim Intensity", 0.0f, 1.0f);
        section.number("rimPower", &Settings::rimPower, L"Rim Power", 0.1f, 64.0f);
        section.colorRgb("rimColor", &Settings::rimColor, L"Rim Color");
        section.group = 7;
        section.number("glossIntensity", &Settings::glossIntensity, L"Gloss Intensity", 0.0f, 1.0f);
        section.choice("glossSource", &Settings::glossSource, L"Gloss Source");
        section.number("glossRelief", &Settings::glossRelief, L"Gloss Relief", 0.0f, 16.0f);
        section.number("glossBands", &Settings::glossBands, L"Gloss Bands", 1.0f, 32.0f);
        section.number("glossSharpness", &Settings::glossSharpness, L"Gloss Sharpness", 1.0f, 256.0f);
        section.number("glossPhase", &Settings::glossPhase, L"Gloss Phase", 0.0f, 1.0f);
        section.colorRgb("glossColor", &Settings::glossColor, L"Gloss Color");
        section.group = 8;
        section.choice("lustreRelief", &Settings::lustreRelief, L"Lustre Relief");
        section.choice("reliefAutoZoom", &Settings::reliefAutoZoom, L"Auto Zoom Compensation");
        section.choice("invertRelief", &Settings::invertRelief, L"Invert Relief");
        section.number("reliefDepth", &Settings::reliefDepth, L"Lustre Depth", 0.0f, 16.0f);
        section.number("normalSmooth", &Settings::normalSmooth, L"Normal Smoothing", 0.0f, 1.0f);
        section.number("aoRadius", &Settings::aoRadius, L"AO Radius", 1.0f, 64.0f);
        section.number("boundaryGuard", &Settings::boundaryGuard, L"Boundary Reflection Guard", 0.0f, 1.0f);
        section.number("reliefCap", &Settings::reliefCap, L"Relief Cap", 0.0f, 8.0f);
        section.number("reliefWaves", &Settings::reliefWaves, L"Relief Waves", 0.0f, 1.0f);
        section.number("waveFrequency", &Settings::waveFrequency, L"Wave Frequency", 0.03f, 1.5f);
        section.choice("reliefWavesCoatOnly", &Settings::reliefWavesCoatOnly, L"Waves on Clear Coat Only");
        model->text("slope.reliefZoomReference", 8, L"Zoom Reference",
                    L"Saved reference zoom: 0 to 16777216, or -1 for an unbound reference.",
                    [](const Attribute &a) { return AttributeFormModel::number(a.shader.slope.reliefZoomReference); },
                    [](Attribute &a, const std::wstring &text) {
                        float value;
                        if (!AttributeFormModel::parse(text, value) ||
                            (value != -1.f && !NumericSettingLimits::logZoom(value))) return false;
                        a.shader.slope.reliefZoomReference = value;
                        return true;
                    });
        model->setNormalizer([](const Attribute &before, Attribute &after, const FormDraft &draft) {
            const auto &previousSlope = before.shader.slope;
            auto &slope = after.shader.slope;
            if (!draft.contains("slope.reliefZoomReference") &&
                ((!previousSlope.lustreRelief && slope.lustreRelief) ||
                 (!previousSlope.reliefAutoZoom && slope.reliefAutoZoom))) {
                slope.reliefZoomReference = after.fractal.logZoom;
            }
        });
        AttributeFormSection studio(model, [](auto &a) -> auto & { return a.shader.slope.studio; }, "studio.", 9);
        studio.choice("use", &ShdStudioAttribute::use, L"Studio");
        studio.choice("model", &ShdStudioAttribute::model, L"Material Model");
        studio.number("roughness", &ShdStudioAttribute::roughness, L"Roughness", .04f, 1.f);
        studio.number("metalness", &ShdStudioAttribute::metalness, L"Metalness", 0.f, 1.f);
        studio.number("clearcoat", &ShdStudioAttribute::clearcoat, L"Clearcoat", 0.f, 1.f);
        studio.number("clearcoatRoughness", &ShdStudioAttribute::clearcoatRoughness, L"Coat Roughness", .04f, 1.f);
        studio.number("coatIor", &ShdStudioAttribute::coatIor, L"Coat IOR", 1.f, 3.f);
        studio.number("coatDarkening", &ShdStudioAttribute::coatDarkening, L"Coat Darkening", 0.f, 1.f);
        studio.colorRgb("coatColor", &ShdStudioAttribute::coatColor, L"Coat Color");
        studio.colorRgb("edgeTint", &ShdStudioAttribute::edgeTint, L"Edge Tint");
        studio.number("fuzz", &ShdStudioAttribute::fuzz, L"Fuzz", 0.f, 1.f);
        studio.number("fuzzRoughness", &ShdStudioAttribute::fuzzRoughness, L"Fuzz Roughness", 0.f, 1.f);
        studio.colorRgb("fuzzColor", &ShdStudioAttribute::fuzzColor, L"Fuzz Color");
        studio.number("filmIor", &ShdStudioAttribute::filmIor, L"Film IOR", 1.f, 3.f);
        section.group = 9;
        section.choice("coatSoftbox", &Settings::coatSoftbox, L"Coat Softbox");
        section.choice("studioEnvironmentSource", &Settings::studioEnvironmentSource, L"Environment Source");
        section.number("environmentBrightness", &Settings::environmentBrightness, L"Environment Brightness", 0.f, 32.f);
        section.number("environmentStop1Position", &Settings::environmentStop1Position, L"Gradient Stop 1 Position", 0.f, 1.f);
        section.colorRgb("environmentStop1Color", &Settings::environmentStop1Color, L"Gradient Stop 1 Color");
        section.number("environmentStop2Position", &Settings::environmentStop2Position, L"Gradient Stop 2 Position", 0.f, 1.f);
        section.colorRgb("environmentStop2Color", &Settings::environmentStop2Color, L"Gradient Stop 2 Color");
        section.number("environmentStop3Position", &Settings::environmentStop3Position, L"Gradient Stop 3 Position", 0.f, 1.f);
        section.colorRgb("environmentStop3Color", &Settings::environmentStop3Color, L"Gradient Stop 3 Color");
        section.number("environmentStop4Position", &Settings::environmentStop4Position, L"Gradient Stop 4 Position", 0.f, 1.f);
        section.colorRgb("environmentStop4Color", &Settings::environmentStop4Color, L"Gradient Stop 4 Color");
        section.number("iridescence", &Settings::iridescence, L"Iridescence", 0.f, 1.f);
        section.number("filmThickness", &Settings::filmThickness, L"Film Thickness (nm)", 0.f, 2000.f);
        auto form =
            model->form(L"Lighting & Relief",
                        {L"Slope Shading", L"Light Direction", L"Specular", L"Relief Detail",
                         L"Lit / Shadow Tint", L"Tone Mapping", L"Rim Light", L"Gloss", L"Lustre Relief", L"Material Presets"});
        form.groupInUse = groupsInUse(
            attribute, form.groups.size(),
            {{0, [](const Attribute &a) { return a.shader.slope.depth != 0 && a.shader.slope.opacity > 0; }},
             {6, [](const Attribute &a) { return a.shader.slope.rimIntensity > 0; }},
             {7, [](const Attribute &a) { return a.shader.slope.glossIntensity > 0; }},
             {8, [](const Attribute &a) { return a.shader.slope.lustreRelief; }}});
        constexpr const wchar_t *presets[] = {L"Bronze", L"Pearl", L"Obsidian", L"Ceramic"};
        for (int i = 0; i < 4; ++i) form.actions.push_back({9, presets[i], [model, i] {
            model->edit([i](Attribute &a) { applyStudioPreset(a.shader.slope, i); });
        }});
        appearanceFormBehavior(form);
        form.addDividers({"specularIndependent", "reliefWaves", "iridescence"});
        form.actions.push_back({8, L"Use Current Zoom", [model, attribute] {
                                    model->apply(
                                        {{"slope.reliefZoomReference",
                                          AttributeFormModel::number(float(attribute().fractal.logZoom))}});
                                }});
        return form;
    }
    WorkspaceForm textureForm(AttributeGetter attribute, std::function<void()> changed, LayerMove onMove) {
        auto model = std::make_shared<AttributeFormModel>(attribute, std::move(changed));
        using Settings = ShdTextureAttribute;
        for (int layerIndex = 0; layerIndex < 4; ++layerIndex) {
            AttributeFormSection section(
                model,
                [layerIndex](auto &attributes) -> auto & { return attributes.shader.textures[layerIndex]; },
                "texture." + std::to_string(layerIndex) + ".", layerIndex);
            section.choice("enabled", &Settings::enabled, L"Enabled");
            section.image("path", &Settings::path, L"Image File");
            section.choice("uvMode", &Settings::uvMode, L"UV Source");
            section.choice("blendMode", &Settings::blendMode, L"Blend Mode");
            section.number("opacity", &Settings::opacity, L"Opacity", 0.0f, 1.0f);
            section.number("periodIterations", &Settings::periodIterations, L"Texture Period", 0.0f,
                           formMaximum, L"Iterations per tile; 0 follows the palette.");
            section.number("size", &Settings::size, L"Size", 0.1f, 20.0f);
            section.choice("keepAspect", &Settings::keepAspect, L"Keep Aspect");
            section.number("scaleU", &Settings::scaleU, L"Repeat U", 0.0f, 20.0f);
            section.number("scaleV", &Settings::scaleV, L"Repeat V", 0.0f, 20.0f);
            section.number("paletteFollow", &Settings::paletteFollow, L"Palette Follow", -2.0f, 2.0f);
            section.number("scrollU", &Settings::scrollU, L"Scroll U", -2.0f, 2.0f);
            section.number("scrollV", &Settings::scrollV, L"Scroll V", -2.0f, 2.0f);
        }
        auto form = model->form(L"Textures", {L"Layer 1 (Bottom)", L"Layer 2", L"Layer 3", L"Layer 4 (Top)"});
        form.groupInUse = layersInUse(attribute, ShdLayer::TEXTURE_1);
        model->setNormalizer([](const Attribute &, Attribute &after, const FormDraft &draft) {
            for (int i = 0; i < 4; ++i) if (draft.contains("texture." + std::to_string(i) + ".path") &&
                !draft.contains("texture." + std::to_string(i) + ".enabled"))
                after.shader.textures[i].enabled = !after.shader.textures[i].path.empty();
        });
        form.updateDraft = [](FormDraft &draft, const std::string &id) {
            if (id.ends_with(".path") && draft.contains(id)) draft[id.substr(0, id.size() - 4) + "enabled"] = draft.at(id).empty() ? L"0" : L"1";
        };
        for (int i = 0; i < 4; ++i) form.actions.push_back({i, L"Clear Image", [model, i] {
            model->edit([i](Attribute &a) { a.shader.textures[i].path.clear(); a.shader.textures[i].enabled = false; });
        }});
        appearanceFormBehavior(form);
        form.addDividers({"paletteFollow"});
        addLayerGuidance(form, "texture.");
        addLayerActions(form, model,
                        [](Attribute &attributes) -> auto & { return attributes.shader.textures; }, onMove);
        return form;
    }
    WorkspaceForm patternForm(AttributeGetter attribute, std::function<void()> changed, LayerMove onMove) {
        auto model = std::make_shared<AttributeFormModel>(attribute, std::move(changed));
        using Settings = ShdPatternAttribute;
        for (int layerIndex = 0; layerIndex < 4; ++layerIndex) {
            AttributeFormSection section(
                model,
                [layerIndex](auto &attributes) -> auto & { return attributes.shader.patterns[layerIndex]; },
                "pattern." + std::to_string(layerIndex) + ".", layerIndex);
            section.choice("enabled", &Settings::enabled, L"Enabled");
            section.choice("type", &Settings::type, L"Pattern");
            section.choice("inkMode", &Settings::inkMode, L"Ink Mode");
            section.colorRgb("color", &Settings::color, L"Ink Color");
            section.number("paletteShift", &Settings::paletteShift, L"Palette Shift", 0.0f, 1.0f);
            section.number("sharpness", &Settings::sharpness, L"Sharpness", 0.0f, 1.0f);
            section.choice("uvMode", &Settings::uvMode, L"UV Source");
            section.choice("blendMode", &Settings::blendMode, L"Blend Mode");
            section.number("opacity", &Settings::opacity, L"Opacity", 0.0f, 1.0f);
            section.number("periodIterations", &Settings::periodIterations, L"Pattern Period", 0.0f,
                           formMaximum, L"Iterations per tile; 0 follows the palette.");
            section.number("scaleU", &Settings::scaleU, L"Repeat U", 0.0f, 200.0f);
            section.number("scaleV", &Settings::scaleV, L"Repeat V", 0.0f, 200.0f);
            section.number("paletteFollow", &Settings::paletteFollow, L"Palette Follow", -2.0f, 2.0f);
            section.number("scrollU", &Settings::scrollU, L"Scroll U", -2.0f, 2.0f);
            section.number("scrollV", &Settings::scrollV, L"Scroll V", -2.0f, 2.0f);
            section.choice("edgeEnabled", &Settings::edgeEnabled, L"Edge Enabled");
            section.colorRgb("edgeColor", &Settings::edgeColor, L"Edge Color");
            section.number("edgeWidth", &Settings::edgeWidth, L"Edge Width", -0.5f, 1.0f);
            section.number("edgeOpacity", &Settings::edgeOpacity, L"Edge Opacity", 0.0f, 1.0f);
            section.choice("edgeRelative", &Settings::edgeRelative, L"Relative Edge Width");
        }
        auto form = model->form(L"Patterns", {L"Layer 1 (Bottom)", L"Layer 2", L"Layer 3", L"Layer 4 (Top)"});
        form.groupInUse = layersInUse(attribute, ShdLayer::PATTERN_1);
        appearanceFormBehavior(form);
        form.addDividers({"paletteFollow", "edgeEnabled"});
        addLayerGuidance(form, "pattern.");
        addLayerActions(form, model,
                        [](Attribute &attributes) -> auto & { return attributes.shader.patterns; }, onMove);
        return form;
    }
    WorkspaceForm warpStripeForm(AttributeGetter attribute, std::function<void()> changed) {
        auto model = std::make_shared<AttributeFormModel>(attribute, std::move(changed));
        using Settings = ShdWarpAttribute;
        AttributeFormSection section(
            model, [](auto &attributes) -> auto & { return attributes.shader.warp; }, "warp.");
        section.choice("enabled", &Settings::enabled, L"Warp Enabled");
        section.choice("source", &Settings::source, L"Warp Source");
        section.choice("uvMode", &Settings::uvMode, L"UV Source");
        section.number("amount", &Settings::amount, L"Warp Amount", 0.0f, 2.0f);
        section.number("octaves", &Settings::octaves, L"Warp Detail", 1.0f, 6.0f);
        section.number("scaleU", &Settings::scaleU, L"Repeat U", 0.0f, 200.0f);
        section.number("scaleV", &Settings::scaleV, L"Repeat V", 0.0f, 200.0f);
        section.number("periodIterations", &Settings::periodIterations, L"Warp Period", 0.0f, formMaximum,
                       L"Iterations per tile; 0 follows the palette.");
        section.number("paletteFollow", &Settings::paletteFollow, L"Palette Follow", -2.0f, 2.0f);
        section.number("scrollU", &Settings::scrollU, L"Scroll U", -2.0f, 2.0f);
        section.number("scrollV", &Settings::scrollV, L"Scroll V", -2.0f, 2.0f);
        {
            using Settings = ShdStripeAttribute;
            AttributeFormSection section(
                model, [](auto &attributes) -> auto & { return attributes.shader.stripe; }, "stripe.", 1);
            section.choice("stripeType", &Settings::stripeType, L"Stripe Type");
            section.number("firstInterval", &Settings::firstInterval, L"Interval 1", formPositive,
                           formMaximum, L"Positive iteration interval.");
            section.number("secondInterval", &Settings::secondInterval, L"Interval 2", formPositive,
                           formMaximum, L"Positive iteration interval.");
            section.number("opacity", &Settings::opacity, L"Opacity", 0.0f, 1.0f);
            section.number("offset", &Settings::offset, L"Offset", -formMaximum, formMaximum,
                           L"Signed stripe offset.");
            section.number("animationSpeed", &Settings::animationSpeed, L"Animation Speed", -formMaximum,
                           formMaximum, L"Signed speed; 0 stops stripe motion.");
        }
        auto form = model->form(L"Warp & Stripe", {L"Domain Warp", L"Stripe"});
        form.groupInUse = groupsInUse(
            attribute, form.groups.size(),
            {{0, [](const Attribute &a) { return a.shader.warp.enabled; }},
             {1, [](const Attribute &a) { return shaderLayerActive(a.shader, ShdLayer::STRIPE); }}});
        appearanceFormBehavior(form);
        return form;
    }
    WorkspaceForm otherForm(AttributeGetter attribute, std::function<void()> changed) {
        auto model = std::make_shared<AttributeFormModel>(attribute, std::move(changed));
        using Settings = ShdPaletteAttribute;
        AttributeFormSection section(
            model, [](auto &attributes) -> auto & { return attributes.shader.palette; }, "palette.");
        section.choice("escapeAngleColoring", &Settings::escapeAngleColoring, L"Escape Angle Coloring");
        section.number("escapeAngleAmount", &Settings::escapeAngleAmount, L"Escape Angle Amount",
                       -Settings::ESCAPE_ANGLE_AMOUNT_LIMIT, Settings::ESCAPE_ANGLE_AMOUNT_LIMIT,
                       L"Palette cycles; 0.50 sets the two halves of Binary Decomposition half a cycle apart.");
        auto form = model->form(L"Other", {L"Escape Angle"});
        form.groupInUse = groupsInUse(attribute, form.groups.size(), {{0, [](const Attribute &a) {
            return a.shader.palette.escapeAngleColoring != ShdPalEscapeAngleColoring::NONE;
        }}});
        appearanceFormBehavior(form);
        return form;
    }
    WorkspaceForm orbitTrapForm(AttributeGetter attribute, std::function<void()> changed) {
        auto model = std::make_shared<AttributeFormModel>(attribute, std::move(changed));
        using Settings = ShdPaletteAttribute;
        AttributeFormSection section(
            model, [](auto &attributes) -> auto & { return attributes.shader.palette; }, "palette.");
        section.choice("orbitTrapColoring", &Settings::orbitTrapColoring, L"Coloring");
        section.choice("orbitTrapShape", &Settings::orbitTrapShape, L"Shape",
                       L"Shape coloring only: the trap each orbit is measured against. Changing it recalculates.");
        section.number("orbitTrapCenterReal", &Settings::orbitTrapCenterReal, L"Center Real",
                       -Settings::ORBIT_TRAP_CENTER_LIMIT, Settings::ORBIT_TRAP_CENTER_LIMIT,
                       L"Real part of the trap's center on the complex plane.");
        section.number("orbitTrapCenterImag", &Settings::orbitTrapCenterImag, L"Center Imaginary",
                       -Settings::ORBIT_TRAP_CENTER_LIMIT, Settings::ORBIT_TRAP_CENTER_LIMIT,
                       L"Imaginary part of the trap's center on the complex plane.");
        section.number("orbitTrapRadius", &Settings::orbitTrapRadius, L"Radius", 0.0,
                       Settings::ORBIT_TRAP_RADIUS_MAX,
                       L"Circle's and Polygon's and Star's size, Grid and Rings spacing, Spiral's step per turn; 0 measures as Point. Recalculates.");
        section.number("orbitTrapAngle", &Settings::orbitTrapAngle, L"Angle",
                       -Settings::ORBIT_TRAP_ANGLE_LIMIT, Settings::ORBIT_TRAP_ANGLE_LIMIT,
                       L"Degrees the trap turns about its center; Point, Circle and Rings do not use it. Recalculates.");
        section.number("orbitTrapSides", &Settings::orbitTrapSides, L"Sides", ORBIT_TRAP_SIDES_MIN,
                       ORBIT_TRAP_SIDES_MAX, L"Polygon's sides and Star's points. Recalculates.");
        section.group = 1;
        section.choice("orbitTrapMapping", &Settings::orbitTrapMapping, L"Color Mapping",
                       L"Shape coloring only: what the trap's colors follow.");
        section.number("orbitTrapArms", &Settings::orbitTrapArms, L"Arms", uint32_t(1),
                       Settings::ORBIT_TRAP_ARMS_MAX, L"Angle color mapping only: palette turns per revolution.");
        section.number("orbitTrapDensity", &Settings::orbitTrapDensity, L"Density",
                       -Settings::ORBIT_TRAP_DENSITY_LIMIT, Settings::ORBIT_TRAP_DENSITY_LIMIT,
                       L"Palette cycles across the trap; Angle color mapping uses Arms instead.");
        section.number("orbitTrapOffset", &Settings::orbitTrapOffset, L"Offset", -1.0f, 1.0f,
                       L"Shifts the trap's colors, in palette cycles.");
        section.number("orbitTrapAnimationSpeed", &Settings::orbitTrapAnimationSpeed, L"Animation Speed",
                       -Settings::ORBIT_TRAP_SPEED_LIMIT, Settings::ORBIT_TRAP_SPEED_LIMIT,
                       L"Palette cycles per second; 0 holds still.");
        section.choice("orbitTrapHoldColorAnimation", &Settings::orbitTrapHoldColorAnimation,
                       L"Hold in Color Animation");
        section.group = 2;
        section.number("orbitTrapWidth", &Settings::orbitTrapWidth, L"Width",
                       Settings::ORBIT_TRAP_WIDTH_MIN, Settings::ORBIT_TRAP_WIDTH_MAX,
                       L"log2 of the distance where the glow halves; lower it at deep zooms.");
        section.number("orbitTrapGlowSharpness", &Settings::orbitTrapGlowSharpness, L"Glow Sharpness",
                       Settings::ORBIT_TRAP_SHAPE_MIN, Settings::ORBIT_TRAP_SHAPE_MAX,
                       L"Higher draws thinner lines; Relief does not use it.");
        section.colorRgb("orbitTrapGlowColor", &Settings::orbitTrapGlowColor, L"Glow Color");
        section.number("orbitTrapGlowStrength", &Settings::orbitTrapGlowStrength, L"Glow Strength", 0.0f,
                       Settings::ORBIT_TRAP_GLOW_STRENGTH_MAX);
        section.choice("orbitTrapRelief", &Settings::orbitTrapRelief, L"Relief",
                       L"Cross Glow and Shape only: draws the trap as a lit tube of radius Width.");
        section.group = 3;
        section.number("orbitTrapAmount", &Settings::orbitTrapAmount, L"Amount", 0.0f, 1.0f,
                       L"Share of the trap coloring over the iteration palette.");
        section.choice("orbitTrapBlend", &Settings::orbitTrapBlend, L"Blend Mode");
        section.choice("orbitTrapInterior", &Settings::orbitTrapRegion, L"Apply To",
                       L"Inside means the pixels that reach Max Iterations, the black inside of the set.");
        section.group = 4;
        section.number("orbitTrapCellBorderWidth", &Settings::orbitTrapCellBorderWidth,
                       L"Border Width", Settings::ORBIT_TRAP_SHAPE_MIN, Settings::ORBIT_TRAP_SHAPE_MAX,
                       L"Cellular only.");
        section.number("orbitTrapCellBorderDarkness", &Settings::orbitTrapCellBorderDarkness,
                       L"Border Darkness", 0.0f, 1.0f, L"Cellular only.");
        auto form = model->form(L"Orbit Trap", {L"Trap", L"Palette", L"Glow", L"Blending", L"Cell Border"});
        form.groupInUse = groupsInUse(attribute, form.groups.size(), {{0, [](const Attribute &a) {
            return a.shader.palette.orbitTrapColoring != ShdPalOrbitTrapColoring::NONE;
        }}});
        appearanceFormBehavior(form);
        return form;
    }
    WorkspaceForm finishingForm(AttributeGetter attribute, std::function<void()> changed,
                                std::function<void(std::function<void()>)> pickFocus) {
        auto model = std::make_shared<AttributeFormModel>(attribute, std::move(changed));
        using Settings = ShdColorAttribute;
        AttributeFormSection section(
            model, [](auto &attributes) -> auto & { return attributes.shader.color; }, "color.");
        section.number("gamma", &Settings::gamma, L"Gamma", NumericSettingLimits::gamma.minimum, NumericSettingLimits::gamma.maximum, L"Brightness curve.");
        section.number("exposure", &Settings::exposure, L"Exposure", NumericSettingLimits::colorExposure.minimum, NumericSettingLimits::colorExposure.maximum,
                       L"0 preserves exposure; negative values darken.");
        section.number("hue", &Settings::hue, L"Hue", NumericSettingLimits::colorHue.minimum, NumericSettingLimits::colorHue.maximum,
                       L"Hue rotation from -1 to 1 turns; 0 keeps the palette hue.");
        section.number("saturation", &Settings::saturation, L"Saturation", NumericSettingLimits::saturation.minimum, NumericSettingLimits::saturation.maximum,
                       L"0 keeps saturation; -1 is grayscale.");
        section.number("brightness", &Settings::brightness, L"Brightness", NumericSettingLimits::colorBrightness.minimum, NumericSettingLimits::colorBrightness.maximum,
                       L"Offset added to every color channel.");
        section.number("contrast", &Settings::contrast, L"Contrast", NumericSettingLimits::contrast.minimum, NumericSettingLimits::contrast.maximum);
        section.number("vignette", &Settings::vignette, L"Vignette", NumericSettingLimits::vignette.minimum, NumericSettingLimits::vignette.maximum,
                       L"Darkness at the frame edges; 0 is off.");
        section.choice("vignetteShape", &Settings::vignetteShape, L"Vignette Shape");
        section.number("vignetteRadius", &Settings::vignetteRadius, L"Vignette Radius", NumericSettingLimits::vignetteRadius.minimum,
                       NumericSettingLimits::vignetteRadius.maximum, L"Size of the untouched center.");
        section.number("vignetteRichness", &Settings::vignetteRichness, L"Vignette Richness", NumericSettingLimits::vignetteRichness.minimum,
                       NumericSettingLimits::vignetteRichness.maximum, L"Saturation added inside the dark rim.");
        {
            using Settings = ShdBloomAttribute;
            AttributeFormSection section(
                model, [](auto &attributes) -> auto & { return attributes.shader.bloom; }, "bloom.", 1);
            section.number("threshold", &Settings::threshold, L"Threshold", 0.0f, 1.0f);
            section.number("radius", &Settings::radius, L"Radius", 0.0f, 1.0f);
            section.number("softness", &Settings::softness, L"Softness", 0.0f, 1.0f);
            section.number("intensity", &Settings::intensity, L"Intensity", NumericSettingLimits::bloomIntensity.minimum, NumericSettingLimits::bloomIntensity.maximum,
                           L"Bloom strength; 0 disables the halo.");
            section.choice("linearAdd", &Settings::linearAdd, L"Linear Light");
        }
        {
            using Settings = ShdFogAttribute;
            AttributeFormSection section(
                model, [](auto &attributes) -> auto & { return attributes.shader.fog; }, "fog.", 2);
            section.number("radius", &Settings::radius, L"Radius", 0.0f, 1.0f);
            section.number("opacity", &Settings::opacity, L"Opacity", 0.0f, 1.0f);
            section.number("centerStart", &Settings::centerStart, L"Center Start", 0.0f, 1.0f);
            section.choice("centerInvert", &Settings::centerInvert, L"Invert Falloff");
            section.number("rimMask", &Settings::rimMask, L"Rim Mask", 0.0f, 1.0f);
            section.number("rimMaskBoost", &Settings::rimMaskBoost, L"Rim Mask Boost", NumericSettingLimits::rimMaskBoost.minimum, NumericSettingLimits::rimMaskBoost.maximum, L"Mask multiplier, 1 to 100.");
            section.number("rimBlur", &Settings::rimBlur, L"Rim Blur", NumericSettingLimits::blur.minimum, NumericSettingLimits::blur.maximum,
                           L"Blur radius in pixels relative to 1280 width.");
            section.choice("blurQuality", &Settings::blurQuality, L"Blur Quality");
            section.group = 3;
            section.number("focusAmount", &Settings::focusAmount, L"Focus Amount", 0.0f, 1.0f);
            section.number("focusRatio", &Settings::focusRatio, L"Focus Depth", 0.0f, 1.0f);
            section.number("focusRange", &Settings::focusRange, L"Focus Range", 0.01f, 1.0f);
            section.number("focusFalloff", &Settings::focusFalloff, L"Focus Falloff", 0.1f, 4.0f);
            section.number("focusBlur", &Settings::focusBlur, L"Focus Blur", NumericSettingLimits::blur.minimum, NumericSettingLimits::blur.maximum,
                           L"Blur radius in pixels relative to 1280 width.");
            section.choice("focusSmoothKeyframes", &Settings::focusSmoothKeyframes, L"Smooth Focus Across Keyframes",
                           L"Video only. With Auto Max Iteration the maximum iteration can change several times over "
                           L"at a keyframe, and the band, being a fraction of it, jumped there in one frame. On glides "
                           L"it between the two keyframes instead. Off keeps the band where earlier versions put it.");
            section.group = 4;
            section.number(
                "chaosAmount", &Settings::chaosAmount, L"Chaos Amount", 0.0f, 1.0f,
                L"Blurs locally intricate fractal regions. Zero disables it; smooth surfaces stay sharp.");
            section.number("chaosScale", &Settings::chaosScale, L"Chaos Detail Scale", 0.5f, 4.0f,
                           L"Detail detection radius in pixels at 1280 width. One targets fine structure; "
                           L"larger values include broader structure.");
            section.number(
                "chaosThreshold", &Settings::chaosThreshold, L"Chaos Threshold", 0.0f, 1.0f,
                L"Minimum local irregularity to blur. Increase to protect more of the smooth surface.");
            section.number("chaosTransition", &Settings::chaosTransition, L"Chaos Transition", 0.01f, 1.0f,
                           L"Smooth transition from sharp through medium blur to full blur as local "
                           L"irregularity increases.");
            section.number("chaosFeather", &Settings::chaosFeather, L"Chaos Feather", 0.0f, 32.0f,
                           L"Softens the detected region boundary, in pixels at 1280 width. Detection "
                           L"follows the current zoom and location.");
            section.number("chaosBlur", &Settings::chaosBlur, L"Chaos Blur Radius", 0.0f, 32.0f,
                           L"Circular aperture radius in pixels at 1280 width. Scales with output size; Blur "
                           L"Quality controls sampling.");
            section.number(
                "chaosShade", &Settings::chaosShade, L"Chaos Shade", 0.0f, 1.0f,
                L"Darkens intricate defocused regions to separate them from smooth foreground surfaces. At 1, "
                L"fully selected regions turn black whatever Chaos Amount is.");
            section.number(
                "chaosInterior", &Settings::chaosInterior, L"Chaos Interior", 0.0f, 1.0f,
                L"Blurs areas that reach Max Iterations, such as the black interior of the Mandelbrot set. "
                L"Zero leaves them sharp; higher values select them more strongly.");
            section.choice("chaosBlurAverage", &Settings::chaosBlurAverage, L"Chaos Blur Averaging",
                           L"Gamma Space keeps the dark gaps between bright details dark. Linear Space averages "
                           L"light physically, so bright detail spreads and blurred regions come out brighter.");
            section.choice("chaosEdgeProtection", &Settings::chaosEdgeProtection, L"Chaos Edge Protection",
                           L"On keeps smooth surfaces next to blurred regions from bleeding into the blur, so "
                           L"their outlines stay crisp. Off blurs every neighbouring pixel into the region, as "
                           L"earlier versions did.");
            section.choice("chaosBokehShape", &Settings::chaosBokehShape, L"Chaos Bokeh Shape",
                           L"The outline of the blur: Circle, or a polygon with three to eight straight "
                           L"edges. Bright specks in blurred regions take this shape.");
            section.number("chaosBokehRotation", &Settings::chaosBokehRotation, L"Chaos Bokeh Rotation", 0.0f, 360.0f,
                           L"Turns the polygon outline, in degrees. With Circle it turns the stretch set by "
                           L"Chaos Bokeh Aspect.");
            section.number("chaosBokehAspect", &Settings::chaosBokehAspect, L"Chaos Bokeh Aspect", 0.25f, 4.0f,
                           L"Height over width of the outline. 1 keeps it even; above 1 stretches it "
                           L"vertically, below 1 horizontally, at the same area.");
        }
        auto form =
            model->form(L"Finishing", {L"Color Correction", L"Bloom", L"Fog", L"Focus Band", L"Chaos Blur"});
        form.groupInUse = groupsInUse(attribute, form.groups.size(),
                                      {{1, [](const Attribute &a) { return a.shader.bloom.intensity != 0; }},
                                       {2, [](const Attribute &a) { return a.shader.fog.opacity > 0; }},
                                       {3, [](const Attribute &a) { return a.shader.fog.focusAmount > 0; }},
                                       {4, [](const Attribute &a) { return a.shader.fog.chaosAmount > 0; }}});
        appearanceFormBehavior(form);
        form.addDividers({"rimMask"});
        if (pickFocus) {
            const auto depth = std::find_if(form.fields.begin(), form.fields.end(),
                                            [](const auto &field) { return field.id == "fog.focusRatio"; })->read;
            form.actions.push_back({3, L"Pick Focus Depth", [model, depth, pickFocus] {
                const auto before = depth();
                pickFocus([model, before] { model->recordExternal({{"fog.focusRatio", before}}); });
            }});
        }
        return form;
    }
    WorkspaceForm materialEffectsForm(AttributeGetter attribute, std::function<void()> changed, LayerMove onMove) {
        auto model = std::make_shared<AttributeFormModel>(attribute, std::move(changed));
        using Settings = ShdEffectAttribute;
        for (int layerIndex = 0; layerIndex < 4; ++layerIndex) {
            AttributeFormSection section(
                model,
                [layerIndex](auto &attributes) -> auto & { return attributes.shader.effects[layerIndex]; },
                "effect." + std::to_string(layerIndex) + ".", layerIndex);
            section.choice("enabled", &Settings::enabled, L"Enabled");
            section.template derivedChoice<ShdEffectVariant>(
                "type", L"Effect Type", [](const Settings &effect) { return effectVariantOf(effect); },
                [](Settings &effect, ShdEffectVariant variant) { applyEffectVariant(effect, variant); },
                L"The material and, in parentheses, its shape or style. Classic is each type's original look.");
            section.choice("blend", &Settings::blend, L"Blend");
            section.choice("mask", &Settings::mask, L"Mask");
            section.number("dropSize", &Settings::dropSize, L"Drop Size", 0.1f, 8.0f);
            section.number("opacity", &Settings::opacity, L"Opacity", 0.0f, 1.0f);
            section.number("scale", &Settings::scale, L"Surface Scale", 0.1f, 100.0f);
            section.number("density", &Settings::density, L"Density", 0.0f, 1.0f);
            section.number("length", &Settings::length, L"Shape Length", 0.05f, 2.0f);
            section.number("width", &Settings::width, L"Shape Width", 0.1f, 10.0f);
            section.number("distortion", &Settings::distortion, L"Surface Depth", 0.0f, 4.0f);
            section.number("glow", &Settings::glow, L"Glow", 0.0f, 4.0f);
            section.choice("syncColorAnimation", &Settings::syncColorAnimation, L"Sync Color Animation");
            section.number("speed", &Settings::speed, L"Speed", -10.0f, 10.0f);
            section.number("evolution", &Settings::evolution, L"Evolution", -10.0f, 10.0f);
            section.number("direction", &Settings::direction, L"Flow Bend", -180.0f, 180.0f);
            section.number("seed", &Settings::seed, L"Seed", 0.0f, 65535.0f);
            section.number("period", &Settings::period, L"Band Period", 1.0f, 1e9f);
            section.color("color", &Settings::color, L"Primary Color");
            section.color("secondary", &Settings::secondary, L"Secondary Color");
        }
        auto form = model->form(L"Animated Materials",
                                {L"Layer 1 (Bottom)", L"Layer 2", L"Layer 3", L"Layer 4 (Top)"});
        form.groupInUse = layersInUse(attribute, ShdLayer::EFFECT_1);
        appearanceFormBehavior(form);
        form.addDividers({"syncColorAnimation", "color"});
        addLayerGuidance(form, "effect.");
        addLayerActions(form, model,
                        [](Attribute &attributes) -> auto & { return attributes.shader.effects; }, onMove);
        return form;
    }
} // namespace merutilm::rff2::workspace
