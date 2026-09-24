//
// Created by Merutilm on 2025-08-10.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <concepts>
#include <cstdint>
#include <type_traits>
#include <vector>

#include "DescriptorTemplateInfo.hpp"
#include "../manage/DescriptorManager.hpp"

namespace merutilm::vkh {
    struct DescriptorTemplate;

    template<typename T>
    concept DescTemplateHasID =
        std::is_base_of_v<DescriptorTemplate, T> &&
        std::default_initializable<T> &&
        requires {
            { T::ID } -> std::convertible_to<uint32_t>;
        };

    struct DescriptorTemplate {
        virtual ~DescriptorTemplate() = default;

        virtual void configure(CoreRef core, std::vector<DescriptorManager> &managers) = 0;

        template<DescTemplateHasID D>
        static DescriptorTemplateInfo from() {
            return DescriptorTemplateInfo{
                .id = D::ID,
                .descriptorGenerator = [](CoreRef core) {
                    std::vector<DescriptorManager> descriptorManagers = {};
                    auto descriptorTemplate = D();
                    descriptorTemplate.configure(core, descriptorManagers);
                    return descriptorManagers;
                }
            };
        }
    };
}
