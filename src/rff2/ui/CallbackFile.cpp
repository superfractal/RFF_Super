//
// Created by Merutilm on 2025-05-14.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21, 2026-08-31.
// Modified by Opus 5 on 2026-08-08, 2026-08-10, 2026-08-14, 2026-08-24, 2026-08-26
//

#include "CallbackFile.hpp"

#include <cwctype>

#include "../constants/Constants.hpp"
#include "Callback.hpp"
#include "IOUtilities.h"
#include "SettingsMenu.hpp"
#include "SettingsWindow.hpp"
#include "../io/ConfigIO.h"
#include "../io/ShaderPresetIO.h"
#include "../io/RFFLocationBinary.h"
#include "../attr/Selectable.h"
#include "../formula/Perturbator.h"


namespace merutilm::rff2 {
    // Returns the lower-cased extension (with dot) of a path, e.g. ".rfl".
    static std::wstring lowerExt(const std::filesystem::path &path) {
        std::wstring ext = path.extension().wstring();
        std::ranges::transform(ext, ext.begin(), [](const wchar_t c) { return std::towlower(c); });
        return ext;
    }
    const std::function<void(SettingsMenu&, RenderScene&)> CallbackFile::SAVE_MAP = [](const SettingsMenu&, const RenderScene& scene) {
        // The map is copied straight out of the buffer the compute threads are filling, so a save
        // taken mid-render writes rows from before the front and rows from after it. Asked for
        // before the dialog, so the answer does not arrive after a file name has been chosen.
        if (!scene.isIdleCompute()) {
            MessageBoxW(nullptr,
                        L"The map is still being calculated.\n\n"
                        L"Wait for the render to finish, then save it again.",
                        L"Map not ready", MB_OK | MB_ICONINFORMATION);
            return;
        }
        // Compressed first, so it is what the dialog offers by default: it holds the same map to the
        // last bit in a fraction of the space, and Load Map opens either.
        const auto path = IOUtilities::ioFileDialogMulti(L"Save Map", IOUtilities::SAVE_FILE, {
            {Constants::Extension::DESC_COMPRESSED_MAP, Constants::Extension::COMPRESSED_MAP},
            {Constants::Extension::DESC_DYNAMIC_MAP, Constants::Extension::DYNAMIC_MAP}
        });
        if (path == nullptr) {
            return;
        }
        if (lowerExt(*path) == std::format(L".{}", Constants::Extension::DYNAMIC_MAP)) {
            scene.generateMap().exportFile(*path);
        } else if (!scene.generateMap().exportCompressedFile(*path)) {
            MessageBoxW(nullptr, L"Failed to save the map", L"Error", MB_OK | MB_ICONERROR);
        }
    };
    const std::function<void(SettingsMenu&, RenderScene&)> CallbackFile::SAVE_IMAGE = [](const SettingsMenu&, RenderScene& scene) {
        scene.getRequests().requestCreateImage();
    };
    const std::function<void(SettingsMenu&, RenderScene&)> CallbackFile::EXPORT_HIGHRES = [](SettingsMenu& settingsMenu, RenderScene& scene) {
        // Fog is handled: the tiles render without it and the export lays it over the stitched image.
        // Its Rim Mask is not, because that needs the iteration buffer of the whole grid at once, and
        // Bloom's full-frame downsample still breaks at the seams.
        const auto blockedReason = [](RenderScene &s) -> const wchar_t * {
            const auto &sh = s.getAttribute().shader;
            if (sh.bloom.intensity > 0.0f) {
                return L"High-resolution tiled export is unavailable while Bloom is enabled.\n"
                       L"Set its intensity to 0 and try again.";
            }
            if (sh.fog.opacity > 0.0f && sh.fog.rimMask > 0.0f) {
                return L"High-resolution tiled export is unavailable while Fog Rim Mask is above 0.\n"
                       L"Set Rim Mask to 0 (plain Fog is supported) and try again.";
            }
            return nullptr;
        };
        if (const wchar_t *reason = blockedReason(scene)) {
            MessageBoxW(nullptr, reason, L"Error", MB_OK | MB_ICONERROR);
            return;
        }
        // Persist the chosen grid across invocations; the field pointers must outlive the window.
        static uint32_t tilesX = 2;
        static uint32_t tilesY = 2;

        auto window = std::make_unique<SettingsWindow>(
            L"Export Tiled Image", Constants::Win32::INIT_SETTINGS_WINDOW_WIDTH, -1, 44);
        window->registerTextInput<uint32_t>(L"Tiles X", &tilesX, Unparser::U_LONG, Parser::U_LONG,
                                            [](const uint32_t& v) { return v >= 2 && v <= 64; }, Callback::NOTHING,
                                            L"Horizontal Tiles",
                                            L"Number of columns the image is split into. Tiles are rendered one by one to bound VRAM use. Aspect ratio stays correct for any X/Y.",
                                            0.0, 0.0, 0.0, 26);
        window->registerTextInput<uint32_t>(L"Tiles Y", &tilesY, Unparser::U_LONG, Parser::U_LONG,
                                            [](const uint32_t& v) { return v >= 2 && v <= 64; }, Callback::NOTHING,
                                            L"Vertical Tiles",
                                            L"Number of rows the image is split into. Tiles are rendered one by one to bound VRAM use. Aspect ratio stays correct for any X/Y.",
                                            0.0, 0.0, 0.0, 26);
        window->registerPrimaryButton(L"Export", [&scene, winPtr = window.get(), blockedReason] {
            // Re-checked here: this window is modeless, so the shader could have changed since it opened.
            if (const wchar_t *reason = blockedReason(scene)) {
                MessageBoxW(nullptr, reason, L"Error", MB_OK | MB_ICONERROR);
                return;
            }
            if (const std::wstring warn = scene.checkExportMemoryBudget(scene.getAttribute(), tilesX, tilesY);
                !warn.empty()) {
                const std::wstring text = L"This high-resolution export may run out of memory:\n\n" +
                                          warn + L"\nContinue anyway?";
                if (MessageBoxW(nullptr, text.c_str(), L"Memory warning", MB_YESNO | MB_ICONWARNING) == IDNO) {
                    return;
                }
            }
            scene.getRequests().requestExportHighRes(tilesX, tilesY);
            SendMessageW(winPtr->getWindow(), WM_CLOSE, 0, 0);
        }, L"Export", L"Start the high-resolution tiled export with the tile grid above.");
        window->registerNotesCard(
            L"Notes",
            {
                {L"Turn off Bloom, and Fog's Rim Mask, before exporting.",
                 L"The export will not start while either is enabled. Plain Fog is supported: it is applied to the finished image instead of to each tile."},
                {L"This image may come out very slightly different from one made by raising Clarity instead.",
                 L"A slight positioning error may become noticeable when zoomed in enough for pixelation to appear."},
                {L"Tiles are rendered slightly larger than they are kept, so the framing is a little tighter than the preview.",
                 L"The overlap is what gives the pixels next to a tile edge their real neighbours; Macro Relief widens it, because its slope reads that far out."},
                {L"A larger grid makes a bigger image and takes longer to save.",
                 L"Progress is shown in the status bar, and Escape cancels."},
            });
        window->setWindowCloseFunction([&settingsMenu] {
            settingsMenu.setCurrentActiveSettingsWindow(nullptr);
        });
        settingsMenu.setCurrentActiveSettingsWindow(std::move(window));
    };
    const std::function<void(SettingsMenu&, RenderScene&)> CallbackFile::LOAD_MAP = [](const SettingsMenu&, RenderScene& scene) {
        const auto path = IOUtilities::ioFileDialogMulti(L"Load Map", IOUtilities::OPEN_FILE, {
            {Constants::Extension::DESC_DYNAMIC_MAP, Constants::Extension::DYNAMIC_MAP},
            {Constants::Extension::DESC_COMPRESSED_MAP, Constants::Extension::COMPRESSED_MAP}
        });
        if (path == nullptr) {
            return;
        }
        scene.overwriteMatrixFromMap(RFFDynamicMapBinary::readAny(*path));
        // The arrow keys walk the rest of the folder from here, so a keyframe run can be looked
        // through without reopening the dialog for every map.
        scene.beginMapBrowse(*path);
    };
    const std::function<void(SettingsMenu&, RenderScene&)> CallbackFile::SAVE_CONFIG = [](const SettingsMenu&, RenderScene& scene) {
        const auto path = IOUtilities::ioFileDialogMulti(L"Save Location / Settings", IOUtilities::SAVE_FILE, {
            {Constants::Extension::DESC_CONFIG, Constants::Extension::CONFIG},
            {Constants::Extension::DESC_LOCATION, Constants::Extension::LOCATION}
        });
        if (path == nullptr) {
            return;
        }
        if (lowerExt(*path) == std::format(L".{}", Constants::Extension::LOCATION)) {
            const auto &fr = scene.getAttribute().fractal;
            RFFLocationBinary(fr.logZoom, fr.center.real.to_string(), fr.center.imag.to_string(),
                              fr.maxIteration).exportFile(*path);
        } else if (!ConfigIO::save(*path, scene.getAttribute(), scene.getClientWidth(), scene.getClientHeight())) {
            MessageBoxW(nullptr, L"Failed to save settings", L"Error", MB_OK | MB_ICONERROR);
        }
    };
    void CallbackFile::warnMissingTextureImages(const ShaderAttribute &shader) {
        const std::vector<std::wstring> missing = ShaderPresetIO::missingTextureImages(shader);
        if (missing.empty()) {
            return;
        }
        std::wstring text = L"The source image of a texture layer in this file was not found:\n\n";
        for (const auto &entry : missing) {
            text.append(entry).append(L"\n");
        }
        text.append(L"\nThose layers render nothing until their image is chosen again.");
        MessageBoxW(nullptr, text.c_str(), L"Warning", MB_OK | MB_ICONWARNING);
    }
    void CallbackFile::warnReuseReference(const FractalAttribute &fractal) {
        if (fractal.reuseReferenceMethod == FrtReuseReferenceMethod::DISABLED) {
            return;
        }
        // The file names a location of its own, but the reference it reuses is the one the previous
        // view left behind, so what it opens on is drawn around the wrong orbit.
        const std::wstring text = std::format(
            L"This settings file has Reuse Reference set to \"{}\".\n\n"
            L"The location it opens is computed from the reference orbit of the view that was on screen "
            L"before the load, which does not belong to it, so the image can come out wrong.\n"
            L"Set Fractal > Reference > Reuse Reference to \"Disabled\" and compute again.",
            Selectable::toString(fractal.reuseReferenceMethod));
        MessageBoxW(nullptr, text.c_str(), L"Warning", MB_OK | MB_ICONWARNING);
    }
    bool CallbackFile::applyConfigFile(RenderScene &scene, const std::filesystem::path &path, const bool generate) {
        uint16_t width = 0;
        uint16_t height = 0;
        if (!ConfigIO::load(path, scene.getAttribute(), &width, &height)) {
            return false;
        }
        scene.applyLoadedConfig();
        if (width > 0 && height > 0) {
            scene.wndRequestClientSize(width, height);
        }
        scene.getRequests().requestResize();
        scene.getRequests().requestShader();
        if (generate) {
            scene.getRequests().requestRecompute();
        }
        warnMissingTextureImages(scene.getAttribute().shader);
        warnReuseReference(scene.getAttribute().fractal);
        return true;
    }
    const std::function<void(SettingsMenu&, RenderScene&)> CallbackFile::LOAD_CONFIG = [](SettingsMenu& settingsMenu, RenderScene& scene) {
        // The panels are bound to the values this load overwrites, so they cannot stay open across it.
        settingsMenu.closeAllSettingsWindows();
        // Among them may be the one a recovery is waiting on. Choosing a file to load answers it:
        // what this brings in is what gets computed.
        scene.setComputeHold(false);
        const auto path = IOUtilities::ioFileDialogMulti(L"Load Location / Settings", IOUtilities::OPEN_FILE, {
            {Constants::Extension::DESC_CONFIG, Constants::Extension::CONFIG},
            {Constants::Extension::DESC_LOCATION, Constants::Extension::LOCATION}
        });
        if (path == nullptr) {
            return;
        }
        if (lowerExt(*path) == std::format(L".{}", Constants::Extension::LOCATION)) {
            // Legacy .rfl: location only, leaving all other settings untouched.
            const RFFLocationBinary location = RFFLocationBinary::read(*path);
            if (!location.hasData()) {
                MessageBoxW(nullptr, L"Failed to load location", L"Error", MB_OK | MB_ICONERROR);
                return;
            }
            auto &fr = scene.getAttribute().fractal;
            fr.center = fp_complex(location.getReal(), location.getImag(),
                                   Perturbator::logZoomToExp10(location.getLogZoom()));
            fr.logZoom = location.getLogZoom();
            fr.maxIteration = location.getMaxIteration();
            scene.getRequests().requestRecompute();
            return;
        }
        if (!applyConfigFile(scene, *path, true)) {
            MessageBoxW(nullptr, L"Failed to load settings", L"Error", MB_OK | MB_ICONERROR);
        }
    };
}
