//
// Created by Merutilm on 2025-07-16.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <functional>

#include "../core/vkh_core.hpp"
#include "Repository.hpp"
#include "../impl/DescriptorSetLayout.hpp"

namespace merutilm::vkh {
    struct GlobalDescriptorSetLayoutRepo final
        : Repository<DescriptorSetLayoutBuilder, const DescriptorSetLayoutBuilder &,
                     DescriptorSetLayout, DescriptorSetLayoutRef,
                     DescriptorSetLayoutBuilderHasher, std::equal_to<>> {
        using Repository::Repository;

        DescriptorSetLayoutRef pick(const DescriptorSetLayoutBuilder &layoutBuilder) override {
            auto it = repository.find(layoutBuilder);
            if (it == repository.end()) {
                it = repository.try_emplace(
                    layoutBuilder,
                    factory::create<DescriptorSetLayout>(core, layoutBuilder)).first;
            }
            return *it->second;
        }
    };
}
