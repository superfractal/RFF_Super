//
// Created by Merutilm on 2025-08-08.
// Modified by Opus 5 on 2026-08-14, 2026-08-15, 2026-08-27, 2026-09-01
// Modified by GPT-5 on 2026-08-23, 2026-09-02
// Modified by GPT-6 on 2026-09-13, 2026-09-14, 2026-09-15, 2026-09-17, 2026-09-23
//

#pragma once
#include <array>
#include <memory>
#include <mutex>
#include <string>

#include "RenderScene.hpp"
#include "workspace/WorkspaceShell.hpp"
#include "workspace/ConfigDocument.hpp"
#include "workspace/ExportWorkspace.hpp"
#include "workspace/ComparisonWorkspace.hpp"
#include "SettingsMenu.hpp"
#include "CustomMenu.hpp"
#include "../../vulkan_helper/impl/Engine.hpp"

namespace merutilm::rff2 {

    class Application final : public vkh::Handler {
        int statusHeight = 0;
        UINT uiDpi = 96;
        HFONT statusFont = nullptr;
        std::array<std::wstring, Constants::Status::LENGTH> statusMessages = {};
        mutable std::mutex statusMessagesMutex;
        // What each status part was last handed. An owner-drawn part carries no text the control
        // could compare against, so this is what says whether a repaint is owed - see refreshStatusBar.
        mutable std::array<std::wstring, Constants::Status::LENGTH> statusBarShown = {};
        mutable bool statusBarPartsStale = true;
        HWND masterWindow = nullptr;
        HWND renderWindow = nullptr;
        HWND statusBar = nullptr;
        vkh::WindowContextPtr wc = nullptr;
        std::unique_ptr<RenderScene> scene = nullptr;
        std::unique_ptr<SettingsMenu> settingsMenu = nullptr;
        std::unique_ptr<CustomMenu> customMenu;
        vkh::Engine engine = nullptr;
        bool windowResizing = false;
        bool layoutPending = false;
        std::unique_ptr<workspace::WorkspaceShell> workspaceShell;
        workspace::ConfigDocument document;
        ULONGLONG documentCheckTime = 0;
        std::wstring documentTitle;
        HWND timelineWorkspace = nullptr;
        std::shared_ptr<workspace::ExportWorkspace> exportWorkspace;
        std::shared_ptr<workspace::ComparisonWorkspace> comparisonWorkspace;
        bool documentPromptOpen = false;

        void refreshDocumentState(bool force = false);
        bool confirmDocumentReplacement();

    public:
        explicit Application();

        ~Application() override;

        Application(const Application &) = delete;

        Application(Application &&) = delete;

        Application &operator=(const Application &) = delete;

        Application &operator=(Application &&) = delete;

        vkh::EngineRef getEngine() const {
            return *engine;
        }

        void start() const;

        // Ends the recovery session this run holds. Called once start() has returned, while the
        // scene is still whole: whether a compute was still running is what decides if the settings
        // are kept for the next start. See RecoveryIO.
        void endRecoverySession() const;

    private:
        // Watches the status bar for a press on its map part, which opens the jump entry.
        static LRESULT CALLBACK statusBarProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                                              UINT_PTR id, DWORD_PTR data);

        void init() override;

        void initWindow();

        HMENU initMenu();

        void setClientSize(int width, int height) const;

        void adjustClient(const RECT &rect) const;

        void refreshStatusBar() const;

        // Redresses everything of the main window that is not the canvas: the DWM frame, the menu
        // bar and the status bar. Runs at startup and again whenever the dark-mode flag is flipped.
        void applyMainWindowTheme() const;

        // One owner-drawn status bar part, in the dark theme's colors.
        void drawStatusBarPart(const DRAWITEMSTRUCT *draw) const;
        void paintStatusBar(HDC target) const;

        void createStatusBar();
        void updateStatusMetrics();

        void createScene();
        HWND openTimelineEditor();

        // Opens the recovery panel when the last run ended without shutting down, marks this run live, and reports whether generation must stay held.
        bool offerRecovery() const;

        void setProcedure();

        void resolveWindowResizeEnd() const;

        void resolveWindowRequests() const;

        // Points both windows at what handles their messages. Done before the window is dressed and
        // shown, so every message either of those raises already reaches its listener.
        void bindWindowHandlers() const;

        // Runs the render loop by hand until the first view is on the canvas, or long enough that
        // waiting further would be worse than opening without it.
        void awaitFirstPicture() const;

        void prepareWindow(bool awaitPicture) const;

        void createMasterWindow(HMENU hMenubar);

        void createRenderWindow();

        void destroy() override;
    };
}
