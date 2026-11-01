//
// Created by Opus 5.5 on 2026-10-13.
//

#pragma once
#include <algorithm>
#include <memory>
#include <vector>
#include "VideoRenderScene.hpp"

namespace merutilm::rff2 {
    // The video scenes built at start-up only so their pipelines land in the pipeline cache. A skip
    // can leave their surface shader compiling, and the compile reads the scene's render passes, so
    // the scene and its hidden window are held here until it is done rather than torn down under it.
    class VideoPipelineWarmup {
    public:
        struct Window {
            vkh::EngineRef engine;
            HWND window;
            uint32_t index;
            bool attached = false;

            Window(vkh::EngineRef engine, HWND window, uint32_t index)
                : engine(engine), window(window), index(index) {
            }
            ~Window() {
                if (attached) {
                    engine.detachWindowContext(index);
                }
                DestroyWindow(window);
            }
            Window(const Window &) = delete;
            Window &operator=(const Window &) = delete;
        };

    private:
        // Declared window first, so the scene goes before the context it was built on.
        struct Entry {
            std::unique_ptr<Window> window;
            std::unique_ptr<VideoRenderScene> scene;
        };
        inline static std::vector<Entry> held;

    public:
        static void hold(std::unique_ptr<Window> window, std::unique_ptr<VideoRenderScene> scene) {
            held.push_back({std::move(window), std::move(scene)});
        }

        // Lets go of every warm-up whose compile has finished; true when one was let go.
        static bool poll() {
            return std::erase_if(held, [](const Entry &entry) {
                entry.scene->collectDeferredSlope();
                return !entry.scene->isSlopeDeferred();
            }) != 0;
        }

        // Waits for the rest with the preparation window up, for a caller about to reuse their
        // window contexts (the video window and the timeline preview) or shutting the engine down.
        static void drain() {
            for (const auto &entry : held) {
                entry.scene->awaitDeferredSlope();
            }
            held.clear();
        }
    };
} // namespace merutilm::rff2
