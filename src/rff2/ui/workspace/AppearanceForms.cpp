//
// Modified by GPT-6 on 2026-09-14, 2026-09-19, 2026-09-20, 2026-09-21, 2026-09-23, 2026-09-24, 2026-09-25
//

#include "AppearanceForms.hpp"
#include "AttributeFormSection.hpp"
#include "../../attr/NumericSettingLimits.hpp"
#include <thread>

namespace merutilm::rff2::workspace {
    WorkspaceForm previewPerformanceForm(AttributeGetter attribute, std::function<void()> changed) {
        auto model = std::make_shared<AttributeFormModel>(attribute, std::move(changed));
        using Settings = RenderAttribute;
        AttributeFormSection section(
            model, [](auto &attributes) -> auto & { return attributes.render; }, "render.");
        section.number("fps", &Settings::fps, L"Rendering FPS", NumericSettingLimits::minimumFps, NumericSettingLimits::maximumFps,
                       L"Live rendering limit: 1 to 1000 frames per second. Lower values reduce rendering load. "
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
        return model->form(L"Render & Preview", {L"Performance", L"Calculation Preview"});
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
        section.number("rimPower", &Settings::rimPower, L"Rim Power", 1.0f, 64.0f);
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
        section.number("reliefWaves", &Settings::reliefWaves, L"Relief Waves", 0.0f, 1.0f);
        section.number("waveFrequency", &Settings::waveFrequency, L"Wave Frequency", 0.03f, 1.5f);
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
        auto form =
            model->form(L"Lighting & Relief",
                        {L"Slope Shading", L"Light Direction", L"Specular", L"Relief Detail",
                         L"Lit / Shadow Tint", L"Tone Mapping", L"Rim Light", L"Gloss", L"Lustre Relief"});
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
        return form;
    }
    WorkspaceForm finishingForm(AttributeGetter attribute, std::function<void()> changed) {
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
            section.number("chaosHighlights", &Settings::chaosHighlights, L"Chaos Highlight Detail", 0.0f,
                           1.0f,
                           L"Retains a little of the original bright detail over the lens blur. Zero gives a "
                           L"pure circular blur.");
            section.number(
                "chaosShade", &Settings::chaosShade, L"Chaos Shade", 0.0f, 0.5f,
                L"Darkens intricate defocused regions to separate them from smooth foreground surfaces.");
        }
        auto form =
            model->form(L"Finishing", {L"Color Correction", L"Bloom", L"Fog", L"Focus Band", L"Chaos Blur"});
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
            section.choice("type", &Settings::type, L"Effect Type");
            section.choice("blend", &Settings::blend, L"Blend");
            section.choice("mask", &Settings::mask, L"Mask");
            section.choice("rainShape", &Settings::rainShape, L"Rain Shape");
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
            section.number("period", &Settings::period, L"Band Period", 1.0f, 1000000.0f);
            section.color("color", &Settings::color, L"Primary Color");
            section.color("secondary", &Settings::secondary, L"Secondary Color");
        }
        auto form = model->form(L"Animated Materials",
                                {L"Layer 1 (Bottom)", L"Layer 2", L"Layer 3", L"Layer 4 (Top)"});
        addLayerGuidance(form, "effect.");
        addLayerActions(form, model,
                        [](Attribute &attributes) -> auto & { return attributes.shader.effects; }, onMove);
        return form;
    }
} // namespace merutilm::rff2::workspace
