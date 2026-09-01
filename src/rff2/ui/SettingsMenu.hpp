//
// Created by Merutilm on 2025-05-14.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21, 2026-08-26, 2026-08-27.
// Modified by Opus 5 on 2026-08-14, 2026-08-27, 2026-08-31
//

#pragma once
#include <deque>
#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include <windef.h>

#include "RenderScene.hpp"
#include "SettingsWindow.hpp"
#include "../preset/shader/example/ShdExamplePresets.h"

namespace merutilm::rff2 {
    struct SettingsMenu {
        // What an owner-drawn menu item needs to draw itself. Windows keeps no string for an
        // MFT_OWNERDRAW item, so the caption is kept here and handed to the item as its item data.
        // The container is a deque because those pointers must outlive every later insertion.
        struct MenuEntry {
            HMENU parent;
            UINT position;
            std::string text;
            bool submenu;
            bool topLevel;
            // Pushed to the right end of the bar (the ? menu). Kept here rather than read back off
            // the item: the display type has to be rewritten whole to switch owner-draw on and off,
            // and this is the one flag beside it that must survive being rewritten.
            bool rightJustified = false;
            // The size the system itself gave this bar item, read off the live bar before the item
            // was ever made owner-drawn. Guessing the padding back put the dark bar visibly wider
            // than the light one; this is the width it actually had. Zero until captured.
            int nativeWidth = 0;
            int nativeHeight = 0;
        };

        HMENU menubar;
        int count = 0;
        std::deque<MenuEntry> menuEntries = {};
        HWND masterWindow = nullptr;
        bool menuNativeSizesCaptured = false;
        bool menuOwnerDrawn = false;
        std::vector<HMENU> childMenus = {};
        std::vector<std::function<void(SettingsMenu &, RenderScene &)> > callbacks = {};
        std::vector<bool> hasCheckboxes = {};
        std::vector<std::optional<std::function<bool*(RenderScene &, bool)> > > checkboxActions = {};
        // What part of the attribute a panel is bound to - all the closers below need to tell them
        // apart by. A shader preset replaces the whole shader, a KFR color file only its palette,
        // and a settings file or a video export reaches everything.
        enum class SettingsWindowKind : uint8_t {
            OTHER,
            SHADER,
            SHADER_PALETTE,
        };

        struct ActiveSettingsWindow {
            std::unique_ptr<SettingsWindow> window;
            SettingsWindowKind kind;
        };
        std::vector<ActiveSettingsWindow> activeSettingsWindows = {};

        explicit SettingsMenu(HMENU hMenubar);

        ~SettingsMenu();

        bool hasCheckbox(int menuID);

        SettingsMenu(const SettingsMenu &) = delete;

        SettingsMenu &operator=(const SettingsMenu &) = delete;

        SettingsMenu(SettingsMenu &&) = delete;

        SettingsMenu &operator=(SettingsMenu &&) = delete;

        void configure();

        HMENU addChildMenu(HMENU target, std::string_view child);

        HMENU addChildItem(HMENU target, std::string_view child,
                           const std::function<void(SettingsMenu &, RenderScene &)> &callback);

        HMENU addChildCheckbox(HMENU target, std::string_view child,
                               const std::function<bool*(RenderScene &, bool)>
                               &checkboxAction);

        template<typename P> requires std::is_base_of_v<Preset, P>
        HMENU addPresetExecutor(HMENU target, P preset);

        // A preset that replaces the whole shader, which takes the shader panels down with it and
        // is answered for its texture layers the way a loaded file is.
        HMENU addFullShaderPreset(HMENU target, const ShdExamplePresets::FromFile &preset);

        HMENU add(HMENU target, std::string_view child,
                  const std::function<void(SettingsMenu &, RenderScene &)> &callback, bool
                  hasChild, bool hasCheckbox, const std::optional<std::function<bool *(RenderScene &, bool)>> &checkboxAction);

        void executeAction(RenderScene &scene, int menuID);

        // The window the bar belongs to. Needed to have the bar redrawn when the theme flips.
        void attachMasterWindow(HWND window);

        // Switches the bar and every popup between the system's own drawing and the dark one below.
        void applyMenuTheme();

        // Records what the system sized the bar items to, while they are still its own to draw.
        void captureNativeMenuSizes();

        // Owner-draw handlers for the menu, called from the master window's WM_MEASUREITEM /
        // WM_DRAWITEM. Both answer false for an item that is not a menu item of this bar.
        static bool measureMenuItem(HWND owner, MEASUREITEMSTRUCT *measure);

        static bool drawMenuItem(const DRAWITEMSTRUCT *draw);

        // Covers the light separator the frame draws between the menu bar and the client area.
        // Called from the master window's non-client painting; a no-op outside dark mode.
        static void paintMenuBarUnderline(HWND window);

        void setCurrentActiveSettingsWindow(std::unique_ptr<SettingsWindow> &&scene,
                                            SettingsWindowKind kind = SettingsWindowKind::OTHER);

        // Shuts every open settings panel. Loading a settings file, and starting a video export,
        // work off the attribute the open panels are bound to, and their rows would go on showing
        // (and writing back) what was there before.
        void closeAllSettingsWindows();

        // Shuts the Shader menu's panels only, leaving the rest open.
        void closeShaderSettingsWindows();

        // Shuts the Palette panel only.
        void closePaletteSettingsWindows();

        bool *getBool(RenderScene &scene, int menuID, bool executeMode) const;

        static int getIndex(int menuID);

        bool checkIndex(int index) const;
    };

    template<typename P> requires std::is_base_of_v<Preset, P>
    HMENU SettingsMenu::addPresetExecutor(HMENU target, const P preset) {
        return add(target, preset.getName(), [p1 = std::move(preset)](SettingsMenu &, RenderScene &scene) {
            scene.changePreset(p1);
        }, false, false, std::nullopt);
    }
}
