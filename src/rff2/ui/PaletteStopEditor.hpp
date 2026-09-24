//
// Modified by GPT-6 on 2026-09-11, 2026-09-14, 2026-09-22
//

#pragma once
#include "NativeDialogs.hpp"
#include "SettingsWindow.hpp"
#include "../attr/ShdPaletteAttribute.h"
#include "Callback.hpp"
#include <commdlg.h>
#include <format>

namespace merutilm::rff2 {
    inline void registerPaletteStopEditor(SettingsWindow &window, ShdPaletteAttribute &palette,
                                          std::function<void()> changed) {
        struct State {
            float index = 1, position = 0, easing = 1;
            HWND indexField = nullptr, positionField = nullptr, easingField = nullptr, colorField = nullptr;
            HWND hexField = nullptr;
            std::wstring hex = L"#000000";
            std::vector<HWND> editable;
        };
        auto state = std::make_shared<State>();
        auto refresh = [state, &window, &palette] {
            const bool active = !palette.stops.empty();
            state->index =
                active ? std::clamp(std::round(state->index), 1.0f, float(palette.stops.size())) : 1;
            state->position = active ? palette.stops[size_t(state->index) - 1].position : 0;
            state->easing = float(palette.stopEasing);
            glm::vec4 color = active ? palette.stops[size_t(state->index) - 1].color : glm::vec4(0);
            state->hex = std::format(L"#{:02X}{:02X}{:02X}", int(std::clamp(color.r, 0.0f, 1.0f) * 255),
                                     int(std::clamp(color.g, 0.0f, 1.0f) * 255),
                                     int(std::clamp(color.b, 0.0f, 1.0f) * 255));
            if (state->hexField) {
                SetWindowTextW(state->hexField, state->hex.c_str());
            }
            for (HWND field : state->editable) {
                window.setRowEnabled(field, active);
            }
            window.setFloatValueByField(state->indexField, state->index);
            window.setFloatValueByField(state->positionField, state->position);
            window.setFloatValueByField(state->easingField, state->easing);
            InvalidateRect(window.getWindow(), nullptr, FALSE);
        };
        auto commit = [&palette, changed, refresh] {
            bakePaletteStops(palette);
            changed();
            refresh();
        };
        window.registerSectionHeader(L"Color Stops", false);
        window.registerButton(
            L"Convert Palette", L"Create 8 Stops (Lossy)",
            [&palette, state, commit] {
                if (palette.colors.empty()) {
                    return;
                }
                std::vector<PaletteStop> stops;
                for (int stopIndex = 0; stopIndex < 8; ++stopIndex) {
                    stops.push_back({float(stopIndex) / 8,
                                     palette.colors[size_t(stopIndex) * palette.colors.size() / 8]});
                }
                palette.stops = std::move(stops);
                state->index = 1;
                commit();
            },
            L"Create Color Stops",
            L"Explicitly replaces the palette with an eight-stop approximation. Large raw and recipe "
            L"palettes remain untouched until this button is pressed. Saved palettes include the baked "
            L"colors for older versions.");
        auto slider = [&](const wchar_t *label, float *value, float lowerPosition, float upperPosition,
                          std::function<void()> callback) {
            HWND field = window.registerSliderInput(
                label, value, lowerPosition, upperPosition, Unparser::floatFixed(4), Parser::FLOAT,
                ValidCondition::floatInRange(lowerPosition, upperPosition), std::move(callback), label,
                L"Stops wrap around the color cycle. Positions must remain distinct and ordered.");
            state->editable.push_back(field);
            return field;
        };
        state->indexField = slider(L"Selected Stop", &state->index, 1, 32, refresh);
        state->positionField =
            slider(L"Stop Position", &state->position, 0, 0.9999f, [state, &palette, commit] {
                if (palette.stops.empty()) {
                    return;
                }
                size_t stopIndex = size_t(state->index) - 1;
                float lowerPosition = stopIndex == 0 ? 0 : palette.stops[stopIndex - 1].position + 0.00001f;
                float upperPosition = stopIndex + 1 == palette.stops.size()
                                          ? 0.99999f
                                          : palette.stops[stopIndex + 1].position - 0.00001f;
                if (lowerPosition <= upperPosition) {
                    palette.stops[stopIndex].position =
                        std::clamp(state->position, lowerPosition, upperPosition);
                }
                commit();
            });
        window.setSliderFractionalSteps(state->positionField);
        state->easingField = window.registerSliderInput(
            L"Stop Easing", &state->easing, 0, 2,
            [](const float &value) -> std::wstring {
                return value < 0.5f ? L"Linear" : value < 1.5f ? L"Smoothstep" : L"Smootherstep";
            },
            [](std::wstring &value) -> float {
                if (value == L"Linear") {
                    return 0;
                }
                if (value == L"Smoothstep") {
                    return 1;
                }
                if (value == L"Smootherstep") {
                    return 2;
                }
                return std::stof(value);
            },
            ValidCondition::floatInRange(0, 2),
            [state, &palette, commit] {
                palette.stopEasing = uint32_t(std::round(state->easing));
                commit();
            },
            L"Stop Easing",
            L"Linear, cubic Smoothstep or quintic Smootherstep between neighboring color stops.");
        state->editable.push_back(state->easingField);
        state->hexField = window.registerTextInput<std::wstring>(
            L"HEX Color", &state->hex, [](const std::wstring &value) { return value; },
            [](std::wstring &value) { return value; },
            [](const std::wstring &value) {
                return value.size() == 7 && value[0] == L'#' &&
                       value.find_first_not_of(L"0123456789abcdefABCDEF", 1) == std::wstring::npos;
            },
            [state, &palette, commit] {
                if (palette.stops.empty()) {
                    return;
                }
                auto value = std::stoul(state->hex.substr(1), nullptr, 16);
                auto &color = palette.stops[size_t(state->index) - 1].color;
                color = {float((value >> 16) & 255) / 255, float((value >> 8) & 255) / 255,
                         float(value & 255) / 255, color.a};
                commit();
            },
            L"HEX Color", L"Encoded sRGB color as #RRGGBB. Updates the selected stop only.");
        state->editable.push_back(state->hexField);
        auto swatch = [state, &palette] {
            glm::vec4 color =
                palette.stops.empty() ? glm::vec4(0) : palette.stops[size_t(state->index) - 1].color;
            return RGB(int(std::clamp(color.r, 0.0f, 1.0f) * 255), int(std::clamp(color.g, 0.0f, 1.0f) * 255),
                       int(std::clamp(color.b, 0.0f, 1.0f) * 255));
        };
        state->colorField = window.registerColorButton(
            L"Stop Color", L"Choose...", swatch,
            [state, &palette, &window, commit, swatch] {
                if (palette.stops.empty()) {
                    return;
                }
                static COLORREF custom[16]{};
                CHOOSECOLORW colorDialog{};
                colorDialog.lStructSize = sizeof(colorDialog);
                colorDialog.hwndOwner = window.getWindow();
                colorDialog.lpCustColors = custom;
                colorDialog.rgbResult = swatch();
                colorDialog.Flags = CC_FULLOPEN | CC_RGBINIT;
                if (NativeDialogs::chooseColor(&colorDialog)) {
                    auto &color = palette.stops[size_t(state->index) - 1].color;
                    color = {GetRValue(colorDialog.rgbResult) / 255.0f,
                             GetGValue(colorDialog.rgbResult) / 255.0f,
                             GetBValue(colorDialog.rgbResult) / 255.0f, color.a};
                    commit();
                }
            },
            L"Stop Color", L"Edits the selected stop using the native RGB color picker.");
        state->editable.push_back(state->colorField);
        auto button = [&](const wchar_t *label, std::function<void()> callback) {
            state->editable.push_back(
                window.registerButton(L"Edit Stops", label, std::move(callback), label,
                                      L"Edits the authored stops. Linear / Smoothstep / Smootherstep "
                                      L"correspond to Stop Easing 0 / 1 / 2."));
        };
        button(L"Add Stop", [state, &palette, commit] {
            if (palette.stops.size() < 2 || palette.stops.size() >= 32) {
                return;
            }
            size_t stopIndex = size_t(state->index) - 1;
            float nextPosition = stopIndex + 1 < palette.stops.size() ? palette.stops[stopIndex + 1].position
                                                                      : palette.stops.front().position + 1;
            if (nextPosition - palette.stops[stopIndex].position < 0.00002f) {
                return;
            }
            float newPosition = std::fmod((palette.stops[stopIndex].position + nextPosition) * 0.5f, 1.0f);
            auto color = samplePaletteStops(palette, newPosition);
            palette.stops.push_back({newPosition, color});
            std::sort(palette.stops.begin(), palette.stops.end(),
                      [](const auto &leftStop, const auto &rightStop) {
                          return leftStop.position < rightStop.position;
                      });
            state->index =
                float(std::find_if(palette.stops.begin(), palette.stops.end(),
                                   [newPosition](const auto &stop) { return stop.position == newPosition; }) -
                      palette.stops.begin() + 1);
            commit();
        });
        button(L"Remove Stop", [state, &palette, commit] {
            if (palette.stops.size() > 2) {
                palette.stops.erase(palette.stops.begin() + size_t(state->index) - 1);
                commit();
            }
        });
        button(L"Equal Spacing", [&palette, commit] {
            for (size_t stopIndex = 0; stopIndex < palette.stops.size(); ++stopIndex) {
                palette.stops[stopIndex].position = float(stopIndex) / palette.stops.size();
            }
            commit();
        });
        button(L"Reverse Colors", [&palette, commit] {
            for (size_t stopIndex = 0; stopIndex < palette.stops.size() / 2; ++stopIndex) {
                std::swap(palette.stops[stopIndex].color,
                          palette.stops[palette.stops.size() - 1 - stopIndex].color);
            }
            commit();
        });
        HWND preview = window.registerButton(L"Stop Preview", L"Refresh", refresh, L"Stop Preview",
                                             L"The cyclic authored palette before grading.");
        window.setRowPreview(preview, [&palette](HDC targetDc, const RECT &previewRect) {
            for (int x = previewRect.left; x < previewRect.right; ++x) {
                auto color =
                    palette.stops.empty()
                        ? glm::vec4(0)
                        : samplePaletteStops(palette, float(x - previewRect.left) /
                                                          std::max(1L, previewRect.right - previewRect.left));
                HBRUSH brush = CreateSolidBrush(RGB(int(std::clamp(color.r, 0.0f, 1.0f) * 255),
                                                    int(std::clamp(color.g, 0.0f, 1.0f) * 255),
                                                    int(std::clamp(color.b, 0.0f, 1.0f) * 255)));
                RECT stripe{x, previewRect.top, x + 1, previewRect.bottom};
                FillRect(targetDc, &stripe, brush);
                DeleteObject(brush);
            }
        });
        refresh();
    }
} // namespace merutilm::rff2
