//
// Modified by GPT-6 on 2026-09-14, 2026-09-23
//

#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "AttributeFormModel.hpp"
#include "FormValues.hpp"
#include "../Callback.hpp"

namespace merutilm::rff2::workspace {
    template<class Getter>
    class AttributeFormSection {
        std::shared_ptr<AttributeFormModel> model;
        Getter getState;
        std::string prefix;
        using State = std::remove_cvref_t<decltype(std::declval<Getter>()(std::declval<Attribute &>()))>;

    public:
        int group = 0;

        AttributeFormSection(std::shared_ptr<AttributeFormModel> model, Getter get,
                             std::string prefix, int group = 0)
            : model(std::move(model)), getState(std::move(get)),
              prefix(std::move(prefix)), group(group) {}

        template<class T>
        void number(const char *id, T State::*member, const wchar_t *label,
                    T low, T high, const wchar_t *hint = L"") {
            const auto range = AttributeFormModel::number(low) + L" to " + AttributeFormModel::number(high);
            model->numeric(prefix + id, group, label, *hint ? hint : range,
                           [getState = getState, member](auto &attribute) -> auto & {
                               return getState(attribute).*member;
                           }, low, high);
        }

        template<class T>
        void choice(const char *id, T State::*member, const wchar_t *label,
                    const wchar_t *hint = L"Select a value, then Apply & Render.") {
            model->choice(prefix + id, group, label, hint,
                          [getState = getState, member](auto &attribute) -> auto & {
                              return getState(attribute).*member;
                          });
        }

        void color(const char *id, glm::vec4 State::*member, const wchar_t *label) {
            model->color(prefix + id, group, label,
                         [getState = getState, member](auto &attribute) -> auto & {
                             return getState(attribute).*member;
                         });
        }
        void colorRgb(const char *id, glm::vec4 State::*member, const wchar_t *label) {
            model->colorRgb(prefix + id, group, label,
                            [getState = getState, member](auto &attribute) -> auto & {
                                return getState(attribute).*member;
                            });
        }

        void image(const char *id, std::string State::*member, const wchar_t *label) {
            model->text(prefix + id, group, label,
                        L"Choose an image file. Enable the layer to display it.",
                        [getState = getState, member](const Attribute &attribute) {
                            return Unparser::STRING(getState(attribute).*member);
                        },
                        [getState = getState, member](Attribute &attribute, const std::wstring &value) {
                            if (value.find(L'\0') != std::wstring::npos) {
                                return false;
                            }
                            getState(attribute).*member = Parser::STRING(value);
                            return true;
                        },
                        FormField::Editor::IMAGE);
        }
    };
    inline void addLayerGuidance(WorkspaceForm &form, std::string prefix) {
        form.inspect = [fields = form.fields, prefix = std::move(prefix)](const FormDraft &draft) {
            FormFeedback feedback;
            const FormValues values(fields, draft);
            for (int layerIndex = 0; layerIndex < 4; ++layerIndex) {
                const auto layer = prefix + std::to_string(layerIndex) + ".";
                const auto enabled = values.number<int>(layer + "enabled");
                const auto opacity = values.number<float>(layer + "opacity");
                if (enabled && !*enabled) {
                    feedback.hints[layer + "enabled"] =
                        L"This layer is Off. Its settings are kept; choose On and Apply & Render to show it.";
                } else if (opacity && *opacity == 0) {
                    feedback.hints[layer + "opacity"] =
                        L"This layer is transparent. Increase Opacity and Apply & Render to show it.";
                }
            }
            return feedback;
        };
    }
    template<class Getter>
    void addLayerActions(WorkspaceForm &form, std::shared_ptr<AttributeFormModel> model, Getter layers,
                         std::function<void(int, int)> onMove = {}) {
        for (int layerIndex = 0; layerIndex < 4; ++layerIndex) {
            form.actions.push_back({layerIndex, L"Reset Layer", [model, layers, layerIndex] {
                model->edit([&](Attribute &attribute) { layers(attribute)[layerIndex] = {}; });
            }});
            if (layerIndex < 3) {
                form.actions.push_back({layerIndex, L"Copy to Next Layer", [model, layers, layerIndex] {
                    model->edit([&](Attribute &attribute) {
                        layers(attribute)[layerIndex + 1] = layers(attribute)[layerIndex];
                    });
                }});
            }
            if (layerIndex > 0) {
                form.actions.push_back({layerIndex, L"Move Toward Bottom", [model, layers, layerIndex, onMove] {
                    model->edit([&](Attribute &attribute) {
                        std::swap(layers(attribute)[layerIndex], layers(attribute)[layerIndex - 1]);
                    }, [onMove, layerIndex] { if (onMove) onMove(layerIndex, layerIndex - 1); });
                }});
            }
            if (layerIndex < 3) {
                form.actions.push_back({layerIndex, L"Move Toward Top", [model, layers, layerIndex, onMove] {
                    model->edit([&](Attribute &attribute) {
                        std::swap(layers(attribute)[layerIndex], layers(attribute)[layerIndex + 1]);
                    }, [onMove, layerIndex] { if (onMove) onMove(layerIndex, layerIndex + 1); });
                }});
            }
        }
    }
}
