//
// Created by Merutilm on 2025-07-09.
// Modified by GPT-6 on 2026-09-22
//

#pragma once

#include "../manage/PipelineLayoutManager.hpp"
#include "VariantHasher.hpp"
#include "VectorHasher.hpp"


namespace merutilm::vkh {

    struct PipelineLayoutBuilderHasher{
        using is_transparent = void;

        size_t operator()(const PipelineLayoutBuilder &key) const {
            size_t seed = 0;
            constexpr auto descHasher = VectorHasher<PipelineLayoutBuildType, VariantHasher>();
            BoostHasher::hash(descHasher(key), &seed);
            return seed;
        }
    };
}
