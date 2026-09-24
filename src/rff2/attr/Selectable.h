//
// Created by Merutilm on 2025-05-04.
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-05, 2026-08-07, 2026-08-13, 2026-08-15, 2026-08-16, 2026-08-17, 2026-08-18, 2026-08-19, 2026-08-20, 2026-08-22, 2026-08-29, 2026-08-31
// Modified by GPT-5 on 2026-08-16, 2026-08-21
// Modified by ox-alpha on 2026-08-22
// Modified by Fable 5.1 on 2026-09-02
// Modified by GPT-6 on 2026-09-10, 2026-09-11, 2026-09-12, 2026-09-13, 2026-09-15, 2026-09-16, 2026-09-20, 2026-09-23
//

#pragma once
#include <string>
#include <type_traits>
#include <vector>

#include "ShdPalColorSmoothingMethod.h"
#include "ShdPalColorInterpolationMethod.h"
#include "ShdFogBlurQuality.h"
#include "ShdToneMapMethod.h"
#include "ShdSlopeGlossSource.h"
#include "ShdSurfaceStyle.h"
#include "ShdSlopeLightBlend.h"
#include "ShdSlopeShadingBlend.h"
#include "ShdSlopeTintBlend.h"
#include "FrtDecimalizeIterationMethod.h"
#include "FrtMPACompressionMethod.h"
#include "FrtMPASelectionMethod.h"
#include "FrtPanoramaLayout.h"
#include "FrtProjectionMethod.h"
#include "FrtReuseReferenceMethod.h"
#include "ShdPaletteAnimationMode.h"
#include "ShdPaletteCycleCurve.h"
#include "ShdPalIterationColoringMode.h"
#include "ShdPatternInkMode.h"
#include "ShdPatternLayerSelection.h"
#include "ShdEffectsAttribute.h"
#include "ShdPatternType.h"
#include "ShdStripeType.h"
#include "ShdTextureBlendMode.h"
#include "ShdTextureLayerSelection.h"
#include "ShdTextureUVMode.h"
#include "ShdWarpSource.h"
#include "VidHdrTransfer.h"
#include "VidKeyInterpolation.h"
#include "VidRotationMode.h"
#include "FractalAttribute.h"


namespace merutilm::rff2 {
    struct Selectable {
        template<typename E> requires std::is_enum_v<E> || std::is_same_v<E, bool>
        static std::vector<E> values() {
            if constexpr (std::is_same_v<E, FrtReuseReferenceMethod>) {
                using enum FrtReuseReferenceMethod;
                return {
                    CURRENT_REFERENCE,
                    CENTERED_REFERENCE,
                    DISABLED
                };
            }
            if constexpr (std::is_same_v<E, FrtPanoramaLayout>) {
                using enum FrtPanoramaLayout;
                return {
                    GROUND,
                    SPHERE
                };
            }
            if constexpr (std::is_same_v<E, FrtProjectionMethod>) {
                using enum FrtProjectionMethod;
                return {
                    PLANAR,
                    EQUIRECTANGULAR_360,
                    PERSPECTIVE_360
                };
            }
            if constexpr (std::is_same_v<E, FrtDecimalizeIterationMethod>) {
                using enum FrtDecimalizeIterationMethod;
                return {
                    LINEAR,
                    SQUARE_ROOT,
                    LOG,
                    LOG_LOG
                };
            }
            if constexpr (std::is_same_v<E, FrtMPASelectionMethod>) {
                using enum FrtMPASelectionMethod;
                return {
                    LOWEST,
                    HIGHEST
                };
            }
            if constexpr (std::is_same_v<E, FrtMPACompressionMethod>) {
                using enum FrtMPACompressionMethod;
                return {
                    NO_COMPRESSION,
                    LITTLE_COMPRESSION,
                    STRONGEST
                };
            }
            if constexpr (std::is_same_v<E, ShdPalColorSmoothingMethod>) {
                using enum ShdPalColorSmoothingMethod;
                return {
                    NONE,
                    NORMAL,
                    REVERSED
                };
            }
            if constexpr (std::is_same_v<E, ShdPalColorInterpolationMethod>) {
                using enum ShdPalColorInterpolationMethod;
                return {
                    RGB,
                    OKLAB,
                    LINEAR_RGB
                };
            }
            if constexpr (std::is_same_v<E, ShdFogBlurQuality>) {
                using enum ShdFogBlurQuality;
                return {
                    SPEED,
                    APPEARANCE
                };
            }
            if constexpr (std::is_same_v<E, ShdToneMapMethod>) {
                using enum ShdToneMapMethod;
                return {
                    CLIP,
                    REINHARD,
                    ACES,
                    FILMIC,
                    MFR_SHOULDER,
                    MFR_LOG,
                    MFR_LINEAR,
                    MFR_FALSE_COLOR
                };
            }
            if constexpr (std::is_same_v<E, VidHdrTransfer>) {
                using enum VidHdrTransfer;
                return {
                    SDR,
                    PQ,
                    HLG
                };
            }
            if constexpr (std::is_same_v<E, ShdSlopeShadingBlend>) {
                using enum ShdSlopeShadingBlend;
                return {
                    OVERLAY,
                    OKLAB_LIGHTNESS
                };
            }
            if constexpr (std::is_same_v<E, ShdSlopeLightBlend>) {
                using enum ShdSlopeLightBlend;
                return {
                    DIRECT,
                    LINEAR
                };
            }
            if constexpr (std::is_same_v<E, ShdSlopeTintBlend>) {
                using enum ShdSlopeTintBlend;
                return {
                    MULTIPLY,
                    OKLAB
                };
            }
            if constexpr (std::is_same_v<E, ShdSurfaceBlend>) {
                return {
                    ShdSurfaceBlend::MIX,
                    ShdSurfaceBlend::ADD,
                    ShdSurfaceBlend::MULTIPLY,
                    ShdSurfaceBlend::SCREEN
                };
            }
            if constexpr (std::is_same_v<E, ShdSurfaceStyle>) {
                return {
                    ShdSurfaceStyle::ORIGINAL,
                    ShdSurfaceStyle::LIQUID_METAL,
                    ShdSurfaceStyle::CYBER_SIGILISM,
                    ShdSurfaceStyle::PHONK,
                    ShdSurfaceStyle::BLACK_METAL,
                    ShdSurfaceStyle::DEEP_SEA,
                    ShdSurfaceStyle::UKIYO_E
                };
            }
            if constexpr (std::is_same_v<E, ShdSlopeGlossSource>) {
                using enum ShdSlopeGlossSource;
                return {
                    SHADING_FINE,
                    SHADING,
                    RELIEF,
                    ASPECT
                };
            }
            if constexpr (std::is_same_v<E, ShdPaletteAnimationMode>) {
                using enum ShdPaletteAnimationMode;
                return {
                    LINEAR,
                    BREATHING,
                    TURBULENCE,
                    PSYCHEDELIC
                };
            }
            if constexpr (std::is_same_v<E, ShdPaletteCycleCurve>) {
                using enum ShdPaletteCycleCurve;
                return {
                    POWER,
                    WAVE
                };
            }
            if constexpr (std::is_same_v<E, ShdPalIterationColoringMode>) {
                using enum ShdPalIterationColoringMode;
                return {
                    LINEAR,
                    SQUARE_ROOT,
                    CUBE_ROOT,
                    LOG,
                    LOG_LOG,
                    SMOOTHSTEP,
                    SMOOTHERSTEP
                };
            }
            if constexpr (std::is_same_v<E, ShdStripeType>) {
                using enum ShdStripeType;
                return {
                    NONE,
                    SINGLE_DIRECTION,
                    SMOOTH,
                    SQUARED
                };
            }
            if constexpr (std::is_same_v<E, ShdTextureUVMode>) {
                using enum ShdTextureUVMode;
                return {
                    CYCLE_BAND,
                    CYCLE_ANGLE,
                    CYCLE_SCREEN,
                    SCREEN
                };
            }
            if constexpr (std::is_same_v<E, ShdTextureBlendMode>) {
                using enum ShdTextureBlendMode;
                return {
                    MULTIPLY,
                    OVERLAY,
                    REPLACE
                };
            }
            if constexpr (std::is_same_v<E, ShdTextureLayerSelection>) {
                using enum ShdTextureLayerSelection;
                return {
                    LAYER_1,
                    LAYER_2,
                    LAYER_3,
                    LAYER_4
                };
            }
            if constexpr (std::is_same_v<E, ShdPatternType>) {
                using enum ShdPatternType;
                return {
                    STRIPES,
                    CHECKER,
                    GRID,
                    DOTS,
                    DIAMOND,
                    HONEYCOMB,
                    WAVES,
                    CLOUD
                };
            }
            if constexpr (std::is_same_v<E, ShdEffectType>) {
                using enum ShdEffectType;
                return {
                    RAIN,
                    FLAME,
                    EMBERS,
                    MIST,
                    HEAT,
                    RIPPLES,
                    FLOW,
                    AURORA
                };
            }
            if constexpr (std::is_same_v<E, ShdEffectBlend>) {
                using enum ShdEffectBlend;
                return {
                    NORMAL,
                    ADD,
                    SCREEN,
                    MULTIPLY
                };
            }
            if constexpr (std::is_same_v<E, ShdRainShape>) {
                return {
                    ShdRainShape::DROPS,
                    ShdRainShape::STREAKS
                };
            }
            if constexpr (std::is_same_v<E, ShdEffectMask>) {
                using enum ShdEffectMask;
                return {
                    ALL,
                    EXTERIOR,
                    INTERIOR,
                    BANDS,
                    EDGE
                };
            }
            if constexpr (std::is_same_v<E, ShdPatternLayerSelection>) {
                using enum ShdPatternLayerSelection;
                return {
                    LAYER_1,
                    LAYER_2,
                    LAYER_3,
                    LAYER_4
                };
            }
            if constexpr (std::is_same_v<E, ShdPatternInkMode>) {
                using enum ShdPatternInkMode;
                return {
                    PALETTE_SHIFT,
                    SOLID
                };
            }
            if constexpr (std::is_same_v<E, ShdWarpSource>) {
                using enum ShdWarpSource;
                return {
                    NOISE,
                    TEXTURE_1,
                    TEXTURE_2,
                    TEXTURE_3,
                    TEXTURE_4
                };
            }
            if constexpr (std::is_same_v<E, VidRotationMode>) {
                return {
                    VidRotationMode::KEYFRAMES,
                    VidRotationMode::CONSTANT_PERIOD
                };
            }
            if constexpr (std::is_same_v<E, VidRotationDirection>) {
                return {
                    VidRotationDirection::CLOCKWISE,
                    VidRotationDirection::COUNTERCLOCKWISE
                };
            }
            if constexpr (std::is_same_v<E, VidKeyInterpolation>) {
                using enum VidKeyInterpolation;
                return {
                    STEP,
                    LINEAR,
                    SMOOTH,
                    CUBIC
                };
            }
            if constexpr (std::is_same_v<E, FractalFormulaType>) {
                using enum FractalFormulaType;
                return {
                    MANDELBROT,
                    CUSTOM
                };
            }
            if constexpr (std::is_same_v<E, bool>) {
                return {true, false};
            }
            return {};
        }

        template<typename E> requires std::is_enum_v<E> || std::is_same_v<E, bool>
        static std::wstring toString(const E &value) {
            if constexpr (std::is_same_v<E, VidRotationMode>) {
                return value == VidRotationMode::CONSTANT_PERIOD ? L"Constant Period" : L"Keyframes";
            }
            if constexpr (std::is_same_v<E, VidRotationDirection>) {
                return value == VidRotationDirection::CLOCKWISE ? L"Clockwise" : L"Counterclockwise";
            }
            if constexpr (std::is_same_v<E, FrtReuseReferenceMethod>) {
                switch (value) {
                    using enum FrtReuseReferenceMethod;
                    case CURRENT_REFERENCE: return L"Current";
                    case CENTERED_REFERENCE: return L"Centered";
                    case DISABLED: return L"Disabled";
                    default: break;
                }

            }
            if constexpr (std::is_same_v<E, FrtPanoramaLayout>) {
                switch (value) {
                    using enum FrtPanoramaLayout;
                    case GROUND: return L"Ground and Sky";
                    case SPHERE: return L"Full Sphere";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, FrtProjectionMethod>) {
                switch (value) {
                    using enum FrtProjectionMethod;
                    case PLANAR: return L"Planar";
                    case EQUIRECTANGULAR_360: return L"360\u00B0 Equirectangular";
                    case PERSPECTIVE_360: return L"360\u00B0 Camera";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, FrtDecimalizeIterationMethod>) {
                switch (value) {
                    using enum FrtDecimalizeIterationMethod;
                    case NONE: return L"None";
                    case LINEAR: return L"Linear";
                    case SQUARE_ROOT: return L"Square root";
                    case LOG: return L"Log";
                    case LOG_LOG: return L"LogLog";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, FrtMPASelectionMethod>) {
                switch (value) {
                    using enum FrtMPASelectionMethod;
                    case LOWEST: return L"Lowest";
                    case HIGHEST: return L"Highest";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, FrtMPACompressionMethod>) {
                switch (value) {
                    using enum FrtMPACompressionMethod;
                    case NO_COMPRESSION: return L"No compression";
                    case LITTLE_COMPRESSION: return L"Little compression";
                    case STRONGEST: return L"Strongest";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdPalColorSmoothingMethod>) {
                switch (value) {
                    using enum ShdPalColorSmoothingMethod;
                    case NONE: return L"None";
                    case NORMAL: return L"Normal";
                    case REVERSED: return L"Reversed";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdPalColorInterpolationMethod>) {
                switch (value) {
                    using enum ShdPalColorInterpolationMethod;
                    case RGB: return L"RGB";
                    case OKLAB: return L"OKLab";
                    case LINEAR_RGB: return L"Linear RGB";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdFogBlurQuality>) {
                switch (value) {
                    using enum ShdFogBlurQuality;
                    case SPEED: return L"Speed";
                    case APPEARANCE: return L"Appearance";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdToneMapMethod>) {
                switch (value) {
                    using enum ShdToneMapMethod;
                    case CLIP: return L"Clip";
                    case REINHARD: return L"Reinhard";
                    case ACES: return L"ACES (Narkowicz fit)";
                    case FILMIC: return L"Filmic";
                    case MFR_SHOULDER: return L"MFR Shoulder";
                    case MFR_LOG: return L"MFR Log View";
                    case MFR_LINEAR: return L"MFR Linear Clip";
                    case MFR_FALSE_COLOR: return L"MFR False Color";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, VidHdrTransfer>) {
                switch (value) {
                    using enum VidHdrTransfer;
                    case SDR: return L"SDR";
                    case PQ: return L"HDR10 (PQ)";
                    case HLG: return L"HLG";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdSlopeShadingBlend>) {
                switch (value) {
                    using enum ShdSlopeShadingBlend;
                    case OVERLAY: return L"Overlay";
                    case OKLAB_LIGHTNESS: return L"OKLab Lightness";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdSlopeLightBlend>) {
                switch (value) {
                    using enum ShdSlopeLightBlend;
                    case DIRECT: return L"Direct";
                    case LINEAR: return L"Linear RGB Add";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdSlopeTintBlend>) {
                switch (value) {
                    using enum ShdSlopeTintBlend;
                    case MULTIPLY: return L"Multiply";
                    case OKLAB: return L"OKLab Tint";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdSurfaceBlend>) {
                switch (value) {
                    case ShdSurfaceBlend::MIX: return L"Mix";
                    case ShdSurfaceBlend::ADD: return L"Add";
                    case ShdSurfaceBlend::MULTIPLY: return L"Multiply";
                    case ShdSurfaceBlend::SCREEN: return L"Screen";
                }
            }
            if constexpr (std::is_same_v<E, ShdSurfaceStyle>) {
                switch (value) {
                    case ShdSurfaceStyle::ORIGINAL: return L"Original";
                    case ShdSurfaceStyle::LIQUID_METAL: return L"Y2K Chrome / Liquid Metal";
                    case ShdSurfaceStyle::CYBER_SIGILISM: return L"Cyber Sigilism";
                    case ShdSurfaceStyle::PHONK: return L"PHONK / Drift Phonk";
                    case ShdSurfaceStyle::BLACK_METAL: return L"Black Metal Album Art";
                    case ShdSurfaceStyle::DEEP_SEA: return L"Deep Sea Bioluminescence";
                    case ShdSurfaceStyle::UKIYO_E: return L"Ukiyo-e Fractal";
                }
            }
            if constexpr (std::is_same_v<E, ShdSlopeGlossSource>) {
                switch (value) {
                    using enum ShdSlopeGlossSource;
                    case SHADING_FINE: return L"Fine Shading";
                    case SHADING: return L"Shading";
                    case RELIEF: return L"Relief Detail";
                    case ASPECT: return L"Slope Facing";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdPaletteAnimationMode>) {
                switch (value) {
                    using enum ShdPaletteAnimationMode;
                    case LINEAR: return L"Linear";
                    case BREATHING: return L"Breathing";
                    case TURBULENCE: return L"Turbulence";
                    case PSYCHEDELIC: return L"Psychedelic";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdPaletteCycleCurve>) {
                switch (value) {
                    using enum ShdPaletteCycleCurve;
                    case POWER: return L"Power";
                    case WAVE: return L"Wave";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdPalIterationColoringMode>) {
                switch (value) {
                    using enum ShdPalIterationColoringMode;
                    case LINEAR: return L"Linear";
                    case SQUARE_ROOT: return L"Square root";
                    case CUBE_ROOT: return L"Cube root";
                    case LOG: return L"Log";
                    case LOG_LOG: return L"LogLog";
                    case SMOOTHSTEP: return L"Smoothstep";
                    case SMOOTHERSTEP: return L"Smootherstep";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdStripeType>) {
                switch (value) {
                    using enum ShdStripeType;
                    case NONE: return L"None";
                    case SINGLE_DIRECTION: return L"Single Direction";
                    case SMOOTH: return L"Smooth";
                    case SQUARED: return L"Squared";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdTextureUVMode>) {
                switch (value) {
                    using enum ShdTextureUVMode;
                    case CYCLE_BAND: return L"Cycle x Band";
                    case CYCLE_ANGLE: return L"Cycle x Angle";
                    case CYCLE_SCREEN: return L"Cycle x Screen";
                    case SCREEN: return L"Screen";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdTextureBlendMode>) {
                switch (value) {
                    using enum ShdTextureBlendMode;
                    case MULTIPLY: return L"Multiply";
                    case OVERLAY: return L"Overlay";
                    case REPLACE: return L"Replace";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdTextureLayerSelection>) {
                switch (value) {
                    using enum ShdTextureLayerSelection;
                    case LAYER_1: return L"1";
                    case LAYER_2: return L"2";
                    case LAYER_3: return L"3";
                    case LAYER_4: return L"4";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdWarpSource>) {
                switch (value) {
                    using enum ShdWarpSource;
                    case NOISE: return L"Noise";
                    case TEXTURE_1: return L"Texture 1";
                    case TEXTURE_2: return L"Texture 2";
                    case TEXTURE_3: return L"Texture 3";
                    case TEXTURE_4: return L"Texture 4";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdPatternType>) {
                switch (value) {
                    using enum ShdPatternType;
                    case STRIPES: return L"Stripes";
                    case CHECKER: return L"Checker";
                    case GRID: return L"Grid";
                    case DOTS: return L"Dots";
                    case DIAMOND: return L"Diamond";
                    case HONEYCOMB: return L"Honeycomb";
                    case WAVES: return L"Waves";
                    case CLOUD: return L"Cloud";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdEffectType>) {
                switch (value) {
                    using enum ShdEffectType;
                    case RAIN: return L"Rain";
                    case FLAME: return L"Flame";
                    case EMBERS: return L"Embers";
                    case MIST: return L"Mist";
                    case HEAT: return L"Heat Haze";
                    case RIPPLES: return L"Ripples";
                    case FLOW: return L"Flow Light";
                    case AURORA: return L"Aurora";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdEffectBlend>) {
                switch (value) {
                    using enum ShdEffectBlend;
                    case NORMAL: return L"Normal";
                    case ADD: return L"Add";
                    case SCREEN: return L"Screen";
                    case MULTIPLY: return L"Multiply";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdRainShape>) {
                switch (value) {
                    case ShdRainShape::STREAKS: return L"Water Streaks";
                    case ShdRainShape::DROPS: return L"Raindrops";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdEffectMask>) {
                switch (value) {
                    using enum ShdEffectMask;
                    case ALL: return L"Surface";
                    case EXTERIOR: return L"Exterior";
                    case INTERIOR: return L"No Surface";
                    case BANDS: return L"Bands";
                    case EDGE: return L"Iteration Edges";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdPatternLayerSelection>) {
                switch (value) {
                    using enum ShdPatternLayerSelection;
                    case LAYER_1: return L"1";
                    case LAYER_2: return L"2";
                    case LAYER_3: return L"3";
                    case LAYER_4: return L"4";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, ShdPatternInkMode>) {
                switch (value) {
                    using enum ShdPatternInkMode;
                    case PALETTE_SHIFT: return L"Palette Shift";
                    case SOLID: return L"Solid Color";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, VidKeyInterpolation>) {
                switch (value) {
                    using enum VidKeyInterpolation;
                    case STEP: return L"Step";
                    case LINEAR: return L"Linear";
                    case SMOOTH: return L"Smooth";
                    case CUBIC: return L"Cubic";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, FractalFormulaType>) {
                switch (value) {
                    using enum FractalFormulaType;
                    case MANDELBROT: return L"Mandelbrot";
                    case CUSTOM: return L"Custom";
                    default: break;
                }
            }
            if constexpr (std::is_same_v<E, bool>)  {
                return value ? L"O" : L"X";
            }

            return L"Unknown Symbol";
        }
    };
}
