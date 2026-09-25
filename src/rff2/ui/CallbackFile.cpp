//
// Created by Merutilm on 2025-05-14.
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-08, 2026-08-10, 2026-08-14, 2026-08-24, 2026-08-26, 2026-09-03
// Modified by GPT-5 on 2026-08-21, 2026-08-31
// Modified by GPT-6 on 2026-09-14, 2026-09-22, 2026-09-23, 2026-09-25
// Modified by Opus 5.5 on 2026-09-23
//

#include "NativeDialogs.hpp"
#include "CallbackFile.hpp"
#include "../io/MapLimits.hpp"

#include <cwctype>

#include "../constants/Constants.hpp"
#include "IOUtilities.h"
#include "SettingsMenu.hpp"
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
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackFile::SAVE_MAP =
        [](const SettingsMenu &, RenderScene &scene) {
            // The map is copied straight out of the buffer the compute threads are filling, so a save
            // taken mid-render writes rows from before the front and rows from after it. Asked for
            // before the dialog, so the answer does not arrive after a file name has been chosen.
            if (!scene.isIdleCompute()) {
                NativeDialogs::message(nullptr,
                                       L"The map is still being calculated.\n\n"
                                       L"Wait for the render to finish, then save it again.",
                                       L"Map not ready", MB_OK | MB_ICONINFORMATION);
                return;
            }
            if (!MapLimits::valid(scene.getIterationBufferWidth(scene.getAttribute()),
                                  scene.getIterationBufferHeight(scene.getAttribute()))) {
                NativeDialogs::message(nullptr, L"Map files support at most 100,000,000 pixels. Reduce resolution or internal scale.",
                                       L"Map too large", MB_OK | MB_ICONERROR);
                return;
            }
            // Compressed first, so it is what the dialog offers by default: it holds the same map to the
            // last bit in a fraction of the space, and Load Map opens either.
            const auto path = IOUtilities::ioFileDialogMulti(
                L"Save Map", IOUtilities::SAVE_FILE,
                {{Constants::Extension::DESC_COMPRESSED_MAP, Constants::Extension::COMPRESSED_MAP},
                 {Constants::Extension::DESC_DYNAMIC_MAP, Constants::Extension::DYNAMIC_MAP}});
            if (path == nullptr) {
                return;
            }
            if (lowerExt(*path) == std::format(L".{}", Constants::Extension::DYNAMIC_MAP)) {
                scene.generateMap().exportFile(*path);
            } else if (!scene.generateMap().exportCompressedFile(*path)) {
                NativeDialogs::message(nullptr, L"Failed to save the map", L"Error", MB_OK | MB_ICONERROR);
            }
        };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackFile::SAVE_IMAGE =
        [](const SettingsMenu &, RenderScene &scene) { scene.getRequests().requestCreateImage(); };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackFile::LOAD_MAP = [](const SettingsMenu &,
                                                                                         RenderScene &scene) {
        const auto path = IOUtilities::ioFileDialogMulti(
            L"Load Map", IOUtilities::OPEN_FILE,
            {{Constants::Extension::DESC_DYNAMIC_MAP, Constants::Extension::DYNAMIC_MAP},
             {Constants::Extension::DESC_COMPRESSED_MAP, Constants::Extension::COMPRESSED_MAP}});
        if (path == nullptr) {
            return;
        }
        if (!scene.overwriteMatrixFromMap(RFFDynamicMapBinary::readAny(*path))) {
            NativeDialogs::message(nullptr, L"The map could not be loaded at the current canvas size.",
                                   L"Load Map", MB_OK | MB_ICONERROR);
            return;
        }
        // The arrow keys walk the rest of the folder from here, so a keyframe run can be looked
        // through without reopening the dialog for every map.
        scene.beginMapBrowse(*path);
    };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackFile::LOAD_IMAGE =
        [](const SettingsMenu &, RenderScene &scene) {
            const auto path = IOUtilities::ioFileDialog(L"Load Image", Constants::Extension::DESC_IMAGE,
                                                        IOUtilities::OPEN_FILE, Constants::Extension::IMAGE);
            if (path == nullptr) {
                return;
            }
            // The arrow keys walk the rest of the folder from here, so a keyframe run can be looked
            // through without reopening the dialog for every picture.
            scene.beginImageBrowse(*path);
        };
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackFile::SAVE_CONFIG =
        [](const SettingsMenu &, RenderScene &scene) {
            if (!scene.prepareConfigSave()) {
                return;
            }
            const auto path = IOUtilities::ioFileDialogMulti(
                L"Save Location / Settings", IOUtilities::SAVE_FILE,
                {{Constants::Extension::DESC_CONFIG, Constants::Extension::CONFIG},
                 {Constants::Extension::DESC_LOCATION, Constants::Extension::LOCATION}});
            if (path == nullptr) {
                return;
            }
            if (lowerExt(*path) == std::format(L".{}", Constants::Extension::LOCATION)) {
                const auto &fr = scene.getAttribute().fractal;
                RFFLocationBinary(fr.logZoom, fr.center.real.to_string(), fr.center.imag.to_string(),
                                  fr.maxIteration)
                    .exportFile(*path);
            } else if (!ConfigIO::save(*path, scene.getAttribute(), scene.documentCanvasSize().cx,
                                       scene.documentCanvasSize().cy)) {
                NativeDialogs::message(nullptr, L"Failed to save settings", L"Error", MB_OK | MB_ICONERROR);
            } else {
                scene.notifyConfigFile(*path, false);
            }
        };
    bool CallbackFile::saveConfigTo(RenderScene &scene, const std::filesystem::path &path) {
        if (!scene.prepareConfigSave() || path.empty()) {
            return false;
        }
        const auto dimensions = scene.documentCanvasSize();
        if (!ConfigIO::save(path, scene.getAttribute(), dimensions.cx, dimensions.cy)) {
            return false;
        }
        scene.notifyConfigFile(path, false);
        return true;
    }
    bool CallbackFile::saveCurrentConfig(RenderScene &scene) {
        if (!scene.prepareConfigSave()) {
            return false;
        }
        auto path = scene.getConfigDocumentPath();
        if (path.empty()) {
            const auto selected =
                IOUtilities::ioFileDialog(L"Save Settings", Constants::Extension::DESC_CONFIG,
                                          IOUtilities::SAVE_FILE, Constants::Extension::CONFIG);
            if (!selected) {
                return false;
            }
            path = *selected;
        }
        if (saveConfigTo(scene, path)) {
            return true;
        }
        NativeDialogs::message(nullptr, L"Settings could not be saved. Your changes remain open.",
                               L"Save failed", MB_OK | MB_ICONERROR);
        return false;
    }
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackFile::NEW_CONFIG =
        [](SettingsMenu &settingsMenu, RenderScene &scene) {
            if (!scene.prepareConfigReplace()) {
                return;
            }
            auto candidate = RenderScene::genDefaultAttr();
            scene.commitConfigReplace();
            settingsMenu.closeAllSettingsWindows();
            scene.cancelRunningCompute();
            scene.setComputeHold(false);
            scene.getAttribute() = std::move(candidate);
            scene.applyLoadedConfig();
            scene.notifyConfigFile({}, true);
            scene.wndRequestFPS();
            scene.getRequests().requestResize();
            scene.getRequests().requestShader();
            scene.getRequests().requestRecompute();
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
        NativeDialogs::message(nullptr, text.c_str(), L"Warning", MB_OK | MB_ICONWARNING);
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
        NativeDialogs::message(nullptr, text.c_str(), L"Warning", MB_OK | MB_ICONWARNING);
    }
    bool CallbackFile::applyConfigFile(RenderScene &scene, const std::filesystem::path &path,
                                       const bool generate, const std::function<void()> &beforeCommit) {
        uint16_t width = 0;
        uint16_t height = 0;
        auto candidate = scene.getAttribute();
        if (!ConfigIO::load(path, candidate, &width, &height)) {
            return false;
        }
        // The same limit the Export workspace enforces, against the canvas this file is about to resize to.
        const uint32_t targetWidth = width > 0 && height > 0 ? width : scene.getClientWidth();
        const uint32_t targetHeight = width > 0 && height > 0 ? height : scene.getClientHeight();
        if (const float maximum = scene.getMaxInternalScale(targetWidth, targetHeight);
            maximum > 0.0f && static_cast<double>(candidate.render.clarityMultiplier) * candidate.render.ssaa *
                              candidate.video.data.sourceScale > maximum) {
            NativeDialogs::message(nullptr, L"This settings file's resolution, clarity and supersampling exceed the "
                                            L"GPU render-size limit. The settings were not loaded.",
                                   L"Warning", MB_OK | MB_ICONWARNING);
            return true;
        }
        if (!scene.prepareConfigReplace()) {
            return true;
        }
        if (!ConfigIO::load(path, candidate, &width, &height)) {
            return false;
        }
        scene.commitConfigReplace();
        if (beforeCommit) {
            beforeCommit();
        }
        scene.cancelRunningCompute();
        scene.setComputeHold(!generate);
        scene.getAttribute() = std::move(candidate);
        scene.applyLoadedConfig();
        scene.wndRequestFPS();
        scene.notifyConfigFile(path, true, width, height);
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
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackFile::LOAD_CONFIG =
        [](SettingsMenu &settingsMenu, RenderScene &scene) {
            // The panels are bound to the values this load overwrites, so they cannot stay open across it.
            // Among them may be the one a recovery is waiting on. Choosing a file to load answers it:
            // what this brings in is what gets computed.
            const auto path = IOUtilities::ioFileDialogMulti(
                L"Load Location / Settings", IOUtilities::OPEN_FILE,
                {{Constants::Extension::DESC_CONFIG, Constants::Extension::CONFIG},
                 {Constants::Extension::DESC_LOCATION, Constants::Extension::LOCATION}});
            if (path == nullptr) {
                return;
            }
            if (lowerExt(*path) == std::format(L".{}", Constants::Extension::LOCATION)) {
                // Legacy .rfl: location only, leaving all other settings untouched.
                const RFFLocationBinary location = RFFLocationBinary::read(*path);
                if (!location.hasData()) {
                    NativeDialogs::message(nullptr, L"Failed to load location", L"Error",
                                           MB_OK | MB_ICONERROR);
                    return;
                }
                if (!scene.prepareConfigReplace()) {
                    return;
                }
                scene.commitConfigReplace();
                settingsMenu.closeAllSettingsWindows();
                scene.cancelRunningCompute();
                scene.setComputeHold(false);
                auto &fr = scene.getAttribute().fractal;
                fr.center = fp_complex(location.getReal(), location.getImag(),
                                       Perturbator::logZoomToExp10(location.getLogZoom()));
                fr.logZoom = location.getLogZoom();
                fr.maxIteration = location.getMaxIteration();
                scene.getRequests().requestRecompute();
                return;
            }
            if (!applyConfigFile(scene, *path, true,
                                 [&settingsMenu] { settingsMenu.closeAllSettingsWindows(); })) {
                NativeDialogs::message(nullptr, L"Failed to load settings", L"Error", MB_OK | MB_ICONERROR);
            }
        };
}
