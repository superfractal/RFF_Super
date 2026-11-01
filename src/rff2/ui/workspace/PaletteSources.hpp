//
// Modified by GPT-6 on 2026-09-14, 2026-09-23
// Modified by Opus 5.5 on 2026-10-25
//

#pragma once
#include "../../preset/Presets.h"
#include <memory>
#include <vector>
#include <filesystem>

namespace merutilm::rff2::workspace {
    const std::vector<std::shared_ptr<const Presets::ShaderPresets::PalettePreset>> &paletteLibrary();
    ShdPaletteAttribute paletteFromLibrary(size_t index, uint32_t seed);
    // Reads the Colors line of a KFR file into a copy of current whose palette is those colors alone.
    bool readKfrPalette(const std::filesystem::path &path, const ShaderAttribute &current,
                        ShaderAttribute &source, std::wstring &error);
    // Reads the shader of an RFC or RFSP into a copy of current, for its palette and color correction.
    bool readSettingsPalette(const std::filesystem::path &path, const ShaderAttribute &current,
                             ShaderAttribute &source, std::wstring &error);
}
