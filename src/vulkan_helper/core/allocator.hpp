//
// Created by Merutilm on 2025-09-09.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <mutex>
#include <utility>

namespace merutilm::vkh {
    struct allocator {
        inline static std::mutex mutex;

        explicit allocator() = delete;

        template<typename Function, typename... Args>
        static auto invoke(Function function, Args &&... args)
            -> decltype(function(std::forward<Args>(args)...)) {
            std::scoped_lock lock(mutex);
            return function(std::forward<Args>(args)...);
        }
    };
}
