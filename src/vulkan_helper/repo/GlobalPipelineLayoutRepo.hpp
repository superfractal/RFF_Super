//
// Created by Merutilm on 2025-07-18.
// Modified by GPT-6 on 2026-09-23
//

#pragma once
#include <functional>
#include <utility>

#include "Repository.hpp"
#include "../hash/PipelineLayoutBuilderHasher.hpp"
#include "../manage/PipelineLayoutManager.hpp"
#include "../impl/PipelineLayout.hpp"

namespace merutilm::vkh {
    struct GlobalPipelineLayoutRepo final
        : Repository<PipelineLayoutBuilder, PipelineLayoutManager &&, PipelineLayout,
                     PipelineLayoutRef, PipelineLayoutBuilderHasher, std::equal_to<>> {
        using Repository::Repository;

        PipelineLayoutRef pick(PipelineLayoutManager &&layoutManager) override {
            const PipelineLayoutBuilder builder = layoutManager->builders; //clone the builder
            auto it = repository.find(builder);
            if (it == repository.end()) {
                it = repository.try_emplace(
                    builder, factory::create<PipelineLayout>(core, std::move(layoutManager))).first;
            }

            return *it->second;
        }
    };
}
