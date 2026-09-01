//
// Created by Merutilm on 2025-05-14.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21.
// Modified by Opus 5 on 2026-08-14, 2026-08-24
//

#pragma once
#include <functional>

#include "RenderScene.hpp"
#include "SettingsMenu.hpp"

namespace merutilm::rff2 {
    struct CallbackFile {
        static const std::function<void(SettingsMenu &, RenderScene &)> SAVE_MAP;
        static const std::function<void(SettingsMenu &, RenderScene &)> SAVE_IMAGE;
        static const std::function<void(SettingsMenu &, RenderScene &)> EXPORT_HIGHRES;
        static const std::function<void(SettingsMenu &, RenderScene &)> LOAD_MAP;
        static const std::function<void(SettingsMenu &, RenderScene &)> SAVE_CONFIG;
        static const std::function<void(SettingsMenu &, RenderScene &)> LOAD_CONFIG;

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
        static bool applyConfigFile(RenderScene &scene, const std::filesystem::path &path, bool generate);
    };
}
