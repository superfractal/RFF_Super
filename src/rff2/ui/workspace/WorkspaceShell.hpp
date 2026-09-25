//
// Modified by GPT-6 on 2026-09-13, 2026-09-14, 2026-09-15, 2026-09-16, 2026-09-17, 2026-09-18, 2026-09-19, 2026-09-20, 2026-09-21, 2026-09-22, 2026-09-23, 2026-09-24, 2026-09-25
// Modified by GPT-5 on 2026-09-17
//

#pragma once
#include "../../attr/SurfaceStyleRecipe.hpp"
#include "../NativeDialogs.hpp"
#include "WorkspaceText.hpp"
#include "WorkspaceNavigation.hpp"
#include "WorkspaceTheme.hpp"
#include "WorkspaceComboDrawing.hpp"
#include "SurfaceEditHistory.hpp"
#include "AppearanceResetHistory.hpp"
#include "AppearanceEditTracker.hpp"
#include "SurfaceResetMenu.hpp"
#include "PanelBackBuffer.hpp"
#include "PanelDrawing.hpp"
#include "InspectorLayout.hpp"
#include "InspectorScroll.hpp"
#include "NumericField.hpp"
#include "SearchField.hpp"
#include "SettingsSearchPanel.hpp"
#include "SurfacePresentation.hpp"
#include "SurfaceInspectorItems.hpp"
#include "SurfaceInspectorContent.hpp"
#include "SurfaceEffectState.hpp"
#include "EffectNavigation.hpp"
#include "EffectsLayout.hpp"
#include "NavigationItems.hpp"
#include "HeaderLayout.hpp"
#include "WorkspaceGeometry.hpp"
#include "PaneSplitter.hpp"
#include "PanelDockHandle.hpp"
#include "WorkspacePreferences.hpp"
#include "FormWorkspace.hpp"
#include "ShaderLayerWorkspace.hpp"
#include "DockedAppearancePanel.hpp"
#include <memory>
#include <commdlg.h>
#include <array>
#include <functional>
#include <commctrl.h>
#include <windowsx.h>

namespace merutilm::rff2::workspace {
    class WorkspaceShell {
        struct Panel {
            WorkspaceShell *owner = nullptr;
            int kind = 0;
            HWND window = nullptr;
            PanelBackBuffer buffer;
        };
        std::array<Panel, 3> panels;
        HWND search = nullptr;
        std::unique_ptr<SearchField> searchControl;
        SettingsSearchIndex searchIndex;
        std::unique_ptr<SettingsSearchPanel> searchResults;
        HWND removeEffect = nullptr;
        HWND baseStyle = nullptr;
        HWND compactCategory = nullptr;
        HWND workspaceModule = nullptr;
        HWND workspaceChooser = nullptr;
        std::array<HWND, 9> headerButtons{};
        HeaderLayout headerLayout;
        std::vector<int> moduleIds{1};
        WorkspaceNavigation workspaceNavigation;
        int moduleTab = -1;
        bool compact = false;
        bool navigationOpen = false;
        bool navigationDockedOpen = true;
        PaneWidths paneWidths, resizeStartWidths;
        WorkspaceGeometry geometry;
        std::array<std::unique_ptr<PaneSplitter>, 2> splitters;
        std::array<std::unique_ptr<PanelDockHandle>, 2> dockHandles;
        int resizeStartWidth = 0;
        int navigationScroll = 0, navigationGrab = 0, navigationWheel = 0;
        bool navigationDragging = false;
        bool navigationPointerFocus = false;
        long navigationFocus = NavigationItems::categoryId(Category::EMISSION);
        std::unique_ptr<AccessibleItems> navigationAccessibility;
        bool inspectorOpen = true;
        std::function<void()> layoutObserver;
        SIZE layoutSize{1, 1};
        std::unique_ptr<NumericField> numeric;
        std::map<int, std::unique_ptr<FormWorkspace>> workspaces;
        HWND layerHost = nullptr;
        std::unique_ptr<ShaderLayerWorkspace> layerPanel;
        std::unique_ptr<PaneSplitter> layerDivider;
        int layerHeight = 300, layerResizeStart = 300, navigationHeight = 1;
        AppearancePanelLayout panelLayout;
        std::array<std::unique_ptr<DockedAppearancePanel>, 4> appearancePanels;
        int selectedPanel = 0;
        std::wstring panelLayoutMessage;
        static constexpr UINT panelCommand = WM_APP + 164;
        int activeWorkspace = 1;
        RECT canvasBounds{};
        std::function<void(int, int, const RECT &)> workspaceObserver;
        bool notifyingWorkspace = false;
        std::function<bool()> operationBusy;
        int focusedRow = 0;
        int inspectorTabFocus = -1;
        std::unique_ptr<AccessibleItems> surfaceAccessibility;
        bool resetFocused = false;
        HBRUSH editBrush = nullptr;
        HFONT font = nullptr;
        HFONT heading = nullptr;
        HFONT brandFont = nullptr;
        std::function<ShdSlopeAttribute &()> attribute;
        std::function<void()> requestRender;
        std::function<void(int)> openWorkspace;
        std::function<HWND()> previewFocusTarget;
        SurfaceEditHistory history;
        std::unique_ptr<AppearanceResetHistory> appearanceHistory;
        std::shared_ptr<AppearanceEditTracker> appearanceTracker;
        static constexpr int appearanceHistoryTarget = 1000;
        std::shared_ptr<HistoryDomain> sharedHistory = std::make_shared<HistoryDomain>();
        Category selected = Category::EMISSION;
        EffectNavigation navigation;
        EffectsLayout::Target hoveredEffect;
        std::filesystem::path preferencesPath;
        bool preferencesSaveFailed = false;
        std::wstring query;
        bool configDirty = false;
        SurfaceInspectorContent surfaceContent;
        const SurfaceParameter *dragging = nullptr;
        float scale = 1;
        int scroll = 0;
        bool scrollDragging = false;
        int scrollGrabOffset = 0;
        bool details = false;
        int inspectorHeight = 1;
        int inspectorWidth = InspectorLayout::width;
        int shortScroll = 0;
        WorkspaceTheme theme = WorkspaceTheme::current();
        WorkspaceComboDrawing::Context comboContext;

        static std::wstring surfaceValueText(const SurfaceParameter &parameter, float value) {
            wchar_t number[32];
            swprintf(number, 32,
                     parameter.logarithmicFloor > 0                        ? L"%.6g"
                     : parameter.isInteger() && std::trunc(value) == value ? L"%.0f"
                                                                         : L"%.2f",
                     value);
            return number;
        }

        FormWorkspace *page() const {
            const auto found = workspaces.find(activeWorkspace);
            return found == workspaces.end() ? nullptr : found->second.get();
        }
        void notifyWorkspace() {
            if (!workspaceObserver || notifyingWorkspace || canvasBounds.right <= canvasBounds.left) {
                return;
            }
            notifyingWorkspace = true;
            workspaceObserver(activeWorkspace, page() ? page()->selectedGroup() : -1, canvasBounds);
            notifyingWorkspace = false;
        }
        int historyTarget(bool redo) const {
            if ((operationBusy && operationBusy()) || dragging || hasPending()) {
                return -1;
            }
            uint64_t selectedHistoryOrder = redo ? history.redoOrder() : history.undoOrder();
            int target = selectedHistoryOrder ? 1 : -1;
            const auto appearanceOrder =
                appearanceHistory ? (redo ? appearanceHistory->redoOrder() : appearanceHistory->undoOrder())
                                  : 0;
            if (appearanceOrder &&
                (!selectedHistoryOrder ||
                 (redo ? appearanceOrder < selectedHistoryOrder : appearanceOrder > selectedHistoryOrder))) {
                selectedHistoryOrder = appearanceOrder;
                target = appearanceHistoryTarget;
            }
            for (const auto &[id, form] : workspaces) {
                const auto serial = form->historyOrder(redo);
                if (serial && (!selectedHistoryOrder ||
                               (redo ? serial < selectedHistoryOrder : serial > selectedHistoryOrder))) {
                    selectedHistoryOrder = serial;
                    target = id;
                }
            }
            return target;
        }
        bool canUndo() const {
            return historyTarget(false) >= 0;
        }
        bool canRedo() const {
            return historyTarget(true) >= 0;
        }
        void undoRedo(bool redo) {
            const auto target = historyTarget(redo);
            if (target < 0) {
                return;
            }
            if (target == 1) {
                if (redo ? history.redo(attribute()) : history.undo(attribute())) {
                    requestRender();
                }
            } else if (target == appearanceHistoryTarget) {
                if (redo ? appearanceHistory->redo() : appearanceHistory->undo()) {
                    requestRender();
                }
            } else {
                auto &form = workspaces.at(target);
                if (redo) {
                    form->redo();
                } else {
                    form->undo();
                }
            }
            refresh();
            notifyWorkspace();
        }
        bool selectBaseStyle(ShdSurfaceStyle style) {
            if (!appearanceHistory || !appearanceTracker) {
                return history.setStyle(attribute(), style);
            }
            auto before = appearanceTracker->current(), after = before;
            if (!applySurfaceStyleRecipe(after, style) || !appearanceHistory->apply(std::move(after))) {
                return false;
            }
            appearanceTracker->record(std::move(before), L"Base Style");
            return true;
        }
        void save() {
            if (!applyAllPending(true)) {
                return;
            }
            openWorkspace(5);
        }
        int px(int value) const {
            return int(value * scale + .5f);
        }
        int navigationWidth() const {
            const int width = geometry.navigation.right - geometry.navigation.left;
            return width > 0 ? int(width / scale + .5f) : paneWidths.navigation;
        }
        int surfaceRight() const {
            return std::max(120, int(inspectorWidth / scale + .5f) - InspectorLayout::rightInset);
        }
        int surfaceTrack() const {
            return surfaceRight() - InspectorLayout::inset;
        }
        int surfaceFieldX() const {
            return surfaceRight() - InspectorLayout::fieldWidth;
        }
        int surfaceLabelWidth() const {
            return surfaceFieldX() - InspectorLayout::inset - 8;
        }
        int surfaceTabWidth() const {
            return surfaceTrack() / 2;
        }
        RECT surfaceTabBounds(int index) const {
            const int left = 20 + index * surfaceTrack() / 2, right = 20 + (index + 1) * surfaceTrack() / 2;
            return box(left, 54, right - left, 32);
        }
        int surfaceFooterHeight() const {
            return query.empty() ? 8 : 48;
        }
        bool flowingSurface() const {
            return inspectorHeight < px(228 + surfaceFooterHeight());
        }
        int surfaceHeight() const {
            return flowingSurface() ? px(152 + surfaceFooterHeight() + surfaceContent.height())
                                    : inspectorHeight;
        }
        int surfaceOffset() const {
            return flowingSurface() ? shortScroll : 0;
        }
        int firstSurfaceRow() const {
            return flowingSurface() ? 0 : surfaceContent.indexAt(scroll);
        }
        int rowTop(int index) const {
            return 152 + surfaceContent.offset(index) - (flowingSurface() ? 0 : scroll);
        }
        int rowCount() const {
            return surfaceContent.count();
        }
        const SurfaceParameter *rowParameter(int index) const {
            const auto *row = surfaceContent.at(index);
            return row ? row->parameter : nullptr;
        }
        const SurfaceColor *rowColor(int index) const {
            const auto *row = surfaceContent.at(index);
            return row ? row->color : nullptr;
        }
        std::wstring detailLabel() const {
            const int count = SurfaceInspectorContent::advancedChanged(selected, attribute());
            return count ? L"Detail · " + std::to_wstring(count) + L" changed" : L"Detail";
        }
        RECT numericBounds() const {
            auto bounds = box(surfaceFieldX(), rowTop(focusedRow), InspectorLayout::fieldWidth,
                              InspectorLayout::fieldHeight);
            OffsetRect(&bounds, 0, -surfaceOffset());
            return bounds;
        }
        void positionSurfaceControls() {
            shortScroll = std::clamp(shortScroll, 0, std::max(0, surfaceHeight() - inspectorHeight));
            if (removeEffect) {
                ShowWindow(removeEffect,
                           !page() && !searching() && query.empty() && selected != Category::RELIEF
                               ? SW_SHOWNA
                               : SW_HIDE);
                EnableWindow(removeEffect,
                             effectState(attribute(), selected).added && !(operationBusy && operationBusy()));
                SetWindowPos(removeEffect, nullptr, px(surfaceRight() - 148), px(100) - surfaceOffset(),
                             px(148), px(28), SWP_NOZORDER | SWP_NOACTIVATE);
            }
            if (compactCategory && IsWindowVisible(compactCategory)) {
                RECT current;
                GetWindowRect(compactCategory, &current);
                MapWindowPoints(nullptr, panels[2].window, reinterpret_cast<POINT *>(&current), 2);
                if (current.left != px(20) || current.top != px(12) - surfaceOffset() ||
                    current.right - current.left != px(surfaceTrack())) {
                    SetWindowPos(compactCategory, nullptr, px(20), px(12) - surfaceOffset(),
                                 px(surfaceTrack()), px(340), SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
                }
            }
            if (numeric && numeric->isEditing()) {
                const auto bounds = numericBounds();
                SetWindowPos(numeric->handle(), nullptr, bounds.left, bounds.top, bounds.right - bounds.left,
                             bounds.bottom - bounds.top, SWP_NOZORDER | SWP_NOACTIVATE);
            }
            if (surfaceAccessibility) {
                surfaceAccessibility->changed();
            }
        }
        void setSurfaceScroll(int position) {
            auto &offset = flowingSurface() ? shortScroll : scroll;
            const int next = std::clamp(position, 0,
                                        flowingSurface() ? std::max(0, surfaceHeight() - inspectorHeight)
                                                         : maximumScroll());
            if (offset == next) {
                return;
            }
            offset = next;
            positionSurfaceControls();
            InvalidateRect(panels[2].window, nullptr, FALSE);
        }
        void revealSurfaceFocus() {
            if (inspectorTabFocus >= 0) {
                shortScroll = 0;
                positionSurfaceControls();
                return;
            }
            if (flowingSurface()) {
                const int top =
                    resetFocused ? surfaceHeight() - px(surfaceFooterHeight()) : px(rowTop(focusedRow));
                const auto *row = surfaceContent.at(focusedRow);
                const int bottom = resetFocused ? surfaceHeight() : top + px(row ? row->height() - 8 : 36);
                if (top < shortScroll) {
                    shortScroll = top;
                }
                if (bottom > shortScroll + inspectorHeight) {
                    shortScroll = std::min(top, bottom - inspectorHeight);
                }
            } else if (!resetFocused) {
                const int top = surfaceContent.offset(focusedRow);
                const auto *row = surfaceContent.at(focusedRow);
                const int bottom = top + (row ? row->height() : 0);
                if (top < scroll) {
                    scroll = top;
                }
                if (bottom > scroll + rowViewportHeight()) {
                    scroll = std::min(top, bottom - rowViewportHeight());
                }
                scroll = std::clamp(scroll, 0, maximumScroll());
            }
            positionSurfaceControls();
        }
        int paletteActionTop() const {
            return EffectsLayout::palette(int(navigation.rows(attribute()).size())).y;
        }
        int navigationContentHeight() const {
            const int saveFailureHeight = preferencesSaveFailed ? 42 : 0;
            if (const auto *form = page()) {
                return px(84 + int(form->groups().size()) * 40 + saveFailureHeight);
            }
            return px(paletteActionTop() + 48 + saveFailureHeight);
        }
        InspectorScroll navigationScrollBar() const {
            RECT bounds;
            GetClientRect(panels[1].window, &bounds);
            return InspectorScroll::content(bounds.right, bounds.bottom, scale, navigationContentHeight(),
                                            navigationScroll, 2);
        }
        void setNavigationScroll(int position) {
            navigationScroll =
                std::clamp(position, 0, std::max(0, navigationContentHeight() - navigationHeight));
            hoveredEffect = {};
            if (baseStyle) {
                SetWindowPos(baseStyle, nullptr, px(18), px(104) - navigationScroll,
                             px(navigationWidth() - 34), px(280), SWP_NOZORDER | SWP_NOACTIVATE);
            }
            InvalidateRect(panels[1].window, nullptr, FALSE);
            if (navigationAccessibility) {
                navigationAccessibility->changed();
            }
        }
        void revealNavigationRange(int top, int bottom) {
            if (top < navigationScroll) {
                setNavigationScroll(top);
            } else if (bottom > navigationScroll + navigationHeight) {
                setNavigationScroll(std::min(top, bottom - navigationHeight));
            }
        }
        void revealNavigationSelection() {
            if (page()) {
                const int top = px(64 + page()->selectedGroup() * 40);
                revealNavigationRange(top, top + px(36));
                return;
            }
            const auto categories = navigation.rows(attribute());
            const auto found = std::find(categories.begin(), categories.end(), selected);
            if (found != categories.end()) {
                const auto row = EffectsLayout::row(int(found - categories.begin())).pixels(scale);
                revealNavigationRange(row.top, row.bottom);
            }
        }
        std::vector<AccessibleItem> navigationItems() const {
            auto result = page() ? NavigationItems::sections(page()->groups(), page()->selectedGroup(),
                                                             navigationWidth(), scale)
                                 : NavigationItems::effects(attribute(), navigation, selected,
                                                            navigationWidth(), scale);
            RECT viewport;
            GetClientRect(panels[1].window, &viewport);
            const auto focus = GetFocus();
            for (auto &item : result) {
                OffsetRect(&item.bounds, 0, -navigationScroll);
                if (item.id == NavigationItems::style && !page()) {
                    item.native = baseStyle;
                    GetWindowRect(baseStyle, &item.bounds);
                    MapWindowPoints(nullptr, panels[1].window, reinterpret_cast<POINT *>(&item.bounds), 2);
                }
                if ((item.native && focus == item.native) ||
                    (!item.native && focus == panels[1].window && navigationFocus == item.id)) {
                    item.state |= STATE_SYSTEM_FOCUSED;
                }
                if (operationBusy && operationBusy()) {
                    item.state |= STATE_SYSTEM_UNAVAILABLE;
                }
                if (!IsWindowVisible(panels[1].window)) {
                    item.state |= STATE_SYSTEM_INVISIBLE;
                }
                RECT intersection;
                if (!IntersectRect(&intersection, &viewport, &item.bounds)) {
                    item.state |= STATE_SYSTEM_OFFSCREEN;
                }
            }
            return result;
        }
        bool searching() const {
            return searchResults && !query.empty();
        }
        static std::wstring workspaceName(int tab) {
            const TextKey names[] = {TextKey::Explore, TextKey::Appearance, TextKey::Animation,
                                     TextKey::Export};
            return uiText(names[std::clamp(tab, 0, 3)]);
        }
        void focusInspector() {
            if (searching()) {
                searchResults->focus();
            } else if (page()) {
                page()->focus();
            } else {
                SetFocus(panels[2].window);
            }
        }
        void updateSearch(std::wstring value) {
            query = std::move(value);
            if (searching()) {
                searchResults->update(searchIndex.find(query));
                showInspector();
            }
            refresh(false);
        }
        void indexSurface() {
            const auto state = [this](Category category) {
                return std::wstring(effectState(attribute(), category).label());
            };
            for (const auto &parameter : surfaceParameters()) {
                const auto *p = &parameter;
                const auto category = p->category();
                if (category == Category::CONTOUR || category == Category::RELIEF ||
                    paletteLineControl(p->id)) {
                    continue;
                }
                std::wstring aliases = std::wstring(p->label) + L" slope " + settingsSearchAliases(p->id);
                if (category == Category::FILM || category == Category::REFLECTION) {
                    aliases += L" liquid metal chrome";
                }
                if (category == Category::CONTOUR) {
                    aliases += L" cyber sigilism spikes branches";
                }
                if (category == Category::EMISSION) {
                    aliases += L" deep sea bioluminescence glow";
                }
                if (category == Category::PRINT) {
                    aliases += L" ukiyo e woodcut";
                }
                searchIndex.add(
                    {1, int(category), std::string(p->id)}, std::wstring(displayLabel(*p)),
                    L"Appearance / Surface Effects / " + categoryLabel(category), aliases,
                    [this, p] { return AttributeFormModel::number(p->value(attribute())); },
                    [state, category] { return state(category); });
            }
            for (const auto &color : surfaceColors()) {
                const auto *c = &color;
                if (c->category == Category::CONTOUR || paletteLineControl(c->id)) {
                    continue;
                }
                searchIndex.add(
                    {1, int(c->category), std::string(c->id)}, std::wstring(c->label),
                    L"Appearance / Surface Effects / " + categoryLabel(c->category),
                    L"slope color PHONK palette",
                    [this, c] { return AttributeFormModel::colorText(attribute().*c->member); },
                    [state, c] { return state(c->category); });
            }
            searchIndex.add(
                {1, -1, "surface.style"}, L"Base Style", L"Appearance / Surface Effects",
                L"slope Liquid Metal Cyber Sigilism PHONK Black Metal Deep Sea Ukiyo-e",
                [this] {
                    const wchar_t *names[] = {L"Original",    L"Liquid Metal", L"Cyber Sigilism", L"PHONK",
                                              L"Black Metal", L"Deep Sea",     L"Ukiyo-e"};
                    return std::wstring(names[std::clamp(int(attribute().surfaceStyle), 0, 6)]);
                },
                [] { return L"Editable"; });
            searchIndex.add(
                {1, -1, "surface.studio"}, L"Studio", L"Appearance / Surface Effects",
                L"slope lighting material enable", [this] { return attribute().studio.use ? L"On" : L"Off"; },
                [] { return L"Editable"; });
        }
        void indexForm(int module) {
            searchIndex.eraseModule(module);
            auto *form = workspaces.at(module).get();
            for (const auto &field : form->searchFields()) {
                const auto *f = &field;
                const auto path = workspaceName(workspaceNavigation.tab(module)) + L" / " + form->title() +
                                  L" / " + form->groups().at(f->group);
                searchIndex.add(
                    {module, f->group, f->id}, f->label, path, f->hint + L" " + settingsSearchAliases(f->id),
                    [form, f] { return form->searchValue(*f); }, [form, f] { return form->searchState(*f); });
            }
        }
        void openLayerSettings(ShdLayer layer) {
            const int id = int(layer);
            if (id >= 2 && id <= 5) {
                openSearchResult({11, id - 2, "texture." + std::to_string(id - 2) + ".enabled"});
                return;
            }
            if (id >= 6 && id <= 9) {
                openSearchResult({12, id - 6, "pattern." + std::to_string(id - 6) + ".enabled"});
                return;
            }
            if (id >= 19 && id <= 22) {
                openSearchResult({15, id - 19, "effect." + std::to_string(id - 19) + ".enabled"});
                return;
            }
            switch (layer) {
            case ShdLayer::BAND_LINE:
                openSearchResult({16, 1, "palette.bandLineEnabled"});
                return;
            case ShdLayer::STRIPE:
                openSearchResult({13, 1, "stripe.stripeType"});
                return;
            case ShdLayer::SURFACE:
                openSearchResult({10, 0, "slope.depth"});
                return;
            case ShdLayer::METAL:
                openSearchResult({1, int(Category::REFLECTION), "surface.studio.roughness"});
                return;
            case ShdLayer::SIGIL:
                openSearchResult({1, int(Category::MIX), "surface.layerSigil"});
                return;
            case ShdLayer::PHONK:
                openSearchResult({1, int(Category::FLAME), "surface.flameStrength"});
                return;
            case ShdLayer::FROST:
                openSearchResult({1, int(Category::FLAME), "surface.frostStrength"});
                return;
            case ShdLayer::SEA:
                openSearchResult({1, int(Category::EMISSION), "surface.seaGlow"});
                return;
            case ShdLayer::PRINT:
                openSearchResult({1, int(Category::PRINT), "surface.ukiyoColors"});
                return;
            case ShdLayer::PRINT_FINISH:
                openSearchResult({1, int(Category::PRINT), "surface.layerQuantize"});
                return;
            case ShdLayer::SURFACE_COLOR:
                openSearchResult({1, int(Category::COLOR), "surface.paletteColorMix"});
                return;
            case ShdLayer::COLOR:
                openSearchResult({14, 0, "color.gamma"});
                return;
            case ShdLayer::FOG:
                openSearchResult({14, 2, "fog.opacity"});
                return;
            case ShdLayer::BLOOM:
                openSearchResult({14, 1, "bloom.intensity"});
                return;
            case ShdLayer::TONE_MAP:
                openSearchResult({3, 4, "export.toneMap"});
                return;
            case ShdLayer::VHS:
                openSearchResult({1, int(Category::NOISE), "surface.layerVhs"});
                return;
            case ShdLayer::MONO:
                openSearchResult({1, int(Category::NOISE), "surface.layerMono"});
                return;
            default:
                return;
            }
        }
        void openSearchResult(const SettingsSearchTarget &target) {
            if (operationBusy && operationBusy()) {
                return;
            }
            if (target.module == 18 && layerPanel) {
                query.clear();
                SetWindowTextW(search, L"");
                showInspector();
                refresh();
                layerPanel->focus(target.field);
                return;
            }
            selectWorkspace(target.module, target.field, target.section);
            if (activeWorkspace != target.module || !query.empty()) {
                return;
            }
            if (auto *form = page()) {
                form->revealField(target.field);
            } else if (target.section < 0) {
                if (!(geometry.compact ? navigationOpen : navigationDockedOpen)) {
                    toggleNavigation();
                }
                focusNavigation(target.field == "surface.style" ? NavigationItems::style
                                                                : NavigationItems::studio);
            } else {
                focusSurface(SurfaceInspectorContent::control(target.field).id);
            }
            revealNavigationSelection();
            notifyWorkspace();
            RedrawWindow(panels[2].window, nullptr, nullptr,
                         RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
        }
        void normalizeNavigationFocus() {
            const auto items = navigationItems();
            if (std::any_of(items.begin(), items.end(),
                            [&](const auto &item) { return item.id == navigationFocus; })) {
                return;
            }
            const auto selectedItem = std::find_if(items.begin(), items.end(), [](const auto &item) {
                return item.state & STATE_SYSTEM_SELECTED;
            });
            navigationFocus = selectedItem != items.end() ? selectedItem->id
                              : items.empty()             ? 0
                                                          : items.front().id;
        }
        bool focusNavigation(long id) {
            if (!IsWindowVisible(panels[1].window)) {
                return false;
            }
            const auto items = navigationItems();
            const auto found =
                std::find_if(items.begin(), items.end(), [&](const auto &item) { return item.id == id; });
            if (found == items.end()) {
                return false;
            }
            if (!(found->state & STATE_SYSTEM_FOCUSABLE)) {
                return false;
            }
            navigationFocus = id;
            const auto rect = found->bounds;
            revealNavigationRange(rect.top + navigationScroll, rect.bottom + navigationScroll);
            SetFocus(found->native ? found->native : panels[1].window);
            InvalidateRect(panels[1].window, nullptr, FALSE);
            if (navigationAccessibility) {
                navigationAccessibility->changed();
            }
            return true;
        }
        void tabNavigation(int direction) {
            auto items = navigationItems();
            std::erase_if(items, [](const auto &item) { return !(item.state & STATE_SYSTEM_FOCUSABLE); });
            const long current = GetFocus() == baseStyle ? NavigationItems::style : navigationFocus;
            const auto found = std::find_if(items.begin(), items.end(),
                                            [&](const auto &item) { return item.id == current; });
            const int next = found == items.end() ? 0 : int(found - items.begin()) + direction;
            if (next >= 0 && next < int(items.size())) {
                focusNavigation(items[next].id);
            } else if (direction < 0) {
                focusHeader(-1);
            } else if (!inspectorOpen) {
                focusHeader();
            } else {
                focusInspector();
            }
        }
        bool activateNavigation(long id) {
            if (operationBusy && operationBusy()) {
                return false;
            }
            if (!numeric->commit()) {
                SetFocus(numeric->handle());
                return false;
            }
            if (!focusNavigation(id)) {
                return false;
            }
            if (page()) {
                if (id < NavigationItems::section) {
                    return false;
                }
                showInspector();
                query.clear();
                SetWindowTextW(search, L"");
                page()->selectGroup(int(id - NavigationItems::section));
                refresh();
                return true;
            }
            if (id == NavigationItems::studio) {
                if (history.setStudio(attribute(), !attribute().studio.use)) {
                    requestRender();
                }
            } else if (id == NavigationItems::style) {
                SendMessageW(baseStyle, CB_SHOWDROPDOWN, TRUE, 0);
                return true;
            } else if (id >= NavigationItems::filter &&
                       id < NavigationItems::filter + EffectNavigation::filterCount) {
                navigation.filter = static_cast<EffectFilter>(id - NavigationItems::filter);
            } else if (id == NavigationItems::palette) {
                if (workspaces.contains(16)) {
                    selectWorkspace(16);
                } else {
                    openWorkspace(4);
                }
                return true;
            } else if (id >= NavigationItems::category && id < NavigationItems::category + 20) {
                const auto category = static_cast<Category>((id - NavigationItems::category) / 2);
                if ((id - NavigationItems::category) % 2) {
                    const auto old = navigation.rows(attribute());
                    const auto position = std::find(old.begin(), old.end(), category) - old.begin();
                    toggleFavorite(category);
                    const auto next = navigation.rows(attribute());
                    if (navigation.filter == EffectFilter::FAVORITES && !navigation.favorite(category)) {
                        navigationFocus = next.empty()
                                              ? NavigationItems::filter + 2
                                              : NavigationItems::favoriteId(
                                                    next[std::min<size_t>(position, next.size() - 1)]);
                    }
                } else {
                    showInspector();
                    openEffect(category);
                    query.clear();
                    SetWindowTextW(search, L"");
                    scroll = 0;
                    shortScroll = 0;
                }
            } else {
                return false;
            }
            refresh();
            focusNavigation(navigationFocus);
            return true;
        }
        bool navigationKey(WPARAM key) {
            const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            if (key == VK_TAB) {
                tabNavigation(shift ? -1 : 1);
                return true;
            }
            if (key == VK_SPACE || key == VK_RETURN) {
                activateNavigation(navigationFocus);
                return true;
            }
            if (key == 'F' && !page() && navigationFocus >= NavigationItems::category &&
                navigationFocus < NavigationItems::category + 20) {
                activateNavigation(NavigationItems::favoriteId(
                    static_cast<Category>((navigationFocus - NavigationItems::category) / 2)));
                return true;
            }
            const bool backward = key == VK_UP || key == VK_LEFT || key == VK_PRIOR || key == VK_HOME;
            if (!(backward || key == VK_DOWN || key == VK_RIGHT || key == VK_NEXT || key == VK_END)) {
                return false;
            }
            const auto items = navigationItems();
            std::vector<long> peers;
            const bool filters = navigationFocus >= NavigationItems::filter &&
                                 navigationFocus < NavigationItems::filter + EffectNavigation::filterCount;
            for (const auto &item : items) {
                if (filters ? item.role == ROLE_SYSTEM_RADIOBUTTON
                    : page()
                        ? item.id >= NavigationItems::section
                        : item.id >= NavigationItems::category && item.id < NavigationItems::category + 20 &&
                              ((item.id - navigationFocus) % 2 == 0)) {
                    peers.push_back(item.id);
                }
            }
            if (peers.empty()) {
                tabNavigation(backward ? -1 : 1);
                return true;
            }
            const auto found = std::find(peers.begin(), peers.end(), navigationFocus);
            int index = found == peers.end() ? 0 : int(found - peers.begin());
            const int step =
                key == VK_PRIOR || key == VK_NEXT ? std::max(1, int(navigationHeight / scale) / 40 - 1) : 1;
            index = key == VK_HOME  ? 0
                    : key == VK_END ? int(peers.size()) - 1
                                    : std::clamp(index + (backward ? -step : step), 0, int(peers.size()) - 1);
            const long next = peers[index];
            if (filters || page() || next % 2 == 0) {
                activateNavigation(next);
            } else {
                focusNavigation(next);
            }
            return true;
        }
        static LRESULT CALLBACK navigationControlProcedure(HWND hwnd, UINT message, WPARAM w, LPARAM l,
                                                           UINT_PTR id, DWORD_PTR data) {
            auto &self = *reinterpret_cast<WorkspaceShell *>(data);
            if (message == WM_SETFOCUS) {
                self.navigationFocus = NavigationItems::style;
                self.revealNavigationRange(self.px(104), self.px(132));
                if (self.navigationAccessibility) {
                    self.navigationAccessibility->changed();
                }
            }
            if (message == WM_KEYDOWN && w == VK_TAB) {
                self.tabNavigation((GetKeyState(VK_SHIFT) & 0x8000) ? -1 : 1);
                return 0;
            }
            if (message == WM_CHAR && w == VK_TAB) {
                return 0;
            }
            if (message == WM_GETDLGCODE) {
                return DefSubclassProc(hwnd, message, w, l) | DLGC_WANTTAB;
            }
            if (message == WM_NCDESTROY) {
                RemoveWindowSubclass(hwnd, navigationControlProcedure, id);
            }
            return DefSubclassProc(hwnd, message, w, l);
        }
        static LRESULT CALLBACK inspectorControlProcedure(HWND hwnd, UINT message, WPARAM w, LPARAM l,
                                                          UINT_PTR id, DWORD_PTR data) {
            auto &self = *reinterpret_cast<WorkspaceShell *>(data);
            if (message == WM_KEYDOWN && w == VK_TAB) {
                if (GetKeyState(VK_SHIFT) & 0x8000) {
                    SetFocus(self.search);
                } else {
                    self.focusSurface(self.details ? SurfaceInspectorItems::detail
                                                   : SurfaceInspectorItems::basic);
                }
                return 0;
            }
            if (message == WM_CHAR && w == VK_TAB) {
                return 0;
            }
            if (message == WM_GETDLGCODE) {
                return DefSubclassProc(hwnd, message, w, l) | DLGC_WANTTAB;
            }
            if (message == WM_SETFOCUS && self.surfaceAccessibility) {
                self.surfaceAccessibility->changed();
            }
            if (message == WM_NCDESTROY) {
                RemoveWindowSubclass(hwnd, inspectorControlProcedure, id);
            }
            return DefSubclassProc(hwnd, message, w, l);
        }
        void toggleNavigation() {
            if (geometry.compact) {
                navigationOpen = !navigationOpen;
            } else {
                navigationDockedOpen = !navigationDockedOpen;
            }
            if (GetFocus() == panels[1].window || IsChild(panels[1].window, GetFocus())) {
                SetFocus(headerButtons[7]);
            }
            requestLayout();
        }
        void requestLayout() {
            if (layoutObserver) {
                layoutObserver();
            } else {
                layout(layoutSize.cx, layoutSize.cy);
            }
        }
        void showInspector() {
            if (!inspectorOpen) {
                inspectorOpen = true;
                requestLayout();
            }
        }
        void toggleInspector() {
            if (!numeric->commit()) {
                SetFocus(numeric->handle());
                return;
            }
            if (dragging) {
                history.commit(attribute());
                dragging = nullptr;
                ReleaseCapture();
            }
            if (scrollDragging) {
                scrollDragging = false;
                ReleaseCapture();
            }
            inspectorOpen = !inspectorOpen;
            if (!inspectorOpen && (GetFocus() == panels[2].window || IsChild(panels[2].window, GetFocus()))) {
                SetFocus(headerButtons[8]);
            }
            requestLayout();
        }
        std::vector<HWND> headerFocusOrder() const {
            std::vector<HWND> result;
            if (IsWindowVisible(workspaceChooser)) {
                result.push_back(workspaceChooser);
            } else {
                for (int i = 3; i < 7; ++i) {
                    if (IsWindowVisible(headerButtons[i])) {
                        result.push_back(headerButtons[i]);
                    }
                }
            }
            if (IsWindowVisible(workspaceModule)) {
                result.push_back(workspaceModule);
            }
            result.push_back(search);
            if (IsWindowVisible(headerButtons[7])) {
                result.push_back(headerButtons[7]);
            }
            if (IsWindowVisible(headerButtons[8])) {
                result.push_back(headerButtons[8]);
            }
            for (const auto &handle : dockHandles) {
                if (handle && IsWindowVisible(handle->handle())) {
                    result.push_back(handle->handle());
                }
            }
            for (const auto &splitter : splitters) {
                if (splitter && IsWindowVisible(splitter->handle())) {
                    result.push_back(splitter->handle());
                }
            }
            return result;
        }
        void focusHeader(int direction = 1) {
            const auto controls = headerFocusOrder();
            if (!controls.empty()) {
                SetFocus(direction < 0 ? controls.back() : controls.front());
            }
        }
        void advanceHeader(HWND current, int direction) {
            const auto controls = headerFocusOrder();
            const auto found = std::find(controls.begin(), controls.end(), current);
            const int next = found == controls.end() ? 0 : int(found - controls.begin()) + direction;
            if (next >= 0 && next < int(controls.size())) {
                SetFocus(controls[next]);
            } else if (direction < 0) {
                SetFocus(GetParent(panels[0].window));
            } else if (IsWindowVisible(panels[1].window)) {
                SetFocus(panels[1].window);
            } else if (!inspectorOpen) {
                SetFocus(GetParent(panels[0].window));
            } else {
                focusInspector();
            }
        }
        void activateHeader(int index) {
            if ((operationBusy && operationBusy()) || !numeric->commit()) {
                if (numeric->isInvalid()) {
                    SetFocus(numeric->handle());
                }
                return;
            }
            if (index < 2) {
                undoRedo(index == 1);
            } else if (index == 2) {
                save();
            } else if (index < 7) {
                selectWorkspace(workspaceNavigation.last(index - 3));
            } else if (index == 7) {
                toggleNavigation();
            } else {
                toggleInspector();
            }
        }
        void updateHeader() {
            if (!headerButtons[0]) {
                return;
            }
            EnableWindow(headerButtons[0], canUndo());
            EnableWindow(headerButtons[1], canRedo());
            const auto tab = workspaceNavigation.tab(activeWorkspace);
            SendMessageW(workspaceChooser, CB_SETCURSEL, tab, 0);
            const bool navigationShown = geometry.compact ? navigationOpen : navigationDockedOpen;
            SetWindowTextW(headerButtons[7], navigationShown ? (page() ? L"Hide Sections" : L"Hide Effects")
                                                             : (page() ? L"Show Sections" : L"Show Effects"));
            SetWindowTextW(headerButtons[8], inspectorOpen ? L"Hide Settings" : L"Show Settings");
            UiLanguage::caption(headerButtons[7]);
            UiLanguage::caption(headerButtons[8]);
            for (auto control : headerButtons) {
                InvalidateRect(control, nullptr, FALSE);
            }
            InvalidateRect(panels[0].window, nullptr, FALSE);
        }
        static LRESULT CALLBACK headerControlProcedure(HWND hwnd, UINT message, WPARAM w, LPARAM l,
                                                       UINT_PTR id, DWORD_PTR data) {
            if (message == WM_ERASEBKGND) {
                return 1;
            }
            auto &self = *reinterpret_cast<WorkspaceShell *>(data);
            if (message == WM_KEYDOWN && w == VK_TAB) {
                self.advanceHeader(hwnd, (GetKeyState(VK_SHIFT) & 0x8000) ? -1 : 1);
                return 0;
            }
            if (message == WM_KEYDOWN && w == VK_RETURN) {
                const auto found = std::find(self.headerButtons.begin(), self.headerButtons.end(), hwnd);
                if (found != self.headerButtons.end()) {
                    self.activateHeader(int(found - self.headerButtons.begin()));
                    return 0;
                }
            }
            if (message == WM_GETDLGCODE) {
                return DefSubclassProc(hwnd, message, w, l) | DLGC_WANTTAB;
            }
            if (message == WM_CHAR && (w == VK_TAB || w == VK_RETURN)) {
                return 0;
            }
            if (message == WM_NCDESTROY) {
                RemoveWindowSubclass(hwnd, headerControlProcedure, id);
            }
            return DefSubclassProc(hwnd, message, w, l);
        }
        void drawHeaderButton(const DRAWITEMSTRUCT &target) {
            auto item = target;
            const int index = int(item.CtlID) - 3000;
            if (index < 0 || index >= int(headerButtons.size())) {
                return;
            }
            const bool tab = index >= 3 && index < 7,
                       active = tab && index - 3 == workspaceNavigation.tab(activeWorkspace);
            if (!tab) {
                WorkspaceButton::draw(item, comboContext);
                return;
            }
            PanelBackBuffer buffer;
            const auto dc = buffer.begin(target.hDC, target.rcItem.right, target.rcItem.bottom);
            if (dc) {
                item.hDC = dc;
            }
            fill(item.hDC, item.rcItem, theme.background);
            const bool pressed = (item.itemState & ODS_SELECTED) != 0,
                       focused = (item.itemState & ODS_FOCUS) != 0;
            if (!tab || focused || pressed) {
                PanelDrawing::rounded(item.hDC, item.rcItem,
                                      pressed ? theme.selected
                                      : tab   ? theme.background
                                              : theme.field,
                                      focused ? theme.accent
                                      : tab   ? theme.background
                                              : theme.track,
                                      px(2));
            }
            wchar_t caption[80];
            GetWindowTextW(item.hwndItem, caption, 80);
            PanelDrawing::text(item.hDC, caption, item.rcItem,
                               item.itemState & ODS_DISABLED ? theme.secondary
                               : pressed                     ? theme.selectedForeground
                               : active                      ? theme.accent
                                                             : theme.foreground,
                               font, DT_CENTER);
            if (active) {
                fill(item.hDC,
                     {item.rcItem.left + px(8), item.rcItem.bottom - px(3), item.rcItem.right - px(8),
                      item.rcItem.bottom - px(1)},
                     theme.accent);
            }
            if (dc) {
                buffer.present(target.hDC);
            }
        }
        void toggleFavorite(Category category) {
            navigation.toggleFavorite(category);
            savePreferences();
        }
        void openEffect(Category category) {
            if (category == Category::CONTOUR) {
                selectWorkspace(16, {}, 1);
                return;
            }
            if (category == Category::RELIEF) {
                selectWorkspace(10, {}, 0);
                return;
            }
            selected = category;
            if (navigation.remember(category)) {
                savePreferences();
            }
        }
        void savePreferences() {
            panelLayout.layerHeight = layerHeight;
            preferencesSaveFailed =
                !WorkspacePreferences::save(preferencesPath, navigation, &paneWidths, &panelLayout);
            setNavigationScroll(navigationScroll);
            InvalidateRect(panels[1].window, nullptr, FALSE);
        }
        static void fill(HDC dc, RECT rect, COLORREF color) {
            PanelDrawing::fill(dc, rect, color);
        }
        void text(HDC dc, std::wstring_view value, RECT rect, COLORREF color, bool large = false) const {
            PanelDrawing::text(dc, value, rect, color, large ? heading : font);
        }
        RECT box(int x, int y, int w, int h) const {
            return {px(x), px(y), px(x + w), px(y + h)};
        }
        void configureInspectorComposition() {
            const auto window = panels[2].window;
            if (!window) {
                return;
            }
            const LONG_PTR style = GetWindowLongPtrW(window, GWL_EXSTYLE);
            const bool composite = page() || searching();
            if (bool(style & WS_EX_COMPOSITED) == composite) {
                return;
            }
            SetWindowLongPtrW(window, GWL_EXSTYLE,
                              composite ? style | WS_EX_COMPOSITED : style & ~LONG_PTR(WS_EX_COMPOSITED));
            panels[2].buffer.reset();
            SetWindowPos(window, nullptr, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED |
                             SWP_NOCOPYBITS);
            RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
        }
        void refreshWorkspaceModules() {
            if (workspaceModule) {
                const int tab = workspaceNavigation.tab(activeWorkspace);
                if (moduleTab != tab) {
                    moduleTab = tab;
                    moduleIds.clear();
                    SendMessageW(workspaceModule, CB_RESETCONTENT, 0, 0);
                    for (const auto &module : workspaceNavigation.modules(tab)) {
                        moduleIds.push_back(module.id);
                        SendMessageW(workspaceModule, CB_ADDSTRING, 0,
                                     reinterpret_cast<LPARAM>(UiLanguage::text(module.title).c_str()));
                    }
                }
                ShowWindow(workspaceModule, moduleIds.size() > 1 ? SW_SHOWNA : SW_HIDE);
                const auto found = std::find(moduleIds.begin(), moduleIds.end(), activeWorkspace);
                SendMessageW(workspaceModule, CB_SETCURSEL,
                             found == moduleIds.end() ? 0 : found - moduleIds.begin(), 0);
            }
        }
        void rebuildSurfaceRows() {
            const auto *oldRow = surfaceContent.at(focusedRow);
            const long focused = oldRow ? oldRow->id : 0;
            surfaceContent.rebuild(selected, details);
            if (const int index = surfaceContent.find(focused); index >= 0) {
                focusedRow = index;
            }
            focusedRow = std::clamp(focusedRow, 0, std::max(0, rowCount() - 1));
            scroll = std::clamp(scroll, 0, maximumScroll());
        }
        void refresh(bool searchValues = true) {
            for (auto &panel : appearancePanels) {
                if (panel) {
                    panel->form().show(true);
                }
            }
            if (layerPanel) {
                layerPanel->refresh();
            }
            const bool composite = page() || searching();
            if (composite) {
                configureInspectorComposition();
            }
            hoveredEffect = {};
            if (searchResults) {
                for (auto &[id, form] : workspaces) {
                    const bool show = id == activeWorkspace && !searching();
                    if (bool(IsWindowVisible(form->handle())) != show) {
                        form->show(show);
                    }
                }
                if (searching() && searchValues) {
                    searchResults->update(searchIndex.find(query), false);
                }
                searchResults->show(searching());
            }
            if (!composite) {
                configureInspectorComposition();
            }
            refreshWorkspaceModules();
            if (baseStyle) {
                ShowWindow(baseStyle, page() ? SW_HIDE : SW_SHOWNA);
            }
            if (compactCategory) {
                ShowWindow(compactCategory, !page() && compact && query.empty() ? SW_SHOWNA : SW_HIDE);
            }
            if (compactCategory) {
                SendMessageW(compactCategory, CB_SETCURSEL, static_cast<int>(selected), 0);
            }
            if (baseStyle) {
                SendMessageW(baseStyle, CB_SETCURSEL, static_cast<int>(attribute().surfaceStyle), 0);
            }
            rebuildSurfaceRows();
            positionSurfaceControls();
            normalizeNavigationFocus();
            setNavigationScroll(navigationScroll);
            updateHeader();
            for (auto &panel : panels) {
                if (panel.window) {
                    InvalidateRect(panel.window, nullptr, FALSE);
                }
            }
        }
        int rowViewportHeight() const {
            return std::max(0, int(inspectorHeight / scale) - 152 - surfaceFooterHeight());
        }
        int maximumScroll() const {
            return std::max(0, surfaceContent.height() - rowViewportHeight());
        }
        InspectorScroll scrollBar() const {
            RECT r;
            GetClientRect(panels[2].window, &r);
            if (flowingSurface()) {
                return InspectorScroll::content(r.right, r.bottom, scale, surfaceHeight(), shortScroll);
            }
            return InspectorScroll::layout(r.right, r.bottom, scale, surfaceFooterHeight(),
                                           rowViewportHeight(), surfaceContent.height(), scroll);
        }
        Category focusedCategory() const {
            if (const auto *parameter = rowParameter(focusedRow)) {
                return parameter->category();
            }
            if (const auto *color = rowColor(focusedRow)) {
                return color->category;
            }
            return selected;
        }
        long surfaceFocusId() const {
            if (GetFocus() == removeEffect) {
                return SurfaceInspectorItems::removeEffect;
            }
            if (GetFocus() == compactCategory) {
                return SurfaceInspectorItems::category;
            }
            if (numeric->isEditing() && GetFocus() == numeric->handle()) {
                return SurfaceInspectorItems::editor;
            }
            if (inspectorTabFocus >= 0) {
                return inspectorTabFocus ? SurfaceInspectorItems::detail : SurfaceInspectorItems::basic;
            }
            if (resetFocused) {
                return SurfaceInspectorItems::footer;
            }
            const auto *row = surfaceContent.at(focusedRow);
            return row ? row->id : 0;
        }
        bool surfaceFooterEnabled() const {
            return !query.empty() && rowCount() > 0;
        }
        std::vector<AccessibleItem> surfaceItems() const {
            std::vector<AccessibleItem> items;
            const auto window = panels[2].window;
            const auto native = [&](long id, HWND control, std::wstring name) {
                AccessibleItem item;
                item.id = id;
                item.name = std::move(name);
                item.native = control;
                item.role = ROLE_SYSTEM_CLIENT;
                GetWindowRect(control, &item.bounds);
                MapWindowPoints(nullptr, window, reinterpret_cast<POINT *>(&item.bounds), 2);
                if (GetFocus() == control || IsChild(control, GetFocus())) {
                    item.state |= STATE_SYSTEM_FOCUSED;
                }
                items.push_back(std::move(item));
            };
            if (searching()) {
                native(200000, searchResults->handle(), L"Search settings");
                return items;
            }
            if (page()) {
                native(SurfaceInspectorItems::forms + activeWorkspace, page()->handle(), page()->title());
                return items;
            }
            if (IsWindowVisible(removeEffect)) {
                native(SurfaceInspectorItems::removeEffect, removeEffect, L"Remove added effects");
            }
            if (IsWindowVisible(compactCategory)) {
                native(SurfaceInspectorItems::category, compactCategory, L"Effect category");
            }
            if (query.empty()) {
                for (int i = 0; i < 2; ++i) {
                    items.push_back({i ? SurfaceInspectorItems::detail : SurfaceInspectorItems::basic,
                                     i ? detailLabel() : L"Basic",
                                     i ? L"Show all controls in expandable groups. The count is advanced "
                                         L"controls changed from reset defaults."
                                       : L"Show the most commonly used controls.",
                                     L"Select", surfaceTabBounds(i), ROLE_SYSTEM_PAGETAB,
                                     STATE_SYSTEM_FOCUSABLE | STATE_SYSTEM_SELECTABLE |
                                         (details == bool(i) ? STATE_SYSTEM_SELECTED : 0)});
                }
            }
            const int count = rowCount();
            for (int i = 0; i < count; ++i) {
                const auto &row = *surfaceContent.at(i);
                AccessibleItem item;
                item.id = row.id;
                item.name = row.label();
                item.bounds = box(20, rowTop(i), surfaceTrack(), row.header() ? 32 : 56);
                item.writable = !row.header();
                if (row.header()) {
                    const bool expanded = surfaceContent.expanded(row.group);
                    item.role = ROLE_SYSTEM_PUSHBUTTON;
                    item.state |= expanded ? STATE_SYSTEM_EXPANDED : STATE_SYSTEM_COLLAPSED;
                    item.action = expanded ? L"Collapse" : L"Expand";
                    item.help = L"Enter or Space toggles this group. Left collapses; Right expands.";
                    item.value =
                        std::to_wstring(SurfaceInspectorContent::changedInGroup(row.group, attribute())) +
                        L" controls changed from reset defaults.";
                } else if (row.parameter) {
                    const auto &parameter = *row.parameter;
                    item.help = SurfaceInspectorItems::range(parameter) +
                                L" Left and Right adjust the value. Enter opens numeric input. Right-click "
                                L"opens reset options; Ctrl+Backspace resets this value.";
                    item.role = ROLE_SYSTEM_SLIDER;
                    item.value = AttributeFormModel::number(parameter.value(attribute()));
                    item.action = L"Edit value";
                } else {
                    const auto &color = *row.color;
                    item.help = L"Enter opens the color picker. Color values accept #RRGGBB or R, G, B, A in "
                                L"0-1. Right-click opens reset options; Ctrl+Backspace resets this color.";
                    item.value = AttributeFormModel::colorText(attribute().*color.member);
                    item.action = L"Choose color";
                }
                if (!flowingSurface()) {
                    const RECT viewport{0, px(152), inspectorWidth,
                                        inspectorHeight - px(surfaceFooterHeight())};
                    RECT visible{};
                    if (!IntersectRect(&visible, &item.bounds, &viewport)) {
                        item.state |= STATE_SYSTEM_OFFSCREEN;
                    }
                    item.bounds = visible;
                }
                items.push_back(std::move(item));
            }
            const bool enabled = surfaceFooterEnabled();
            if (!query.empty()) {
                items.push_back({SurfaceInspectorItems::footer,
                                 L"Show in effect",
                                 L"Open the focused result's effect group.",
                                 L"Activate",
                                 {px(20), surfaceHeight() - px(38), px(156), surfaceHeight() - px(10)},
                                 ROLE_SYSTEM_PUSHBUTTON,
                                 STATE_SYSTEM_FOCUSABLE | (enabled ? 0 : STATE_SYSTEM_UNAVAILABLE)});
            }
            if (numeric->isEditing()) {
                native(SurfaceInspectorItems::editor, numeric->handle(), L"Parameter value");
            }
            RECT client;
            GetClientRect(window, &client);
            const long focused = surfaceFocusId();
            for (auto &item : items) {
                if (!item.native) {
                    OffsetRect(&item.bounds, 0, -surfaceOffset());
                }
                if (!item.native && GetFocus() == window && focused == item.id) {
                    item.state |= STATE_SYSTEM_FOCUSED;
                }
                if (operationBusy && operationBusy()) {
                    item.state |= STATE_SYSTEM_UNAVAILABLE;
                }
                if (!IsWindowVisible(window)) {
                    item.state |= STATE_SYSTEM_INVISIBLE;
                }
                RECT intersection;
                if (!IntersectRect(&intersection, &client, &item.bounds)) {
                    item.state |= STATE_SYSTEM_OFFSCREEN;
                }
            }
            return items;
        }
        bool focusSurface(long id) {
            if (!IsWindowVisible(panels[2].window)) {
                return false;
            }
            if (searching()) {
                if (id != 200000) {
                    return false;
                }
                searchResults->focus();
                return true;
            }
            if (page()) {
                if (id != SurfaceInspectorItems::forms + activeWorkspace) {
                    return false;
                }
                page()->focus();
                return true;
            }
            if (id == SurfaceInspectorItems::removeEffect && IsWindowVisible(removeEffect) &&
                IsWindowEnabled(removeEffect)) {
                if (!numeric->commit()) {
                    return false;
                }
                shortScroll = 0;
                positionSurfaceControls();
                SetFocus(removeEffect);
                return true;
            }
            if (id == SurfaceInspectorItems::editor && numeric->isEditing()) {
                SetFocus(numeric->handle());
                return true;
            }
            if (!numeric->commit()) {
                SetFocus(numeric->handle());
                return false;
            }
            inspectorTabFocus = -1;
            resetFocused = false;
            if (id == SurfaceInspectorItems::category) {
                if (!IsWindowVisible(compactCategory)) {
                    return false;
                }
                shortScroll = 0;
                positionSurfaceControls();
                SetFocus(compactCategory);
                return true;
            }
            if (id == SurfaceInspectorItems::basic || id == SurfaceInspectorItems::detail) {
                if (!query.empty()) {
                    return false;
                }
                inspectorTabFocus = int(id - SurfaceInspectorItems::basic);
            } else if (id == SurfaceInspectorItems::footer) {
                if (query.empty()) {
                    return false;
                }
                resetFocused = true;
            } else {
                int index = surfaceContent.find(id);
                if (index < 0 && details) {
                    surfaceContent.reveal(id);
                    surfaceContent.rebuild(selected, details);
                    index = surfaceContent.find(id);
                }
                if (index < 0) {
                    return false;
                }
                focusedRow = index;
            }
            SetFocus(panels[2].window);
            revealSurfaceFocus();
            InvalidateRect(panels[2].window, nullptr, FALSE);
            if (surfaceAccessibility) {
                surfaceAccessibility->changed();
            }
            return true;
        }
        bool activateSurface(long id) {
            if (operationBusy && operationBusy()) {
                return false;
            }
            if (!focusSurface(id)) {
                return false;
            }
            if (searching() || page()) {
                return true;
            }
            if (id == SurfaceInspectorItems::removeEffect) {
                SendMessageW(removeEffect, BM_CLICK, 0, 0);
                return true;
            }
            if (id == SurfaceInspectorItems::basic || id == SurfaceInspectorItems::detail) {
                details = id == SurfaceInspectorItems::detail;
                scroll = 0;
                shortScroll = 0;
                refresh();
            } else if (id == SurfaceInspectorItems::category) {
                SendMessageW(compactCategory, CB_SHOWDROPDOWN, TRUE, 0);
            } else if (id == SurfaceInspectorItems::footer) {
                activateInspectorFooter();
            } else if (id != SurfaceInspectorItems::editor) {
                editFocused();
            }
            return true;
        }
        bool setSurfaceValue(long id, std::wstring_view text) {
            if (searching() || page() || (operationBusy && operationBusy())) {
                return false;
            }
            if (const auto *parameter = SurfaceInspectorItems::parameter(id)) {
                float value;
                if (!AttributeFormModel::parse(text, value) || !parameter->valid(value)) {
                    return false;
                }
                if (!focusSurface(id)) {
                    return false;
                }
                if (history.set(attribute(), *parameter, value)) {
                    requestRender();
                    refresh();
                }
                return true;
            }
            if (const auto *color = SurfaceInspectorItems::color(id)) {
                auto value = attribute().*color->member;
                if (!AttributeFormModel::parseColor(text, value) || !color->valid(value)) {
                    return false;
                }
                if (!focusSurface(id)) {
                    return false;
                }
                if (history.setColor(attribute(), *color, value)) {
                    requestRender();
                    refresh();
                }
                return true;
            }
            return false;
        }
        void revealResult() {
            if (query.empty() || rowCount() == 0) {
                return;
            }
            if (!numeric->commit()) {
                SetFocus(numeric->handle());
                return;
            }
            const long id = surfaceContent.at(focusedRow)->id;
            openEffect(focusedCategory());
            details = true;
            query.clear();
            SetWindowTextW(search, L"");
            refresh();
            focusSurface(id);
        }
        void activateInspectorFooter() {
            if (!numeric->commit()) {
                SetFocus(numeric->handle());
                return;
            }
            if (!query.empty()) {
                revealResult();
            }
        }
        void executeReset(int command) {
            if (page() || searching() || dragging || (operationBusy && operationBusy()) ||
                !numeric->commit()) {
                return;
            }
            bool changed = false;
            if (command == SurfaceResetMenu::VALUE) {
                const auto *row = surfaceContent.at(focusedRow);
                if (row && row->parameter) {
                    changed = history.resetValue(attribute(), *row->parameter);
                } else if (row && row->color) {
                    changed = history.resetValue(attribute(), *row->color);
                }
            }
            if (command == SurfaceResetMenu::EFFECT) {
                changed = history.resetGroup(attribute(), selected);
            }
            if (command == SurfaceResetMenu::ADDED) {
                changed = history.clearAddedEffects(attribute());
            }
            if (command == SurfaceResetMenu::APPEARANCE && appearanceHistory) {
                auto before = appearanceTracker->current();
                changed = appearanceHistory->reset();
                if (changed) {
                    appearanceTracker->record(std::move(before), L"Reset all appearance");
                }
            }
            if (changed) {
                requestRender();
                refresh();
            }
        }
        void showResetMenu(std::optional<POINT> at = {}) {
            if (page() || searching() || dragging || (operationBusy && operationBusy())) {
                return;
            }
            if (!numeric->commit()) {
                SetFocus(numeric->handle());
                return;
            }
            POINT point = at.value_or(POINT{px(20), std::clamp(px(rowTop(focusedRow) + 28) - surfaceOffset(),
                                                               0, std::max(0, inspectorHeight - px(8)))});
            if (!at) {
                ClientToScreen(panels[2].window, &point);
            }
            const auto options = SurfaceResetMenu::items(attribute(), selected, surfaceContent.at(focusedRow),
                                                         appearanceHistory && appearanceHistory->canReset());
            const int command =
                SurfaceResetMenu::show(panels[2].window, point, options, UINT(scale * 96 + .5f));
            if (command) {
                executeReset(command);
            }
            SetFocus(panels[2].window);
            InvalidateRect(panels[2].window, nullptr, FALSE);
        }
        void moveFocus(int direction) {
            if (!numeric->commit()) {
                SetFocus(numeric->handle());
                return;
            }
            inspectorTabFocus = -1;
            resetFocused = false;
            const int count = rowCount();
            if (count == 0) {
                return;
            }
            focusedRow = std::clamp(focusedRow + direction, 0, count - 1);
            revealSurfaceFocus();
            InvalidateRect(panels[2].window, nullptr, FALSE);
        }
        void tabInspector(int direction) {
            const int count = rowCount();
            const bool removable = IsWindowVisible(removeEffect) && IsWindowEnabled(removeEffect);
            if (GetFocus() == removeEffect) {
                if (direction < 0) {
                    focusSurface(details ? SurfaceInspectorItems::detail : SurfaceInspectorItems::basic);
                } else {
                    SetFocus(panels[2].window);
                    focusedRow = 0;
                    moveFocus(0);
                }
                return;
            }
            if (removable && ((inspectorTabFocus >= 0 && direction > 0) ||
                              (!resetFocused && inspectorTabFocus < 0 && focusedRow == 0 && direction < 0))) {
                focusSurface(SurfaceInspectorItems::removeEffect);
                return;
            }
            if (inspectorTabFocus >= 0) {
                if (direction < 0) {
                    if (IsWindowVisible(compactCategory)) {
                        focusSurface(SurfaceInspectorItems::category);
                    } else {
                        SetFocus(search);
                    }
                } else {
                    inspectorTabFocus = -1;
                    focusedRow = 0;
                    if (count) {
                        moveFocus(0);
                    } else if (surfaceFooterEnabled()) {
                        resetFocused = true;
                        revealSurfaceFocus();
                    } else {
                        SetFocus(search);
                    }
                }
            } else if (resetFocused) {
                resetFocused = false;
                if (direction < 0) {
                    focusedRow = std::max(0, count - 1);
                    moveFocus(0);
                } else {
                    SetFocus(search);
                }
            } else if (direction > 0 && focusedRow >= count - 1) {
                if (surfaceFooterEnabled()) {
                    focusSurface(SurfaceInspectorItems::footer);
                } else if (layerPanel) {
                    layerPanel->focus();
                } else {
                    SetFocus(search);
                }
            } else if (direction < 0 && focusedRow == 0) {
                if (query.empty()) {
                    focusSurface(details ? SurfaceInspectorItems::detail : SurfaceInspectorItems::basic);
                } else {
                    SetFocus(search);
                }
            } else {
                moveFocus(direction);
            }
            InvalidateRect(panels[2].window, nullptr, FALSE);
        }
        void pageInspector(int direction) {
            if (!numeric->commit()) {
                SetFocus(numeric->handle());
                return;
            }
            inspectorTabFocus = -1;
            resetFocused = false;
            if (flowingSurface()) {
                setSurfaceScroll(shortScroll + direction * std::max(px(76), inspectorHeight - px(32)));
                focusedRow = surfaceContent.indexAt(std::max(0, int(shortScroll / scale) - 152));
                return;
            }
            setSurfaceScroll(scroll + direction * std::max(1, rowViewportHeight() - 32));
            focusedRow = firstSurfaceRow();
            revealSurfaceFocus();
            InvalidateRect(panels[2].window, nullptr, FALSE);
        }
        void toggleSurfaceGroup(int group, bool expanded) {
            if (!numeric->commit()) {
                SetFocus(numeric->handle());
                return;
            }
            surfaceContent.setExpanded(group, expanded);
            refresh();
            focusSurface(SurfaceInspectorContent::groupIds + group);
        }
        void editFocused() {
            if (focusedRow < 0) {
                return;
            }
            if (!numeric->commit()) {
                SetFocus(numeric->handle());
                return;
            }
            moveFocus(0);
            const auto *row = surfaceContent.at(focusedRow);
            if (!row) {
                return;
            }
            if (row->header()) {
                toggleSurfaceGroup(row->group, !surfaceContent.expanded(row->group));
                return;
            }
            if (row->color) {
                chooseColor(panels[2].window, *row->color);
                return;
            }
            const auto *parameter = row->parameter;
            const auto rangeHint = SurfaceInspectorItems::range(*parameter);
            numeric->open(
                numericBounds(), surfaceValueText(*parameter, parameter->value(attribute())),
                [this, parameter](float value) {
                    if (!parameter->valid(value)) {
                        return false;
                    }
                    if (history.set(attribute(), *parameter, value)) {
                        requestRender();
                        refresh();
                    }
                    return true;
                },
                [this] { refresh(); }, rangeHint, std::wstring(displayLabel(*parameter)),
                std::wstring(parameter->id.begin(), parameter->id.end()));
        }
        void chooseColor(HWND owner, const SurfaceColor &parameter) {
            const glm::vec4 before = attribute().*parameter.member;
            COLORREF custom[16]{};
            CHOOSECOLORW picker{};
            picker.lStructSize = sizeof(picker);
            picker.hwndOwner = owner;
            picker.lpCustColors = custom;
            picker.Flags = CC_FULLOPEN | CC_RGBINIT;
            picker.rgbResult = RGB(int(std::round(before.r * 255)), int(std::round(before.g * 255)),
                                   int(std::round(before.b * 255)));
            if (NativeDialogs::chooseColor(&picker)) {
                glm::vec4 value{AttributeFormModel::colorByte(GetRValue(picker.rgbResult)),
                                AttributeFormModel::colorByte(GetGValue(picker.rgbResult)),
                                AttributeFormModel::colorByte(GetBValue(picker.rgbResult)), 1.f};
                if (history.setColor(attribute(), parameter, value)) {
                    requestRender();
                    refresh();
                }
            }
        }
        void paintNavigationContent(HDC dc) {
            const int width = navigationWidth();
            if (const auto *form = page()) {
                text(dc, form->title(), box(18, 12, width - 40, 36), theme.foreground, true);
                for (int i = 0; i < int(form->groups().size()); ++i) {
                    if (i == form->selectedGroup()) {
                        fill(dc, box(10, 64 + i * 40, width - 20, 36), theme.selected);
                    }
                    text(dc, form->groups()[i], box(20, 64 + i * 40, width - 36, 36),
                         i == form->selectedGroup() ? theme.selectedForeground : theme.secondary);
                }
                if (preferencesSaveFailed) {
                    text(dc, L"Preferences not saved",
                         box(12, 84 + int(form->groups().size()) * 40, width - 24, 32), theme.error);
                }
                return;
            }
            text(dc, uiText(TextKey::Effects), box(18, 12, width - 40, 36), theme.foreground, true);
            text(dc, uiText(TextKey::BaseStyle), box(18, 64, width - 114, 28), theme.secondary);
            PanelDrawing::rounded(
                dc, EffectsLayout::studio(width).pixels(scale),
                attribute().studio.use ? theme.selected : theme.field,
                hoveredEffect.kind == EffectsLayout::Kind::STUDIO ? theme.accent : theme.track, px(2));
            text(dc, attribute().studio.use ? L"Studio On" : L"Studio Off", box(width - 84, 64, 72, 28),
                 attribute().studio.use
                     ? (highContrastSettingsMode() ? theme.selectedForeground : theme.accent)
                     : theme.secondary);
            for (int i = 0; i < EffectNavigation::filterCount; ++i) {
                const auto bounds = EffectsLayout::filter(i, width).pixels(scale);
                if (static_cast<int>(navigation.filter) == i) {
                    fill(dc, bounds, theme.selected);
                } else if (hoveredEffect == EffectsLayout::Target{EffectsLayout::Kind::FILTER, i}) {
                    fill(dc, bounds, theme.field);
                }
                PanelDrawing::text(
                    dc, EffectNavigation::filterLabels[i], bounds,
                    static_cast<int>(navigation.filter) == i
                        ? (highContrastSettingsMode() ? theme.selectedForeground : theme.accent)
                        : theme.secondary,
                    font, DT_CENTER);
            }
            const auto categories = navigation.rows(attribute());
            for (int i = 0; i < static_cast<int>(categories.size()); ++i) {
                auto category = categories[i];
                const auto row = EffectsLayout::category(i, width);
                if (category == selected) {
                    fill(dc, row.pixels(scale), theme.selected);
                } else if (hoveredEffect == EffectsLayout::Target{EffectsLayout::Kind::CATEGORY, i}) {
                    fill(dc, row.pixels(scale), theme.field);
                }
                const auto favorite = EffectsLayout::favorite(i, width).pixels(scale);
                if (hoveredEffect == EffectsLayout::Target{EffectsLayout::Kind::FAVORITE, i}) {
                    fill(dc, favorite, theme.field);
                }
                const bool contrastSelected = category == selected && highContrastSettingsMode();
                const auto status = navigation.stateLabel(attribute(), category);
                auto label = EffectsLayout::label(i, width);
                if (status.empty()) {
                    label.y = row.y;
                    label.height = row.height;
                }
                text(dc, categoryLabel(category), label.pixels(scale),
                     category == selected ? theme.selectedForeground : theme.secondary);
                PanelDrawing::text(dc, navigation.favorite(category) ? L"★" : L"☆", favorite,
                                   navigation.favorite(category) ||
                                           hoveredEffect ==
                                               EffectsLayout::Target{EffectsLayout::Kind::FAVORITE, i}
                                       ? theme.accent
                                       : theme.secondary,
                                   font, DT_CENTER);
                if (!status.empty()) {
                    text(dc, status, EffectsLayout::status(i, width).pixels(scale),
                         contrastSelected ? theme.selectedForeground : theme.secondary);
                }
            }
            if (categories.empty()) {
                const auto empty = navigation.emptyText();
                for (int i = 0; i < 2; ++i) {
                    text(dc, empty[i], box(20, EffectsLayout::categoryTop + i * 20, width - 34, 20),
                         theme.secondary);
                }
            }
            if (hoveredEffect.kind == EffectsLayout::Kind::PALETTE) {
                fill(dc, EffectsLayout::palette(int(categories.size()), width).pixels(scale), theme.field);
            }
            text(dc, uiText(TextKey::EditPalette), box(20, paletteActionTop(), width - 34, 36), theme.accent);
            if (preferencesSaveFailed) {
                text(dc, L"Preferences not saved", box(12, paletteActionTop() + 42, width - 24, 32),
                     theme.error);
            }
        }
        void paint(Panel &panel, HDC dc, int width, int height) {
            fill(dc, {0, 0, width, height}, theme.background);
            if (panel.kind == 0) {
                fill(dc, {0, height - std::max(1, px(1)), width, height}, theme.track);
                return;
            }
            if (panel.kind == 1) {
                const int saved = SaveDC(dc);
                if (saved == 0) {
                    return;
                }
                try {
                    OffsetViewportOrgEx(dc, 0, -navigationScroll, nullptr);
                    paintNavigationContent(dc);
                } catch (...) {
                    RestoreDC(dc, saved);
                    throw;
                }
                RestoreDC(dc, saved);
                if (GetFocus() == panel.window) {
                    for (const auto &item : navigationItems()) {
                        if (item.id == navigationFocus) {
                            auto bounds = item.bounds;
                            InflateRect(&bounds, -1, -1);
                            PanelDrawing::border(dc, bounds, theme.accent, std::max(2, px(1)));
                            break;
                        }
                    }
                }
                const auto bar = navigationScrollBar();
                if (bar.visible()) {
                    fill(dc, bar.track, theme.field);
                    fill(dc, bar.thumb, navigationDragging ? theme.accent : theme.secondary);
                }
                return;
            }
            if (searching() || page()) {
                return;
            }
            const int viewportState = SaveDC(dc);
            if (viewportState == 0) {
                return;
            }
            try {
                OffsetViewportOrgEx(dc, 0, -surfaceOffset(), nullptr);
                height = surfaceHeight();
                if (!compact || !query.empty()) {
                    text(dc, query.empty() ? categoryLabel(selected) : uiText(TextKey::SearchResults),
                         box(20, 10, surfaceTrack(), 38), theme.foreground, true);
                }
                if (query.empty()) {
                    fill(dc, surfaceTabBounds(details ? 1 : 0), theme.selected);
                    for (int i = 0; i < 2; ++i) {
                        auto bounds = surfaceTabBounds(i);
                        InflateRect(&bounds, -px(16), 0);
                        text(dc, i ? detailLabel() : L"Basic", bounds,
                             details == bool(i) ? theme.selectedForeground : theme.secondary);
                    }
                } else {
                    const int count = rowCount();
                    text(dc, std::to_wstring(count) + (count == 1 ? L" matching control" : L" matching controls"),
                         box(20, 54, 300, 32), theme.foreground);
                }
                const auto state = effectState(attribute(), selected);
                const std::wstring status = query.empty() ? std::wstring(state.label())
                                            : rowCount()  ? categoryLabel(focusedCategory())
                                                          : L"Try another name.";
                text(dc, numeric->isInvalid() ? numeric->errorMessage() : std::wstring_view(status),
                     box(20, 100, IsWindowVisible(removeEffect) ? surfaceTrack() - 156 : surfaceTrack(), 28),
                     numeric->isInvalid() ? theme.error : theme.secondary);
                scroll = std::clamp(scroll, 0, maximumScroll());
                const int limit = rowCount();
                const int rowsState = SaveDC(dc);
                if (rowsState == 0) {
                    RestoreDC(dc, viewportState);
                    return;
                }
                try {
                    IntersectClipRect(dc, 0, px(152), width, height - px(surfaceFooterHeight()));
                    for (int i = firstSurfaceRow(); i < limit; ++i) {
                        const auto &row = *surfaceContent.at(i);
                        const int y = rowTop(i);
                        if (px(y) >= height - px(surfaceFooterHeight())) {
                            break;
                        }
                        const bool editing = inspectorTabFocus < 0 && !resetFocused && i == focusedRow &&
                                             (GetFocus() == panel.window || GetFocus() == numeric->handle());
                        if (editing) {
                            fill(dc,
                                 row.header() ? box(20, y, surfaceTrack(), 32) : box(10, y - 5, surfaceRight(), 70),
                                 theme.selected);
                        }
                        const bool contrastEditing = editing && highContrastSettingsMode();
                        if (row.header()) {
                            if (!editing) {
                                fill(dc, box(20, y, surfaceTrack(), 32), theme.field);
                            }
                            const COLORREF ink = editing ? theme.selectedForeground : theme.foreground;
                            const auto pen = CreatePen(PS_SOLID, std::max(1, px(1)), ink);
                            const auto old = SelectObject(dc, pen);
                            POINT points[3];
                            if (surfaceContent.expanded(row.group)) {
                                points[0] = {px(27), px(y + 14)};
                                points[1] = {px(31), px(y + 18)};
                                points[2] = {px(35), px(y + 14)};
                            } else {
                                points[0] = {px(29), px(y + 12)};
                                points[1] = {px(33), px(y + 16)};
                                points[2] = {px(29), px(y + 20)};
                            }
                            Polyline(dc, points, 3);
                            SelectObject(dc, old);
                            DeleteObject(pen);
                            const int changed = SurfaceInspectorContent::changedInGroup(row.group, attribute());
                            text(dc, row.label(), box(44, y, surfaceTrack() - 32 - (changed ? 84 : 0), 32), ink);
                            if (changed) {
                                PanelDrawing::text(
                                    dc, std::to_wstring(changed) + L" changed", box(surfaceRight() - 84, y, 76, 32),
                                    editing ? theme.selectedForeground : theme.secondary, font, DT_RIGHT);
                            }
                            if (editing) {
                                PanelDrawing::border(dc, box(20, y, surfaceTrack(), 32), theme.accent,
                                                     std::max(1, px(1)));
                            }
                            continue;
                        }
                        fill(dc, box(20, y + 65, surfaceTrack(), 1), theme.track);
                        if (row.color) {
                            const auto &color = *row.color;
                            const auto value = attribute().*color.member;
                            text(dc, color.label, box(20, y, surfaceLabelWidth(), InspectorLayout::fieldHeight),
                                 editing ? theme.selectedForeground : theme.foreground);
                            PanelDrawing::rounded(dc, box(20, y + 32, 112, 24),
                                                  RGB(int(std::round(value.r * 255)), int(std::round(value.g * 255)),
                                                      int(std::round(value.b * 255))),
                                                  theme.track, px(6));
                            wchar_t hex[16];
                            swprintf(hex, 16, L"#%02X%02X%02X", int(std::round(value.r * 255)),
                                     int(std::round(value.g * 255)), int(std::round(value.b * 255)));
                            text(dc, hex, box(148, y + 30, 150, 28),
                                 contrastEditing ? theme.selectedForeground : theme.secondary);
                            continue;
                        }
                        const auto &p = *row.parameter;
                        const float value = p.value(attribute());
                        text(dc, displayLabel(p), box(20, y, surfaceLabelWidth(), InspectorLayout::fieldHeight),
                             editing ? theme.selectedForeground : theme.foreground);
                        PanelDrawing::rounded(
                            dc, box(surfaceFieldX(), y, InspectorLayout::fieldWidth, InspectorLayout::fieldHeight),
                            theme.field, editing ? theme.accent : theme.track, px(2));
                        const bool inputHere = i == focusedRow && numeric->isEditing();
                        const std::wstring shownNumber = inputHere ? numeric->text() : surfaceValueText(p, value);
                        PanelDrawing::text(dc, shownNumber,
                                           box(surfaceFieldX() + 4, y, InspectorLayout::fieldWidth - 8,
                                               InspectorLayout::fieldHeight),
                                           inputHere && numeric->isInvalid() ? theme.error : theme.foreground, font,
                                           DT_RIGHT);
                        fill(dc, box(20, y + 39, surfaceTrack(), 3),
                             contrastEditing ? theme.selectedForeground : theme.track);
                        const int length = int(surfaceTrack() * p.sliderFraction(value));
                        fill(dc, box(20, y + 39, length, 3),
                             contrastEditing ? theme.selectedForeground : theme.accent);
                        PanelDrawing::rounded(dc, box(15 + length, y + 35, 11, 11),
                                              contrastEditing ? theme.selectedForeground : theme.foreground,
                                              contrastEditing ? theme.selectedForeground : theme.accent, px(11));
                    }
                } catch (...) {
                    RestoreDC(dc, rowsState);
                    throw;
                }
                RestoreDC(dc, rowsState);
                if (!rowCount()) {
                    text(dc, uiText(TextKey::NoResults), box(20, 154, 300, 32), theme.secondary);
                }
                if (!query.empty()) {
                    const bool enabled = surfaceFooterEnabled();
                    PanelDrawing::rounded(
                        dc, {px(20), height - px(38), px(156), height - px(10)}, theme.background,
                        resetFocused && GetFocus() == panel.window ? theme.accent : theme.track, px(2));
                    text(dc, L"Show in effect", {px(28), height - px(38), px(150), height - px(10)},
                         enabled ? theme.foreground : theme.secondary);
                }
            } catch (...) {
                RestoreDC(dc, viewportState);
                throw;
            }
            RestoreDC(dc, viewportState);
            if (inspectorTabFocus >= 0 && GetFocus() == panel.window) {
                auto rect = surfaceTabBounds(inspectorTabFocus);
                OffsetRect(&rect, 0, -surfaceOffset());
                InflateRect(&rect, -1, -1);
                PanelDrawing::border(dc, rect, theme.accent, std::max(2, px(1)));
            }
            const auto bar = scrollBar();
            if (bar.visible()) {
                fill(dc, bar.track, theme.field);
                fill(dc, bar.thumb, scrollDragging ? theme.accent : theme.secondary);
            }
        }
        void moveValue(int x) {
            if (!dragging) {
                return;
            }
            float ratio = std::clamp((x / scale - InspectorLayout::inset) / surfaceTrack(), 0.f, 1.f);
            const float value = dragging->sliderValue(ratio);
            if (history.set(attribute(), *dragging, value)) {
                requestRender();
                InvalidateRect(panels[2].window, nullptr, FALSE);
                if (surfaceAccessibility) {
                    surfaceAccessibility->changed();
                }
            }
        }
        static LRESULT CALLBACK procedure(HWND hwnd, UINT message, WPARAM w, LPARAM l) {
            auto *panel = reinterpret_cast<Panel *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (message == WM_NCCREATE) {
                panel = static_cast<Panel *>(reinterpret_cast<CREATESTRUCTW *>(l)->lpCreateParams);
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(panel));
                panel->window = hwnd;
            }
            if (!panel) {
                return DefWindowProcW(hwnd, message, w, l);
            }
            auto &self = *panel->owner;
            switch (message) {
            case panelCommand: {
                const int slot = int(w);
                if (slot < 0 || slot >= int(self.appearancePanels.size())) {
                    return 0;
                }
                auto next = self.panelLayout;
                if (l == 3) {
                    next.slots[slot].open = false;
                } else if (l >= 0 && l <= 2) {
                    next.slots[slot].side = int(l);
                }
                self.changePanelLayout(next);
                return 0;
            }
            case WM_GETOBJECT:
                if (static_cast<DWORD>(l) == static_cast<DWORD>(OBJID_CLIENT)) {
                    if (panel->kind == 1 && self.navigationAccessibility) {
                        return self.navigationAccessibility->object(w);
                    }
                    if (panel->kind == 2 && self.surfaceAccessibility) {
                        return self.surfaceAccessibility->object(w);
                    }
                }
                break;
            case WM_MEASUREITEM: {
                auto *item = reinterpret_cast<MEASUREITEMSTRUCT *>(l);
                if (SurfaceResetMenu::measure(hwnd, item)) {
                    return TRUE;
                }
                if (item->CtlType == ODT_COMBOBOX) {
                    item->itemHeight = self.px(20);
                    return TRUE;
                }
                break;
            }
            case WM_DRAWITEM: {
                const auto *item = reinterpret_cast<DRAWITEMSTRUCT *>(l);
                if (SurfaceResetMenu::draw(item)) {
                    return TRUE;
                }
                if (item->CtlType == ODT_COMBOBOX) {
                    WorkspaceComboDrawing::draw(*item, self.comboContext, self.px(6));
                    return TRUE;
                }
                if (item->hwndItem == self.removeEffect) {
                    WorkspaceButton::draw(*item, self.comboContext);
                    return TRUE;
                }
                if (panel->kind == 0 && item->CtlType == ODT_BUTTON) {
                    self.drawHeaderButton(*item);
                    return TRUE;
                }
                break;
            }
            case WM_SETFOCUS:
                if (panel->kind == 2 && self.searching()) {
                    self.searchResults->focus();
                    return 0;
                }
                if (panel->kind == 1) {
                    self.normalizeNavigationFocus();
                    if (!self.navigationPointerFocus) {
                        const auto items = self.navigationItems();
                        for (const auto &item : items) {
                            if (item.id == self.navigationFocus) {
                                self.revealNavigationRange(item.bounds.top + self.navigationScroll,
                                                           item.bounds.bottom + self.navigationScroll);
                                break;
                            }
                        }
                    }
                    if (self.navigationAccessibility) {
                        self.navigationAccessibility->changed();
                    }
                }
                if (panel->kind == 2 && self.surfaceAccessibility) {
                    self.surfaceAccessibility->changed();
                }
                InvalidateRect(hwnd, nullptr, FALSE);
                break;
            case WM_KILLFOCUS:
                if (panel->kind == 1 && self.navigationAccessibility) {
                    self.navigationAccessibility->changed();
                }
                InvalidateRect(hwnd, nullptr, FALSE);
                break;
            case WM_GETDLGCODE:
                return DLGC_WANTARROWS | DLGC_WANTTAB;
            case WM_ERASEBKGND:
                return 1;
            case WM_CTLCOLORSTATIC:
            case WM_CTLCOLORLISTBOX:
            case WM_CTLCOLOREDIT:
                SetTextColor(reinterpret_cast<HDC>(w),
                             self.numeric && reinterpret_cast<HWND>(l) == self.numeric->handle() &&
                                     self.numeric->isInvalid()
                                 ? self.theme.error
                                 : self.theme.foreground);
                SetBkColor(reinterpret_cast<HDC>(w), self.theme.field);
                return reinterpret_cast<LRESULT>(self.editBrush);
            case WM_PRINTCLIENT: {
                RECT rc;
                GetClientRect(hwnd, &rc);
                self.paint(*panel, reinterpret_cast<HDC>(w), rc.right, rc.bottom);
                return 0;
            }
            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC target = BeginPaint(hwnd, &ps);
                try {
                    RECT rc;
                    GetClientRect(hwnd, &rc);
                    if (HDC dc = panel->buffer.begin(target, rc.right, rc.bottom)) {
                        self.paint(*panel, dc, rc.right, rc.bottom);
                        panel->buffer.present(target);
                    }
                } catch (...) {
                    EndPaint(hwnd, &ps);
                    throw;
                }
                EndPaint(hwnd, &ps);
                return 0;
            }
            case WM_COMMAND:
                if (panel->kind == 0 && LOWORD(w) >= 3000 && LOWORD(w) < 3009 && HIWORD(w) == BN_CLICKED) {
                    self.activateHeader(LOWORD(w) - 3000);
                    return 0;
                }
                if (HIWORD(w) == CBN_SELCHANGE && !self.numeric->commit()) {
                    self.refresh();
                    SetFocus(self.numeric->handle());
                    return 0;
                }
                if (self.operationBusy && self.operationBusy()) {
                    return 0;
                }
                if (reinterpret_cast<HWND>(l) == self.removeEffect && HIWORD(w) == BN_CLICKED) {
                    if (self.dragging || !self.numeric->commit()) {
                        return 0;
                    }
                    if (self.history.clearCategoryEffects(self.attribute(), self.selected)) {
                        self.focusSurface(self.details ? SurfaceInspectorItems::detail
                                                       : SurfaceInspectorItems::basic);
                        self.requestRender();
                        self.refresh();
                    }
                    return 0;
                }
                if (reinterpret_cast<HWND>(l) == self.workspaceChooser && HIWORD(w) == CBN_SELCHANGE) {
                    const int tab = int(SendMessageW(self.workspaceChooser, CB_GETCURSEL, 0, 0));
                    if (tab >= 0 && tab < 4) {
                        self.selectWorkspace(self.workspaceNavigation.last(tab));
                    }
                    return 0;
                }
                if (reinterpret_cast<HWND>(l) == self.workspaceModule && HIWORD(w) == CBN_SELCHANGE) {
                    const int index = int(SendMessageW(self.workspaceModule, CB_GETCURSEL, 0, 0));
                    if (index >= 0 && index < int(self.moduleIds.size())) {
                        self.selectWorkspace(self.moduleIds[index]);
                    }
                    return 0;
                }
                if (reinterpret_cast<HWND>(l) == self.compactCategory && HIWORD(w) == CBN_SELCHANGE) {
                    self.openEffect(
                        static_cast<Category>(SendMessageW(self.compactCategory, CB_GETCURSEL, 0, 0)));
                    self.query.clear();
                    SetWindowTextW(self.search, L"");
                    self.scroll = 0;
                    self.shortScroll = 0;
                    self.refresh();
                    return 0;
                }
                if (reinterpret_cast<HWND>(l) == self.baseStyle && HIWORD(w) == CBN_SELCHANGE) {
                    const int choice = int(SendMessageW(self.baseStyle, CB_GETCURSEL, 0, 0));
                    if (self.selectBaseStyle(static_cast<ShdSurfaceStyle>(choice))) {
                        self.requestRender();
                        self.refresh();
                    }
                    return 0;
                }
                break;
            case WM_LBUTTONDOWN: {
                if (self.operationBusy && self.operationBusy()) {
                    return 0;
                }
                self.navigationPointerFocus = panel->kind == 1;
                SetFocus(hwnd);
                self.navigationPointerFocus = false;
                const int x = GET_X_LPARAM(l),
                          y = int((GET_Y_LPARAM(l) + (panel->kind == 2   ? self.surfaceOffset()
                                                      : panel->kind == 1 ? self.navigationScroll
                                                                         : 0)) /
                                  self.scale);
                if (!self.numeric->commit()) {
                    SetFocus(self.numeric->handle());
                    return 0;
                }
                if (panel->kind == 0) {
                    if (HeaderLayout::contains(self.headerLayout.inspector, x, GET_Y_LPARAM(l))) {
                        self.activateHeader(8);
                        return 0;
                    }
                    if (self.compact &&
                        HeaderLayout::contains(self.headerLayout.navigation, x, GET_Y_LPARAM(l))) {
                        self.activateHeader(7);
                        return 0;
                    }
                    if (!self.headerLayout.chooser) {
                        for (int i = 0; i < 4; ++i) {
                            if (HeaderLayout::contains(self.headerLayout.tabs[i], x, GET_Y_LPARAM(l))) {
                                self.activateHeader(i + 3);
                                return 0;
                            }
                        }
                    }
                } else if (panel->kind == 1) {
                    const auto bar = self.navigationScrollBar();
                    const int pointerY = GET_Y_LPARAM(l);
                    if (bar.visible() && x >= bar.track.left - self.px(2) && pointerY >= bar.track.top &&
                        pointerY < bar.track.bottom) {
                        if (pointerY >= bar.thumb.top && pointerY < bar.thumb.bottom) {
                            self.navigationDragging = true;
                            self.navigationGrab = pointerY - bar.thumb.top;
                            SetCapture(hwnd);
                        } else {
                            self.setNavigationScroll(
                                self.navigationScroll +
                                (pointerY < bar.thumb.top ? -1 : 1) *
                                    std::max(self.px(40), self.navigationHeight - self.px(32)));
                        }
                        return 0;
                    }
                    for (const auto &item : self.navigationItems()) {
                        if (PtInRect(&item.bounds, POINT{x, GET_Y_LPARAM(l)})) {
                            self.activateNavigation(item.id);
                            break;
                        }
                    }
                } else {
                    if (auto *form = self.page()) {
                        form->focus();
                        return 0;
                    }
                    const auto bar = self.scrollBar();
                    if (bar.visible() && x >= bar.track.left - self.px(4) &&
                        GET_Y_LPARAM(l) >= bar.track.top && GET_Y_LPARAM(l) < bar.track.bottom) {
                        self.numeric->cancel();
                        const int pointerY = GET_Y_LPARAM(l);
                        if (pointerY >= bar.thumb.top && pointerY < bar.thumb.bottom) {
                            self.scrollDragging = true;
                            self.scrollGrabOffset = pointerY - bar.thumb.top;
                            SetCapture(hwnd);
                        } else if (self.flowingSurface()) {
                            self.setSurfaceScroll(
                                self.shortScroll +
                                (pointerY < bar.thumb.top ? -1 : 1) *
                                    std::max(self.px(76), self.inspectorHeight - self.px(32)));
                        } else {
                            self.setSurfaceScroll(self.scroll +
                                                  (pointerY < bar.thumb.top ? -1 : 1) *
                                                      std::max(1, self.rowViewportHeight() - 32));
                        }
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                    if (!self.query.empty() &&
                        GET_Y_LPARAM(l) + self.surfaceOffset() >= self.surfaceHeight() - self.px(38) &&
                        GET_Y_LPARAM(l) + self.surfaceOffset() < self.surfaceHeight() - self.px(10) &&
                        x >= self.px(20) && x < self.px(156)) {
                        self.activateInspectorFooter();
                        return 0;
                    }
                    if (self.query.empty() && y >= 54 && y < 86 && x >= self.px(20) &&
                        x < self.px(self.surfaceRight())) {
                        self.activateSurface(x >= self.px(20 + self.surfaceTabWidth())
                                                 ? SurfaceInspectorItems::detail
                                                 : SurfaceInspectorItems::basic);
                    } else if (y >= 152 && GET_Y_LPARAM(l) + self.surfaceOffset() <
                                               self.surfaceHeight() - self.px(self.surfaceFooterHeight())) {
                        const int row =
                            self.surfaceContent.indexAt((self.flowingSurface() ? 0 : self.scroll) + y - 152);
                        const auto *item = self.surfaceContent.at(row);
                        if (!item || y >= self.rowTop(row) + item->height()) {
                            return 0;
                        }
                        if (item->header() || item->color) {
                            self.focusedRow = row;
                            self.editFocused();
                            return 0;
                        }
                        if (item->parameter) {
                            self.focusedRow = row;
                            self.inspectorTabFocus = -1;
                            self.resetFocused = false;
                            InvalidateRect(hwnd, nullptr, FALSE);
                            if (x >= self.px(self.surfaceFieldX()) && x < self.px(self.surfaceRight()) &&
                                y - self.rowTop(row) < 28) {
                                self.editFocused();
                                return 0;
                            }
                            if (x >= self.px(12) && x < self.px(self.surfaceRight() + 8) &&
                                y - self.rowTop(row) >= 30 && y - self.rowTop(row) < 56) {
                                self.dragging = item->parameter;
                                self.history.begin(self.attribute(), std::wstring(self.dragging->label));
                                SetCapture(hwnd);
                                self.moveValue(x);
                            }
                        }
                    }
                }
                return 0;
            }
            case WM_MOUSEMOVE:
                if (self.navigationDragging && GetCapture() == hwnd) {
                    self.setNavigationScroll(
                        self.navigationScrollBar().positionAt(GET_Y_LPARAM(l), self.navigationGrab));
                    return 0;
                }
                if (self.scrollDragging && GetCapture() == hwnd) {
                    self.setSurfaceScroll(
                        self.scrollBar().positionAt(GET_Y_LPARAM(l), self.scrollGrabOffset));
                    return 0;
                }
                if (panel->kind == 1 && !self.page()) {
                    const auto target =
                        EffectsLayout::hit(GET_X_LPARAM(l), GET_Y_LPARAM(l) + self.navigationScroll,
                                           int(self.navigation.rows(self.attribute()).size()), self.scale,
                                           self.navigationWidth());
                    if (target != self.hoveredEffect) {
                        self.hoveredEffect = target;
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    TRACKMOUSEEVENT tracking{sizeof(tracking), TME_LEAVE, hwnd, 0};
                    TrackMouseEvent(&tracking);
                    SetCursor(
                        LoadCursor(nullptr, target.kind == EffectsLayout::Kind::NONE ? IDC_ARROW : IDC_HAND));
                }
                if (self.dragging && GetCapture() == hwnd) {
                    self.moveValue(GET_X_LPARAM(l));
                }
                return 0;
            case WM_MOUSELEAVE:
                if (panel->kind == 1) {
                    self.hoveredEffect = {};
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            case WM_CONTEXTMENU:
                if (panel->kind == 2 && !self.page() && !self.searching()) {
                    POINT point{GET_X_LPARAM(l), GET_Y_LPARAM(l)};
                    if (point.x != -1 || point.y != -1) {
                        POINT local = point;
                        ScreenToClient(hwnd, &local);
                        const int y = int((local.y + self.surfaceOffset()) / self.scale);
                        if (y >= 152 && local.y + self.surfaceOffset() <
                                            self.surfaceHeight() - self.px(self.surfaceFooterHeight())) {
                            const int row = self.surfaceContent.indexAt(
                                (self.flowingSurface() ? 0 : self.scroll) + y - 152);
                            if (const auto *item = self.surfaceContent.at(row)) {
                                if (!self.focusSurface(item->id)) {
                                    return 0;
                                }
                            }
                        }
                        self.showResetMenu(point);
                    } else {
                        self.showResetMenu();
                    }
                    return 0;
                }
                break;
            case WM_LBUTTONUP:
                if (self.navigationDragging) {
                    self.navigationDragging = false;
                    ReleaseCapture();
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                if (self.scrollDragging) {
                    self.scrollDragging = false;
                    ReleaseCapture();
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                if (self.dragging) {
                    self.history.commit(self.attribute());
                    self.dragging = nullptr;
                    ReleaseCapture();
                    self.refresh();
                }
                return 0;
            case WM_CAPTURECHANGED:
                if (self.navigationDragging) {
                    self.navigationDragging = false;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                if (self.scrollDragging) {
                    self.scrollDragging = false;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                if (self.dragging) {
                    self.history.commit(self.attribute());
                    self.dragging = nullptr;
                    self.refresh();
                }
                return 0;
            case WM_MOUSEWHEEL:
                if (panel->kind == 1) {
                    self.navigationWheel += GET_WHEEL_DELTA_WPARAM(w);
                    const int steps = self.navigationWheel / WHEEL_DELTA;
                    self.navigationWheel %= WHEEL_DELTA;
                    if (steps) {
                        self.setNavigationScroll(self.navigationScroll - steps * self.px(80));
                    }
                    return 0;
                }
                if (panel->kind == 2) {
                    if (!self.numeric->commit()) {
                        SetFocus(self.numeric->handle());
                        return 0;
                    }
                    self.setSurfaceScroll((self.flowingSurface() ? self.shortScroll : self.scroll) -
                                          MulDiv(GET_WHEEL_DELTA_WPARAM(w),
                                                 self.flowingSurface() ? self.px(32) : 32, WHEEL_DELTA));
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            case WM_KEYDOWN:
                if (self.operationBusy && self.operationBusy()) {
                    return 0;
                }
                if (w == VK_ESCAPE && self.navigationDragging) {
                    self.navigationDragging = false;
                    ReleaseCapture();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                if (w == VK_ESCAPE && self.scrollDragging) {
                    self.scrollDragging = false;
                    ReleaseCapture();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                if ((GetKeyState(VK_CONTROL) & 0x8000) && w == 'F') {
                    SetFocus(self.search);
                    SendMessageW(self.search, EM_SETSEL, 0, -1);
                    return 0;
                }
                if ((GetKeyState(VK_CONTROL) & 0x8000) && (w == 'Z' || w == 'Y')) {
                    self.undoRedo(w == 'Y');
                    return 0;
                }
                if (panel->kind == 1 && self.navigationKey(w)) {
                    return 0;
                }
                if (auto *form = self.page()) {
                    if (w == VK_TAB) {
                        if (panel->kind == 0) {
                            SetFocus(self.search);
                        } else if (!self.inspectorOpen) {
                            self.focusHeader();
                        } else {
                            form->focus();
                        }
                        return 0;
                    }
                    if (panel->kind != 0) {
                        return 0;
                    }
                }
                if (panel->kind == 2) {
                    if (w == VK_BACK && (GetKeyState(VK_CONTROL) & 0x8000) && !self.resetFocused) {
                        self.executeReset(SurfaceResetMenu::VALUE);
                        return 0;
                    }
                    if (w == VK_TAB) {
                        self.tabInspector((GetKeyState(VK_SHIFT) & 0x8000) ? -1 : 1);
                        return 0;
                    }
                    if (self.inspectorTabFocus >= 0) {
                        if (w == VK_LEFT || w == VK_HOME) {
                            self.activateSurface(SurfaceInspectorItems::basic);
                            return 0;
                        }
                        if (w == VK_RIGHT || w == VK_END) {
                            self.activateSurface(SurfaceInspectorItems::detail);
                            return 0;
                        }
                        if (w == VK_RETURN || w == VK_SPACE) {
                            self.activateSurface(self.inspectorTabFocus ? SurfaceInspectorItems::detail
                                                                        : SurfaceInspectorItems::basic);
                            return 0;
                        }
                        if (w == VK_DOWN) {
                            self.focusedRow = 0;
                            self.moveFocus(0);
                            return 0;
                        }
                        if (w == VK_UP) {
                            if (IsWindowVisible(self.compactCategory)) {
                                self.focusSurface(SurfaceInspectorItems::category);
                            } else {
                                SetFocus(self.search);
                            }
                            return 0;
                        }
                    }
                    if (!self.resetFocused) {
                        if (const auto *row = self.surfaceContent.at(self.focusedRow); row && row->header()) {
                            if (w == VK_LEFT || w == VK_RIGHT || w == VK_SPACE || w == VK_RETURN) {
                                self.toggleSurfaceGroup(
                                    row->group, w == VK_RIGHT || (w != VK_LEFT &&
                                                                  !self.surfaceContent.expanded(row->group)));
                                return 0;
                            }
                        }
                    }
                    if (w == VK_UP || w == VK_DOWN) {
                        self.moveFocus(w == VK_DOWN ? 1 : -1);
                        return 0;
                    }
                    if (w == VK_HOME || w == VK_END) {
                        self.moveFocus(w == VK_HOME ? -10000 : 10000);
                        return 0;
                    }
                    if (w == VK_PRIOR || w == VK_NEXT) {
                        self.pageInspector(w == VK_NEXT ? 1 : -1);
                        return 0;
                    }
                    if (w == VK_RETURN || (w == VK_SPACE && self.resetFocused)) {
                        if (self.resetFocused) {
                            self.activateInspectorFooter();
                        } else {
                            self.editFocused();
                        }
                        return 0;
                    }
                }
                if (w == VK_TAB) {
                    if (panel->kind == 0) {
                        SetFocus(self.search);
                    } else if (!self.inspectorOpen) {
                        self.focusHeader();
                    } else {
                        SetFocus(self.panels[2].window);
                    }
                    return 0;
                }
                if (w == VK_ESCAPE && self.compact && self.navigationOpen && !self.dragging) {
                    self.toggleNavigation();
                    return 0;
                }
                if (panel->kind == 2 && !self.resetFocused && (w == VK_LEFT || w == VK_RIGHT)) {
                    if (const auto *parameter = self.rowParameter(self.focusedRow)) {
                        const auto &p = *parameter;
                        const float value = p.nudged(p.value(self.attribute()), w == VK_RIGHT ? 1 : -1);
                        if (self.history.set(self.attribute(), p, value)) {
                            self.requestRender();
                            self.refresh();
                        }
                        return 0;
                    }
                }
                if (w == VK_ESCAPE && self.dragging) {
                    self.history.cancel(self.attribute());
                    self.dragging = nullptr;
                    ReleaseCapture();
                    self.requestRender();
                    self.refresh();
                    return 0;
                }
                break;
            }
            return DefWindowProcW(hwnd, message, w, l);
        }

      public:
        void bindAppearanceReset(std::function<ShaderAttribute &()> getter) {
            appearanceTracker = std::make_shared<AppearanceEditTracker>(getter);
            appearanceHistory = std::make_unique<AppearanceResetHistory>(std::move(getter));
            appearanceHistory->bindHistory(sharedHistory);
            history.observeEdits(
                [tracker = appearanceTracker](const auto &prior, std::wstring label) {
                    tracker->beginSurface(prior, std::move(label));
                },
                [tracker = appearanceTracker](bool accepted) { tracker->endSurface(accepted); });
        }
        std::shared_ptr<AppearanceEditTracker> appearanceEdits() const {
            return appearanceTracker;
        }
        void setOperationGuard(std::function<bool()> busy) {
            operationBusy = std::move(busy);
        }
        WorkspaceShell(HWND parent, std::function<ShdSlopeAttribute &()> getter, std::function<void()> render,
                       std::function<void(int)> action, std::filesystem::path preferences = {})
            : attribute(std::move(getter)), requestRender(std::move(render)),
              openWorkspace(std::move(action)), preferencesPath(std::move(preferences)) {
            if (!preferencesPath.empty()) {
                WorkspacePreferences::load(preferencesPath, navigation, &paneWidths, &panelLayout);
            }
            layerHeight = panelLayout.layerHeight;
            scale = UiDpi::forWindow(parent) / 96.f;
            editBrush = CreateSolidBrush(theme.field);
            font = CreateFontW(-px(13), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH,
                               UiLanguage::fontFace());
            heading = CreateFontW(-px(16), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH,
                                  UiLanguage::fontFace());
            brandFont = CreateFontW(-px(20), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH,
                                    UiLanguage::fontFace());
            comboContext = {&theme, font, scale};
            WNDCLASSW cls{};
            cls.hInstance = GetModuleHandleW(nullptr);
            cls.lpfnWndProc = procedure;
            cls.lpszClassName = L"RFF.Workspace.Panel";
            cls.hCursor = LoadCursor(nullptr, IDC_ARROW);
            RegisterClassW(&cls);
            for (int i = 0; i < 3; ++i) {
                panels[i].owner = this;
                panels[i].kind = i;
                // Paint the inspector and its native inputs together when changing forms.
                const DWORD extendedStyle = 0;
                CreateWindowExW(extendedStyle, cls.lpszClassName, L"",
                                WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 1, 1, parent,
                                nullptr, cls.hInstance, &panels[i]);
            }
            const wchar_t *headerLabels[] = {
                uiText(TextKey::Undo),    uiText(TextKey::Redo),       uiText(TextKey::Save),
                uiText(TextKey::Explore), uiText(TextKey::Appearance), uiText(TextKey::Animation),
                uiText(TextKey::Export),  uiText(TextKey::Effects),    L"Hide Settings"};
            for (int i = 0; i < int(headerButtons.size()); ++i) {
                headerButtons[i] = CreateWindowExW(
                    0, L"BUTTON", headerLabels[i], WS_CHILD | WS_TABSTOP | BS_OWNERDRAW, 0, 0, 1, 1,
                    panels[0].window, reinterpret_cast<HMENU>(INT_PTR(3000 + i)), cls.hInstance, nullptr);
                SendMessageW(headerButtons[i], WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
                if (i < 3 || i >= 7) {
                    WorkspaceButton::attach(headerButtons[i]);
                }
                SetWindowSubclass(headerButtons[i], headerControlProcedure, 1,
                                  reinterpret_cast<DWORD_PTR>(this));
            }
            workspaceModule = CreateWindowExW(0, L"COMBOBOX", L"",
                                              WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED |
                                                  CBS_HASSTRINGS | WS_VSCROLL,
                                              0, 0, 1, 1, panels[0].window, nullptr, cls.hInstance, nullptr);
            SetWindowSubclass(workspaceModule, headerControlProcedure, 1, reinterpret_cast<DWORD_PTR>(this));
            SendMessageW(workspaceModule, CB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(UiLanguage::label(L"Surface Effects")));
            SendMessageW(workspaceModule, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
            WorkspaceComboDrawing::attach(workspaceModule, comboContext);
            workspaceChooser = CreateWindowExW(0, L"COMBOBOX", L"",
                                               WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED |
                                                   CBS_HASSTRINGS,
                                               0, 0, 1, 1, panels[0].window, nullptr, cls.hInstance, nullptr);
            for (int i = 3; i < 7; ++i) {
                SendMessageW(workspaceChooser, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(headerLabels[i]));
            }
            SendMessageW(workspaceChooser, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
            WorkspaceComboDrawing::attach(workspaceChooser, comboContext);
            SetWindowSubclass(workspaceChooser, headerControlProcedure, 1, reinterpret_cast<DWORD_PTR>(this));
            baseStyle = CreateWindowExW(0, L"COMBOBOX", L"",
                                        WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST |
                                            CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL,
                                        px(18), px(104), px(186), px(280), panels[1].window, nullptr,
                                        cls.hInstance, nullptr);
            for (const wchar_t *label : {L"Original", L"Liquid Metal", L"Cyber Sigilism", L"PHONK",
                                         L"Black Metal", L"Deep Sea", L"Ukiyo-e"}) {
                SendMessageW(baseStyle, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(UiLanguage::label(label)));
            }
            SendMessageW(baseStyle, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
            WorkspaceComboDrawing::attach(baseStyle, comboContext);
            SetWindowSubclass(baseStyle, navigationControlProcedure, 1, reinterpret_cast<DWORD_PTR>(this));
            searchControl = std::make_unique<SearchField>(
                panels[0].window, font, theme, scale,
                [this](std::wstring value) {
                    if (numeric && !numeric->commit()) {
                        if (value != query) {
                            SetWindowTextW(search, query.c_str());
                        }
                        SetFocus(numeric->handle());
                        return;
                    }
                    updateSearch(std::move(value));
                },
                [this](SearchField::Navigation destination) {
                    if (destination == SearchField::Navigation::PREVIOUS) {
                        advanceHeader(search, -1);
                    } else if (destination == SearchField::Navigation::NEXT) {
                        advanceHeader(search, 1);
                    } else {
                        showInspector();
                        focusInspector();
                    }
                    InvalidateRect(panels[2].window, nullptr, FALSE);
                });
            search = searchControl->editor();
            searchResults = std::make_unique<SettingsSearchPanel>(
                panels[2].window, font, heading, theme, scale,
                [this](SettingsSearchTarget target) { openSearchResult(target); },
                [this](bool clear) {
                    SetFocus(search);
                    if (clear) {
                        SetWindowTextW(search, L"");
                    }
                });
            indexSurface();
            history.bindHistory(sharedHistory);
            numeric = std::make_unique<NumericField>(
                panels[2].window, font, [this](int direction) { tabInspector(direction); },
                [this] { InvalidateRect(panels[2].window, nullptr, FALSE); });
            WorkspaceEditDrawing::attach(numeric->handle(), comboContext);
            removeEffect =
                CreateWindowExW(0, L"BUTTON", L"Remove added effects", WS_CHILD | WS_TABSTOP | BS_OWNERDRAW,
                                0, 0, 1, 1, panels[2].window, nullptr, cls.hInstance, nullptr);
            WorkspaceButton::attach(removeEffect);
            SetWindowSubclass(
                removeEffect,
                [](HWND window, UINT message, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR data) -> LRESULT {
                    auto &self = *reinterpret_cast<WorkspaceShell *>(data);
                    if (message == WM_GETDLGCODE) {
                        return DLGC_WANTTAB | DLGC_BUTTON;
                    }
                    if (message == WM_KEYDOWN && w == VK_TAB) {
                        self.tabInspector((GetKeyState(VK_SHIFT) & 0x8000) ? -1 : 1);
                        return 0;
                    }
                    if (message == WM_KEYDOWN && w == VK_RETURN) {
                        SendMessageW(window, BM_CLICK, 0, 0);
                        return 0;
                    }
                    return DefSubclassProc(window, message, w, l);
                },
                5, reinterpret_cast<DWORD_PTR>(this));
            AccessibleControl::describe(removeEffect, L"Remove added effects",
                                        L"Remove this category's added effects. Keep the base style and "
                                        L"control values. Undo restores the effects.",
                                        L"surface.removeAdded");
            compactCategory = CreateWindowExW(0, L"COMBOBOX", L"",
                                              WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED |
                                                  CBS_HASSTRINGS | WS_VSCROLL,
                                              px(20), px(12), px(InspectorLayout::trackWidth), px(340),
                                              panels[2].window, nullptr, cls.hInstance, nullptr);
            for (int i = 0; i < 10; ++i) {
                const auto label = categoryLabel(static_cast<Category>(i));
                SendMessageW(compactCategory, CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>(UiLanguage::text(label).c_str()));
            }
            SendMessageW(compactCategory, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
            WorkspaceComboDrawing::attach(compactCategory, comboContext);
            SetWindowSubclass(compactCategory, inspectorControlProcedure, 1,
                              reinterpret_cast<DWORD_PTR>(this));
            AccessibleControl::describe(workspaceModule, L"Settings module",
                                        L"Choose a module within the current workspace.",
                                        L"workspace.module");
            AccessibleControl::describe(workspaceChooser, L"Workspace",
                                        L"Choose Explore, Appearance, Animation or Export.",
                                        L"workspace.choose");
            AccessibleControl::describe(baseStyle, L"Base Style",
                                        L"Choose the starting surface style. Every effect remains editable.",
                                        L"surface.style");
            AccessibleControl::describe(compactCategory, L"Effect category",
                                        L"Choose the effect settings to edit.", L"surface.category");
            for (int side = 0; side < 2; ++side) {
                dockHandles[side] = std::make_unique<PanelDockHandle>(
                    parent, side == 0 ? L"Drag section list" : L"Drag settings panel",
                    [this] {
                        if (operationBusy && operationBusy()) {
                            return false;
                        }
                        if (!numeric->commit()) {
                            SetFocus(numeric->handle());
                            return false;
                        }
                        return true;
                    },
                    [this, side](bool right, bool outside) {
                        auto next = panelLayout;
                        if (side == 0) {
                            next.navigationRight = right;
                        } else {
                            next.mainLeft = !right;
                        }
                        next.navigationOuter = outside ? (side == 0) : (side != 0);
                        if (next != panelLayout) {
                            changePanelLayout(next);
                        }
                    },
                    [this, side](int direction) { advanceHeader(dockHandles[side]->handle(), direction); });
                splitters[side] = std::make_unique<PaneSplitter>(
                    parent, side == 0, side == 0 ? L"Resize navigation panel" : L"Resize settings panel",
                    [this, side] {
                        if (operationBusy && operationBusy()) {
                            return false;
                        }
                        resizeStartWidths = paneWidths;
                        resizeStartWidth = side == 0 ? navigationWidth() : int(inspectorWidth / scale + .5f);
                        return true;
                    },
                    [this, side](int delta) {
                        int &value = side == 0 ? paneWidths.navigation : paneWidths.inspector;
                        value = delta == 0
                                    ? (side == 0 ? resizeStartWidths.navigation : resizeStartWidths.inspector)
                                    : resizeStartWidth +
                                          ((side == 0 ? !panelLayout.navigationRight : panelLayout.mainLeft)
                                               ? 1
                                               : -1) *
                                              int(std::round(delta / scale));
                        paneWidths.normalize();
                        requestLayout();
                    },
                    [this](bool cancel) {
                        if (cancel) {
                            paneWidths = resizeStartWidths;
                            requestLayout();
                        } else if (paneWidths != resizeStartWidths) {
                            savePreferences();
                        }
                    },
                    [this, side] {
                        if (operationBusy && operationBusy()) {
                            return;
                        }
                        if (side == 0) {
                            paneWidths.navigation = PaneWidths{}.navigation;
                        } else {
                            paneWidths.inspector = PaneWidths{}.inspector;
                        }
                        requestLayout();
                        savePreferences();
                    },
                    [this, side](int direction) { advanceHeader(splitters[side]->handle(), direction); });
            }
            navigationAccessibility = std::make_unique<AccessibleItems>(
                panels[1].window, [this] { return page() ? page()->title() : L"Effects"; },
                [this] { return navigationItems(); },
                [this](long id, bool activate) {
                    return activate ? activateNavigation(id) : focusNavigation(id);
                });
            surfaceAccessibility = std::make_unique<AccessibleItems>(
                panels[2].window,
                [this] {
                    return searching() ? L"Search Results"
                           : page()    ? page()->title()
                                       : categoryLabel(selected);
                },
                [this] { return surfaceItems(); },
                [this](long id, bool activate) { return activate ? activateSurface(id) : focusSurface(id); },
                [this](long id, std::wstring_view text) { return setSurfaceValue(id, text); });
            refresh();
        }
        ~WorkspaceShell() {
            for (auto &handle : dockHandles) {
                handle.reset();
            }
            for (auto &panel : appearancePanels) {
                panel.reset();
            }
            layerDivider.reset();
            layerPanel.reset();
            if (layerHost) {
                DestroyWindow(layerHost);
            }
            surfaceAccessibility.reset();
            navigationAccessibility.reset();
            for (auto &splitter : splitters) {
                splitter.reset();
            }
            searchResults.reset();
            workspaces.clear();
            searchControl.reset();
            numeric.reset();
            for (auto &p : panels) {
                if (IsWindow(p.window)) {
                    DestroyWindow(p.window);
                }
                p.buffer.reset();
            }
            DeleteObject(font);
            DeleteObject(heading);
            DeleteObject(brandFont);
            DeleteObject(editBrush);
        }
        std::shared_ptr<HistoryDomain> historyDomain() const {
            return sharedHistory;
        }
        void navigateHistory(bool redo) {
            undoRedo(redo);
        }
        void bindPreviewFocus(std::function<HWND()> target) {
            previewFocusTarget = std::move(target);
        }
        bool handleShortcut(const MSG &message) {
            if (message.message != WM_KEYDOWN || (GetKeyState(VK_MENU) & 0x8000)) {
                return false;
            }
            if (PaneSplitter::handleCapturedShortcut(message)) {
                return true;
            }
            const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0,
                       shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            if (ctrl && message.wParam == 'F') {
                SetFocus(search);
                SendMessageW(search, EM_SETSEL, 0, -1);
                return true;
            }
            if (ctrl && (message.wParam == 'Z' || message.wParam == 'Y')) {
                wchar_t type[32];
                GetClassNameW(message.hwnd, type, 32);
                if (_wcsicmp(type, L"Edit")) {
                    undoRedo(message.wParam == 'Y' || shift);
                    return true;
                }
            }
            if (ctrl && message.wParam == VK_TAB) {
                const int tab = workspaceNavigation.tab(activeWorkspace);
                selectWorkspace(workspaceNavigation.last((tab + (shift ? 3 : 1)) % 4));
                if (IsWindowVisible(workspaceModule)) {
                    SetFocus(workspaceModule);
                } else {
                    focusHeader();
                }
                return true;
            }
            if (message.wParam == VK_F6 && !ctrl) {
                std::vector<HWND> regions{panels[0].window};
                if (IsWindowVisible(panels[1].window)) {
                    regions.push_back(panels[1].window);
                }
                if (inspectorOpen) {
                    regions.push_back(panels[2].window);
                    if (layerPanel) {
                        regions.push_back(layerPanel->handle());
                    }
                }
                for (const auto &panel : appearancePanels) {
                    if (panel && IsWindowVisible(panel->handle())) {
                        regions.push_back(panel->handle());
                    }
                }
                const HWND preview = previewFocusTarget ? previewFocusTarget() : nullptr;
                regions.push_back(preview && IsWindowVisible(preview) ? preview
                                                                      : GetParent(panels[0].window));
                const HWND current = GetFocus();
                int index = int(regions.size()) - 1;
                for (size_t i = 0; i + 1 < regions.size(); ++i) {
                    if (current == regions[i] || IsChild(regions[i], current)) {
                        index = int(i);
                        break;
                    }
                }
                const auto target = regions[(index + (shift ? int(regions.size()) - 1 : 1)) % regions.size()];
                if (target == panels[0].window) {
                    focusHeader();
                } else if (target == panels[2].window) {
                    focusInspector();
                } else {
                    SetFocus(target);
                }
                return true;
            }
            if (!ctrl && message.wParam == VK_TAB &&
                (message.hwnd == workspaceModule || message.hwnd == workspaceChooser ||
                 std::find(headerButtons.begin(), headerButtons.end(), message.hwnd) !=
                     headerButtons.end())) {
                advanceHeader(message.hwnd, shift ? -1 : 1);
                return true;
            }
            return false;
        }
        void installWorkspace(int id, WorkspaceForm form, int parentTab = -1) {
            if (appearanceTracker && id >= 10 && id <= 16) {
                form = appearanceTracker->wrap(std::move(form));
            }
            if (form.bindHistory) {
                form.bindHistory(sharedHistory);
            }
            form.requestHistory = [this](bool redo) { undoRedo(redo); };
            workspaceNavigation.add(id, parentTab < 0 ? (id < 4 ? id : 1) : parentTab, form.title);
            moduleTab = -1;
            form.canEdit = [this] { return !operationBusy || !operationBusy(); };
            workspaces[id] = std::make_unique<FormWorkspace>(
                panels[2].window, std::move(form), font, theme, scale,
                [this] {
                    updateHeader();
                    if (layerPanel) {
                        layerPanel->refresh();
                    }
                    notifyWorkspace();
                },
                [this](int direction) {
                    if (direction > 0 && layerPanel && inspectorOpen) {
                        layerPanel->focus();
                    } else if (direction < 0 && IsWindowVisible(panels[1].window)) {
                        SetFocus(panels[1].window);
                    } else {
                        focusHeader(-1);
                    }
                },
                [this] {
                    query.clear();
                    SetWindowTextW(search, L"");
                });
            indexForm(id);
            RECT r;
            GetClientRect(panels[2].window, &r);
            workspaces[id]->layout(r.right, r.bottom, compact);
            workspaces[id]->show(id == activeWorkspace);
            refresh();
        }
        void selectWorkspace(int id, std::string_view field = {}, int section = -1) {
            if (operationBusy && operationBusy()) {
                return;
            }
            if (id == 1 && section == int(Category::CONTOUR)) {
                selectWorkspace(16, {}, 1);
                return;
            }
            if (id == 1 && section == int(Category::RELIEF)) {
                selectWorkspace(10, {}, 0);
                return;
            }
            if (id == 18 && layerPanel) {
                showInspector();
                layerPanel->focus();
                return;
            }
            if (id != 1 && !workspaces.contains(id)) {
                openWorkspace(id);
                return;
            }
            if (!numeric->commit()) {
                SetFocus(numeric->handle());
                return;
            }
            if (dragging) {
                history.commit(attribute());
                dragging = nullptr;
                ReleaseCapture();
            }
            if (const auto target = workspaces.find(id); target != workspaces.end()) {
                if (!field.empty()) {
                    target->second->revealField(field, false);
                } else {
                    target->second->search(L"");
                    if (section >= 0) {
                        target->second->selectGroup(section);
                    }
                }
            } else if (id == 1 && section >= 0) {
                openEffect(static_cast<Category>(section));
                details = true;
                scroll = shortScroll = 0;
            }
            activeWorkspace = id;
            query.clear();
            if (GetWindowTextLengthW(search)) {
                SetWindowTextW(search, L"");
            }
            showInspector();
            navigationScroll = 0;
            workspaceNavigation.selected(id);
            for (auto &[index, form] : workspaces) {
                form->show(index == id);
            }
            refresh();
            notifyWorkspace();
            RedrawWindow(panels[2].window, nullptr, nullptr,
                         RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
        }
        void observeWorkspace(std::function<void(int, int, const RECT &)> observer) {
            workspaceObserver = std::move(observer);
            notifyWorkspace();
        }
        WorkspaceForm surfacePanelForm() {
            WorkspaceForm form;
            form.title = L"Surface Effects";
            for (int i = 0; i < 10; ++i) {
                form.groups.push_back(categoryLabel(Category(i)));
            }
            form.groups.push_back(L"Base Style");
            for (const auto &entry : surfaceParameters()) {
                if (entry.category() == Category::CONTOUR || entry.category() == Category::RELIEF ||
                    paletteLineControl(entry.id)) {
                    continue;
                }
                const auto *p = &entry;
                FormField field{std::string(p->id), int(p->category()), std::wstring(displayLabel(*p)),
                                SurfaceInspectorItems::range(*p),
                                [this, p] { return AttributeFormModel::number(p->value(attribute())); }};
                field.validate = [p](const std::wstring &text) {
                    float value;
                    return AttributeFormModel::parse(text, value) && p->valid(value)
                               ? std::wstring{}
                               : SurfaceInspectorItems::range(*p);
                };
                form.fields.push_back(std::move(field));
            }
            for (const auto &entry : surfaceColors()) {
                if (entry.category == Category::CONTOUR || paletteLineControl(entry.id)) {
                    continue;
                }
                const auto *c = &entry;
                FormField field{std::string(c->id), int(c->category), std::wstring(c->label), L"",
                                [this, c] { return AttributeFormModel::colorText(attribute().*c->member); }};
                field.editor = FormField::Editor::COLOR;
                field.validate = [](const std::wstring &text) {
                    glm::vec4 value;
                    return AttributeFormModel::parseColor(text, value)
                               ? std::wstring{}
                               : L"Enter #RRGGBB or four RGBA values from 0 to 1.";
                };
                form.fields.push_back(std::move(field));
            }
            form.actions.push_back(
                {int(Category::RELIEF), L"Open Lighting & Relief", [this] { selectWorkspace(10, {}, 0); }});
            form.actions.push_back(
                {int(Category::CONTOUR), L"Open Band Line", [this] { selectWorkspace(16, {}, 1); }});
            form.fields.push_back({"surface.studio",
                                   10,
                                   L"Studio",
                                   L"",
                                   [this] { return attribute().studio.use ? L"1" : L"0"; },
                                   {{L"0", L"Off"}, {L"1", L"On"}}});
            form.fields.push_back({"surface.replace",
                                   10,
                                   L"Style Application",
                                   L"Complete replacement removes the original palette color. Legacy "
                                   L"compositing keeps the existing blend settings.",
                                   [this] { return attribute().replaceSurfaceStyle ? L"1" : L"0"; },
                                   {{L"0", L"Legacy Compositing"}, {L"1", L"Complete Replacement"}}});
            FormField style{"surface.style", 10, L"Base Style", L"",
                            [this] { return std::to_wstring(int(attribute().surfaceStyle)); }};
            for (int i = 0; i < 7; ++i) {
                style.choices.push_back({std::to_wstring(i), Selectable::toString(ShdSurfaceStyle(i))});
            }
            form.fields.push_back(std::move(style));
            form.apply = [this](const FormDraft &draft) -> std::wstring {
                if (dragging) {
                    return L"Finish the current slider edit first.";
                }
                const auto before = appearanceTracker ? appearanceTracker->current() : ShaderAttribute{};
                auto candidate = before;
                candidate.slope = attribute();
                const bool styleEdit = draft.contains("surface.style");
                if (styleEdit) {
                    int value;
                    if (!AttributeFormModel::parse(draft.at("surface.style"), value) ||
                        !applySurfaceStyleRecipe(candidate, ShdSurfaceStyle(value))) {
                        return L"Invalid style.";
                    }
                }
                auto next = candidate.slope;
                for (const auto &[id, text] : draft) {
                    if (id == "surface.studio") {
                        next.studio.use = text == L"1";
                        continue;
                    }
                    if (id == "surface.replace") {
                        if (text != L"0" && text != L"1") {
                            return L"Invalid style application.";
                        }
                        next.replaceSurfaceStyle = text == L"1";
                        continue;
                    }
                    if (id == "surface.style") {
                        continue;
                    }
                    const auto p = std::find_if(surfaceParameters().begin(), surfaceParameters().end(),
                                                [&](const auto &p) { return p.id == id; });
                    if (p != surfaceParameters().end()) {
                        float value;
                        if (!AttributeFormModel::parse(text, value) || !p->valid(value)) {
                            return SurfaceInspectorItems::range(*p);
                        }
                        if (p->value(next) != value) {
                            p->value(next) = value;
                            p->activate(next);
                            if (p->category() == Category::MIX) {
                                next.studio.use = true;
                            }
                        }
                        continue;
                    }
                    const auto c = std::find_if(surfaceColors().begin(), surfaceColors().end(),
                                                [&](const auto &c) { return c.id == id; });
                    if (c == surfaceColors().end()) {
                        return L"Unknown setting.";
                    }
                    glm::vec4 value;
                    if (!AttributeFormModel::parseColor(text, value) || !c->valid(value)) {
                        return L"Invalid color.";
                    }
                    if (next.*c->member != value) {
                        next.*c->member = value;
                        activateSurfaceColor(next, &(next.*c->member));
                    }
                }
                if (styleEdit && appearanceHistory) {
                    candidate.slope = next;
                    if (appearanceHistory->apply(std::move(candidate))) {
                        appearanceTracker->record(before, L"Base Style");
                        requestRender();
                    }
                } else if (history.applyState(attribute(), next, L"Surface Effects")) {
                    requestRender();
                }
                return {};
            };
            form.canUndo = [this] { return history.canUndo(); };
            form.canRedo = [this] { return history.canRedo(); };
            form.undo = [this] {
                const bool result = history.undo(attribute());
                if (result) {
                    requestRender();
                }
                return result;
            };
            form.redo = [this] {
                const bool result = history.redo(attribute());
                if (result) {
                    requestRender();
                }
                return result;
            };
            form.clearHistory = [] {};
            form.undoOrder = [this] { return history.undoOrder(); };
            form.redoOrder = [this] { return history.redoOrder(); };
            form.requestHistory = [this](bool redo) { undoRedo(redo); };
            form.canEdit = [this] { return (!operationBusy || !operationBusy()) && !dragging; };
            return form;
        }
        void syncAppearancePanels() {
            for (size_t i = 0; i < appearancePanels.size(); ++i) {
                const auto &slot = panelLayout.slots[i];
                if (!slot.open) {
                    appearancePanels[i].reset();
                    continue;
                }
                if (!appearancePanels[i]) {
                    auto form =
                        slot.module == 1 ? surfacePanelForm() : workspaces.at(slot.module)->sharedForm();
                    appearancePanels[i] = std::make_unique<DockedAppearancePanel>(
                        GetParent(panels[0].window), std::move(form), font, theme, scale, slot.side,
                        [this] {
                            refresh();
                            notifyWorkspace();
                        },
                        [this, i](int action) { PostMessageW(panels[0].window, panelCommand, i, action); },
                        [this](int) { focusHeader(); });
                    if (slot.module == 1) {
                        appearancePanels[i]->form().selectGroup(int(selected));
                    }
                }
                appearancePanels[i]->setSide(slot.side);
            }
        }
        bool changePanelLayout(const AppearancePanelLayout &next) {
            if (operationBusy && operationBusy()) {
                return false;
            }
            if (!numeric->commit()) {
                SetFocus(numeric->handle());
                return false;
            }
            for (size_t i = 0; i < appearancePanels.size(); ++i) {
                if (appearancePanels[i] &&
                    (!next.slots[i].open || next.slots[i].module != panelLayout.slots[i].module)) {
                    if (!appearancePanels[i]->form().applyPending()) {
                        appearancePanels[i]->form().focus();
                        return false;
                    }
                }
            }
            for (size_t i = 0; i < appearancePanels.size(); ++i) {
                if (appearancePanels[i] && next.slots[i].module != panelLayout.slots[i].module) {
                    appearancePanels[i].reset();
                }
            }
            panelLayout = next;
            layerHeight = panelLayout.layerHeight;
            syncAppearancePanels();
            requestLayout();
            savePreferences();
            refresh();
            return true;
        }
        RECT layoutAppearancePanels(RECT area) {
            if (!inspectorOpen) {
                for (auto &panel : appearancePanels) {
                    if (panel) {
                        panel->layout({}, false);
                    }
                }
                return area;
            }
            std::array<std::vector<int>, 3> sides;
            for (int i = 0; i < int(appearancePanels.size()); ++i) {
                if (appearancePanels[i]) {
                    sides[panelLayout.slots[i].side].push_back(i);
                }
            }
            const int sideCount = int(!sides[0].empty()) + int(!sides[1].empty());
            if (area.right - area.left < px(320 + 300 * sideCount)) {
                for (int side = 0; side < 2; ++side) {
                    sides[2].insert(sides[2].end(), sides[side].begin(), sides[side].end());
                    sides[side].clear();
                }
            }
            if (!sides[2].empty()) {
                const int extent =
                    std::min(px(panelLayout.bottomHeight), std::max(1, int(area.bottom - area.top) / 2));
                const int columns = std::max(
                              1, std::min(int(sides[2].size()), int(area.right - area.left) / px(300))),
                          rows = (int(sides[2].size()) + columns - 1) / columns;
                const int top = area.bottom - extent;
                if (extent / rows - px(4) < px(96) && sides[2].size() > 1) {
                    const int shown = std::ranges::find(sides[2], selectedPanel) != sides[2].end()
                                          ? selectedPanel : sides[2].front();
                    for (const int index : sides[2]) {
                        appearancePanels[index]->layout(
                            {area.left, top, area.right - px(4), area.bottom - px(4)}, index == shown);
                    }
                } else {
                    for (size_t i = 0; i < sides[2].size(); ++i) {
                        const int x = int(i) % columns, y = int(i) / columns;
                        appearancePanels[sides[2][i]]->layout(
                            {area.left + (area.right - area.left) * x / columns, top + extent * y / rows,
                             area.left + (area.right - area.left) * (x + 1) / columns - px(4),
                             top + extent * (y + 1) / rows - px(4)},
                            true);
                    }
                }
                area.bottom = std::max<LONG>(area.top + 1, top - px(8));
            }
            const int count = int(!sides[0].empty()) + int(!sides[1].empty());
            const int extent = count ? std::min(px(panelLayout.sideWidth),
                                                std::max(1, int(area.right - area.left - px(160)) / count))
                                     : 0;
            for (int side = 0; side < 2; ++side) {
                if (!sides[side].empty()) {
                    const int left = side == 0 ? area.left : area.right - extent;
                    for (size_t n = 0; n < sides[side].size(); ++n) {
                        appearancePanels[sides[side][n]]->layout(
                            {left, area.top + (area.bottom - area.top) * LONG(n) / LONG(sides[side].size()),
                             left + extent,
                             area.top + (area.bottom - area.top) * LONG(n + 1) / LONG(sides[side].size()) -
                                 px(4)},
                            true);
                    }
                    if (side == 0) {
                        area.left += extent + px(8);
                    } else {
                        area.right -= extent + px(8);
                    }
                }
            }
            area.right = std::max(area.left + 1, area.right);
            area.bottom = std::max(area.top + 1, area.bottom);
            return area;
        }
        void installPanelLayout() {
            WorkspaceForm form;
            form.title = L"Panel Layout";
            form.groups = {L"Main Settings Panel", L"Additional Appearance Panels"};
            const std::vector<FormChoice> sides = {
                {L"0", L"Dock Left"}, {L"1", L"Dock Right"}, {L"2", L"Dock Bottom"}};
            form.fields.push_back({"panel.mainLeft",
                                   0,
                                   L"Main Panel Position",
                                   L"The settings and shader layers move together.",
                                   [this] { return panelLayout.mainLeft ? L"1" : L"0"; },
                                   {{L"0", L"Dock Right"}, {L"1", L"Dock Left"}}});
            form.fields.push_back({"panel.navigationRight",
                                   0,
                                   L"Navigation Panel Position",
                                   L"Place the section list on either side in every workspace.",
                                   [this] { return panelLayout.navigationRight ? L"1" : L"0"; },
                                   {{L"0", L"Dock Left"}, {L"1", L"Dock Right"}}});
            form.fields.push_back({"panel.navigationOuter",
                                   0,
                                   L"Section List Order",
                                   L"When both panels share a side, choose which one sits at the outer edge.",
                                   [this] { return panelLayout.navigationOuter ? L"1" : L"0"; },
                                   {{L"1", L"Section List Outside"}, {L"0", L"Settings Panel Outside"}}});
            const auto number = [&](const char *id, int group, const wchar_t *label, int low, int high,
                                    std::function<std::wstring()> read) {
                FormField field{id, group, label, L"", std::move(read)};
                field.validate = [low, high](const std::wstring &text) {
                    try {
                        size_t end;
                        const auto value = std::stoi(text, &end);
                        if (end == text.size() && value >= low && value <= high) {
                            return std::wstring{};
                        }
                    } catch (...) {
                    }
                    return L"Enter a whole number from " + std::to_wstring(low) + L" to " +
                           std::to_wstring(high) + L".";
                };
                form.fields.push_back(std::move(field));
            };
            number("panel.layerHeight", 0, L"Layer Panel Height", 200, 800,
                   [this] { return std::to_wstring(layerHeight); });
            number("panel.sideWidth", 1, L"Side Panel Width", 300, 640,
                   [this] { return std::to_wstring(panelLayout.sideWidth); });
            number("panel.bottomHeight", 1, L"Bottom Panel Height", 240, 800,
                   [this] { return std::to_wstring(panelLayout.bottomHeight); });
            form.fields.push_back(
                {"panel.selected",
                 1,
                 L"Selected Panel",
                 L"Choose a panel slot, then apply before changing its content. On short windows, this also shows that bottom panel.",
                 [this] { return std::to_wstring(selectedPanel); },
                 {{L"0", L"Panel 1"}, {L"1", L"Panel 2"}, {L"2", L"Panel 3"}, {L"3", L"Panel 4"}}});
            FormField type{"panel.module", 1, L"Panel Content", L"Each panel has its own selected section.",
                           [this] { return std::to_wstring(panelLayout.slots[selectedPanel].module); }};
            type.choices.push_back({L"1", L"Surface Effects"});
            for (int id = 10; id <= 16; ++id) {
                type.choices.push_back({std::to_wstring(id), workspaces.at(id)->title()});
            }
            form.fields.push_back(std::move(type));
            form.fields.push_back({"panel.side", 1, L"Panel Position",
                                   L"Narrow windows temporarily place additional panels below the preview.",
                                   [this] { return std::to_wstring(panelLayout.slots[selectedPanel].side); },
                                   sides});
            for (auto &field : form.fields) {
                field.persisted = false;
            }
            form.apply = [this](const FormDraft &draft) -> std::wstring {
                auto next = panelLayout;
                int selected = selectedPanel;
                const auto read = [&](const char *key, int &value) {
                    if (auto it = draft.find(key); it != draft.end()) {
                        value = std::stoi(it->second);
                    }
                };
                int mainLeft = next.mainLeft;
                read("panel.mainLeft", mainLeft);
                next.mainLeft = mainLeft != 0;
                read("panel.layerHeight", next.layerHeight);
                read("panel.sideWidth", next.sideWidth);
                read("panel.bottomHeight", next.bottomHeight);
                int navigationRight = next.navigationRight;
                read("panel.navigationRight", navigationRight);
                next.navigationRight = navigationRight != 0;
                int navigationOuter = next.navigationOuter;
                read("panel.navigationOuter", navigationOuter);
                next.navigationOuter = navigationOuter != 0;
                read("panel.selected", selected);
                if (selected < 0 || selected >= 4) {
                    return L"Invalid panel.";
                }
                read("panel.module", next.slots[selected].module);
                read("panel.side", next.slots[selected].side);
                if ((next.slots[selected].module != 1 &&
                     (next.slots[selected].module < 10 || next.slots[selected].module > 16)) ||
                    next.slots[selected].side < 0 || next.slots[selected].side > 2) {
                    return L"Invalid panel.";
                }
                if (!changePanelLayout(next)) {
                    return L"Apply or discard the unfinished edit in the panel first.";
                }
                selectedPanel = selected;
                panelLayoutMessage.clear();
                requestLayout();
                return {};
            };
            form.actions = {{1, L"Add Appearance Panel",
                             [this] {
                                 auto next = panelLayout;
                                 int slot = selectedPanel;
                                 if (next.slots[slot].open) {
                                     slot = 0;
                                     while (slot < 4 && next.slots[slot].open) {
                                         ++slot;
                                     }
                                 }
                                 if (slot == 4) {
                                     panelLayoutMessage =
                                         L"All four panels are open. Remove a panel before adding another.";
                                     return;
                                 }
                                 next.slots[slot].open = true;
                                 if (changePanelLayout(next)) {
                                     selectedPanel = slot;
                                     panelLayoutMessage.clear();
                                 }
                             }},
                            {1, L"Remove Selected Panel",
                             [this] {
                                 auto next = panelLayout;
                                 next.slots[selectedPanel].open = false;
                                 if (changePanelLayout(next)) {
                                     panelLayoutMessage.clear();
                                 }
                             }},
                            {0, L"Restore Panel Layout", [this] {
                                 if (changePanelLayout({})) {
                                     selectedPanel = 0;
                                     panelLayoutMessage.clear();
                                 }
                             }}};
            form.canUndo = form.canRedo = form.undo = form.redo = [] { return false; };
            form.clearHistory = [] {};
            form.status = [this] {
                if (!panelLayoutMessage.empty()) {
                    return panelLayoutMessage;
                }
                return std::wstring(
                    panelLayout.slots[selectedPanel].open
                        ? L"Selected panel is open. Removing a panel keeps its appearance settings."
                        : L"Selected panel is closed. Choose its content and position, apply, then add it.");
            };
            installWorkspace(19, std::move(form));
            syncAppearancePanels();
            requestLayout();
        }
        void installLayerPanel(std::shared_ptr<ShaderLayerModel> model) {
            layerHost =
                CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 1, 1,
                                GetParent(panels[2].window), nullptr, GetModuleHandleW(nullptr), nullptr);
            layerPanel = std::make_unique<ShaderLayerWorkspace>(
                WorkspaceContentContext{
                    layerHost, font, scale, theme, [this] { updateHeader(); },
                    [this] { return (!operationBusy || !operationBusy()) && !hasPending(); },
                    [this](bool redo) { undoRedo(redo); },
                    [this](int direction) {
                        if (direction < 0) {
                            focusInspector();
                        } else {
                            focusHeader(-1);
                        }
                    }},
                std::move(model), [this](ShdLayer layer) { openLayerSettings(layer); });
            layerDivider = std::make_unique<PaneSplitter>(
                GetParent(panels[2].window), false, L"Resize layer panel",
                [this] {
                    if ((operationBusy && operationBusy()) || !numeric->commit()) {
                        return false;
                    }
                    layerResizeStart = layerHeight;
                    return true;
                },
                [this](int delta) {
                    layerHeight = std::clamp(layerResizeStart - int(std::round(delta / scale)), 200, 800);
                    requestLayout();
                },
                [this](bool cancel) {
                    if (cancel) {
                        layerHeight = layerResizeStart;
                        requestLayout();
                    } else {
                        savePreferences();
                    }
                },
                [this] {
                    layerHeight = 300;
                    requestLayout();
                    savePreferences();
                },
                [this](int direction) {
                    if (direction < 0) {
                        focusInspector();
                    } else {
                        layerPanel->focus();
                    }
                },
                true);
            requestLayout();
        }
        void setLayoutObserver(std::function<void()> observer) {
            layoutObserver = std::move(observer);
        }
        void openSection(int id, int section) {
            if ((operationBusy && operationBusy()) || !numeric->commit()) {
                return;
            }
            selectWorkspace(id, {}, section);
            if (activeWorkspace == id) {
                if (auto *form = page()) {
                    form->focus();
                }
            }
        }
        bool hasPending() const {
            if (numeric->hasPending()) {
                return true;
            }
            for (const auto &panel : appearancePanels) {
                if (panel && panel->form().hasPending()) {
                    return true;
                }
            }
            return std::any_of(workspaces.begin(), workspaces.end(),
                               [](const auto &entry) { return entry.second->hasPending(); });
        }
        bool hasDocumentPending() const {
            if (numeric->hasPending()) {
                return true;
            }
            for (const auto &panel : appearancePanels) {
                if (panel && panel->form().hasDocumentPending()) {
                    return true;
                }
            }
            return std::any_of(workspaces.begin(), workspaces.end(),
                               [](const auto &entry) { return entry.second->hasDocumentPending(); });
        }
        bool applyAllPending(bool documentOnly = false) {
            if (!numeric->commit()) {
                showInspector();
                SetFocus(numeric->handle());
                return false;
            }
            for (auto &panel : appearancePanels) {
                if (panel && !panel->form().applyPending(documentOnly)) {
                    panel->form().focus();
                    return false;
                }
            }
            for (auto &[id, form] : workspaces) {
                if (!form->applyPending(documentOnly)) {
                    selectWorkspace(id);
                    form->focus();
                    return false;
                }
            }
            return true;
        }
        void discardAllPending() {
            numeric->cancel();
            for (auto &panel : appearancePanels) {
                if (panel) {
                    panel->form().discardPending();
                }
            }
            for (auto &[id, form] : workspaces) {
                form->discardPending();
            }
            refresh();
        }
        SIZE clientSizeForCanvas(int width, int height) const {
            const int inspector =
                inspectorOpen ? px(width < px(320) ? PaneWidths::minimumInspector : paneWidths.inspector) : 0;
            const int rightDivider = inspectorOpen ? px(8) : 0;
            std::array<bool, 3> extra{};
            if (inspectorOpen) {
                for (const auto &slot : panelLayout.slots) {
                    if (slot.open) {
                        extra[slot.side] = true;
                    }
                }
            }
            const int additional = (int(extra[0]) + int(extra[1])) * px(panelLayout.sideWidth + 8);
            const int side = inspector + rightDivider + additional +
                             (navigationDockedOpen && width + inspector + rightDivider + additional +
                                                              px(paneWidths.navigation + 8) >=
                                                          px(1180)
                                  ? px(paneWidths.navigation + 8)
                                  : 0);
            const int total = std::max(1, width) + side;
            return {total, std::max(1, height) + (extra[2] ? px(panelLayout.bottomHeight + 8) : 0) +
                               HeaderLayout::arrange(total, scale).height};
        }
        void repaintPanels() {
            for (const auto &panel : panels) {
                if (IsWindowVisible(panel.window)) {
                    RedrawWindow(panel.window, nullptr, nullptr,
                                 RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
                }
            }
            if (layerHost && IsWindowVisible(layerHost)) {
                RedrawWindow(layerHost, nullptr, nullptr,
                             RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
            }
        }
        RECT layout(int width, int height, int menuInset = 0) {
            layoutSize = {width, height};
            headerLayout = HeaderLayout::arrange(width, scale);
            geometry = WorkspaceGeometry::arrange(width, height, headerLayout.height + menuInset, scale,
                                                  paneWidths, inspectorOpen, navigationOpen,
                                                  navigationDockedOpen, panelLayout.mainLeft,
                                                  panelLayout.navigationRight, panelLayout.navigationOuter);
            const int top = headerLayout.height + menuInset,
                      right = geometry.inspector.right - geometry.inspector.left;
            const int grip = px(32), inspectorTop = top + grip;
            canvasBounds = layoutAppearancePanels(geometry.canvas);
            compact = geometry.compact || !navigationDockedOpen;
            ShowWindow(compactCategory, !page() && compact && query.empty() ? SW_SHOWNA : SW_HIDE);
            navigationHeight = std::max(1, height - top - grip);
            const int dock =
                layerPanel
                    ? std::clamp(px(layerHeight), std::min(px(160), navigationHeight / 2),
                                 std::max(1, navigationHeight - std::min(px(240), navigationHeight / 2)))
                    : 0;
            const int divider = dock ? px(8) : 0;
            inspectorWidth = right;
            inspectorHeight = std::max(1, height - inspectorTop - dock - divider);
            scroll = std::clamp(scroll, 0, maximumScroll());
            SetWindowPos(panels[0].window, nullptr, 0, menuInset, width, headerLayout.height,
                         SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
            SetWindowPos(panels[1].window, geometry.compact && navigationOpen ? HWND_TOP : nullptr,
                         geometry.navigation.left, top + grip,
                         geometry.navigation.right - geometry.navigation.left, navigationHeight,
                         SWP_NOACTIVATE | SWP_NOCOPYBITS |
                             (geometry.compact && navigationOpen ? 0 : SWP_NOZORDER));
            ShowWindow(panels[1].window,
                       geometry.navigation.right > geometry.navigation.left ? SW_SHOWNA : SW_HIDE);
            setNavigationScroll(navigationScroll);
            if (GetFocus() == panels[1].window || GetFocus() == baseStyle) {
                focusNavigation(navigationFocus);
            }
            SetWindowPos(panels[2].window, nullptr, geometry.inspector.left, inspectorTop, right,
                         inspectorHeight, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
            ShowWindow(panels[2].window, inspectorOpen ? SW_SHOWNA : SW_HIDE);
            if (layerPanel) {
                SetWindowPos(layerHost, nullptr, geometry.inspector.left,
                             inspectorTop + inspectorHeight + divider, right, dock,
                             SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
                layerPanel->layout(right, dock);
                ShowWindow(layerHost, inspectorOpen ? SW_SHOWNA : SW_HIDE);
                layerDivider->layout(
                    inspectorOpen ? RECT{geometry.inspector.left, inspectorTop + inspectorHeight,
                                         geometry.inspector.right, inspectorTop + inspectorHeight + divider}
                                  : RECT{},
                    scale);
            }
            for (auto &[id, form] : workspaces) {
                form->layout(right, inspectorHeight, compact);
            }
            if (searchResults) {
                searchResults->layout(right, inspectorHeight);
            }
            splitters[1]->layout(geometry.inspectorDivider, scale);
            splitters[0]->layout(geometry.navigationDivider, scale);
            dockHandles[0]->layout(
                {geometry.navigation.left, top, geometry.navigation.right, top + grip},
                {0, top, width, height}, font, scale, geometry.navigation.right > geometry.navigation.left,
                inspectorOpen ? RECT{geometry.inspector.left, top, geometry.inspector.right, height}
                              : RECT{});
            dockHandles[1]->layout({geometry.inspector.left, top, geometry.inspector.right, top + grip},
                                   {0, top, width, height}, font, scale, inspectorOpen,
                                   RECT{geometry.navigation.left, top, geometry.navigation.right, height});
            const auto place = [&](HWND control, RECT bounds, bool show, bool combo = false) {
                const bool lostFocus = GetFocus() == control && !show;
                SetWindowPos(control, nullptr, bounds.left, bounds.top,
                             std::max(1L, bounds.right - bounds.left),
                             combo ? px(300) : std::max(1L, bounds.bottom - bounds.top),
                             SWP_NOZORDER | SWP_NOACTIVATE);
                ShowWindow(control, show ? SW_SHOWNA : SW_HIDE);
                if (lostFocus) {
                    SetFocus(search);
                }
            };
            for (int i = 0; i < 3; ++i) {
                ShowWindow(headerButtons[i], SW_HIDE);
            }
            for (int i = 0; i < 4; ++i) {
                place(headerButtons[3 + i], headerLayout.tabs[i], !headerLayout.chooser);
            }
            place(headerButtons[7], headerLayout.navigation, true);
            place(headerButtons[8], headerLayout.inspector, true);
            place(workspaceChooser, headerLayout.workspace, headerLayout.chooser, true);
            place(workspaceModule, headerLayout.module, moduleIds.size() > 1, true);
            searchControl->layout(headerLayout.search);
            if (numeric->isEditing() || GetFocus() == panels[2].window) {
                revealSurfaceFocus();
            } else {
                positionSurfaceControls();
            }
            updateHeader();
            notifyWorkspace();
            return canvasBounds;
        }
        void applyTheme() {
            theme = WorkspaceTheme::current();
            for (const auto &handle : dockHandles) {
                if (handle) {
                    handle->themeChanged();
                }
            }
            if (layerPanel) {
                layerPanel->themeChanged();
            }
            for (auto &panel : appearancePanels) {
                if (panel) {
                    panel->applyTheme();
                }
            }
            for (const auto &splitter : splitters) {
                if (splitter) {
                    InvalidateRect(splitter->handle(), nullptr, FALSE);
                }
            }
            if (layerDivider) {
                InvalidateRect(layerDivider->handle(), nullptr, FALSE);
            }
            HBRUSH replacement = CreateSolidBrush(theme.field);
            if (replacement) {
                DeleteObject(editBrush);
                editBrush = replacement;
            }
            searchControl->applyTheme();
            searchResults->applyTheme();
            for (auto &[id, form] : workspaces) {
                form->applyTheme();
            }
            for (HWND control : {baseStyle, compactCategory, workspaceModule, workspaceChooser,
                                 numeric->handle(), removeEffect}) {
                applyDarkThemeClass(control, control != numeric->handle());
                InvalidateRect(control, nullptr, FALSE);
            }
            refresh();
        }
        void applyDpi(UINT dpi) {
            if (!dpi || scale == dpi / 96.f) {
                return;
            }
            for (auto &splitter : splitters) {
                if (splitter) {
                    splitter->cancel();
                }
            }
            for (auto &handle : dockHandles) {
                if (handle) {
                    handle->cancel();
                }
            }
            if (layerDivider) {
                layerDivider->cancel();
            }
            const float newScale = dpi / 96.f;
            const auto replacement =
                CreateFontW(-UiDpi::pixels(13, dpi), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH,
                            UiLanguage::fontFace());
            const auto newHeading =
                CreateFontW(-UiDpi::pixels(16, dpi), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                            DEFAULT_PITCH, UiLanguage::fontFace());
            const auto newBrandFont =
                CreateFontW(-UiDpi::pixels(20, dpi), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                            DEFAULT_PITCH, UiLanguage::fontFace());
            if (!replacement || !newHeading || !newBrandFont) {
                if (replacement) {
                    DeleteObject(replacement);
                }
                if (newHeading) {
                    DeleteObject(newHeading);
                }
                if (newBrandFont) {
                    DeleteObject(newBrandFont);
                }
                return;
            }
            const auto oldFont = font, oldHeading = heading, oldBrandFont = brandFont;
            shortScroll = int(shortScroll * newScale / scale + .5f);
            navigationScroll = int(navigationScroll * newScale / scale + .5f);
            navigationGrab = int(navigationGrab * newScale / scale + .5f);
            scrollGrabOffset = int(scrollGrabOffset * newScale / scale + .5f);
            scale = newScale;
            font = replacement;
            heading = newHeading;
            brandFont = newBrandFont;
            comboContext.font = font;
            comboContext.scale = scale;
            for (HWND control : headerButtons) {
                SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
            }
            for (HWND control : {baseStyle, compactCategory, workspaceModule, workspaceChooser}) {
                WorkspaceComboDrawing::applyMetrics(control, comboContext);
            }
            searchControl->applyMetrics(font, scale);
            searchResults->applyMetrics(font, heading, scale);
            SendMessageW(numeric->handle(), WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
            for (auto &[id, form] : workspaces) {
                form->applyMetrics(font, scale);
            }
            if (layerPanel) {
                layerPanel->metrics(font, scale);
            }
            for (auto &panel : appearancePanels) {
                if (panel) {
                    panel->metrics(font, scale);
                }
            }
            DeleteObject(oldFont);
            DeleteObject(oldHeading);
            DeleteObject(oldBrandFont);
            for (auto &panel : panels) {
                InvalidateRect(panel.window, nullptr, FALSE);
            }
        }
        void invalidateModel() {
            history.clear();
            if (appearanceHistory) {
                appearanceHistory->clear();
            }
            if (appearanceTracker) {
                appearanceTracker->clear();
            }
            refresh();
        }
        void setDocumentDirty(bool dirty) {
            if (configDirty == dirty) {
                return;
            }
            configDirty = dirty;
            InvalidateRect(panels[0].window, nullptr, FALSE);
        }
        void setConfigFile(const std::filesystem::path &, bool loaded) {
            if (loaded) {
                numeric->cancel();
                if (scrollDragging) {
                    scrollDragging = false;
                    ReleaseCapture();
                }
                if (dragging) {
                    dragging = nullptr;
                    ReleaseCapture();
                }
                history.clear();
                focusedRow = 0;
                resetFocused = false;
                inspectorTabFocus = -1;
                scroll = 0;
                shortScroll = 0;
                if (appearanceHistory) {
                    appearanceHistory->clear();
                }
                if (appearanceTracker) {
                    appearanceTracker->clear();
                }
                for (auto &[id, form] : workspaces) {
                    form->loaded();
                }
                for (auto &panel : appearancePanels) {
                    if (panel) {
                        panel->form().discardPending();
                    }
                }
            }
            configDirty = false;
            refresh();
        }
    };
} // namespace merutilm::rff2::workspace
