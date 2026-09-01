//
// Created and modified by AI; earlier exact dates unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-31
//

#pragma once
#include <filesystem>

#include "../attr/Attribute.h"

namespace merutilm::rff2 {
    // Saves/loads the full settings bundle (view location, Fractal calc/formula/rotation,
    // Render, Resolution, Shader, Video) to a single binary file.
    struct ConfigIO {
        ConfigIO() = delete;

        static constexpr uint32_t MAGIC = 0x52464643; // "RFFC"
        static constexpr uint32_t VERSION = 5;
        // Marks the projection block, behind a marker of its own for the same reason.
        static constexpr uint32_t PROJECTION_MAGIC = 0x50524A43; // "PRJC"

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
