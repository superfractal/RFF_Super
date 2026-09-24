//
// Modified by GPT-6 on 2026-09-14, 2026-09-23
//

#pragma once
#include "HistoryOrder.hpp"
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace merutilm::rff2::workspace {
    using FormDraft = std::map<std::string, std::wstring>;

    struct FormFeedback {
        FormDraft hints;
        FormDraft errors;
        std::wstring summary;
    };

    struct FormChoice {
        std::wstring value;
        std::wstring label;
    };

    struct FormField {
        enum class Editor { TEXT, COLOR, RGB_COLOR, IMAGE, FILE };
        std::string id;
        int group;
        std::wstring label;
        std::wstring hint;
        std::function<std::wstring()> read;
        std::vector<FormChoice> choices;
        Editor editor = Editor::TEXT;
        bool persisted = true;
        std::function<std::wstring(const std::wstring &)> validate;
    };

    struct FormAction {
        int group;
        std::wstring label;
        std::function<void()> invoke;
        bool allowPending = false;
        bool allowDuringJob = false;
    };

    struct WorkspaceForm {
        std::wstring title;
        std::vector<std::wstring> groups;
        std::vector<FormField> fields;
        std::vector<FormAction> actions;
        std::function<std::wstring(const FormDraft &)> apply;
        std::function<bool()> canUndo;
        std::function<bool()> canRedo;
        std::function<bool()> undo;
        std::function<bool()> redo;
        std::function<void()> clearHistory;
        std::function<void(std::shared_ptr<HistoryDomain>)> bindHistory;
        std::function<uint64_t()> undoOrder;
        std::function<uint64_t()> redoOrder;
        std::function<void(bool)> requestHistory;
        std::function<std::wstring()> status;
        std::function<bool()> canEdit;
        std::function<FormFeedback(const FormDraft &)> inspect;
    };
}
