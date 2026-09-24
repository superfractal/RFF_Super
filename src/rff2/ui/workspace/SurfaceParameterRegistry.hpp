//
// Modified by GPT-6 on 2026-09-13, 2026-09-14, 2026-09-18, 2026-09-19, 2026-09-22, 2026-09-23, 2026-09-24
//

#pragma once
#include "WorkspaceText.hpp"
#include "../../attr/SurfaceControlRouting.h"
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <string>
#include <string_view>
#include <vector>

namespace merutilm::rff2::workspace {
    enum class Category { COLOR, REFLECTION, FILM, CONTOUR, EMISSION, FLAME, PRINT, NOISE, RELIEF, MIX };
    inline bool paletteLineControl(std::string_view id) {
        return id == "surface.styleRimStrength" || id == "surface.styleRimWidth" || id == "surface.styleRimColor";
    }
    inline ShdSlopeAttribute surfaceDefaults(){
        // Legacy control defaults match ShdSlopePresets::Normal2::genSlope; newer fields retain their declared defaults.
        return ShdSlopeAttribute{1000.f,.5f,1.f,60.f,135.f,.15f,100.f,0.f,3.f,2.25f,3.f};
    }
    struct SurfaceParameter {
        enum class NumberKind { CONTINUOUS, INTEGER, ANGLE };
        std::string_view id;
        std::wstring_view label;
        float ShdSlopeAttribute::*member;
        float minimum;
        float maximum;
        float ShdStudioAttribute::*studioMember = nullptr;
        NumberKind numberKind=NumberKind::CONTINUOUS;
        float logarithmicFloor=0;
        bool isInteger()const{return numberKind==NumberKind::INTEGER;}
        float snap(float value)const{return isInteger()?std::round(value):value;}
        float keyStep()const{return numberKind!=NumberKind::CONTINUOUS?1.f:(maximum-minimum)/100.f;}
        float sliderValue(float fraction)const{
            fraction=std::clamp(fraction,0.f,1.f);
            if(logarithmicFloor>0){if(fraction<=.05f)return logarithmicFloor*fraction/.05f;return std::min(maximum,logarithmicFloor*std::pow(maximum/logarithmicFloor,(fraction-.05f)/.95f));}
            return snap(minimum+fraction*(maximum-minimum));
        }
        float sliderFraction(float value)const{
            if(logarithmicFloor>0){if(value<=logarithmicFloor)return .05f*std::clamp(value/logarithmicFloor,0.f,1.f);return std::clamp(.05f+.95f*std::log(value/logarithmicFloor)/std::log(maximum/logarithmicFloor),0.f,1.f);}
            return std::clamp((value-minimum)/(maximum-minimum),0.f,1.f);
        }
        float nudged(float value,int direction)const{
            if(numberKind==NumberKind::ANGLE)return std::fmod(value+direction+360.f,360.f);
            if(logarithmicFloor>0){if(value<=logarithmicFloor)return std::clamp(value+direction*logarithmicFloor/10.f,minimum,maximum);return std::clamp(value*std::pow(10.f,direction*.1f),minimum,maximum);}
            return std::clamp(snap(value)+direction*keyStep(),minimum,maximum);
        }
        float& value(ShdSlopeAttribute& slope) const { return studioMember?slope.studio.*studioMember:slope.*member; }
        const float& value(const ShdSlopeAttribute& slope) const { return studioMember?slope.studio.*studioMember:slope.*member; }
        void activate(ShdSlopeAttribute& slope) const {
            if(studioMember)slope.studio.use=true;
            else activateSurfaceControl(slope,&value(slope));
        }
        Category category() const {
            if(studioMember||id.starts_with("surface.studio"))return Category::REFLECTION;
            if(id=="surface.iridescence"||id=="surface.filmThickness")return Category::FILM;
            if(id=="surface.specularAA")return Category::REFLECTION;
            const ShdSlopeAttribute defaults{};
            if (id == "surface.backgroundBrightness" || id == "surface.styleInkPreserve") return Category::COLOR;
            if(id=="surface.layerVhs"||id=="surface.layerMono")return Category::NOISE;
            if(id=="surface.layerQuantize")return Category::PRINT;
            if (id.starts_with("surface.layer")) return Category::MIX;
            if (id == "surface.styleRimStrength" || id == "surface.styleRimWidth" || id.starts_with("surface.styleDetail")) return Category::EMISSION;
            if (id == "surface.filmStrength" || id == "surface.filmHue" || id.starts_with("surface.prism") || id == "surface.sparkleStrength" || id == "surface.styleGlintSize") return Category::FILM;
            switch (surfaceControlGroup(defaults, &(defaults.*member))) {
                case 1: return Category::COLOR;
                case 2: return Category::REFLECTION;
                case 3: return Category::CONTOUR;
                case 4: case 5: return Category::FLAME;
                case 6: return Category::EMISSION;
                case 7: case 8: return Category::PRINT;
                case 9: case 10: return Category::NOISE;
                default: return Category::RELIEF;
            }
        }
        bool valid(float value) const { return std::isfinite(value) && value >= minimum && (numberKind==NumberKind::ANGLE?value<360.f:value<=maximum) && (!isInteger()||std::trunc(value)==value); }
    };
    inline const std::vector<SurfaceParameter> &surfaceParameters() {
        static const std::vector<SurfaceParameter> parameters = {
            {"slope.depth",L"Shading Depth",&ShdSlopeAttribute::depth,0.f,10000.f,nullptr,SurfaceParameter::NumberKind::CONTINUOUS,.000001f},
            {"slope.opacity",L"Opacity",&ShdSlopeAttribute::opacity,0.f,1.f},
            {"slope.reflectionRatio",L"Shadow Floor",&ShdSlopeAttribute::reflectionRatio,0.f,1.f},
            {"slope.zenith",L"Light Zenith",&ShdSlopeAttribute::zenith,0.f,359.f,nullptr,SurfaceParameter::NumberKind::ANGLE},
            {"slope.azimuth",L"Light Direction",&ShdSlopeAttribute::azimuth,0.f,359.f,nullptr,SurfaceParameter::NumberKind::ANGLE},
            {"surface.studio.roughness",L"Roughness",nullptr,.04f,1.f,&ShdStudioAttribute::roughness},
            {"surface.studio.metalness",L"Metalness",nullptr,0.f,1.f,&ShdStudioAttribute::metalness},
            {"surface.studio.ior",L"Index of Refraction",nullptr,1.f,3.f,&ShdStudioAttribute::ior},
            {"surface.studio.directIntensity",L"Direct Light",nullptr,0.f,8.f,&ShdStudioAttribute::directIntensity},
            {"surface.studio.environmentIntensity",L"Studio Reflections",nullptr,0.f,8.f,&ShdStudioAttribute::environmentIntensity},
            {"surface.studio.clearcoat",L"Clearcoat",nullptr,0.f,1.f,&ShdStudioAttribute::clearcoat},
            {"surface.studio.clearcoatRoughness",L"Coat Roughness",nullptr,.04f,1.f,&ShdStudioAttribute::clearcoatRoughness},
            {"surface.studioEnvironmentRotation",L"Environment Rotation",&ShdSlopeAttribute::studioEnvironmentRotation,0.f,359.f,nullptr,SurfaceParameter::NumberKind::ANGLE},
            {"surface.studioEnvironmentFollow",L"Follow Light Direction",&ShdSlopeAttribute::studioEnvironmentFollow,0.0f,1.0f},
            {"surface.chromeStrength", L"Chrome Strength", &ShdSlopeAttribute::chromeStrength, 0, 1},
            {"surface.filmStrength", L"Thin Film Color", &ShdSlopeAttribute::filmStrength, 0, 1},
            {"surface.iridescence", L"Iridescence", &ShdSlopeAttribute::iridescence, 0, 1},
            {"surface.filmThickness", L"Film Thickness (nm)", &ShdSlopeAttribute::filmThickness, 0, 2000},
            {"surface.specularAA", L"Specular Antialiasing", &ShdSlopeAttribute::specularAA, 0, 1},
            {"surface.boundaryGuard", L"Boundary Reflection Guard", &ShdSlopeAttribute::boundaryGuard, 0, 1},
            {"surface.reflectionDetail", L"Reflection Detail", &ShdSlopeAttribute::reflectionDetail, 0.25f, 4.0f},
            {"surface.reflectionContrast", L"Reflection Contrast", &ShdSlopeAttribute::reflectionContrast, 0.25f, 3.0f},
            {"surface.reflectionBrightness", L"Reflection Brightness", &ShdSlopeAttribute::reflectionBrightness, 0.0f, 3.0f},
            {"surface.reflectionCurve", L"Reflection Curvature", &ShdSlopeAttribute::reflectionCurve, 0.1f, 3.0f},
            {"surface.surfacePhase", L"Surface Phase", &ShdSlopeAttribute::surfacePhase, 0.0f, 1.0f},
            {"surface.prismWidth", L"Rainbow Width", &ShdSlopeAttribute::prismWidth, 0.1f, 3.0f},
            {"surface.prismSpread", L"Rainbow Spread", &ShdSlopeAttribute::prismSpread, 0.0f, 1.0f},
            {"surface.filmHue", L"Film Hue", &ShdSlopeAttribute::filmHue, 0.0f, 1.0f},
            {"surface.sparkleStrength", L"Glint Strength", &ShdSlopeAttribute::sparkleStrength, 0.0f, 3.0f},
            {"surface.backgroundBrightness", L"Sigil Background", &ShdSlopeAttribute::backgroundBrightness, 0.0f, 8.0f},
            {"surface.shadowCrush", L"Shadow Crush", &ShdSlopeAttribute::shadowCrush, 0.0f, 1.0f},
            {"surface.flameStrength", L"Chrome Flames", &ShdSlopeAttribute::flameStrength, 0.0f, 3.0f},
            {"surface.phonkRed", L"Red / Purple", &ShdSlopeAttribute::phonkRed, 0.0f, 1.0f},
            {"surface.vhsNoise", L"VHS Damage", &ShdSlopeAttribute::vhsNoise, 0.0f, 1.0f},
            {"surface.chromaticShift", L"Chromatic Shift", &ShdSlopeAttribute::chromaticShift, 0.0f, 8.0f},
            {"surface.pixelMix", L"Nearest Mix", &ShdSlopeAttribute::pixelMix, 0.0f, 1.0f},
            {"surface.pixelSize", L"Nearest Block Size", &ShdSlopeAttribute::pixelSize, 1.0f, 16.0f},
            {"surface.filmGrain", L"Film Grain", &ShdSlopeAttribute::filmGrain, 0.0f, 1.0f},
            {"surface.frostStrength", L"Frost Strength", &ShdSlopeAttribute::frostStrength, 0.0f, 3.0f},
            {"surface.frostThreshold", L"Frost Threshold", &ShdSlopeAttribute::frostThreshold, 0.0f, 1.0f},
            {"surface.grungeScale", L"Grunge Scale", &ShdSlopeAttribute::grungeScale, 0.25f, 4.0f},
            {"surface.paletteColorMix", L"Palette Color", &ShdSlopeAttribute::paletteColorMix, 0.0f, 1.0f},
            {"surface.styleColorMix", L"Surface Color Amount", &ShdSlopeAttribute::styleColorMix, 0.0f, 1.0f},
            {"surface.styleHighlightMix", L"Reflection Color Amount", &ShdSlopeAttribute::styleHighlightMix, 0.0f, 1.0f},
            {"surface.styleBackgroundMix", L"Background Color Amount", &ShdSlopeAttribute::styleBackgroundMix, 0.0f, 1.0f},
            {"surface.styleRimStrength", L"Surface Rim Strength", &ShdSlopeAttribute::styleRimStrength, 0.0f, 4.0f},
            {"surface.styleRimWidth", L"Surface Rim Width", &ShdSlopeAttribute::styleRimWidth, 0.1f, 32.0f},
            {"surface.styleInkPreserve", L"Preserve Palette Ink", &ShdSlopeAttribute::styleInkPreserve, 0.0f, 1.0f},
            {"surface.styleMonochrome", L"Black Metal Monochrome", &ShdSlopeAttribute::styleMonochrome, 0.0f, 1.0f},
            {"surface.styleGlintSize", L"Glint Size", &ShdSlopeAttribute::styleGlintSize, 0.25f, 4.0f},
            {"surface.styleDetailLight", L"Detail Light", &ShdSlopeAttribute::styleDetailLight, 0.0f, 2.0f},
            {"surface.styleDetailSuppress", L"Dense Detail Suppression", &ShdSlopeAttribute::styleDetailSuppress, 0.0f, 1.0f},
            {"surface.styleDetailThreshold", L"Detail Density Threshold", &ShdSlopeAttribute::styleDetailThreshold, 0.005f, 0.4f},
            {"surface.styleDamage", L"PHONK Surface Damage", &ShdSlopeAttribute::styleDamage, 0.0f, 1.0f},
            {"surface.seaGlow", L"Sea Glow", &ShdSlopeAttribute::seaGlow, 0.0f, 4.0f},
            {"surface.seaThreshold", L"Sea Glow Threshold", &ShdSlopeAttribute::seaThreshold, 0.0f, 1.0f},
            {"surface.seaBody", L"Sea Body Light", &ShdSlopeAttribute::seaBody, 0.0f, 1.0f},
            {"surface.seaParticles", L"Sea Particles", &ShdSlopeAttribute::seaParticles, 0.0f, 1.0f},
            {"surface.seaParticleSize", L"Sea Particle Size", &ShdSlopeAttribute::seaParticleSize, 0.25f, 3.0f},
            {"surface.seaBalance", L"Sea Color Balance", &ShdSlopeAttribute::seaBalance, 0.0f, 1.0f},
            {"surface.ukiyoColors", L"Print Color Count", &ShdSlopeAttribute::ukiyoColors, 3.0f, 6.0f,nullptr,SurfaceParameter::NumberKind::INTEGER},
            {"surface.ukiyoFoam", L"Wave Foam", &ShdSlopeAttribute::ukiyoFoam, 0.0f, 1.0f},
            {"surface.ukiyoGrain", L"Woodcut Grain", &ShdSlopeAttribute::ukiyoGrain, 0.0f, 1.0f},
            {"surface.ukiyoFlatness", L"Print Flatness", &ShdSlopeAttribute::ukiyoFlatness, 0.0f, 1.0f},
            {"surface.ukiyoBalance", L"Print Tone Balance", &ShdSlopeAttribute::ukiyoBalance, 0.0f, 1.0f},
            {"surface.reliefDepth", L"Lustre Depth", &ShdSlopeAttribute::reliefDepth, 0, 16},
            {"surface.normalSmooth", L"Normal Smoothing", &ShdSlopeAttribute::normalSmooth, 0, 1},
            {"surface.aoRadius", L"AO Radius", &ShdSlopeAttribute::aoRadius, 1, 64},
            {"surface.reliefWaves", L"Relief Waves", &ShdSlopeAttribute::reliefWaves, 0, 1},
            {"surface.waveFrequency", L"Wave Frequency", &ShdSlopeAttribute::waveFrequency, 0.03f, 1.5f},
            {"surface.layerMetal", L"Liquid Metal Mix", &ShdSlopeAttribute::layerMetal, 0, 1},
            {"surface.layerSigil", L"Cyber Sigilism Mix", &ShdSlopeAttribute::layerSigil, 0, 1},
            {"surface.layerPhonk", L"PHONK Mix", &ShdSlopeAttribute::layerPhonk, 0, 1},
            {"surface.layerFrost", L"Black Metal Mix", &ShdSlopeAttribute::layerFrost, 0, 1},
            {"surface.layerSea", L"Deep Sea Light", &ShdSlopeAttribute::layerSea, 0, 1},
            {"surface.layerPrint", L"Ukiyo-e Wave Mix", &ShdSlopeAttribute::layerPrint, 0, 1},
            {"surface.layerQuantize", L"Color Quantization", &ShdSlopeAttribute::layerQuantize, 0, 1},
            {"surface.layerVhs", L"VHS / Nearest Effects", &ShdSlopeAttribute::layerVhs, 0, 1},
            {"surface.layerMono", L"Monochrome / Grain Effects", &ShdSlopeAttribute::layerMono, 0, 1},
        };
        return parameters;
    }
    inline std::wstring categoryLabel(Category category) {
        switch (category) {
            case Category::COLOR: return uiText(TextKey::Color);
            case Category::REFLECTION: return uiText(TextKey::Reflection);
            case Category::FILM: return uiText(TextKey::Film);
            case Category::CONTOUR: return uiText(TextKey::Contour);
            case Category::EMISSION: return uiText(TextKey::Emission);
            case Category::FLAME: return uiText(TextKey::Flame);
            case Category::PRINT: return uiText(TextKey::Print);
            case Category::NOISE: return uiText(TextKey::Noise);
            case Category::RELIEF: return uiText(TextKey::Relief);
            case Category::MIX: return uiText(TextKey::Mix);
        }
        return {};
    }
}
