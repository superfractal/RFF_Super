//
// Modified by GPT-6 on 2026-09-14, 2026-09-23
//

#pragma once
#include "../../preset/Presets.h"
#include <memory>
#include <vector>
#include <filesystem>

namespace merutilm::rff2::workspace {
    const std::vector<std::shared_ptr<const Presets::ShaderPresets::PalettePreset>> &paletteLibrary();
    ShdPaletteAttribute paletteFromLibrary(size_t index, uint32_t seed);
    bool readPaletteSource(const std::filesystem::path &path, const ShaderAttribute &current,
                           ShaderAttribute &source, std::wstring &error);
}
