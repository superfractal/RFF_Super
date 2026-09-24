//
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 4.8 on 2026-07-05
// Modified by Opus 5 on 2026-08-08, 2026-08-15, 2026-08-16, 2026-08-17, 2026-08-20, 2026-08-22, 2026-08-29
// Modified by GPT-5 on 2026-08-16, 2026-08-21
// Modified by ox-alpha on 2026-08-22
// Modified by Fable 5.1 on 2026-09-02
// Modified by GPT-6 on 2026-09-10, 2026-09-11, 2026-09-12, 2026-09-13, 2026-09-18, 2026-09-20, 2026-09-23, 2026-09-24
//

#pragma once
#include <algorithm>
#include <iterator>
#include <glm/glm.hpp>
#include "ShdSurfaceStyle.h"

#include "ShdSlopeGlossSource.h"
#include "ShdSlopeLightBlend.h"
#include "ShdSlopeShadingBlend.h"
#include "ShdSlopeTintBlend.h"

namespace merutilm::rff2 {
    struct ShdStudioAttribute {
        bool use = false;
        float roughness = 0.32f;
        float metalness = 0.6f;
        float ior = 1.5f;
        float directIntensity = 0.8f;
        float environmentIntensity = 0.7f;
        float clearcoat = 0.3f;
        float clearcoatRoughness = 0.15f;
    };

    struct ShdSlopeAttribute {
        float depth;
        float reflectionRatio;
        float opacity;
        float zenith;
        float azimuth;
        float specularIntensity;  // 0.0 - 1.0: specular highlight intensity
        float specularPower;      // 8 - 256: specular highlight sharpness
        float rimIntensity;       // 0.0 - 1.0: rim light intensity
        float rimPower;           // 2.0 - 5.0 (approx): rim light sharpness/falloff
        float brightness;         // 1.0 = default
        float gamma;              // 1.0 = default
        glm::vec4 rimColor = {1.0f, 1.0f, 1.0f, 1.0f}; // Color for rim lighting
        glm::vec4 specularColor = {1.0f, 1.0f, 1.0f, 1.0f}; // Tint of the specular highlight (metallic look)
        float aoIntensity = 0.0f;     // 0.0 - 1.0: ambient occlusion strength (darkens concave pits)
        float ambientIntensity = 0.0f;                       // 0.0 - 1.0: hemisphere ambient strength
        glm::vec4 skyColor = {1.0f, 0.93f, 0.82f, 1.0f};     // warm light tint (lit areas)
        glm::vec4 groundColor = {0.45f, 0.55f, 0.78f, 1.0f}; // cool shadow tint (shadowed areas)
        bool specularIndependent = false;      // false = follow main light, true = use specular zenith/azimuth
        float specularZenith = 60.0f;          // 0 ~ 360: specular light zenith (used when independent)
        float specularAzimuth = 135.0f;        // 0 ~ 360: specular light azimuth (used when independent)
        float specularAnisotropy = 0.0f;       // 0 = round, >0 stretches the highlight
        float specularAnisotropyAngle = 0.0f;  // 0 ~ 360: stretch direction (degrees)
        float macroRelief = 0.0f;              // N3: 0 = single-scale relief (existing), >0 blends a wide Sobel for broad form
        float macroRadius = 8.0f;              // N3: macro Sobel sampling radius (1280px-reference pixels)
        // P6: how the shading meets the palette color. Overlay is the original composite.
        ShdSlopeShadingBlend shadingBlend = ShdSlopeShadingBlend::OVERLAY;
        // L1: lighting. The highlight's normal is tilted to sit on the half vector, because Shading
        // Depth's 1e5 gain saturates the gradient magnitude and leaves its direction as the only
        // part of the relief still carrying information. reliefResponse dials the tilt off that
        // anchor and onto the surface's own, for views where the magnitude is not saturated.
        // Every one of these defaults to the behaviour of 2.0.8 and earlier, so a settings file or
        // preset written by one of those looks exactly as it did until the control is moved.
        float reliefResponse = 0.0f;       // 0 = anchored highlight, 1 = the surface's own tilt
        float terminatorSoftness = 0.0f;   // 0 = the original hard ambient floor, 1 = fully wrapped light
        float highlightKnee = 0.75f;       // linear level the highlight shoulder starts at; only used by LINEAR
        ShdSlopeLightBlend lightBlend = ShdSlopeLightBlend::DIRECT;
        // L2: chromatic shading. The relief can be carried by color temperature alone rather than by
        // lightness, which is the one composite a saturated palette survives intact - a shaded area
        // keeps every bit of its color and only turns cooler. Each default is the behaviour earlier
        // versions had, so a file written by one of those is unmoved until a control is.
        float lumaAmount = 1.0f;       // 1 = the directional shading as it was, 0 = no lightness shading at all
        float tintResponse = 1.0f;     // curve on the lit/shadow mix; >1 holds the tint back to the lit side
        float shadowChroma = 1.0f;     // chroma the shadow side is scaled by; >1 deepens color into shadow. OKLab only
        ShdSlopeTintBlend tintBlend = ShdSlopeTintBlend::MULTIPLY;
        // A second diffuse light that lifts the key's shadows from its own direction; the default
        // direction sits opposite the key light's. 0 intensity leaves every earlier version's single light.
        float fillIntensity = 0.0f;
        float fillZenith = 60.0f;
        float fillAzimuth = 315.0f;
        // The gloss. The palette's own Gloss lays narrow bright bands along the palette cycle,
        // which is what ties that look to the coloring: the bands slide as the palette animates,
        // and at the next location they fall wherever the iteration count happens to put them.
        // These lay the same bands along a coordinate the relief owns instead, so they sit on the
        // surface itself and neither the colors nor the location move them. 0 intensity is every
        // earlier version's picture, so a settings file or preset from one is unmoved.
        // The defaults below are the ones a fresh gloss starts on; a file that carries the gloss
        // block carries every one of them, so no saved picture is moved by them.
        float glossIntensity = 0.0f;
        ShdSlopeGlossSource glossSource = ShdSlopeGlossSource::SHADING_FINE;
        float glossBands = 2.0f;       // bright bands across the coordinate's whole range
        float glossSharpness = 6.0f;   // exponent on each band; higher is a narrower line
        float glossPhase = 0.25f;      // 0 - 1 slide of the bands along the coordinate; 0.25 seats a crest at g = 1
        glm::vec4 glossColor = {1.0f, 1.0f, 1.0f, 1.0f};
        float glossRelief = 8.0f;      // log2 gain on the gloss's own normal, independent of depth; Fine Shading only
        ShdStudioAttribute studio{};
        float iridescence = 0.0f;
        float filmThickness = 400.0f;
        float specularAA = 0.0f;
        bool lustreRelief = false;
        float reliefDepth = 1.0f;
        float normalSmooth = 0.0f;
        float aoRadius = 8.0f;
        float reliefWaves = 0.0f;
        float waveFrequency = 0.5f;
        bool invertRelief = false;
        float boundaryGuard = 0.0f;
        bool reliefAutoZoom = true;
        float reliefZoomReference = -1.0f;
        ShdSurfaceStyle surfaceStyle = ShdSurfaceStyle::ORIGINAL;
        float chromeStrength = 1.0f;
        float filmStrength = 0.65f;
        float reflectionDetail = 1.0f;
        float reflectionContrast = 1.0f;
        float reflectionBrightness = 1.0f;
        float reflectionCurve = 1.0f;
        float surfacePhase = 0.0f;
        float prismWidth = 1.0f;
        float prismSpread = 1.0f;
        float filmHue = 0.0f;
        float sparkleStrength = 1.0f;
        float backgroundBrightness = 4.5f;
        float shadowCrush = 0.72f;
        float flameStrength = 1.0f;
        float phonkRed = 0.85f;
        float vhsNoise = 0.45f;
        float chromaticShift = 1.5f;
        float pixelMix = 0.2f;
        float pixelSize = 4.0f;
        float filmGrain = 0.3f;
        float frostStrength = 1.0f;
        float frostThreshold = 0.45f;
        float grungeScale = 1.0f;
        float paletteColorMix = 0.0f;
        float styleColorMix = 0.0f;
        float styleHighlightMix = 0.0f;
        float styleBackgroundMix = 0.0f;
        float styleRimStrength = 0.0f;
        float styleRimWidth = 2.0f;
        float styleInkPreserve = 1.0f;
        float styleMonochrome = 1.0f;
        float styleGlintSize = 1.0f;
        float styleDetailLight = 1.0f;
        float styleDetailSuppress = 0.0f;
        float styleDetailThreshold = 0.08f;
        float styleDamage = 0.0f;
        ShdSurfaceBlend surfaceBlend = ShdSurfaceBlend::MIX;
        bool replaceSurfaceStyle = false;
        glm::vec4 styleColor = {1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec4 styleHighlightColor = {1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec4 styleBackgroundColor = {0.0f, 0.0f, 0.0f, 1.0f};
        glm::vec4 styleRimColor = {1.0f, 1.0f, 1.0f, 1.0f};

        float seaGlow = 1.6f;
        float seaThreshold = 0.3f;
        float seaBody = 0.15f;
        float seaParticles = 0.5f;
        float seaParticleSize = 1.0f;
        float seaBalance = 0.55f;
        float ukiyoColors = 5.0f;
        float ukiyoFoam = 0.75f;
        float ukiyoGrain = 0.15f;
        float ukiyoFlatness = 1.0f;
        float ukiyoBalance = 0.55f;
        glm::vec4 seaBodyColor = {0.02f, 0.16f, 0.38f, 1.0f};
        glm::vec4 seaGlowColor = {0.0f, 0.8f, 1.0f, 1.0f};
        glm::vec4 seaAccentColor = {0.55f, 1.0f, 0.16f, 1.0f};
        glm::vec4 printInk = {0.008f, 0.014f, 0.025f, 1.0f};
        glm::vec4 printIndigo = {0.025f, 0.12f, 0.27f, 1.0f};
        glm::vec4 printAsagi = {0.13f, 0.48f, 0.58f, 1.0f};
        glm::vec4 printBlue = {0.5f, 0.72f, 0.76f, 1.0f};
        glm::vec4 printFoam = {0.82f, 0.89f, 0.85f, 1.0f};
        glm::vec4 printPaper = {0.95f, 0.91f, 0.8f, 1.0f};

        float layerMetal = 0.0f;
        float layerSigil = 0.0f;
        float layerPhonk = 0.0f;
        float layerFrost = 0.0f;
        float layerSea = 0.0f;
        float layerPrint = 0.0f;
        float layerQuantize = 0.0f;
        float layerVhs = 0.0f;
        float layerMono = 0.0f;
        float layerCommon = 0.0f;

        float studioEnvironmentRotation = 0.0f;
        float studioEnvironmentFollow = 1.0f;

        [[nodiscard]] float reliefZoomLog2(float logZoom) const {
            if (!lustreRelief || !reliefAutoZoom || reliefZoomReference < 0.0f) {
                return 0.0f;
            }
            return static_cast<float>(std::clamp(
                (double(logZoom) - double(reliefZoomReference)) * 3.321928094887362,
                -512.0,
                512.0));
        }

    };

    // Numeric material recipes from the supplied Lustre guide; no HTML source copied, provenance in NOTICE.
    inline void applyStudioPreset(ShdSlopeAttribute &attribute, int preset) {
        struct StudioRecipe {
            float roughness;
            float metalness;
            float clearcoat;
            float clearcoatRoughness;
            float specularIntensity;
            float specularAnisotropy;
            float iridescence;
            float filmThickness;
        };
        constexpr StudioRecipe recipes[] = {
            {.28f, .72f, .3f, .14f, .75f, .28f, .07f, 410},
            {.32f, .24f, .72f, .12f, .85f, .12f, .4f, 360},
            {.16f, .87f, .34f, .1f, .85f, .38f, .02f, 410},
            {.43f, .02f, .83f, .17f, .63f, 0, 0, 410}
        };
        if (preset < 0 || preset >= static_cast<int>(std::size(recipes))) {
            return;
        }

        const auto &recipe = recipes[preset];
        attribute.studio.use = true;
        attribute.studio.roughness = recipe.roughness;
        attribute.studio.metalness = recipe.metalness;
        attribute.studio.clearcoat = recipe.clearcoat;
        attribute.studio.clearcoatRoughness = recipe.clearcoatRoughness;
        attribute.specularIntensity = recipe.specularIntensity;
        attribute.specularAnisotropy = recipe.specularAnisotropy;
        attribute.iridescence = recipe.iridescence;
        attribute.filmThickness = recipe.filmThickness;
    }
}
