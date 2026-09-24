//
// Modified by GPT-6 on 2026-09-14, 2026-09-21
//

#pragma once
#include "AttributeFormModel.hpp"

namespace merutilm::rff2::workspace {
    class FormValues {
        const std::vector<FormField> &fields;
        const FormDraft &draft;

      public:
        FormValues(const std::vector<FormField> &fields, const FormDraft &draft)
            : fields(fields), draft(draft) {}
        template <class T> std::optional<T> number(std::string_view id) const {
            const auto pendingValue = draft.find(std::string(id));
            const auto matchingField = std::find_if(fields.begin(), fields.end(),
                                                    [id](const auto &field) { return field.id == id; });
            if (matchingField == fields.end()) {
                return {};
            }
            const auto text = pendingValue == draft.end() ? matchingField->read() : pendingValue->second;
            T parsedValue{};
            if (!AttributeFormModel::parse(text, parsedValue)) {
                return {};
            }
            return parsedValue;
        }
    };
} // namespace merutilm::rff2::workspace
