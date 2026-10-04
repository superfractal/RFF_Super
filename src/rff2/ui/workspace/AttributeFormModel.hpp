//
// Modified by GPT-6 on 2026-09-14, 2026-09-21, 2026-09-23, 2026-10-01
//

#pragma once
#include "WorkspaceForm.hpp"
#include "NumericAdjustment.hpp"
#include "../../attr/Attribute.h"
#include "../../attr/Selectable.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <deque>
#include <limits>
#include <memory>
#include <optional>

namespace merutilm::rff2::workspace {
    class AttributeFormModel : public std::enable_shared_from_this<AttributeFormModel> {
        struct Binding {
            FormField field;
            std::function<std::wstring(const Attribute &)> read;
            std::function<bool(Attribute &, const std::wstring &)> write;
        };
        struct Change {
            size_t binding;
            std::wstring before, after;
        };
        struct Edit : std::vector<Change> {
            uint64_t serial = 0;
            std::function<void()> afterApply;
        };
        HistoryOrder order;
        std::function<Attribute &()> attribute;
        std::function<void()> changed;
        std::vector<Binding> bindings;
        std::deque<Edit> undoEntries, redoEntries;
        std::function<void(const Attribute &, Attribute &, const FormDraft &)> normalize;
        std::function<std::wstring(const Attribute &)> validate;
        std::optional<Attribute> previewBefore;

        Edit collectChanges(const Attribute &before, const Attribute &after) const {
            Edit edit;
            for (size_t index = 0; index < bindings.size(); ++index) {
                const auto beforeValue = bindings[index].read(before);
                const auto afterValue = bindings[index].read(after);
                if (beforeValue != afterValue) {
                    edit.push_back({index, beforeValue, afterValue});
                }
            }
            return edit;
        }

        void commitHistory(Edit edit) {
            if (edit.afterApply) {
                edit.afterApply();
            }
            edit.serial = order.commit();
            undoEntries.push_back(std::move(edit));
            if (undoEntries.size() > 128) {
                undoEntries.pop_front();
            }
            redoEntries.clear();
            changed();
        }

        bool restore(bool redo) {
            auto &from = redo ? redoEntries : undoEntries;
            auto &to = redo ? undoEntries : redoEntries;
            if (from.empty() || (redo && !order.validRedo())) {
                return false;
            }
            for (const auto &change : from.back()) {
                if (bindings[change.binding].read(attribute()) != (redo ? change.before : change.after)) {
                    undoEntries.clear();
                    redoEntries.clear();
                    return false;
                }
            }
            auto target = attribute();
            for (const auto &change : from.back()) {
                if (!bindings[change.binding].write(target, redo ? change.after : change.before)) {
                    return false;
                }
            }
            if (validate && !validate(target).empty()) {
                return false;
            }
            if (!redo) {
                order.prepareUndo(redoEntries);
            }
            attribute() = std::move(target);
            if (from.back().afterApply) {
                from.back().afterApply();
            }
            to.push_back(std::move(from.back()));
            from.pop_back();
            changed();
            return true;
        }

      public:
        AttributeFormModel(std::function<Attribute &()> getter, std::function<void()> update)
            : attribute(std::move(getter)), changed(std::move(update)) {}
        static std::optional<std::string> ascii(std::wstring_view value) {
            std::string text;
            for (wchar_t c : value) {
                if (c > 127) {
                    return {};
                }
                text.push_back(char(c));
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
        template <class T> static bool parse(std::wstring_view text, T &value) {
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
                     std::remove_cvref_t<decltype(get(std::declval<Attribute &>()))> minimum,
                     std::remove_cvref_t<decltype(get(std::declval<Attribute &>()))> maximum) {
            using T = std::remove_cvref_t<decltype(get(std::declval<Attribute &>()))>;
            bindings.push_back({{std::move(id), group, std::move(label), std::move(hint), {}, {}},
                                [get](const Attribute &a) { return number(get(a)); },
                                [get, minimum, maximum](Attribute &a, const std::wstring &text) {
                                    T value{};
                                    if (!parse(text, value) || value < minimum || value > maximum) {
                                        return false;
                                    }
                                    get(a) = value;
                                    return true;
                                }});
            bindings.back().field.validate = rangeValidation<T>(minimum, maximum);
            bindings.back().field.nudge = numericAdjustment(minimum, maximum);
            auto &field = bindings.back().field;
            if constexpr (std::is_integral_v<T>) {
                if (minimum < maximum && double(minimum) >= -10000 && double(maximum) <= 10000) {
                    field.sliderValue = [=](double fraction) {
                        return number(static_cast<T>(std::round(double(minimum) + std::clamp(fraction, 0.0, 1.0) * (double(maximum) - minimum))));
                    };
                    field.sliderPosition = [=](const std::wstring &text) {
                        T value{};
                        if (!parse(text, value)) return 0.0;
                        return (double(std::clamp(value, minimum, maximum)) - minimum) / (double(maximum) - minimum);
                    };
                }
            } else if constexpr (std::is_floating_point_v<T>) {
                const bool fractional = field.id.ends_with(".repeatU") || field.id.ends_with(".repeatV") ||
                    field.id.ends_with(".scaleU") || field.id.ends_with(".scaleV") || field.id.ends_with(".size") ||
                    field.id == "animation.flowScale" || field.id == "slope.depth" || field.id == "slope.macroRadius" ||
                    field.id == "slope.reliefDepth" || field.id == "slope.aoRadius" || field.id == "slope.glossRelief" ||
                    field.id == "slope.glossBands" || field.id == "slope.filmThickness" || field.id.starts_with("studio.") ||
                    field.id.starts_with("effect.");
                double step = field.id == "animation.speed" ? 1 : field.id == "animation.flowAmount" ? 10 : 0;
                if (field.id.starts_with("color.") || field.id.starts_with("bloom.") || field.id == "export.exposure") step = .01;
                if (field.id == "slope.brightness" || field.id == "slope.gamma") step = .05;
                if (field.id == "stripe.offset" || field.id == "stripe.animationSpeed" || field.id == "fog.rimBlur" ||
                    field.id == "fog.focusBlur" || field.id == "fog.rimMaskBoost") step = 1;
                if (field.id == "export.headroom") step = .1;
                if (field.id == "export.peak" || field.id == "export.mfrPeak") step = 100;
                const bool decade = field.id == "stripe.firstInterval" || field.id == "stripe.secondInterval" ||
                                    field.id.ends_with(".periodIterations");
                const double zeroStep = field.id == "slope.depth" ? 1e-6 :
                    ((field.id.starts_with("texture.") || field.id.starts_with("pattern.") || field.id.starts_with("warp.")) &&
                     (field.id.ends_with(".scaleU") || field.id.ends_with(".scaleV"))) ? .1 : 0;
                field.nudge = numericAdjustment(minimum, maximum, step, fractional, decade, zeroStep);
                if (double(maximum) - double(minimum) <= 1.1e18 && minimum >= -1e18 && minimum < maximum) {
                    const bool logarithmic = minimum > 0 && double(maximum) / minimum >= 100;
                    const bool whole = !fractional && step == 0 && !decade && double(maximum) - minimum > 10;
                    auto base = std::make_shared<double>(minimum);
                    auto initialized = std::make_shared<bool>(false);
                    const double topBase = logarithmic ? std::max(double(minimum), std::pow(10.0, std::ceil(std::log10(double(maximum)) - 1e-9) - 1)) : 0;
                    auto span = [=] { return std::max(1e-6, std::log10(std::min(double(maximum) / *base, 10.0))); };
                    if (logarithmic) field.sliderReleased = [=](double fraction) {
                        if (fraction >= 1) *base = std::min(*base * 10, topBase);
                        else if (fraction <= 0) *base = std::max(*base / 10, double(minimum));
                    };
                    field.sliderValue = [=](double fraction) {
                        fraction = std::clamp(fraction, 0.0, 1.0);
                        double value = logarithmic ? *base * std::pow(10.0, fraction * span())
                                                   : double(minimum) + fraction * (double(maximum) - minimum);
                        if (whole) value = std::round(value);
                        return number(static_cast<T>(std::clamp(value, double(minimum), double(maximum))));
                    };
                    field.sliderPosition = [=](const std::wstring &text) {
                        T value{};
                        if (!parse(text, value)) return 0.0;
                        value = std::clamp(value, minimum, maximum);
                        if (logarithmic) {
                            if (!*initialized || value < *base * (1 - 1e-7) || value > std::min(double(maximum), *base * 10) * (1 + 1e-7)) {
                                *base = std::clamp(std::pow(10.0, std::floor(std::log10(double(value)))), double(minimum), topBase);
                                *initialized = true;
                            }
                            return std::log10(double(value) / *base) / span();
                        }
                        return (double(value) - minimum) / (double(maximum) - minimum);
                    };
                }
            }
        }
        template <class T> static auto rangeValidation(T minimum, T maximum) {
            return [minimum, maximum](const std::wstring &text) -> std::wstring {
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
        }
        template <class Getter>
        void choice(std::string id, int group, std::wstring label, std::wstring hint, Getter get) {
            using T = std::remove_cvref_t<decltype(get(std::declval<Attribute &>()))>;
            const auto values = Selectable::values<T>();
            std::vector<FormChoice> choices;
            for (T value : values) {
                if constexpr (std::is_same_v<T, bool>) {
                    choices.push_back({number(int(value)), value ? L"On" : L"Off"});
                } else {
                    choices.push_back({number(int(value)), Selectable::toString(value)});
                }
            }
            bindings.push_back(
                {{std::move(id), group, std::move(label), std::move(hint), {}, std::move(choices)},
                 [get](const Attribute &a) { return number(int(get(a))); },
                 [get, values](Attribute &a, const std::wstring &text) {
                     int value;
                     if (!parse(text, value)) {
                         return false;
                     }
                     for (T candidate : values) {
                         if (int(candidate) == value) {
                             get(a) = candidate;
                             return true;
                         }
                     }
                     return false;
                 }});
        }
        void text(std::string id, int group, std::wstring label, std::wstring hint,
                  std::function<std::wstring(const Attribute &)> read,
                  std::function<bool(Attribute &, const std::wstring &)> write,
                  FormField::Editor editor = FormField::Editor::TEXT) {
            bindings.push_back({{std::move(id), group, std::move(label), std::move(hint), {}, {}, editor},
                                std::move(read),
                                std::move(write)});
        }
        void swatches(std::function<std::vector<std::pair<std::wstring, uint32_t>>(const std::wstring &)> colors,
                      std::function<std::wstring(const std::wstring &, size_t)> remove) {
            bindings.back().field.swatches = std::move(colors);
            bindings.back().field.removeSwatch = std::move(remove);
        }
        static std::wstring colorText(const glm::vec4 &color) {
            return number(color.r) + L", " + number(color.g) + L", " + number(color.b) + L", " +
                   number(color.a);
        }
        static std::wstring colorTextRgb(const glm::vec4 &color) {
            return number(color.r) + L", " + number(color.g) + L", " + number(color.b);
        }
        static float colorByte(uint8_t value) {
            // Constant evaluation keeps byte normalization identical when runtime fast-math is enabled.
            static constexpr auto channels = [] {
                std::array<float, 256> result{};
                for (size_t i = 0; i < result.size(); ++i) {
                    result[i] = float(i) / 255.f;
                }
                return result;
            }();
            return channels[value];
        }
        static bool parseColor(std::wstring_view text, glm::vec4 &color) {
            if (text.size() == 7 && text.front() == L'#') {
                unsigned value = 0;
                for (wchar_t c : text.substr(1)) {
                    unsigned digit = c >= L'0' && c <= L'9'   ? c - L'0'
                                     : c >= L'a' && c <= L'f' ? c - L'a' + 10
                                     : c >= L'A' && c <= L'F' ? c - L'A' + 10
                                                              : 16;
                    if (digit == 16) {
                        return false;
                    }
                    value = value * 16 + digit;
                }
                color = {colorByte(uint8_t(value >> 16)), colorByte(uint8_t(value >> 8)),
                         colorByte(uint8_t(value)), color.a};
                return true;
            }
            glm::vec4 value;
            for (int i = 0; i < 4; ++i) {
                const auto end = text.find(L',');
                if ((i < 3) == (end == std::wstring_view::npos)) {
                    return false;
                }
                if (!parse(text.substr(0, end), value[i]) || value[i] < 0 || value[i] > 1) {
                    return false;
                }
                if (i < 3) {
                    text.remove_prefix(end + 1);
                }
            }
            color = value;
            return true;
        }
        static bool parseColorRgb(std::wstring_view value, glm::vec4 &color) {
            if (value.size() == 7 && value.front() == L'#') {
                return parseColor(value, color);
            }
            glm::vec4 parsed = color;
            for (int i = 0; i < 3; ++i) {
                const auto end = value.find(L',');
                if ((i < 2) == (end == std::wstring_view::npos) ||
                    !parse(value.substr(0, end), parsed[i]) || parsed[i] < 0 || parsed[i] > 1) {
                    return false;
                }
                if (i < 2) {
                    value.remove_prefix(end + 1);
                }
            }
            color = parsed;
            return true;
        }
        template <class Getter> void color(std::string id, int group, std::wstring label, Getter get) {
            text(
                std::move(id), group, std::move(label),
                L"Click the color swatch to choose a color, then Apply & Render.",
                [get](const Attribute &a) { return colorText(get(a)); },
                [get](Attribute &a, const std::wstring &text) { return parseColor(text, get(a)); },
                FormField::Editor::COLOR);
        }
        template <class Getter> void colorRgb(std::string id, int group, std::wstring label, Getter get) {
            text(
                std::move(id), group, std::move(label),
                L"Click the color swatch to choose a color, then Apply & Render.",
                [get](const Attribute &a) { return colorTextRgb(get(a)); },
                [get](Attribute &a, const std::wstring &value) { return parseColorRgb(value, get(a)); },
                FormField::Editor::RGB_COLOR);
        }
        bool edit(const std::function<void(Attribute &)> &action,
                  std::function<void()> afterApply = {}) {
            const auto before = attribute();
            auto after = before;
            action(after);
            if (validate && !validate(after).empty()) {
                return false;
            }
            Edit edit = collectChanges(before, after);
            if (edit.empty()) {
                return false;
            }
            attribute() = std::move(after);
            edit.afterApply = std::move(afterApply);
            commitHistory(std::move(edit));
            return true;
        }
        void setNormalizer(std::function<void(const Attribute &, Attribute &, const FormDraft &)> action) {
            normalize = std::move(action);
        }
        void setValidator(std::function<std::wstring(const Attribute &)> action) {
            validate = std::move(action);
        }
        void recordExternal(const FormDraft &before) {
            Edit edit;
            for (size_t i = 0; i < bindings.size(); ++i) {
                const auto found = before.find(bindings[i].field.id);
                if (found == before.end()) {
                    continue;
                }
                const auto after = bindings[i].read(attribute());
                if (found->second != after) {
                    edit.push_back({i, found->second, after});
                }
            }
            if (edit.empty()) {
                return;
            }
            commitHistory(std::move(edit));
        }
        std::wstring apply(const FormDraft &draft, bool preview = false) {
            const auto before = attribute();
            auto after = before;
            for (const auto &[id, value] : draft) {
                const auto found = std::find_if(bindings.begin(), bindings.end(),
                                                [&](const auto &b) { return b.field.id == id; });
                if (found == bindings.end()) {
                    return L"Unknown setting.";
                }
                if (!found->write(after, value)) {
                    return found->field.label + L": " + found->field.hint;
                }
            }
            if (normalize) {
                normalize(before, after, draft);
            }
            if (validate) {
                const auto error = validate(after);
                if (!error.empty()) {
                    return error;
                }
            }
            Edit edit = collectChanges(before, after);
            if (edit.empty()) {
                return L"";
            }
            attribute() = std::move(after);
            if (preview) changed();
            else commitHistory(std::move(edit));
            return L"";
        }
        std::wstring preview(const FormDraft &draft) {
            if (!previewBefore) previewBefore = attribute();
            return apply(draft, true);
        }
        void cancelPreview() {
            if (!previewBefore) return;
            const auto changes = collectChanges(*previewBefore, attribute());
            for (const auto &change : changes) bindings[change.binding].write(attribute(), change.before);
            previewBefore.reset();
            changed();
        }
        WorkspaceForm form(std::wstring title, std::vector<std::wstring> groups) {
            auto self = shared_from_this();
            WorkspaceForm result;
            result.title = std::move(title);
            result.groups = std::move(groups);
            for (size_t i = 0; i < bindings.size(); ++i) {
                auto field = bindings[i].field;
                field.read = [self, i] { return self->bindings[i].read(self->attribute()); };
                if (!field.validate) {
                    field.validate = [self, i](const std::wstring &text) {
                        auto candidate = self->attribute();
                        const auto &binding = self->bindings[i];
                        return binding.write(candidate, text) ? std::wstring{}
                                                              : L"Check this value. " + binding.field.hint;
                    };
                }
                result.fields.push_back(std::move(field));
            }
            result.apply = [self](const FormDraft &draft) { return self->apply(draft); };
            result.preview = [self](const FormDraft &draft) { return self->preview(draft); };
            result.cancelPreview = [self] { self->cancelPreview(); };
            result.canUndo = [self] { return !self->undoEntries.empty(); };
            result.canRedo = [self] { return self->order.validRedo() && !self->redoEntries.empty(); };
            result.undo = [self] { return self->restore(false); };
            result.redo = [self] { return self->restore(true); };
            result.clearHistory = [self] {
                self->undoEntries.clear();
                self->redoEntries.clear();
            };
            result.bindHistory = [self](auto domain) {
                self->undoEntries.clear();
                self->redoEntries.clear();
                self->order.bind(std::move(domain));
            };
            result.undoOrder = [self] {
                return self->undoEntries.empty() ? 0 : self->undoEntries.back().serial;
            };
            result.redoOrder = [self] {
                return !self->order.validRedo() || self->redoEntries.empty()
                           ? 0
                           : self->redoEntries.back().serial;
            };
            return result;
        }
    };
} // namespace merutilm::rff2::workspace
