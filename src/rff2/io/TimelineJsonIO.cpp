//
// Modified by GPT-6 on 2026-09-23
//

#include "TimelineJsonIO.hpp"
#include "LocalAiSettings.hpp"
#include "../video/TimelineParams.hpp"
#include "../attr/Selectable.h"
#include "../attr/VidTimelineTarget.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>
#include <windows.h>

namespace merutilm::rff2 {
    namespace {
        using Json = nlohmann::json;
        void require(bool ok, const std::string &message) {
            if (!ok) throw std::runtime_error(message);
        }
        Json color(glm::vec4 c) { return Json::array({c.r, c.g, c.b, c.a}); }
        glm::vec4 readColor(const Json &j) {
            require(j.is_array() && j.size() == 4, "color: expected four RGBA numbers");
            glm::vec4 result;
            for (int i = 0; i < 4; ++i) result[i] = j.at(i).get<float>();
            return result;
        }
        std::string utf8(const wchar_t *value) {
            const int n = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
            std::string result(n, '\0');
            WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), n, nullptr, nullptr);
            result.pop_back();
            return result;
        }
        void shape(const Json &value, const Json &sample, const std::string &path) {
            if (sample.is_object()) {
                require(value.is_object() && value.size() == sample.size(), path + ": missing or unknown fields");
                for (const auto &[key, item] : sample.items()) {
                    require(value.contains(key), path + ": missing " + key);
                    shape(value.at(key), item, path + "." + key);
                }
            } else if (sample.is_array()) {
                require(value.is_array(), path + ": expected array");
                if (!sample.empty()) {
                    for (const auto &item : value) shape(item, sample.front(), path + "[]");
                }
            } else if (sample.is_boolean()) {
                require(value.is_boolean(), path + ": expected boolean");
            } else if (sample.is_string()) {
                require(value.is_string(), path + ": expected string");
                require(value.get_ref<const std::string &>().find('\0') == std::string::npos, path + ": NUL is not allowed");
            } else if (sample.is_number()) {
                require(value.is_number(), path + ": expected number");
                const double n = value.get<double>();
                require(std::isfinite(n) && std::abs(n) <= std::numeric_limits<float>::max(), path + ": non-finite number");
                if (sample.is_number_integer()) {
                    require(value.is_number_integer(), path + ": expected integer");
                    require(n >= (sample.is_number_unsigned() ? 0.0 : -9e15) && n <= 9e15, path + ": integer out of range");
                }
            }
        }
        void range(float value, float lo, float hi, const char *name) {
            require(std::isfinite(value) && value >= lo && value <= hi, std::string(name) + ": out of range");
        }
        void validColor(glm::vec4 c) {
            for (int i = 0; i < 4; ++i) range(c[i], 0, 1, "RGBA");
        }
        template<typename T> Json options() {
            Json result = Json::object();
            for (auto value : Selectable::values<T>())
                result[std::to_string(int(value))] = utf8(Selectable::toString(value).c_str());
            return result;
        }
        Json options(const TimelineParamDesc &p) {
            if (p.kind == TimelineParamKind::BOOL) return {{"0", "Off"}, {"1", "On"}};
            if (p.kind != TimelineParamKind::ENUM) return nullptr;
            const std::wstring_view group = p.group, name = p.name;
            if (name == L"UV Mode") return options<ShdTextureUVMode>();
            if (name == L"Blend Mode") return options<ShdTextureBlendMode>();
            if (name == L"Ink Mode") return options<ShdPatternInkMode>();
            if (name == L"Type" && group.starts_with(L"Pattern")) return options<ShdPatternType>();
            if (name == L"Type" && group == L"Stripe") return options<ShdStripeType>();
            if (name == L"Source" && group == L"Warp") return options<ShdWarpSource>();
            if (p.id == uint16_t(VidTimelineTarget::PALETTE_CYCLE_CURVE)) return options<ShdPaletteCycleCurve>();
            if (p.id == uint16_t(VidTimelineTarget::PALETTE_ITERATION_COLORING)) return options<ShdPalIterationColoringMode>();
            if (p.id == uint16_t(VidTimelineTarget::CAMERA_PROJECTION)) return options<FrtProjectionMethod>();
            if (p.id == uint16_t(VidTimelineTarget::CAMERA_LAYOUT)) return options<FrtPanoramaLayout>();
            return nullptr;
        }
    }

    nlohmann::json TimelineJsonIO::document(const VidTimelineAttribute &t) {
        Json tracks = Json::array(), holds = Json::array(), clips = Json::array();
        for (const auto &track : t.tracks) {
            Json keys = Json::array();
            for (const auto &k : track.keys)
                keys.push_back({{"depth", k.depth}, {"value", k.value}, {"color", color(k.color)}, {"out", int(k.out)}});
            tracks.push_back({{"targetId", track.targetId}, {"enabled", track.enabled}, {"keys", keys}});
        }
        for (const auto &h : t.holds) holds.push_back({{"depth", h.depth}, {"seconds", h.seconds}});
        for (const auto &c : t.audio.clips)
            clips.push_back({{"id", c.id}, {"path", c.path}, {"sourceDuration", c.sourceDuration},
                {"start", c.start}, {"in", c.in}, {"out", c.out}, {"fadeIn", c.fadeIn}, {"fadeOut", c.fadeOut},
                {"gain", c.gain}, {"muted", c.muted}});
        const auto &o = t.zoomOverlay;
        Json overlay{{"decimalPlaces", o.decimalPlaces}, {"visible", o.visible}, {"custom", o.custom},
            {"anchor", o.anchor}, {"x", o.x}, {"y", o.y}, {"size", o.size}, {"family", o.family},
            {"style", o.style}, {"color", color(o.color)}, {"outline", o.outline}, {"outlineWidth", o.outlineWidth},
            {"outlineColor", color(o.outlineColor)}, {"shadow", o.shadow}, {"shadowX", o.shadowX},
            {"shadowY", o.shadowY}, {"shadowColor", color(o.shadowColor)}};
        return {{"format", "RFF_Super.timeline"}, {"version", 1}, {"timeline", {
            {"enabled", t.enabled}, {"estimateKeyframes", t.estimateKeyframes}, {"tracks", tracks}, {"holds", holds},
            {"rotationMode", int(t.rotationMode)}, {"rotationPeriod", t.rotationPeriod},
            {"rotationDirection", int(t.rotationDirection)}, {"rotationStartAngle", t.rotationStartAngle},
            {"zoomOverlay", overlay}, {"audio", {{"exportEnabled", t.audio.exportEnabled}, {"gain", t.audio.gain}, {"clips", clips}}}}}};
    }

    VidTimelineAttribute TimelineJsonIO::parse(std::string_view text) {
        require(text.size() <= maximumBytes, "JSON exceeds 16 MiB");
        std::vector<std::unordered_set<std::string>> objectKeys;
        const auto callback = [&](int depth, Json::parse_event_t event, Json &parsed) {
            require(depth < 32, "JSON nesting exceeds 32 levels");
            if (event == Json::parse_event_t::object_start) objectKeys.emplace_back();
            if (event == Json::parse_event_t::key)
                require(objectKeys.back().insert(parsed.get<std::string>()).second, "Duplicate JSON field");
            if (event == Json::parse_event_t::object_end) objectKeys.pop_back();
            return true;
        };
        const auto j = Json::parse(text, callback);
        VidTimelineAttribute sample;
        sample.tracks.push_back({0, true, {{1, 1, glm::vec4(1), VidKeyInterpolation::LINEAR}}});
        sample.holds.push_back({1, 1});
        sample.audio.clips.emplace_back();
        shape(j, document(sample), "document");
        require(j.at("format") == "RFF_Super.timeline" && j.at("version") == 1, "Unsupported timeline JSON format/version");
        const auto &v = j.at("timeline");
        VidTimelineAttribute t;
        t.enabled = v.at("enabled");
        t.estimateKeyframes = v.at("estimateKeyframes");
        range(t.estimateKeyframes, 1, 1000000, "estimateKeyframes");
        const int64_t mode = v.at("rotationMode"), direction = v.at("rotationDirection");
        require(mode >= 0 && mode <= 1 && direction >= 0 && direction <= 1, "Invalid rotation mode/direction");
        t.rotationMode = VidRotationMode(mode);
        t.rotationDirection = VidRotationDirection(direction);
        t.rotationPeriod = v.at("rotationPeriod");
        t.rotationStartAngle = v.at("rotationStartAngle");
        range(t.rotationPeriod, .01f, 86400, "rotationPeriod");
        range(t.rotationStartAngle, -360000, 360000, "rotationStartAngle");
        require(v.at("tracks").size() <= 4096 && v.at("holds").size() <= 65536, "Too many tracks/holds");
        std::unordered_set<uint16_t> targets;
        for (const auto &track : v.at("tracks")) {
            const auto id = track.at("targetId").get<uint64_t>();
            require(id <= 65535 && targets.insert(uint16_t(id)).second, "Invalid or duplicate targetId");
            VidTimelineTrack result{uint16_t(id), track.at("enabled").get<bool>(), {}};
            const auto *p = TimelineParams::find(result.targetId);
            require(track.at("keys").size() <= 65536, "Too many keys");
            for (const auto &key : track.at("keys")) {
                const int64_t out = key.at("out");
                require(out >= 0 && out <= 3, "out: expected 0..3");
                VidTimelineKey k{key.at("depth"), key.at("value"), readColor(key.at("color")), VidKeyInterpolation(out)};
                range(k.depth, -1000000, 1000000, "key.depth");
                validColor(k.color);
                if (id == 0) range(k.value, VidTimelineAttribute::MIN_SPEED, std::numeric_limits<float>::max(), "speed");
                if (p && p->kind != TimelineParamKind::COLOR) {
                    range(k.value, p->minValue, p->maxValue, "key.value");
                    if (p->kind == TimelineParamKind::BOOL || p->kind == TimelineParamKind::ENUM)
                        require(std::floor(k.value) == k.value && out == 0, "Boolean/enum keys require integer values and STEP (out=0)");
                }
                result.keys.push_back(k);
            }
            std::ranges::sort(result.keys, std::greater{}, &VidTimelineKey::depth);
            for (size_t i = 1; i < result.keys.size(); ++i)
                require(result.keys[i - 1].depth != result.keys[i].depth, "Duplicate key depth");
            t.tracks.push_back(std::move(result));
        }
        for (const auto &h : v.at("holds")) {
            VidTimelineHold hold{h.at("depth"), h.at("seconds")};
            range(hold.depth, -1000000, 1000000, "hold.depth");
            range(hold.seconds, 0, 604800, "hold.seconds");
            t.holds.push_back(hold);
        }
        const auto &o = v.at("zoomOverlay");
        auto &z = t.zoomOverlay;
        for (const auto *name : {"anchor", "style", "decimalPlaces"})
            require(o.at(name).get<uint64_t>() <= 9, std::string(name) + ": out of range");
        z.decimalPlaces = o.at("decimalPlaces"); z.visible = o.at("visible"); z.custom = o.at("custom");
        z.anchor = o.at("anchor"); z.x = o.at("x"); z.y = o.at("y"); z.size = o.at("size");
        z.family = o.at("family"); z.style = o.at("style"); z.color = readColor(o.at("color"));
        z.outline = o.at("outline"); z.outlineWidth = o.at("outlineWidth"); z.outlineColor = readColor(o.at("outlineColor"));
        z.shadow = o.at("shadow"); z.shadowX = o.at("shadowX"); z.shadowY = o.at("shadowY"); z.shadowColor = readColor(o.at("shadowColor"));
        require(z.anchor <= 8 && z.style <= 3 && !z.family.empty() && z.family.size() <= 256, "Invalid overlay anchor/style/font");
        range(z.x, 0, 1, "overlay.x"); range(z.y, 0, 1, "overlay.y"); range(z.size, .005f, .2f, "overlay.size");
        range(z.outlineWidth, 0, .25f, "overlay.outlineWidth");
        range(z.shadowX, -1, 1, "overlay.shadowX"); range(z.shadowY, -1, 1, "overlay.shadowY");
        validColor(z.color); validColor(z.outlineColor); validColor(z.shadowColor);
        const auto &a = v.at("audio");
        t.audio.exportEnabled = a.at("exportEnabled"); t.audio.gain = a.at("gain");
        require(a.at("clips").size() <= VidAudioAttribute::maximumClips, "Too many audio clips");
        for (const auto &c : a.at("clips")) {
            t.audio.clips.push_back({c.at("id"), c.at("path"), c.at("sourceDuration"), c.at("start"),
                c.at("in"), c.at("out"), c.at("fadeIn"), c.at("fadeOut"), c.at("gain"), c.at("muted")});
        }
        require(t.audio.valid(), "Invalid audio clip timing, ID, path or gain");
        return t;
    }

    std::string TimelineJsonIO::systemPrompt(const ShaderAttribute &shader) {
        Json catalog = Json::array();
        catalog.push_back({{"targetId", 0}, {"name", "Zoom speed"}, {"type", "float"},
            {"minimum", VidTimelineAttribute::MIN_SPEED}, {"maximum", std::numeric_limits<float>::max()}, {"unit", "keyframe depths/second"}});
        constexpr const char *kinds[] = {"float", "rgba", "boolean", "enum"};
        for (const auto &p : TimelineParams::all()) {
            Json row{{"targetId", p.id}, {"group", utf8(p.group)}, {"name", utf8(p.name)},
                {"type", kinds[int(p.kind)]}, {"minimum", p.minValue}, {"maximum", p.maxValue},
                {"affectsPNG", TimelineParams::movesOverStaticImage(p.id)}};
            row["options"] = options(p);
            if (p.kind == TimelineParamKind::COLOR) row["base"] = color(p.getColor(shader));
            else row["base"] = p.getValue(shader);
            catalog.push_back(std::move(row));
        }
        auto guide = LocalAiSettings::systemPrompt(shader);
        const auto begin = guide.find("## Appearance guide");
        const auto end = guide.find("Catalog rows are");
        require(begin != std::string::npos && end != std::string::npos && end > begin, "Appearance guide headings are missing");
        guide = guide.substr(begin, end - begin);
        Json base = Json::array();
        const auto baseSettings = LocalAiSettings::catalog(shader);
        for (const auto &[key, item] : baseSettings.items())
            base.push_back(Json::array({key, item.at("description"), item.at("type"),
                item.value("minimum", Json()), item.value("maximum", Json()),
                item.value("current", Json()), item.value("options", Json())}));
        return std::string(R"PROMPT(You edit an RFF_Super video timeline. Reply with exactly one complete JSON document in the supplied RFF_Super.timeline version 1 format, without Markdown or prose. Preserve every field unless the user requests a change. Do not return a shader patch. The timeline JSON is the only editable document; context and catalogs are read-only. Do not copy context/catalog fields into your response. Never invent target IDs, paths, keyframes or visual evidence. Unknown existing tracks must be preserved unchanged.

Timeline behavior:
- Depth is a keyframe number, NOT seconds, magnification or displayed travel distance. Playback moves from the context startDepth down to endDepth. Travel distance=startDepth-depth. Keys and holds stay at a fractal location when speed changes. estimateKeyframes must remain unchanged for the current folder. Image labels K identify this depth; pages are in descending playback order, left-to-right then top-to-bottom. Only the selected page is attached; do not assume unseen frames. Each tile is a rendered preview at timeAt(depth), not the raw iteration map. RFMZ/RFM can be recolored; PNG already has baked colors, and only affectsPNG=true parameters can move it. Blank cells are not frames.
- tracks contain unique targetId, enabled and keys. Each key is {depth,value,color:[r,g,b,a],out}. Keep unused value/color fields. Keys have distinct depths in descending order; a track's first/last value extends beyond its end keys. Disabled or empty tracks use base settings. Target 0 is speed in keyframe depths/second and must stay >=0.0001; use holds {depth,seconds} to pause zoom. Total duration integrates 1/speed plus holds; context seconds are derived and not editable. Higher speed means less time. Timeline enabled=false disables speed and parameter automation.
- out controls the segment to the NEXT, lower-depth key: 0 STEP holds until the next key; 1 LINEAR; 2 SMOOTH uses u*u*(3-2*u); 3 CUBIC uses neighboring values and may overshoot before clamping. Bool/enum values are integers and require out=0. Colors are RGBA in [0,1], interpolated perceptually in OKLab. Numeric parameters clamp to catalog ranges. Bounds are limits, not recommended artistic values.
- rotationMode=0 uses camera rotation keys; 1 uses constant-period rotation. rotationPeriod is seconds per revolution [0.01,86400], rotationDirection=0 clockwise/1 counterclockwise, rotationStartAngle is degrees [-360000,360000]. Constant rotation continues during holds. Speed-like animation parameters are integrated over elapsed time; changing speed does not reset phase.
- Camera tracks: Rotation is degrees; Projection 0=planar, 1=equirectangular 360, 2=perspective 360. Pitch tilts the 360 camera in degrees; Field of View controls the perspective cone. Range bounds the far field as a power of ten of the horizon radius; Layout 0=ground plane with sky, 1=whole sphere. Camera projection works from available keyframes and does not calculate new fractal locations. Texture layers require an existing image, Enabled and nonzero Opacity; Scale U/V control repetition, Size scales the layer, Keep Aspect preserves its source ratio. Shared UV/Blend/Scroll modes follow the guide below. Legacy Studio Softbox tracks are retained for compatibility but no longer affect rendering.
- zoomOverlay: visible toggles the zoom label; custom=false uses the default appearance. x/y are normalized frame coordinates [0,1]; anchor=0..8 selects a 3x3 anchor grid, row-major. size is relative to frame height [.005,.2]; family names a local font; style bit 1=bold, bit 2=italic; decimalPlaces=0..9. RGBA colors are [0,1]. outlineWidth=[0,.25], shadowX/Y=[-1,1] are relative to text size. Overlay is composited separately and is not included in contact-sheet tiles.
- audio: all times are INTEGER MICROSECONDS, 1000000 per second, not depth. clip start is video time, in/out trim the source, sourceDuration is source length, fadeIn/fadeOut are durations within the trimmed clip. IDs must remain unique; preserve existing paths and sourceDuration. gain is linear amplitude, muted silences a clip, exportEnabled controls audio export. Clips must not overlap. Master and clip gain are [0,4]. Fade durations must be nonnegative and their sum must not exceed the trimmed duration. Times must fit within seven days. Changing zoom speed does not automatically move audio. Do not fabricate media files.
- JSON import replaces the whole timeline atomically and can be undone. It does not import the read-only shader base, keyframe folder, export settings or media. Keep all required fields, no extra fields, finite numbers, no duplicate fields/IDs/depths. Maximum JSON size is 16 MiB. Keep edits within the current context depth range. Do not change estimateKeyframes to stretch an existing folder.

Appearance behavior reference follows. References to shader patch keys, protected fields or unavailable texture settings concern base appearance only, not the timeline target catalog. Use ONLY the target catalog to determine which properties can be animated. Texture tracks operate on existing local image layers; do not invent or replace image paths. Most material/effect recipe controls are base-only and cannot be animated. Base settings and catalog values describe the current scene, not an instruction to change it.
)PROMPT") + guide + "\nTimeline target catalog:\n" + catalog.dump() +
            "\nRead-only base appearance catalog: rows are [key,label,type,min,max,current,options]. Null means not applicable or omitted; options explain enum values.\n" + base.dump();
    }
}
