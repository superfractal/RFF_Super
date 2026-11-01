//
// Created and modified by AI; earlier exact dates unavailable.

//
// Modified by GPT-5 on 2026-08-21
// Modified by Opus 5 on 2026-08-31
// Modified by GPT-6 on 2026-09-14, 2026-09-23, 2026-09-24, 2026-09-25
// Modified by Opus 5.5 on 2026-10-03, 2026-10-06, 2026-10-07, 2026-10-10, 2026-10-26
//

#pragma once
#include <cstdint>
#include <filesystem>
#include <ostream>

#include "../attr/Attribute.h"

namespace merutilm::rff2 {
    // Saves/loads the full settings bundle (view location, Fractal calc/formula/rotation,
    // Render, Resolution, Shader, Video) to a single binary file.
    struct ConfigIO {
        ConfigIO() = delete;

        static constexpr uint32_t MAGIC = 0x52464643; // "RFFC"
        static constexpr uint32_t VERSION = 8;
        // Marks the projection block, behind a marker of its own for the same reason.
        static constexpr uint32_t PROJECTION_MAGIC = 0x50524A43; // "PRJC"
        // Marks the GPU perturbation block appended after CHI1.
        static constexpr uint32_t GPU_PERTURBATION_MAGIC = 0x31555047; // "GPU1"
        // Marks the Adaptive SSAA block appended after GPU1.
        static constexpr uint32_t ADAPTIVE_SSAA_MAGIC = 0x31415341; // "ASA1"
        // Marks the SSAA Filter block appended after EAC1.
        static constexpr uint32_t SSAA_FILTER_MAGIC = 0x31465353; // "SSF1"
        // Marks the Video Output Size block appended after FSK1.
        static constexpr uint32_t OUTPUT_SIZE_MAGIC = 0x31534F56; // "VOS1"
        // Marks the Buddhabrot block appended after OTM1.
        static constexpr uint32_t BUDDHABROT_MAGIC = 0x31424442; // "BDB1"

        // The exact RFC byte stream, shared by saving and document-state comparison.
        static void write(std::ostream &out, const Attribute &attr, uint16_t width, uint16_t height);

        static bool save(const std::filesystem::path &path, const Attribute &attr,
                         uint16_t width, uint16_t height);

        // Loads into the existing attribute in place (including the view location).
        // The saved resolution (client size) is returned via width/height.
        static bool load(const std::filesystem::path &path, Attribute &out,
                         uint16_t *width, uint16_t *height);

        // Only the shader of a config, for taking a look out of a settings file and leaving the
        // location and everything else in it alone. Leaves out untouched when the file cannot be read.
        static bool loadShader(const std::filesystem::path &path, ShaderAttribute &out);
    };
}
