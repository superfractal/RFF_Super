//
// Modified by GPT-6 on 2026-09-19, 2026-09-20, 2026-09-22, 2026-09-23, 2026-09-26, 2026-09-30, 2026-10-01
// Modified by Opus 5.5 on 2026-10-03
//

#pragma once
#include "AttributeFormModel.hpp"
#include <string_view>
#include <windows.h>

namespace merutilm::rff2::workspace {
    inline std::wstring overlayPercentText(float value) {
        const double percent = double(value) * 100.;
        for (int precision = 1; precision <= std::numeric_limits<double>::max_digits10; ++precision) {
            char buffer[96];
            const auto conversion = std::to_chars(buffer, buffer + sizeof(buffer), percent,
                                                  std::chars_format::general, precision);
            if (conversion.ec != std::errc{}) {
                continue;
            }
            const std::wstring text(buffer, conversion.ptr);
            double parsedPercent = 0;
            if (AttributeFormModel::parse(text, parsedPercent) && float(parsedPercent / 100.) == value) {
                return AttributeFormModel::number(parsedPercent);
            }
        }
        return AttributeFormModel::number(percent);
    }
    inline std::wstring overlayFontName(const std::string &name) {
        const int wideLength =
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name.data(), int(name.size()), nullptr, 0);
        std::wstring wideName(wideLength, L'\0');
        if (wideLength) {
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name.data(), int(name.size()), wideName.data(),
                                wideLength);
        }
        return wideName;
    }
    inline std::string overlayFontName(const std::wstring &name) {
        const int utf8Length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, name.data(),
                                                   int(name.size()), nullptr, 0, nullptr, nullptr);
        std::string utf8Name(utf8Length, '\0');
        if (utf8Length) {
            WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, name.data(), int(name.size()), utf8Name.data(),
                                utf8Length, nullptr, nullptr);
        }
        return utf8Name;
    }
    inline bool parseOverlayPercent(std::wstring_view text, double minimum, double maximum, float &fraction) {
        double value = 0;
        if (!AttributeFormModel::parse(text, value) || value < minimum || value > maximum) {
            return false;
        }
        fraction = float(value / 100.);
        return true;
    }
    inline void addTimelineOverlayFields(AttributeFormModel &model, bool iteration = false) {
        using Model = AttributeFormModel;
        const std::string prefix = iteration ? "iterationOverlay." : "overlay.";
        const int group = iteration ? 4 : 2;
        const auto get = [iteration](auto &attribute) -> auto & {
            return iteration ? attribute.video.timeline.maxIterationOverlay : attribute.video.timeline.zoomOverlay;
        };
        const auto addPercentField = [&model, get, group](const std::string &id, const wchar_t *label, auto member,
                                              float minimum, float maximum, const wchar_t *hint) {
            model.text(
                id, group, label, hint,
                [get, member](const Attribute &attribute) {
                    return overlayPercentText(get(attribute).*member);
                },
                [get, member, minimum, maximum](Attribute &attribute, const std::wstring &text) {
                    return parseOverlayPercent(text, minimum, maximum,
                                               get(attribute).*member);
                });
        };
        const auto addColorFields = [&model, get, group](const std::string &id, const wchar_t *label, auto member,
                                             const std::string &opacityId, const wchar_t *opacityLabel) {
            model.text(
                id, group, label, L"Choose a color, enter #RRGGBBAA, #RRGGBB, or R, G, B, A in 0–1.",
                [get, member](const Attribute &attribute) {
                    return Model::colorText(get(attribute).*member);
                },
                [get, member](Attribute &attribute, const std::wstring &text) {
                    auto &color = get(attribute).*member;
                    if (text.size() == 9 && text.front() == L'#') {
                        if (text.find_first_not_of(L"0123456789abcdefABCDEF", 1) != std::wstring::npos) {
                            return false;
                        }
                        const auto value = std::stoul(text.substr(1), nullptr, 16);
                        color = glm::vec4((value >> 24) & 255, (value >> 16) & 255, (value >> 8) & 255,
                                          value & 255) /
                                255.f;
                        return true;
                    }
                    return Model::parseColor(text, color);
                },
                FormField::Editor::COLOR);
            model.text(
                opacityId, group, opacityLabel, L"0 is transparent; 100 is opaque.",
                [get, member](const Attribute &attribute) {
                    return overlayPercentText((get(attribute).*member).a);
                },
                [get, member](Attribute &attribute, const std::wstring &text) {
                    return parseOverlayPercent(text, 0, 100,
                                               (get(attribute).*member).a);
                });
        };
        model.choice(prefix + "visible", group, iteration ? L"Show Max Iterations" : L"Show Zoom Ratio",
                     L"Show this text in preview and exported video.",
                     [get](auto &attribute) -> auto & { return get(attribute).visible; });
        const auto addTime = [&](const char *id, const wchar_t *label, const wchar_t *hint, auto member) {
            model.text(prefix + id, group, label, hint,
                [get, member](const Attribute &a) { return Model::number(get(a).limitDisplayTime ? get(a).*member : 0.0); },
                [get, member](Attribute &a, const std::wstring &text) {
                    double seconds = 0;
                    if (!Model::parse(text, seconds) || !std::isfinite(seconds) || seconds < 0 || seconds > 604800) return false;
                    auto &overlay = get(a);
                    if (!overlay.limitDisplayTime) overlay.displayStart = overlay.displayEnd = 0;
                    overlay.limitDisplayTime = true;
                    overlay.*member = seconds;
                    return true;
                });
        };
        addTime("displayStart", L"Display Start (s)", L"0 = video start.", &VidZoomOverlayAttribute::displayStart);
        addTime("displayEnd", L"Display End (s)", L"0 = video end.", &VidZoomOverlayAttribute::displayEnd);
        if (iteration) model.choice(prefix + "interpolate", group, L"Interpolate per Frame",
            L"Gradually change the displayed count between source maps. The rendering limit is unchanged.",
            [](auto &a) -> auto & { return a.video.timeline.interpolateMaxIteration; });
        if (!iteration) model.numeric(
            prefix + "decimalPlaces", group, L"Decimal Places",
            L"Digits after the decimal point in the zoom ratio: 0 to 9. Default: 6.",
            [get](auto &attribute) -> auto & { return get(attribute).decimalPlaces; },
            uint32_t(0), uint32_t(9));
        if (!iteration) model.choice(prefix + "custom", group, L"Custom Appearance",
                     L"Off preserves the legacy font and placement. Editing a style enables Custom "
                     L"Appearance.",
                     [get](auto &attribute) -> auto & { return get(attribute).custom; });
        model.numeric(
            prefix + "anchor", group, L"Alignment",
            L"Choose an anchor and a position with a two-percent margin.",
            [get](auto &attribute) -> auto & { return get(attribute).anchor; },
            uint32_t(0), uint32_t(8));
        addPercentField(prefix + "x", L"Position X (%)", &VidZoomOverlayAttribute::x, 0, 100,
                        L"Horizontal anchor position, 0 to 100 percent of the video width.");
        addPercentField(prefix + "y", L"Position Y (%)", &VidZoomOverlayAttribute::y, 0, 100,
                        L"Vertical anchor position, 0 to 100 percent of the video height.");
        model.text(
            prefix + "family", group, L"Font",
            L"Installed font family. Use Choose Font to browse available fonts.",
            [get](const Attribute &attribute) {
                return overlayFontName(get(attribute).family);
            },
            [get](Attribute &attribute, const std::wstring &text) {
                auto name = overlayFontName(text);
                if (name.empty() || name.size() > 256 || name.find('\0') != std::string::npos ||
                    text.find_first_not_of(L" \t\r\n") == std::wstring::npos) {
                    return false;
                }
                get(attribute).family = std::move(name);
                return true;
            });
        model.numeric(
            prefix + "style", group, L"Font Style", L"Regular, bold, italic, or bold italic.",
            [get](auto &attribute) -> auto & { return get(attribute).style; }, uint32_t(0),
            uint32_t(3));
        addPercentField(prefix + "size", L"Font Size (% of height)", &VidZoomOverlayAttribute::size, .5f, 20,
                        L"0.5 to 20 percent of video height. Independent of window size and monitor DPI.");
        addColorFields(prefix + "color", L"Text Color", &VidZoomOverlayAttribute::color,
                       prefix + "colorOpacity", L"Text Color Opacity (%)");
        model.choice(prefix + "outline", group, L"Enable Outline",
                     L"Draw a continuous border around each glyph.",
                     [get](auto &attribute) -> auto & { return get(attribute).outline; });
        addPercentField(prefix + "outlineWidth", L"Outline Width (% of font)",
                        &VidZoomOverlayAttribute::outlineWidth, 0, 25,
                        L"Outward glyph border, 0 to 25 percent of the font size.");
        addColorFields(prefix + "outlineColor", L"Outline Color", &VidZoomOverlayAttribute::outlineColor,
                       prefix + "outlineOpacity", L"Outline Color Opacity (%)");
        model.choice(prefix + "shadow", group, L"Enable Shadow",
                     L"Draw an offset shadow independently of the outline.",
                     [get](auto &attribute) -> auto & { return get(attribute).shadow; });
        addPercentField(prefix + "shadowX", L"Shadow X (% of font)", &VidZoomOverlayAttribute::shadowX, -100,
                        100, L"-100 to 100 percent of the font size.");
        addPercentField(prefix + "shadowY", L"Shadow Y (% of font)", &VidZoomOverlayAttribute::shadowY, -100,
                        100, L"-100 to 100 percent of the font size.");
        addColorFields(prefix + "shadowColor", L"Shadow Color", &VidZoomOverlayAttribute::shadowColor,
                       prefix + "shadowOpacity", L"Shadow Color Opacity (%)");
    }
    // A row Custom Appearance governs: anything but whether, when and with how many digits the overlay shows.
    inline bool overlayAppearanceField(std::string_view id, std::string_view prefix) {
        if (!id.starts_with(prefix)) {
            return false;
        }
        const std::string_view name = id.substr(prefix.size());
        return name != "visible" && name != "decimalPlaces" && name != "interpolate" && name != "limitDisplayTime" &&
               name != "displayStart" && name != "displayEnd" && name != "custom" && name != "positionMode";
    }
    inline void normalizeTimelineOverlay(Attribute &after, const FormDraft &draft) {
        for (bool iteration : {false, true}) {
            const std::string prefix = iteration ? "iterationOverlay." : "overlay.";
            auto &overlay = iteration ? after.video.timeline.maxIterationOverlay : after.video.timeline.zoomOverlay;
            if (draft.contains(prefix + "anchor")) {
                constexpr float positions[]{.02f, .5f, .98f};
                if (!draft.contains(prefix + "x")) overlay.x = positions[overlay.anchor % 3];
                if (!draft.contains(prefix + "y")) overlay.y = positions[overlay.anchor / 3];
            }
            if (!draft.contains(prefix + "custom") && std::ranges::any_of(draft, [&prefix](const auto &entry) {
                    return overlayAppearanceField(entry.first, prefix);
                })) overlay.custom = true;
        }
        after.video.timeline.zoomOverlay.showMaxIteration = after.video.timeline.maxIterationOverlay.visible;
    }
} // namespace merutilm::rff2::workspace
