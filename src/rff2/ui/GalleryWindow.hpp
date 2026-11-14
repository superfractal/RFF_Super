//
// Created by Opus 5.5 on 2026-11-07
// Modified by Opus 5.5 on 2026-11-10
//

#pragma once

namespace merutilm::rff2 {
    struct SettingsMenu;
    class RenderScene;

    // A window of its own showing the saved files of one folder - or the files used most recently - as
    // thumbnails. Gallery (Open) opens them the way Load File did; Gallery (Save) saves into that folder.
    struct GalleryWindow {
        GalleryWindow() = delete;

        // BROWSE opens from the folder, SAVE saves into the folder.
        enum class Mode { BROWSE, SAVE };

        // Opens the Gallery, or brings it forward.
        static void open(SettingsMenu &menu, RenderScene &scene, Mode mode);

        // Redraws it in the colors of the current theme.
        static void applyTheme();

        // Closes it ahead of the scene it opens files into.
        static void close();
    };
}
