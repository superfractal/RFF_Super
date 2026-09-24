//
// Modified by GPT-6 on 2026-09-14, 2026-09-18, 2026-09-19, 2026-09-22, 2026-09-24
//

#pragma once
#include "SurfaceInspectorItems.hpp"
#include <array>
#include <set>

namespace merutilm::rff2::workspace {
    class SurfaceInspectorContent {
      public:
        struct Group {
            Category category;
            std::wstring_view label;
            std::vector<std::string_view> controls;
        };
        struct Row {
            long id = 0;
            const SurfaceParameter *parameter = nullptr;
            const SurfaceColor *color = nullptr;
            int group = -1, top = 0;
            bool header() const {
                return !parameter && !color;
            }
            int height() const {
                return header() ? 44 : 76;
            }
            std::wstring_view label() const {
                if (parameter) {
                    return displayLabel(*parameter);
                }
                if (color) {
                    return color->label;
                }
                return groups()[group].label;
            }
            bool changed(const ShdSlopeAttribute &value) const {
                const auto defaults = surfaceDefaults();
                if (parameter) {
                    return parameter->value(value) != parameter->value(defaults);
                }
                if (color) {
                    return value.*color->member != defaults.*color->member;
                }
                return false;
            }
        };
        static constexpr long groupIds = 10000;
        static const std::vector<Group> &groups() {
            static const std::vector<Group> entries = {
                {Category::COLOR,
                 L"Palette & surface",
                 {"surface.paletteColorMix", "surface.chromeStrength", "surface.surfacePhase",
                  "surface.styleColorMix", "surface.styleColor"}},
                {Category::COLOR,
                 L"Reflections & rim",
                 {"surface.styleHighlightMix", "surface.styleHighlightColor", "surface.styleRimColor"}},
                {Category::COLOR,
                 L"Background",
                 {"surface.styleBackgroundMix", "surface.styleBackgroundColor"}},
                {Category::REFLECTION,
                 L"Material",
                 {"surface.studio.roughness", "surface.studio.metalness", "surface.studio.ior"}},
                {Category::REFLECTION,
                 L"Studio lighting",
                 {"surface.studio.directIntensity", "surface.studio.environmentIntensity",
                  "surface.studioEnvironmentRotation", "surface.studioEnvironmentFollow"}},
                {Category::REFLECTION,
                 L"Coating",
                 {"surface.studio.clearcoat", "surface.studio.clearcoatRoughness"}},
                {Category::REFLECTION,
                 L"Reflection shaping",
                 {"surface.reflectionDetail", "surface.reflectionContrast", "surface.reflectionBrightness",
                  "surface.reflectionCurve", "surface.specularAA"}},
                {Category::FILM,
                 L"Thin film",
                 {"surface.filmStrength", "surface.iridescence", "surface.filmThickness", "surface.filmHue"}},
                {Category::FILM, L"Prism", {"surface.prismWidth", "surface.prismSpread"}},
                {Category::FILM, L"Glints", {"surface.sparkleStrength", "surface.styleGlintSize"}},
                {Category::CONTOUR,
                 L"Sigil ink",
                 {"surface.styleInkPreserve", "surface.backgroundBrightness"}},
                {Category::EMISSION,
                 L"Glow & color",
                 {"surface.seaGlow", "surface.seaThreshold", "surface.seaBalance", "surface.seaGlowColor",
                  "surface.seaAccentColor"}},
                {Category::EMISSION,
                 L"Dense detail",
                 {"surface.styleDetailLight", "surface.styleDetailSuppress", "surface.styleDetailThreshold"}},
                {Category::EMISSION,
                 L"Body & rim",
                 {"surface.seaBody", "surface.seaBodyColor", "surface.styleRimStrength",
                  "surface.styleRimWidth"}},
                {Category::EMISSION, L"Particles", {"surface.seaParticles", "surface.seaParticleSize"}},
                {Category::FLAME,
                 L"Chrome flames",
                 {"surface.flameStrength", "surface.phonkRed", "surface.shadowCrush", "surface.styleDamage"}},
                {Category::FLAME, L"Frost", {"surface.frostStrength", "surface.frostThreshold"}},
                {Category::PRINT,
                 L"Print & outlines",
                 {"surface.layerQuantize", "surface.ukiyoColors", "surface.ukiyoFlatness",
                  "surface.ukiyoBalance"}},
                {Category::PRINT, L"Waves & grain", {"surface.ukiyoFoam", "surface.ukiyoGrain"}},
                {Category::PRINT,
                 L"Print colors",
                 {"surface.printInk", "surface.printIndigo", "surface.printAsagi", "surface.printBlue",
                  "surface.printFoam", "surface.printPaper"}},
                {Category::NOISE,
                 L"VHS & color",
                 {"surface.layerVhs", "surface.vhsNoise", "surface.chromaticShift"}},
                {Category::NOISE, L"Pixelation", {"surface.pixelMix", "surface.pixelSize"}},
                {Category::NOISE,
                 L"Grain & monochrome",
                 {"surface.layerMono", "surface.filmGrain", "surface.grungeScale",
                  "surface.styleMonochrome"}},
                {Category::RELIEF,
                 L"Relief",
                 {"slope.depth", "slope.opacity", "surface.reliefDepth", "surface.normalSmooth",
                  "surface.aoRadius", "surface.boundaryGuard"}},
                {Category::RELIEF, L"Lighting", {"slope.reflectionRatio", "slope.zenith", "slope.azimuth"}},
                {Category::RELIEF, L"Waves", {"surface.reliefWaves", "surface.waveFrequency"}},
                {Category::MIX,
                 L"Surface layers",
                 {"surface.layerMetal", "surface.layerSigil", "surface.layerPhonk", "surface.layerFrost",
                  "surface.layerSea", "surface.layerPrint"}},
            };
            return entries;
        }
        static const std::vector<std::string_view> &basic(Category category) {
            static const std::array<std::vector<std::string_view>, 10> entries = {
                {{"surface.paletteColorMix", "surface.chromeStrength", "surface.styleColorMix",
                  "surface.styleColor", "surface.styleHighlightMix", "surface.styleHighlightColor"},
                 {"surface.studio.roughness", "surface.studio.metalness",
                  "surface.studio.environmentIntensity", "surface.reflectionContrast",
                  "surface.reflectionBrightness", "surface.studio.clearcoat"},
                 {"surface.filmStrength", "surface.prismWidth", "surface.prismSpread", "surface.filmHue",
                  "surface.sparkleStrength", "surface.styleGlintSize"},
                 {"surface.backgroundBrightness"},
                 {"surface.seaGlow", "surface.styleDetailLight", "surface.styleDetailSuppress",
                  "surface.styleDetailThreshold", "surface.seaGlowColor", "surface.seaAccentColor"},
                 {"surface.flameStrength", "surface.phonkRed", "surface.shadowCrush", "surface.styleDamage",
                  "surface.frostStrength", "surface.frostThreshold"},
                 {"surface.layerQuantize", "surface.ukiyoColors", "surface.ukiyoFlatness",
                  "surface.ukiyoFoam", "surface.printIndigo", "surface.printPaper"},
                 {"surface.layerVhs", "surface.vhsNoise", "surface.chromaticShift", "surface.layerMono",
                  "surface.filmGrain", "surface.styleMonochrome"},
                 {"slope.depth", "slope.opacity", "slope.zenith", "slope.azimuth", "surface.reliefDepth",
                  "surface.normalSmooth"},
                 {"surface.layerMetal", "surface.layerSigil", "surface.layerPhonk", "surface.layerFrost",
                  "surface.layerSea", "surface.layerPrint"}}};
            return entries[static_cast<int>(category)];
        }
        static Row control(std::string_view id) {
            if (paletteLineControl(id)) {
                return {};
            }
            for (const auto &parameter : surfaceParameters()) {
                if (parameter.id == id) {
                    return {SurfaceInspectorItems::id(parameter), &parameter};
                }
            }
            for (const auto &color : surfaceColors()) {
                if (color.id == id) {
                    return {SurfaceInspectorItems::id(color), nullptr, &color};
                }
            }
            return {};
        }
        static int groupFor(long id) {
            for (int i = 0; i < int(groups().size()); ++i) {
                for (const auto key : groups()[i].controls) {
                    if (control(key).id == id) {
                        return i;
                    }
                }
            }
            return -1;
        }
        static int changedInGroup(int group, const ShdSlopeAttribute &value) {
            int result = 0;
            for (const auto key : groups()[group].controls) {
                if (control(key).changed(value)) {
                    ++result;
                }
            }
            return result;
        }
        static int advancedChanged(Category category, const ShdSlopeAttribute &value) {
            int result = 0;
            const auto &common = basic(category);
            for (const auto &group : groups()) {
                if (group.category == category) {
                    for (const auto key : group.controls) {
                        if (std::find(common.begin(), common.end(), key) == common.end() &&
                            control(key).changed(value)) {
                            ++result;
                        }
                    }
                }
            }
            return result;
        }

      private:
        std::set<int> collapsed;
        std::vector<Row> rows;
        int contentHeight = 0;

      public:
        SurfaceInspectorContent() {
            for (int i = 0; i < int(groups().size()); ++i) {
                if (i && groups()[i - 1].category == groups()[i].category) {
                    collapsed.insert(i);
                }
            }
        }
        bool expanded(int group) const {
            return !collapsed.contains(group);
        }
        void setExpanded(int group, bool open) {
            if (open) {
                collapsed.erase(group);
            } else {
                collapsed.insert(group);
            }
        }
        void reveal(long id) {
            if (const int group = groupFor(id); group >= 0) {
                setExpanded(group, true);
            }
        }
        void rebuild(Category category, bool detail) {
            rows.clear();
            contentHeight = 0;
            const auto append = [&](Row row) {
                if (!row.id) {
                    return;
                }
                row.top = contentHeight;
                contentHeight += row.height();
                rows.push_back(row);
            };
            if (!detail) {
                for (const auto key : basic(category)) {
                    append(control(key));
                }
                return;
            }
            std::set<long> known;
            for (int i = 0; i < int(groups().size()); ++i) {
                if (groups()[i].category == category) {
                    append({groupIds + i, nullptr, nullptr, i});
                    for (const auto key : groups()[i].controls) {
                        auto row = control(key);
                        known.insert(row.id);
                        if (expanded(i)) {
                            row.group = i;
                            append(row);
                        }
                    }
                }
            }
            // Keep newly registered controls reachable until their presentation group is assigned.
            for (const auto &parameter : surfaceParameters()) {
                if (parameter.category() == category &&
                    !known.contains(SurfaceInspectorItems::id(parameter))) {
                    append(control(parameter.id));
                }
            }
            for (const auto &color : surfaceColors()) {
                if (color.category == category && !known.contains(SurfaceInspectorItems::id(color))) {
                    append(control(color.id));
                }
            }
        }
        const std::vector<Row> &items() const {
            return rows;
        }
        int count() const {
            return int(rows.size());
        }
        int height() const {
            return contentHeight;
        }
        int find(long id) const {
            for (int i = 0; i < count(); ++i) {
                if (rows[i].id == id) {
                    return i;
                }
            }
            return -1;
        }
        const Row *at(int index) const {
            if (index >= 0 && index < count()) {
                return &rows[index];
            }
            return nullptr;
        }
        int offset(int index) const {
            if (index >= count()) {
                return contentHeight;
            }
            if (index <= 0) {
                return 0;
            }
            return rows[index].top;
        }
        int indexAt(int position) const {
            for (int i = 0; i < count(); ++i) {
                if (position < rows[i].top + rows[i].height()) {
                    return i;
                }
            }
            return std::max(0, count() - 1);
        }
    };
}
