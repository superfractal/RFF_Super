//
// Created by Merutilm on 2025-07-07.
// Modified by GPT-6 on 2026-09-14, 2026-09-17, 2026-09-22, 2026-09-23
//

#pragma once
#include "../core/config.hpp"
#include "../core/vkh_base.hpp"

namespace merutilm::vkh {

    class GraphicsContextWindowImpl;

    using GraphicsContextWindow = std::unique_ptr<GraphicsContextWindowImpl>;
    using GraphicsContextWindowPtr = GraphicsContextWindowImpl *;
    using GraphicsContextWindowRef = GraphicsContextWindowImpl &;

    class GraphicsContextWindowImpl final {
        using Listener = std::function<LRESULT(GraphicsContextWindowRef, HWND, WPARAM, LPARAM)>;
        using Listeners = std::unordered_map<UINT, Listener>;
        HWND window = nullptr;
        float framerate = config::INITIAL_FPS;
        Listeners listeners = {};
        std::vector<std::function<void()> > renderers = {};
        std::function<bool(const MSG&)> messageFilter;
        inline static thread_local bool pipelineCompilationPending = false;

    public:
        explicit GraphicsContextWindowImpl(HWND window);


        ~GraphicsContextWindowImpl() = default;

        GraphicsContextWindowImpl(GraphicsContextWindowImpl &) = delete;

        GraphicsContextWindowImpl(GraphicsContextWindowImpl &&) = delete;

        GraphicsContextWindowImpl &operator=(const GraphicsContextWindowImpl &) = delete;

        GraphicsContextWindowImpl &operator=(GraphicsContextWindowImpl &&) = delete;

        HWND getWindowHandle() const { return window; }

        static bool isPipelineCompilationPending() { return pipelineCompilationPending; }

        static void setPipelineCompilationPending(bool pending) { pipelineCompilationPending = pending; }

        void setFramerate(const float framerate) { this->framerate = framerate; }

        void renderOnce() const;

        void setMessageFilter(std::function<bool(const MSG &)> filter) {
            messageFilter = std::move(filter);
        }

        void dispatchMessage(const MSG& message) const;

        [[nodiscard]] bool isUnrenderable() const {
            RECT rect;
            GetClientRect(window, &rect);
            return !IsWindow(window) ||
                   !IsWindowVisible(window) ||
                   IsIconic(window) ||
                   rect.bottom - rect.top <= 0 ||
                   rect.right - rect.left <= 0;
        }

        template<typename F>
            requires std::is_invocable_r_v<LRESULT, F, GraphicsContextWindowRef, HWND, WPARAM, LPARAM>
        void setListener(UINT message, F &&func);

        template<typename F>
            requires std::is_invocable_r_v<void, F>
        void appendRenderer(F &&func);

        void start() const;

        const Listeners &getListeners() const { return listeners; }

    };

    template<typename F>
        requires std::is_invocable_r_v<LRESULT, F, GraphicsContextWindowRef, HWND, WPARAM, LPARAM>
    void GraphicsContextWindowImpl::setListener(const UINT message, F &&func) {
        listeners[message] = std::forward<F>(func);
    }

    template<typename F>
        requires std::is_invocable_r_v<void, F>
    void GraphicsContextWindowImpl::appendRenderer(F &&func) {
        renderers.emplace_back(std::forward<F>(func));
    }


}
