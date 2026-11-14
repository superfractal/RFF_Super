//
// Created by Merutilm on 2025-05-14.
// Modified by AI; earlier exact modification date unavailable.
// Modified by Opus 5 on 2026-08-14, 2026-08-24, 2026-09-03
// Modified by GPT-5 on 2026-08-21
// Modified by GPT-6 on 2026-09-14, 2026-09-23
// Modified by Opus 5.5 on 2026-10-25, 2026-11-07, 2026-11-10, 2026-11-14
//

#pragma once
#include <filesystem>
#include <functional>

#include "RenderScene.hpp"
#include "SettingsMenu.hpp"

namespace merutilm::rff2 {
    struct CallbackFile {
        static const std::function<void(SettingsMenu &, RenderScene &)> NEW_CONFIG;
        // Overwrites the open settings file; a document never saved opens the Gallery to be named there.
        static const std::function<void(SettingsMenu &, RenderScene &)> SAVE_SETTINGS;

        // What the Gallery's save bar writes. WORK is the settings and an image, and the map when asked for.
        enum class SaveKind : uint8_t { WORK, SETTINGS, LOCATION, MAP, PLAIN_MAP, IMAGE, APPEARANCE };

        // Saves the view as name in folder, asking before any file there is replaced. False when nothing was written.
        // askReplace false is for a caller that has already asked.
        static bool saveInto(RenderScene &scene, SaveKind kind, const std::filesystem::path &folder,
                             const std::wstring &name, bool withMap, bool askReplace = true);
        static bool saveCurrentConfig(RenderScene &scene);
        static bool saveConfigTo(RenderScene &scene, const std::filesystem::path &path);

        // Opens any file Load File offers, told apart by extension, and lists it under Recent Files. False
        // when it was not opened: it could not be read, or the unsaved-changes prompt was cancelled.
        static bool openFile(SettingsMenu &settingsMenu, RenderScene &scene, const std::filesystem::path &path);

        // Leaves a thumbnail of the view beside file, when saving with thumbnails is turned on.
        static void attachThumbnail(RenderScene &scene, const std::filesystem::path &file);

        // What each save keeps, shown inside its dialog.
        static constexpr auto SETTINGS_NOTE =
            L"Settings (.rfc) keep the place and every setting, so you can continue from here later. "
            L"Location (.rfl) keeps only the place, the zoom and the iteration limit.";

        // Warns about texture layers whose source image the just-loaded file points at but which is
        // not on disk. Shared with the shader-preset loader, which carries the same paths.
        static void warnMissingTextureImages(const ShaderAttribute &shader);

        // Warns that the just-loaded settings file reuses a reference orbit. The one on hand belongs
        // to the view that was open before the load, not to the location the file names.
        static void warnReuseReference(const FractalAttribute &fractal);

        // Puts a settings file on the scene: the load itself, the window size it names, and the
        // requests that bring the view to what it says. generate=false leaves the view uncomputed,
        // which is what recovery's "restore without generating" needs. False when the file is not a
        // settings file this build can read, which leaves the scene as it was.
        static bool applyConfigFile(RenderScene &scene, const std::filesystem::path &path,
                                    bool generate,
                                    const std::function<void()> &beforeCommit = {});
    };
}
