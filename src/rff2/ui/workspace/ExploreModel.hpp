//
// Modified by GPT-6 on 2026-09-14, 2026-09-22, 2026-09-24
//

#pragma once
#include "WorkspaceForm.hpp"
#include "../../attr/FractalAttribute.h"
#include "../../attr/NumericSettingLimits.hpp"
#include "../../attr/Selectable.h"
#include "../../formula/ExpressionParser.h"
#include "../../formula/Perturbator.h"
#include <charconv>
#include <cmath>
#include <deque>
#include <limits>
#include <memory>
#include <optional>

namespace merutilm::rff2::workspace {
    class ExploreModel : public std::enable_shared_from_this<ExploreModel> {
        struct Binding {
            FormField field;
            std::function<std::wstring(const FractalAttribute &)> read;
            std::function<bool(FractalAttribute &, const std::wstring &)> write;
        };
        struct Edit {
            FractalAttribute before, after;
            uint64_t serial;
        };
        HistoryOrder order;
        std::function<FractalAttribute &()> attribute;
        std::function<void()> recompute;
        FractalAttribute defaults;
        std::vector<Binding> bindings;
        std::deque<Edit> past, future;

        static std::optional<std::string> ascii(std::wstring_view value) {
            std::string text;
            for (wchar_t character : value) {
                if (character > 127) {
                    return {};
                }
                text.push_back(char(character));
            }
            const auto first = text.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) {
                return std::string();
            }
            return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
        }
        template <class T> static std::wstring number(T value) {
            char buffer[96];
            const auto result = [&] {
                if constexpr (std::is_floating_point_v<T>) {
                    return std::to_chars(buffer, buffer + 96, value, std::chars_format::general);
                } else {
                    return std::to_chars(buffer, buffer + 96, value);
                }
            }();
            return {buffer, result.ptr};
        }
        template <class T> static bool parse(const std::wstring &text, T &value) {
            auto narrow = ascii(text);
            if (!narrow || narrow->empty()) {
                return false;
            }
            std::string_view token = *narrow;
            if (token.front() == '+') {
                token.remove_prefix(1);
            }
            for (char c : token) {
                if ((c < '0' || c > '9') && c != '-' && c != '+' && c != '.' && c != 'e' && c != 'E') {
                    return false;
                }
            }
            const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
            if (result.ec != std::errc() || result.ptr != token.data() + token.size()) {
                return false;
            }
            if constexpr (std::is_floating_point_v<T>) {
                return std::isfinite(value);
            }
            return true;
        }
        template <class Getter>
        void numeric(std::string id, int group, std::wstring label, std::wstring hint, Getter get,
                     std::remove_reference_t<decltype(get(std::declval<FractalAttribute &>()))> minimum,
                     std::remove_reference_t<decltype(get(std::declval<FractalAttribute &>()))> maximum) {
            using T = std::remove_reference_t<decltype(get(std::declval<FractalAttribute &>()))>;
            Binding binding;
            binding.field = {std::move(id), group, std::move(label), std::move(hint), {}, {}};
            binding.read = [get](const FractalAttribute &value) { return number(get(value)); };
            binding.write = [get, minimum, maximum](FractalAttribute &target, const std::wstring &text) {
                T value{};
                if (!parse(text, value) || value < minimum || value > maximum) {
                    return false;
                }
                get(target) = value;
                return true;
            };
            binding.field.validate = [minimum, maximum](const std::wstring &text) -> std::wstring {
                T value{};
                if (parse(text, value) && value >= minimum && value <= maximum) {
                    return {};
                }
                if constexpr (std::is_floating_point_v<T>) {
                    if (minimum == std::numeric_limits<T>::lowest() &&
                        maximum == std::numeric_limits<T>::max()) {
                        return L"Enter a finite number.";
                    }
                }
                return std::wstring(std::is_integral_v<T> ? L"Enter a whole number from "
                                                          : L"Enter a finite number from ") +
                       number(minimum) + L" to " + number(maximum) + L".";
            };
            bindings.push_back(std::move(binding));
        }
        template <class Getter>
        void choice(std::string id, int group, std::wstring label, std::wstring hint, Getter get) {
            using T = std::remove_cvref_t<decltype(get(std::declval<FractalAttribute &>()))>;
            Binding binding;
            binding.field = {std::move(id), group, std::move(label), std::move(hint), {}, {}};
            const auto values = Selectable::values<T>();
            for (T value : values) {
                binding.field.choices.push_back({number(int(value)), Selectable::toString(value)});
            }
            binding.read = [get](const FractalAttribute &value) { return number(int(get(value))); };
            binding.write = [get, values](FractalAttribute &target, const std::wstring &text) {
                int value;
                if (!parse(text, value)) {
                    return false;
                }
                for (T candidate : values) {
                    if (int(candidate) == value) {
                        get(target) = candidate;
                        return true;
                    }
                }
                return false;
            };
            bindings.push_back(std::move(binding));
        }
        bool equal(const FractalAttribute &a, const FractalAttribute &b, bool ignoreDerived = false) const {
            for (const auto &binding : bindings) {
                if (ignoreDerived && a.autoMaxIteration && b.autoMaxIteration &&
                    binding.field.id == "explore.maxIterations") {
                    continue;
                }
                if (binding.read(a) != binding.read(b)) {
                    return false;
                }
            }
            return true;
        }
        bool commit(FractalAttribute before, FractalAttribute after) {
            if (equal(before, after)) {
                return false;
            }
            attribute() = after;
            past.push_back({std::move(before), std::move(after), order.commit()});
            if (past.size() > 128) {
                past.pop_front();
            }
            future.clear();
            recompute();
            return true;
        }

      public:
        ExploreModel(std::function<FractalAttribute &()> getter, std::function<void()> render)
            : attribute(std::move(getter)), recompute(std::move(render)), defaults(attribute()) {
            const auto real = [](const FractalAttribute &value) {
                const auto coordinateText = value.center.real.to_string();
                return std::wstring(coordinateText.begin(), coordinateText.end());
            };
            const auto imag = [](const FractalAttribute &value) {
                const auto coordinateText = value.center.imag.to_string();
                return std::wstring(coordinateText.begin(), coordinateText.end());
            };
            bindings.push_back(
                {{"explore.real", 0, L"Real", L"High-precision center coordinate.", {}, {}}, real, {}});
            bindings.push_back(
                {{"explore.imag", 0, L"Imaginary", L"High-precision center coordinate.", {}, {}}, imag, {}});
            numeric(
                "explore.zoom", 0, L"Log Zoom (e)", L"Natural-log magnification. 0 to 16777216.",
                [](auto &a) -> auto & { return a.logZoom; }, NumericSettingLimits::logZoom.minimum, NumericSettingLimits::logZoom.maximum);
            numeric(
                "explore.rotation", 0, L"Rotation", L"Degrees. Also controls panorama yaw.",
                [](auto &a) -> auto & { return a.rotation; }, -std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max());
            choice("explore.autoIterations", 1, L"Automatic Iterations",
                   L"Mandelbrot only. Period detection stays automatic.",
                   [](auto &a) -> auto & { return a.autoMaxIteration; });
            numeric(
                "explore.maxIterations", 1, L"Max Iteration", L"Used when Automatic Iterations is off.",
                [](auto &a) -> auto & { return a.maxIteration; }, uint64_t(1),
                std::numeric_limits<uint64_t>::max());
            numeric(
                "explore.autoMultiplier", 1, L"Auto Iteration Multiplier",
                L"Used when Automatic Iterations is on.",
                [](auto &a) -> auto & { return a.autoIterationMultiplier; }, uint16_t(1),
                std::numeric_limits<uint16_t>::max());
            numeric(
                "explore.bailout", 1, L"Bailout", L"Escape radius, 2 to 1e38.",
                [](auto &a) -> auto & { return a.bailout; }, 2.f, 1e38f);
            choice("explore.decimalize", 1, L"Decimalize Iteration", L"Fractional iteration coloring.",
                   [](auto &a) -> auto & { return a.decimalizeIterationMethod; });
            choice("explore.absolute", 1, L"Absolute Iteration Mode", L"Uses absolute iteration counts.",
                   [](auto &a) -> auto & { return a.absoluteIterationMode; });
            choice("explore.reuse", 2, L"Reuse Reference",
                   L"Disable when the saved reference belongs to another location.",
                   [](auto &a) -> auto & { return a.reuseReferenceMethod; });
            numeric(
                "explore.compressionCriteria", 2, L"Compression Criteria",
                L"Minimum reference batch; 0 disables compression.",
                [](auto &a) -> auto & { return a.referenceCompAttribute.compressCriteria; }, uint32_t(0),
                std::numeric_limits<uint32_t>::max());
            numeric(
                "explore.compressionThreshold", 2, L"Compression Threshold",
                L"Negative power of ten; 0 disables compression.",
                [](auto &a) -> auto & { return a.referenceCompAttribute.compressionThresholdPower; },
                uint8_t(0), std::numeric_limits<uint8_t>::max());
            choice("explore.noNormalization", 2, L"Disable Normalization",
                   L"Advanced reference-compression option.",
                   [](auto &a) -> auto & { return a.referenceCompAttribute.noCompressorNormalization; });
            numeric(
                "explore.minSkip", 3, L"Min Skip Reference", L"MPA follows periodic structure; minimum 4.",
                [](auto &a) -> auto & { return a.mpaAttribute.minSkipReference; }, uint16_t(4),
                std::numeric_limits<uint16_t>::max());
            numeric(
                "explore.maxMultiplier", 3, L"Max Multiplier Between Levels",
                L"Ratio between adjacent period levels, 1 to 255.",
                [](auto &a) -> auto & { return a.mpaAttribute.maxMultiplierBetweenLevel; }, uint8_t(1),
                std::numeric_limits<uint8_t>::max());
            numeric(
                "explore.precision", 3, L"Precision Level", L"-15 to -3. Lower values favor accuracy.",
                [](auto &a) -> auto & { return a.mpaAttribute.epsilonPower; }, -15.f, -3.f);
            choice("explore.mpaSelection", 3, L"Selection Method", L"MPA period-level selection strategy.",
                   [](auto &a) -> auto & { return a.mpaAttribute.mpaSelectionMethod; });
            choice("explore.mpaCompression", 3, L"Compression Method", L"MPA table compression strategy.",
                   [](auto &a) -> auto & { return a.mpaAttribute.mpaCompressionMethod; });
            choice("explore.formulaType", 4, L"Formula Type",
                   L"Changing the formula resets the view; explicit location edits take precedence.",
                   [](auto &a) -> auto & { return a.formulaType; });
            bindings.push_back({{"explore.formula",
                                 4,
                                 L"Custom Formula",
                                 L"Examples: z^3+c; conj(z)^2+c. Custom formulas use manual iterations.",
                                 {},
                                 {}},
                                [](const FractalAttribute &a) {
                                    return std::wstring(a.customFormula.begin(), a.customFormula.end());
                                },
                                [](FractalAttribute &a, const std::wstring &text) {
                                    auto value = ascii(text);
                                    if (!value || value->empty()) {
                                        return false;
                                    }
                                    ExpressionParser parser;
                                    if (!parser.parse(*value)) {
                                        return false;
                                    }
                                    a.customFormula = *value;
                                    return true;
                                }});
            choice("explore.projection", 5, L"Projection", L"Planar or a 360-degree view.",
                   [](auto &a) -> auto & { return a.projectionMethod; });
            choice("explore.panoramaLayout", 5, L"Layout", L"Ground and Sky or Full Sphere.",
                   [](auto &a) -> auto & { return a.panoramaLayout; });
            numeric(
                "explore.pitch", 5, L"Pitch", L"-90 to 90 degrees. Used by the 360 Camera.",
                [](auto &a) -> auto & { return a.panoramaPitch; }, -90.f, 90.f);
            numeric(
                "explore.fov", 5, L"Field of View", L"1 to 179 degrees. Used by the 360 Camera.",
                [](auto &a) -> auto & { return a.panoramaFov; }, 1.f, 179.f);
            numeric(
                "explore.panoramaRange", 5, L"Panorama Range", L"Furthest radius, log10 scale: 0 to 6.",
                [](auto &a) -> auto & { return a.panoramaRange; }, 0.f, 6.f);
        }

        std::wstring apply(const FormDraft &draft) {
            const auto before = attribute();
            auto after = before;
            for (const auto &[id, value] : draft) {
                if (std::none_of(bindings.begin(), bindings.end(),
                                 [&](const auto &b) { return b.field.id == id; })) {
                    return L"Unknown setting.";
                }
            }
            for (const auto &binding : bindings) {
                if (binding.field.group == 0) {
                    continue;
                }
                if (const auto found = draft.find(binding.field.id);
                    found != draft.end() && !binding.write(after, found->second)) {
                    return binding.field.label + L": " + binding.field.hint;
                }
            }
            const bool formulaChanged =
                before.formulaType != after.formulaType || (after.formulaType == FractalFormulaType::CUSTOM &&
                                                            before.customFormula != after.customFormula);
            if (formulaChanged) {
                after.logZoom = defaults.logZoom;
                after.rotation = defaults.rotation;
                after.center = after.formulaType == FractalFormulaType::CUSTOM
                                   ? fp_complex("0", "0", Perturbator::logZoomToExp10(after.logZoom))
                                   : defaults.center;
            }
            for (const auto &binding : bindings) {
                if (binding.field.group != 0 || !binding.write) {
                    continue;
                }
                if (const auto found = draft.find(binding.field.id);
                    found != draft.end() && !binding.write(after, found->second)) {
                    return binding.field.label + L": " + binding.field.hint;
                }
            }
            const auto real = draft.find("explore.real"), imag = draft.find("explore.imag");
            if (real != draft.end() || imag != draft.end()) {
                const auto r = real == draft.end() ? std::optional<std::string>(after.center.real.to_string())
                                                   : ascii(real->second);
                const auto i = imag == draft.end() ? std::optional<std::string>(after.center.imag.to_string())
                                                   : ascii(imag->second);
                if (!r || !i || !fp_decimal_calculator::isValidString(*r) ||
                    !fp_decimal_calculator::isValidString(*i)) {
                    return L"Enter valid real and imaginary coordinates.";
                }
                after.center = fp_complex(*r, *i, Perturbator::logZoomToExp10(after.logZoom));
            }
            if (after.formulaType == FractalFormulaType::CUSTOM) {
                const auto automatic = draft.find("explore.autoIterations");
                if (automatic != draft.end() && automatic->second != L"0") {
                    return L"Custom formulas require Automatic Iterations to be off.";
                }
                after.autoMaxIteration = false;
            }
            commit(before, std::move(after));
            return L"";
        }
        bool undo() {
            if (past.empty()) {
                return false;
            }
            if (!equal(attribute(), past.back().after, true)) {
                past.clear();
                future.clear();
                return false;
            }
            order.prepareUndo(future);
            attribute() = past.back().before;
            future.push_back(std::move(past.back()));
            past.pop_back();
            recompute();
            return true;
        }
        bool redo() {
            if (future.empty() || !order.validRedo()) {
                return false;
            }
            if (!equal(attribute(), future.back().before, true)) {
                past.clear();
                future.clear();
                return false;
            }
            attribute() = future.back().after;
            past.push_back(std::move(future.back()));
            future.pop_back();
            recompute();
            return true;
        }
        WorkspaceForm form() {
            auto self = shared_from_this();
            WorkspaceForm result;
            result.title = L"Explore";
            result.groups = {L"Location", L"Iterations", L"Reference", L"MP-Approximation",
                             L"Formula",  L"Projection", L"Tools"};
            for (size_t i = 0; i < bindings.size(); ++i) {
                auto field = bindings[i].field;
                field.read = [self, i] { return self->bindings[i].read(self->attribute()); };
                if (!field.validate) {
                    field.validate = [self, i](const std::wstring &text) -> std::wstring {
                        const auto &binding = self->bindings[i];
                        if (!binding.write) {
                            const auto value = ascii(text);
                            return value && fp_decimal_calculator::isValidString(*value)
                                       ? L""
                                       : L"Enter a valid decimal coordinate.";
                        }
                        auto candidate = self->attribute();
                        return binding.write(candidate, text) ? L""
                                                              : L"Check this value. " + binding.field.hint;
                    };
                }
                result.fields.push_back(std::move(field));
            }
            result.apply = [self](const FormDraft &draft) { return self->apply(draft); };
            result.canUndo = [self] { return !self->past.empty(); };
            result.canRedo = [self] { return self->order.validRedo() && !self->future.empty(); };
            result.undo = [self] { return self->undo(); };
            result.redo = [self] { return self->redo(); };
            result.clearHistory = [self] {
                self->past.clear();
                self->future.clear();
            };
            result.bindHistory = [self](auto domain) {
                self->past.clear();
                self->future.clear();
                self->order.bind(std::move(domain));
            };
            result.undoOrder = [self] { return self->past.empty() ? 0 : self->past.back().serial; };
            result.redoOrder = [self] {
                return !self->order.validRedo() || self->future.empty() ? 0 : self->future.back().serial;
            };
            return result;
        }
        void resetView() {
            const auto before = attribute();
            auto after = before;
            after.logZoom = defaults.logZoom;
            after.rotation = defaults.rotation;
            after.center = after.formulaType == FractalFormulaType::CUSTOM
                               ? fp_complex("0", "0", Perturbator::logZoomToExp10(after.logZoom))
                               : defaults.center;
            commit(before, std::move(after));
        }
    };
}
