//
// Created by Merutilm on 2025-08-13.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <functional>

#include "../core/vkh_core.hpp"
#include "Repository.hpp"
#include "../impl/Sampler.hpp"
#include "../hash/SamplerCreateInfoEquals.hpp"
#include "../hash/SamplerCreateInfoHasher.hpp"

namespace merutilm::vkh {

    struct GlobalSamplerRepo final
        : public Repository<VkSamplerCreateInfo, const VkSamplerCreateInfo &, Sampler, SamplerRef,
                            SamplerCreateInfoHasher, SamplerCreateInfoEquals> {
        using Repository::Repository;

        SamplerRef pick(const VkSamplerCreateInfo &samplerCreateInfo) override {
            auto it = repository.find(samplerCreateInfo);
            if (it == repository.end()) {
                it = repository.try_emplace(
                    samplerCreateInfo,
                    factory::create<Sampler>(core, samplerCreateInfo)).first;
            }
            return *it->second;
        }
    };
}
