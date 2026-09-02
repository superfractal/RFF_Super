//
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-16, 2026-08-21.
// Modified by Opus 4.8 on 2026-07-05
// Modified by Opus 5 on 2026-08-08, 2026-08-15, 2026-08-16, 2026-08-17, 2026-08-20, 2026-08-22, 2026-08-29
// Modified by ox-alpha on 2026-08-22.
// Modified by Fable 5.1 on 2026-09-02
//

#pragma once

#include "ShdSlopeGlossSource.h"
#include "ShdSlopeLightBlend.h"
#include "ShdSlopeShadingBlend.h"
#include "ShdSlopeTintBlend.h"

namespace merutilm::rff2 {
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
    };
}
