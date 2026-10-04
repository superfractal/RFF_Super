//
// Modified by GPT-6 on 2026-09-14, 2026-09-15, 2026-09-17, 2026-09-19, 2026-09-20, 2026-09-21, 2026-09-23, 2026-09-24, 2026-09-25, 2026-10-01
// Modified by GPT-5 on 2026-09-17
// Modified by Opus 5.5 on 2026-10-03, 2026-10-04
//

#pragma once
#include "../UiLanguage.hpp"
#include "../../io/PreferencesIO.h"
#include "../NativeDialogs.hpp"
#include "../UiDpi.hpp"
#include "WorkspaceForm.hpp"
#include "AttributeFormModel.hpp"
#include "WorkspaceComboDrawing.hpp"
#include "PanelBackBuffer.hpp"
#include "WorkspaceButton.hpp"
#include "WorkspaceEditDrawing.hpp"
#include "FormLayout.hpp"
#include "AccessibleControl.hpp"
#include "ChoiceEditors.hpp"
#include "ChoicePicker.hpp"
#include <cwctype>
#include <memory>
#include <windowsx.h>
#include <commdlg.h>

namespace merutilm::rff2::workspace {
    class FormWorkspace {
        struct Row {
            const FormField *field;
            HWND control;
            HWND picker = nullptr;
            HWND slider = nullptr;
            std::vector<HWND> swatches;
            // The height of a choice picker in this row, measured with the row; 0 for other editors.
            int pickerHeight = 0;
        };
        WorkspaceForm form;
        HWND window = nullptr, viewport = nullptr, groupControl = nullptr, applyControl = nullptr,
             discardControl = nullptr;
        std::vector<Row> rows;
        std::vector<HWND> actionControls;
        FormDraft draft;
        FormDraft sliderDraft;
        bool sliding = false;
        HWND editingControl = nullptr;
        FormDraft beforeEditing;
        FormDraft errors;
        FormDraft hints;
        std::wstring feedbackSummary;
        bool validationAttempted = false;
        bool descriptionsShown = PreferencesIO::showSettingDescriptions();
        std::vector<int> rowOffsets;
        std::vector<FormLayout::Row> rowLayouts;
        int actionsOffset = 0, bodyHeight = 0, footerExtent = 104;
        std::wstring query, message;
        std::wstring lastStatus;
        bool lastUndo = false, lastRedo = false;
        int group = 0, scroll = 0, contentHeight = 0, wheelRemainder = 0;
        bool flowingFooter = false;
        HFONT font;
        const WorkspaceTheme &theme;
        float scale;
        HBRUSH brush;
        PanelBackBuffer buffer, contentBuffer;
        WorkspaceComboDrawing::Context comboContext;
        bool syncing = false, rebuilding = false, compact = false;
        std::function<void()> changed;
        std::function<void(int)> leaveFocus;
        std::function<void()> resetSearch;

        int px(int value) const {
            return int(value * scale + .5f);
        }
        static constexpr const wchar_t *checkedProperty = L"RFF.Form.Checked";
        static constexpr const wchar_t *dimmedProperty = L"RFF.Form.Dimmed";
        static bool checkbox(const FormField &field) {
            return field.editor == FormField::Editor::CHECKBOX && field.choices.size() == 2;
        }
        // A field picked from a list: one with choices that is shown neither as a checkbox nor as a picker.
        static bool dropDown(const FormField &field) {
            return !field.choices.empty() && !checkbox(field) && !ChoicePicker::handles(field);
        }
        static bool checked(HWND control) {
            return GetPropW(control, checkedProperty) != nullptr;
        }
        static void setFlag(HWND control, const wchar_t *property, bool on) {
            if (on) {
                SetPropW(control, property, reinterpret_cast<HANDLE>(1));
            } else {
                RemovePropW(control, property);
            }
        }
        bool dimmed(const FormField &field) const {
            return field.dimmed && field.dimmed(draft);
        }
        static constexpr const wchar_t *pickerClass = L"RFF.Workspace.Choice";
        static constexpr const wchar_t *choiceProperty = L"RFF.Form.Choice";
        static constexpr const wchar_t *hoverProperty = L"RFF.Form.Hover";
        // A picker keeps the index of the choice it shows, and of the part under the pointer, plus one.
        static int storedIndex(HWND control, const wchar_t *property) {
            return int(reinterpret_cast<INT_PTR>(GetPropW(control, property))) - 1;
        }
        static void storeIndex(HWND control, const wchar_t *property, int index) {
            SetPropW(control, property, reinterpret_cast<HANDLE>(INT_PTR(index + 1)));
        }
        static int choiceIndex(const FormField &field, const std::wstring &value) {
            const auto found = std::find_if(field.choices.begin(), field.choices.end(),
                                            [&](const auto &choice) { return choice.value == value; });
            return found == field.choices.end() ? -1 : int(found - field.choices.begin());
        }
        ChoicePicker::Look pickerLook() const {
            return {theme, font, scale};
        }
        static void positionControl(HWND control, int x, int y, int width, int height) {
            RECT previousBounds;
            GetWindowRect(control, &previousBounds);
            MapWindowPoints(nullptr, GetParent(control), reinterpret_cast<POINT *>(&previousBounds), 2);
            wchar_t className[32];
            GetClassNameW(control, className, 32);
            const bool isComboBox = std::wstring_view(className) == L"ComboBox";
            const bool heightChanged = isComboBox ? INT_PTR(GetPropW(control, L"RFF.Layout.Height")) != height
                                                  : previousBounds.bottom - previousBounds.top != height;
            if (previousBounds.left != x || previousBounds.top != y ||
                previousBounds.right - previousBounds.left != width || heightChanged) {
                SetWindowPos(control, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
                if (isComboBox) {
                    SetPropW(control, L"RFF.Layout.Height", HANDLE(INT_PTR(height)));
                }
            }
        }
        RECT bounds() const {
            RECT r;
            GetClientRect(window, &r);
            return r;
        }
        static std::wstring lower(std::wstring value) {
            for (auto &c : value) {
                c = std::towlower(c);
            }
            return value;
        }
        std::vector<const FormField *> fields() const {
            std::vector<const FormField *> matchingFields;
            const auto normalizedQuery = lower(query);
            for (const auto &field : form.fields) {
                const std::wstring fieldId(field.id.begin(), field.id.end());
                if (query.empty()
                        ? field.group == group
                        : lower(field.label + L" " + field.hint + L" " + fieldId).find(normalizedQuery) !=
                              std::wstring::npos) {
                    matchingFields.push_back(&field);
                }
            }
            return matchingFields;
        }
        int inputWidth(int width, bool picker, bool colorSwatch = false) const {
            const int available = std::max(1, width - px(2 * FormLayout::inset +
                                          (picker ? FormLayout::pickerWidth + FormLayout::pickerGap : 0)));
            return colorSwatch ? std::min(available, px(56)) : available;
        }
        int pickerLeft(int width) const {
            return width - px(FormLayout::inset + FormLayout::pickerWidth);
        }
        RECT contentBounds() const {
            RECT r{};
            GetClientRect(viewport, &r);
            return r;
        }
        const std::wstring &visibleHintFor(const FormField &field) const {
            static const std::wstring empty;
            return PreferencesIO::showSettingDescriptions() ? hintFor(field) : empty;
        }
        bool waitingForSelection(const FormField &field) const {
            return std::any_of(field.dependencies.begin(), field.dependencies.end(),
                [&](const auto &id) { return draft.contains(id); });
        }
        const std::wstring &hintFor(const FormField &field) const {
            static const std::wstring waiting = L"Confirm the related selection with Enter or Apply & Render before editing this value.";
            if (waitingForSelection(field)) return waiting;
            const auto hint = hints.find(field.id);
            return hint == hints.end() ? field.hint : hint->second;
        }
        void validatePendingFields() {
            for (const auto &field : form.fields) {
                if (const auto pending = draft.find(field.id); pending != draft.end()) {
                    const auto error = validate(field, pending->second);
                    if (!error.empty()) {
                        errors[field.id] = error;
                    }
                }
            }
        }
        bool updateFeedback() {
            const auto oldErrors = errors, oldHints = hints;
            const auto oldSummary = feedbackSummary;
            hints.clear();
            feedbackSummary.clear();
            if (validationAttempted) {
                errors.clear();
            }
            try {
                if (validationAttempted) {
                    validatePendingFields();
                }
                if (form.inspect) {
                    auto feedback = form.inspect(draft);
                    hints = std::move(feedback.hints);
                    if (validationAttempted && !draft.empty()) {
                        if (errors.empty() && !feedback.errors.empty()) {
                            feedbackSummary = std::move(feedback.summary);
                        }
                        for (auto &[id, error] : feedback.errors) {
                            if (!error.empty()) {
                                errors.try_emplace(id, std::move(error));
                            }
                        }
                    }
                }
            } catch (const std::exception &) {
                if (validationAttempted) {
                    message = L"Unable to check these values. Your changes have not been applied.";
                }
            }
            for (const auto &field : form.fields) if (waitingForSelection(field)) hints[field.id] = hintFor(field);
            return oldErrors != errors || oldHints != hints || oldSummary != feedbackSummary;
        }
        int footerTop() const {
            return flowingFooter ? bodyHeight - scroll : bounds().bottom - footerExtent;
        }
        int rowTop(int index) const {
            return (index < int(rowOffsets.size())
                        ? rowOffsets[index]
                        : px(FormLayout::rowTop(index) - FormLayout::headerHeight)) -
                   scroll;
        }
        int inputTop(int index) const {
            return rowTop(index) + (index < int(rowLayouts.size()) ? rowLayouts[index].inputOffset
                                                                   : px(FormLayout::inputOffset));
        }
        // A checkbox control spans its box plus the 2px focus ring above and below it.
        int controlTop(int index, const FormField &field) const {
            return inputTop(index) - (checkbox(field) ? px(2) : 0);
        }
        int controlHeight(const FormField &field) const {
            return px(checkbox(field) ? FormLayout::checkboxHeight + 4 : dropDown(field) ? 260 : FormLayout::inputHeight);
        }
        static int wrappedHeight(HDC dc, std::wstring_view text, int width) {
            RECT measured{0, 0, std::max(1, width), 0};
            UiLanguage::drawText(dc, text.data(), int(text.size()), &measured,
                                 DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
            return measured.bottom;
        }
        void measureFlow() {
            const auto outer = bounds();
            RECT nativeViewport;
            GetWindowRect(viewport, &nativeViewport);
            if (nativeViewport.right - nativeViewport.left != outer.right) {
                SetWindowPos(viewport, nullptr, 0, 0, outer.right,
                             std::max(1L, nativeViewport.bottom - nativeViewport.top),
                             SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
            }
            const int width = std::max(1, int(contentBounds().right) - px(40));
            const HDC dc = GetDC(viewport);
            const auto previous = SelectObject(dc, font);
            rowOffsets.clear();
            rowLayouts.clear();
            int next = px(FormLayout::firstRow - FormLayout::headerHeight);
            for (auto &row : rows) {
                const auto error = errors.find(row.field->id);
                const auto &hint = error == errors.end() ? visibleHintFor(*row.field) : error->second;
                auto layout =
                    FormLayout::measuredRow(wrappedHeight(dc, row.field->label + L" *", width),
                                            wrappedHeight(dc, hint, width), scale,
                                            checkbox(*row.field) ? FormLayout::checkboxHeight : FormLayout::inputHeight);
                row.pickerHeight =
                    ChoicePicker::handles(*row.field) ? ChoicePicker::height(*row.field, width, dc, pickerLook()) : 0;
                const int pickerExtra = std::max(0, row.pickerHeight - px(FormLayout::inputHeight));
                const int extra = (row.slider ? px(32) : 0) + (row.field->previewColors ? px(28) : 0) + (row.field->swatches ? px(32) : 0) + pickerExtra;
                layout.hintOffset += extra;
                layout.height += extra;
                if (row.field->divider && !rowOffsets.empty()) next += px(FormLayout::dividerSpace);
                rowOffsets.push_back(next);
                rowLayouts.push_back(layout);
                next += layout.height;
            }
            actionsOffset = next + px(12);
            bodyHeight =
                next + px(12) +
                (actionControls.empty() ? 0 : px(12 + int(actionControls.size()) * FormLayout::actionPitch));
            footerExtent =
                std::max(px(FormLayout::footerHeight), px(62) + wrappedHeight(dc, statusText(), width));
            SelectObject(dc, previous);
            ReleaseDC(viewport, dc);
            flowingFooter =
                outer.bottom < px(FormLayout::headerHeight + FormLayout::rowPitch + 12) + footerExtent;
            const HWND parent = flowingFooter ? viewport : window;
            for (HWND control : {applyControl, discardControl}) {
                if (GetParent(control) != parent) {
                    SetParent(control, parent);
                }
            }
            positionControl(viewport, 0, px(FormLayout::headerHeight), outer.right,
                            std::max(1, int(outer.bottom) - px(FormLayout::headerHeight) -
                                            (flowingFooter ? 0 : footerExtent)));
            contentHeight = bodyHeight + (flowingFooter ? footerExtent : 0);
        }
        void placeContent(bool remeasure = true) {
            if (remeasure) {
                measureFlow();
            }
            const auto r = contentBounds();
            scroll = std::clamp(scroll, 0, std::max(0, contentHeight - int(r.bottom)));
            positionControl(groupControl, px(20), px(12), std::max(1, int(r.right) - px(40)), px(300));
            for (size_t i = 0; i < rows.size(); ++i) {
                const auto &row = rows[i];
                const int y = inputTop(int(i));
                positionControl(row.control, px(20), controlTop(int(i), *row.field), inputWidth(r.right, row.picker != nullptr,
                                    row.field->editor == FormField::Editor::COLOR || row.field->editor == FormField::Editor::RGB_COLOR),
                                row.pickerHeight ? row.pickerHeight : controlHeight(*row.field));
                if (row.picker) {
                    positionControl(row.picker, pickerLeft(r.right), y, px(FormLayout::pickerWidth), px(28));
                }
                if (row.slider) positionControl(row.slider, px(20), y + px(30), std::max(1L, r.right - px(40)), px(28));
                const int cell = std::max(1L, (r.right - px(40)) / 8);
                for (size_t j = 0; j < row.swatches.size(); ++j)
                    positionControl(row.swatches[j], px(20) + int(j % 8) * cell, y + px(int(j / 8) * 30), std::max(1, cell - px(4)), px(26));
            }
            for (size_t i = 0; i < actionControls.size(); ++i) {
                positionControl(actionControls[i], px(20),
                                actionsOffset + px(int(i) * FormLayout::actionPitch) - scroll,
                                std::max(1, int(r.right) - px(40)), px(32));
            }
            const int available = std::max(1, int(r.right) - px(40)), gap = px(10),
                      primary = std::min(px(144), std::max(1, (available - gap) * 3 / 5));
            positionControl(applyControl, px(20), footerTop() + px(12), primary, px(30));
            positionControl(discardControl, px(20) + primary + gap, footerTop() + px(12),
                            std::max(1, available - primary - gap), px(30));
            SCROLLINFO info{sizeof(info), SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL};
            info.nMax = std::max(0, contentHeight - 1);
            info.nPage = std::max(1L, r.bottom);
            info.nPos = scroll;
            SetScrollInfo(viewport, SB_VERT, &info, TRUE);
            InvalidateRect(viewport, nullptr, FALSE);
        }
        void reveal(HWND control) {
            if (rebuilding || GetParent(control) != viewport) {
                return;
            }
            RECT r;
            GetWindowRect(control, &r);
            MapWindowPoints(nullptr, viewport, reinterpret_cast<POINT *>(&r), 2);
            int top = r.top, bottom = r.bottom;
            const int height = contentBounds().bottom;
            for (size_t i = 0; i < rows.size(); ++i) {
                if (rows[i].control == control || rows[i].picker == control) {
                    // Native offscreen child coordinates saturate at 32767; scrolling uses logical content positions.
                    top = inputTop(int(i));
                    bottom = top + (checkbox(*rows[i].field) ? px(FormLayout::checkboxHeight) : px(FormLayout::inputHeight));
                    if (height >= rowLayouts[i].height) {
                        top = rowTop(int(i));
                        bottom = top + rowLayouts[i].height;
                    }
                    break;
                }
            }
            for (size_t i = 0; i < actionControls.size(); ++i) {
                if (actionControls[i] == control) {
                    top = actionsOffset + px(int(i) * FormLayout::actionPitch) - scroll;
                    bottom = top + px(32);
                    break;
                }
            }
            if (control == applyControl || control == discardControl) {
                top = footerTop() + px(12);
                bottom = top + px(30);
            }
            const int offset = top < 0 ? top : bottom > height ? bottom - height : 0;
            if (offset) {
                scroll += offset;
                placeContent(false);
            }
        }
        void scrollContent(UINT code, int position = 0) {
            const int page = std::max(px(32), int(contentBounds().bottom) - px(32));
            if (code == SB_LINEUP) {
                scroll -= px(32);
            } else if (code == SB_LINEDOWN) {
                scroll += px(32);
            } else if (code == SB_PAGEUP) {
                scroll -= page;
            } else if (code == SB_PAGEDOWN) {
                scroll += page;
            } else if (code == SB_TOP) {
                scroll = 0;
            } else if (code == SB_BOTTOM) {
                scroll = contentHeight;
            } else if (code == SB_THUMBTRACK || code == SB_THUMBPOSITION) {
                scroll = position;
            } else {
                return;
            }
            placeContent(false);
        }
        void wheel(WPARAM w) {
            wheelRemainder += GET_WHEEL_DELTA_WPARAM(w);
            const int steps = wheelRemainder / WHEEL_DELTA;
            wheelRemainder %= WHEEL_DELTA;
            if (steps) {
                scroll -= steps * px(FormLayout::rowPitch);
                placeContent(false);
            }
        }
        void updateEnabled() {
            const bool editable = !form.canEdit || form.canEdit();
            for (const auto &row : rows) {
                const bool enabled = editable && !waitingForSelection(*row.field) && (!row.field->enabled || row.field->enabled(draft));
                EnableWindow(row.control, enabled);
                if (row.picker) EnableWindow(row.picker, enabled);
                if (row.slider) EnableWindow(row.slider, enabled);
                for (HWND swatch : row.swatches) EnableWindow(swatch, enabled);
                const bool faded = dimmed(*row.field);
                if (faded != (GetPropW(row.control, dimmedProperty) != nullptr)) {
                    setFlag(row.control, dimmedProperty, faded);
                    InvalidateRect(row.control, nullptr, FALSE);
                    InvalidateRect(viewport, nullptr, FALSE);
                }
            }
            EnableWindow(applyControl, editable && (!draft.empty() || sliding));
            EnableWindow(discardControl, !draft.empty());
            AccessibleControl::describe(applyControl, L"Apply and Render",
                                        message.empty() && errors.empty()
                                            ? L"Apply pending settings and render the preview."
                                            : L"Cannot apply. " + statusText(),
                                        L"workspace.apply");
            AccessibleControl::describe(discardControl, L"Discard",
                                        L"Discard pending changes in " + form.title + L".",
                                        L"workspace.discard");
            for (HWND control : actionControls) {
                const auto &action = form.actions[size_t(GetDlgCtrlID(control) - 100)];
                EnableWindow(control,
                             !sliding && (editable || action.allowDuringJob) && (draft.empty() || action.allowPending));
            }
        }
        void invalidate(bool notify = true) {
            updateEnabled();
            lastStatus = statusText();
            lastUndo = canUndo();
            lastRedo = canRedo();
            InvalidateRect(window, nullptr, FALSE);
            InvalidateRect(viewport, nullptr, FALSE);
            if (notify && changed && !rebuilding) {
                changed();
            }
        }
        void updateStatus() {
            const auto status = statusText();
            const bool undo = canUndo(), redo = canRedo();
            if (status == lastStatus && undo == lastUndo && redo == lastRedo) {
                return;
            }
            if (status != lastStatus) {
                const auto dc = GetDC(viewport);
                const auto previous = SelectObject(dc, font);
                const int extent = std::max(
                    px(FormLayout::footerHeight),
                    px(62) + wrappedHeight(dc, status, std::max(1, int(contentBounds().right) - px(40))));
                SelectObject(dc, previous);
                ReleaseDC(viewport, dc);
                if (extent != footerExtent) {
                    placeContent();
                    InvalidateRect(window, nullptr, FALSE);
                } else {
                    RECT footer{0, footerTop(), contentBounds().right, footerTop() + footerExtent};
                    InvalidateRect(flowingFooter ? viewport : window, &footer, FALSE);
                }
            }
            lastStatus = status;
            lastUndo = undo;
            lastRedo = redo;
            if (changed) {
                changed();
            }
        }
        void refreshErrors() {
            for (const auto &row : rows) {
                AccessibleControl::describe(row.control, row.field->label, hintFor(*row.field),
                                            std::wstring(row.field->id.begin(), row.field->id.end()));
                if (row.picker) {
                    AccessibleControl::describe(
                        row.picker, L"Choose " + row.field->label, hintFor(*row.field),
                        std::wstring(row.field->id.begin(), row.field->id.end()) + L".choose");
                }
                const bool wasInvalid = GetPropW(row.control, L"RFF.Form.Invalid") != nullptr;
                if (errors.contains(row.field->id)) {
                    SetPropW(row.control, L"RFF.Form.Invalid", reinterpret_cast<HANDLE>(1));
                } else {
                    RemovePropW(row.control, L"RFF.Form.Invalid");
                }
                const auto error = errors.find(row.field->id);
                AccessibleControl::validation(row.control,
                                              error == errors.end() ? std::wstring_view{} : error->second);
                if (wasInvalid != errors.contains(row.field->id)) {
                    RedrawWindow(row.control, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME);
                }
            }
            placeContent();
            invalidate();
        }
        std::wstring validate(const FormField &field, const std::wstring &value) const {
            if (field.validate) {
                return field.validate(value);
            }
            if (field.editor == FormField::Editor::COLOR || field.editor == FormField::Editor::RGB_COLOR) {
                glm::vec4 color(1);
                const bool rgbOnly = field.editor == FormField::Editor::RGB_COLOR;
                if (!(rgbOnly ? AttributeFormModel::parseColorRgb(value, color)
                              : AttributeFormModel::parseColor(value, color))) {
                    return rgbOnly ? L"Enter #RRGGBB or three RGB values from 0 to 1."
                                   : L"Enter #RRGGBB or four RGBA values from 0 to 1.";
                }
            }
            return {};
        }
        void focusError() {
            for (const auto &field : form.fields) {
                if (errors.contains(field.id)) {
                    if (std::none_of(rows.begin(), rows.end(),
                                     [&](const auto &row) { return row.field == &field; })) {
                        if (!query.empty() && resetSearch) {
                            resetSearch();
                        }
                        query.clear();
                        group = field.group;
                        scroll = 0;
                        rebuild();
                    }
                    for (const auto &row : rows) {
                        if (row.field == &field) {
                            SetFocus(row.control);
                            reveal(row.control);
                            return;
                        }
                    }
                }
            }
            if (!message.empty()) {
                SetFocus(applyControl);
                reveal(applyControl);
                return;
            }
            SetFocus(window);
        }
        void finishSlider(bool commit) {
            if (!sliding) return;
            sliding = false;
            auto values = std::move(sliderDraft);
            sliderDraft.clear();
            form.cancelPreview();
            if (commit && (!form.canEdit || form.canEdit())) {
                try { message = form.apply(values); }
                catch (const std::exception &) { message = L"Unable to apply these values. Check the entered settings."; }
                if (message.empty()) clearApplied(values);
            }
            syncing = true;
            for (const auto &row : rows) {
                const auto pending = draft.find(row.field->id);
                setValue(row, pending == draft.end() ? row.field->read() : pending->second);
            }
            syncing = false;
            updateValues(true);
            invalidate();
        }
        void clearApplied(const FormDraft &values) {
            for (const auto &[id, value] : values) {
                draft.erase(id);
                beforeEditing.erase(id);
                errors.erase(id);
            }
        }
        bool applyImmediate(const FormField &field, const std::wstring &value) {
            if (form.canEdit && !form.canEdit()) return false;
            if (waitingForSelection(field) || (field.enabled && !field.enabled(draft))) {
                message = waitingForSelection(field) ? hintFor(field) : L"This setting is inactive.";
                refreshErrors();
                return true;
            }
            if (const auto error = validate(field, value); !error.empty()) {
                errors[field.id] = error;
                refreshErrors();
                return false;
            }
            finishSlider(true);
            FormDraft values{{field.id, value}};
            if (form.updateDraft) form.updateDraft(values, field.id);
            try { message = form.apply(values); }
            catch (const std::exception &) { message = L"Unable to apply these values. Check the entered settings."; }
            if (message.empty()) clearApplied(values);
            updateValues(true);
            refreshErrors();
            return true;
        }
        void slide(HWND control, int notification) {
            if (syncing || (form.canEdit && !form.canEdit())) return;
            if (notification == TB_ENDTRACK) {
                if (!sliding) return;
                for (const auto &row : rows) if (row.slider == control && row.field->sliderReleased)
                    row.field->sliderReleased(double(SendMessageW(control, TBM_GETPOS, 0, 0)) / 10000);
                finishSlider(true);
                return;
            }
            for (const auto &row : rows) {
                if (row.slider != control || !IsWindowEnabled(control)) continue;
                const auto value = row.field->sliderValue(double(SendMessageW(control, TBM_GETPOS, 0, 0)) / 10000);
                if (!validate(*row.field, value).empty()) return;
                FormDraft values{{row.field->id, value}};
                if (form.updateDraft) form.updateDraft(values, row.field->id);
                sliding = true;
                try { message = form.preview(values); }
                catch (const std::exception &) { message = L"Unable to apply these values. Check the entered settings."; }
                if (!message.empty()) { finishSlider(false); return; }
                sliderDraft = std::move(values);
                updateEnabled();
                syncing = true;
                for (const auto &peer : rows) {
                    const auto pending = draft.find(peer.field->id);
                    setValue(peer, sliderDraft.contains(peer.field->id) || pending == draft.end() ? peer.field->read() : pending->second);
                }
                syncing = false;
                InvalidateRect(viewport, nullptr, FALSE);
                if (notification != TB_THUMBTRACK && notification != TB_THUMBPOSITION) {
                    if (row.field->sliderReleased) row.field->sliderReleased(double(SendMessageW(control, TBM_GETPOS, 0, 0)) / 10000);
                    finishSlider(true);
                }
                return;
            }
        }
        void capture(Row &row) {
            if (syncing) {
                return;
            }
            const bool wasPending = draft.contains(row.field->id);
            std::wstring value;
            if (checkbox(*row.field)) {
                value = row.field->choices[checked(row.control) ? 1 : 0].value;
            } else if (ChoicePicker::handles(*row.field)) {
                const int index = storedIndex(row.control, choiceProperty);
                if (index < 0 || index >= int(row.field->choices.size())) {
                    return;
                }
                value = row.field->choices[size_t(index)].value;
            } else if (row.field->choices.empty()) {
                const int n = GetWindowTextLengthW(row.control);
                value.resize(n + 1);
                GetWindowTextW(row.control, value.data(), n + 1);
                value.resize(n);
            } else {
                const int index = int(SendMessageW(row.control, CB_GETCURSEL, 0, 0));
                if (index < 0 || index >= int(row.field->choices.size())) {
                    return;
                }
                value = row.field->choices[index].value;
            }
            if (value == row.field->read()) {
                draft.erase(row.field->id);
            } else {
                draft[row.field->id] = std::move(value);
            }
            if (form.updateDraft) {
                form.updateDraft(draft, row.field->id);
                syncing = true;
                for (const auto &peer : rows) {
                    if (peer.control == row.control) continue;
                    const auto pending = draft.find(peer.field->id);
                    setValue(peer, pending == draft.end() ? peer.field->read() : pending->second);
                }
                syncing = false;
                invalidate();
            }
            updateSwatches(row, draft.contains(row.field->id) ? draft.at(row.field->id) : row.field->read());
            const bool hadError = errors.erase(row.field->id) > 0;
            message.clear();
            const bool feedbackChanged = updateFeedback();
            if (hadError || feedbackChanged) {
                refreshErrors();
                reveal(row.control);
            } else {
                if (wasPending != draft.contains(row.field->id)) {
                    const int index = int(&row - rows.data());
                    RECT label{px(20), rowTop(index), contentBounds().right - px(20),
                               rowTop(index) + rowLayouts[index].labelHeight};
                    InvalidateRect(viewport, &label, FALSE);
                }
                updateEnabled();
                updateStatus();
            }
        }
        void updateSwatches(const Row &row, const std::wstring &value) {
            if (!row.field->swatches) return;
            const auto colors = row.field->swatches(value);
            for (size_t i = 0; i < row.swatches.size(); ++i) {
                const HWND button = row.swatches[i];
                if (i < colors.size()) {
                    const auto label = UiLanguage::text(L"Remove Frozen Color") + L" " + std::to_wstring(i + 1) + L" (" + colors[i].first + L")";
                    wchar_t previous[256]{};
                    GetWindowTextW(button, previous, 256);
                    if (label != previous) {
                        SetWindowTextW(button, label.c_str());
                        AccessibleControl::describe(button, label, row.field->hint,
                            std::wstring(row.field->id.begin(), row.field->id.end()) + L".remove." + std::to_wstring(i));
                    }
                    const HANDLE color = HANDLE(UINT_PTR(colors[i].second) + 1);
                    if (GetPropW(button, L"RFF.Swatch.Color") != color) {
                        SetPropW(button, L"RFF.Swatch.Color", color);
                        InvalidateRect(button, nullptr, FALSE);
                    }
                    ShowWindow(button, SW_SHOWNA);
                } else ShowWindow(button, SW_HIDE);
            }
        }
        void setValue(const Row &row, const std::wstring &value) {
            updateSwatches(row, value);
            if (row.slider && !sliding) SendMessageW(row.slider, TBM_SETPOS, TRUE,
                LPARAM(std::lround(std::clamp(row.field->sliderPosition(value), 0.0, 1.0) * 10000)));
            if (checkbox(*row.field)) {
                const bool on = value == row.field->choices[1].value;
                if (checked(row.control) != on) {
                    setFlag(row.control, checkedProperty, on);
                    InvalidateRect(row.control, nullptr, FALSE);
                }
            } else if (ChoicePicker::handles(*row.field)) {
                const int index = choiceIndex(*row.field, value);
                if (storedIndex(row.control, choiceProperty) != index) {
                    storeIndex(row.control, choiceProperty, index);
                    InvalidateRect(row.control, nullptr, FALSE);
                }
            } else if (row.field->choices.empty()) {
                const int length = GetWindowTextLengthW(row.control);
                std::wstring current(length + 1, L'\0');
                GetWindowTextW(row.control, current.data(), length + 1);
                current.resize(length);
                if (current != value) {
                    SetWindowTextW(row.control, value.c_str());
                }
            } else {
                const auto found = std::find_if(row.field->choices.begin(), row.field->choices.end(),
                                                [&](const auto &choice) { return choice.value == value; });
                SendMessageW(row.control, CB_SETCURSEL,
                             found == row.field->choices.end() ? -1 : found - row.field->choices.begin(), 0);
            }
        }
        void toggle(Row &row) {
            setValue(row, row.field->choices[checked(row.control) ? 0 : 1].value);
            capture(row);
        }
        void pick(Row &row) {
            const auto pending = draft.find(row.field->id);
            const auto current = pending == draft.end() ? row.field->read() : pending->second;
            if (row.field->editor == FormField::Editor::COLOR ||
                row.field->editor == FormField::Editor::RGB_COLOR) {
                glm::vec4 color(1);
                const bool rgbOnly = row.field->editor == FormField::Editor::RGB_COLOR;
                if (!(rgbOnly ? AttributeFormModel::parseColorRgb(current, color)
                              : AttributeFormModel::parseColor(current, color))) {
                    errors[row.field->id] = rgbOnly ? L"Enter #RRGGBB or three RGB values from 0 to 1."
                                                    : L"Enter #RRGGBB or four RGBA values from 0 to 1.";
                    refreshErrors();
                    focusError();
                    return;
                }
                static COLORREF custom[16]{};
                CHOOSECOLORW dialog{sizeof(dialog)};
                dialog.hwndOwner = window;
                dialog.lpCustColors = custom;
                dialog.Flags = CC_FULLOPEN | CC_RGBINIT;
                dialog.rgbResult =
                    RGB(BYTE(color.r * 255 + .5f), BYTE(color.g * 255 + .5f), BYTE(color.b * 255 + .5f));
                if (!NativeDialogs::chooseColor(&dialog)) {
                    return;
                }
                color.r = AttributeFormModel::colorByte(GetRValue(dialog.rgbResult));
                color.g = AttributeFormModel::colorByte(GetGValue(dialog.rgbResult));
                color.b = AttributeFormModel::colorByte(GetBValue(dialog.rgbResult));
                setValue(row, rgbOnly ? AttributeFormModel::colorTextRgb(color)
                                      : AttributeFormModel::colorText(color));
            } else if (row.field->editor == FormField::Editor::IMAGE ||
                       row.field->editor == FormField::Editor::FILE) {
                std::vector<wchar_t> path(32768);
                std::copy_n(current.data(), std::min(current.size(), path.size() - 1), path.data());
                OPENFILENAMEW dialog{sizeof(dialog)};
                dialog.hwndOwner = window;
                dialog.lpstrFile = path.data();
                dialog.nMaxFile = DWORD(path.size());
                dialog.lpstrFilter = row.field->editor == FormField::Editor::IMAGE
                                         ? L"Images\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All files\0*.*\0"
                                         : L"Color settings\0*.rfc;*.rfsp;*.kfr;*.kfp\0All files\0*.*\0";
                dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
                if (!NativeDialogs::openFile(&dialog)) {
                    return;
                }
                setValue(row, path.data());
            }
            capture(row);
        }
        void updateValues(bool includeFocus = false) {
            syncing = true;
            for (const auto &row : rows) {
                if (row.field->swatches) updateSwatches(row, draft.contains(row.field->id) ? draft.at(row.field->id) : row.field->read());
                if (draft.contains(row.field->id) || (!includeFocus && GetFocus() == row.control)) {
                    continue;
                }
                const auto live = row.field->read();
                if (row.slider && !sliding) SendMessageW(row.slider, TBM_SETPOS, TRUE,
                    LPARAM(std::lround(std::clamp(row.field->sliderPosition(live), 0.0, 1.0) * 10000)));
                if (checkbox(*row.field)) {
                    if (checked(row.control) != (live == row.field->choices[1].value)) {
                        setValue(row, live);
                    }
                } else if (ChoicePicker::handles(*row.field)) {
                    if (storedIndex(row.control, choiceProperty) != choiceIndex(*row.field, live)) {
                        setValue(row, live);
                    }
                } else if (row.field->choices.empty()) {
                    const int n = GetWindowTextLengthW(row.control);
                    std::wstring text(n + 1, L'\0');
                    GetWindowTextW(row.control, text.data(), n + 1);
                    text.resize(n);
                    if (text != live) {
                        setValue(row, live);
                    }
                } else {
                    const int index = int(SendMessageW(row.control, CB_GETCURSEL, 0, 0));
                    if (index < 0 || index >= int(row.field->choices.size()) ||
                        row.field->choices[index].value != live) {
                        setValue(row, live);
                    }
                }
            }
            syncing = false;
            updateEnabled();
            if (form.inspect && updateFeedback()) {
                refreshErrors();
                reveal(GetFocus());
            }
            updateStatus();
        }
        std::wstring statusText() const {
            if (!message.empty()) {
                return message;
            }
            if (!errors.empty() && !feedbackSummary.empty()) {
                return feedbackSummary;
            }
            if (!errors.empty()) {
                return std::to_wstring(errors.size()) +
                       (errors.size() == 1
                            ? L" setting needs attention. Your changes have not been applied."
                            : L" settings need attention. Your changes have not been applied.");
            }
            if (!draft.empty()) {
                return std::to_wstring(draft.size()) +
                       (draft.size() == 1 ? L" pending change in " : L" pending changes across ") +
                       form.title + L".";
            }
            return form.status ? form.status() : L"No pending changes.";
        }
        void rebuild() {
            if (rebuilding || !discardControl) {
                return;
            }
            finishSlider(true);
            rebuilding = true;
            syncing = true;
            const bool visibleWindow = (GetWindowLongPtrW(window, GWL_STYLE) & WS_VISIBLE) != 0;
            std::vector<HWND> paused;
            if (visibleWindow) {
                SendMessageW(window, WM_SETREDRAW, FALSE, 0);
                EnumChildWindows(
                    window,
                    [](HWND child, LPARAM data) -> BOOL {
                        if (GetWindowLongPtrW(child, GWL_STYLE) & WS_VISIBLE) {
                            reinterpret_cast<std::vector<HWND> *>(data)->push_back(child);
                            SendMessageW(child, WM_SETREDRAW, FALSE, 0);
                        }
                        return TRUE;
                    },
                    LPARAM(&paused));
            }
            auto previousRows = std::move(rows);
            rows.clear();
            auto previousActions = std::move(actionControls);
            actionControls.clear();
            const auto visible = fields();
            const auto outer = bounds();
            const RECT r{
                0, 0, std::max(1, int(outer.right) - UiDpi::metric(SM_CXVSCROLL, UINT(scale * 96 + .5f))), 0};
            for (int i = 0; i < int(visible.size()); ++i) {
                const auto *field = visible[i];
                const int y = inputTop(i);
                const bool combo = dropDown(*field);
                const bool checkboxRow = checkbox(*field);
                const bool colorButton = field->editor == FormField::Editor::COLOR || field->editor == FormField::Editor::RGB_COLOR;
                const bool ownerDrawnButton = colorButton || checkboxRow;
                const bool choicePicker = ChoicePicker::handles(*field);
                const bool pickable = field->editor != FormField::Editor::TEXT && !checkboxRow && !choicePicker;
                auto previous = std::find_if(previousRows.begin(), previousRows.end(),
                                             [field](const auto &row) { return row.field == field; });
                HWND control =
                    previous != previousRows.end() ? previous->control
                    : choicePicker
                        ? CreateWindowExW(0, pickerClass, L"", WS_CHILD | WS_TABSTOP, px(20), y,
                                          inputWidth(r.right, false), px(FormLayout::inputHeight), viewport, nullptr,
                                          GetModuleHandleW(nullptr), this)
                        : CreateWindowExW(
                              combo || ownerDrawnButton ? 0 : WS_EX_CLIENTEDGE, combo ? L"COMBOBOX" : ownerDrawnButton ? L"BUTTON" : L"EDIT", L"",
                              WS_CHILD | WS_TABSTOP |
                                  (combo ? CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL
                                         : ownerDrawnButton ? BS_OWNERDRAW : ES_AUTOHSCROLL),
                              px(20), y, inputWidth(r.right, pickable, colorButton),
                              px(combo ? 260 : FormLayout::inputHeight), viewport, nullptr,
                              GetModuleHandleW(nullptr), nullptr);
                SetWindowPos(control, nullptr, px(20), controlTop(i, *field), inputWidth(r.right, pickable, colorButton),
                             controlHeight(*field), SWP_NOZORDER | SWP_NOACTIVATE);
                SetPropW(control, L"RFF.Form.Field", const_cast<FormField *>(field));
                SetPropW(control, L"RFF.Form.Row", reinterpret_cast<HANDLE>(INT_PTR(i + 1)));
                AccessibleControl::describe(control, field->label, field->hint,
                                            std::wstring(field->id.begin(), field->id.end()));
                SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
                if (combo) {
                    WorkspaceComboDrawing::applyMetrics(control, comboContext, px(FormLayout::inputHeight));
                }
                if (combo && previous == previousRows.end()) {
                    for (const auto &choice : field->choices) {
                        SendMessageW(control, CB_ADDSTRING, 0,
                                     reinterpret_cast<LPARAM>(UiLanguage::text(choice.label).c_str()));
                    }
                    WorkspaceComboDrawing::attach(control, comboContext);
                } else if (!combo && !ownerDrawnButton && !choicePicker) {
                    SendMessageW(control, EM_SETLIMITTEXT, 16 * 1024 * 1024, 0);
                    WorkspaceEditDrawing::attach(control, comboContext);
                }
                SetWindowSubclass(control, controlProcedure, 2, reinterpret_cast<DWORD_PTR>(this));
                rows.push_back({field, control});
                if (field->swatches) {
                    if (previous != previousRows.end()) rows.back().swatches = previous->swatches;
                    else for (int j = 0; j < 16; ++j) {
                        const HWND button = CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_TABSTOP | BS_OWNERDRAW,
                            0, 0, 1, 1, viewport, nullptr, GetModuleHandleW(nullptr), nullptr);
                        SetPropW(button, L"RFF.Swatch.Index", HANDLE(INT_PTR(j + 1)));
                        SetWindowSubclass(button, controlProcedure, 2, reinterpret_cast<DWORD_PTR>(this));
                        rows.back().swatches.push_back(button);
                    }
                }
                if (pickable) {
                    auto &picker = rows.back().picker;
                    picker = previous == previousRows.end()
                                 ? CreateWindowExW(0, L"BUTTON",
                                                   (field->editor == FormField::Editor::COLOR ||
                                                    field->editor == FormField::Editor::RGB_COLOR) ? L"Color..."
                                                                                             : L"Browse...",
                                                   WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON, pickerLeft(r.right),
                                                   y, px(FormLayout::pickerWidth), px(28), viewport, nullptr,
                                                   GetModuleHandleW(nullptr), nullptr)
                                 : previous->picker;
                    SetWindowPos(picker, nullptr, pickerLeft(r.right), y, px(FormLayout::pickerWidth), px(28),
                                 SWP_NOZORDER | SWP_NOACTIVATE);
                    WorkspaceButton::attach(picker);
                    SendMessageW(picker, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
                    applyDarkThemeClass(picker, false);
                    SetWindowSubclass(picker, controlProcedure, 2, reinterpret_cast<DWORD_PTR>(this));
                    AccessibleControl::describe(picker, L"Choose " + field->label, field->hint,
                                                std::wstring(field->id.begin(), field->id.end()) +
                                                    L".choose");
                }
                const auto pending = draft.find(field->id);
                if (GetFocus() != control || previous == previousRows.end() || pending == draft.end()) {
                    setValue(rows.back(), pending == draft.end() ? field->read() : pending->second);
                }
                if (previous != previousRows.end()) {
                    previousRows.erase(previous);
                }
                applyDarkThemeClass(control, combo);
            }
            for (const auto &row : previousRows) {
                for (HWND swatch : row.swatches) DestroyWindow(swatch);
                if (row.slider) DestroyWindow(row.slider);
                DestroyWindow(row.control);
                if (row.picker) {
                    DestroyWindow(row.picker);
                }
            }
            if (query.empty()) {
                for (size_t i = 0; i < form.actions.size(); ++i) {
                    const auto &action = form.actions[i];
                    if (action.group != group) {
                        continue;
                    }
                    const int actionsTop = FormLayout::actionsTop(int(rows.size()));
                    const auto previous =
                        std::find_if(previousActions.begin(), previousActions.end(),
                                     [i](HWND button) { return GetDlgCtrlID(button) == int(100 + i); });
                    HWND control =
                        previous == previousActions.end()
                            ? CreateWindowExW(
                                  0, L"BUTTON", action.label.c_str(), WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
                                  px(20),
                                  px(actionsTop + int(actionControls.size()) * FormLayout::actionPitch),
                                  std::max(1, int(r.right) - px(40)), px(32), viewport,
                                  reinterpret_cast<HMENU>(100 + i), GetModuleHandleW(nullptr), nullptr)
                            : *previous;
                    SetWindowPos(control, nullptr, px(20),
                                 px(actionsTop + int(actionControls.size()) * FormLayout::actionPitch),
                                 std::max(1, int(r.right) - px(40)), px(32), SWP_NOZORDER | SWP_NOACTIVATE);
                    WorkspaceButton::attach(control);
                    if (previous != previousActions.end()) {
                        previousActions.erase(previous);
                    }
                    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
                    applyDarkThemeClass(control, false);
                    SetWindowSubclass(control, controlProcedure, 2, reinterpret_cast<DWORD_PTR>(this));
                    actionControls.push_back(control);
                }
            }
            for (HWND action : previousActions) {
                DestroyWindow(action);
            }
            SetWindowPos(groupControl, nullptr, px(20), px(12), std::max(1, int(r.right) - px(40)), px(300),
                         SWP_NOZORDER | SWP_NOACTIVATE);
            SendMessageW(groupControl, CB_SETCURSEL, group, 0);
            updateFeedback();
            refreshErrors();
            for (HWND control : paused) {
                if (IsWindow(control)) {
                    SendMessageW(control, WM_SETREDRAW, TRUE, 0);
                }
            }
            for (const auto &row : rows) {
                if (row.slider) ShowWindow(row.slider, SW_SHOWNA);
                ShowWindow(row.control, row.field->swatches ? SW_HIDE : SW_SHOWNA);
                if (row.picker) {
                    ShowWindow(row.picker, SW_SHOWNA);
                }
            }
            for (HWND control : actionControls) {
                ShowWindow(control, SW_SHOWNA);
            }
            ShowWindow(groupControl, compact && query.empty() ? SW_SHOWNA : SW_HIDE);
            if (visibleWindow) {
                SendMessageW(window, WM_SETREDRAW, TRUE, 0);
            }
            syncing = false;
            rebuilding = false;
            reveal(GetFocus());
            RedrawWindow(window, nullptr, nullptr,
                         RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
            if (changed) {
                changed();
            }
        }
        void apply() {
            finishSlider(true);
            if (draft.empty()) {
                return;
            }
            if (form.canEdit && !form.canEdit()) {
                message = L"An export is running. Wait or cancel before applying settings.";
                placeContent();
                invalidate();
                return;
            }
            errors.clear();
            message.clear();
            validationAttempted = true;
            updateFeedback();
            if (!errors.empty() || !message.empty()) {
                refreshErrors();
                focusError();
                return;
            }
            try {
                message = form.apply(draft);
            } catch (const std::exception &) {
                message = L"Unable to apply these values. Check the entered settings.";
            }
            if (message.empty()) {
                draft.clear();
                validationAttempted = false;
                updateValues(true);
                refreshErrors();
            } else {
                refreshErrors();
                focusError();
            }
        }
        void moveFocus(HWND current, int direction) {
            std::vector<HWND> controls;
            if (compact && query.empty()) {
                controls.push_back(groupControl);
            }
            for (const auto &row : rows) {
                controls.push_back(row.control);
                controls.insert(controls.end(), row.swatches.begin(), row.swatches.end());
                if (row.slider) controls.push_back(row.slider);
                if (row.picker) {
                    controls.push_back(row.picker);
                }
            }
            controls.insert(controls.end(), actionControls.begin(), actionControls.end());
            controls.push_back(applyControl);
            controls.push_back(discardControl);
            std::erase_if(controls, [](HWND control) {
                return !IsWindowEnabled(control) || !(GetWindowLongPtrW(control, GWL_STYLE) & WS_VISIBLE);
            });
            const auto found = std::find(controls.begin(), controls.end(), current);
            const int next = found == controls.end() ? (direction > 0 ? 0 : int(controls.size()) - 1)
                                                     : int(found - controls.begin()) + direction;
            if (next < 0 || next >= int(controls.size())) {
                if (leaveFocus) {
                    leaveFocus(direction);
                }
                return;
            }
            reveal(controls[next]);
            SetFocus(controls[next]);
        }
        void paintFooter(HDC dc, int top) const {
            const int right = contentBounds().right;
            PanelDrawing::fill(dc, {px(20), top, right - px(20), top + 1}, theme.track);
            const std::wstring status = statusText();
            const int saved = SaveDC(dc);
            if (saved == 0) {
                return;
            }
            try {
                SelectObject(dc, font);
                SetBkMode(dc, TRANSPARENT);
                SetTextColor(dc, message.empty() && errors.empty() ? theme.secondary : theme.error);
                RECT text{px(20), top + px(50), right - px(20), top + footerExtent - px(12)};
                UiLanguage::drawText(dc, status.c_str(), int(status.size()), &text, DT_WORDBREAK | DT_NOPREFIX);
            } catch (...) {
                RestoreDC(dc, saved);
                throw;
            }
            RestoreDC(dc, saved);
        }
        void paintContent(HDC dc) const {
            if (rebuilding) {
                return;
            }
            const auto r = contentBounds();
            PanelDrawing::fill(dc, r, theme.background);
            for (size_t i = 0; i < rows.size(); ++i) {
                const auto &layout = rowLayouts[i];
                const int y = rowTop(int(i));
                if (i > 0 && rows[i].field->divider) {
                    const int rule = y - px(FormLayout::dividerSpace / 2);
                    PanelDrawing::fill(dc, {px(20), rule, r.right - px(20), rule + 1}, theme.track);
                }
                if (y + layout.height <= 0 || y >= r.bottom) {
                    continue;
                }
                const auto &field = *rows[i].field;
                const int saved = SaveDC(dc);
                if (saved == 0) {
                    continue;
                }
                try {
                    SelectObject(dc, font);
                    SetBkMode(dc, TRANSPARENT);
                    SetTextColor(dc, IsWindowEnabled(rows[i].control) && !dimmed(field) ? theme.foreground
                                                                                       : theme.secondary);
                    const auto label = field.label + (draft.contains(field.id) ? L" *" : L"");
                    RECT labelRect{px(20), y, r.right - px(20), y + layout.labelHeight};
                    UiLanguage::drawText(dc, label.c_str(), int(label.size()), &labelRect,
                                         DT_WORDBREAK | DT_NOPREFIX);
                    const auto error = errors.find(field.id);
                    const bool invalid = error != errors.end();
                    const auto &help = invalid ? error->second : visibleHintFor(field);
                    SetTextColor(dc, invalid ? theme.error : theme.secondary);
                    RECT hint{px(20), y + layout.hintOffset, r.right - px(20), y + layout.height - px(12)};
                    UiLanguage::drawText(dc, help.c_str(), int(help.size()), &hint, DT_WORDBREAK | DT_NOPREFIX);
                    if (field.previewColors) {
                        const int width = std::max(1L, r.right - px(40));
                        const auto colors = field.previewColors(draft, width);
                        const int top = inputTop(int(i)) + px(32) + (rows[i].slider ? px(32) : 0);
                        for (int x = 0; x < int(colors.size()) && x < width; ++x) {
                            SetDCBrushColor(dc, COLORREF(colors[x]));
                            RECT column{px(20) + x, top, px(20) + x + 1, top + px(20)};
                            FillRect(dc, &column, HBRUSH(GetStockObject(DC_BRUSH)));
                        }
                    }
                } catch (...) {
                    RestoreDC(dc, saved);
                    throw;
                }
                RestoreDC(dc, saved);
            }
            if (rows.empty() && actionControls.empty()) {
                PanelDrawing::text(dc, L"No matching settings",
                                   {px(20), px(12) - scroll, r.right - px(20), px(44) - scroll},
                                   theme.secondary, font);
            }
            if (flowingFooter) {
                paintFooter(dc, footerTop());
            }
        }
        void drawCheckbox(const DRAWITEMSTRUCT &item, const FormField &field) const {
            const bool on = checked(item.hwndItem);
            const bool focused = (item.itemState & ODS_FOCUS) != 0;
            const bool faded = !IsWindowEnabled(item.hwndItem) || dimmed(field);
            PanelDrawing::fill(item.hDC, item.rcItem, theme.background);
            const int size = px(16);
            const int top = int(item.rcItem.top + item.rcItem.bottom - size) / 2;
            const RECT box{item.rcItem.left + px(2), top, item.rcItem.left + px(2) + size, top + size};
            PanelDrawing::fill(item.hDC, box, on ? theme.accent : theme.field);
            PanelDrawing::border(item.hDC, box, on || focused ? theme.accent : theme.track, px(1));
            if (on) {
                drawCheckMark(item.hDC, box, theme.background);
            }
            if (focused) {
                RECT ring = box;
                InflateRect(&ring, px(2), px(2));
                PanelDrawing::border(item.hDC, ring, theme.accent, px(1));
            }
            // The box is followed by the state it stands for, so On and Off read the same as in a list.
            const RECT text{box.right + px(8), item.rcItem.top, item.rcItem.right, item.rcItem.bottom};
            PanelDrawing::text(item.hDC, field.choices[on ? 1 : 0].label, text,
                               faded ? theme.secondary : theme.foreground, font);
        }
        void drawCheckMark(HDC dc, const RECT &box, COLORREF color) const {
            const int size = box.right - box.left;
            const POINT mark[3] = {{box.left + size * 4 / 16, box.top + size * 8 / 16},
                                   {box.left + size * 7 / 16, box.top + size * 11 / 16},
                                   {box.left + size * 12 / 16, box.top + size * 5 / 16}};
            const HPEN pen = CreatePen(PS_SOLID, std::max(1, px(2)), color);
            const HGDIOBJ previous = SelectObject(dc, pen);
            Polyline(dc, mark, 3);
            SelectObject(dc, previous);
            DeleteObject(pen);
        }
        void paint(HDC dc) const {
            const auto r = bounds();
            PanelDrawing::fill(dc, r, theme.background);
            const int right = contentBounds().right;
            if (!query.empty()) {
                PanelDrawing::text(dc, L"Search " + form.title, {px(20), px(12), right - px(20), px(44)},
                                   theme.foreground, font);
            }
            if (query.empty() && !compact) {
                PanelDrawing::text(dc, form.groups[group], {px(20), px(12), right - px(20), px(44)},
                                   theme.foreground, font);
            }
            PanelDrawing::fill(dc, {px(20), px(51), right - px(20), px(52)}, theme.track);
            if (!flowingFooter) {
                paintFooter(dc, footerTop());
            }
        }
        static LRESULT CALLBACK contentProcedure(HWND handle, UINT message, WPARAM w, LPARAM l) {
            auto *self = reinterpret_cast<FormWorkspace *>(GetWindowLongPtrW(handle, GWLP_USERDATA));
            if (message == WM_NCCREATE) {
                self = static_cast<FormWorkspace *>(reinterpret_cast<CREATESTRUCTW *>(l)->lpCreateParams);
                self->viewport = handle;
                SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            if (!self) {
                return DefWindowProcW(handle, message, w, l);
            }
            switch (message) {
            case WM_ERASEBKGND:
                return 1;
            case WM_SETFOCUS:
                self->moveFocus(nullptr, 1);
                return 0;
            case WM_MOUSEWHEEL:
                self->wheel(w);
                return 0;
            case WM_VSCROLL: {
                SCROLLINFO info{sizeof(info), SIF_TRACKPOS};
                GetScrollInfo(handle, SB_VERT, &info);
                self->scrollContent(LOWORD(w), info.nTrackPos);
                return 0;
            }
            case WM_COMMAND:
            case WM_HSCROLL:
            case WM_DRAWITEM:
            case WM_MEASUREITEM:
            case WM_CTLCOLORSTATIC:
            case WM_CTLCOLORLISTBOX:
            case WM_CTLCOLOREDIT:
                return SendMessageW(self->window, message, w, l);
            case WM_PRINTCLIENT:
                self->paintContent(reinterpret_cast<HDC>(w));
                return 0;
            case WM_PAINT: {
                if (self->rebuilding) {
                    ValidateRect(handle, nullptr);
                    return 0;
                }
                PAINTSTRUCT ps;
                HDC target = BeginPaint(handle, &ps);
                try {
                    const auto r = self->contentBounds();
                    if (HDC dc = self->contentBuffer.begin(target, r.right, r.bottom)) {
                        self->paintContent(dc);
                        self->contentBuffer.present(target);
                    }
                } catch (...) {
                    EndPaint(handle, &ps);
                    throw;
                }
                EndPaint(handle, &ps);
                return 0;
            }
            }
            return DefWindowProcW(handle, message, w, l);
        }
        // Shows a new choice in the picker and reports it the way a drop-down reports a selection.
        void choose(HWND control, int choice) {
            if (choice < 0 || choice == storedIndex(control, choiceProperty)) {
                return;
            }
            storeIndex(control, choiceProperty, choice);
            InvalidateRect(control, nullptr, FALSE);
            SendMessageW(GetParent(control), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(control), CBN_SELCHANGE),
                         reinterpret_cast<LPARAM>(control));
        }
        void paintPicker(HWND control, const FormField &field) const {
            PAINTSTRUCT paintState;
            const HDC target = BeginPaint(control, &paintState);
            RECT client;
            GetClientRect(control, &client);
            // Drawn off screen first so a click never shows the parts half repainted.
            const HDC dc = CreateCompatibleDC(target);
            const HBITMAP bitmap = CreateCompatibleBitmap(target, std::max(1L, client.right), std::max(1L, client.bottom));
            const HGDIOBJ previous = SelectObject(dc, bitmap);
            ChoicePicker::paint(dc, field, client, storedIndex(control, choiceProperty),
                                storedIndex(control, hoverProperty), GetFocus() == control, IsWindowEnabled(control),
                                dimmed(field), pickerLook());
            BitBlt(target, 0, 0, client.right, client.bottom, dc, 0, 0, SRCCOPY);
            SelectObject(dc, previous);
            DeleteObject(bitmap);
            DeleteDC(dc);
            EndPaint(control, &paintState);
        }
        static LRESULT CALLBACK pickerProcedure(HWND control, UINT message, WPARAM w, LPARAM l) {
            auto *self = reinterpret_cast<FormWorkspace *>(GetWindowLongPtrW(control, GWLP_USERDATA));
            if (message == WM_NCCREATE) {
                self = static_cast<FormWorkspace *>(reinterpret_cast<CREATESTRUCTW *>(l)->lpCreateParams);
                SetWindowLongPtrW(control, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            const auto *field = reinterpret_cast<const FormField *>(GetPropW(control, L"RFF.Form.Field"));
            if (message == WM_NCDESTROY) {
                RemovePropW(control, choiceProperty);
                RemovePropW(control, hoverProperty);
            }
            if (!self || !field) {
                return DefWindowProcW(control, message, w, l);
            }
            const auto partAt = [&](LPARAM point) {
                RECT client;
                GetClientRect(control, &client);
                const auto parts =
                    ChoicePicker::parts(*field, client, storedIndex(control, choiceProperty), self->pickerLook());
                const int index = ChoicePicker::hit(parts, {GET_X_LPARAM(point), GET_Y_LPARAM(point)});
                return std::make_pair(index, index < 0 ? -1 : parts[size_t(index)].choice);
            };
            switch (message) {
            case WM_ERASEBKGND:
                return 1;
            case WM_PAINT:
                self->paintPicker(control, *field);
                return 0;
            case WM_GETDLGCODE:
                return DLGC_WANTARROWS | DLGC_WANTCHARS;
            case WM_SETFOCUS:
            case WM_KILLFOCUS:
            case WM_ENABLE:
                InvalidateRect(control, nullptr, FALSE);
                break;
            case WM_MOUSEMOVE: {
                const int hovered = partAt(l).first;
                if (hovered != storedIndex(control, hoverProperty)) {
                    storeIndex(control, hoverProperty, hovered);
                    InvalidateRect(control, nullptr, FALSE);
                }
                TRACKMOUSEEVENT leave{sizeof(leave), TME_LEAVE, control, 0};
                TrackMouseEvent(&leave);
                return 0;
            }
            case WM_MOUSELEAVE:
                storeIndex(control, hoverProperty, -1);
                InvalidateRect(control, nullptr, FALSE);
                return 0;
            case WM_LBUTTONDOWN:
                SetFocus(control);
                self->choose(control, partAt(l).second);
                return 0;
            case WM_KEYDOWN: {
                const int current = storedIndex(control, choiceProperty);
                if (field->editor == FormField::Editor::STYLE_TOGGLES && (w == 'B' || w == 'I')) {
                    self->choose(control, std::max(current, 0) ^ (w == 'B' ? 1 : 2));
                    return 0;
                }
                const int next = ChoicePicker::step(*field, current, w);
                if (next != current) {
                    self->choose(control, next);
                    return 0;
                }
                break;
            }
            }
            return DefWindowProcW(control, message, w, l);
        }
        static LRESULT CALLBACK controlProcedure(HWND control, UINT message, WPARAM w, LPARAM l, UINT_PTR id,
                                                 DWORD_PTR data) {
            auto &self = *reinterpret_cast<FormWorkspace *>(data);
            if (message == WM_SETFOCUS) {
                self.editingControl = control;
                self.beforeEditing = self.draft;
                self.reveal(control);
                const auto result = DefSubclassProc(control, message, w, l);
                wchar_t type[32];
                GetClassNameW(control, type, 32);
                if (std::wstring_view(type) == L"Edit") {
                    const int length = GetWindowTextLengthW(control);
                    SendMessageW(control, EM_SETSEL, length, length);
                }
                return result;
            }
            if (message == WM_KEYDOWN && (w == 'Z' || w == 'Y') && (GetKeyState(VK_CONTROL) & 0x8000) &&
                self.form.requestHistory) {
                wchar_t type[32];
                GetClassNameW(control, type, 32);
                if (std::wstring_view(type) != L"Edit") {
                    self.form.requestHistory(w == 'Y');
                    return 0;
                }
            }
            if (message == WM_KEYDOWN && w == VK_TAB) {
                self.finishSlider(true);
                self.moveFocus(control, (GetKeyState(VK_SHIFT) & 0x8000) ? -1 : 1);
                return 0;
            }
            if (message == WM_KEYDOWN && w == VK_ESCAPE) {
                if (self.sliding) { self.finishSlider(false); ReleaseCapture(); }
                else for (auto &row : self.rows) {
                    if (row.control != control) continue;
                    self.syncing = true;
                    if (self.editingControl == control) self.draft = self.beforeEditing;
                    self.draft.erase(row.field->id);
                    std::erase_if(self.errors, [&](const auto &entry) { return !self.draft.contains(entry.first); });
                    self.message.clear();
                    for (const auto &peer : self.rows) {
                        const auto pending = self.draft.find(peer.field->id);
                        self.setValue(peer, pending == self.draft.end() ? peer.field->read() : pending->second);
                    }
                    self.syncing = false;
                    self.updateFeedback();
                    self.refreshErrors();
                    break;
                }
                return 0;
            }
            if (message == WM_KILLFOCUS) self.finishSlider(true);
            if (message == WM_KEYDOWN && (w == VK_LEFT || w == VK_RIGHT || w == VK_UP || w == VK_DOWN || w == VK_PRIOR || w == VK_NEXT)) {
                for (const auto &row : self.rows) if (row.slider == control && row.field->nudge) {
                    const int direction = w == VK_LEFT || w == VK_DOWN || w == VK_NEXT ? -1 : 1;
                    self.applyImmediate(*row.field, row.field->nudge(row.field->read(), direction,
                        w == VK_PRIOR || w == VK_NEXT || (GetKeyState(VK_SHIFT) & 0x8000)));
                    return 0;
                }
            }
            if (message == WM_KEYDOWN && (w == VK_UP || w == VK_DOWN)) {
                for (const auto &row : self.rows) {
                    if (row.control != control || !row.field->nudge) continue;
                    const int length = GetWindowTextLengthW(control);
                    std::wstring current(length + 1, L'\0');
                    GetWindowTextW(control, current.data(), length + 1);
                    current.resize(length);
                    const auto next = row.field->nudge(current, w == VK_UP ? 1 : -1,
                                                       (GetKeyState(VK_SHIFT) & 0x8000) != 0);
                    if (next != current && self.validate(*row.field, next).empty()) {
                        if (!self.applyImmediate(*row.field, next)) SetWindowTextW(control, next.c_str());
                        SendMessageW(control, EM_SETSEL, 0, -1);
                    }
                    return 0;
                }
            }
            if (message == WM_KEYDOWN && w == VK_RETURN) {
                wchar_t type[32];
                GetClassNameW(control, type, 32);
                // Enter applies a checkbox's pending value like a list's, rather than flipping it again.
                const bool checkboxControl = std::any_of(self.rows.begin(), self.rows.end(), [&](const auto &row) {
                    return row.control == control && checkbox(*row.field);
                });
                if (std::wstring_view(type) == L"Button" && !checkboxControl) {
                    SendMessageW(control, BM_CLICK, 0, 0);
                } else if (std::wstring_view(type) != L"ComboBox" ||
                           !SendMessageW(control, CB_GETDROPPEDSTATE, 0, 0)) {
                    for (const auto &row : self.rows) if (row.control == control) {
                        const auto pending = self.draft.find(row.field->id);
                        if (pending != self.draft.end()) self.applyImmediate(*row.field, pending->second);
                        break;
                    }
                } else {
                    return DefSubclassProc(control, message, w, l);
                }
                return 0;
            }
            if (message == WM_CHAR && (w == VK_TAB || w == VK_RETURN || w == VK_ESCAPE)) {
                return 0;
            }
            if (message == WM_GETDLGCODE) {
                return DefSubclassProc(control, message, w, l) | DLGC_WANTTAB;
            }
            if (message == WM_NCDESTROY) {
                RemovePropW(control, checkedProperty);
                RemovePropW(control, dimmedProperty);
                RemoveWindowSubclass(control, controlProcedure, id);
            }
            return DefSubclassProc(control, message, w, l);
        }
        static LRESULT CALLBACK procedure(HWND handle, UINT message, WPARAM w, LPARAM l) {
            auto *self = reinterpret_cast<FormWorkspace *>(GetWindowLongPtrW(handle, GWLP_USERDATA));
            if (message == WM_NCCREATE) {
                self = static_cast<FormWorkspace *>(reinterpret_cast<CREATESTRUCTW *>(l)->lpCreateParams);
                self->window = handle;
                SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            if (!self) {
                return DefWindowProcW(handle, message, w, l);
            }
            switch (message) {
            case WM_ERASEBKGND:
                return 1;
            case WM_SETFOCUS:
                self->moveFocus(nullptr, 1);
                return 0;
            case WM_TIMER:
                if (self->sliding) return 0;
                if (IsWindowVisible(handle)) {
                    if (self->descriptionsShown != PreferencesIO::showSettingDescriptions()) {
                        self->descriptionsShown = PreferencesIO::showSettingDescriptions();
                        self->placeContent();
                        self->invalidate(false);
                    }
                    self->updateValues();
                }
                return 0;
            case WM_SIZE:
                if (self->discardControl && !self->rebuilding) {
                    self->placeContent();
                    self->invalidate(false);
                }
                return 0;
            case WM_MEASUREITEM:
                reinterpret_cast<MEASUREITEMSTRUCT *>(l)->itemHeight = self->px(20);
                return TRUE;
            case WM_DRAWITEM: {
                const auto *item = reinterpret_cast<DRAWITEMSTRUCT *>(l);
                const auto *colorField = reinterpret_cast<FormField *>(GetPropW(item->hwndItem, L"RFF.Form.Field"));
                if (item->CtlType == ODT_BUTTON && colorField && checkbox(*colorField)) {
                    self->drawCheckbox(*item, *colorField);
                    return TRUE;
                }
                if (item->CtlType == ODT_BUTTON && colorField &&
                    (colorField->editor == FormField::Editor::COLOR || colorField->editor == FormField::Editor::RGB_COLOR)) {
                    const auto pending = self->draft.find(colorField->id);
                    const auto value = pending == self->draft.end() ? colorField->read() : pending->second;
                    glm::vec4 color(1);
                    const bool valid = colorField->editor == FormField::Editor::RGB_COLOR
                        ? AttributeFormModel::parseColorRgb(value, color) : AttributeFormModel::parseColor(value, color);
                    PanelDrawing::fill(item->hDC, item->rcItem, self->theme.field);
                    RECT inner = item->rcItem;
                    PanelDrawing::border(item->hDC, inner, item->itemState & (ODS_FOCUS | ODS_SELECTED) ? self->theme.accent : self->theme.track, self->px(1));
                    InflateRect(&inner, -self->px(4), -self->px(4));
                    if (valid) PanelDrawing::fill(item->hDC, inner,
                        RGB(BYTE(std::clamp(color.r, 0.f, 1.f) * 255 + .5f), BYTE(std::clamp(color.g, 0.f, 1.f) * 255 + .5f), BYTE(std::clamp(color.b, 0.f, 1.f) * 255 + .5f)));
                    return TRUE;
                }
                if (item->CtlType == ODT_COMBOBOX) {
                    WorkspaceComboDrawing::draw(*item, self->comboContext, self->px(6));
                    return TRUE;
                }
                if (item->CtlType == ODT_BUTTON && GetPropW(item->hwndItem, L"RFF.Swatch.Index")) {
                    PanelDrawing::fill(item->hDC, item->rcItem, self->theme.field);
                    RECT edge = item->rcItem;
                    PanelDrawing::border(item->hDC, edge, item->itemState & (ODS_FOCUS | ODS_SELECTED) ? self->theme.accent : self->theme.track, self->px(1));
                    RECT color = item->rcItem;
                    InflateRect(&color, -self->px(5), -self->px(5));
                    const COLORREF sampled = COLORREF(UINT_PTR(GetPropW(item->hwndItem, L"RFF.Swatch.Color")) - 1);
                    const COLORREF rgb = sampled == CLR_INVALID ? self->theme.field : sampled;
                    PanelDrawing::fill(item->hDC, color, rgb);
                    const auto label = sampled == CLR_INVALID ? L"?" : L"";
                    SetBkMode(item->hDC, TRANSPARENT);
                    SetTextColor(item->hDC, int(GetRValue(rgb)) + GetGValue(rgb) + GetBValue(rgb) > 384 ? RGB(0, 0, 0) : RGB(255, 255, 255));
                    const auto oldFont = SelectObject(item->hDC, self->font);
                    DrawTextW(item->hDC, label, -1, &color, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    SelectObject(item->hDC, oldFont);
                    return TRUE;
                }
                if (item->CtlType == ODT_BUTTON) {
                    WorkspaceButton::draw(*item, self->comboContext);
                    return TRUE;
                }
                break;
            }
            case WM_CTLCOLORSTATIC:
            case WM_CTLCOLORLISTBOX:
            case WM_CTLCOLOREDIT:
                SetTextColor(reinterpret_cast<HDC>(w), IsWindowEnabled(reinterpret_cast<HWND>(l)) ? self->theme.foreground : self->theme.secondary);
                SetBkColor(reinterpret_cast<HDC>(w), self->theme.field);
                return reinterpret_cast<LRESULT>(self->brush);
            case WM_HSCROLL:
                self->slide(reinterpret_cast<HWND>(l), LOWORD(w));
                return 0;
            case WM_COMMAND:
                if (self->syncing) {
                    return 0;
                }
                if (reinterpret_cast<HWND>(l) == self->groupControl && HIWORD(w) == CBN_SELCHANGE) {
                    self->selectGroup(int(SendMessageW(self->groupControl, CB_GETCURSEL, 0, 0)));
                    return 0;
                }
                if (reinterpret_cast<HWND>(l) == self->applyControl && HIWORD(w) == BN_CLICKED) {
                    self->apply();
                    return 0;
                }
                if (reinterpret_cast<HWND>(l) == self->discardControl && HIWORD(w) == BN_CLICKED) {
                    self->discardPending();
                    return 0;
                }
                for (auto &row : self->rows) {
                    if (row.control == reinterpret_cast<HWND>(l) && HIWORD(w) == BN_CLICKED && checkbox(*row.field)) {
                        self->toggle(row);
                        return 0;
                    }
                    if (row.control == reinterpret_cast<HWND>(l) && HIWORD(w) == BN_CLICKED &&
                        (row.field->editor == FormField::Editor::COLOR || row.field->editor == FormField::Editor::RGB_COLOR)) {
                        self->pick(row);
                        return 0;
                    }
                    const auto swatch = std::find(row.swatches.begin(), row.swatches.end(), reinterpret_cast<HWND>(l));
                    if (swatch != row.swatches.end() && HIWORD(w) == BN_CLICKED) {
                        const auto pending = self->draft.find(row.field->id);
                        const auto value = pending == self->draft.end() ? row.field->read() : pending->second;
                        self->applyImmediate(*row.field, row.field->removeSwatch(value, size_t(swatch - row.swatches.begin())));
                        if (!(GetWindowLongPtrW(*swatch, GWL_STYLE) & WS_VISIBLE)) self->moveFocus(*swatch, 1);
                        return 0;
                    }
                    if (row.picker && row.picker == reinterpret_cast<HWND>(l) && HIWORD(w) == BN_CLICKED) {
                        self->pick(row);
                        return 0;
                    }
                }
                for (auto &row : self->rows) {
                    if (row.control == reinterpret_cast<HWND>(l) &&
                        (HIWORD(w) == EN_CHANGE || HIWORD(w) == CBN_SELCHANGE)) {
                        self->capture(row);
                        return 0;
                    }
                }
                if (HIWORD(w) == BN_CLICKED && LOWORD(w) >= 100 &&
                    size_t(LOWORD(w) - 100) < self->form.actions.size()) {
                    if (self->form.canEdit && !self->form.canEdit() &&
                        !self->form.actions[LOWORD(w) - 100].allowDuringJob) {
                        self->message = L"An export is running. Wait or cancel before using this tool.";
                        self->placeContent();
                        self->invalidate();
                        return 0;
                    }
                    if (!self->draft.empty() && !self->form.actions[LOWORD(w) - 100].allowPending) {
                        self->message = L"Apply or discard pending changes before using tools.";
                        self->placeContent();
                        self->invalidate();
                        return 0;
                    }
                    try {
                        self->form.actions[LOWORD(w) - 100].invoke();
                    } catch (const std::exception &) {
                        self->message =
                            L"This tool could not complete. Check the current render and settings.";
                    }
                    self->updateValues();
                    self->invalidate();
                    return 0;
                }
                break;
            case WM_MOUSEWHEEL:
                self->wheel(w);
                return 0;
            case WM_VSCROLL:
                return SendMessageW(self->viewport, message, w, l);
            case WM_PRINTCLIENT:
                self->paint(reinterpret_cast<HDC>(w));
                return 0;
            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC target = BeginPaint(handle, &ps);
                try {
                    const auto r = self->bounds();
                    if (HDC dc = self->buffer.begin(target, r.right, r.bottom)) {
                        self->paint(dc);
                        self->buffer.present(target);
                    }
                } catch (...) {
                    EndPaint(handle, &ps);
                    throw;
                }
                EndPaint(handle, &ps);
                return 0;
            }
            case WM_NCDESTROY:
                KillTimer(handle, 1);
                break;
            }
            return DefWindowProcW(handle, message, w, l);
        }

      public:
        FormWorkspace(HWND parent, WorkspaceForm model, HFONT font, const WorkspaceTheme &theme, float scale,
                      std::function<void()> changed, std::function<void(int)> leaveFocus = {},
                      std::function<void()> resetSearch = {})
            : form(std::move(model)), font(font), theme(theme), scale(scale),
              brush(CreateSolidBrush(theme.field)), comboContext{&theme, font, scale},
              changed(std::move(changed)), leaveFocus(std::move(leaveFocus)),
              resetSearch(std::move(resetSearch)) {
            ChoiceEditors::assign(form.fields);
            WNDCLASSW pickerWindow{};
            pickerWindow.lpfnWndProc = pickerProcedure;
            pickerWindow.hInstance = GetModuleHandleW(nullptr);
            pickerWindow.lpszClassName = pickerClass;
            pickerWindow.hCursor = LoadCursor(nullptr, IDC_HAND);
            RegisterClassW(&pickerWindow);
            WNDCLASSW cls{};
            cls.lpfnWndProc = procedure;
            cls.hInstance = GetModuleHandleW(nullptr);
            cls.lpszClassName = L"RFF.Workspace.Form";
            cls.hCursor = LoadCursor(nullptr, IDC_ARROW);
            RegisterClassW(&cls);
            window =
                CreateWindowExW(WS_EX_COMPOSITED, cls.lpszClassName, UiLanguage::text(form.title).c_str(),
                                WS_CHILD | WS_CLIPCHILDREN, 0, 0, 1, 1, parent, nullptr, cls.hInstance, this);
            applyDarkThemeClass(window, false);
            cls.lpfnWndProc = contentProcedure;
            cls.lpszClassName = L"RFF.Workspace.FormContent";
            RegisterClassW(&cls);
            viewport = CreateWindowExW(0, cls.lpszClassName, L"Settings",
                                       WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_VSCROLL, 0, 0, 1, 1,
                                       window, nullptr, cls.hInstance, this);
            applyDarkThemeClass(viewport, false);
            groupControl = CreateWindowExW(0, L"COMBOBOX", L"",
                                           WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST |
                                               CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL,
                                           0, 0, 1, 1, window, nullptr, cls.hInstance, nullptr);
            for (const auto &label : form.groups) {
                SendMessageW(groupControl, CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>(UiLanguage::text(label).c_str()));
            }
            SendMessageW(groupControl, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
            WorkspaceComboDrawing::attach(groupControl, comboContext);
            applyControl = CreateWindowExW(0, L"BUTTON", L"Apply && Render",
                                           WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 1, 1,
                                           window, nullptr, cls.hInstance, nullptr);
            discardControl =
                CreateWindowExW(0, L"BUTTON", L"Discard", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                0, 0, 1, 1, window, nullptr, cls.hInstance, nullptr);
            for (HWND control : {groupControl, applyControl, discardControl}) {
                SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
                SetWindowSubclass(control, controlProcedure, 2, reinterpret_cast<DWORD_PTR>(this));
            }
            AccessibleControl::describe(groupControl, form.title + L" section",
                                        L"Choose the settings section to edit.", L"workspace.section");
            WorkspaceButton::attach(applyControl, true);
            WorkspaceButton::attach(discardControl);
            SetTimer(window, 1, 500, nullptr);
        }
        ~FormWorkspace() {
            finishSlider(false);
            if (IsWindow(window)) {
                DestroyWindow(window);
            }
            DeleteObject(brush);
        }
        const std::vector<std::wstring> &groups() const {
            return form.groups;
        }
        const std::wstring &title() const {
            return form.title;
        }
        WorkspaceForm sharedForm() const {
            return form;
        }
        const std::vector<FormField> &searchFields() const {
            return form.fields;
        }
        std::wstring searchValue(const FormField &field) const {
            const auto pending = draft.find(field.id);
            const auto value = pending == draft.end() ? field.read() : pending->second;
            const auto choice = std::find_if(field.choices.begin(), field.choices.end(),
                                             [&](const auto &item) { return item.value == value; });
            return choice == field.choices.end() ? value : choice->label;
        }
        std::wstring searchState(const FormField &field) const {
            if (form.canEdit && !form.canEdit()) {
                return L"Busy";
            }
            std::wstring state;
            for (const auto &toggle : form.fields) {
                if (toggle.group == field.group &&
                    (toggle.id.ends_with(".enabled") || toggle.id.ends_with(".use")) &&
                    toggle.choices.size() == 2) {
                    const auto value = toggle.read();
                    if (value == L"0" || value == L"1") {
                        state = value == L"0" ? L"Effect off" : L"Effect on";
                        break;
                    }
                }
            }
            if (draft.contains(field.id)) {
                return state.empty() ? L"Pending" : state + L" · Pending";
            }
            return state.empty() ? L"Editable" : state;
        }
        bool revealField(std::string_view id, bool focus = true) {
            const auto field = std::find_if(form.fields.begin(), form.fields.end(),
                                            [&](const auto &value) { return value.id == id; });
            if (field == form.fields.end()) {
                return false;
            }
            const bool rebuildNeeded = !query.empty() || group != field->group;
            query.clear();
            group = field->group;
            if (rebuildNeeded) {
                rebuild();
            }
            const auto row = std::find_if(rows.begin(), rows.end(),
                                          [&](const auto &value) { return value.field == &*field; });
            if (row == rows.end()) {
                return false;
            }
            if (focus) {
                SetFocus(row->control);
            }
            reveal(row->control);
            return true;
        }
        HWND handle() const {
            return window;
        }
        int selectedGroup() const {
            return group;
        }
        int scrollPosition() const {
            return scroll;
        }
        void restoreScrollPosition(int position) {
            scroll = position;
            placeContent(false);
        }
        void selectGroup(int selected) {
            group = std::clamp(selected, 0, int(form.groups.size()) - 1);
            scroll = 0;
            message.clear();
            rebuild();
        }
        void search(std::wstring value) {
            if (query == value) {
                return;
            }
            query = std::move(value);
            scroll = 0;
            rebuild();
        }
        void layoutAt(int x, int y, int width, int height, bool narrow = false) {
            const bool changed = compact != narrow;
            compact = narrow;
            positionControl(window, x, y, width, height);
            if (changed || rows.empty()) {
                rebuild();
            }
        }
        void layout(int width, int height, bool narrow = false) {
            layoutAt(0, 0, width, height, narrow);
        }
        void show(bool visible) {
            if (!visible) finishSlider(true);
            ShowWindow(window, visible ? SW_SHOWNA : SW_HIDE);
            if (visible) {
                updateValues();
            }
        }
        void focus() {
            focusError();
        }
        bool applyPending(bool documentOnly = false) {
            if (documentOnly && !hasDocumentPending()) {
                return true;
            }
            apply();
            return draft.empty();
        }
        bool hasPending() const {
            return sliding || !draft.empty();
        }
        bool hasDocumentPending() const {
            if (sliding) return true;
            return std::any_of(form.fields.begin(), form.fields.end(), [this](const auto &field) {
                return field.persisted && draft.contains(field.id);
            });
        }
        void discardPending() {
            finishSlider(false);
            draft.clear();
            errors.clear();
            message.clear();
            validationAttempted = false;
            updateValues(true);
            refreshErrors();
        }
        uint64_t historyOrder(bool redo) const {
            const auto &callback = redo ? form.redoOrder : form.undoOrder;
            return callback && (redo ? canRedo() : canUndo()) ? callback() : 0;
        }
        bool canUndo() const {
            return !sliding && (!form.canEdit || form.canEdit()) && draft.empty() && form.canUndo();
        }
        bool canRedo() const {
            return !sliding && (!form.canEdit || form.canEdit()) && draft.empty() && form.canRedo();
        }
        bool undo() {
            if (!canUndo()) {
                return false;
            }
            const bool result = form.undo();
            updateValues(true);
            refreshErrors();
            return result;
        }
        bool redo() {
            if (!canRedo()) {
                return false;
            }
            const bool result = form.redo();
            updateValues(true);
            refreshErrors();
            return result;
        }
        void loaded() {
            draft.clear();
            errors.clear();
            message.clear();
            validationAttempted = false;
            form.clearHistory();
            rebuild();
        }
        void applyTheme() {
            if (const auto replacement = CreateSolidBrush(theme.field)) {
                DeleteObject(brush);
                brush = replacement;
            }
            applyDarkThemeClass(window, false);
            applyDarkThemeClass(viewport, false);
            rebuild();
        }
        void applyMetrics(HFONT replacement, float newScale) {
            scroll = int(scroll * newScale / scale + .5f);
            font = replacement;
            scale = newScale;
            comboContext.font = font;
            comboContext.scale = scale;
            for (HWND control : {groupControl, applyControl, discardControl}) {
                SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
            }
            WorkspaceComboDrawing::applyMetrics(groupControl, comboContext);
            for (const auto &row : rows) {
                SendMessageW(row.control, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
                if (dropDown(*row.field)) {
                    WorkspaceComboDrawing::applyMetrics(row.control, comboContext);
                }
                if (row.picker) {
                    SendMessageW(row.picker, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
                }
            }
            for (const auto &action : actionControls) {
                SendMessageW(action, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
            }
            rebuild();
        }
    };
} // namespace merutilm::rff2::workspace
