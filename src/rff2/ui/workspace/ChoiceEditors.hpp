//
// Created by Opus 5.5 on 2026-10-04
//

#pragma once
#include "WorkspaceForm.hpp"
#include "../../video/TimelineSchedule.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>
#include <string>
#include <utility>

namespace merutilm::rff2::workspace::ChoiceEditors {
    inline bool labelsAre(const FormField &field, std::initializer_list<const wchar_t *> labels) {
        return field.choices.size() == labels.size() &&
               std::equal(labels.begin(), labels.end(), field.choices.begin(),
                          [](const wchar_t *label, const FormChoice &choice) { return choice.label == label; });
    }

    // The choice values are their own positions 0, 1, 2 ..., which the grid and the toggles rely on.
    inline bool valuesArePositions(const FormField &field) {
        for (size_t i = 0; i < field.choices.size(); ++i) {
            if (field.choices[i].value != std::to_wstring(i)) {
                return false;
            }
        }
        return true;
    }

    // An Off/On pair in either order; a bool setting lists On first, so the pair is put in Off, On order.
    inline bool makeOffOnPair(FormField &field) {
        const auto is = [](const FormChoice &choice, const wchar_t *value, const wchar_t *label) {
            return choice.value == value && choice.label == label;
        };
        if (field.choices.size() != 2) {
            return false;
        }
        if (is(field.choices[0], L"1", L"On") && is(field.choices[1], L"0", L"Off")) {
            std::swap(field.choices[0], field.choices[1]);
        }
        return is(field.choices[0], L"0", L"Off") && is(field.choices[1], L"1", L"On");
    }

    // Short fixed lists fit side by side; lists built from the document, such as audio clips, stay drop-downs.
    inline bool shortFixedList(const FormField &field) {
        if (field.choices.size() < 2 || field.choices.size() > 4 || field.id.ends_with(".selection")) {
            return false;
        }
        return std::ranges::all_of(field.choices, [](const FormChoice &choice) { return choice.label.size() <= 24; });
    }

    // Key interpolation through three sample keys, evaluated by the timeline itself so each icon is exact.
    inline double interpolationCurve(size_t choice, double x) {
        constexpr std::array<VidKeyInterpolation, 4> modes{VidKeyInterpolation::STEP, VidKeyInterpolation::LINEAR,
                                                           VidKeyInterpolation::SMOOTH, VidKeyInterpolation::CUBIC};
        const VidKeyInterpolation mode = modes[std::min(choice, modes.size() - 1)];
        VidTimelineTrack track{};
        track.enabled = true;
        track.keys = {{2.f, .15f, {}, mode}, {1.f, .85f, {}, mode}, {0.f, .35f, {}, mode}};
        return TimelineSchedule::evaluateTrack(track, float(2 - 2 * x), 0.f);
    }

    // Mirrors coloring_curve in vk_2_map_iter_stripe.comp over three cycles, scaled to end at 1.
    // Smoothstep and Smootherstep are the eases recorded under Ken Perlin in NOTICE.
    inline double iterationColoringCurve(size_t choice, double x) {
        const auto eased = [](double ratio, bool smoother) {
            const double whole = std::floor(ratio), u = ratio - whole;
            return whole + (smoother ? u * u * u * (u * (6 * u - 15) + 10) : u * u * (3 - 2 * u));
        };
        const auto curve = [&](double ratio) {
            switch (choice) {
            case 1: return std::sqrt(ratio);
            case 2: return std::cbrt(ratio);
            case 3: return std::log1p(ratio);
            case 4: return std::log1p(std::log1p(ratio));
            case 5: return eased(ratio, false);
            case 6: return eased(ratio, true);
            default: return ratio;
            }
        };
        constexpr double cycles = 3;
        return curve(x * cycles) / curve(cycles);
    }

    // Gives every drop-down the form left as TEXT the editor that suits its choices.
    inline void assign(std::vector<FormField> &fields) {
        using Editor = FormField::Editor;
        for (auto &field : fields) {
            if (field.editor != Editor::TEXT || field.choices.empty()) {
                continue;
            }
            if (makeOffOnPair(field)) {
                field.editor = Editor::CHECKBOX;
            } else if (labelsAre(field, {L"Top Left", L"Top Center", L"Top Right", L"Middle Left", L"Center",
                                         L"Middle Right", L"Bottom Left", L"Bottom Center", L"Bottom Right"}) &&
                       valuesArePositions(field)) {
                field.editor = Editor::ANCHOR_GRID;
            } else if (labelsAre(field, {L"Regular", L"Bold", L"Italic", L"Bold Italic"}) &&
                       valuesArePositions(field)) {
                field.editor = Editor::STYLE_TOGGLES;
            } else if (labelsAre(field, {L"Linear", L"Square root", L"Cube root", L"Log", L"LogLog", L"Smoothstep",
                                         L"Smootherstep"})) {
                field.editor = Editor::CURVE_TILES;
                field.curve = iterationColoringCurve;
                // Smootherstep is not offered as a tile; a file that uses it still renders with it.
                field.choices.pop_back();
            } else if (labelsAre(field, {L"Step", L"Linear", L"Smooth", L"Cubic"})) {
                field.editor = Editor::SEGMENTS;
                field.curve = interpolationCurve;
            } else if (shortFixedList(field)) {
                field.editor = Editor::SEGMENTS;
            }
        }
    }
} // namespace merutilm::rff2::workspace::ChoiceEditors
