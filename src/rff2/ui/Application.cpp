//
// Created by Merutilm on 2025-08-08.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21, 2026-08-23, 2026-08-24, 2026-08-27, 2026-08-31, 2026-09-01, 2026-09-02.
// Modified by Opus 5 on 2026-08-10, 2026-08-14, 2026-08-15, 2026-08-26, 2026-08-27, 2026-09-01, 2026-09-02, 2026-09-03, 2026-09-04
//

#include "Application.hpp"

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
    static void paintStatusBarGrip(const HWND statusBar) {
        if ((GetWindowLongW(statusBar, GWL_STYLE) & SBARS_SIZEGRIP) == 0) {
            return;
        }
        RECT rc;
        GetClientRect(statusBar, &rc);
        const int side = GetSystemMetrics(SM_CXVSCROLL);
        RECT grip = {rc.right - side, rc.top, rc.right, rc.bottom};
        if (grip.left <= rc.left) {
            return;
        }
        const HDC hdc = GetDC(statusBar);
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
        ReleaseDC(statusBar, hdc);
    }

    Application::Application() {
        Application::init();
    }

    Application::~Application() {
        Application::destroy();
    }


    void Application::init() {
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
        const int initialWidth = static_cast<int>(
            std::lround(Constants::Win32::INIT_RENDER_SCENE_WIDTH * displayScale));
        const int initialHeight = static_cast<int>(
            std::lround(Constants::Win32::INIT_RENDER_SCENE_HEIGHT * displayScale));
        setClientSize(initialWidth, initialHeight);
        createScene();
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


    void Application::setClientSize(const int width, const int height) const {
        const RECT rect = {0, 0, width, height};
        RECT adjusted = rect;
        AdjustWindowRect(&adjusted, WS_OVERLAPPEDWINDOW | WS_SYSMENU, true);

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
        SetWindowPos(renderWindow, nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);
        // A picture put up by Load Image stands over the canvas, so it takes the rectangle the canvas
        // was just given. Does nothing while no picture is up.
        if (scene != nullptr) {
            const RECT canvas = {0, 0, rect.right - rect.left, rect.bottom - rect.top};
            scene->layoutImageCanvas(canvas);
        }
        SetWindowPos(statusBar, nullptr, 0, rect.bottom - rect.top, rect.right - rect.left, statusHeight, SWP_NOZORDER);

        auto rightEdges = std::array<int, Constants::Status::LENGTH>{};

        const int statusBarWidth = rect.right - rect.left;
        for (int i = 0; i < Constants::Status::LENGTH; i++) {
            rightEdges[i] = (i + 1) * statusBarWidth / Constants::Status::LENGTH;
        }

        SendMessageW(statusBar, SB_SETPARTS, Constants::Status::LENGTH, (LPARAM) rightEdges.data());
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
                SendMessageW(statusBar, SB_SETTEXTW, i, reinterpret_cast<LPARAM>(messages[i].data()));
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
        const bool dark = darkSettingsMode();
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
        const auto font = reinterpret_cast<HFONT>(SendMessageW(statusBar, WM_GETFONT, 0, 0));
        const auto previousFont = SelectObject(draw->hDC,
                                               font != nullptr ? font : GetStockObject(DEFAULT_GUI_FONT));
        SetBkMode(draw->hDC, TRANSPARENT);
        SetBkColor(draw->hDC, settingsTheme().background);
        SetTextColor(draw->hDC, settingsTheme().text);
        DrawTextW(draw->hDC, text.c_str(), -1, &rc,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(draw->hDC, previousFont);
    }

    void Application::createMasterWindow(const HMENU hMenubar) {
        // WS_CLIPCHILDREN: the class paints its background in black, and the canvas and the status
        // bar are children of this window that repaint on their own clock rather than on a WM_PAINT.
        // Without it an erase of this window covers both in black until each puts itself back, which
        // is the canvas blacking out for a frame as the theme is switched.
        masterWindow = CreateWindowExW(
            0,
            Constants::Win32::CLASS_MASTER_WINDOW,
            L"RFF Super",
            WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_CLIPCHILDREN,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT, nullptr, hMenubar, nullptr, nullptr
        );

        if (!masterWindow) {
            vkh::logger::log_err("Failed to create window!\n");
        }
    }

    void Application::createRenderWindow() {
        renderWindow = CreateWindowExW(
            0,
            Constants::Win32::CLASS_VK_RENDER_SCENE,
            L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT, masterWindow, nullptr, nullptr, nullptr);

        if (!renderWindow) {
            vkh::logger::log_err("Failed to create window!\n");
        }
    }

    void Application::createStatusBar() {
        statusBar = CreateWindowExW(
            0,
            STATUSCLASSNAMEW,
            nullptr,
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP | WS_CLIPCHILDREN,
            0, 0, 0, 0,
            masterWindow,
            nullptr,
            nullptr,
            nullptr);

        statusHeight = 0;
        if (statusBar) {
            RECT statusRect;
            GetWindowRect(statusBar, &statusRect);
            statusHeight = statusRect.bottom - statusRect.top;
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
        // The native control keeps the original part geometry while its complete result is transferred in one paint.
        if (message == WM_PAINT && darkSettingsMode()) {
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
            if (bitmap != nullptr) {
                const auto previousBitmap = SelectObject(mem, bitmap);
                FillRect(mem, &client, statusBarBrush());
                DefSubclassProc(window, WM_PRINTCLIENT, reinterpret_cast<WPARAM>(mem),
                                PRF_CLIENT | PRF_ERASEBKGND);
                BitBlt(target, client.left, client.top, width, height, mem, 0, 0, SRCCOPY);
                SelectObject(mem, previousBitmap);
                DeleteObject(bitmap);
            } else {
                DefSubclassProc(window, WM_PRINTCLIENT, reinterpret_cast<WPARAM>(target),
                                PRF_CLIENT | PRF_ERASEBKGND);
            }
            if (mem != nullptr) {
                DeleteDC(mem);
            }
            EndPaint(window, &paint);
            paintStatusBarGrip(window);
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

        window.setListener(
            WM_GETMINMAXINFO, [](vkh::GraphicsContextWindowRef, HWND, WPARAM, const LPARAM lparam) {
                const auto min = reinterpret_cast<LPMINMAXINFO>(lparam);
                min->ptMinTrackSize.x = Constants::Win32::MIN_WINDOW_WIDTH;
                min->ptMinTrackSize.y = Constants::Win32::MIN_WINDOW_HEIGHT;
                return static_cast<LRESULT>(0);
            });
        window.setListener(WM_MOUSEMOVE, [hCursor](vkh::GraphicsContextWindowRef, HWND, WPARAM, LPARAM) {
            SetCursor(hCursor);
            return static_cast<LRESULT>(true);
        });
        window.setListener(WM_SIZING, [this](vkh::GraphicsContextWindowRef, HWND, WPARAM, LPARAM) {
            windowResizing = true;
            return static_cast<LRESULT>(0);
        });
        window.setListener(WM_SIZE, [this](vkh::GraphicsContextWindowRef, HWND, const WPARAM wparam, LPARAM) {
            // Recreating the swapchain under a long job would pull the images it is reading back.
            if (scene->isLongJobBusy()) {
                return static_cast<LRESULT>(0);
            }
            if (wparam == SIZE_MAXIMIZED) {
                resolveWindowResizeEnd();
            }
            if (wparam == SIZE_RESTORED && !windowResizing) {
                resolveWindowResizeEnd();
            }
            return static_cast<LRESULT>(0);
        });

        window.setListener(WM_EXITSIZEMOVE, [this](vkh::GraphicsContextWindowRef, HWND, WPARAM, LPARAM) {
            if (scene->isLongJobBusy()) {
                return static_cast<LRESULT>(0);
            }
            if (windowResizing) {
                windowResizing = false;
                resolveWindowResizeEnd();
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
        window.setListener(WM_ENTERMENULOOP, [](vkh::GraphicsContextWindowRef, const HWND hwnd, WPARAM, LPARAM) {
            SetTimer(hwnd, Constants::Win32::TIMER_MENU_LOOP_RENDER,
                     Constants::Win32::TIMER_MENU_LOOP_RENDER_INTERVAL, nullptr);
            return static_cast<LRESULT>(0);
        });
        window.setListener(WM_EXITMENULOOP, [](vkh::GraphicsContextWindowRef, const HWND hwnd, WPARAM, LPARAM) {
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
                        if (const UINT id = info.wID;
                            settingsMenu->hasCheckbox(id)
                        ) {
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
        window.setListener(WM_COMMAND, [this](vkh::GraphicsContextWindowRef, HWND, const WPARAM wparam, LPARAM) {
            // Menu actions run arbitrary scene work; the pump dispatches this from inside a long job.
            if (scene->isLongJobBusy()) {
                return static_cast<LRESULT>(0);
            }
            const HMENU menu = GetMenu(masterWindow);
            if (const int menuID = LOWORD(wparam);
                settingsMenu->hasCheckbox(menuID)
            ) {
                bool *ref = settingsMenu->getBool(*scene, menuID, true);
                if (ref == nullptr) {
                    throw vkh::exception_invalid_state("checkbox bool cannot be null");
                }
                *ref = !*ref;
                settingsMenu->executeAction(*scene, menuID);
                CheckMenuItem(menu, menuID, *ref ? MF_CHECKED : MF_UNCHECKED);
            } else {
                settingsMenu->executeAction(*scene, menuID);
            }
            return static_cast<LRESULT>(0);
        });
        // Pressing the status bar's map part opens the number entry, so a place in the folder can be
        // jumped to with the mouse to hand.
        SetWindowSubclass(statusBar, statusBarProc, 1, reinterpret_cast<DWORD_PTR>(this));
        // Keystrokes land here rather than on the canvas, which is a child window that never takes
        // focus. Keys the scene leaves alone are passed on, so F10 still reaches the menu.
        window.setListener(WM_KEYDOWN, [this](vkh::GraphicsContextWindowRef, const HWND hwnd, const WPARAM wparam,
                                              const LPARAM lparam) {
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
            if (IOUtilities::isModalDialogOpen()) {
                return static_cast<LRESULT>(0);
            }
            // Destroying the window mid-job would tear the device down under it; stop the job instead.
            if (scene->isLongJobBusy()) {
                scene->requestLongJobCancel();
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
        window.setListener(WM_NCPAINT,
                           [](vkh::GraphicsContextWindowRef, const HWND hwnd, const WPARAM wparam,
                              const LPARAM lparam) {
                               const LRESULT result = DefWindowProcW(hwnd, WM_NCPAINT, wparam, lparam);
                               SettingsMenu::paintMenuBarUnderline(hwnd);
                               return result;
                           });
        window.setListener(WM_NCACTIVATE,
                           [](vkh::GraphicsContextWindowRef, const HWND hwnd, const WPARAM wparam,
                              const LPARAM lparam) {
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
        window.setListener(WM_MEASUREITEM,
                           [](vkh::GraphicsContextWindowRef, const HWND hwnd, const WPARAM wparam,
                              const LPARAM lparam) {
                               if (SettingsMenu::measureMenuItem(
                                   hwnd, reinterpret_cast<MEASUREITEMSTRUCT *>(lparam))) {
                                   return static_cast<LRESULT>(TRUE);
                               }
                               return DefWindowProcW(hwnd, WM_MEASUREITEM, wparam, lparam);
                           });
        window.setListener(WM_DRAWITEM,
                           [this](vkh::GraphicsContextWindowRef, const HWND hwnd, const WPARAM wparam,
                                  const LPARAM lparam) {
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

        // Long synchronous jobs (the tiled export) hold this thread, so they call back here to paint
        // the status bar and drain the queue. Escape is read directly because the job never returns
        // to the loop that would otherwise deliver a keystroke to a listener.
        scene->setLongJobPump([this] {
            refreshStatusBar();
            if (GetForegroundWindow() == masterWindow && (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0) {
                scene->requestLongJobCancel();
            }
            MSG pumped;
            while (PeekMessageW(&pumped, nullptr, 0, 0, PM_REMOVE)) {
                if (pumped.message == WM_QUIT) {
                    scene->requestLongJobCancel();
                    PostQuitMessage(static_cast<int>(pumped.wParam));
                    break;
                }
                TranslateMessage(&pumped);
                DispatchMessageW(&pumped);
            }
        });

        window.appendRenderer([this] {
            resolveWNDRequest();
            const auto &exportation = scene->getAttribute().video.exportation;
            const bool timelineOpen = TimelineWindow::isOpen();
            if (scene->getVideoExportActive() && (exportation.pauseMainPreview || timelineOpen)) {
                // The export owns a window context of its own, so nothing here has to run at all.
            } else if (scene->getVideoGenerationActive() && (exportation.pauseKeyframePreview || timelineOpen)) {
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
                scene->render();
            }
            // Outside the branches on purpose: the status bar is what the paused preview leaves the
            // user to watch (zoom ratio, period, elapsed time), so it keeps updating either way.
            refreshStatusBar();
        });
    }

    void Application::resolveWindowResizeEnd() const {
        RECT rect;
        GetClientRect(masterWindow, &rect);
        rect.bottom -= statusHeight;
        // Both axes: a canvas with either of them at zero cannot be drawn into at all.
        if (rect.bottom - rect.top > 0 && rect.right - rect.left > 0) {
            adjustClient(rect);
            scene->resolveWindowResizeEnd();
            scene->getRequests().requestResize();
            scene->getRequests().requestRecompute();
        }
    }

    void Application::resolveWNDRequest() const {
        if (scene->getWndCWRequest() != 0) {
            setClientSize(scene->getWndCWRequest(), scene->getWndCHRequest());
            scene->wndClientSizeRequestSolved();
        }
        if (scene->isFPSRequested() != 0) {
            engine->getWindowContext(Constants::VulkanWindow::MAIN_WINDOW_ATTACHMENT_INDEX).getWindow().setFramerate(
                scene->getAttribute().render.fps);
            scene->wndFPSRequestSolved();
        }
    }


    // DWMWA_CLOAK: the window counts as visible to everything that draws into it, and shows nothing
    // on screen until it is uncloaked.
    static void setWindowCloaked(const HWND window, const bool cloaked) {
        using DwmSetWindowAttributeFn = HRESULT (WINAPI *)(HWND, DWORD, LPCVOID, DWORD);
        static const auto dwmSetWindowAttribute = reinterpret_cast<DwmSetWindowAttributeFn>(
            GetProcAddress(LoadLibraryW(L"dwmapi.dll"), "DwmSetWindowAttribute"));
        if (dwmSetWindowAttribute == nullptr || window == nullptr) {
            return;
        }
        const BOOL value = cloaked;
        dwmSetWindowAttribute(window, 13, &value, sizeof(value));
    }

    void Application::bindWindowHandlers() const {
        SetWindowLongPtr(masterWindow, GWLP_USERDATA,
                         reinterpret_cast<LONG_PTR>(&wc->getWindow()));
        SetWindowLongPtr(renderWindow, GWLP_USERDATA,
                         reinterpret_cast<LONG_PTR>(scene.get()));
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
        ShowWindow(masterWindow, SW_SHOW);
        UpdateWindow(masterWindow);
        if (awaitPicture) {
            awaitFirstPicture();
        }
        setWindowCloaked(masterWindow, false);
    }

    void Application::start() const {
        wc->getWindow().start();
    }

    void Application::destroy() {
        engine->getCore().getLogicalDevice().waitDeviceIdle();
        scene = nullptr;
        vkh::GeneralPostProcessGraphicsPipelineConfigurator::cleanup();
        engine = nullptr;
        settingsMenu = nullptr;
    }
}
