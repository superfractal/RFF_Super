//
// Modified by GPT-6 on 2026-09-14, 2026-09-21, 2026-09-23
//

#include "PaletteSources.hpp"
#include "../../preset/shader/palette/ShdPalettePresets.h"
#include "../../calc/rff_math.h"
#include "../../io/ConfigIO.h"
#include "../../io/ShaderPresetIO.h"
#include "../../io/KFRColorLoader.hpp"
#include <fstream>
#include <algorithm>

namespace merutilm::rff2::workspace {
    const std::vector<std::shared_ptr<const Presets::ShaderPresets::PalettePreset>> &paletteLibrary() {
        using namespace ShdPalettePresets;
        static const std::vector<std::shared_ptr<const Presets::ShaderPresets::PalettePreset>> library{
            std::make_shared<LongRandom64>(),
            std::make_shared<RandomSmooth>(),
            std::make_shared<Classic1>(),
            std::make_shared<Classic2>(),
            std::make_shared<ArcticAurora>(),
            std::make_shared<Azure>(),
            std::make_shared<Cinematic>(),
            std::make_shared<CrimsonMagma>(),
            std::make_shared<DeepSpace>(),
            std::make_shared<Desert>(),
            std::make_shared<ElectricDreams>(),
            std::make_shared<Flame>(),
            std::make_shared<GlossyBerry>(),
            std::make_shared<GlossyCyber>(),
            std::make_shared<GlossyFire>(),
            std::make_shared<GlossyForest>(),
            std::make_shared<GlossyIce>(),
            std::make_shared<GlossyMetal>(),
            std::make_shared<GlossyNeon>(),
            std::make_shared<GlossyOcean>(),
            std::make_shared<GlossyPastel>(),
            std::make_shared<GlossySunset>(),
            std::make_shared<LongRainbow7>(),
            std::make_shared<MidnightNeon>(),
            std::make_shared<MistyForest>(),
            std::make_shared<PastelDream>(),
            std::make_shared<Rainbow>(),
            std::make_shared<VolcanicAsh>(),
            std::make_shared<LongRandom64_2>()
        };
        return library;
    }

    ShdPaletteAttribute paletteFromLibrary(size_t index, uint32_t seed) {
        const auto &preset = *paletteLibrary().at(index);
        const auto state = rff_math::captureState();
        struct RandomStateRestorer {
            decltype(state) value;
            ~RandomStateRestorer() {
                rff_math::restoreState(value);
            }
        } restoreRandomState{state};
        rff_math::reseed(seed);
        auto palette = preset.genPalette();
        palette.recipePresetId = preset.getPaletteRecipeId();
        palette.recipeSeed = seed;
        return palette;
    }

    bool readPaletteSource(const std::filesystem::path &path, const ShaderAttribute &current,
                           ShaderAttribute &source, std::wstring &error) {
        try {
            auto extension = path.extension().wstring();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                           [](wchar_t c) { return std::towlower(c); });
            ShaderAttribute loadedShader = current;
            if (extension == L".rfc" || extension == L".rfsp") {
                bool loaded = false;
                if (extension == L".rfc") {
                    loaded = ConfigIO::loadShader(path, loadedShader);
                } else {
                    loaded = ShaderPresetIO::load(path, loadedShader);
                }
                if (!loaded) {
                    error = L"Unable to read the color settings file.";
                    return false;
                }
            } else if (extension == L".kfr" || extension == L".kfp") {
                std::wifstream input(path);
                if (!input) {
                    error = L"Unable to open the palette file.";
                    return false;
                }
                std::wstring line;
                std::vector<glm::vec4> colors;
                while (std::getline(input, line)) {
                    if (line.starts_with(L"Colors: ")) {
                        colors = KFRColorLoader::parseColorString(line);
                        break;
                    }
                }
                if (colors.empty()) {
                    error = L"No Colors entry was found in this palette file.";
                    return false;
                }
                for (const auto &color : colors) {
                    for (int c = 0; c < 4; ++c) {
                        if (!finitePaletteStopValue(color[c]) || color[c] < 0 || color[c] > 1) {
                            error = L"Palette colors must contain values from 0 to 255.";
                            return false;
                        }
                    }
                }
                loadedShader.palette.colors = std::move(colors);
                loadedShader.palette.recipePresetId = -1;
                loadedShader.palette.recipeSeed = 0;
                loadedShader.palette.stops.clear();
            } else {
                error = L"Choose an RFC, RFSP, KFR or KFP file.";
                return false;
            }
            source = std::move(loadedShader);
            error.clear();
            return true;
        } catch (const std::exception &) {
            error = L"The color file could not be read. Current settings are unchanged.";
            return false;
        }
    }
} // namespace merutilm::rff2::workspace
