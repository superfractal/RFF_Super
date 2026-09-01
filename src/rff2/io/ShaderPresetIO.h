//
// Created and modified by AI; earlier exact dates unavailable.
// Modified by GPT-5 on 2026-08-21, 2026-08-31.
// Modified by Opus 5 on 2026-08-05, 2026-08-07, 2026-08-13, 2026-08-14, 2026-08-15, 2026-08-17, 2026-08-19, 2026-08-20, 2026-08-22, 2026-08-27, 2026-08-29, 2026-08-31
//

#pragma once
#include <filesystem>
#include <fstream>
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
        static constexpr uint32_t VERSION = 3;

        static bool save(const std::filesystem::path &path, const ShaderAttribute &shader);

        static bool load(const std::filesystem::path &path, ShaderAttribute &out);

        [[nodiscard]] static bool validate(const ShaderAttribute &shader);

        // Stream-level helpers (no magic/version), reused by the full-config serializer.
        static void writeShader(std::ofstream &out, const ShaderAttribute &shader);

        // newPaletteFormat=false reads the legacy palette section (full RGBA color array, no recipe).
        // hasFrozenColors=false skips the eyedropper frozen-color list (absent before v3).
        static void readShader(std::ifstream &in, ShaderAttribute &out, bool newPaletteFormat, bool hasFrozenColors);

        static void writeAnimationShape(std::ofstream &out, const ShaderAttribute &shader);

        static void readAnimationShape(std::ifstream &in, ShaderAttribute &out);

        // One exterior texture layer. Layer 0 is written where the single texture block always sat,
        // appended after every earlier field so old files stay readable.
        static void writeTexture(std::ofstream &out, const ShaderAttribute &shader, uint32_t layer);

        static void readTexture(std::ifstream &in, ShaderAttribute &out, uint32_t layer);

        // Generated pattern block, appended after the texture block for the same reason.
        static void writePattern(std::ofstream &out, const ShaderAttribute &shader);

        static void readPattern(std::ifstream &in, ShaderAttribute &out);

        // Texture layers 1 and up, appended at the very end so a file written before the stack
        // existed still loads with those layers left at their defaults.
        static void writeExtraTextureLayers(std::ofstream &out, const ShaderAttribute &shader);

        static void readExtraTextureLayers(std::ifstream &in, ShaderAttribute &out);

        // Every layer's Size and Keep Aspect, appended at the very end so a file written before they existed still reads every field that follows the layer's own block.
        static void writeTextureSize(std::ofstream &out, const ShaderAttribute &shader);

        // A file carrying no such block was written with the image stretched to a square tile, which is what clearLegacyTextureSize leaves the layers on.
        static void readTextureSize(std::ifstream &in, ShaderAttribute &out);

        static void clearLegacyTextureSize(ShaderAttribute &out);

        // Palette interpolation space and slope shading blend, appended at the very end for the same reason.
        static void writeOklabModes(std::ofstream &out, const ShaderAttribute &shader);

        static void readOklabModes(std::ifstream &in, ShaderAttribute &out);

        // Domain warp block, appended at the very end for the same reason.
        static void writeWarp(std::ofstream &out, const ShaderAttribute &shader);

        static void readWarp(std::ifstream &in, ShaderAttribute &out);

        // The pattern's outline. Written at the very end rather than inside the pattern block, so a
        // file from a build without it still reads every field that follows that block.
        static void writePatternEdge(std::ofstream &out, const ShaderAttribute &shader);

        static void readPatternEdge(std::ifstream &in, ShaderAttribute &out);

        // Marks the start of the block below. Every trailing field before it is bare, which only
        // works while every build that ever wrote the file agreed on where the last one ended: a
        // file carrying a field this build does not know hands its bytes to the first field of the
        // block instead, and each of those is a legal value on its own, so nothing downstream can
        // tell. A settings file saved partway through this release's own work does carry one.
        static constexpr uint32_t TRAILER_MAGIC = 0x4C325348; // "L2SH"

        // Everything appended after the pattern's outline - the chromatic shading controls, the
        // fog's focus band, and whether the outline's width is relative - behind that marker.
        static void writeTrailer(std::ofstream &out, const ShaderAttribute &shader);

        // Leaves every field of the block at its default when the marker is not the one written,
        // which is also the point the read stops: past an unknown field nothing can be located.
        static void readTrailer(std::ifstream &in, ShaderAttribute &out);

        // The HDR block, written after every field above it for the same reason each of those was:
        // a file from a build without it loads with HDR off, which is what that build rendered.
        static void writeHdr(std::ofstream &out, const ShaderAttribute &shader);

        static void readHdr(std::ifstream &in, ShaderAttribute &out);

        // The palette's band lines, written after every field above it for the same reason: a file
        // from a build without them loads with the lines off, which is the picture it was written under.
        static void writeBandLine(std::ofstream &out, const ShaderAttribute &shader);

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
        static void writeGloss(std::ofstream &out, const ShaderAttribute &shader);

        static void readGloss(std::ifstream &in, ShaderAttribute &out);

        // Marks the start of the palette's iteration coloring block below, for the reason GLOSS_MAGIC is there: the gloss block ends the stream in every build before this one.
        static constexpr uint32_t PALETTE_COLORING_MAGIC = 0x50434C52; // "PCLR"

        // The palette's Iteration Coloring mode, written after the gloss block and behind that marker. A file whose next four bytes are not the marker is treated as carrying no such block, the destination is left alone, and nothing further is read - which is the straight count every build without it drew.
        static void writePaletteColoring(std::ofstream &out, const ShaderAttribute &shader);

        static void readPaletteColoring(std::ifstream &in, ShaderAttribute &out);

        // "Layer N: <path>" for every texture layer whose saved image is not on disk. Only the path
        // is stored, so a file written on another machine - or one whose image has since moved -
        // loads with those layers painting nothing; the caller warns with this.
        static std::vector<std::wstring> missingTextureImages(const ShaderAttribute &shader);
    };
}
