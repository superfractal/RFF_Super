//
// Created by Opus 5 on 2026-08-31.
// Modified by Opus 5 on 2026-09-01
//

#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "../../Presets.h"

namespace merutilm::rff2::ShdExamplePresets {
    // The shader of one file in the example folder, applied whole. No name and no setting of it is
    // written down here: the menu is built from whatever the folder holds, so a look is added by
    // dropping a settings or shader preset file in and naming the file what the menu should say.
    struct FromFile final : public Presets::ShaderPresets::FullShaderPreset {
        std::filesystem::path path;
        std::string name;
        // Whether the file carries a whole config rather than a shader preset. Decided by the magic
        // word the file itself begins with, never by its extension.
        bool wholeConfig = false;

        FromFile(std::filesystem::path path, std::string name, bool wholeConfig);

        [[nodiscard]] std::string getName() const override;

        // The shader the file holds. A file that has become unreadable since the folder was read
        // yields the default shader, and says so in the log.
        [[nodiscard]] ShaderAttribute genShader() const override;
    };

    // The folder the examples are read from, beside the shaders the program loads the same way. It
    // is located from the running program rather than from the folder it happens to be started in,
    // since the shaders beside it are found that way too and a launcher is free to start anywhere.
    constexpr auto EXAMPLE_FOLDER_NAME = L"shader_example";

    // Every file in the example folder whose own header says it carries a shader, by name. The
    // files are only identified here; each one is read for its settings when it is picked.
    std::vector<FromFile> collect();
}
