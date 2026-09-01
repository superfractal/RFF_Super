//
// Created by Merutilm on 2025-07-19.
// Modified by Opus 5 on 2026-08-26
//

#pragma once
#include "../core/vkh_base.hpp"

#include "Core.hpp"
#include "../context/WindowContext.hpp"

namespace merutilm::vkh {
    class EngineImpl final : public Handler {
        Core core = nullptr;
        Repositories globalRepositories = nullptr;
        std::vector<WindowContext> windowContexts = {};
        // A video export builds its own window context on its worker thread while the main thread
        // is still drawing off this list every frame. Attaching moves the list itself, so reading
        // it unguarded means dereferencing storage that has just been freed under the reader. Only
        // the list is guarded; a context handed out stays put, since the list holds pointers to it.
        mutable std::mutex windowContextsMutex = {};

    public:
        explicit EngineImpl(Core &&core);

        ~EngineImpl() override;

        bool isValidWindowContext(uint32_t windowAttachmentIndex) const;

        EngineImpl(const EngineImpl &) = delete;

        EngineImpl &operator=(const EngineImpl &) = delete;

        EngineImpl(EngineImpl &&) = delete;

        EngineImpl &operator=(EngineImpl &&) = delete;

        [[nodiscard]] WindowContextPtr attachWindowContext(HWND hwnd, uint32_t windowAttachmentIndexExpected);

        void detachWindowContext(uint32_t windowAttachmentIndex);

        [[nodiscard]] CoreRef getCore() const { return *core; }

        [[nodiscard]] WindowContextRef getWindowContext(const uint32_t windowContextIndex) const {
            std::scoped_lock lock(windowContextsMutex);
            return *windowContexts.at(windowContextIndex);
        }

        [[nodiscard]] RepositoriesRef getGlobalRepositories() const {
            return *globalRepositories;
        }

    private:
        void init() override;

        void configureRepositories() const;

        void destroy() override;
    };


    using Engine = std::unique_ptr<EngineImpl>;
    using EnginePtr = EngineImpl *;
    using EngineRef = EngineImpl &;
}
