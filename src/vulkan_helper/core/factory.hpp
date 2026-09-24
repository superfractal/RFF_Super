//
// Created by Merutilm on 2025-08-16.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <memory>
#include <utility>

namespace merutilm::vkh {
    class factory {
        template<typename T>
        struct unique_pointer_type_getter {
            using type = T;
        };

        template<typename T>
        struct unique_pointer_type_getter<std::unique_ptr<T>> {
            using type = T;
        };

    public:
        template<typename Ptr, typename... Args>
        static Ptr create(Args &&... args) {
            using Object = typename unique_pointer_type_getter<Ptr>::type;
            return std::make_unique<Object>(std::forward<Args>(args)...);
        }
    };
}
