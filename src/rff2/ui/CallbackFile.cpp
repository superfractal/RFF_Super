//
// Created by Merutilm on 2025-05-14.
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-08, 2026-08-10, 2026-08-14, 2026-08-24, 2026-08-26, 2026-09-03
// Modified by GPT-5 on 2026-08-21, 2026-08-31
// Modified by GPT-6 on 2026-09-14, 2026-09-22, 2026-09-23, 2026-09-25
// Modified by Opus 5.5 on 2026-09-23, 2026-10-04, 2026-10-25, 2026-10-27, 2026-11-07, 2026-11-10
//

#include "NativeDialogs.hpp"
#include "CallbackFile.hpp"
#include "../io/MapLimits.hpp"

#include <cwctype>

#include "../constants/Constants.hpp"
#include "IOUtilities.h"
#include "SettingsMenu.hpp"
#include "CallbackShader.hpp"
#include "GalleryWindow.hpp"
#include "../io/ConfigIO.h"
#include "../io/GalleryIO.hpp"
#include "../io/PreferencesIO.h"
#include "../io/ThumbnailIO.hpp"
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
    // Whether the map on screen can be written out now; tell explains why not.
    static bool mapReady(RenderScene &scene, const bool tell) {
        // The map is copied straight out of the buffer the compute threads are filling, so a save
        // taken mid-render writes rows from before the front and rows from after it. Asked for
        // before the dialog, so the answer does not arrive after a file name has been chosen.
        // A paused-preview export holds the map aside at one pixel; saving then would write that instead.
        if (scene.isPreviewImagesReleased() ||
            (scene.getVideoExportActive() && scene.getAttribute().video.exportation.pauseMainPreview)) {
            if (tell) {
                NativeDialogs::message(nullptr,
                                       L"The map is set aside while the video export runs.\n\n"
                                       L"Save it again after the export ends.",
                                       L"Map not ready", MB_OK | MB_ICONINFORMATION);
            }
            return false;
        }
        if (!scene.isIdleCompute()) {
            if (tell) {
                NativeDialogs::message(nullptr,
                                       L"The map is still being calculated.\n\n"
                                       L"Wait for the render to finish, then save it again.",
                                       L"Map not ready", MB_OK | MB_ICONINFORMATION);
            }
            return false;
        }
        if (!MapLimits::valid(scene.getIterationBufferWidth(scene.getAttribute()),
                              scene.getIterationBufferHeight(scene.getAttribute()))) {
            if (tell) {
                NativeDialogs::message(nullptr, L"Map files support at most 200,000,000 pixels. Reduce resolution or internal scale.",
                                       L"Map too large", MB_OK | MB_ICONERROR);
            }
            return false;
        }
        return true;
    }
    void CallbackFile::attachThumbnail(RenderScene &scene, const std::filesystem::path &file) {
        if (PreferencesIO::thumbnailsForSaves()) {
            scene.requestThumbnail(ThumbnailIO::pathFor(file), PreferencesIO::thumbnailWidth());
        }
    }
    static bool loadMap(RenderScene &scene, const std::filesystem::path &path) {
        if (!scene.overwriteMatrixFromMap(RFFDynamicMapBinary::readAny(path))) {
            NativeDialogs::message(nullptr, L"The map could not be loaded at the current canvas size.",
                                   L"Load Map", MB_OK | MB_ICONERROR);
            return false;
        }
        // The arrow keys walk the rest of the folder from here, so a keyframe run can be looked
        // through without reopening the dialog for every map.
        scene.beginMapBrowse(path);
        return true;
    }
    bool CallbackFile::saveConfigTo(RenderScene &scene, const std::filesystem::path &path) {
        if (!scene.prepareConfigSave() || path.empty()) {
            return false;
        }
        const auto dimensions = scene.documentCanvasSize();
        if (!ConfigIO::save(path, scene.getAttribute(), dimensions.cx, dimensions.cy)) {
            return false;
        }
        scene.notifyConfigFile(path, false);
        attachThumbnail(scene, path);
        GalleryIO::addRecent(path);
        return true;
    }
    bool CallbackFile::saveCurrentConfig(RenderScene &scene) {
        if (!scene.prepareConfigSave()) {
            return false;
        }
        auto path = scene.getConfigDocumentPath();
        if (path.empty()) {
            IOUtilities::FileDialogRequest request{
                .title = L"Save Settings As", .type = IOUtilities::SAVE_FILE,
                .filters = {{Constants::Extension::DESC_CONFIG, Constants::Extension::CONFIG}},
                .note = SETTINGS_NOTE};
            const auto selected = IOUtilities::fileDialog(request);
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
        // Translated sentence by sentence: with the file names in it, no table entry matches the whole text.
        std::wstring text = UiLanguage::text(L"The source image of a texture layer in this file was not found:") + L"\n\n";
        for (const auto &entry : missing) {
            text.append(entry).append(L"\n");
        }
        text.append(L"\n").append(UiLanguage::text(L"Those layers render nothing until their image is chosen again."));
        NativeDialogs::message(nullptr, text.c_str(), L"Warning", MB_OK | MB_ICONWARNING);
    }
    void CallbackFile::warnReuseReference(const FractalAttribute &fractal) {
        if (fractal.reuseReferenceMethod == FrtReuseReferenceMethod::DISABLED) {
            return;
        }
        // The file names a location of its own, but the reference it reuses is the one the previous
        // view left behind, so what it opens on is drawn around the wrong orbit.
        // The pattern is translated before the setting's name goes in, which no table entry could match.
        const std::wstring method = UiLanguage::text(Selectable::toString(fractal.reuseReferenceMethod));
        const std::wstring text = std::vformat(
            UiLanguage::text(
                L"This settings file has Reuse Reference set to \"{}\".\n\n"
                L"The location it opens is computed from the reference orbit of the view that was on screen "
                L"before the load, which does not belong to it, so the image can come out wrong.\n"
                L"Set Fractal > Reference > Reuse Reference to \"Disabled\" and compute again."),
            std::make_wformat_args(method));
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
    static bool loadConfig(SettingsMenu &settingsMenu, RenderScene &scene, const std::filesystem::path &path) {
        // The panels are bound to the values this load overwrites, so they cannot stay open across it.
        // Among them may be the one a recovery is waiting on. Choosing a file to load answers it:
        // what this brings in is what gets computed.
        if (lowerExt(path) == std::format(L".{}", Constants::Extension::LOCATION)) {
            // Legacy .rfl: location only, leaving all other settings untouched.
            const RFFLocationBinary location = RFFLocationBinary::read(path);
            if (!location.hasData()) {
                NativeDialogs::message(nullptr, L"Failed to load location", L"Error",
                                       MB_OK | MB_ICONERROR);
                return false;
            }
            if (!scene.prepareConfigReplace()) {
                return false;
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
            return true;
        }
        if (!CallbackFile::applyConfigFile(scene, path, true,
                                           [&settingsMenu] { settingsMenu.closeAllSettingsWindows(); })) {
            NativeDialogs::message(nullptr, L"Failed to load settings", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }
        // A cancelled prompt or a size past the GPU limit leaves the document as it was.
        return scene.getConfigDocumentPath() == path;
    }
    // One dialog for every kind of file the program reads. The extension picks what the file is put
    // to, so each kind keeps its own load: a map onto the canvas, a picture into the viewer, a
    // settings or location file onto the scene, an appearance preset onto the shader.
    bool CallbackFile::openFile(SettingsMenu &settingsMenu, RenderScene &scene, const std::filesystem::path &path) {
        using namespace Constants::Extension;
        const std::wstring ext = lowerExt(path);
        const auto is = [&ext](const wchar_t *kind) { return ext == std::format(L".{}", kind); };
        if (GalleryIO::kindOf(path)) {
            GalleryIO::addRecent(path);
        }
        if (is(COMPRESSED_MAP) || is(DYNAMIC_MAP)) {
            return loadMap(scene, path);
        }
        if (is(IMAGE)) {
            // The arrow keys walk the rest of the folder from here, so a keyframe run can be looked
            // through without reopening the dialog for every picture.
            scene.beginImageBrowse(path);
            return scene.isImageBrowsing();
        }
        if (is(CONFIG) || is(LOCATION)) {
            return loadConfig(settingsMenu, scene, path);
        }
        if (is(SHADER_PRESET)) {
            return CallbackShader::loadPreset(settingsMenu, scene, path);
        }
        NativeDialogs::message(nullptr, L"This kind of file cannot be loaded.", L"Load File",
                               MB_OK | MB_ICONERROR);
        return false;
    }
    const std::function<void(SettingsMenu &, RenderScene &)> CallbackFile::SAVE_SETTINGS =
        [](SettingsMenu &settingsMenu, RenderScene &scene) {
            if (scene.getConfigDocumentPath().empty()) {
                GalleryWindow::open(settingsMenu, scene, GalleryWindow::Mode::SAVE);
                return;
            }
            saveCurrentConfig(scene);
        };
    bool CallbackFile::saveInto(RenderScene &scene, const SaveKind kind, const std::filesystem::path &folder,
                                const std::wstring &name, bool withMap, const bool askReplace) {
        using namespace Constants::Extension;
        const auto file = [&folder, &name](const wchar_t *extension) {
            return folder / std::format(L"{}.{}", name, extension);
        };
        std::vector<std::filesystem::path> targets;
        switch (kind) {
        case SaveKind::WORK:
            targets = {file(CONFIG), file(IMAGE)};
            break;
        case SaveKind::SETTINGS: targets = {file(CONFIG)}; break;
        case SaveKind::LOCATION: targets = {file(LOCATION)}; break;
        case SaveKind::MAP: targets = {file(COMPRESSED_MAP)}; break;
        case SaveKind::PLAIN_MAP: targets = {file(DYNAMIC_MAP)}; break;
        case SaveKind::IMAGE: targets = {file(IMAGE)}; break;
        case SaveKind::APPEARANCE: targets = {file(SHADER_PRESET)}; break;
        }
        if ((kind == SaveKind::MAP || kind == SaveKind::PLAIN_MAP) && !mapReady(scene, true)) {
            return false;
        }
        if (kind == SaveKind::WORK && withMap) {
            if (mapReady(scene, false)) {
                targets.push_back(file(COMPRESSED_MAP));
            } else if (NativeDialogs::message(nullptr,
                                              L"The map cannot be saved until the calculation has finished.\n\n"
                                              L"Save the settings and the image now, without the map?",
                                              L"Save Work", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                withMap = false;
            } else {
                return false;
            }
        }
        if ((kind == SaveKind::WORK || kind == SaveKind::SETTINGS) && !scene.prepareConfigSave()) {
            return false;
        }
        // Asked before anything is written, so a No leaves the folder as it was.
        std::wstring replaced;
        std::error_code error;
        for (const auto &target : targets) {
            if (std::filesystem::exists(target, error)) {
                replaced.append(target.filename().wstring()).append(L"\n");
            }
        }
        if (!replaced.empty() && askReplace) {
            const std::wstring text = UiLanguage::text(L"These files will be replaced:") + L"\n\n" + replaced;
            if (NativeDialogs::message(nullptr, text.c_str(), L"Confirm replacement",
                                       MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) {
                return false;
            }
        }
        const std::filesystem::path &first = targets.front();
        switch (kind) {
        case SaveKind::WORK: {
            const auto dimensions = scene.documentCanvasSize();
            if (!ConfigIO::save(first, scene.getAttribute(), dimensions.cx, dimensions.cy)) {
                NativeDialogs::message(nullptr, L"Failed to save settings", L"Error", MB_OK | MB_ICONERROR);
                return false;
            }
            scene.notifyConfigFile(first, false);
            GalleryIO::addRecent(first);
            // The image is written once the view has finished calculating, the way an image save waits.
            RenderSceneRequests::CreateImageRequest picture{.filename = targets[1], .downsample = true};
            if (PreferencesIO::thumbnailsForSaves()) {
                picture.thumbnail = ThumbnailIO::pathFor(first);
                picture.thumbnailWidth = PreferencesIO::thumbnailWidth();
            }
            scene.getRequests().requestCreateImage(std::move(picture));
            if (withMap && !scene.generateMap().exportCompressedFile(targets[2])) {
                NativeDialogs::message(nullptr, L"Failed to save the map", L"Error", MB_OK | MB_ICONERROR);
            }
            return true;
        }
        case SaveKind::SETTINGS: {
            const auto dimensions = scene.documentCanvasSize();
            if (!ConfigIO::save(first, scene.getAttribute(), dimensions.cx, dimensions.cy)) {
                NativeDialogs::message(nullptr, L"Failed to save settings", L"Error", MB_OK | MB_ICONERROR);
                return false;
            }
            scene.notifyConfigFile(first, false);
            break;
        }
        case SaveKind::LOCATION: {
            const auto &fr = scene.getAttribute().fractal;
            RFFLocationBinary(fr.logZoom, fr.center.real.to_string(), fr.center.imag.to_string(), fr.maxIteration)
                .exportFile(first);
            if (!std::filesystem::is_regular_file(first, error)) {
                NativeDialogs::message(nullptr, L"Failed to save settings", L"Error", MB_OK | MB_ICONERROR);
                return false;
            }
            break;
        }
        case SaveKind::MAP:
            if (!scene.generateMap().exportCompressedFile(first)) {
                NativeDialogs::message(nullptr, L"Failed to save the map", L"Error", MB_OK | MB_ICONERROR);
                return false;
            }
            break;
        case SaveKind::PLAIN_MAP:
            scene.generateMap().exportFile(first);
            if (!std::filesystem::is_regular_file(first, error)) {
                NativeDialogs::message(nullptr, L"Failed to save the map", L"Error", MB_OK | MB_ICONERROR);
                return false;
            }
            break;
        case SaveKind::IMAGE:
            // The picture is its own thumbnail.
            scene.getRequests().requestCreateImage(first, true);
            GalleryIO::addRecent(first);
            return true;
        case SaveKind::APPEARANCE:
            if (!ShaderPresetIO::save(first, scene.getAttribute().shader)) {
                NativeDialogs::message(nullptr, L"Failed to save shader preset", L"Error", MB_OK | MB_ICONERROR);
                return false;
            }
            break;
        }
        attachThumbnail(scene, first);
        GalleryIO::addRecent(first);
        return true;
    }
}
