//
// Modified by GPT-6 on 2026-09-19, 2026-09-20, 2026-09-22, 2026-09-23
//

#pragma once
#include "AttributeFormModel.hpp"
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
    inline void addTimelineOverlayFields(AttributeFormModel &model) {
        using Model = AttributeFormModel;
        const auto addPercentField = [&model](const char *id, const wchar_t *label, auto member,
                                              float minimum, float maximum, const wchar_t *hint) {
            model.text(
                id, 2, label, hint,
                [member](const Attribute &attribute) {
                    return overlayPercentText(attribute.video.timeline.zoomOverlay.*member);
                },
                [member, minimum, maximum](Attribute &attribute, const std::wstring &text) {
                    return parseOverlayPercent(text, minimum, maximum,
                                               attribute.video.timeline.zoomOverlay.*member);
                });
        };
        const auto addColorFields = [&model](const char *id, const wchar_t *label, auto member,
                                             const char *opacityId, const wchar_t *opacityLabel) {
            model.text(
                id, 2, label, L"Choose a color, enter #RRGGBBAA, #RRGGBB, or R, G, B, A in 0–1.",
                [member](const Attribute &attribute) {
                    return Model::colorText(attribute.video.timeline.zoomOverlay.*member);
                },
                [member](Attribute &attribute, const std::wstring &text) {
                    auto &color = attribute.video.timeline.zoomOverlay.*member;
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
                opacityId, 2, opacityLabel, L"0 is transparent; 100 is opaque.",
                [member](const Attribute &attribute) {
                    return overlayPercentText((attribute.video.timeline.zoomOverlay.*member).a);
                },
                [member](Attribute &attribute, const std::wstring &text) {
                    return parseOverlayPercent(text, 0, 100,
                                               (attribute.video.timeline.zoomOverlay.*member).a);
                });
        };
        model.choice("overlay.visible", 2, L"Show Zoom Ratio",
                     L"Show the zoom ratio in preview and exported video.",
                     [](auto &attribute) -> auto & { return attribute.video.timeline.zoomOverlay.visible; });
        model.numeric(
            "overlay.decimalPlaces", 2, L"Decimal Places",
            L"Digits after the decimal point in the zoom ratio: 0 to 9. Default: 6.",
            [](auto &attribute) -> auto & { return attribute.video.timeline.zoomOverlay.decimalPlaces; },
            uint32_t(0), uint32_t(9));
        model.choice("overlay.custom", 2, L"Custom Appearance",
                     L"Off preserves the legacy font and placement. Editing a style enables Custom "
                     L"Appearance.",
                     [](auto &attribute) -> auto & { return attribute.video.timeline.zoomOverlay.custom; });
        model.numeric(
            "overlay.anchor", 2, L"Alignment",
            L"Choose an anchor and a position with a two-percent margin.",
            [](auto &attribute) -> auto & { return attribute.video.timeline.zoomOverlay.anchor; },
            uint32_t(0), uint32_t(8));
        addPercentField("overlay.x", L"Position X (%)", &VidZoomOverlayAttribute::x, 0, 100,
                        L"Horizontal anchor position, 0 to 100 percent of the video width.");
        addPercentField("overlay.y", L"Position Y (%)", &VidZoomOverlayAttribute::y, 0, 100,
                        L"Vertical anchor position, 0 to 100 percent of the video height.");
        model.text(
            "overlay.family", 2, L"Font",
            L"Installed font family. Use Choose Font to browse available fonts.",
            [](const Attribute &attribute) {
                return overlayFontName(attribute.video.timeline.zoomOverlay.family);
            },
            [](Attribute &attribute, const std::wstring &text) {
                auto name = overlayFontName(text);
                if (name.empty() || name.size() > 256 || name.find('\0') != std::string::npos ||
                    text.find_first_not_of(L" \t\r\n") == std::wstring::npos) {
                    return false;
                }
                attribute.video.timeline.zoomOverlay.family = std::move(name);
                return true;
            });
        model.numeric(
            "overlay.style", 2, L"Font Style", L"Regular, bold, italic, or bold italic.",
            [](auto &attribute) -> auto & { return attribute.video.timeline.zoomOverlay.style; }, uint32_t(0),
            uint32_t(3));
        addPercentField("overlay.size", L"Font Size (% of height)", &VidZoomOverlayAttribute::size, .5f, 20,
                        L"0.5 to 20 percent of video height. Independent of window size and monitor DPI.");
        addColorFields("overlay.color", L"Text Color", &VidZoomOverlayAttribute::color,
                       "overlay.colorOpacity", L"Text Color Opacity (%)");
        model.choice("overlay.outline", 2, L"Enable Outline",
                     L"Draw a continuous border around each glyph.",
                     [](auto &attribute) -> auto & { return attribute.video.timeline.zoomOverlay.outline; });
        addPercentField("overlay.outlineWidth", L"Outline Width (% of font)",
                        &VidZoomOverlayAttribute::outlineWidth, 0, 25,
                        L"Outward glyph border, 0 to 25 percent of the font size.");
        addColorFields("overlay.outlineColor", L"Outline Color", &VidZoomOverlayAttribute::outlineColor,
                       "overlay.outlineOpacity", L"Outline Color Opacity (%)");
        model.choice("overlay.shadow", 2, L"Enable Shadow",
                     L"Draw an offset shadow independently of the outline.",
                     [](auto &attribute) -> auto & { return attribute.video.timeline.zoomOverlay.shadow; });
        addPercentField("overlay.shadowX", L"Shadow X (% of font)", &VidZoomOverlayAttribute::shadowX, -100,
                        100, L"-100 to 100 percent of the font size.");
        addPercentField("overlay.shadowY", L"Shadow Y (% of font)", &VidZoomOverlayAttribute::shadowY, -100,
                        100, L"-100 to 100 percent of the font size.");
        addColorFields("overlay.shadowColor", L"Shadow Color", &VidZoomOverlayAttribute::shadowColor,
                       "overlay.shadowOpacity", L"Shadow Color Opacity (%)");
    }
    inline void normalizeTimelineOverlay(Attribute &after, const FormDraft &draft) {
        auto &overlay = after.video.timeline.zoomOverlay;
        if (draft.contains("overlay.anchor")) {
            constexpr uint32_t anchorColumns = 3;
            constexpr float anchorPositions[]{.02f, .5f, .98f};
            const uint32_t column = overlay.anchor % anchorColumns;
            const uint32_t row = overlay.anchor / anchorColumns;
            if (!draft.contains("overlay.x")) {
                overlay.x = anchorPositions[column];
            }
            if (!draft.contains("overlay.y")) {
                overlay.y = anchorPositions[row];
            }
        }
        if (!draft.contains("overlay.custom") && std::ranges::any_of(draft, [](const auto &entry) {
                return entry.first.starts_with("overlay.") && entry.first != "overlay.visible" &&
                       entry.first != "overlay.decimalPlaces";
            })) {
            overlay.custom = true;
        }
    }
} // namespace merutilm::rff2::workspace
