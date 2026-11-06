// Modified by GPT-6 on 2026-09-21, 2026-09-23
// Modified by Opus 5.5 on 2026-10-08, 2026-10-09, 2026-10-20, 2026-10-22, 2026-10-23, 2026-11-04
#define AI_FIELD(member, label, minimum, maximum)                                                            \
    appendShaderField(                                                                                       \
        registeredFields, registeredAddresses, referenceShader, #member, label,                              \
        [](auto &shader) -> auto & {                                                                         \
            return shader.member;                                                                            \
        },                                                                                                   \
        minimum, maximum)
#define AI_ENUM(member, label, minimum, maximum, ...)                                                        \
    appendShaderField(                                                                                       \
        registeredFields, registeredAddresses, referenceShader, #member, label,                              \
        [](auto &shader) -> auto & {                                                                         \
            return shader.member;                                                                            \
        },                                                                                                   \
        minimum, maximum, Json::parse(__VA_ARGS__))
AI_ENUM(palette.colorSmoothing, "Palette / Smoothing", 0, 2, R"({"0":"None","1":"Normal","2":"Reversed"})");
AI_ENUM(palette.colorInterpolation, "Palette / Interpolation", 0, 2,
        R"({"0":"RGB","1":"OKLab","2":"Linear RGB"})");
AI_ENUM(palette.animationMode, "Palette / Animation mode", 0, 4,
        R"({"0":"Linear","1":"Psychedelic","3":"Breathing","4":"Turbulence"})");
AI_FIELD(palette.enableGloss, "Palette / Gloss enabled", 0, 1);
AI_FIELD(palette.glossColor, "Palette / Gloss color", 0, 1);
AI_FIELD(palette.seamless, "Palette / Seamless", 0, 1);
AI_ENUM(palette.stopEasing, "Palette / Stop easing", 0, 2,
        R"({"0":"Linear","1":"Smoothstep","2":"Smootherstep"})");
AI_FIELD(palette.bandLineEnabled, "Band Line / Enabled", 0, 1);
AI_FIELD(palette.bandLineCount, "Band Line / Count", 1, 256);
AI_FIELD(palette.bandLineWidth, "Band Line / Width", 0, 1);
AI_FIELD(palette.bandLineOpacity, "Band Line / Opacity", 0, 1);
AI_FIELD(palette.bandLineSoftness, "Band Line / Softness", 0, 1);
AI_FIELD(palette.bandLineColor, "Band Line / Color", 0, 1);
AI_FIELD(palette.bandSpineAmount, "Band Line / Spine amount", 0, 1);
AI_FIELD(palette.bandSpineLength, "Band Line / Spine length", 0, 3);
AI_FIELD(palette.bandSpineDensity, "Band Line / Spine density", .25, 3);
AI_FIELD(palette.bandBranchAmount, "Band Line / Branch amount", 0, 1);
AI_FIELD(palette.bandOrnamentAmount, "Band Line / Ornament amount", 0, 1);
AI_FIELD(palette.bandOrnamentSize, "Band Line / Ornament size", .25, 3);
AI_FIELD(palette.bandOrnamentDensity, "Band Line / Ornament density", .25, 2);
AI_FIELD(palette.bandOrnamentInset, "Band Line / Ornament inset", 0, .25);
AI_FIELD(palette.bandLineGroove, "Band Line / Groove enabled", 0, 1);
AI_FIELD(palette.grooveDepth, "Band Line / Groove depth", 0, 20);
AI_FIELD(palette.grooveWidth, "Band Line / Groove width", 0, .5);
AI_FIELD(palette.grooveAuto, "Band Line / Automatic groove", 0, 1);
AI_FIELD(palette.grooveSmooth, "Band Line / Smooth grooves", 0, 1);
AI_FIELD(palette.grooveRounded, "Band Line / Rounded bands", 0, 1);
AI_FIELD(palette.grooveCreaseBlur, "Band Line / Rounded band crease blur", 0, 24);
AI_ENUM(slope.shadingBlend, "Slope / Shading blend", 0, 2, R"({"0":"Overlay","2":"OKLab lightness"})");
AI_ENUM(slope.lightBlend, "Slope / Light blend", 0, 1, R"({"0":"Direct","1":"Linear"})");
AI_ENUM(slope.tintBlend, "Slope / Tint blend", 0, 1, R"({"0":"Multiply","1":"OKLab"})");
AI_ENUM(slope.specularAnisotropyDirection, "Slope / Studio anisotropy direction", 0, 2,
        R"({"0":"Fixed Angle","1":"Along Bands","2":"Across Bands"})");
AI_ENUM(slope.glossSource, "Slope / Gloss source", 0, 3,
        R"({"0":"Shading","1":"Relief","2":"Aspect","3":"Fine shading"})");
AI_FIELD(slope.glossIntensity, "Slope / Gloss intensity", 0, 4);
AI_FIELD(slope.glossBands, "Slope / Gloss bands", 1, 32);
AI_FIELD(slope.glossSharpness, "Slope / Gloss sharpness", 1, 256);
AI_FIELD(slope.glossPhase, "Slope / Gloss phase", 0, 1);
AI_FIELD(slope.glossColor, "Slope / Gloss color", 0, 1);
AI_FIELD(slope.glossRelief, "Slope / Gloss relief", 0, 16);
AI_FIELD(slope.lustreRelief, "Surface / Lustre relief enabled", 0, 1);
AI_FIELD(slope.invertRelief, "Surface / Invert relief", 0, 1);
AI_FIELD(slope.reliefWavesCoatOnly, "Surface / Relief waves on clear coat only", 0, 1);
AI_FIELD(slope.coatSoftbox, "Studio / Clear coat softbox reflections", 0, 1);
AI_ENUM(slope.studio.model, "Studio / Material model", 0, 1, R"({"0":"Studio GGX","1":"OpenPBR Surface"})");
AI_FIELD(slope.studio.coatIor, "Studio / OpenPBR clear coat IOR", 1, 3);
AI_FIELD(slope.studio.coatDarkening, "Studio / OpenPBR clear coat darkening", 0, 1);
AI_FIELD(slope.studio.coatColor, "Studio / OpenPBR clear coat color", 0, 1);
AI_FIELD(slope.studio.edgeTint, "Studio / OpenPBR metal edge tint", 0, 1);
AI_FIELD(slope.studio.fuzz, "Studio / OpenPBR fuzz", 0, 1);
AI_FIELD(slope.studio.fuzzRoughness, "Studio / OpenPBR fuzz roughness", 0, 1);
AI_FIELD(slope.studio.fuzzColor, "Studio / OpenPBR fuzz color", 0, 1);
AI_FIELD(slope.studio.filmIor, "Studio / OpenPBR thin film IOR", 1, 3);
AI_ENUM(slope.studioEnvironmentSource, "Studio / Environment source", 0, 5,
        R"({"0":"Two colors","1":"Gradient","2":"Texture layer 1","3":"Texture layer 2","4":"Texture layer 3","5":"Texture layer 4"})");
AI_FIELD(slope.environmentBrightness, "Studio / Environment brightness", 0, 32);
AI_FIELD(slope.environmentStop1Position, "Studio / Environment gradient stop 1 position", 0, 1);
AI_FIELD(slope.environmentStop2Position, "Studio / Environment gradient stop 2 position", 0, 1);
AI_FIELD(slope.environmentStop3Position, "Studio / Environment gradient stop 3 position", 0, 1);
AI_FIELD(slope.environmentStop4Position, "Studio / Environment gradient stop 4 position", 0, 1);
AI_FIELD(slope.reliefAutoZoom, "Surface / Automatic relief zoom", 0, 1);
AI_FIELD(slope.layerCommon, "Surface / Common mix", 0, 1);
AI_FIELD(slope.replaceSurfaceStyle, "Surface / Replace style", 0, 1);
AI_ENUM(slope.surfaceBlend, "Surface / Blend", 0, 3, R"({"0":"Mix","1":"Add","2":"Multiply","3":"Screen"})");
AI_FIELD(fog.focusSmoothKeyframes, "Fog / Smooth focus across video keyframes", 0, 1);
AI_ENUM(fog.blurQuality, "Fog / Blur quality", 0, 1, R"({"0":"Speed","1":"Appearance"})");
AI_FIELD(hdr.use, "HDR / Enabled", 0, 1);
AI_FIELD(hdr.exposure, "HDR / Exposure stops", -16, 16);
AI_FIELD(hdr.headroom, "HDR / Headroom", .01, 64);
AI_FIELD(hdr.mfrPeakNits, "HDR / Peak nits", 100, 10000);
AI_ENUM(
    hdr.method, "HDR / Tone mapping", 0, 7,
    R"({"0":"Clip","1":"Reinhard","2":"ACES","3":"Filmic","4":"MFR shoulder","5":"MFR log","6":"MFR linear","7":"MFR false color"})");
for (size_t i = 0; i < EFFECT_LAYER_COUNT; ++i) {
    const auto key = "effect." + std::to_string(i + 1) + ".";
    const auto label = "Effect " + std::to_string(i + 1) + " / ";
#define AI_EFFECT(member, minimum, maximum)                                                                  \
    appendShaderField(                                                                                       \
        registeredFields, registeredAddresses, referenceShader, key + #member, label + #member,              \
        [i](auto &shader) -> auto & {                                                                        \
            return shader.effects[i].member;                                                                 \
        },                                                                                                   \
        minimum, maximum)
#define AI_EFFECT_ENUM(member, minimum, maximum, ...)                                                        \
    appendShaderField(                                                                                       \
        registeredFields, registeredAddresses, referenceShader, key + #member, label + #member,              \
        [i](auto &shader) -> auto & {                                                                        \
            return shader.effects[i].member;                                                                 \
        },                                                                                                   \
        minimum, maximum, Json::parse(__VA_ARGS__))
    AI_EFFECT(enabled, 0, 1);
    AI_EFFECT(opacity, 0, 1);
    AI_EFFECT(scale, .1, 100);
    AI_EFFECT(speed, -10, 10);
    AI_EFFECT(evolution, -10, 10);
    AI_EFFECT(density, 0, 1);
    AI_EFFECT(length, .05, 2);
    AI_EFFECT(width, .1, 10);
    AI_EFFECT(direction, -180, 180);
    AI_EFFECT(distortion, 0, 4);
    AI_EFFECT(glow, 0, 4);
    AI_EFFECT(seed, 0, 65535);
    AI_EFFECT(period, 1, 1e9);
    AI_EFFECT(color, 0, 1);
    AI_EFFECT(secondary, 0, 1);
    AI_EFFECT(dropSize, .1, 8);
    AI_EFFECT(syncColorAnimation, 0, 1);
    AI_EFFECT_ENUM(
        type, 0, 7,
        R"({"0":"Rain","1":"Flame","2":"Embers","3":"Mist","5":"Ripples","6":"Flow","7":"Aurora"})");
    AI_EFFECT_ENUM(blend, 0, 3, R"({"0":"Normal","1":"Add","2":"Screen","3":"Multiply"})");
    AI_EFFECT_ENUM(mask, 0, 4, R"({"0":"All","1":"Exterior","2":"Interior","3":"Bands","4":"Edge"})");
    AI_EFFECT_ENUM(rainShape, 0, 1, R"({"0":"Streaks","1":"Drops"})");
    AI_EFFECT_ENUM(flameStyle, 0, 2, R"({"0":"Classic","1":"Turbulent","2":"BurningEdge"})");
    AI_EFFECT_ENUM(embersStyle, 0, 2, R"({"0":"Classic","1":"Rising","2":"Burst"})");
    AI_EFFECT_ENUM(mistStyle, 0, 2, R"({"0":"Classic","1":"Drifting","2":"Pooled"})");
    AI_EFFECT_ENUM(ripplesStyle, 0, 2, R"({"0":"Classic","1":"Interference","2":"Caustics"})");
    AI_EFFECT_ENUM(flowStyle, 0, 1, R"({"0":"Classic","1":"Chase"})");
    AI_EFFECT_ENUM(auroraStyle, 0, 2, R"({"0":"Classic","1":"Curtain","2":"Ribbons"})");
#undef AI_EFFECT
#undef AI_EFFECT_ENUM
}
AI_FIELD(layerOrder.enabled, "Layers / Custom order enabled", 0, 1);
for (uint32_t i = 1; i <= ShdLayerOrder::COUNT; ++i) {
    if (i >= 2 && i <= 5) {
        continue;
    }
    static constexpr const char *names[] = {
        "Band Line", "Texture 1", "Texture 2",    "Texture 3", "Texture 4", "Pattern 1",
        "Pattern 2", "Pattern 3", "Pattern 4",    "Stripe",    "Surface",   "Metal",
        "Sigil",     "PHONK",     "Frost",        "Sea",       "Print",     "Surface color",
        "Effect 1",  "Effect 2",  "Effect 3",     "Effect 4",  "Color",     "Fog",
        "Bloom",     "Tone map",  "Print finish", "VHS",       "Monochrome"};
    static_assert(std::size(names) == ShdLayerOrder::COUNT);
    registeredFields.push_back({"layer.visible." + std::to_string(i),
                                "Layers / Visible: " + std::string(names[i - 1]),
                                {{"type", "boolean"}},
                                [i](const auto &shader) {
                                    return Json(shader.layerOrder.visible(ShdLayer(i)));
                                },
                                [i](auto &shader, const Json &value) {
                                    if (!value.is_boolean()) {
                                        throw std::runtime_error("expected boolean");
                                    }
                                    shader.layerOrder.setVisible(ShdLayer(i), value.get<bool>());
                                }});
}
registeredFields.push_back({"layer.order",
                            "Layers / Composition order",
                            {{"type", "permutation"}, {"minimum", 1}, {"maximum", ShdLayerOrder::COUNT}},
                            [](const auto &shader) {
                                Json values = Json::array();
                                for (auto layer : shader.layerOrder.layers) {
                                    values.push_back(uint32_t(layer));
                                }
                                return values;
                            },
                            [](auto &shader, const Json &value) {
                                if (!value.is_array() || value.size() != ShdLayerOrder::COUNT) {
                                    throw std::runtime_error("Expected all layer IDs exactly once");
                                }
                                std::set<int> ids;
                                for (size_t i = 0; i < value.size(); ++i) {
                                    const int number =
                                        int(readNumber(value[i], 1, ShdLayerOrder::COUNT, true));
                                    if (!ids.insert(number).second) {
                                        throw std::runtime_error("Duplicate layer ID");
                                    }
                                    const auto previousLayer = uint32_t(shader.layerOrder.layers[i]);
                                    if ((previousLayer >= 2 && previousLayer <= 5) ||
                                        (number >= 2 && number <= 5)) {
                                        if (previousLayer != uint32_t(number)) {
                                            throw std::runtime_error("Texture layer positions are protected");
                                        }
                                    }
                                    shader.layerOrder.layers[i] = ShdLayer(number);
                                }
                                shader.layerOrder.enabled = true;
                            }});
registeredFields.push_back({"palette.stops",
                            "Palette / Positioned color stops",
                            {{"type", "stops"}},
                            [](const auto &shader) {
                                Json values = Json::array();
                                for (const auto &stop : shader.palette.stops) {
                                    values.push_back(Json::array({stop.position, stop.color.r, stop.color.g,
                                                                  stop.color.b, stop.color.a}));
                                }
                                return values;
                            },
                            [](auto &shader, const Json &value) {
                                if (!value.is_array() || value.size() < 2 || value.size() > 32) {
                                    throw std::runtime_error("Expected 2 to 32 positioned stops");
                                }
                                std::vector<PaletteStop> stops;
                                for (const auto &row : value) {
                                    if (!row.is_array() || row.size() != 5) {
                                        throw std::runtime_error("Expected [position,r,g,b,a]");
                                    }
                                    const float position = float(readNumber(row[0], 0, 1));
                                    if (position >= 1) {
                                        throw std::runtime_error("Stop position must be below 1");
                                    }
                                    auto rgba = readColor(Json::array({row[1], row[2], row[3], row[4]}));
                                    if (rgba.a != 1) {
                                        throw std::runtime_error("Palette alpha must be 1");
                                    }
                                    stops.push_back({position, rgba});
                                }
                                shader.palette.stops = std::move(stops);
                                if (!validPaletteStops(shader.palette)) {
                                    throw std::runtime_error(
                                        "Palette stops must have distinct increasing positions");
                                }
                                bakePaletteStops(shader.palette);
                            }});
#undef AI_FIELD
#undef AI_ENUM
