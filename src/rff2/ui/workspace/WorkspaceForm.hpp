//
// Modified by GPT-6 on 2026-09-14, 2026-09-23, 2026-10-01, 2026-10-08
// Modified by Opus 5.5 on 2026-10-03, 2026-10-04, 2026-11-01, 2026-11-15
//

#pragma once
#include "HistoryOrder.hpp"
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

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

    // A button under a text field that types into it.
    struct FormKey {
        enum class Action {
            INSERT,    // types text at the cursor, over any selection
            WRAP,      // text is a function name: name(selection), or name() with the cursor inside
            REPLACE,   // replaces the whole field with text
            BACKSPACE, // deletes before the cursor; a function name goes with its "("
            CLEAR,
            LEFT,
            RIGHT
        };
        std::wstring label;
        std::wstring text;
        std::wstring tip;
        Action action = Action::INSERT;
        // A key that types an operator is drawn on the quieter face.
        bool symbol = false;
        // Columns the key spans in the pad.
        int span = 1;
        // REPLACE: the name drawn after the formula.
        std::wstring name;
    };

    // Calculator keys under a text field: a pad, pages of function keys picked with tabs, and examples.
    struct FormKeypad {
        std::vector<FormKey> pad;
        int padColumns = 6;
        std::vector<std::pair<std::wstring, std::vector<FormKey>>> pages;
        int pageColumns = 5;
        std::wstring examplesCaption;
        std::vector<FormKey> examples;
    };

    struct FormField {
        // How a field with choices is shown; ChoiceEditors picks one for each field a form leaves as TEXT.
        enum class Editor {
            TEXT,
            SECTION,
            COLOR,
            RGB_COLOR,
            IMAGE,
            FILE,
            CHECKBOX,      // Two choices as one box: the first choice is unchecked, the second checked.
            SEGMENTS,      // A few short choices side by side, one click each.
            ANCHOR_GRID,   // Nine positions as a 3x3 grid, Top Left first.
            CURVE_TILES,   // Choices as tiles, each with the curve it applies.
            STYLE_TOGGLES  // Regular, Bold, Italic, Bold Italic as separate B and I toggles.
        };
        std::string id;
        int group;
        std::wstring label;
        std::wstring hint;
        std::function<std::wstring()> read;
        std::vector<FormChoice> choices;
        Editor editor = Editor::TEXT;
        bool persisted = true;
        std::function<std::wstring(const std::wstring &)> validate;
        std::function<std::wstring(const std::wstring &, int, bool)> nudge;
        std::function<std::wstring(double)> sliderValue;
        std::function<double(const std::wstring &)> sliderPosition;
        std::function<bool(const FormDraft &)> enabled;
        std::function<std::vector<uint32_t>(const FormDraft &, int)> previewColors;
        std::function<void(double)> sliderReleased;
        std::function<std::vector<std::pair<std::wstring, uint32_t>>(const std::wstring &)> swatches;
        std::function<std::wstring(const std::wstring &, size_t)> removeSwatch;
        std::vector<std::string> dependencies;
        // True while the setting is kept but has no effect, so its label is drawn dimmed; it stays editable.
        std::function<bool(const FormDraft &)> dimmed;
        // For SEGMENTS and CURVE_TILES: the shape of each choice, y from 0 to 1 for x from 0 to 1.
        std::function<double(size_t choice, double x)> curve;
        // Draws a rule above the field, unless it is the first field shown.
        bool divider = false;
        std::string section;
        bool initiallyExpanded = false;
        // Keys drawn under a TEXT field; a field with keys is also checked as it is typed.
        std::shared_ptr<const FormKeypad> keypad;
        // For an error found in this text: the first character and the count to underline; npos when none applies.
        std::function<std::pair<size_t, size_t>(const std::wstring &)> errorSpan;
    };

    struct FormAction {
        int group;
        std::wstring label;
        std::function<void()> invoke;
        bool allowPending = false;
        bool allowDuringJob = false;
        std::string section;
        // After invoke, the label of the action whose button takes the focus, so Enter goes on to it; empty keeps it here.
        std::function<std::wstring()> next;
    };

    struct WorkspaceForm {
        std::wstring title;
        std::vector<std::wstring> groups;
        // Per group, whether a group that can be switched off is on; a group with no entry is never in use.
        std::vector<std::function<bool()>> groupInUse;
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
        std::function<void(FormDraft &, const std::string &)> updateDraft;
        std::function<std::wstring(const FormDraft &)> preview;
        std::function<void()> cancelPreview;

        // Draws a rule above each field whose id ends in .name, so every layer of a layered form gets it.
        void addDividers(std::initializer_list<std::string_view> names) {
            for (auto &field : fields)
                for (const auto name : names)
                    if (field.id.size() > name.size() && field.id.ends_with(name) &&
                        field.id[field.id.size() - name.size() - 1] == '.') field.divider = true;
        }
    };
}
