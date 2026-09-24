//
// Created by Merutilm on 2025-08-08.
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-10, 2026-08-14, 2026-08-15, 2026-08-26, 2026-08-27, 2026-09-01, 2026-09-02, 2026-09-03, 2026-09-04
// Modified by GPT-5 on 2026-08-21, 2026-08-23, 2026-08-24, 2026-08-27, 2026-08-31, 2026-09-01, 2026-09-02
// Modified by GPT-6 on 2026-09-13, 2026-09-14, 2026-09-15, 2026-09-16, 2026-09-17, 2026-09-18, 2026-09-19, 2026-09-20, 2026-09-22, 2026-09-23, 2026-09-24, 2026-09-25
//

#include "NativeDialogs.hpp"
#include "UnsavedChangesDialog.hpp"
#include "PipelinePreparationWindow.hpp"
#include "Application.hpp"
#include "CallbackShader.hpp"
#include "CallbackFractal.hpp"
#include "CallbackVideo.hpp"
#include "CallbackFile.hpp"
#include "CallbackExplore.hpp"
#include "workspace/ExploreModel.hpp"
#include "workspace/AnimationModel.hpp"
#include "workspace/AppearanceForms.hpp"
#include "ShaderLayerWindow.hpp"
#include "workspace/ShaderLayerWorkspace.hpp"
#include "workspace/PreviewGeometry.hpp"

#include <chrono>
#include <cmath>

#include "../../vulkan_helper/configurator/GeneralPostProcessGraphicsPipelineConfigurator.hpp"
#include "../vulkan/SharedDescriptorTemplate.hpp"
#include "IOUtilities.h"
#include "RecoveryPrompt.hpp"
#include "TimelineWindow.hpp"
#include "../io/RecoveryIO.h"
#include "../io/PreferencesIO.h"

namespace merutilm::rff2 {
    // Fill behind the status bar's owner-drawn parts, rebuilt when the theme's panel color changes.
    static HBRUSH statusBarBrush() {
        static HBRUSH brush = nullptr;
        static COLORREF color = 0;
        if (brush == nullptr || color != settingsTheme().background) {
            if (brush != nullptr) {
                DeleteObject(brush);
            }
            color = settingsTheme().background;
            brush = CreateSolidBrush(color);
        }
        return brush;
    }

    // The status bar's resize grip, redrawn in the dark theme. Off the visual styles the control
    // falls back to the classic one, which is a white wedge in the corner of a dark bar.
    static void paintStatusBarGrip(const HWND statusBar, const HDC hdc, const UINT dpi) {
        if ((GetWindowLongW(statusBar, GWL_STYLE) & SBARS_SIZEGRIP) == 0) {
            return;
        }
        RECT rc;
        GetClientRect(statusBar, &rc);
        const int side = UiDpi::metric(SM_CXVSCROLL, dpi);
        RECT grip = {rc.right - side, rc.top, rc.right, rc.bottom};
        if (grip.left <= rc.left) {
            return;
        }
        FillRect(hdc, &grip, statusBarBrush());
        const HPEN pen = CreatePen(PS_SOLID, 1, settingsTheme().buttonBorder);
        const auto previousPen = SelectObject(hdc, pen);
        // Three short rules stepping out of the corner, the shape the native grip draws.
        for (int i = 1; i <= 3; ++i) {
            const int offset = i * side / 4;
            MoveToEx(hdc, rc.right - offset, rc.bottom - 2, nullptr);
            LineTo(hdc, rc.right - 2, rc.bottom - offset);
        }
        SelectObject(hdc, previousPen);
        DeleteObject(pen);
    }

    Application::Application() {
        Application::init();
    }

    Application::~Application() {
        Application::destroy();
    }

    void Application::init() {
        PipelinePreparationWindow::install();
        initWindow();
    }

    void Application::initWindow() {
        SetProcessDPIAware();
        const HMENU hMenubar = initMenu();
        createMasterWindow(hMenubar);
        createRenderWindow();
        createStatusBar();
        settingsMenu->attachMasterWindow(masterWindow);
        const double displayScale = Constants::Win32::initialWindowScale();
        const int initialWidth =
            static_cast<int>(std::lround(Constants::Win32::INIT_RENDER_SCENE_WIDTH * displayScale));
        const int initialHeight =
            static_cast<int>(std::lround(Constants::Win32::INIT_RENDER_SCENE_HEIGHT * displayScale));
        setClientSize(initialWidth, initialHeight);
        createScene();
        if (CustomMenu::requested()) {
            customMenu = std::make_unique<CustomMenu>(
                masterWindow, [this] { return settingsMenu->menuModel(*scene); },
                [this](UINT command) { settingsMenu->dispatchCommand(*scene, command); });
            SetMenu(masterWindow, nullptr);
        }
        workspaceShell = std::make_unique<workspace::WorkspaceShell>(
            masterWindow, [this]() -> ShdSlopeAttribute & { return scene->getAttribute().shader.slope; },
            [this] { scene->getRequests().requestShader(); },
            [this](int action) {
                if (action == 0) {
                    CallbackFractal::REFERENCE(*settingsMenu, *scene);
                }
                if (action == 2) {
                    openTimelineEditor();
                }
                if (action == 3) {
                    CallbackVideo::EXPORT_SETTINGS(*settingsMenu, *scene);
                }
                if (action == 4) {
                    CallbackShader::PALETTE(*settingsMenu, *scene);
                }
                if (action == 5) {
                    CallbackFile::saveCurrentConfig(*scene);
                }
            },
            Utilities::getDefaultPath() / L"workspace-favorites.txt");
        workspaceShell->bindPreviewFocus([this] { return timelineWorkspace; });
        workspaceShell->bindAppearanceReset(
            [this]() -> ShaderAttribute & { return scene->getAttribute().shader; });
        auto explore = std::make_shared<workspace::ExploreModel>(
            [this]() -> FractalAttribute & { return scene->getAttribute().fractal; },
            [this] {
                scene->setComputeHold(false);
                scene->applyLoadedConfig();
                scene->getRequests().requestShader();
                scene->getRequests().requestResize();
                scene->getRequests().requestRecompute();
            });
        auto exploreForm = explore->form();
        exploreForm.actions = {
            {6, L"Recompute", [this] { CallbackExplore::RECOMPUTE(*settingsMenu, *scene); }},
            {6, L"Cancel Render", [this] { CallbackExplore::CANCEL_RENDER(*settingsMenu, *scene); }},
            {6, L"Reset View", [explore] { explore->resetView(); }},
            {6, L"Find Center", [this] { CallbackExplore::FIND_CENTER(*settingsMenu, *scene); }},
            {6, L"Locate Minibrot", [this] { CallbackExplore::LOCATE_MINIBROT(*settingsMenu, *scene); }}};
        workspaceShell->installWorkspace(0, std::move(exploreForm));
        auto animation = workspace::animationModel([this]() -> Attribute & { return scene->getAttribute(); },
                                                   [this] {
                                                       scene->getRequests().requestShader();
                                                       if (IsWindow(timelineWorkspace)) {
                                                           TimelineWindow::syncWorkspace(timelineWorkspace);
                                                       }
                                                   });
        auto animationForm =
            animation->form(L"Animation", {L"Color Animation", L"Frozen Colors", L"Zoom Motion", L"Timeline",
                                           L"Keyframes", L"Video Camera"});
        workspace::addAnimationGuidance(animationForm);
        auto frozenField =
            std::find_if(animationForm.fields.begin(), animationForm.fields.end(), [](const auto &field) {
                return field.id == "animation.frozenIterations";
            })->read;
        animationForm.actions = {
            {0, L"Pause Preview", [this] { scene->setPreviewAnimationPaused(true); }, true},
            {0, L"Play Preview", [this] { scene->setPreviewAnimationPaused(false); }, true},
            {1, L"Pick Color to Freeze",
             [this, animation, frozenField] {
                 const auto before = frozenField();
                 scene->beginColorFreezePick(masterWindow, [animation, before] {
                     animation->recordExternal({{"animation.frozenIterations", before}});
                 });
             }},
            {1, L"Clear Frozen Colors",
             [animation] { animation->apply({{"animation.frozenIterations", L""}}); }},
            {3, L"Open Timeline Editor", [this] { openTimelineEditor(); }},
            {3, L"Play / Pause Timeline",
             [this] { TimelineWindow::workspaceAction(openTimelineEditor(), 0); }, true},
            {3, L"Stop Timeline", [this] { TimelineWindow::workspaceAction(timelineWorkspace, 1); }, true},
            {3, L"Load Keyframe Folder",
             [this] { TimelineWindow::workspaceAction(openTimelineEditor(), 2); }},
            {4, L"Generate Keyframes",
             [this] { CallbackVideo::GENERATE_VID_KEYFRAME(*settingsMenu, *scene); }}};
        animationForm.status = [this]() -> std::wstring {
            if (IsWindowVisible(timelineWorkspace)) {
                return TimelineWindow::workspaceStatus(timelineWorkspace);
            }
            if (scene->isColorFreezePickActive()) {
                return L"Click the fractal to freeze a color.";
            }
            if (scene->isPreviewAnimationPaused()) {
                return L"Preview paused. Video export uses saved speeds.";
            }
            return L"Preview playing. Settings are shared with video export.";
        };
        const auto animationUndoOrder = animationForm.undoOrder, animationRedoOrder = animationForm.redoOrder;
        animationForm.undoOrder = [this, base = animationUndoOrder] {
            return std::max(base(), TimelineWindow::workspaceHistoryOrder(timelineWorkspace, false));
        };
        animationForm.redoOrder = [this, base = animationRedoOrder] {
            const auto animationOrder = base();
            const auto timelineOrder = TimelineWindow::workspaceHistoryOrder(timelineWorkspace, true);
            if (animationOrder == 0) {
                return timelineOrder;
            }
            if (timelineOrder == 0) {
                return animationOrder;
            }
            return std::min(animationOrder, timelineOrder);
        };
        animationForm.canUndo = [order = animationForm.undoOrder] { return order() != 0; };
        animationForm.canRedo = [order = animationForm.redoOrder] { return order() != 0; };
        animationForm.undo = [this, base = animationForm.undo, order = animationUndoOrder] {
            if (TimelineWindow::workspaceHistoryOrder(timelineWorkspace, false) > order()) {
                return TimelineWindow::workspaceHistory(timelineWorkspace, false, true);
            }
            return base();
        };
        animationForm.redo = [this, base = animationForm.redo, order = animationRedoOrder] {
            const auto animationOrder = order();
            const auto timelineOrder = TimelineWindow::workspaceHistoryOrder(timelineWorkspace, true);
            if (timelineOrder != 0 && (animationOrder == 0 || timelineOrder < animationOrder)) {
                return TimelineWindow::workspaceHistory(timelineWorkspace, true, true);
            }
            return base();
        };
        workspaceShell->installWorkspace(2, std::move(animationForm));
        exportWorkspace = std::make_shared<workspace::ExportWorkspace>(*scene, masterWindow);
        auto exportForm = exportWorkspace->form();
        exportForm.actions.push_back(
            {2, L"Rendering FPS & Performance", [this] { workspaceShell->openSection(20, 0); }});
        workspaceShell->installWorkspace(3, std::move(exportForm));
        const workspace::AttributeGetter appearanceAttribute = [this]() -> Attribute & {
            return scene->getAttribute();
        };
        const std::function<void()> appearanceChanged = [this] { scene->getRequests().requestShader(); };
        workspaceShell->installWorkspace(10, workspace::lightingForm(appearanceAttribute, appearanceChanged));
        workspaceShell->installWorkspace(11, workspace::textureForm(appearanceAttribute, appearanceChanged,
            [this](int from, int to) { scene->swapAnimationLayers(AnimatedLayerFamily::TEXTURE, from, to); }));
        workspaceShell->installWorkspace(12, workspace::patternForm(appearanceAttribute, appearanceChanged,
            [this](int from, int to) { scene->swapAnimationLayers(AnimatedLayerFamily::PATTERN, from, to); }));
        workspaceShell->installWorkspace(13,
                                         workspace::warpStripeForm(appearanceAttribute, appearanceChanged));
        workspaceShell->installWorkspace(14,
                                         workspace::finishingForm(appearanceAttribute, appearanceChanged));
        workspaceShell->installWorkspace(
            15, workspace::materialEffectsForm(appearanceAttribute, appearanceChanged,
                [this](int from, int to) { scene->swapAnimationLayers(AnimatedLayerFamily::EFFECT, from, to); }));
        workspaceShell->installWorkspace(16, workspace::paletteForm(appearanceAttribute, appearanceChanged));
        comparisonWorkspace = std::make_shared<workspace::ComparisonWorkspace>(
            *scene, masterWindow, renderWindow, workspaceShell->appearanceEdits());
        workspaceShell->installWorkspace(17, comparisonWorkspace->form());
        auto shaderLayers = std::make_shared<workspace::ShaderLayerModel>(
            [this]() -> ShaderAttribute & { return scene->getAttribute().shader; }, appearanceChanged,
            [tracker = workspaceShell->appearanceEdits()](ShaderAttribute before) {
                tracker->record(std::move(before), L"Layer Order");
            });
        auto layerForm = shaderLayers->form([] {});
        workspaceShell->installWorkspace(18, std::move(layerForm));
        workspaceShell->installLayerPanel(shaderLayers);
        workspaceShell->installPanelLayout();
        auto previewState = std::make_shared<RenderAttribute>(scene->getAttribute().render);
        auto performance = workspace::previewPerformanceForm(appearanceAttribute, [this, previewState] {
            const auto &value = scene->getAttribute().render;
            if (value.fps != previewState->fps) {
                scene->wndRequestFPS();
            }
            if (value.boundaryTraceFill != previewState->boundaryTraceFill ||
                value.preview2Color != previewState->preview2Color ||
                value.coarsePreview != previewState->coarsePreview) {
                scene->getRequests().requestRecompute();
            }
            *previewState = value;
        });
        performance.clearHistory = [this, previewState, clear = performance.clearHistory] {
            clear();
            *previewState = scene->getAttribute().render;
        };
        workspaceShell->installWorkspace(20, std::move(performance), 0);
        workspaceShell->setOperationGuard(
            [this] { return exportWorkspace->ownsActiveJob() || scene->isLongJobBusy(); });
        workspaceShell->setLayoutObserver([this] { layoutPending = true; });
        settingsMenu->workspaceNavigation = [this](int id, int section) {
            if (id == 2 && section == 3) {
                openTimelineEditor();
            } else {
                workspaceShell->openSection(id, section);
            }
        };
        workspaceShell->observeWorkspace([this](int id, int group, const RECT &) {
            if (id != 2 || group != 1) {
                scene->cancelColorFreezePick();
            }
        });
        scene->setConfigFileObserver(
            [this](const std::filesystem::path &path, bool loaded, uint16_t width, uint16_t height) {
                if (loaded && IsWindow(timelineWorkspace)) {
                    DestroyWindow(timelineWorkspace);
                    timelineWorkspace = nullptr;
                }
                document.accept(scene->getAttribute(), width, height);
                documentTitle = path.filename().wstring();
                documentCheckTime = 0;
                if (workspaceShell) {
                    workspaceShell->setConfigFile(path, loaded);
                }
            });
        scene->setConfigGuards(
            [this] {
                if (!workspaceShell->applyAllPending(true)) {
                    return false;
                }
                resolveWindowRequests();
                return true;
            },
            [this] { return confirmDocumentReplacement(); }, [this] { workspaceShell->discardAllPending(); });
        setClientSize(initialWidth, initialHeight);
        document.accept(scene->getAttribute(), scene->getClientWidth(), scene->getClientHeight());
        scene->getRequests().requestResize();
        bindWindowHandlers();
        setProcedure();
        // After setProcedure: an owner-drawn menu is measured and drawn through listeners that are
        // only registered there, so dressing the window any earlier would ask for both too soon.
        applyMainWindowTheme();
        // Recovery takes the compute hold before any startup frame can begin the default view.
        const bool recoveryPending = offerRecovery();
        // Shown last, once everything it wears is settled. Recoloring the frame changes it, and a
        // frame change on a window already on screen resizes the client under the canvas: a
        // swapchain rebuilt and a compute restarted right where the first picture was arriving,
        // which is the blink seen at startup.
        prepareWindow(!recoveryPending);
    }

    bool Application::offerRecovery() const {
        // Taken before this run marks itself live, so a leftover of a run that held the same process
        // id cannot be mistaken for this one's.
        const std::optional<RecoveredSnapshot> kept = RecoveryIO::takeSnapshot();
        RecoveryIO::beginSession();
        if (kept.has_value()) {
            RecoveryPrompt::offer(*settingsMenu, *scene, kept->path, kept->reason);
        }
        return kept.has_value();
    }

    void Application::endRecoverySession() const {
        // Called where start() has returned and nothing has been torn down yet, so the scene can
        // still say whether the view it was asked for ever arrived. destroy() cancels the compute,
        // which would leave every shutdown looking like a finished one.
        RecoveryIO::endSession(scene != nullptr && scene->isComputeUnfinished());
    }

    HMENU Application::initMenu() {
        const HMENU hMenubar = CreateMenu();
        settingsMenu = std::make_unique<SettingsMenu>(hMenubar);
        return hMenubar;
    }

    void Application::setClientSize(const int canvasWidth, const int canvasHeight) const {
        if (scene) {
            scene->configureCanvasSize(canvasWidth, canvasHeight);
        }
        const SIZE size = workspaceShell ? workspaceShell->clientSizeForCanvas(canvasWidth, canvasHeight)
                                         : SIZE{canvasWidth, canvasHeight};
        const int width = size.cx, height = size.cy + (customMenu ? customMenu->height() : 0);
        const RECT rect = {0, 0, width, height};
        RECT adjusted = rect;
        AdjustWindowRect(&adjusted, WS_OVERLAPPEDWINDOW | WS_SYSMENU, customMenu ? FALSE : TRUE);

        const int windowWidth = adjusted.right - adjusted.left;
        const int windowHeight = adjusted.bottom - adjusted.top + statusHeight;
        // Align to the left edge of the screen (leaving the right side free for the settings /
        // palette windows) instead of centering horizontally; stay vertically centered.
        const int invisibleBorder = GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
        const int x = -invisibleBorder;
        const int y = (GetSystemMetrics(SM_CYSCREEN) - windowHeight) / 2;

        SetWindowPos(masterWindow, nullptr, x, y, windowWidth, windowHeight, SWP_NOZORDER);

        // AdjustWindowRect answers for a menu bar of one row, so a bar that wraps to two leaves the client short and the resize-end path sizes the canvas - and the swapchain with it - from that shorter rect while the call below sets the requested one.
        RECT client;
        GetClientRect(masterWindow, &client);
        const int shortfallWidth = width - (client.right - client.left);
        const int shortfallHeight = height - (client.bottom - client.top - statusHeight);
        if (shortfallWidth != 0 || shortfallHeight != 0) {
            const int correctedHeight = windowHeight + shortfallHeight;
            SetWindowPos(masterWindow, nullptr, x, (GetSystemMetrics(SM_CYSCREEN) - correctedHeight) / 2,
                         windowWidth + shortfallWidth, correctedHeight, SWP_NOZORDER);
        }
        adjustClient(rect);
    }

    void Application::adjustClient(const RECT &rect) const {
        const int menuHeight = customMenu ? customMenu->height() : 0;
        if (customMenu) {
            customMenu->layout(rect.right - rect.left);
        }
        RECT available =
            workspaceShell
                ? workspaceShell->layout(rect.right - rect.left, rect.bottom - rect.top + statusHeight,
                                         menuHeight)
                : RECT{0, menuHeight, rect.right - rect.left, rect.bottom - rect.top + statusHeight};
        const int barHeight = std::min(statusHeight, int(std::max(0L, available.bottom - available.top - 1)));
        const RECT statusBounds{available.left, available.bottom - barHeight, available.right,
                                available.bottom};
        available.bottom = statusBounds.top;
        const RECT canvas =
            scene ? workspace::PreviewGeometry::fit(available, scene->documentCanvasSize()) : available;
        SetWindowPos(renderWindow, nullptr, canvas.left, canvas.top, canvas.right - canvas.left,
                     canvas.bottom - canvas.top, SWP_NOZORDER);
        // A picture put up by Load Image stands over the canvas, so it takes the rectangle the canvas
        // was just given. Does nothing while no picture is up.
        if (scene != nullptr) {
            scene->layoutImageCanvas(canvas);
        }
        SetWindowPos(statusBar, nullptr, statusBounds.left, statusBounds.top,
                     statusBounds.right - statusBounds.left, barHeight, SWP_NOZORDER | SWP_NOACTIVATE);

        auto rightEdges = std::array<int, Constants::Status::LENGTH>{};

        const int statusBarWidth = statusBounds.right - statusBounds.left;
        for (int i = 0; i < Constants::Status::LENGTH; i++) {
            rightEdges[i] = (i + 1) * statusBarWidth / Constants::Status::LENGTH;
        }

        SendMessageW(statusBar, SB_SETPARTS, Constants::Status::LENGTH, (LPARAM)rightEdges.data());
        InvalidateRect(masterWindow, nullptr, TRUE);
        if (workspaceShell) {
            workspaceShell->repaintPanels();
        }
    }

    void Application::refreshStatusBar() const {
        std::array<std::wstring, Constants::Status::LENGTH> messages;
        {
            std::scoped_lock lock(statusMessagesMutex);
            messages = statusMessages;
        }
        const bool dark = darkSettingsMode();
        const bool forced = statusBarPartsStale;
        statusBarPartsStale = false;
        for (int i = 0; i < Constants::Status::LENGTH; ++i) {
            if (!dark) {
                // The control holds the string itself and skips a part whose text has not moved.
                SendMessageW(statusBar, SB_SETTEXTW, i | SBT_NOBORDERS,
                             reinterpret_cast<LPARAM>(UiLanguage::text(messages[i]).c_str()));
                continue;
            }
            // An owner-drawn part is handed its index and nothing else, so the control sees the same
            // value every time and never repaints on its own: this runs once per frame, and asking
            // for a repaint unconditionally would redraw the whole bar at the render rate.
            if (!forced && messages[i] == statusBarShown[i]) {
                continue;
            }
            statusBarShown[i] = messages[i];
            SendMessageW(statusBar, SB_SETTEXTW, i | SBT_OWNERDRAW | SBT_NOBORDERS, i);
            if (RECT part; SendMessageW(statusBar, SB_GETRECT, i, reinterpret_cast<LPARAM>(&part))) {
                InvalidateRect(statusBar, &part, FALSE);
            }
        }
    }

    void Application::applyMainWindowTheme() const {
        if (customMenu) {
            customMenu->refresh();
        }
        if (workspaceShell) {
            workspaceShell->applyTheme();
        }
        TimelineWindow::applyWorkspaceTheme(timelineWorkspace);
        if (comparisonWorkspace) {
            comparisonWorkspace->applyTheme();
        }
        const bool dark = darkSettingsMode();
        VideoWindow::applyWorkspaceTheme(dark);
        if (scene) {
        }
        applyDarkWindowFrame(masterWindow);
        if (settingsMenu != nullptr) {
            settingsMenu->applyMenuTheme();
        }
        if (statusBar != nullptr) {
            // SB_SETBKCOLOR is ignored while the control is themed, so the dark bar comes off the
            // visual styles and has its parts drawn here instead.
            if (dark) {
                disableThemeClass(statusBar);
            } else {
                applyDarkThemeClass(statusBar, false);
            }
            SendMessageW(statusBar, SB_SETBKCOLOR, 0,
                         dark ? static_cast<LPARAM>(settingsTheme().background) : CLR_DEFAULT);
            // The part flags themselves changed, so every part has to be re-sent whatever it says.
            statusBarPartsStale = true;
            refreshStatusBar();
            InvalidateRect(statusBar, nullptr, TRUE);
        }
        RedrawWindow(masterWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE);
    }

    void Application::drawStatusBarPart(const DRAWITEMSTRUCT *draw) const {
        const auto index = static_cast<int>(draw->itemData);
        if (index < 0 || index >= Constants::Status::LENGTH) {
            return;
        }
        std::wstring text;
        {
            std::scoped_lock lock(statusMessagesMutex);
            text = statusMessages[index];
        }
        RECT rc = draw->rcItem;
        FillRect(draw->hDC, &rc, statusBarBrush());
        rc.left += UiDpi::pixels(8, uiDpi);
        rc.right -= UiDpi::pixels(6, uiDpi);
        const auto font = reinterpret_cast<HFONT>(SendMessageW(statusBar, WM_GETFONT, 0, 0));
        const auto previousFont =
            SelectObject(draw->hDC, font != nullptr ? font : GetStockObject(DEFAULT_GUI_FONT));
        SetBkMode(draw->hDC, TRANSPARENT);
        SetBkColor(draw->hDC, settingsTheme().background);
        SetTextColor(draw->hDC, settingsTheme().text);
        try {
            UiLanguage::drawText(draw->hDC, text.c_str(), -1, &rc,
                                 (index == Constants::Status::RENDER_STATUS ? DT_RIGHT : DT_LEFT) | DT_VCENTER |
                                     DT_SINGLELINE | DT_END_ELLIPSIS);
        } catch (...) {
            SelectObject(draw->hDC, previousFont);
            throw;
        }
        SelectObject(draw->hDC, previousFont);
    }

    void Application::paintStatusBar(const HDC target) const {
        RECT client;
        GetClientRect(statusBar, &client);
        FillRect(target, &client, statusBarBrush());
        for (int i = 0; i < Constants::Status::LENGTH; ++i) {
            DRAWITEMSTRUCT part{};
            part.hDC = target;
            part.itemData = i;
            if (SendMessageW(statusBar, SB_GETRECT, i, reinterpret_cast<LPARAM>(&part.rcItem))) {
                drawStatusBarPart(&part);
            }
        }
        const HPEN pen = CreatePen(PS_SOLID, 1, settingsTheme().sectionFrame);
        const auto previousPen = SelectObject(target, pen);
        MoveToEx(target, client.left, client.top, nullptr);
        LineTo(target, client.right, client.top);
        for (int i = 0; i + 1 < Constants::Status::LENGTH; ++i) {
            RECT part;
            if (SendMessageW(statusBar, SB_GETRECT, i, reinterpret_cast<LPARAM>(&part))) {
                MoveToEx(target, part.right, client.top + UiDpi::pixels(5, uiDpi), nullptr);
                LineTo(target, part.right, client.bottom - UiDpi::pixels(5, uiDpi));
            }
        }
        SelectObject(target, previousPen);
        DeleteObject(pen);
        paintStatusBarGrip(statusBar, target, uiDpi);
    }

    void Application::createMasterWindow(const HMENU hMenubar) {
        const UiDpi::AwarenessScope awareness(true);
        // WS_CLIPCHILDREN: the class paints its background in black, and the canvas and the status
        // bar are children of this window that repaint on their own clock rather than on a WM_PAINT.
        // Without it an erase of this window covers both in black until each puts itself back, which
        // is the canvas blacking out for a frame as the theme is switched.
        masterWindow =
            CreateWindowExW(0, Constants::Win32::CLASS_MASTER_WINDOW, L"RFF Super",
                            WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT, nullptr, hMenubar, nullptr, nullptr);

        if (!masterWindow) {
            vkh::logger::log_err("Failed to create window!\n");
        }
        uiDpi = UiDpi::forWindow(masterWindow);
    }

    void Application::createRenderWindow() {
        renderWindow = CreateWindowExW(0, Constants::Win32::CLASS_VK_RENDER_SCENE, L"",
                                       WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, CW_USEDEFAULT, CW_USEDEFAULT,
                                       CW_USEDEFAULT, CW_USEDEFAULT, masterWindow, nullptr, nullptr, nullptr);

        if (!renderWindow) {
            vkh::logger::log_err("Failed to create window!\n");
        }
    }

    void Application::createStatusBar() {
        statusBar = CreateWindowExW(0, STATUSCLASSNAMEW, nullptr,
                                    WS_CHILD | WS_VISIBLE | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_NODIVIDER |
                                        WS_CLIPCHILDREN,
                                    0, 0, 0, 0, masterWindow, nullptr, nullptr, nullptr);

        statusHeight = 0;
        updateStatusMetrics();
    }

    void Application::updateStatusMetrics() {
        if (statusBar) {
            const auto replacement = CreateFontW(
                -UiDpi::pixels(13, uiDpi), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            if (replacement) {
                SendMessageW(statusBar, WM_SETFONT, reinterpret_cast<WPARAM>(replacement), FALSE);
                if (statusFont) {
                    DeleteObject(statusFont);
                }
                statusFont = replacement;
            }
            SendMessageW(statusBar, SB_SETMINHEIGHT, UiDpi::pixels(24, uiDpi), 0);
            statusHeight = UiDpi::pixels(26, uiDpi);
        }
    }

    void Application::createScene() {
        auto core = vkh::factory::create<vkh::Core>();
        engine = vkh::factory::create<vkh::Engine>(std::move(core));
        wc = engine->attachWindowContext(renderWindow, Constants::VulkanWindow::MAIN_WINDOW_ATTACHMENT_INDEX);
        scene = std::make_unique<RenderScene>(*engine, *wc, &statusMessages, &statusMessagesMutex);
    }

    LRESULT Application::statusBarProc(const HWND window, const UINT message, const WPARAM wParam,
                                       const LPARAM lParam, const UINT_PTR id, const DWORD_PTR data) {
        if (message == WM_NCDESTROY) {
            RemoveWindowSubclass(window, statusBarProc, id);
        }
        const auto *application = reinterpret_cast<Application *>(data);
        if (message == WM_ERASEBKGND) {
            return 1;
        }
        if (application && (message == WM_PRINTCLIENT || message == WM_PRINT)) {
            application->paintStatusBar(reinterpret_cast<HDC>(wParam));
            return 0;
        }
        // The native control keeps the original part geometry while its complete result is transferred in one paint.
        if (message == WM_PAINT && application) {
            PAINTSTRUCT paint;
            const HDC target = BeginPaint(window, &paint);
            RECT client;
            GetClientRect(window, &client);
            const int width = client.right - client.left;
            const int height = client.bottom - client.top;
            const HDC mem = CreateCompatibleDC(target);
            const HBITMAP bitmap = mem != nullptr && width > 0 && height > 0
                                       ? CreateCompatibleBitmap(target, width, height)
                                       : nullptr;
            HGDIOBJ previousBitmap = nullptr;
            if (bitmap != nullptr) {
                previousBitmap = SelectObject(mem, bitmap);
            }
            const bool bitmapSelected = previousBitmap != nullptr && previousBitmap != HGDI_ERROR;
            try {
                if (bitmapSelected) {
                    application->paintStatusBar(mem);
                    BitBlt(target, client.left, client.top, width, height, mem, 0, 0, SRCCOPY);
                } else {
                    application->paintStatusBar(target);
                }
            } catch (...) {
                if (bitmapSelected) {
                    SelectObject(mem, previousBitmap);
                }
                if (bitmap != nullptr) {
                    DeleteObject(bitmap);
                }
                if (mem != nullptr) {
                    DeleteDC(mem);
                }
                EndPaint(window, &paint);
                throw;
            }
            if (bitmapSelected) {
                SelectObject(mem, previousBitmap);
            }
            if (bitmap != nullptr) {
                DeleteObject(bitmap);
            }
            if (mem != nullptr) {
                DeleteDC(mem);
            }
            EndPaint(window, &paint);
            return 0;
        }
        // Read from the press rather than from the click the status bar reports: a dialog closing
        // over this window sends the release of the button that dismissed it down here, and that
        // release alone would otherwise open the entry on its own.
        if (const auto *app = reinterpret_cast<Application *>(data);
            message == WM_LBUTTONDOWN && app != nullptr && app->scene != nullptr &&
            !app->scene->isLongJobBusy()) {
            RECT part;
            const POINT pressed = {static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam))};
            if (SendMessageW(window, SB_GETRECT, Constants::Status::RENDER_STATUS,
                             reinterpret_cast<LPARAM>(&part)) &&
                PtInRect(&part, pressed) && app->scene->armBrowsedMapTyping()) {
                // The digits land on the master window, so the press must not leave focus elsewhere.
                SetFocus(app->masterWindow);
                return 0;
            }
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }

    void Application::setProcedure() {
        const HCURSOR hCursor = LoadCursor(nullptr, IDC_ARROW);

        vkh::GraphicsContextWindowRef window = wc->getWindow();

        // Aspect-fit margins use the editor theme while WS_CLIPCHILDREN protects the Vulkan canvas.
        const auto paintBackground = [](vkh::GraphicsContextWindowRef, HWND hwnd, WPARAM target, LPARAM) {
            RECT client;
            GetClientRect(hwnd, &client);
            FillRect(reinterpret_cast<HDC>(target), &client, statusBarBrush());
            return static_cast<LRESULT>(1);
        };
        window.setListener(WM_ERASEBKGND, paintBackground);
        window.setListener(WM_PRINTCLIENT, paintBackground);

        window.setListener(
            WM_GETMINMAXINFO, [this](vkh::GraphicsContextWindowRef, HWND, WPARAM, const LPARAM lparam) {
                const auto min = reinterpret_cast<LPMINMAXINFO>(lparam);
                min->ptMinTrackSize.x = UiDpi::pixels(Constants::Win32::MIN_WINDOW_WIDTH, uiDpi);
                min->ptMinTrackSize.y = UiDpi::pixels(Constants::Win32::MIN_WINDOW_HEIGHT, uiDpi);
                return static_cast<LRESULT>(0);
            });
        window.setListener(WM_DPICHANGED,
                           [this](vkh::GraphicsContextWindowRef, HWND, WPARAM dpi, LPARAM position) {
                               if (!HIWORD(dpi) || !position) {
                                   return LRESULT(0);
                               }
                               uiDpi = HIWORD(dpi);
                               settingsMenu->applyDpi(uiDpi);
                               if (customMenu) {
                                   customMenu->refresh();
                               }
                               workspaceShell->applyDpi(uiDpi);
                               comparisonWorkspace->applyDpi(uiDpi);
                               TimelineWindow::applyWorkspaceDpi(timelineWorkspace, uiDpi);
                               updateStatusMetrics();
                               const auto &suggested = *reinterpret_cast<const RECT *>(position);
                               SetWindowPos(masterWindow, nullptr, suggested.left, suggested.top,
                                            suggested.right - suggested.left,
                                            suggested.bottom - suggested.top, SWP_NOZORDER | SWP_NOACTIVATE);
                               layoutPending = true;
                               InvalidateRect(masterWindow, nullptr, TRUE);
                               return LRESULT(0);
                           });
        window.setListener(WM_MOUSEMOVE, [hCursor](vkh::GraphicsContextWindowRef, HWND, WPARAM, LPARAM) {
            SetCursor(hCursor);
            return static_cast<LRESULT>(true);
        });
        window.setListener(WM_SIZING, [this](vkh::GraphicsContextWindowRef, HWND, WPARAM, LPARAM) {
            windowResizing = true;
            layoutPending = true;
            return static_cast<LRESULT>(TRUE);
        });
        window.setListener(WM_SIZE, [this](vkh::GraphicsContextWindowRef, HWND, const WPARAM wparam, LPARAM) {
            // Recreating the swapchain under a long job would pull the images it is reading back.
            if (wparam != SIZE_MINIMIZED) {
                layoutPending = true;
            }
            return static_cast<LRESULT>(0);
        });

        window.setListener(WM_EXITSIZEMOVE, [this](vkh::GraphicsContextWindowRef, HWND, WPARAM, LPARAM) {
            if (windowResizing) {
                windowResizing = false;
                layoutPending = true;
            }
            return static_cast<LRESULT>(0);
        });
        // The system's popup fade samples the canvas the popup is about to cover, and what it
        // finds there is the frame before the last present: one stale frame flashes through the
        // popup as it opens. The animation is held off while a menu is up and put back on the way
        // out. Both fire for the bar's own loop and for any popup tracked over this window.
        // The menu runs a modal loop of its own, so the main loop stops for as long as one is open
        // and the canvas presents nothing at all. Everything the compositor has of the client area
        // while the popup goes up is then whatever it kept from the last present, which is the
        // stale picture that shows through the popup for its first frame. A timer is the one thing
        // that still reaches this window from inside the menu's loop, so the canvas is driven off
        // one for the length of it and the compositor always has the frame it is drawing over.
        window.setListener(WM_ENTERMENULOOP,
                           [](vkh::GraphicsContextWindowRef, const HWND hwnd, WPARAM, LPARAM) {
                               SetTimer(hwnd, Constants::Win32::TIMER_MENU_LOOP_RENDER,
                                        Constants::Win32::TIMER_MENU_LOOP_RENDER_INTERVAL, nullptr);
                               return static_cast<LRESULT>(0);
                           });
        window.setListener(WM_EXITMENULOOP,
                           [](vkh::GraphicsContextWindowRef, const HWND hwnd, WPARAM, LPARAM) {
                               KillTimer(hwnd, Constants::Win32::TIMER_MENU_LOOP_RENDER);
                               return static_cast<LRESULT>(0);
                           });
        // The menu runs a modal loop of its own, so the main loop stops for as long as one is open
        // and the canvas presents nothing at all. Driving it from a timer - the one thing that still
        // reaches this window from inside that loop - halves how long the swap between two popups
        // leaves the canvas uncovered, from two frames to one.
        window.setListener(WM_TIMER, [this](vkh::GraphicsContextWindowRef menuWindow, const HWND hwnd,
                                            const WPARAM wparam, const LPARAM lparam) {
            if (wparam != Constants::Win32::TIMER_MENU_LOOP_RENDER) {
                return DefWindowProcW(hwnd, WM_TIMER, wparam, lparam);
            }
            // Same guard the menu's own commands carry: a long job holds this thread and owns the
            // images a present would read.
            if (scene->isLongJobBusy()) {
                return static_cast<LRESULT>(0);
            }
            menuWindow.renderOnce();
            return static_cast<LRESULT>(0);
        });
        window.setListener(
            WM_INITMENUPOPUP, [this](vkh::GraphicsContextWindowRef, HWND, const WPARAM wparam, LPARAM) {
                const auto popup = reinterpret_cast<HMENU>(wparam);
                const int count = GetMenuItemCount(popup);
                for (int i = 0; i < count; ++i) {
                    //synchronize current attr
                    MENUITEMINFO info = {};
                    info.cbSize = sizeof(MENUITEMINFO);
                    info.fMask = MIIM_ID;
                    if (GetMenuItemInfo(popup, i, TRUE, &info)) {
                        if (const UINT id = info.wID; settingsMenu->hasCheckbox(id)) {
                            const bool *ref = settingsMenu->getBool(*scene, id, false);
                            if (ref == nullptr) {
                                throw vkh::exception_invalid_state("checkbox bool cannot be null");
                            }
                            CheckMenuItem(popup, id, MF_BYCOMMAND | (*ref ? MF_CHECKED : MF_UNCHECKED));
                        }
                    }
                }
                return static_cast<LRESULT>(0);
            });
        window.setListener(WM_COMMAND,
                           [this](vkh::GraphicsContextWindowRef, HWND, const WPARAM wparam, LPARAM) {
                               // Menu actions run arbitrary scene work; the pump dispatches this from inside a long job.
                               if (scene->isLongJobBusy()) {
                                   return static_cast<LRESULT>(0);
                               }
                               settingsMenu->dispatchCommand(*scene, LOWORD(wparam));
                               return static_cast<LRESULT>(0);
                           });
        // Pressing the status bar's map part opens the number entry, so a place in the folder can be
        // jumped to with the mouse to hand.
        SetWindowSubclass(statusBar, statusBarProc, 1, reinterpret_cast<DWORD_PTR>(this));
        // Keystrokes land here rather than on the canvas, which is a child window that never takes
        // focus. Keys the scene leaves alone are passed on, so F10 still reaches the menu.
        window.setMessageFilter([this](const MSG &message) {
            if (!IOUtilities::isModalDialogOpen() && !documentPromptOpen &&
                SettingsWindow::filterSearchMessage(message)) {
                return true;
            }
            if (customMenu && !IOUtilities::isModalDialogOpen() && !documentPromptOpen &&
                customMenu->filter(message)) {
                return true;
            }
            if (message.message != WM_KEYDOWN || !IsWindowEnabled(masterWindow) ||
                IOUtilities::isModalDialogOpen() || documentPromptOpen) {
                return false;
            }
            if (message.hwnd != masterWindow && !IsChild(masterWindow, message.hwnd)) {
                return false;
            }
            if ((GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000) &&
                message.wParam == 'S') {
                CallbackFile::saveCurrentConfig(*scene);
                return true;
            }
            return workspaceShell && workspaceShell->handleShortcut(message);
        });
        window.setListener(WM_SETTINGCHANGE, [this](vkh::GraphicsContextWindowRef, HWND hwnd,
                                                    WPARAM parameter, LPARAM section) {
            if (parameter == 0 || parameter == SPI_SETNONCLIENTMETRICS) {
                settingsMenu->applyDpi(uiDpi, true);
            }
            if (customMenu && (parameter == 0 || parameter == SPI_SETNONCLIENTMETRICS)) {
                customMenu->refresh();
                layoutPending = true;
            }
            const bool changed = refreshSystemSettingsTheme();
            if (changed || parameter == 0 || parameter == SPI_SETHIGHCONTRAST) {
                applyMainWindowTheme();
            }
            return DefWindowProcW(hwnd, WM_SETTINGCHANGE, parameter, section);
        });
        window.setListener(WM_SYSCOLORCHANGE,
                           [this](vkh::GraphicsContextWindowRef, HWND hwnd, WPARAM w, LPARAM l) {
                               refreshSystemSettingsTheme();
                               applyMainWindowTheme();
                               return DefWindowProcW(hwnd, WM_SYSCOLORCHANGE, w, l);
                           });
        window.setListener(WM_THEMECHANGED,
                           [this](vkh::GraphicsContextWindowRef, HWND hwnd, WPARAM w, LPARAM l) {
                               if (refreshSystemSettingsTheme()) {
                                   applyMainWindowTheme();
                               }
                               return DefWindowProcW(hwnd, WM_THEMECHANGED, w, l);
                           });
        window.setListener(WM_KEYDOWN, [this](vkh::GraphicsContextWindowRef, const HWND hwnd,
                                              const WPARAM wparam, const LPARAM lparam) {
            if ((GetKeyState(VK_CONTROL) & 0x8000) && wparam == 'S') {
                CallbackFile::saveCurrentConfig(*scene);
                return static_cast<LRESULT>(0);
            }
            if (scene->runKeyAction(wparam)) {
                return static_cast<LRESULT>(0);
            }
            return DefWindowProcW(hwnd, WM_KEYDOWN, wparam, lparam);
        });
        window.setListener(WM_CLOSE, [this](vkh::GraphicsContextWindowRef, HWND, WPARAM, LPARAM) {
            // A file dialog is up. It owns this window, and the callback that opened it is still on
            // the stack waiting for a path, holding the scene this would destroy: closing here left
            // that callback reporting a failure against a torn-down scene and the process never
            // reaching its exit. The dialog's own Cancel is the way out.
            if (IOUtilities::isModalDialogOpen() || documentPromptOpen) {
                return static_cast<LRESULT>(0);
            }
            // Destroying the window mid-job would tear the device down under it; stop the job instead.
            if (scene->isLongJobBusy()) {
                scene->requestLongJobCancel();
                return static_cast<LRESULT>(0);
            }
            if (exportWorkspace && exportWorkspace->ownsActiveJob()) {
                exportWorkspace->cancel();
                return static_cast<LRESULT>(0);
            }
            if (!confirmDocumentReplacement()) {
                return static_cast<LRESULT>(0);
            }
            DestroyWindow(masterWindow);
            return static_cast<LRESULT>(0);
        });
        window.setListener(WM_DESTROY, [](vkh::GraphicsContextWindowRef, HWND, WPARAM, LPARAM) {
            PostQuitMessage(0);
            return static_cast<LRESULT>(0);
        });
        // The frame draws a light rule under the menu bar that no menu color reaches. Both of these
        // repaint the non-client area, so the cover goes back on right after the frame is drawn.
        window.setListener(WM_NCPAINT, [](vkh::GraphicsContextWindowRef, const HWND hwnd, const WPARAM wparam,
                                          const LPARAM lparam) {
            const LRESULT result = DefWindowProcW(hwnd, WM_NCPAINT, wparam, lparam);
            SettingsMenu::paintMenuBarUnderline(hwnd);
            return result;
        });
        window.setListener(WM_NCACTIVATE, [](vkh::GraphicsContextWindowRef, const HWND hwnd,
                                             const WPARAM wparam, const LPARAM lparam) {
            const LRESULT result = DefWindowProcW(hwnd, WM_NCACTIVATE, wparam, lparam);
            SettingsMenu::paintMenuBarUnderline(hwnd);
            return result;
        });
        // Posted by the View menu's Dark Mode item, once the flag it toggles has actually flipped.
        window.setListener(Constants::Win32::WM_MAIN_THEME_CHANGED,
                           [this](vkh::GraphicsContextWindowRef, HWND, WPARAM, LPARAM) {
                               applyMainWindowTheme();
                               // Posted after the flag has flipped, which is where what to keep is known.
                               PreferencesIO::save();
                               return static_cast<LRESULT>(0);
                           });
        // The menu bar and the status bar are owner-drawn while the dark theme is on; both report
        // back here. Anything neither of them owns is left to the default handling.
        window.setListener(WM_MEASUREITEM, [](vkh::GraphicsContextWindowRef, const HWND hwnd,
                                              const WPARAM wparam, const LPARAM lparam) {
            if (SettingsMenu::measureMenuItem(hwnd, reinterpret_cast<MEASUREITEMSTRUCT *>(lparam))) {
                return static_cast<LRESULT>(TRUE);
            }
            return DefWindowProcW(hwnd, WM_MEASUREITEM, wparam, lparam);
        });
        window.setListener(WM_DRAWITEM, [this](vkh::GraphicsContextWindowRef, const HWND hwnd,
                                               const WPARAM wparam, const LPARAM lparam) {
            const auto draw = reinterpret_cast<const DRAWITEMSTRUCT *>(lparam);
            if (SettingsMenu::drawMenuItem(draw)) {
                return static_cast<LRESULT>(TRUE);
            }
            if (draw != nullptr && draw->hwndItem == statusBar) {
                drawStatusBarPart(draw);
                return static_cast<LRESULT>(TRUE);
            }
            return DefWindowProcW(hwnd, WM_DRAWITEM, wparam, lparam);
        });

        window.appendRenderer([this] {
            if (windowResizing) {
                return;
            }
            if (layoutPending && !scene->isLongJobBusy()) {
                layoutPending = false;
                resolveWindowResizeEnd();
            }
            resolveWindowRequests();
            const auto &exportation = scene->getAttribute().video.exportation;
            const bool timelineOpen = TimelineWindow::isOpen();
            if (scene->getVideoExportActive() && (exportation.pauseMainPreview || timelineOpen)) {
                // The export owns a window context of its own, so nothing here has to run at all.
            } else if (scene->getVideoGenerationActive() &&
                       (exportation.pauseKeyframePreview || timelineOpen)) {
                // Generation is served by this very call - its recompute / image requests are
                // resolved inside render() - so only the drawing is held back.
                scene->render(false);
            } else if (timelineOpen) {
                // The editor renders previews in its own context, so the main preview holds its last
                // frame - except when a settings panel has changed the shader. The editor opens those
                // panels itself, and a value set in one of them would otherwise reach nothing on
                // screen until the editor was closed. Only that pass runs; nothing else is resumed.
                if (scene->getRequests().shaderRequested.load()) {
                    scene->render();
                }
            } else if (scene->isImageBrowsing()) {
                // A picture covers the canvas whole, margins and all, so there is nothing of the
                // fractal to show while it is up. Presenting under it puts the swapchain and the
                // window holding the picture on the same pixels every frame, which is the one-frame
                // blackout seen while the arrow keys walk a folder. The requests are still resolved
                // here - only the drawing is held - and the frame already presented stays on the
                // canvas underneath, so taking the picture away brings it straight back.
                scene->render(false);
            } else {
                scene->render(!comparisonWorkspace->active() || !scene->isIdleCompute() ||
                              scene->getRequests().recomputeRequested);
            }
            comparisonWorkspace->update(timelineOpen || scene->isImageBrowsing() ||
                                        scene->getVideoExportActive() || scene->getVideoGenerationActive());
            // Outside the branches on purpose: the status bar is what the paused preview leaves the
            // user to watch (zoom ratio, period, elapsed time), so it keeps updating either way.
            refreshStatusBar();
            refreshDocumentState();
        });
    }

    void Application::refreshDocumentState(const bool force) {
        const ULONGLONG now = GetTickCount64();
        if (!force && now - documentCheckTime < 500) {
            return;
        }
        documentCheckTime = now;
        const auto dimensions = scene->documentCanvasSize();
        const bool dirty = document.isDirty(scene->getAttribute(), dimensions.cx, dimensions.cy) ||
                           workspaceShell->hasDocumentPending();
        workspaceShell->setDocumentDirty(dirty);
        const std::wstring title = L"RFF_Super — " +
                                   (documentTitle.empty() ? UiLanguage::text(L"Untitled") : documentTitle) +
                                   (dirty ? L" *" : L"");
        const int length = GetWindowTextLengthW(masterWindow);
        std::wstring current(length + 1, L'\0');
        GetWindowTextW(masterWindow, current.data(), length + 1);
        current.resize(length);
        if (title != current) {
            SetWindowTextW(masterWindow, title.c_str());
        }
    }

    bool Application::confirmDocumentReplacement() {
        if (documentPromptOpen) {
            return false;
        }
        const auto dimensions = scene->documentCanvasSize();
        if (!workspaceShell->hasDocumentPending() &&
            !document.isDirty(scene->getAttribute(), dimensions.cx, dimensions.cy)) {
            return true;
        }
        documentPromptOpen = true;
        const int choice = UnsavedChangesDialog::show(masterWindow);
        const bool accepted = choice == IDNO || (choice == IDYES && CallbackFile::saveCurrentConfig(*scene));
        documentPromptOpen = false;
        return accepted;
    }

    void Application::resolveWindowResizeEnd() const {
        RECT rect;
        GetClientRect(masterWindow, &rect);
        rect.bottom -= statusHeight;
        // Both axes: a canvas with either of them at zero cannot be drawn into at all.
        if (rect.bottom - rect.top > 0 && rect.right - rect.left > 0) {
            adjustClient(rect);
            scene->recoverStaleSwapchain();
        }
    }

    void Application::resolveWindowRequests() const {
        if (scene->getWndCWRequest() != 0) {
            scene->configureCanvasSize(scene->getWndCWRequest(), scene->getWndCHRequest());
            scene->wndClientSizeRequestSolved();
            RECT client;
            GetClientRect(masterWindow, &client);
            client.bottom -= statusHeight;
            adjustClient(client);
            scene->recoverStaleSwapchain();
        }
        if (scene->isFPSRequested() != 0) {
            engine->getWindowContext(Constants::VulkanWindow::MAIN_WINDOW_ATTACHMENT_INDEX)
                .getWindow()
                .setFramerate(scene->getAttribute().render.fps);
            scene->wndFPSRequestSolved();
        }
    }

    // DWMWA_CLOAK: the window counts as visible to everything that draws into it, and shows nothing
    // on screen until it is uncloaked.
    static void setWindowCloaked(const HWND window, const bool cloaked) {
        using DwmSetWindowAttributeFn = HRESULT(WINAPI *)(HWND, DWORD, LPCVOID, DWORD);
        static const auto dwmSetWindowAttribute = reinterpret_cast<DwmSetWindowAttributeFn>(
            GetProcAddress(LoadLibraryW(L"dwmapi.dll"), "DwmSetWindowAttribute"));
        if (dwmSetWindowAttribute == nullptr || window == nullptr) {
            return;
        }
        const BOOL value = cloaked;
        dwmSetWindowAttribute(window, 13, &value, sizeof(value));
    }

    void Application::bindWindowHandlers() const {
        SetWindowLongPtr(masterWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&wc->getWindow()));
        SetWindowLongPtr(renderWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(scene.get()));
    }

    void Application::awaitFirstPicture() const {
        // The first view is only computed once the window counts as visible, so this is where it is
        // waited for. Bounded: past this the window opens on whatever it has, which is the black
        // canvas a first compute too slow to wait for would have left anyway.
        constexpr auto LIMIT = std::chrono::milliseconds(500);
        const auto started = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - started < LIMIT) {
            wc->getWindow().renderOnce();
            if (scene->isIdleCompute() && !scene->getRequests().recomputeRequested.load()) {
                // The map is complete; this is the frame that puts it on screen.
                wc->getWindow().renderOnce();
                break;
            }
        }
    }

    void Application::prepareWindow(const bool awaitPicture) const {
        // Shown cloaked, drawn, then revealed. A window is not rendered into until it is visible, so
        // uncloaking is the only point at which both are true: without it the canvas is on screen as
        // black for the frame it takes the first picture to arrive, which is the blink at startup.
        setWindowCloaked(masterWindow, true);
        ShowWindow(masterWindow, SW_SHOWMAXIMIZED);
        UpdateWindow(masterWindow);
        if (awaitPicture) {
            awaitFirstPicture();
        }
        setWindowCloaked(masterWindow, false);
    }

    HWND Application::openTimelineEditor() {
        if (scene->isLongJobBusy() || exportWorkspace->ownsActiveJob()) {
            return nullptr;
        }
        if (!IsWindow(timelineWorkspace)) {
            timelineWorkspace = TimelineWindow::createWorkspace(*settingsMenu, *scene, masterWindow, true);
            if (!timelineWorkspace) {
                return nullptr;
            }
            TimelineWindow::bindWorkspaceHistory(
                timelineWorkspace, workspaceShell->historyDomain(),
                [this](bool redo) { workspaceShell->navigateHistory(redo); });
            TimelineWindow::applyWorkspaceTheme(timelineWorkspace);
        }
        TimelineWindow::showWorkspace(timelineWorkspace, {}, true);
        return timelineWorkspace;
    }

    void Application::start() const {
        wc->getWindow().start();
    }

    void Application::destroy() {
        if (exportWorkspace) {
            exportWorkspace->cancel();
        }
        if (scene) {
            scene->getBackgroundThreads().requestStopAll();
        }
        engine->getCore().getLogicalDevice().waitDeviceIdle();
        if (IsWindow(timelineWorkspace)) {
            DestroyWindow(timelineWorkspace);
            timelineWorkspace = nullptr;
        }
        customMenu.reset();
        workspaceShell.reset();
        if (statusFont) {
            if (IsWindow(statusBar)) {
                SendMessageW(statusBar, WM_SETFONT,
                             reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), FALSE);
            }
            DeleteObject(statusFont);
            statusFont = nullptr;
        }
        exportWorkspace.reset();
        comparisonWorkspace.reset();
        scene = nullptr;
        vkh::GeneralPostProcessGraphicsPipelineConfigurator::cleanup();
        engine = nullptr;
        settingsMenu = nullptr;
    }
} // namespace merutilm::rff2
