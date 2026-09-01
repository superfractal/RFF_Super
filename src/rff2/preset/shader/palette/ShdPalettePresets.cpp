//
// Created by Merutilm on 2025-05-28.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-20, 2026-08-25, 2026-08-31
//

#include "ShdPalettePresets.h"

#include <algorithm>
#include <glm/glm.hpp>

#include "../../../data/ColorUtils.h"

namespace merutilm::rff2 {
#define PI static_cast<float>(M_PI)
    std::string ShdPalettePresets::LongRandom64::getName() const {
        return "LongRandom64 [Recommend]";
    }

    ShdPaletteAttribute ShdPalettePresets::LongRandom64::genPalette() const {
        auto p = ShdPaletteAttribute();

        if (!baseColors.empty()) {
            // Use provided base colors
            p.colors = baseColors;
        } else {
            // Generate random colors
            p.colors.reserve(64);
            for (int i = 0; i < 64; ++i) {
                p.colors.push_back(ColorUtils::random());
            }
        }

        p.iterationInterval = glm::vec4(1.0f);


        const ShdPaletteAttribute p1 = p;
        constexpr uint64_t r1 = 100;

        p.colors.clear();
        p.colors.reserve(p1.colors.size() * r1);

        for (uint64_t i = 0; i < p1.colors.size() * r1; i++) {
            const float irv = static_cast<float>(i) / static_cast<float>(r1);
            const glm::vec4 c2 = p1.getMidColor(irv / static_cast<float>(p1.colors.size()));
            const glm::vec4 cr = ColorUtils::random();

            p.colors.push_back(ColorUtils::forEachExceptAlpha(c2, cr, [](const float c, const float t) { return c + t / 6; }));
        }

        const ShdPaletteAttribute p2 = p;
        constexpr uint64_t r2 = 100;
        p.colors.clear();
        p.colors.reserve(p2.colors.size() * r2);

        for (uint64_t i = 0; i < p2.colors.size() * r2; i++) {
            const float irv = static_cast<float>(i) / static_cast<float>(r2);

            glm::vec4 c2 = p2.getMidColor(irv / static_cast<float>(p2.colors.size()));
            const float v = (c2.r + c2.g + c2.b) / 3;
            const float o = 0.5f + 0.5f * std::sin(std::fmod(irv, 1.0f) * 150.0f);

            p.colors.push_back(ColorUtils::forEachExceptAlpha(c2, [v, o](const float c) {
                return std::lerp(c, v / (1 + rff_math::random_f() * 2.0f), o);
            }));
        }


        p.iterationInterval = glm::vec4(1000000.0f);
        p.offsetRatio = 0;
        p.colorSmoothing = ShdPalColorSmoothingMethod::NORMAL;
        return p;
    }

    std::string ShdPalettePresets::LongRandom64_2::getName() const {
        return "LongRandom64 2 [DANGER]";
    }

    ShdPaletteAttribute ShdPalettePresets::LongRandom64_2::genPalette() const {
        auto p = ShdPaletteAttribute();

        if (!baseColors.empty()) {
            // Use provided base colors
            p.colors = baseColors;
        } else {
            // Generate random colors
            p.colors.reserve(64);
            for (int i = 0; i < 64; ++i) {
                p.colors.push_back(ColorUtils::random());
            }
        }

        p.iterationInterval = glm::vec4(1.0f);


        // Stage 1: expand x100 and blend in random color variation
        const ShdPaletteAttribute p1 = p;
        constexpr uint64_t r1 = 100;

        p.colors.clear();
        p.colors.reserve(p1.colors.size() * r1);

        for (uint64_t i = 0; i < p1.colors.size() * r1; i++) {
            const float irv = static_cast<float>(i) / static_cast<float>(r1);
            const glm::vec4 c2 = p1.getMidColor(irv / static_cast<float>(p1.colors.size()));
            const glm::vec4 cr = ColorUtils::random();

            p.colors.push_back(ColorUtils::forEachExceptAlpha(c2, cr, [](const float c, const float t) { return c + t / 6; }));
        }

        // Stage 2: expand x100 and apply sinusoidal value modulation (period 150)
        const ShdPaletteAttribute p2 = p;
        constexpr uint64_t r2 = 100;
        p.colors.clear();
        p.colors.reserve(p2.colors.size() * r2);

        for (uint64_t i = 0; i < p2.colors.size() * r2; i++) {
            const float irv = static_cast<float>(i) / static_cast<float>(r2);

            glm::vec4 c2 = p2.getMidColor(irv / static_cast<float>(p2.colors.size()));
            const float v = (c2.r + c2.g + c2.b) / 3;
            const float o = 0.5f + 0.5f * std::sin(std::fmod(irv, 1.0f) * 150.0f);

            p.colors.push_back(ColorUtils::forEachExceptAlpha(c2, [v, o](const float c) {
                return std::lerp(c, v / (1 + rff_math::random_f() * 2.0f), o);
            }));
        }

        // Stage 3: full x100 expansion with a finer, higher-frequency value modulation (period 311).
        const ShdPaletteAttribute p3 = p;
        constexpr uint64_t r3 = 100;
        p.colors.clear();
        p.colors.reserve(p3.colors.size() * r3);

        for (uint64_t i = 0; i < p3.colors.size() * r3; i++) {
            const float irv = static_cast<float>(i) / static_cast<float>(r3);

            glm::vec4 c2 = p3.getMidColor(irv / static_cast<float>(p3.colors.size()));
            const float v = (c2.r + c2.g + c2.b) / 3;
            const float o = 0.5f + 0.5f * std::sin(std::fmod(irv, 1.0f) * 311.0f);

            p.colors.push_back(ColorUtils::forEachExceptAlpha(c2, [v, o](const float c) {
                return std::lerp(c, v / (1 + rff_math::random_f() * 2.0f), o);
            }));
        }


        p.iterationInterval = glm::vec4(100000000.0f);
        p.offsetRatio = 0;
        p.colorSmoothing = ShdPalColorSmoothingMethod::NORMAL;
        return p;
    }

    std::string ShdPalettePresets::RandomSmooth::getName() const {
        return "RandomSmooth [Recommend]";
    }

    ShdPaletteAttribute ShdPalettePresets::RandomSmooth::genPalette() const {
        ShdPaletteAttribute p = {};
        p.colors.reserve(200);

        // Procedural Cosine Palette: a + b * cos( 2pi * (c*t + d) )
        // Using IQ's palette technique

        // Random components
        const float a_r = 0.5f;
        const float a_g = 0.5f;
        const float a_b = 0.5f;

        const float b_r = 0.5f;
        const float b_g = 0.5f;
        const float b_b = 0.5f;

        // Frequencies (c)
        // Integer frequencies guarantee that starting and ending colors match (seamless)
        const float c_r = std::floor(1.0f + rff_math::random_f() * 3.0f); // 1.0, 2.0, 3.0
        const float c_g = std::floor(1.0f + rff_math::random_f() * 3.0f);
        const float c_b = std::floor(1.0f + rff_math::random_f() * 3.0f);

        // Phases (d)
        // 0.0 ~ 1.0
        const float d_r = rff_math::random_f();
        const float d_g = rff_math::random_f();
        const float d_b = rff_math::random_f();

        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            const float t = static_cast<float>(cnt) / 200.0f;

            // Apply formula
            const float r = a_r + b_r * std::cos(2.0f * PI * (c_r * t + d_r));
            const float g = a_g + b_g * std::cos(2.0f * PI * (c_g * t + d_g));
            const float b = a_b + b_b * std::cos(2.0f * PI * (c_b * t + d_b));

            p.colors.emplace_back(std::clamp(r, 0.0f, 1.0f), std::clamp(g, 0.0f, 1.0f), std::clamp(b, 0.0f, 1.0f), 1.0f);
        }

        p.iterationInterval = glm::vec4(50 + rff_math::random_f() * 200); // Random interval 50-250
        p.offsetRatio = 0.0f;
        p.colorSmoothing = ShdPalColorSmoothingMethod::NORMAL;
        return p;
    }

    std::string ShdPalettePresets::Classic1::getName() const {
        return "Classic 1";
    }

    ShdPaletteAttribute ShdPalettePresets::Classic1::genPalette() const {
        ShdPaletteAttribute p = {};
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            const float i = PI * cnt / 100;
            const float r = 0.5f + 0.5f * std::sin(i - 2);
            const float g = 0.5f + 0.5f * std::sin(i - 1.3f);
            const float b = 0.5f + 0.5f * std::sin(i - 0.6f);
            p.colors.emplace_back(r, g, b, 1);
        }
        p.iterationInterval = glm::vec4(250.0f);
        return p;
    }

    std::string ShdPalettePresets::Classic2::getName() const {
        return "Classic 2";
    }

    ShdPaletteAttribute ShdPalettePresets::Classic2::genPalette() const {
        ShdPaletteAttribute p = {};
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            const float i = PI * cnt / 100;
            const float r = 0.5f + 0.5f * std::sin(i - 2);
            const float g = 0.5f + 0.5f * std::sin(i - 0.6f);
            const float b = 0.5f + 0.5f * std::sin(i - 1.3f);
            p.colors.emplace_back(r, g, b, 1);
        }
        p.iterationInterval = glm::vec4(250.0f);
        return p;
    }

    std::string ShdPalettePresets::ArcticAurora::getName() const {
        return "Arctic Aurora";
    }

    ShdPaletteAttribute ShdPalettePresets::ArcticAurora::genPalette() const {
        ShdPaletteAttribute p = {};
        p.colors.reserve(200);
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            const float i = PI * cnt / 100.0f;
            // Cool spectrum: Teal -> Deep Blue -> Purple -> Green
            const float r = 0.2f + 0.4f * std::sin(i - 1.0f);
            const float g = 0.5f + 0.4f * std::sin(i + 0.5f);
            const float b = 0.6f + 0.4f * std::sin(i - 0.5f);
            p.colors.emplace_back(std::clamp(r, 0.0f, 1.0f), std::clamp(g, 0.0f, 1.0f), std::clamp(b, 0.0f, 1.0f), 1);
        }
        p.iterationInterval = glm::vec4(300.0f);
        p.offsetRatio = 0.2f;
        p.colorSmoothing = ShdPalColorSmoothingMethod::NORMAL;
        return p;
    }

    std::string ShdPalettePresets::Azure::getName() const {
        return "Azure";
    }

    ShdPaletteAttribute ShdPalettePresets::Azure::genPalette() const {
        ShdPaletteAttribute p = {};
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            const float i = PI * cnt / 100;
            const float r = 0.5f + 0.5f * std::sin(1.5f * std::sin(i) - 0.5f);
            const float g = 0.5f + 0.5f * std::sin(1.5f * std::sin(i));
            const float b = 0.5f + 0.5f * std::sin(1.5f * std::sin(i) + 0.5f);
            p.colors.emplace_back(r, g, b, 1);
        }
        p.iterationInterval = glm::vec4(300.0f);
        p.offsetRatio = 0.7f;
        return p;
    }

    std::string ShdPalettePresets::Cinematic::getName() const {
        return "Cinematic";
    }

    ShdPaletteAttribute ShdPalettePresets::Cinematic::genPalette() const {
        ShdPaletteAttribute p = {};
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            const float i = PI * cnt / 100;
            float v = 0.5f + 0.5f * std::sin(i);
            glm::vec4 c{v, v, v, 1};
            c = ColorUtils::forEachExceptAlpha(c, glm::vec4{1.000000f, 0.647058f, 0.000000f, 1.000000f},
                                     [v](const float e, const float ta) { return e * (1 - v * 0.3f) + ta * v * 0.3f; });
            p.colors.push_back(c);
        }
        p.iterationInterval = glm::vec4(100.0f);
        p.offsetRatio = 0.7f;
        return p;
    }

    std::string ShdPalettePresets::CrimsonMagma::getName() const {
        return "Crimson Magma";
    }

    ShdPaletteAttribute ShdPalettePresets::CrimsonMagma::genPalette() const {
        ShdPaletteAttribute p = {};
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            const float i = PI * cnt / 100;
            const float r = 0.5f + 0.5f * std::sin(1.5f * std::sin(i) + 0.8f);
            const float g = 0.5f + 0.5f * std::sin(1.5f * std::sin(i) - 0.2f);
            const float b = 0.5f + 0.5f * std::sin(1.5f * std::sin(i) - 1.5f);
            p.colors.emplace_back(r, g, b, 1);
        }
        p.iterationInterval = glm::vec4(300.0f);
        p.offsetRatio = 0.7f;
        return p;
    }

    std::string ShdPalettePresets::DeepSpace::getName() const {
        return "Deep Space";
    }

    ShdPaletteAttribute ShdPalettePresets::DeepSpace::genPalette() const {
        ShdPaletteAttribute p = {};
        p.colors.reserve(200);
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            const float i = PI * cnt / 100.0f;
            // Deep blue/black background
            float bg_osc = 0.5f + 0.5f * std::sin(i);
            glm::vec4 bg = {0.0f, 0.0f, 0.1f * bg_osc + 0.05f, 1.0f};

            // Occasional stars/nebula brightness
            float star = 0.5f + 0.5f * std::sin(i * 5.0f);
            star = std::pow(std::abs(star), 20.0f); // Spikes

            glm::vec4 starColor = {0.8f, 0.9f, 1.0f, 1.0f};

            p.colors.push_back(ColorUtils::lerp(bg, starColor, star));
        }
        p.iterationInterval = glm::vec4(500.0f);
        p.offsetRatio = 0.0f;
        p.colorSmoothing = ShdPalColorSmoothingMethod::NORMAL;
        return p;
    }

    std::string ShdPalettePresets::Desert::getName() const {
        return "Desert";
    }

    ShdPaletteAttribute ShdPalettePresets::Desert::genPalette() const {
        ShdPaletteAttribute p = {};
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            const float i = PI * cnt / 100;
            glm::vec4 c = {0.75f, 0.5f, 0.25f, 1};
            p.colors.emplace_back(ColorUtils::lerp(c, glm::vec4{1, 1, 1, 1}, -0.3f + 0.3f * std::cos(i)));
        }
        p.iterationInterval = glm::vec4(250.0f);
        p.offsetRatio = 0.7f;
        return p;
    }

    std::string ShdPalettePresets::ElectricDreams::getName() const {
        return "Electric Dreams";
    }

    ShdPaletteAttribute ShdPalettePresets::ElectricDreams::genPalette() const {
        ShdPaletteAttribute p = {};
        p.colors.reserve(200);
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            const float i = PI * cnt / 100.0f;
            // High freq, neon: Cyan, Magenta, Yellow
            const float r = 0.5f + 0.5f * std::sin(3.0f * i);
            const float g = 0.5f + 0.5f * std::sin(3.0f * i + 2.0f * PI / 3.0f);
            const float b = 0.5f + 0.5f * std::sin(3.0f * i + 4.0f * PI / 3.0f);
            // Push to extremes for neon look
            float nr = std::pow(std::abs(r), 0.5f);
            float ng = std::pow(std::abs(g), 0.5f);
            float nb = std::pow(std::abs(b), 0.5f);
            p.colors.emplace_back(nr, ng, nb, 1);
        }
        p.iterationInterval = glm::vec4(100.0f);
        p.offsetRatio = 0.0f;
        p.colorSmoothing = ShdPalColorSmoothingMethod::NORMAL;
        return p;
    }

    std::string ShdPalettePresets::Flame::getName() const {
        return "Flame";
    }

    ShdPaletteAttribute ShdPalettePresets::Flame::genPalette() const {
        ShdPaletteAttribute p = {};
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            const float i = PI * cnt / 100;
            const float r = 0.5f + 0.5f * std::sin(1.5f * std::sin(i) + 0.5f);
            const float g = 0.5f + 0.5f * std::sin(1.5f * std::sin(i));
            const float b = 0.5f + 0.5f * std::sin(1.5f * std::sin(i) - 0.5f);
            p.colors.emplace_back(r, g, b, 1);
        }
        p.iterationInterval = glm::vec4(300.0f);
        p.offsetRatio = 0.7f;
        return p;
    }

    std::string ShdPalettePresets::GlossyBerry::getName() const { return "Glossy Berry"; }
    ShdPaletteAttribute ShdPalettePresets::GlossyBerry::genPalette() const {
        ShdPaletteAttribute p = {};
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            float t = static_cast<float>(cnt) / 200.0f;
            float r = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.3f));
            float g = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.2f));
            float b = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.4f));
            p.colors.emplace_back(r, g*0.3f, b, 1.0f);
        }
        p.iterationInterval = glm::vec4(250.0f);
        return p;
    }

    std::string ShdPalettePresets::GlossyCyber::getName() const { return "Glossy Cyber"; }
    ShdPaletteAttribute ShdPalettePresets::GlossyCyber::genPalette() const {
        ShdPaletteAttribute p = {};
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            float t = static_cast<float>(cnt) / 200.0f;
            float r = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.0f));
            float g = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.7f));
            float b = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.4f));
            p.colors.emplace_back(r*0.2f, g, b, 1.0f);
        }
        p.iterationInterval = glm::vec4(250.0f);
        return p;
    }

    std::string ShdPalettePresets::GlossyFire::getName() const { return "Glossy Fire"; }
    ShdPaletteAttribute ShdPalettePresets::GlossyFire::genPalette() const {
        ShdPaletteAttribute p = {};
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            float t = static_cast<float>(cnt) / 200.0f;
            float r = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.0f));
            float g = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.15f));
            float b = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.3f));
            p.colors.emplace_back(r, g*0.5f, b*0.1f, 1.0f);
        }
        p.iterationInterval = glm::vec4(200.0f);
        return p;
    }

    std::string ShdPalettePresets::GlossyForest::getName() const { return "Glossy Forest"; }
    ShdPaletteAttribute ShdPalettePresets::GlossyForest::genPalette() const {
        ShdPaletteAttribute p = {};
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            float t = static_cast<float>(cnt) / 200.0f;
            float r = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.04f));
            float g = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.37f));
            float b = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.69f));
            p.colors.emplace_back(std::clamp(r*0.4f,0.0f,1.0f), std::clamp(g+0.2f,0.0f,1.0f), std::clamp(b*0.4f,0.0f,1.0f), 1.0f);
        }
        p.iterationInterval = glm::vec4(250.0f);
        return p;
    }

    std::string ShdPalettePresets::GlossyIce::getName() const { return "Glossy Ice"; }
    ShdPaletteAttribute ShdPalettePresets::GlossyIce::genPalette() const {
        ShdPaletteAttribute p = {};
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            float t = static_cast<float>(cnt) / 200.0f;
            float r = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.5f));
            float g = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.2f));
            float b = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.1f));
            p.colors.emplace_back(r*0.4f, g*0.8f, b+0.2f, 1.0f);
        }
        p.iterationInterval = glm::vec4(250.0f);
        return p;
    }

    std::string ShdPalettePresets::GlossyMetal::getName() const { return "Glossy Metal"; }
    ShdPaletteAttribute ShdPalettePresets::GlossyMetal::genPalette() const {
        ShdPaletteAttribute p = {};
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            float t = static_cast<float>(cnt) / 200.0f;
            float v = 0.5f + 0.5f * std::cos(2.0f * PI * (2.0f * t));
            // Slight metallic tint (bluish grey)
            p.colors.emplace_back(v, v*1.05f, v*1.1f, 1.0f);
        }
        p.iterationInterval = glm::vec4(200.0f);
        return p;
    }

    std::string ShdPalettePresets::GlossyNeon::getName() const { return "Glossy Neon"; }
    ShdPaletteAttribute ShdPalettePresets::GlossyNeon::genPalette() const {
        ShdPaletteAttribute p = {};
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            float t = static_cast<float>(cnt) / 200.0f;
            // High saturation/variation
            float r = 0.5f + 0.5f * std::cos(2.0f * PI * (2.0f * t + 0.02f));
            float g = 0.5f + 0.5f * std::cos(2.0f * PI * (2.0f * t + 0.36f));
            float b = 0.5f + 0.5f * std::cos(2.0f * PI * (2.0f * t + 0.71f));
            p.colors.emplace_back(r, g, b, 1.0f);
        }
        p.iterationInterval = glm::vec4(150.0f);
        return p;
    }

    std::string ShdPalettePresets::GlossyOcean::getName() const { return "Glossy Ocean"; }
    ShdPaletteAttribute ShdPalettePresets::GlossyOcean::genPalette() const {
        ShdPaletteAttribute p = {};
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            float t = static_cast<float>(cnt) / 200.0f;
            float r = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.5f));
            float g = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.55f));
            float b = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.6f));
            p.colors.emplace_back(std::clamp(r*0.4f,0.0f,1.0f), std::clamp(g*0.8f,0.0f,1.0f), std::clamp(b+0.2f,0.0f,1.0f), 1.0f);
        }
        p.iterationInterval = glm::vec4(250.0f);
        return p;
    }

    std::string ShdPalettePresets::GlossyPastel::getName() const { return "Glossy Pastel"; }
    ShdPaletteAttribute ShdPalettePresets::GlossyPastel::genPalette() const {
        ShdPaletteAttribute p = {};
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            float t = static_cast<float>(cnt) / 200.0f;
            float r = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.07f));
            float g = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.41f));
            float b = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.74f));
            // Wash out for pastel look
            p.colors.emplace_back(r*0.5f + 0.5f, g*0.5f + 0.5f, b*0.5f + 0.5f, 1.0f);
        }
        p.iterationInterval = glm::vec4(250.0f);
        return p;
    }

    std::string ShdPalettePresets::GlossySunset::getName() const { return "Glossy Sunset"; }
    ShdPaletteAttribute ShdPalettePresets::GlossySunset::genPalette() const {
        ShdPaletteAttribute p = {};
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            float t = static_cast<float>(cnt) / 200.0f;
            // The following line records the former reference tuple; the code uses custom phase values.
            // a=0.5,0.5,0.5 b=0.5,0.5,0.5 c=1,1,1 d=0.3,0.20,0.20
            float r = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.31f));
            float g = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.18f));
            float b = 0.5f + 0.5f * std::cos(2.0f * PI * (1.0f * t + 0.24f));
            p.colors.emplace_back(std::clamp(r+0.2f,0.0f,1.0f), std::clamp(g*0.8f,0.0f,1.0f), std::clamp(b*0.5f,0.0f,1.0f), 1.0f);
        }
        p.iterationInterval = glm::vec4(250.0f);
        return p;
    }

    std::string ShdPalettePresets::LongRainbow7::getName() const {
        return "Long Rainbow 7";
    }

    ShdPaletteAttribute ShdPalettePresets::LongRainbow7::genPalette() const {
        auto p = ShdPaletteAttribute();
        p.colors.reserve(7);
        p.colors.push_back(glm::vec4{0.909803f, 0.078431f, 0.086274f, 1.000000f});
        p.colors.push_back(glm::vec4{1.000000f, 0.647058f, 0.000000f, 1.000000f});
        p.colors.push_back(glm::vec4{0.980392f, 0.921568f, 0.211764f, 1.000000f});
        p.colors.push_back(glm::vec4{0.474509f, 0.764705f, 0.078431f, 1.000000f});
        p.colors.push_back(glm::vec4{0.282352f, 0.490196f, 0.905882f, 1.000000f});
        p.colors.push_back(glm::vec4{0.294117f, 0.211764f, 0.615686f, 1.000000f});
        p.colors.push_back(glm::vec4{0.439215f, 0.211764f, 0.615686f, 1.000000f});

        p.iterationInterval = glm::vec4(1.0f);


        const ShdPaletteAttribute p1 = p;
        constexpr uint64_t r1 = 100;

        p.colors.clear();
        p.colors.reserve(p1.colors.size() * r1);

        for (uint64_t i = 0; i < p1.colors.size() * r1; i++) {
            const float irv = static_cast<float>(i) / static_cast<float>(r1);
            const glm::vec4 c2 = p1.getMidColor(irv / static_cast<float>(p1.colors.size()));
            const glm::vec4 cr = ColorUtils::random();

            p.colors.push_back(ColorUtils::forEachExceptAlpha(c2, cr, [](const float c, const float t) { return c + t / 6; }));
        }

        const ShdPaletteAttribute p2 = p;
        constexpr uint64_t r2 = 100;
        p.colors.clear();
        p.colors.reserve(p2.colors.size() * r2);

        for (uint64_t i = 0; i < p2.colors.size() * r2; i++) {
            const float irv = static_cast<float>(i) / static_cast<float>(r2);

            glm::vec4 c2 = p2.getMidColor(irv / static_cast<float>(p2.colors.size()));
            const float v = (c2.r + c2.g + c2.b) / 3;
            const float o = 0.5f + 0.5f * std::sin(std::fmod(irv, 1.0f) * 150.0f);

            p.colors.push_back(ColorUtils::forEachExceptAlpha(c2, [v, o](const float c) {
                return std::lerp(c, v / (1 + rff_math::random_f() * 2.0f), o);
            }));
        }


        p.iterationInterval = glm::vec4(2000000.0f);
        p.offsetRatio = 0.55f;
        p.colorSmoothing = ShdPalColorSmoothingMethod::NORMAL;
        return p;
    }

    std::string ShdPalettePresets::MidnightNeon::getName() const {
        return "Midnight Neon";
    }

    ShdPaletteAttribute ShdPalettePresets::MidnightNeon::genPalette() const {
        ShdPaletteAttribute p = {};
        p.colors.reserve(200);
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            const float i = PI * cnt / 100.0f;
            // High contrast vibrant colors: Dark blue -> Deep purple -> Hot pink/Neon Cyan
            const float r = 0.5f + 0.5f * std::sin(i - 1.5f);
            const float g = 0.2f + 0.2f * std::sin(i * 2.0f);
            const float b = 0.7f + 0.3f * std::cos(i);
            p.colors.emplace_back(r, g, b, 1);
        }
        p.iterationInterval = glm::vec4(150.0f);
        p.offsetRatio = 0.0f;
        p.colorSmoothing = ShdPalColorSmoothingMethod::NORMAL;
        return p;
    }

    std::string ShdPalettePresets::MistyForest::getName() const {
        return "Misty Forest";
    }

    ShdPaletteAttribute ShdPalettePresets::MistyForest::genPalette() const {
        ShdPaletteAttribute p = {};
        p.colors.reserve(200);
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            const float i = PI * cnt / 100.0f;
            // Nature focused: Dark forest green -> Teal -> Misty white/Pale green
            const float r = 0.3f + 0.3f * std::sin(i - 0.5f);
            const float g = 0.5f + 0.4f * std::sin(i);
            const float b = 0.4f + 0.3f * std::sin(i + 0.5f);
            p.colors.emplace_back(r, g, b, 1);
        }
        p.iterationInterval = glm::vec4(250.0f);
        p.offsetRatio = 0.3f;
        p.colorSmoothing = ShdPalColorSmoothingMethod::NORMAL;
        return p;
    }

    std::string ShdPalettePresets::PastelDream::getName() const {
        return "Pastel Dream";
    }

    ShdPaletteAttribute ShdPalettePresets::PastelDream::genPalette() const {
        ShdPaletteAttribute p = {};
        p.colors.reserve(200);
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            const float i = PI * cnt / 100.0f;
            // High brightness, lower saturation variation
            const float r = 0.8f + 0.2f * std::sin(i);
            const float g = 0.8f + 0.2f * std::sin(i + 2.0f);
            const float b = 0.8f + 0.2f * std::sin(i + 4.0f);
            p.colors.emplace_back(r, g, b, 1);
        }
        p.iterationInterval = glm::vec4(400.0f);
        p.offsetRatio = 0.0f;
        p.colorSmoothing = ShdPalColorSmoothingMethod::NORMAL;
        return p;
    }

    std::string ShdPalettePresets::Rainbow::getName() const {
        return "Rainbow";
    }

    ShdPaletteAttribute ShdPalettePresets::Rainbow::genPalette() const {
        auto p = ShdPaletteAttribute();
        p.colors.reserve(7);

        p.colors.push_back(glm::vec4{0.909803f, 0.078431f, 0.086274f, 1.000000f});
        p.colors.push_back(glm::vec4{1.000000f, 0.647058f, 0.000000f, 1.000000f});
        p.colors.push_back(glm::vec4{0.980392f, 0.921568f, 0.211764f, 1.000000f});
        p.colors.push_back(glm::vec4{0.474509f, 0.764705f, 0.078431f, 1.000000f});
        p.colors.push_back(glm::vec4{0.282352f, 0.490196f, 0.905882f, 1.000000f});
        p.colors.push_back(glm::vec4{0.294117f, 0.211764f, 0.615686f, 1.000000f});
        p.colors.push_back(glm::vec4{0.439215f, 0.211764f, 0.615686f, 1.000000f});
        p.iterationInterval = glm::vec4(300.0f);
        p.offsetRatio = 0;
        p.colorSmoothing = ShdPalColorSmoothingMethod::NORMAL;
        return p;
    }

    std::string ShdPalettePresets::VolcanicAsh::getName() const {
        return "Volcanic Ash";
    }

    ShdPaletteAttribute ShdPalettePresets::VolcanicAsh::genPalette() const {
        ShdPaletteAttribute p = {};
        p.colors.reserve(200);
        for (uint8_t cnt = 0; cnt < 200; ++cnt) {
            const float i = PI * cnt / 100.0f;
            float v = 0.5f + 0.5f * std::sin(i);
            // Black/Grey base
            glm::vec4 dark = {0.1f, 0.1f, 0.12f, 1.0f};
            // Bright Orange/Red lava
            glm::vec4 bright = {1.0f, 0.3f, 0.0f, 1.0f};

            // Sharp transition for 'cracks'
            float t = std::pow(v, 4.0f);

            p.colors.push_back(ColorUtils::lerp(dark, bright, t));
        }
        p.iterationInterval = glm::vec4(150.0f);
        p.offsetRatio = 0.5f;
        p.colorSmoothing = ShdPalColorSmoothingMethod::NORMAL;
        return p;
    }

    std::string ShdPalettePresets::FromColors::getName() const {
        return "Custom Colors";
    }

    ShdPaletteAttribute ShdPalettePresets::FromColors::genPalette() const {
        ShdPaletteAttribute p = {};
        p.colors = colors;
        p.iterationInterval = glm::vec4(64.0f);
        p.offsetRatio = 0;
        p.colorSmoothing = ShdPalColorSmoothingMethod::NORMAL;
        return p;
    }

    // Recipe 4 is the retired KFR Banded palette: it is gone from the menu, and its colors are still drawn here only so a file written while it existed opens with the palette it was saved with.
    namespace {
        std::vector<glm::vec4> retiredKFRBandedColors() {
            constexpr uint32_t BAND_COUNT = 16;
            std::vector<glm::vec4> colors;
            colors.reserve(BAND_COUNT);
            for (uint32_t i = 0; i < BAND_COUNT; ++i) {
                const glm::vec4 c = ColorUtils::random();
                const float peak = std::max({c.r, c.g, c.b});
                // A dark draw is lifted to full brightness so every band reads against the lines.
                const float gain = peak > 0.0f ? 1.0f / peak : 1.0f;
                colors.push_back(ColorUtils::forEachExceptAlpha(c, [gain](const float v) { return v * gain; }));
            }
            return colors;
        }
    }

    std::vector<glm::vec4> ShdPalettePresets::regenerateRecipeColors(const int32_t recipeId, const uint32_t seed) {
        // The recipe runs on the shared generator, which is what makes it come out the colors it was
        // saved as. The generator is handed back afterwards so opening a file does not also settle
        // what the next Random palette turns out to be.
        const auto borrowed = rff_math::captureState();
        rff_math::reseed(seed);
        std::vector<glm::vec4> colors;
        switch (recipeId) {
            case 1: colors = LongRandom64().genPalette().colors; break;
            case 2: colors = LongRandom64_2().genPalette().colors; break;
            case 3: colors = LongRainbow7().genPalette().colors; break;
            case 4: colors = retiredKFRBandedColors(); break;
            default: break;
        }
        rff_math::restoreState(borrowed);
        return colors;
    }
}
