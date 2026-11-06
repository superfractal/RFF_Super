//
// Created and modified by AI; earlier exact dates unavailable.

//
// Modified by Opus 5 on 2026-08-05, 2026-08-07, 2026-08-13, 2026-08-14, 2026-08-15, 2026-08-17, 2026-08-19, 2026-08-20, 2026-08-22, 2026-08-27, 2026-08-29, 2026-08-31
// Modified by GPT-5 on 2026-08-21, 2026-08-31
// Modified by Fable 5.1 on 2026-09-02
// Modified by GPT-6 on 2026-09-10, 2026-09-11, 2026-09-12, 2026-09-14, 2026-09-16, 2026-09-18, 2026-09-19, 2026-09-20, 2026-09-23, 2026-09-24
// Modified by Opus 5.5 on 2026-10-03, 2026-10-06, 2026-10-07, 2026-10-08, 2026-10-09, 2026-10-12, 2026-10-16, 2026-10-20, 2026-10-22, 2026-10-23, 2026-10-24, 2026-10-29, 2026-11-04, 2026-11-05
//

#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>
#include <vector>

#include "../attr/ShaderAttribute.h"

namespace merutilm::rff2 {
    // Saves/loads a full ShaderAttribute bundle (palette, stripe, slope, color, fog, bloom)
    // to a single binary file, letting users store and recall their own shader looks.
    struct ShaderPresetIO {
        ShaderPresetIO() = delete;

        static constexpr uint32_t MAGIC = 0x52465350; // "RFSP"
        // v2 added the hybrid palette section (recipe {id, seed} or alpha-less raw colors).
        // v3 added the eyedropper frozen-color list (tolerance + iteration values).
        static constexpr uint32_t VERSION = 5;

        static bool save(const std::filesystem::path &path, const ShaderAttribute &shader);

        static bool load(const std::filesystem::path &path, ShaderAttribute &out);

        [[nodiscard]] static bool validate(const ShaderAttribute &shader);

        // Stream-level helpers (no magic/version), reused by the full-config serializer.
        static void writeShader(std::ostream &out, const ShaderAttribute &shader);

        // newPaletteFormat=false reads the legacy palette section (full RGBA color array, no recipe).
        // hasFrozenColors=false skips the eyedropper frozen-color list (absent before v3).
        static void readShader(std::ifstream &in, ShaderAttribute &out, bool newPaletteFormat, bool hasFrozenColors, bool legacyLayout = false);

        static void writeAnimationShape(std::ostream &out, const ShaderAttribute &shader);

        static void readAnimationShape(std::ifstream &in, ShaderAttribute &out);

        // One exterior texture layer. Layer 0 is written where the single texture block always sat,
        // appended after every earlier field so old files stay readable.
        static void writeTexture(std::ostream &out, const ShaderAttribute &shader, uint32_t layer);

        static void readTexture(std::ifstream &in, ShaderAttribute &out, uint32_t layer);

        // Generated pattern block, appended after the texture block for the same reason.
        static void writePattern(std::ostream &out, const ShaderAttribute &shader);

        static void readPattern(std::ifstream &in, ShaderAttribute &out);

        // Texture layers 1 and up, appended at the very end so a file written before the stack
        // existed still loads with those layers left at their defaults.
        static void writeExtraTextureLayers(std::ostream &out, const ShaderAttribute &shader);

        static void readExtraTextureLayers(std::ifstream &in, ShaderAttribute &out);

        // Every layer's Size and Keep Aspect, appended at the very end so a file written before they existed still reads every field that follows the layer's own block.
        static void writeTextureSize(std::ostream &out, const ShaderAttribute &shader);

        // A file carrying no such block was written with the image stretched to a square tile, which is what clearLegacyTextureSize leaves the layers on.
        static void readTextureSize(std::ifstream &in, ShaderAttribute &out);

        static void clearLegacyTextureSize(ShaderAttribute &out);

        // Palette interpolation space and slope shading blend, appended at the very end for the same reason.
        static void writeOklabModes(std::ostream &out, const ShaderAttribute &shader);

        static void readOklabModes(std::ifstream &in, ShaderAttribute &out);

        // Domain warp block, appended at the very end for the same reason.
        static void writeWarp(std::ostream &out, const ShaderAttribute &shader);

        static void readWarp(std::ifstream &in, ShaderAttribute &out);

        // The pattern's outline. Written at the very end rather than inside the pattern block, so a
        // file from a build without it still reads every field that follows that block.
        static void writePatternEdge(std::ostream &out, const ShaderAttribute &shader);

        static void readPatternEdge(std::ifstream &in, ShaderAttribute &out);

        // Marks the start of the block below. Every trailing field before it is bare, which only
        // works while every build that ever wrote the file agreed on where the last one ended: a
        // file carrying a field this build does not know hands its bytes to the first field of the
        // block instead, and each of those is a legal value on its own, so nothing downstream can
        // tell. A settings file saved partway through this release's own work does carry one.
        static constexpr uint32_t TRAILER_MAGIC = 0x4C325348; // "L2SH"

        // Everything appended after the pattern's outline - the chromatic shading controls, the
        // fog's focus band, and whether the outline's width is relative - behind that marker.
        static void writeTrailer(std::ostream &out, const ShaderAttribute &shader);

        // Leaves every field of the block at its default when the marker is not the one written,
        // which is also the point the read stops: past an unknown field nothing can be located.
        static void readTrailer(std::ifstream &in, ShaderAttribute &out);

        // The HDR block, written after every field above it for the same reason each of those was:
        // a file from a build without it loads with HDR off, which is what that build rendered.
        static void writeHdr(std::ostream &out, const ShaderAttribute &shader);

        static void readHdr(std::ifstream &in, ShaderAttribute &out);

        // The palette's band lines, written after every field above it for the same reason: a file
        // from a build without them loads with the lines off, which is the picture it was written under.
        static void writeChaosBlur(std::ostream &out, const ShaderAttribute &shader);

        static void readChaosBlur(std::ifstream &in, ShaderAttribute &shader, bool legacyHighlights);

        static void writeChaosAverage(std::ostream &out, const ShaderAttribute &shader);

        static void readChaosAverage(std::ifstream &in, ShaderAttribute &shader);

        static void writeChaosInterior(std::ostream &out, const ShaderAttribute &shader);

        static void readChaosInterior(std::ifstream &in, ShaderAttribute &shader);

        // Marks the palette's Escape Angle block: uint32 mode, then float32 amount in cycles.
        static constexpr uint32_t ESCAPE_ANGLE_MAGIC = 0x31434145; // "EAC1"

        static void writeEscapeAngle(std::ostream &out, const ShaderAttribute &shader);

        // A file without the block loads with the coloring off; a wrong marker or a cut-short block fails the stream.
        static void readEscapeAngle(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Waves on Clear Coat Only block: one uint32 flag, 0 or 1.
        static constexpr uint32_t WAVES_COAT_ONLY_MAGIC = 0x314F4357; // "WCO1"

        static void writeWavesCoatOnly(std::ostream &out, const ShaderAttribute &shader);

        // A file without the block loads with the switch off; a wrong marker, a cut-short block or a flag above one fails the stream.
        static void readWavesCoatOnly(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Coat Softbox block: one uint32 flag, 0 or 1.
        static constexpr uint32_t COAT_SOFTBOX_MAGIC = 0x31425343; // "CSB1"

        static void writeCoatSoftbox(std::ostream &out, const ShaderAttribute &shader);

        // A file without the block loads with the switch off; a wrong marker, a cut-short block or a flag above one fails the stream.
        static void readCoatSoftbox(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Chaos Edge Protection block: one uint32 flag, 0 or 1.
        static constexpr uint32_t CHAOS_EDGES_MAGIC = 0x31504543; // "CEP1"

        static void writeChaosEdges(std::ostream &out, const ShaderAttribute &shader);

        // A file without the block loads with the switch off; a wrong marker, a cut-short block or a flag above one fails the stream.
        static void readChaosEdges(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Chaos Bokeh block: uint32 shape, float32 rotation in degrees, float32 aspect.
        static constexpr uint32_t CHAOS_BOKEH_MAGIC = 0x314B4243; // "CBK1"

        static void writeChaosBokeh(std::ostream &out, const ShaderAttribute &shader);

        // A file without the block loads Circle, 0 and 1; a wrong marker, a cut-short block or a value out of range fails the stream.
        static void readChaosBokeh(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Smooth Focus Across Keyframes block: one uint32 flag, 0 or 1.
        static constexpr uint32_t FOCUS_SMOOTH_MAGIC = 0x314B5346; // "FSK1"

        static void writeFocusSmooth(std::ostream &out, const ShaderAttribute &shader);

        // A file without the block loads with the switch off; a wrong marker, a cut-short block or a flag above one fails the stream.
        static void readFocusSmooth(std::ifstream &in, ShaderAttribute &shader);

        // Marks the palette's Orbit Trap block: uint32 mode, then float32 amount, width and density.
        static constexpr uint32_t ORBIT_TRAP_MAGIC = 0x3150544F; // "OTP1"

        static void writeOrbitTrap(std::ostream &out, const ShaderAttribute &shader);

        // A file without the block loads with the coloring off; a wrong marker or a cut-short block fails the stream.
        static void readOrbitTrap(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Orbit Trap Inside Set block: one uint32 flag, 0 or 1.
        static constexpr uint32_t ORBIT_TRAP_INTERIOR_MAGIC = 0x3149544F; // "OTI1"

        static void writeOrbitTrapInterior(std::ostream &out, const ShaderAttribute &shader);

        // A file without the block loads with the switch on; a wrong marker, a cut-short block or a flag above one fails the stream.
        static void readOrbitTrapInterior(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Orbit Trap region block: one uint32 region (0 outside, 1 both, 2 inside), written only for Inside Only.
        static constexpr uint32_t ORBIT_TRAP_REGION_MAGIC = 0x3152544F; // "OTR1"

        static void writeOrbitTrapRegion(std::ostream &out, const ShaderAttribute &shader);

        // Without the block OTI1's region stands; another marker is left unread, a cut-short block or a region above two fails the stream.
        static void readOrbitTrapRegion(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Orbit Trap shape extension block: uint32 shape (5 to ORBIT_TRAP_SHAPE_LAST), then uint32 sides
        // (3 to 64), written only for a shape past Line, which OTS1 stores as the nearest of Point to Line.
        static constexpr uint32_t ORBIT_TRAP_SHAPE_EXTENSION_MAGIC = 0x3158544F; // "OTX1"

        static void writeOrbitTrapShapeExtension(std::ostream &out, const ShaderAttribute &shader);

        // Without the block OTS1's shape stands and the sides load 5; another marker is left unread, a cut-short
        // block or a value out of range fails the stream.
        static void readOrbitTrapShapeExtension(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Vignette block: float32 strength, radius and richness, then uint32 shape (0 Ellipse, 1 Frame),
        // written only when one differs from its default.
        static constexpr uint32_t VIGNETTE_MAGIC = 0x31474956; // "VIG1"

        static void writeVignette(std::ostream &out, const ShaderAttribute &shader);

        // Without the block the vignette loads off at its default radius, richness and shape; another marker is left unread,
        // a cut-short block or a value out of range fails the stream.
        static void readVignette(std::ifstream &in, ShaderAttribute &shader);

        // Marks the OpenPBR block, written only off its defaults: uint32 model, then float32 coat IOR, darkening, coat color, edge tint, fuzz, fuzz roughness, fuzz color and film IOR.
        static constexpr uint32_t OPENPBR_MAGIC = 0x3142504F; // "OPB1"

        static void writeOpenPbr(std::ostream &out, const ShaderAttribute &shader);

        // Without the block Studio loads as Studio GGX with the OpenPBR defaults; another marker is left unread, a bad block fails the stream.
        static void readOpenPbr(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Anisotropy Direction block, written only off Fixed Angle: uint32 direction (1 Along Bands, 2 Across Bands).
        static constexpr uint32_t ANISOTROPY_DIRECTION_MAGIC = 0x31494E41; // "ANI1"

        static void writeAnisotropyDirection(std::ostream &out, const ShaderAttribute &shader);

        // Without the block the direction loads as Fixed Angle; another marker is left unread, a bad block fails the stream.
        static void readAnisotropyDirection(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Relief Cap block, written only when the cap is on: float32 periods above the lowest escaped iteration.
        static constexpr uint32_t RELIEF_CAP_MAGIC = 0x31504352; // "RCP1"

        static void writeReliefCap(std::ostream &out, const ShaderAttribute &shader);

        // Without the block Relief Cap loads off; another marker is left unread, a bad block fails the stream.
        static void readReliefCap(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Orbit Trap look block: float32 offset and speed, four float32 glow color, float32 strength
        // and sharpness, uint32 blend, then float32 border width and border darkness.
        static constexpr uint32_t ORBIT_TRAP_LOOK_MAGIC = 0x324C544F; // "OTL2"
        // The same block with a trailing float32 inside amount, a setting since removed; read and discarded.
        static constexpr uint32_t ORBIT_TRAP_LOOK_V1_MAGIC = 0x314C544F; // "OTL1"

        static void writeOrbitTrapLook(std::ostream &out, const ShaderAttribute &shader);

        // A file without the block loads the defaults; a wrong marker or a cut-short block fails the stream.
        static void readOrbitTrapLook(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Orbit Trap Shape block: uint32 shape, then float64 center real, center imaginary, radius and angle.
        static constexpr uint32_t ORBIT_TRAP_SHAPE_MAGIC = 0x3153544F; // "OTS1"

        static void writeOrbitTrapShape(std::ostream &out, const ShaderAttribute &shader);

        // A file without the block loads a Point at the origin; a wrong marker or a cut-short block fails the stream.
        static void readOrbitTrapShape(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Effect Styles block: per layer, uint32 Flame, Embers, Mist, Ripples, Flow Light and Aurora styles.
        static constexpr uint32_t EFFECT_STYLES_MAGIC = 0x31534645; // "EFS1"

        static void writeEffectStyles(std::ostream &out, const ShaderAttribute &shader);

        // A file without the block loads every style as Classic; a wrong marker or a cut-short block fails the stream.
        static void readEffectStyles(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Orbit Trap hold block: one uint32 flag, 0 or 1, for holding the trap still under color animation.
        static constexpr uint32_t ORBIT_TRAP_HOLD_MAGIC = 0x3148544F; // "OTH1"

        static void writeOrbitTrapHold(std::ostream &out, const ShaderAttribute &shader);

        // A file without the block loads with the switch off; a wrong marker, a cut-short block or a flag above one fails the stream.
        static void readOrbitTrapHold(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Smooth Grooves block: one uint32 flag, 0 or 1.
        static constexpr uint32_t GROOVE_SMOOTH_MAGIC = 0x314D5347; // "GSM1"

        static void writeGrooveSmooth(std::ostream &out, const ShaderAttribute &shader);

        // A file without the block loads with the switch off; a wrong marker, a cut-short block or a flag above one fails the stream.
        static void readGrooveSmooth(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Rounded Bands block: one uint32 flag, 0 or 1.
        static constexpr uint32_t GROOVE_ROUNDED_MAGIC = 0x31425247; // "GRB1"

        static void writeGrooveRounded(std::ostream &out, const ShaderAttribute &shader);

        // A file without the block loads with the switch off; a wrong marker, a cut-short block or a flag above one fails the stream.
        static void readGrooveRounded(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Crease Blur block: one float32 width, 0 to GROOVE_CREASE_BLUR_MAX.
        static constexpr uint32_t GROOVE_CREASE_BLUR_MAGIC = 0x31424347; // "GCB1"

        static void writeGrooveCreaseBlur(std::ostream &out, const ShaderAttribute &shader);

        // A file without the block loads 0; a wrong marker, a cut-short block or a width out of range fails the stream.
        static void readGrooveCreaseBlur(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Orbit Trap mapping block: uint32 mapping, uint32 arms (1 to ORBIT_TRAP_ARMS_MAX), uint32 relief flag.
        static constexpr uint32_t ORBIT_TRAP_MAPPING_MAGIC = 0x314D544F; // "OTM1"

        static void writeOrbitTrapMapping(std::ostream &out, const ShaderAttribute &shader);

        // A file without the block loads Distance, 3 arms and no relief; a wrong marker, a cut-short block or a value out of range fails the stream.
        static void readOrbitTrapMapping(std::ifstream &in, ShaderAttribute &shader);

        // Marks the Environment Source block: uint32 source, float32 brightness, then four float32 position, r, g, b stops.
        static constexpr uint32_t STUDIO_ENVIRONMENT_MAGIC = 0x314E4553; // "SEN1"

        static void writeStudioEnvironment(std::ostream &out, const ShaderAttribute &shader);

        // A file without the block loads Two Colors and the default stops; a wrong marker, a cut-short block or a value out of range fails the stream.
        static void readStudioEnvironment(std::ifstream &in, ShaderAttribute &shader);

        static void writeSurfaceReplacement(std::ostream &out, const ShaderAttribute &shader);

        static void readSurfaceReplacement(std::ifstream &in, ShaderAttribute &shader);

        static void writeBandDecorations(std::ostream &out, const ShaderAttribute &shader);

        static void readBandDecorations(std::ifstream &in, ShaderAttribute &shader);

        static void writeBandLine(std::ostream &out, const ShaderAttribute &shader);

        static void readBandLine(std::ifstream &in, ShaderAttribute &out);

        // Marks the start of the gloss block below, and is there for the same reason TRAILER_MAGIC
        // is: every trailing field before it is bare, so a file carrying one this build does not
        // write hands its bytes to the block's first field instead, and each of those is a legal
        // value on its own. A settings file saved by a build that carried the sheen prototype ends
        // in exactly such a field.
        static constexpr uint32_t GLOSS_MAGIC = 0x474C5353; // "GLSS"

        // The slope's gloss, written after every field above it and behind that marker. A file
        // whose last word is not the marker is treated as carrying no gloss at all, the destination
        // is left alone, and nothing further is read - which is the picture that file was written
        // under, since the gloss is off in every build that did not write this block.
        static void writeGloss(std::ostream &out, const ShaderAttribute &shader);

        static void readGloss(std::ifstream &in, ShaderAttribute &out);

        // Marks the start of the palette's iteration coloring block below, for the reason GLOSS_MAGIC is there: the gloss block ends the stream in every build before this one.
        static constexpr uint32_t PALETTE_COLORING_MAGIC = 0x50434C52; // "PCLR"

        // The palette's Iteration Coloring mode, written after the gloss block and behind that marker. A file whose next four bytes are not the marker is treated as carrying no such block, the destination is left alone, and nothing further is read - which is the straight count every build without it drew.
        static void writePaletteColoring(std::ostream &out, const ShaderAttribute &shader);

        static void readPaletteColoring(std::ifstream &in, ShaderAttribute &out);

        // Marks the gloss relief block below, for the reason the blocks above carry one: in a config the projection block ends the stream before it, and its last field is a legal int.
        static constexpr uint32_t GLOSS_RELIEF_MAGIC = 0x474C524C; // "GLRL"

        // The gloss's Relief, one float behind that marker. It is the last block of a shader preset and of a config. A file whose next four bytes are not the marker is treated as carrying none, the destination is left alone, and nothing further is read.
        static void writeGlossRelief(std::ostream &out, const ShaderAttribute &shader);

        static void readGlossRelief(std::ifstream &in, ShaderAttribute &out);

        static constexpr uint32_t STUDIO_LIGHTING_MAGIC = 0x53424C31;

        static void writeStudioLighting(std::ostream &out, const ShaderAttribute &shader);

        static void readStudioLighting(std::ifstream &in, ShaderAttribute &shader, bool legacyLayout = false);

        static constexpr uint32_t STUDIO_MAGIC = 0x53544758;

        static void writeEffects(std::ostream &out, const ShaderAttribute &shader);

        static void readEffects(std::ifstream &in, ShaderAttribute &shader);

        static void writeEffectSync(std::ostream &out, const ShaderAttribute &shader);

        static void readEffectSync(std::ifstream &in, ShaderAttribute &shader);

        static void writeLayerOrder(std::ostream &out, const ShaderAttribute &shader);

        static void readLayerOrder(std::ifstream &in, ShaderAttribute &shader);

        static void writeChrome(std::ostream &out, const ShaderAttribute &shader);

        static void readChrome(std::ifstream &in, ShaderAttribute &shader, bool legacyLayout = false);

        static void writeGroove(std::ostream &out, const ShaderAttribute &shader);

        static void readGroove(std::ifstream &in, ShaderAttribute &shader);

        static void writeSurface(std::ostream &out, const ShaderAttribute &shader);

        static void readSurface(std::ifstream &in, ShaderAttribute &shader);

        static void writeStudio(std::ostream &out, const ShaderAttribute &shader);

        static void readStudio(std::ifstream &in, ShaderAttribute &out, bool legacyLayout = false);

        // "Layer N: <path>" for every texture layer whose saved image is not on disk. Only the path
        // is stored, so a file written on another machine - or one whose image has since moved -
        // loads with those layers painting nothing; the caller warns with this.
        static std::vector<std::wstring> missingTextureImages(const ShaderAttribute &shader);
    };
}
