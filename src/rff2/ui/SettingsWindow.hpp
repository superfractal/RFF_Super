//
// Created by Merutilm on 2025-05-13.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21, 2026-08-26.
// Modified by Opus 5 on 2026-08-06, 2026-08-11, 2026-08-12, 2026-08-13, 2026-08-14, 2026-08-23, 2026-08-26, 2026-08-31, 2026-09-01
//

#pragma once
#include <any>
#include <functional>
#include <iostream>
#include <string>

#include "../constants/Constants.hpp"
#include "SettingsTheme.hpp"
#include <windows.h>
#include <commctrl.h>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "../attr/Selectable.h"

namespace merutilm::rff2 {
    class SettingsWindow {
        HWND window;
        int count = 0;
        int yCursor = 0;
        std::vector<std::any> references;
        std::vector<std::unique_ptr<std::function<std::any(std::wstring &)> > > parsers;
        std::vector<std::unique_ptr<std::function<std::wstring(const std::any &)> > > unparsers;
        std::vector<std::unique_ptr<std::function<bool(const std::any &)> > > validConditions;
        std::vector<std::unique_ptr<std::function<void(std::any &)> > > callbacks;
        std::vector<std::unique_ptr<std::vector<std::any> > > enumValues;
        std::vector<HWND> createdChildWindows;
        std::unordered_map<int, std::vector<HWND>> rowControlGroups;
        std::unordered_map<HWND, std::function<COLORREF()> > colorSwatches;
        struct SliderBinding {
            HWND trackbar;
            HWND textField;
            int index;        // index into references/callbacks/etc.
            double minValue;  // overall value lower bound
            double maxValue;
            double currentBase; // left edge of the currently shown decade window (10^k)
            bool logScale;
            bool wholeSteps;
            // The small "lowest value" caption under the bar's left end, when the row has one.
            // Held so a zero stop applied after the row was built can correct it.
            HWND minLabel = nullptr;
            // The x range (client coordinates) the trackbar's own thumb center travels over,
            // measured off the control at build time. The face is painted and the min/max captions
            // are aligned to this, so both land exactly where the control's hit-testing puts the
            // thumb: its channel rect is a wider span than the thumb ever reaches.
            int trackLeft = 0;
            int trackRight = 0;
            // Log slider whose bottom window holds 0 as well: that window runs linearly from 0 to
            // minValue*10 instead of logarithmically from minValue. A log axis has nowhere to put 0
            // (log10(0) is -inf), so a control whose 0 is an off switch needs this to reach it.
            bool zeroStop = false;
        };
        std::vector<SliderBinding> sliders;
        std::unordered_map<HWND, int> trackbarToSlider; // trackbar HWND -> sliders[] index
        std::unordered_map<int, int> indexToSlider;     // control index -> sliders[] index
        std::vector<bool> error;
        std::vector<bool> edited;
        std::vector<bool> modified;
        // The value each row writes back to, in row order. Kept so a caller can tell one row's
        // setting from another's by what it is bound to (see disableRowsInObjectExcept).
        std::vector<const void *> valuePointers;
        struct CollapsibleSection {
            HWND header;
            HWND arrow;
            int bodyTop;
            int bodyEnd; // -1 until the next header (or capture) closes the band
            bool collapsed;
        };
        std::vector<CollapsibleSection> sections;
        std::unordered_map<HWND, int> childOriginalTop;
        bool sectionLayoutCaptured = false;
        int expandedContentHeight = 0;
        // Collapsing only earns its keep on a long panel. This latches true once the laid-out
        // content overflows the available height; until then the disclosure arrows stay hidden and
        // headers don't fold. It never reverts (a fold shrinks content but mustn't drop the arrows).
        bool sectionsCollapsible = false;
        // Controls the caller hid via setRowVisible; a section expand must not re-reveal them.
        std::unordered_set<HWND> rowHidden;
        // A deferred surface repaint is already on its way; further row changes need not post one.
        bool repaintPending = false;
        std::unordered_set<HWND> rangeLabels;
        std::unordered_set<HWND> noteLabels;
        std::unordered_map<HWND, std::function<void(HDC, const RECT &)>> rowPreviews;
        // Owner-drawn full-panel "cards" (export dialog): each paints its whole client rect.
        std::unordered_map<HWND, std::function<void(HDC, const RECT &)>> cardPainters;
        // Buttons painted as a filled blue accent button instead of the soft near-white face.
        std::unordered_set<HWND> primaryButtons;
        // Dropdowns the panel paints itself: their closed face and their dropped rows.
        std::unordered_set<HWND> selectionBoxes;
        std::unordered_map<int, double> textFieldArrowSteps;
        std::unordered_map<int, std::pair<double, double>> textFieldDecadeRanges;
        std::function<void()> windowCloseFunction;
        LPARAM font;
        LPARAM headerFont;
        LPARAM smallFont;
        // Per-field fonts (e.g. an enlarged font for a specific row); freed on WM_DESTROY.
        std::vector<HFONT> extraFonts;
        int windowWidth;
        int labelWidth;
        int inputHeight;
        int extraRowGap = 0;
        int topMargin = Constants::Win32::settingsScaled(
            Constants::Win32::GAP_SETTINGS_INPUT / 3 + Constants::Win32::GAP_SETTINGS_INPUT / 2 + 3);
        int scrollY = 0;
        int viewportHeight = 0; // visible client height (== contentHeight when no scrollbar)
        int contentHeight = 0;  // full laid-out content height
        int snapLeft = 0;
        int snapTop = 0;
        int masterLeft = 0;
        int masterRight = 0;
        // Master window's client height, the floor for how tall a panel may grow (see
        // maxSettingsClientHeight).
        int masterHeight = 0;
        // The Light/Dark choice this panel is drawn under, empty while it follows the View menu's.
        std::optional<bool> darkOverride;

    public:
        explicit SettingsWindow(const std::wstring &name,
                                int width = Constants::Win32::INIT_SETTINGS_WINDOW_WIDTH,
                                int labelWidth = -1,
                                int inputHeight = Constants::Win32::SETTINGS_INPUT_HEIGHT);

        ~SettingsWindow();

        SettingsWindow(const SettingsWindow &) = delete;

        SettingsWindow &operator=(const SettingsWindow &) = delete;

        SettingsWindow(SettingsWindow &&) = delete;

        SettingsWindow &operator=(SettingsWindow &&) = delete;


        void setWindowCloseFunction(std::function<void()> &&function);
        template<typename T>
        HWND registerTextInput(const std::wstring &settingsName, T *ptr,
                               std::function<std::wstring(const T &)> &&unparser,
                               std::function<T(std::wstring &)> &&parser,
                               std::function<bool(const T &)> &&validCondition,
                               std::function<void()> &&callback, const std::wstring &descriptionTitle,
                               const std::wstring &descriptionDetail, double arrowStep = 0.0,
                               double decadeMin = 0.0, double decadeMax = 0.0, int fieldFontSize = 0);


        template<typename T> requires std::is_enum_v<T>
        HWND registerSelectionInput(const std::wstring &settingsName, T *ptr, std::function<void()> &&callback,
                                    const std::wstring &descriptionTitle, const std::wstring &descriptionDetail);


        template<typename T> requires std::is_enum_v<T> || std::is_same_v<T, bool>
        std::vector<HWND> registerRadioButtonInput(const std::wstring &settingsName, T *defaultValue,
                                                   std::function<void()> &&callback,
                                                   const std::wstring &descriptionTitle,
                                                   const std::wstring &descriptionDetail);

        HWND registerCheckboxInput(const std::wstring &settingsName, bool *defaultValue,
                                            std::function<void()> &&callback, const std::wstring &descriptionTitle,
                                            const std::wstring &descriptionDetail);

        HWND registerButton(const std::wstring &settingsName, const std::wstring &buttonText,
                            std::function<void()> &&callback,
                            const std::wstring &descriptionTitle, const std::wstring &descriptionDetail);

        HWND registerSliderInput(const std::wstring &settingsName, float *ptr, float minValue, float maxValue,
                                 std::function<std::wstring(const float &)> &&unparser,
                                 std::function<float(std::wstring &)> &&parser,
                                 std::function<bool(const float &)> &&validCondition,
                                 std::function<void()> &&callback,
                                 const std::wstring &descriptionTitle, const std::wstring &descriptionDetail,
                                 const std::wstring &trailingButtonText = L"",
                                 std::function<void()> &&trailingButtonCallback = nullptr);

        // boundValue names the color the row stands for, so a caller can tell one row's setting from
        // another's by what it is bound to (see disableRowsInObjectExcept). The row reaches its color
        // through the two callbacks either way, so leaving it out keeps the row out of that reckoning.
        HWND registerColorButton(const std::wstring &settingsName, const std::wstring &buttonText,
                                 std::function<COLORREF()> &&colorProvider,
                                 std::function<void()> &&callback,
                                 const std::wstring &descriptionTitle, const std::wstring &descriptionDetail,
                                 const void *boundValue = nullptr);

        // Lets a slider step in fractions. Wide ranges snap to whole numbers by default, which
        // suits iterations or degrees but not a short range whose value column shows decimals.
        void setSliderFractionalSteps(HWND textField);

        // Puts 0 on the bar's left end (see SliderBinding::zeroStop). For a log slider whose 0 is an
        // off switch rather than its smallest value.
        void setSliderZeroStop(HWND textField);
        void setFloatValueByField(HWND textField, float value);
        void setRadioValueByGroup(const std::vector<HWND> &items, const std::any &value);
        // Mirrors a checkbox onto a value it did not itself produce, for a row whose backing value
        // can change underneath it (the Texture window switching which layer it edits).
        void setCheckboxValue(HWND checkbox, bool value);
        void setRowPreview(HWND control, std::function<void(HDC, const RECT &)> &&painter);
        void setRowEnabled(HWND control, bool enabled);
        // Disables every row bound to a value inside the given object that is not one of `kept`,
        // leaving rows bound to anything else - a panel's own staging or display values - alone.
        // The Timeline Editor opens a settings panel through this, so the rows it cannot carry as a
        // track are shown where they belong and greyed rather than left out.
        void disableRowsInObjectExcept(const void *object, size_t size,
                                       const std::unordered_set<const void *> &kept);
        void setRowVisible(HWND control, bool visible);

        // Display static text (for information display, no input)
        void registerStaticText(const std::wstring &text);
        // separateFromPrevious closes the previous section's band right at this heading. Pass false
        // on a window's first heading, where there is no band above to close. (It used to also
        // decide whether a rule was drawn above the title; the rows are enclosed by a frame now.)
        void registerSectionHeader(const std::wstring &title, bool separateFromPrevious = true);

        // Loosen (or reset) the vertical gap beneath each subsequently registered row by the
        // given unscaled pixel amount. Apply around a section, then call with 0 to restore.
        void setExtraRowGap(int extraPx);

        HWND registerPrimaryButton(const std::wstring &buttonText, std::function<void()> &&callback,
                                   const std::wstring &descriptionTitle, const std::wstring &descriptionDetail);

        void registerNotesCard(const std::wstring &title,
                               const std::vector<std::pair<std::wstring, std::wstring>> &notes);

        void registerHelpButton(const std::wstring &title,
                                const std::vector<std::pair<std::wstring, std::wstring>> &notes);

        void registerInsertChips(HWND targetField, const std::vector<std::wstring> &chips,
                                 bool replaceWhole);


        [[nodiscard]] HWND getWindow() const;

        void scheduleThemeRefresh() const;

        // Draws this panel under the given Light/Dark choice instead of the View menu's, which is
        // what a panel opened from the Timeline Editor is given so it matches that editor's colors.
        void setDarkOverride(bool dark);

        // The panel behind a window handle, null for a handle that is not one of these panels.
        [[nodiscard]] static SettingsWindow *of(HWND panelWindow);

        // The choice a panel's colors are read under while its procedures run.
        [[nodiscard]] static ScopedSettingsMode scopedMode(const SettingsWindow *panel) {
            return ScopedSettingsMode(panel == nullptr ? std::nullopt : panel->darkOverride);
        }

        // Full-width owner-drawn block whose painter renders its whole client rect (used for cards
        // and custom widgets such as the frozen-color swatch strip).
        // Passing a rowLabel lays it out as an ordinary row instead: the caption takes the name
        // column and the block the value column, so a block that reads as a field (the texture's
        // path display) lines up with the fields around it. A non-positive pixelHeight then takes
        // one standard row height.
        HWND registerOwnerDrawnPanel(int pixelHeight, std::function<void(HDC, const RECT &)> &&painter,
                                     const std::wstring &rowLabel = L"",
                                     const std::wstring &descriptionTitle = L"",
                                     const std::wstring &descriptionDetail = L"");


        static LRESULT CALLBACK settingsWindowProc(HWND window, UINT message, WPARAM wParam,
                                                   LPARAM lParam);

        static LRESULT CALLBACK textFieldProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                                              UINT_PTR uIdSubclass,
                                              DWORD_PTR dwRefData);

        // Attached to every static that carries text: row labels, slider range captions, notes and
        // section headings. See the definition for what the control's own drawing did to them.
        static LRESULT CALLBACK labelProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                                          UINT_PTR uIdSubclass,
                                          DWORD_PTR dwRefData);

        static LRESULT CALLBACK trackbarProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                                             UINT_PTR uIdSubclass,
                                             DWORD_PTR dwRefData);

        // Attached to every button in the panel: they are all BS_OWNERDRAW, and this is what keeps
        // the erase they would otherwise do off the screen. See the definition.
        static LRESULT CALLBACK ownerDrawnButtonProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                                                     UINT_PTR uIdSubclass,
                                                     DWORD_PTR dwRefData);

        // Attached to every dropdown: comctl32 draws a combo box's closed face in system colors no
        // theme here reaches, so the panel takes that face over. See the definition.
        static LRESULT CALLBACK selectionBoxProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                                                 UINT_PTR uIdSubclass,
                                                 DWORD_PTR dwRefData);

        // Row height, theme and owner-drawing a dropdown needs once its items are in place.
        void finishSelectionBox(HWND combobox);

    private:
        // Draw a rounded outline around the rows of every open section, so each group of
        // settings reads as one card instead of a run of rows under a heading.
        void paintSectionFrames(HDC hdc) const;

        // Everything WM_DRAWITEM paints, into dis->hDC. Split out of the message handler so that
        // handler can point dis->hDC at a memory bitmap for the length of the call. Returns false
        // when the item is none of ours and the default handling should run.
        static bool drawOwnerDrawnItem(SettingsWindow &wnd, const DRAWITEMSTRUCT *dis);

        // Repaint the panel's own surface (background + section frames) and bring every row paint
        // still pending into the same frame. See the definition for why it takes two passes.
        void repaintPanel() const;

        // The same repaint, deferred and collapsed to one: for callers that change several rows in
        // a row and must not have each change painted on its own.
        void schedulePanelRepaint();

        // Pay off a deferred repaint now, if one is owed. Called at the end of the message that
        // caused it, so the change lands in the frame the user's click produced rather than in the
        // next one; does nothing when nothing is owed.
        void flushPendingRepaint();

        // Re-center the trackbar's decade window on the value and position the thumb
        // within it. Updates binding.currentBase, so it takes a mutable reference.
        // Decade-window geometry of a log slider. See the definitions in SettingsWindow.cpp.
        static double sliderTopBase(double minValue, double maxValue);

        static double sliderWindowDecades(double base, double maxValue);

        // Lowest value the slider can produce: 0 with a zero stop, its declared minimum otherwise.
        static double sliderFloor(const SliderBinding &binding);

        // Bar fraction <-> value, within the window starting at `base`. Inverses of each other.
        static double sliderWindowFraction(const SliderBinding &binding, double base, double value);

        static double sliderWindowValue(const SliderBinding &binding, double base, double t);

        static void setTrackbarFromValue(SliderBinding &binding, double value);

        // Measure SliderBinding::trackLeft / trackRight off a freshly built trackbar.
        static void measureTrackTravel(HWND bar, int &left, int &right);

        // Paint the whole slider face into hdc, in place of the trackbar's own drawing.
        static void paintSlider(const SliderBinding &binding, HDC hdc);

        void nudgeSliderValue(int sliderIdx, int direction, bool coarse);
        void nudgeTextFieldValue(HWND field, int index, int direction, bool coarse);

        static void applyRoundedRegion(HWND control);

        static void applyNativeControlTheme(HWND control);

        void refreshTheme() const;

        // Vertically center a multiline-but-single-line edit's text via its formatting rect.
        static int editTextTop(HWND edit, const TEXTMETRICW &tm);

        static LRESULT relayoutQuietly(HWND edit, UINT message, WPARAM wParam, LPARAM lParam);

        static void layoutEditText(HWND edit);

        static int getIndex(HWND wnd);

        static bool isCheckbox(HWND wnd);

        static int getRadioIndex(HWND wnd);

        // Checkbox / radio box size, adjusted so it centres exactly on a row. See the definition.
        [[nodiscard]] int rowBoxSize() const;

        [[nodiscard]] int getRadioButtonWidth(const std::wstring &text, int maxWidth) const;

        [[nodiscard]] bool checkIndex(int index) const;

        [[nodiscard]] int getYOffset() const;

        // Advance the layout cursor past one standard-height row, or past an arbitrary
        // pixel-height block (multi-line statics, cards), each followed by a row gap.
        void advanceRow();
        void advancePixels(int usedHeight);

        // Hands one text static over to labelProc.
        void subclassLabel(HWND label) const;

        HWND createLabel(const std::wstring &settingsName, const std::wstring &descriptionTitle,
                         const std::wstring &descriptionDetail, int nw, HFONT labelFont = nullptr);
        void registerRowControls(int index, std::vector<HWND> &&controls);

        HFONT createExtraFont(int fontSize, bool bold);

        template<typename T>
        void registerActions(T *defaultValuePtr,
                             const std::function<std::wstring(const T &)> &unparser,
                             const std::optional<std::function<T(std::wstring &)> > &parser,
                             const std::optional<std::function<bool(const T &)> > &validCondition,
                             const std::function<void()> &callback,
                             std::optional<std::vector<T> > values);


        [[nodiscard]] std::wstring currValueToString(int index) const;

        [[nodiscard]] int getFixedNameWidth() const;

        [[nodiscard]] int getFixedValueWidth() const;

        void adjustWindowHeight();
        // Resize the window / toggle its scrollbar to fit the current `contentHeight`. Split out
        // of adjustWindowHeight so a section collapse can apply a reduced height directly.
        void applyContentSizing();
        // Just the window-resize half of applyContentSizing (no scroll side effects), so a
        // section reflow can resize then position children against a directly-clamped scrollY.
        // keepPosition leaves the window where it is (a toggle must not yank it back to the snap
        // position); the initial layout passes false to snap it flush against the render window.
        void resizeToContent(bool keepPosition);
        // Fold or unfold the section's body rows and reflow everything below it. `changedIndex`
        // is the toggled section, used to repaint only from that section down (avoids a full flash).
        void toggleSection(int sectionIndex);
        void relayoutSections(int changedIndex);
        // Move every laid-out child to its absolute slot for the current collapsed state + scroll,
        // hiding folded / caller-hidden rows. skipUnchanged leaves controls already in place
        // untouched (so a ScrollWindowEx blit that handled the visible rows adds no extra repaint).
        void positionSectionChildren(bool skipUnchanged);
        void captureSectionLayout();
        // True when the control sits inside a currently-collapsed section's folded band.
        [[nodiscard]] bool isControlFolded(HWND control) const;
        void refreshScrollInfo();
        void scrollTo(int newY);
        void callError(int index);
    };


    // DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW
    // DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW
    // DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW
    // DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW
    // DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW  DEFINITION OF RFF SETTINGS WINDOW


    template<typename T>
    HWND SettingsWindow::registerTextInput(const std::wstring &settingsName, T *ptr,
                                           std::function<std::wstring(const T &)> &&unparser,
                                           std::function<T(std::wstring &)> &&parser,
                                           std::function<bool(const T &)> &&validCondition,
                                           std::function<void()> &&callback,
                                           const std::wstring &descriptionTitle,
                                           const std::wstring &descriptionDetail,
                                           const double arrowStep,
                                           const double decadeMin, const double decadeMax,
                                           const int fieldFontSize) {
        const int nw = getFixedNameWidth();
        const int vw = getFixedValueWidth();

        // Optional enlarged font for this row only (label + the box text); 0 = default.
        const HFONT rowFont = fieldFontSize > 0 ? createExtraFont(fieldFontSize, false) : nullptr;

        const int index = count;
        const HWND label = createLabel(settingsName, descriptionTitle, descriptionDetail, nw, rowFont);
        const HWND text = CreateWindowExW(0, WC_EDITW, unparser(*ptr).data(),
                                          Constants::Win32::STYLE_TEXT_FIELD, nw,
                                          getYOffset(), vw,
                                          inputHeight, window,
                                          reinterpret_cast<HMENU>(Constants::Win32::ID_OPTIONS + index),
                                          nullptr,
                                          nullptr);

        SetWindowSubclass(text, textFieldProc, 1, 0);
        SetWindowLongPtr(text, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        SendMessage(text, WM_SETFONT, rowFont ? reinterpret_cast<LPARAM>(rowFont) : font, TRUE);
        SendMessage(text, EM_SETLIMITTEXT, 0, 0);
        // Rounded, borderless field with a little horizontal padding (matches the sliders'
        // numeric boxes and the rounded buttons).
        SendMessage(text, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN,
                    MAKELPARAM(Constants::Win32::settingsScaled(8), Constants::Win32::settingsScaled(8)));
        applyRoundedRegion(text);
        layoutEditText(text);
        createdChildWindows.push_back(text);
        registerRowControls(index, {label, text});
        advanceRow();
        registerActions<T>(ptr, std::move(unparser), std::optional{std::move(parser)},
                           std::optional{std::move(validCondition)},
                           std::move(callback), std::nullopt);
        // registerActions just incremented count, so this field's index is count - 1.
        if (decadeMax > 0.0) {
            textFieldDecadeRanges.emplace(count - 1, std::pair{decadeMin, decadeMax});
        } else if (arrowStep > 0.0) {
            textFieldArrowSteps.emplace(count - 1, arrowStep);
        }
        adjustWindowHeight();

        return text;
    }

    template<typename T> requires std::is_enum_v<T>
    HWND SettingsWindow::registerSelectionInput(const std::wstring &settingsName, T *ptr,
                                                std::function<void()> &&callback,
                                                const std::wstring &descriptionTitle,
                                                const std::wstring &descriptionDetail) {
        const int nw = getFixedNameWidth();
        const int vw = getFixedValueWidth();
        const int index = count;
        const HWND label = createLabel(settingsName, descriptionTitle, descriptionDetail, nw);
        const HWND combobox = CreateWindowExW(0, WC_COMBOBOXW,
                                              L"",
                                              Constants::Win32::STYLE_COMBOBOX, nw,
                                              getYOffset(), vw,
                                              inputHeight *
                                              (Constants::Win32::MAX_AMOUNT_COMBOBOX + 1),
                                              window,
                                              reinterpret_cast<HMENU>(Constants::Win32::ID_OPTIONS + index),
                                              nullptr,
                                              nullptr);
        std::vector<T> values = Selectable::values<T>();
        int defaultValueIndex = 0;

        for (int i = 0; i < values.size(); ++i) {
            SendMessageW(combobox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Selectable::toString(values[i]).data()));
            if (values[i] == *ptr) {
                defaultValueIndex = i;
            }
        }

        SendMessageW(combobox, CB_SETCURSEL, defaultValueIndex, 0);
        finishSelectionBox(combobox);
        auto unparser = [](T v) { return Selectable::toString(v); };
        createdChildWindows.push_back(combobox);
        registerRowControls(index, {label, combobox});
        advanceRow();
        registerActions<T>(ptr, unparser, std::nullopt, std::nullopt, std::move(callback), values);
        adjustWindowHeight();
        return combobox;
    }

    template<typename T> requires std::is_enum_v<T> || std::is_same_v<T, bool>
    std::vector<HWND> SettingsWindow::registerRadioButtonInput(const std::wstring &settingsName, T *defaultValue,
                                                               std::function<void()> &&callback,
                                                               const std::wstring &descriptionTitle,
                                                               const std::wstring &descriptionDetail) {
        const int nw = getFixedNameWidth();
        const int vw = getFixedValueWidth();
        const int index = count;
        const HWND label = createLabel(settingsName, descriptionTitle, descriptionDetail, nw);
        const auto values = Selectable::values<T>();
        auto createdItem = std::vector<HWND>();
        createdItem.reserve(values.size());
        std::vector<HWND> rowControls = {label};

        for (int i = 0; i < values.size(); ++i) {
            const std::wstring text = Selectable::toString(values[i]);
            const HWND item = CreateWindowExW(0, WC_BUTTONW,
                                              text.data(),
                                              Constants::Win32::STYLE_RADIOBUTTON | (i == 0 ? WS_GROUP : 0), nw,
                                              getYOffset(), getRadioButtonWidth(text, vw),
                                              inputHeight, window,
                                              reinterpret_cast<HMENU>(
                                                  Constants::Win32::ID_OPTIONS + i *
                                                  Constants::Win32::ID_OPTIONS_RADIO + count),
                                              nullptr,
                                              nullptr);

            SendMessage(item, WM_SETFONT, font, TRUE);
            SetWindowSubclass(item, ownerDrawnButtonProc, 1, 0);
            createdItem.push_back(item);
            createdChildWindows.push_back(item);
            rowControls.push_back(item);
            advanceRow();
        }
        auto unparser = [](T v) { return Selectable::toString(v); };
        registerRowControls(index, std::move(rowControls));
        registerActions<T>(defaultValue, unparser, std::nullopt, std::nullopt, std::move(callback),
                           values);
        adjustWindowHeight();
        // Force an owner-draw pass now that the selection is registered in references[].
        for (const HWND item: createdItem) {
            InvalidateRect(item, nullptr, TRUE);
        }
        return createdItem;
    }


    inline HWND SettingsWindow::registerCheckboxInput(const std::wstring &settingsName, bool *defaultValue,
                                                               std::function<void()> &&callback,
                                                               const std::wstring &descriptionTitle,
                                                               const std::wstring &descriptionDetail) {
        const int nw = getFixedNameWidth();
        // The checkbox control is sized to the box itself (not the full value column)
        // so the checked-state highlight color only covers the square + checkmark.
        const int cb = rowBoxSize();
        const int cbYOffset = getYOffset() + (inputHeight - cb) / 2;
        const int index = count;
        const HWND label = createLabel(settingsName, descriptionTitle, descriptionDetail, nw);
        const auto item = CreateWindowExW(0, WC_BUTTONW, L"", Constants::Win32::STYLE_CHECKBOX, nw, cbYOffset, cb,
                                    cb, window, reinterpret_cast<HMENU>(
                                        Constants::Win32::ID_OPTIONS + Constants::Win32::ID_OPTIONS_CHECKBOX_FLAG + index), nullptr, nullptr);

        SendMessage(item, WM_SETFONT, font, TRUE);
        SetWindowSubclass(item, ownerDrawnButtonProc, 1, 0);
        createdChildWindows.push_back(item);
        registerRowControls(index, {label, item});
        auto unparser = [](const bool v) { return Selectable::toString(v); };
        advanceRow();
        registerActions<bool>(defaultValue, unparser, std::nullopt, std::nullopt, std::move(callback), std::nullopt);
        adjustWindowHeight();
        InvalidateRect(item, nullptr, TRUE);
        return item;
    }


    inline HWND SettingsWindow::getWindow() const {
        return window;
    }

    template<typename T>
    void SettingsWindow::registerActions(T *defaultValuePtr, const std::function<std::wstring(const T &)> &unparser,
                                         const std::optional<std::function<T(std::wstring &)> > &parser,
                                         const std::optional<std::function<bool(const T &)> > &validCondition,
                                         const std::function<void()> &callback,
                                         const std::optional<std::vector<T> > values) {
        const std::any defaultValue = *defaultValuePtr;
        references.emplace_back(defaultValue);
        unparsers.emplace_back(std::make_unique<std::function<std::wstring(const std::any &)> >(
            [unparser](const std::any &value) {
                return unparser(std::any_cast<T>(value));
            }
        ));

        parsers.emplace_back(parser == std::nullopt
                                 ? nullptr
                                 : std::make_unique<std::function<std::any(std::wstring &)> >(
                                     [parser](std::wstring &value) {
                                         auto f = *parser;
                                         std::any result = f(value);
                                         return result;
                                     }));


        validConditions.emplace_back(validCondition == std::nullopt
                                         ? nullptr
                                         : std::make_unique<std::function<bool(const std::any &)> >(
                                             [validCondition](const std::any &value) {
                                                 auto f = *validCondition;
                                                 return f(std::any_cast<T>(value));
                                             }
                                         )
        );

        callbacks.emplace_back(std::make_unique<std::function<void(std::any &v)> >(
            [defaultValuePtr, callback](std::any &v) {
                *defaultValuePtr = std::any_cast<T &>(v);
                callback();
            }
        ));
        enumValues.emplace_back(values == std::nullopt
                                    ? nullptr
                                    : [&values] {
                                        auto result = std::make_unique<std::vector<std::any> >();
                                        result->reserve((*values).size());
                                        for (const auto &value: *values) {
                                            result->push_back(value);
                                        }
                                        return result;
                                    }());
        error.emplace_back(false);
        edited.emplace_back(false);
        modified.emplace_back(false);
        valuePointers.emplace_back(static_cast<const void *>(defaultValuePtr));
        ++count;
    }
}
