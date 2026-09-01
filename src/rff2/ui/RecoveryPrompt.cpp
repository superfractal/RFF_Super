//
// Created by Opus 5 on 2026-08-14.
// Modified by Opus 5 on 2026-08-15.
//

#include "RecoveryPrompt.hpp"

#include "CallbackFile.hpp"
#include "SettingsWindow.hpp"
#include "../constants/Constants.hpp"
#include "../io/ConfigIO.h"

namespace merutilm::rff2 {
    namespace {
        // Held back generation, waiting for the user to say go. Opened by "Restore and wait", and
        // the only thing left on screen from the recovery: the menus are what the settings are
        // lowered through, and this is what puts them to work.
        void openApprovalWindow(SettingsMenu &settingsMenu, RenderScene &scene) {
            auto window = std::make_unique<SettingsWindow>(
                L"Generate", Constants::Win32::INIT_SETTINGS_WINDOW_WIDTH, -1, 40);
            window->registerStaticText(
                L"The settings of the last run are loaded, and nothing has been computed yet. "
                L"Lower whatever looks too heavy through the menus, then generate.");
            window->registerPrimaryButton(L"Generate", [&scene, winPtr = window.get()] {
                scene.setComputeHold(false);
                scene.getRequests().requestRecompute();
                SendMessageW(winPtr->getWindow(), WM_CLOSE, 0, 0);
            }, L"Generate", L"Computes the view with the settings as they now stand.");
            // Closing this panel is not an approval: the view stays held, and the menu's Recompute
            // is what releases it. Set all the same, because every panel's close runs this.
            window->setWindowCloseFunction([] {
            });
            settingsMenu.setCurrentActiveSettingsWindow(std::move(window));
        }

        // Reads the kept settings without touching the scene, for the choice that takes only part
        // of them. Empty when the file is not one this build can read.
        std::optional<Attribute> readSnapshot(const std::filesystem::path &snapshot) {
            Attribute loaded = RenderScene::genDefaultAttr();
            if (!ConfigIO::load(snapshot, loaded, nullptr, nullptr)) {
                return std::nullopt;
            }
            return loaded;
        }

        void reportUnreadable() {
            MessageBoxW(nullptr, L"The kept settings could not be read.", L"Error", MB_OK | MB_ICONERROR);
        }

        // Grays the whole menu bar while the recovery panel is up. The choices decide what the run
        // starts from, so nothing may be steered through the menus until one of them is taken; the
        // bar is handed back when the panel closes, which is what the held "Restore and wait" needs.
        void setMenuBarEnabled(const bool enabled) {
            const HWND master = FindWindowW(Constants::Win32::CLASS_MASTER_WINDOW, nullptr);
            if (master == nullptr) {
                return;
            }
            const HMENU bar = GetMenu(master);
            if (bar == nullptr) {
                return;
            }
            const int items = GetMenuItemCount(bar);
            for (int i = 0; i < items; ++i) {
                EnableMenuItem(bar, i, MF_BYPOSITION | (enabled ? MF_ENABLED : MF_GRAYED));
            }
            DrawMenuBar(master);
        }
    }

    void RecoveryPrompt::offer(SettingsMenu &settingsMenu, RenderScene &scene,
                               const std::filesystem::path &snapshot, const RecoveryReason reason) {
        // Nothing is computed until one of the four is taken: the settings on offer are the ones the
        // last run ended on, and the second choice exists so they can be lowered before they run.
        scene.setComputeHold(true);
        setMenuBarEnabled(false);

        const bool interrupted = reason == RecoveryReason::INTERRUPTED;
        auto window = std::make_unique<SettingsWindow>(
            L"Recovery", Constants::Win32::INIT_SETTINGS_WINDOW_WIDTH, -1, 40);
        window->registerStaticText(
            interrupted
                ? L"RFF_Super was closed last time while it was still computing. The location and the "
                  L"settings it was working on were kept, and can be taken up again here. If the wait "
                  L"is what ended it, restore the settings and lower what is heavy before generating."
                : L"RFF_Super did not shut down last time. The location and the settings it was working "
                  L"on were kept, and can be taken up again here.");

        // Set by the choice that keeps the view held after this panel closes, so closing it - by a
        // button or by its own close box - does not release what that choice is holding.
        const auto keepHold = std::make_shared<bool>(false);
        const auto winPtr = window.get();
        const auto close = [winPtr] { SendMessageW(winPtr->getWindow(), WM_CLOSE, 0, 0); };

        window->registerButton(
            L"Settings and view", L"Restore and generate", [&scene, snapshot, close] {
                if (!CallbackFile::applyConfigFile(scene, snapshot, true)) {
                    reportUnreadable();
                }
                scene.setComputeHold(false);
                close();
            }, L"Restore everything, and generate",
            L"Restores the whole of the last run - location, fractal, render, shader, video and window "
            L"size - and starts computing it at once.");

        window->registerButton(
            L"Settings only", L"Restore and wait", [&settingsMenu, &scene, snapshot, keepHold, close] {
                if (!CallbackFile::applyConfigFile(scene, snapshot, false)) {
                    reportUnreadable();
                    scene.setComputeHold(false);
                    close();
                    return;
                }
                *keepHold = true;
                // Opened before this panel closes: closing it first would have the list of open
                // panels sweep this one away while its own button is still running.
                openApprovalWindow(settingsMenu, scene);
                close();
            }, L"Restore everything, and hold",
            interrupted
                ? L"Restores the same settings but computes nothing, so a setting that is too heavy "
                  L"can be lowered through the menus first. Generate when it is ready. Use this when "
                  L"the last run was too slow to wait out."
                : L"Restores the same settings but computes nothing, so a setting that is too heavy "
                  L"can be lowered through the menus first. Generate when it is ready. Use this when "
                  L"restoring and generating ends the same way as last time.");

        window->registerButton(
            L"Location only", L"Generate the location", [&scene, snapshot, close] {
                if (const std::optional<Attribute> loaded = readSnapshot(snapshot)) {
                    scene.applyRecoveredLocation(*loaded);
                    scene.getRequests().requestResize();
                    scene.getRequests().requestShader();
                    scene.getRequests().requestRecompute();
                } else {
                    reportUnreadable();
                }
                scene.setComputeHold(false);
                close();
            }, L"Take the place, not the settings",
            L"Takes the location - center, zoom and iterations - and the fractal formula, and leaves "
            L"every other setting, the shader included, at its default. Use this when a setting rather "
            L"than the place is what is suspected.");

        window->registerButton(
            L"Nothing", L"Ignore", [&scene, close] {
                scene.setComputeHold(false);
                close();
            }, L"Start as usual",
            L"Starts on the default view. The kept settings stay in the recovery folder and can still "
            L"be opened by hand.");

        window->setWindowCloseFunction([&scene, keepHold] {
            // The close box is a fourth "Ignore": the view must not stay held with nothing left on
            // screen to release it.
            if (!*keepHold) {
                scene.setComputeHold(false);
            }
            // Every way out of this panel hands the menu bar back, the held choice included: lowering
            // a setting through the menus is the whole of what it is holding the view for.
            setMenuBarEnabled(true);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window));
    }
}
