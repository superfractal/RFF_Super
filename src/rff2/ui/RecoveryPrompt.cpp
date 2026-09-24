//
// Created by Opus 5 on 2026-08-14.
// Modified by Opus 5 on 2026-08-15
// Modified by GPT-6 on 2026-09-14, 2026-09-23
//

#include "NativeDialogs.hpp"
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
                L"Your saved location and settings are restored. Review them in the workspace, "
                L"then choose Generate when ready.");
            window->registerPrimaryButton(L"Generate", [&scene, approvalWindow = window.get()] {
                scene.setComputeHold(false);
                scene.getRequests().requestRecompute();
                SendMessageW(approvalWindow->getWindow(), WM_CLOSE, 0, 0);
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
            NativeDialogs::message(nullptr, L"The kept settings could not be read.", L"Error", MB_OK | MB_ICONERROR);
        }

        // Grays the whole menu bar while the recovery panel is up. The choices decide what the run
        // starts from, so nothing may be steered through the menus until one of them is taken; the
        // bar is handed back when the panel closes, which is what the held "Restore and wait" needs.
        void setMenuBarEnabled(const bool enabled) {
            const HWND master = NativeDialogs::mainWindow();
            if (master == nullptr) {
                return;
            }
            const HMENU bar = GetMenu(master);
            if (bar == nullptr) {
                return;
            }
            const int itemCount = GetMenuItemCount(bar);
            for (int position = 0; position < itemCount; ++position) {
                EnableMenuItem(bar, position, MF_BYPOSITION | (enabled ? MF_ENABLED : MF_GRAYED));
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

        const bool renderingWasInterrupted = reason == RecoveryReason::INTERRUPTED;
        auto window = std::make_unique<SettingsWindow>(
            L"Recovery", Constants::Win32::INIT_SETTINGS_WINDOW_WIDTH, -1, 40);
        window->registerStaticText(
            renderingWasInterrupted
                ? L"Your previous session ended during rendering. Its location and settings are available "
                  L"to restore. Choose Restore and wait to review the settings before rendering."
                : L"Your previous session did not close normally. Its location and settings are available "
                  L"to restore. Choose Restore and wait to review the settings before rendering.");

        // Set by the choice that keeps the view held after this panel closes, so closing it - by a
        // button or by its own close box - does not release what that choice is holding.
        const auto keepComputeHeld = std::make_shared<bool>(false);
        const auto recoveryWindow = window.get();
        const auto closeRecoveryWindow = [recoveryWindow] {
            SendMessageW(recoveryWindow->getWindow(), WM_CLOSE, 0, 0);
        };

        window->registerButton(
            L"Resume rendering", L"Restore and generate", [&scene, snapshot, closeRecoveryWindow] {
                if (!CallbackFile::applyConfigFile(scene, snapshot, true)) {
                    reportUnreadable();
                } else {
                    scene.notifyConfigFile({}, true, scene.getWndCWRequest(), scene.getWndCHRequest());
                }
                scene.setComputeHold(false);
                closeRecoveryWindow();
            }, L"Restore everything, and generate",
            L"Restores the whole of the last run - location, fractal, render, shader, video and window "
            L"size - and starts computing it at once.");

        window->registerButton(
            L"Review settings", L"Restore and wait",
            [&settingsMenu, &scene, snapshot, keepComputeHeld, closeRecoveryWindow] {
                if (!CallbackFile::applyConfigFile(scene, snapshot, false)) {
                    reportUnreadable();
                    scene.setComputeHold(false);
                    closeRecoveryWindow();
                    return;
                }
                *keepComputeHeld = true;
                scene.notifyConfigFile({}, true, scene.getWndCWRequest(), scene.getWndCHRequest());
                // Opened before this panel closes: closing it first would have the list of open
                // panels sweep this one away while its own button is still running.
                openApprovalWindow(settingsMenu, scene);
                closeRecoveryWindow();
            }, L"Restore everything, and hold",
            renderingWasInterrupted
                ? L"Restores the same settings but computes nothing, so a setting that is too heavy "
                  L"can be lowered in the workspace first. Generate when it is ready. Use this when "
                  L"the last run was too slow to wait out."
                : L"Restores the same settings but computes nothing, so a setting that is too heavy "
                  L"can be lowered in the workspace first. Generate when it is ready. Use this when "
                  L"restoring and generating ends the same way as last time.");

        window->registerButton(
            L"Keep only location", L"Generate the location", [&scene, snapshot, closeRecoveryWindow] {
                if (const std::optional<Attribute> recovered = readSnapshot(snapshot)) {
                    scene.applyRecoveredLocation(*recovered);
                    scene.getRequests().requestResize();
                    scene.getRequests().requestShader();
                    scene.getRequests().requestRecompute();
                } else {
                    reportUnreadable();
                }
                scene.setComputeHold(false);
                closeRecoveryWindow();
            }, L"Take the place, not the settings",
            L"Takes the location - center, zoom and iterations - and the fractal formula, and leaves "
            L"every other setting, the shader included, at its default. Use this when a setting rather "
            L"than the place is what is suspected.");

        window->registerButton(
            L"Start fresh", L"Ignore", [&scene, closeRecoveryWindow] {
                scene.setComputeHold(false);
                closeRecoveryWindow();
            }, L"Start as usual",
            L"Starts on the default view. The kept settings stay in the recovery folder and can still "
            L"be opened by hand.");

        window->setWindowCloseFunction([&scene, keepComputeHeld] {
            // The close box is a fourth "Ignore": the view must not stay held with nothing left on
            // screen to release it.
            if (!*keepComputeHeld) {
                scene.setComputeHold(false);
            }
            // Every way out of this panel hands the menu bar back, the held choice included: lowering
            // a setting through the menus is the whole of what it is holding the view for.
            setMenuBarEnabled(true);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window));
    }
}
