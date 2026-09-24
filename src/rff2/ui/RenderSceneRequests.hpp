//
// Created by Merutilm on 2025-09-05.
// Modified by AI; earlier exact modification date unavailable.
// Modified by GPT-5 on 2026-08-21, 2026-08-23
// Modified by Opus 5 on 2026-08-26
// Modified by GPT-6 on 2026-09-14, 2026-09-23
//

#pragma once
#include "ExportProgress.hpp"
#include <atomic>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <windows.h>

namespace merutilm::rff2 {
    struct RenderSceneRequests {
        struct CreateImageRequest {
            std::filesystem::path filename;
            bool downsample = true;
            std::shared_ptr<ExportProgress> progress;
        };

        std::atomic<bool> defaultAttrRequested = false;
        std::atomic<bool> recomputeRequested = false;
        std::atomic<bool> resizeRequested = false;
        std::atomic<bool> shaderRequested = false;
        std::atomic<bool> createImageRequested = false;
        std::mutex createImageMutex;
        std::deque<CreateImageRequest> pendingCreateImages;


        void requestDefaultSettings() {
            defaultAttrRequested = true;
        }

        // A window told whenever a shader re-render is asked for, so an editor watching the shader
        // attribute can see that a settings panel has changed it. The Timeline Editor records keys
        // off this. Null while nobody is watching, and posted to rather than called, so the message
        // is handled by the window's own thread whichever thread asked.
        static constexpr UINT WM_SHADER_EDITED = WM_APP + 0x253;
        std::atomic<HWND> shaderEditListener = nullptr;

        void requestShader() {
            shaderRequested = true;
            if (const HWND listener = shaderEditListener.load(std::memory_order_acquire);
                listener != nullptr) {
                PostMessageW(listener, WM_SHADER_EDITED, 0, 0);
            }
        }

        void requestResize() {
            resizeRequested = true;
        }

        void requestRecompute() {
            recomputeRequested = true;
        }

        void requestCreateImage(const std::filesystem::path &filename = {},
                                const bool downsample = true,
                                const std::shared_ptr<ExportProgress> &progress = {}) {
            std::scoped_lock lock(createImageMutex);
            pendingCreateImages.push_back(CreateImageRequest{filename, downsample, progress});
            createImageRequested.store(true, std::memory_order_release);
        }

        std::optional<CreateImageRequest> takeCreateImageRequest() {
            if (!createImageRequested.load(std::memory_order_acquire)) {
                return std::nullopt;
            }
            std::scoped_lock lock(createImageMutex);
            if (pendingCreateImages.empty()) {
                return std::nullopt;
            }
            CreateImageRequest request = std::move(pendingCreateImages.front());
            pendingCreateImages.pop_front();
            return request;
        }

        // The flag stays set for the active image until completion updates it from the queue.
        void completeCreateImageRequest() {
            std::scoped_lock lock(createImageMutex);
            createImageRequested.store(!pendingCreateImages.empty(), std::memory_order_release);
        }

    };
}
