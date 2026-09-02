//
// Created and modified by AI; earlier exact dates unavailable.
// Modified by GPT-5 on 2026-08-16.,2026-08-21, 2026-08-23, 2026-08-27, 2026-08-31, 2026-09-01
// Modified by Opus 4.8 on 2026-07-05
// Modified by Opus 5 on 2026-08-05, 2026-08-07, 2026-08-12, 2026-08-13, 2026-08-14, 2026-08-15, 2026-08-16, 2026-08-17, 2026-08-18, 2026-08-19, 2026-08-20, 2026-08-22, 2026-08-24, 2026-08-27, 2026-08-29, 2026-08-31
// Modified by ox-alpha on 2026-08-22.
// Modified by Fable 5.1 on 2026-09-02
//

#include "ConfigIO.h"

#include <cmath>
#include <fstream>
#include <thread>
#include <vector>

#include "../../vulkan_helper/core/logger.hpp"
#include "../calc/fp_decimal_calculator.h"
#include "../ui/IOUtilities.h"
#include "../formula/Perturbator.h"
#include "ShaderPresetIO.h"
#include "TimelineIO.h"

namespace merutilm::rff2 {
    namespace {
        constexpr uint64_t MAX_CONFIG_STRING_BYTES = 16ULL * 1024 * 1024;

        template<typename E>
        bool enumInRange(const E value, const int32_t min, const int32_t max) {
            const int32_t raw = static_cast<int32_t>(value);
            return raw >= min && raw <= max;
        }

        bool validateConfig(const Attribute &attr) {
            const auto &fr = attr.fractal;
            const auto &re = attr.render;
            const auto &vi = attr.video;
            if (!std::isfinite(fr.logZoom) || fr.logZoom < 0.0f ||
                fr.logZoom > static_cast<float>(MAX_CONFIG_STRING_BYTES) ||
                !std::isfinite(fr.bailout) || fr.bailout < 2.0f || fr.bailout > 1000000.0f ||
                !std::isfinite(fr.mpaAttribute.epsilonPower) || fr.mpaAttribute.epsilonPower < -15.0f ||
                fr.mpaAttribute.epsilonPower > -3.0f || fr.mpaAttribute.minSkipReference < 4 ||
                fr.mpaAttribute.maxMultiplierBetweenLevel == 0 || !std::isfinite(fr.rotation) ||
                !enumInRange(fr.decimalizeIterationMethod, 0, 4) ||
                !enumInRange(fr.mpaAttribute.mpaSelectionMethod, 0, 1) ||
                !enumInRange(fr.mpaAttribute.mpaCompressionMethod, 0, 2) ||
                !enumInRange(fr.reuseReferenceMethod, 0, 2) || !enumInRange(fr.formulaType, 0, 1) ||
                !enumInRange(fr.projectionMethod, 0, 2) || !enumInRange(fr.panoramaLayout, 0, 1) ||
                !std::isfinite(fr.panoramaRange) || fr.panoramaRange < 0.0f || fr.panoramaRange > 6.0f ||
                !std::isfinite(fr.panoramaPitch) || fr.panoramaPitch < -90.0f || fr.panoramaPitch > 90.0f ||
                !std::isfinite(fr.panoramaFov) || fr.panoramaFov < 1.0f || fr.panoramaFov > 179.0f) {
                return false;
            }
            if (!std::isfinite(re.clarityMultiplier) || re.clarityMultiplier <= 0.01f ||
                re.ssaa < 1 || re.ssaa > 8 || !std::isfinite(re.fps) || re.fps <= 0.0f || re.threads == 0) {
                return false;
            }
            if (!std::isfinite(vi.data.defaultZoomIncrement) || vi.data.defaultZoomIncrement <= 1.0f ||
                !std::isfinite(vi.animation.overZoom) || vi.animation.overZoom < 0.0f ||
                !std::isfinite(vi.animation.mps) || vi.animation.mps <= 0.0f ||
                !std::isfinite(vi.exportation.fps) || vi.exportation.fps <= 0.0f ||
                vi.exportation.bitrate < 1 || vi.exportation.bitrate > 1000000 ||
                vi.exportation.keyframeAA < 1 || vi.exportation.keyframeAA > 8 ||
                vi.exportation.colorAA < 1 || vi.exportation.colorAA > 8 ||
                !enumInRange(vi.exportation.hdrTransfer, 0, 2) ||
                !std::isfinite(vi.exportation.hdrPeakNits) || vi.exportation.hdrPeakNits < 100.0f ||
                vi.exportation.hdrPeakNits > 10000.0f || !std::isfinite(vi.timeline.estimateKeyframes) ||
                vi.timeline.estimateKeyframes < 0.0f) {
                return false;
            }
            return ShaderPresetIO::validate(attr.shader);
        }
    }

    bool ConfigIO::save(const std::filesystem::path &path, const Attribute &attr,
                        const uint16_t width, const uint16_t height) {
        const std::filesystem::path temporary = IOUtilities::temporaryFilePath(path);
        std::ofstream out(temporary, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            vkh::logger::w_log(L"ERROR : Cannot save config");
            return false;
        }

        auto writeString = [&out](const std::string &s) {
            IOUtilities::encodeAndWrite(out, static_cast<uint64_t>(s.length()));
            IOUtilities::encodeAndWrite(out, s.data(), s.length());
        };

        IOUtilities::encodeAndWrite(out, MAGIC);
        IOUtilities::encodeAndWrite(out, VERSION);

        // Location (high-precision center stored as decimal strings)
        const auto &fr = attr.fractal;
        writeString(fr.center.real.to_string());
        writeString(fr.center.imag.to_string());
        IOUtilities::encodeAndWrite(out, fr.logZoom);
        IOUtilities::encodeAndWrite(out, fr.maxIteration);

        // Fractal
        IOUtilities::encodeAndWrite(out, fr.bailout);
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(fr.decimalizeIterationMethod));
        IOUtilities::encodeAndWrite(out, fr.mpaAttribute.minSkipReference);
        IOUtilities::encodeAndWrite(out, fr.mpaAttribute.maxMultiplierBetweenLevel);
        IOUtilities::encodeAndWrite(out, fr.mpaAttribute.epsilonPower);
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(fr.mpaAttribute.mpaSelectionMethod));
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(fr.mpaAttribute.mpaCompressionMethod));
        IOUtilities::encodeAndWrite(out, fr.referenceCompAttribute.compressCriteria);
        IOUtilities::encodeAndWrite(out, fr.referenceCompAttribute.compressionThresholdPower);
        IOUtilities::encodeAndWrite(out, fr.referenceCompAttribute.noCompressorNormalization);
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(fr.reuseReferenceMethod));
        IOUtilities::encodeAndWrite(out, fr.autoMaxIteration);
        IOUtilities::encodeAndWrite(out, fr.autoIterationMultiplier);
        IOUtilities::encodeAndWrite(out, fr.absoluteIterationMode);
        IOUtilities::encodeAndWrite(out, fr.rotation);
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(fr.formulaType));
        writeString(fr.customFormula);

        // Render
        const auto &re = attr.render;
        IOUtilities::encodeAndWrite(out, re.clarityMultiplier);
        IOUtilities::encodeAndWrite(out, re.ssaa);
        IOUtilities::encodeAndWrite(out, re.fps);
        IOUtilities::encodeAndWrite(out, re.linearInterpolation);
        IOUtilities::encodeAndWrite(out, re.threads);
        IOUtilities::encodeAndWrite(out, re.boundaryTraceFill);
        IOUtilities::encodeAndWrite(out, re.preview2Color);
        IOUtilities::encodeAndWrite(out, re.coarsePreview);

        // Resolution (client window size; not part of Attribute)
        IOUtilities::encodeAndWrite(out, width);
        IOUtilities::encodeAndWrite(out, height);

        // Shader
        ShaderPresetIO::writeShader(out, attr.shader);

        // Video
        const auto &vi = attr.video;
        IOUtilities::encodeAndWrite(out, vi.data.defaultZoomIncrement);
        IOUtilities::encodeAndWrite(out, vi.data.isStatic);
        IOUtilities::encodeAndWrite(out, vi.animation.overZoom);
        IOUtilities::encodeAndWrite(out, vi.animation.showText);
        IOUtilities::encodeAndWrite(out, vi.animation.mps);
        IOUtilities::encodeAndWrite(out, vi.exportation.fps);
        IOUtilities::encodeAndWrite(out, vi.exportation.bitrate);
        IOUtilities::encodeAndWrite(out, vi.exportation.keyframeAA);
        IOUtilities::encodeAndWrite(out, vi.exportation.autoCreateVideo);
        ShaderPresetIO::writeAnimationShape(out, attr.shader);
        // Appended last for backward compatibility: configs written before colorAA
        // existed simply lack this trailing field and fall back to the default.
        IOUtilities::encodeAndWrite(out, vi.exportation.colorAA);
        IOUtilities::encodeAndWrite(out, vi.exportation.pauseMainPreview);
        // Appended last: slope dual-scale relief (N3). Older configs lack these and fall back to defaults.
        IOUtilities::encodeAndWrite(out, attr.shader.slope.macroRelief);
        IOUtilities::encodeAndWrite(out, attr.shader.slope.macroRadius);
        // Appended last: exterior texture block.
        ShaderPresetIO::writeTexture(out, attr.shader, 0);
        // Appended last: generated pattern block.
        ShaderPresetIO::writePattern(out, attr.shader);
        // Appended last: fog rim mask. Older configs lack it and fall back to the unmasked fog.
        IOUtilities::encodeAndWrite(out, attr.shader.fog.rimMask);
        IOUtilities::encodeAndWrite(out, attr.shader.fog.rimMaskBoost);
        IOUtilities::encodeAndWrite(out, attr.shader.fog.rimBlur);
        IOUtilities::encodeAndWrite(out, attr.shader.fog.centerStart);
        IOUtilities::encodeAndWrite(out, attr.shader.fog.centerInvert);
        // Appended last: lossless video export. Older configs lack it and fall back to off.
        IOUtilities::encodeAndWrite(out, vi.exportation.lossless);
        // Appended last: texture layers 1 and up. Older configs lack them and keep the defaults.
        ShaderPresetIO::writeExtraTextureLayers(out, attr.shader);
        // Appended last: pausing the preview through keyframe generation. Older configs lack it.
        IOUtilities::encodeAndWrite(out, vi.exportation.pauseKeyframePreview);
        // Appended last: compressed keyframes. Older configs lack it and fall back to the default.
        IOUtilities::encodeAndWrite(out, vi.exportation.compressKeyframes);
        // Appended last: the OKLab blend choices. Older configs lack them and keep the original blends.
        ShaderPresetIO::writeOklabModes(out, attr.shader);
        // Appended last: dithering. Older configs lack it and keep whatever the session has.
        IOUtilities::encodeAndWrite(out, re.dither);
        // Appended last: the domain warp. Older configs lack it and keep the coloring unwarped.
        ShaderPresetIO::writeWarp(out, attr.shader);
        // Appended last: the slope lighting controls. Older configs lack these and take the
        // defaults, every one of which is the behaviour those configs were written under.
        IOUtilities::encodeAndWrite(out, attr.shader.slope.reliefResponse);
        IOUtilities::encodeAndWrite(out, attr.shader.slope.terminatorSoftness);
        IOUtilities::encodeAndWrite(out, attr.shader.slope.highlightKnee);
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(attr.shader.slope.lightBlend));
        // Appended last: the pattern's outline. Older configs lack it and keep the pattern unoutlined.
        ShaderPresetIO::writePatternEdge(out, attr.shader);
        // Appended last: the chromatic shading controls, the fog's focus band, and whether the
        // outline's width is relative. Older configs lack these and take the defaults, every one of
        // which is the behaviour those configs were written under. Behind a marker word, because a
        // config saved partway through this release's own work ends in a field no build writes
        // now, and its bytes would otherwise be read as the first of these.
        ShaderPresetIO::writeTrailer(out, attr.shader);
        // Appended last: the video timeline. Behind a marker of its own, because the block above it
        // may still grow and this one is counted rather than positional.
        TimelineIO::writeConfigBlock(out, vi.timeline);
        // Appended after the timeline rather than into the trailer above it: the trailer's fields are
        // guarded on bytes remaining, and past the trailer those bytes are the timeline's own marker.
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(attr.shader.fog.blurQuality));
        // Appended last: the HDR block and the HDR video output it feeds. An older config lacks both
        // and loads with HDR off, which is the picture that config was saved under.
        ShaderPresetIO::writeHdr(out, attr.shader);
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(vi.exportation.hdrTransfer));
        IOUtilities::encodeAndWrite(out, vi.exportation.hdrPeakNits);
        // Appended last: the palette's band lines. An older config carries none and leaves the
        // session's own alone, so a fresh start comes up with them off.
        ShaderPresetIO::writeBandLine(out, attr.shader);
        // Appended last: the slope's fill light. Older configs lack it and keep the single light.
        IOUtilities::encodeAndWrite(out, attr.shader.slope.fillIntensity);
        IOUtilities::encodeAndWrite(out, attr.shader.slope.fillZenith);
        IOUtilities::encodeAndWrite(out, attr.shader.slope.fillAzimuth);
        // Appended last: the palette's cycle bias. Older configs lack it and keep the straight mapping.
        IOUtilities::encodeAndWrite(out, attr.shader.palette.cycleBias);
        // Appended last: the palette's cycle curve choice. Older configs lack it and keep Power.
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(attr.shader.palette.cycleCurve));
        // Appended last: the bloom's linear sum. Older configs lack it and keep the encoded one they were drawn with.
        IOUtilities::encodeAndWrite(out, attr.shader.bloom.linearAdd);
        // Retained as zero so configs from the Line Depth prototype keep every later field aligned.
        IOUtilities::encodeAndWrite(out, 0.0f);
        // Appended last: the texture layers' Size and Keep Aspect. Older configs lack them and load stretched to a square tile, as they were drawn.
        ShaderPresetIO::writeTextureSize(out, attr.shader);
        // Appended last, behind a marker of its own: the slope's gloss. The marker is what makes a
        // config saved by the sheen prototype - which ends in bytes this build writes nothing for -
        // load with the gloss off rather than reading those bytes as the block's own fields.
        ShaderPresetIO::writeGloss(out, attr.shader);
        // Appended last, behind a marker of its own for the reason the gloss block carries one: the palette's iteration coloring.
        ShaderPresetIO::writePaletteColoring(out, attr.shader);
        // Appended last, behind a marker of its own for the reason the block above it carries one: the projection. An older config lacks it and loads on the flat view it was drawn with.
        IOUtilities::encodeAndWrite(out, PROJECTION_MAGIC);
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(fr.projectionMethod));
        IOUtilities::encodeAndWrite(out, fr.panoramaRange);
        IOUtilities::encodeAndWrite(out, fr.panoramaPitch);
        IOUtilities::encodeAndWrite(out, fr.panoramaFov);
        IOUtilities::encodeAndWrite(out, static_cast<int32_t>(fr.panoramaLayout));
        // Appended last, behind a marker of its own for the reason the block above it carries one: the gloss's Relief. An older config lacks it and keeps the session's value.
        ShaderPresetIO::writeGlossRelief(out, attr.shader);

        out.close();
        if (out.fail()) {
            IOUtilities::discardTemporaryFile(temporary);
            vkh::logger::w_log(L"ERROR : Cannot save config");
            return false;
        }
        if (!IOUtilities::commitTemporaryFile(temporary, path)) {
            IOUtilities::discardTemporaryFile(temporary);
            vkh::logger::w_log(L"ERROR : Cannot replace config");
            return false;
        }
        return true;
    }

    bool ConfigIO::load(const std::filesystem::path &path, Attribute &out,
                        uint16_t *width, uint16_t *height) {
        if (!std::filesystem::exists(path)) {
            return false;
        }
        std::ifstream in(path, std::ios::in | std::ios::binary);
        if (!in.is_open()) {
            return false;
        }

        auto readString = [&in](std::string &s) {
            uint64_t len = 0;
            IOUtilities::readAndDecode(in, &len);
            if (!IOUtilities::validateReadCount(in, len, sizeof(char), MAX_CONFIG_STRING_BYTES)) {
                return false;
            }
            s.resize(static_cast<size_t>(len));
            IOUtilities::readAndDecode(in, len, s.data());
            return !in.fail();
        };

        auto hasMore = [&in] {
            return in.rdbuf()->sgetc() != std::char_traits<char>::eof();
        };

        uint32_t magic;
        uint32_t version;
        IOUtilities::readAndDecode(in, &magic);
        IOUtilities::readAndDecode(in, &version);
        if (magic != MAGIC || version > VERSION || version < 3) {
            vkh::logger::w_log(L"ERROR : Not a valid config file");
            return false;
        }

        // Decode into a temporary, then commit only on success so a corrupt file
        // never leaves the live attribute half-overwritten.
        Attribute t = out;
        // The texture layers do not come along on that copy. They are read from an optional
        // trailing block, so a config written before that block existed would otherwise keep the
        // image the previous file had loaded: its pixels in every render and export, and its full
        // path written straight back out on the next save or recovery snapshot. A file that names
        // no texture now loads with none, the way a preset does - ShaderPresetIO::load decodes
        // into a default-constructed shader for the same reason.
        t.shader.textures = {};

        // Location (high-precision center stored as decimal strings)
        auto &fr = t.fractal;
        std::string realStr;
        std::string imagStr;
        if (!readString(realStr) || !readString(imagStr)) {
            vkh::logger::w_log(L"ERROR : Config file is corrupted");
            return false;
        }
        IOUtilities::readAndDecode(in, &fr.logZoom);
        IOUtilities::readAndDecode(in, &fr.maxIteration);
        if (in.fail() || !std::isfinite(fr.logZoom) || fr.logZoom < 0.0f ||
            fr.logZoom > static_cast<float>(MAX_CONFIG_STRING_BYTES) ||
            !fp_decimal_calculator::isValidString(realStr) ||
            !fp_decimal_calculator::isValidString(imagStr)) {
            vkh::logger::w_log(L"ERROR : Config file has an invalid location");
            return false;
        }
        fr.center = fp_complex(realStr, imagStr, Perturbator::logZoomToExp10(fr.logZoom));

        // Fractal
        IOUtilities::readAndDecode(in, &fr.bailout);
        int32_t decimalizeIterationMethod;
        IOUtilities::readAndDecode(in, &decimalizeIterationMethod);
        fr.decimalizeIterationMethod = static_cast<FrtDecimalizeIterationMethod>(decimalizeIterationMethod);
        IOUtilities::readAndDecode(in, &fr.mpaAttribute.minSkipReference);
        IOUtilities::readAndDecode(in, &fr.mpaAttribute.maxMultiplierBetweenLevel);
        IOUtilities::readAndDecode(in, &fr.mpaAttribute.epsilonPower);
        int32_t mpaSelectionMethod;
        IOUtilities::readAndDecode(in, &mpaSelectionMethod);
        fr.mpaAttribute.mpaSelectionMethod = static_cast<FrtMPASelectionMethod>(mpaSelectionMethod);
        int32_t mpaCompressionMethod;
        IOUtilities::readAndDecode(in, &mpaCompressionMethod);
        fr.mpaAttribute.mpaCompressionMethod = static_cast<FrtMPACompressionMethod>(mpaCompressionMethod);
        IOUtilities::readAndDecode(in, &fr.referenceCompAttribute.compressCriteria);
        IOUtilities::readAndDecode(in, &fr.referenceCompAttribute.compressionThresholdPower);
        IOUtilities::readAndDecode(in, &fr.referenceCompAttribute.noCompressorNormalization);
        int32_t reuseReferenceMethod;
        IOUtilities::readAndDecode(in, &reuseReferenceMethod);
        fr.reuseReferenceMethod = static_cast<FrtReuseReferenceMethod>(reuseReferenceMethod);
        IOUtilities::readAndDecode(in, &fr.autoMaxIteration);
        IOUtilities::readAndDecode(in, &fr.autoIterationMultiplier);
        IOUtilities::readAndDecode(in, &fr.absoluteIterationMode);
        IOUtilities::readAndDecode(in, &fr.rotation);
        int32_t formulaType;
        IOUtilities::readAndDecode(in, &formulaType);
        fr.formulaType = static_cast<FractalFormulaType>(formulaType);
        if (!readString(fr.customFormula)) {
            vkh::logger::w_log(L"ERROR : Config file is corrupted");
            return false;
        }

        // Render
        auto &re = t.render;
        IOUtilities::readAndDecode(in, &re.clarityMultiplier);
        IOUtilities::readAndDecode(in, &re.ssaa);
        IOUtilities::readAndDecode(in, &re.fps);
        IOUtilities::readAndDecode(in, &re.linearInterpolation);
        IOUtilities::readAndDecode(in, &re.threads);
        // Clamp the saved thread count to this machine's logical cores: a config saved on a 16-thread CPU is meaningless on an 8-thread one.
        if (const uint32_t hw = std::thread::hardware_concurrency(); hw > 0 && re.threads > hw) {
            re.threads = hw;
        }
        IOUtilities::readAndDecode(in, &re.boundaryTraceFill);
        IOUtilities::readAndDecode(in, &re.preview2Color);
        IOUtilities::readAndDecode(in, &re.coarsePreview);

        // Resolution (client window size; not part of Attribute)
        uint16_t w;
        uint16_t h;
        IOUtilities::readAndDecode(in, &w);
        IOUtilities::readAndDecode(in, &h);

        // Shader
        ShaderPresetIO::readShader(in, t.shader, version >= 4, version >= 5);

        // Video
        auto &vi = t.video;
        IOUtilities::readAndDecode(in, &vi.data.defaultZoomIncrement);
        IOUtilities::readAndDecode(in, &vi.data.isStatic);
        IOUtilities::readAndDecode(in, &vi.animation.overZoom);
        IOUtilities::readAndDecode(in, &vi.animation.showText);
        IOUtilities::readAndDecode(in, &vi.animation.mps);
        IOUtilities::readAndDecode(in, &vi.exportation.fps);
        IOUtilities::readAndDecode(in, &vi.exportation.bitrate);
        IOUtilities::readAndDecode(in, &vi.exportation.keyframeAA);

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &vi.exportation.autoCreateVideo);
        }

        if (hasMore()) {
            ShaderPresetIO::readAnimationShape(in, t.shader);
        }

        // Trailing optional field (see save()): older configs keep the default colorAA.
        if (hasMore()) {
            IOUtilities::readAndDecode(in, &vi.exportation.colorAA);
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &vi.exportation.pauseMainPreview);
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &t.shader.slope.macroRelief);
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &t.shader.slope.macroRadius);
        }

        if (hasMore()) {
            ShaderPresetIO::readTexture(in, t.shader, 0);
        }

        if (hasMore()) {
            ShaderPresetIO::readPattern(in, t.shader);
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &t.shader.fog.rimMask);
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &t.shader.fog.rimMaskBoost);
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &t.shader.fog.rimBlur);
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &t.shader.fog.centerStart);
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &t.shader.fog.centerInvert);
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &vi.exportation.lossless);
        }

        if (hasMore()) {
            ShaderPresetIO::readExtraTextureLayers(in, t.shader);
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &vi.exportation.pauseKeyframePreview);
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &vi.exportation.compressKeyframes);
        }

        if (hasMore()) {
            ShaderPresetIO::readOklabModes(in, t.shader);
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &re.dither);
        }

        if (hasMore()) {
            ShaderPresetIO::readWarp(in, t.shader);
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &t.shader.slope.reliefResponse);
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &t.shader.slope.terminatorSoftness);
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &t.shader.slope.highlightKnee);
        }

        if (hasMore()) {
            int32_t lightBlend;
            IOUtilities::readAndDecode(in, &lightBlend);
            // A config may name a mode this build no longer has; fall back to the original composite.
            t.shader.slope.lightBlend = lightBlend == 1
                                            ? ShdSlopeLightBlend::LINEAR
                                            : ShdSlopeLightBlend::DIRECT;
        }

        if (hasMore()) {
            ShaderPresetIO::readPatternEdge(in, t.shader);
        }

        if (hasMore()) {
            ShaderPresetIO::readTrailer(in, t.shader);
        }

        // Older configs carry no timeline and leave the session's own alone, the way the Warp and
        // Pattern Edge blocks were added.
        if (hasMore()) {
            TimelineIO::readConfigBlock(in, vi.timeline);
        }

        if (hasMore()) {
            int32_t blurQuality = 0;
            IOUtilities::readAndDecode(in, &blurQuality);
            // A config may name a mode this build no longer has; fall back to the ceiling every
            // earlier version rendered under.
            t.shader.fog.blurQuality = blurQuality == 1
                                           ? ShdFogBlurQuality::APPEARANCE
                                           : ShdFogBlurQuality::SPEED;
        }

        if (hasMore()) {
            ShaderPresetIO::readHdr(in, t.shader);
        }

        if (hasMore()) {
            int32_t transfer = 0;
            IOUtilities::readAndDecode(in, &transfer);
            // A config may name a curve this build no longer has; fall back to the SDR picture.
            vi.exportation.hdrTransfer = transfer >= 0 && transfer <= 2
                                             ? static_cast<VidHdrTransfer>(transfer)
                                             : VidHdrTransfer::SDR;
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &vi.exportation.hdrPeakNits);
        }

        if (hasMore()) {
            ShaderPresetIO::readBandLine(in, t.shader);
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &t.shader.slope.fillIntensity);
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &t.shader.slope.fillZenith);
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &t.shader.slope.fillAzimuth);
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &t.shader.palette.cycleBias);
        }

        if (hasMore()) {
            int32_t cycleCurve;
            IOUtilities::readAndDecode(in, &cycleCurve);
            // A config may name a curve this build no longer has; fall back to the power mapping.
            t.shader.palette.cycleCurve = cycleCurve == 1
                                              ? ShdPaletteCycleCurve::WAVE
                                              : ShdPaletteCycleCurve::POWER;
        }

        if (hasMore()) {
            IOUtilities::readAndDecode(in, &t.shader.bloom.linearAdd);
        }

        if (hasMore()) {
            float removedBandLineDepth;
            IOUtilities::readAndDecode(in, &removedBandLineDepth);
        }

        if (hasMore()) {
            ShaderPresetIO::readTextureSize(in, t.shader);
        } else {
            ShaderPresetIO::clearLegacyTextureSize(t.shader);
        }

        if (hasMore()) {
            ShaderPresetIO::readGloss(in, t.shader);
        }

        if (hasMore()) {
            ShaderPresetIO::readPaletteColoring(in, t.shader);
        }

        if (hasMore()) {
            uint32_t blockMagic = 0;
            IOUtilities::readAndDecode(in, &blockMagic);
            const bool recognized = !in.fail() && blockMagic == PROJECTION_MAGIC;
            if (recognized) {
                // Guarded one field at a time, so a config written between the two of them loads.
                int32_t projectionMethod;
                IOUtilities::readAndDecode(in, &projectionMethod);
                if (!in.fail()) {
                    fr.projectionMethod = static_cast<FrtProjectionMethod>(projectionMethod);
                }
                if (hasMore()) {
                    IOUtilities::readAndDecode(in, &fr.panoramaRange);
                }
                if (hasMore()) {
                    IOUtilities::readAndDecode(in, &fr.panoramaPitch);
                }
                if (hasMore()) {
                    IOUtilities::readAndDecode(in, &fr.panoramaFov);
                }
                if (hasMore()) {
                    int32_t panoramaLayout;
                    IOUtilities::readAndDecode(in, &panoramaLayout);
                    if (!in.fail()) {
                        fr.panoramaLayout = static_cast<FrtPanoramaLayout>(panoramaLayout);
                    }
                }
            }
            // Short of what the block asked for means it is simply not in this file, not corruption.
            if (in.fail() && !recognized) {
                in.clear();
            }
        }

        if (hasMore()) {
            ShaderPresetIO::readGlossRelief(in, t.shader);
        }

        if (in.fail() || !validateConfig(t)) {
            vkh::logger::w_log(L"ERROR : Config file is corrupted");
            return false;
        }

        out = std::move(t);
        if (width != nullptr) {
            *width = w;
        }
        if (height != nullptr) {
            *height = h;
        }
        return true;
    }

    bool ConfigIO::loadShader(const std::filesystem::path &path, ShaderAttribute &out) {
        // The shader's fields sit all the way down the stream, so the file is decoded whole into a
        // scratch attribute - built around a center the loader overwrites - and only its shader kept.
        Attribute scratch{.fractal = FractalAttribute{.center = fp_complex("0", "0", 0)}};
        if (!load(path, scratch, nullptr, nullptr)) {
            return false;
        }
        out = std::move(scratch.shader);
        return true;
    }
}
