//
// Created by Opus 5 on 2026-08-05
// Modified by Opus 5 on 2026-08-07, 2026-08-13, 2026-08-15, 2026-08-17, 2026-08-18, 2026-08-27
//

#pragma once
#include <string>

#include "../attr/ShdPatternAttribute.h"
#include "../attr/ShdTextureAttribute.h"
#include "../attr/ShdWarpAttribute.h"
#include "../../vulkan_helper/core/factory.hpp"
#include "../../vulkan_helper/impl/Descriptor.hpp"
#include "../../vulkan_helper/manage/DescriptorManager.hpp"

namespace merutilm::rff2::TextureDescriptor {
    // Samplers and params share one descriptor set: the video compute pipeline already binds seven
    // sets, and maxBoundDescriptorSets is 8 on common hardware. Each layer takes a binding of its
    // own rather than one array binding, because the descriptor helper builds every binding with a
    // descriptorCount of 1.
    constexpr uint32_t BINDING_SAMPLER_0 = 0;
    constexpr uint32_t BINDING_UBO_TEXTURE = TEXTURE_LAYER_COUNT;

    // Binding holding layer's image sampler.
    constexpr uint32_t samplerBinding(const uint32_t layer) {
        return BINDING_SAMPLER_0 + layer;
    }

    // Field offsets within one layer's block, and the layout of layer 0's block in the UBO.
    constexpr uint32_t TARGET_TEXTURE_ENABLED = 0;
    constexpr uint32_t TARGET_TEXTURE_UV_MODE = 1;
    constexpr uint32_t TARGET_TEXTURE_BLEND_MODE = 2;
    constexpr uint32_t TARGET_TEXTURE_OPACITY = 3;
    constexpr uint32_t TARGET_TEXTURE_SCALE_U = 4;
    constexpr uint32_t TARGET_TEXTURE_SCALE_V = 5;
    constexpr uint32_t TARGET_TEXTURE_SCROLL_U = 6;
    constexpr uint32_t TARGET_TEXTURE_SCROLL_V = 7;
    constexpr uint32_t TARGET_TEXTURE_PALETTE_FOLLOW = 8;
    constexpr uint32_t TARGET_TEXTURE_PERIOD = 9;
    constexpr uint32_t TARGET_TEXTURE_FIELD_COUNT = 10;

    // The generated pattern rides in the same UBO for the same reason the sampler shares this set:
    // a set of its own would push the video compute pipeline past maxBoundDescriptorSets.
    constexpr uint32_t TARGET_PATTERN_ENABLED = 10;
    constexpr uint32_t TARGET_PATTERN_TYPE = 11;
    constexpr uint32_t TARGET_PATTERN_UV_MODE = 12;
    constexpr uint32_t TARGET_PATTERN_BLEND_MODE = 13;
    constexpr uint32_t TARGET_PATTERN_OPACITY = 14;
    constexpr uint32_t TARGET_PATTERN_SCALE_U = 15;
    constexpr uint32_t TARGET_PATTERN_SCALE_V = 16;
    constexpr uint32_t TARGET_PATTERN_SCROLL_U = 17;
    constexpr uint32_t TARGET_PATTERN_SCROLL_V = 18;
    constexpr uint32_t TARGET_PATTERN_PALETTE_FOLLOW = 19;
    constexpr uint32_t TARGET_PATTERN_PERIOD = 20;
    constexpr uint32_t TARGET_PATTERN_SHARPNESS = 21;
    constexpr uint32_t TARGET_PATTERN_COLOR_R = 22;
    constexpr uint32_t TARGET_PATTERN_COLOR_G = 23;
    constexpr uint32_t TARGET_PATTERN_COLOR_B = 24;
    constexpr uint32_t TARGET_PATTERN_INK_MODE = 25;
    constexpr uint32_t TARGET_PATTERN_PALETTE_SHIFT = 26;

    // Layers past the first are appended behind the pattern block, so layer 0 and the pattern keep
    // the UBO offsets they already had and only the shader's trailing fields are new.
    constexpr uint32_t TARGET_TEXTURE_LAYER_1_BASE = 27;

    // The domain warp rides the same UBO, appended behind the last texture layer for the same reason.
    constexpr uint32_t TARGET_WARP_BASE = TARGET_TEXTURE_LAYER_1_BASE
                                          + (TEXTURE_LAYER_COUNT - 1) * TARGET_TEXTURE_FIELD_COUNT;
    constexpr uint32_t TARGET_WARP_ENABLED = TARGET_WARP_BASE + 0;
    constexpr uint32_t TARGET_WARP_SOURCE = TARGET_WARP_BASE + 1;
    constexpr uint32_t TARGET_WARP_UV_MODE = TARGET_WARP_BASE + 2;
    constexpr uint32_t TARGET_WARP_AMOUNT = TARGET_WARP_BASE + 3;
    constexpr uint32_t TARGET_WARP_OCTAVES = TARGET_WARP_BASE + 4;
    constexpr uint32_t TARGET_WARP_SCALE_U = TARGET_WARP_BASE + 5;
    constexpr uint32_t TARGET_WARP_SCALE_V = TARGET_WARP_BASE + 6;
    constexpr uint32_t TARGET_WARP_SCROLL_U = TARGET_WARP_BASE + 7;
    constexpr uint32_t TARGET_WARP_SCROLL_V = TARGET_WARP_BASE + 8;
    constexpr uint32_t TARGET_WARP_PALETTE_FOLLOW = TARGET_WARP_BASE + 9;
    constexpr uint32_t TARGET_WARP_PERIOD = TARGET_WARP_BASE + 10;

    // The pattern's outline, appended behind the warp rather than beside the rest of the pattern
    // block, so every offset already in the UBO keeps the place the shaders read it from.
    constexpr uint32_t TARGET_PATTERN_EDGE_BASE = TARGET_WARP_PERIOD + 1;
    constexpr uint32_t TARGET_PATTERN_EDGE_ENABLED = TARGET_PATTERN_EDGE_BASE + 0;
    constexpr uint32_t TARGET_PATTERN_EDGE_COLOR_R = TARGET_PATTERN_EDGE_BASE + 1;
    constexpr uint32_t TARGET_PATTERN_EDGE_COLOR_G = TARGET_PATTERN_EDGE_BASE + 2;
    constexpr uint32_t TARGET_PATTERN_EDGE_COLOR_B = TARGET_PATTERN_EDGE_BASE + 3;
    constexpr uint32_t TARGET_PATTERN_EDGE_WIDTH = TARGET_PATTERN_EDGE_BASE + 4;
    constexpr uint32_t TARGET_PATTERN_EDGE_OPACITY = TARGET_PATTERN_EDGE_BASE + 5;
    constexpr uint32_t TARGET_PATTERN_EDGE_RELATIVE = TARGET_PATTERN_EDGE_BASE + 6;

    // One pattern layer's fields, in the order every layer past the first is laid out in. Layer 0
    // is the odd one: its shape block and its outline block sit at either end of the UBO, because
    // each was appended when it was added rather than beside the other. patternTarget() below is
    // what hides that, so a caller only ever names a layer and a field.
    constexpr uint32_t TARGET_PATTERN_F_ENABLED = 0;
    constexpr uint32_t TARGET_PATTERN_F_TYPE = 1;
    constexpr uint32_t TARGET_PATTERN_F_UV_MODE = 2;
    constexpr uint32_t TARGET_PATTERN_F_BLEND_MODE = 3;
    constexpr uint32_t TARGET_PATTERN_F_OPACITY = 4;
    constexpr uint32_t TARGET_PATTERN_F_SCALE_U = 5;
    constexpr uint32_t TARGET_PATTERN_F_SCALE_V = 6;
    constexpr uint32_t TARGET_PATTERN_F_SCROLL_U = 7;
    constexpr uint32_t TARGET_PATTERN_F_SCROLL_V = 8;
    constexpr uint32_t TARGET_PATTERN_F_PALETTE_FOLLOW = 9;
    constexpr uint32_t TARGET_PATTERN_F_PERIOD = 10;
    constexpr uint32_t TARGET_PATTERN_F_SHARPNESS = 11;
    constexpr uint32_t TARGET_PATTERN_F_COLOR_R = 12;
    constexpr uint32_t TARGET_PATTERN_F_COLOR_G = 13;
    constexpr uint32_t TARGET_PATTERN_F_COLOR_B = 14;
    constexpr uint32_t TARGET_PATTERN_F_INK_MODE = 15;
    constexpr uint32_t TARGET_PATTERN_F_PALETTE_SHIFT = 16;
    constexpr uint32_t TARGET_PATTERN_F_EDGE_ENABLED = 17;
    constexpr uint32_t TARGET_PATTERN_F_EDGE_COLOR_R = 18;
    constexpr uint32_t TARGET_PATTERN_F_EDGE_COLOR_G = 19;
    constexpr uint32_t TARGET_PATTERN_F_EDGE_COLOR_B = 20;
    constexpr uint32_t TARGET_PATTERN_F_EDGE_WIDTH = 21;
    constexpr uint32_t TARGET_PATTERN_F_EDGE_OPACITY = 22;
    constexpr uint32_t TARGET_PATTERN_F_EDGE_RELATIVE = 23;
    constexpr uint32_t TARGET_PATTERN_FIELD_COUNT = 24;

    // Pattern layers past the first, appended behind everything else for the same reason.
    constexpr uint32_t TARGET_PATTERN_LAYER_1_BASE = TARGET_PATTERN_EDGE_RELATIVE + 1;

    // Every layer's Size and Keep Aspect, appended behind everything else for the same reason: widening one layer's block would move every offset behind it.
    constexpr uint32_t TARGET_TEXTURE_EXT_BASE = TARGET_PATTERN_LAYER_1_BASE
                                                 + (PATTERN_LAYER_COUNT - 1) * TARGET_PATTERN_FIELD_COUNT;
    constexpr uint32_t TARGET_TEXTURE_EXT_FIELD_COUNT = 2;
    // Named past the end of one layer's block, so textureTarget() takes them like any other field.
    constexpr uint32_t TARGET_TEXTURE_SIZE = TARGET_TEXTURE_FIELD_COUNT + 0;
    constexpr uint32_t TARGET_TEXTURE_KEEP_ASPECT = TARGET_TEXTURE_FIELD_COUNT + 1;

    // UBO field index of one pattern layer's field, where field is one of the TARGET_PATTERN_F_*.
    constexpr uint32_t patternTarget(const uint32_t layer, const uint32_t field) {
        if (layer == 0) {
            return field < TARGET_PATTERN_F_EDGE_ENABLED
                       ? TARGET_PATTERN_ENABLED + field
                       : TARGET_PATTERN_EDGE_BASE + (field - TARGET_PATTERN_F_EDGE_ENABLED);
        }
        return TARGET_PATTERN_LAYER_1_BASE + (layer - 1) * TARGET_PATTERN_FIELD_COUNT + field;
    }

    // Layer 0's own two blocks have to stay where they are for older files to keep loading, so the
    // mapping above is only true while they are laid out exactly as they were.
    static_assert(patternTarget(0, TARGET_PATTERN_F_PALETTE_SHIFT) == TARGET_PATTERN_PALETTE_SHIFT);
    static_assert(patternTarget(0, TARGET_PATTERN_F_EDGE_RELATIVE) == TARGET_PATTERN_EDGE_RELATIVE);

    // UBO field index of one layer's field, where field is one of the TARGET_TEXTURE_* offsets.
    constexpr uint32_t textureTarget(const uint32_t layer, const uint32_t field) {
        if (field >= TARGET_TEXTURE_FIELD_COUNT) {
            return TARGET_TEXTURE_EXT_BASE + layer * TARGET_TEXTURE_EXT_FIELD_COUNT
                   + (field - TARGET_TEXTURE_FIELD_COUNT);
        }
        return layer == 0
                   ? field
                   : TARGET_TEXTURE_LAYER_1_BASE + (layer - 1) * TARGET_TEXTURE_FIELD_COUNT + field;
    }

    // Builds the unique descriptor set (one combined image sampler per layer + params UBO) for one pipeline.
    vkh::DescriptorManager createManager(vkh::CoreRef core, vkh::SamplerRef sampler, VkShaderStageFlags stage);

    // Binds an opaque white 1x1 texel so the layer's sampler is valid before any image is chosen.
    void uploadPlaceholder(vkh::CoreRef core, vkh::CommandPoolRef commandPool, vkh::DescriptorRef desc,
                           uint32_t layer);

    // Loads path into the layer's sampler when it differs from cachedPath. Returns true when the bound
    // image changed, so the caller knows whether the sampler binding needs rewriting. An empty or
    // unreadable path falls back to the placeholder and clears cachedPath.
    bool uploadImage(vkh::CoreRef core, vkh::CommandPoolRef commandPool, vkh::DescriptorRef desc,
                     uint32_t layer, const std::string &path, std::string &cachedPath);

    // Pushes one layer's scalar params. textureReady=false forces the shader's enable flag off.
    void updateParams(vkh::DescriptorRef desc, uint32_t layer, const ShdTextureAttribute &texture,
                      bool textureReady);

    // Pushes one pattern layer's params. Independent of the image, so it works with no file chosen.
    void updatePatternParams(vkh::DescriptorRef desc, uint32_t layer, const ShdPatternAttribute &pattern);

    // Pushes the domain warp's params. A layer source reads that layer's image whether or not the
    // layer itself is painting, so the warp works with a texture chosen but switched off.
    // sourceReady=false forces the shader's enable flag off: the sampler then holds the opaque white
    // placeholder, which the warp would read as its field at full strength and turn the whole
    // palette by, rather than as the absence of a field it is.
    void updateWarpParams(vkh::DescriptorRef desc, const ShdWarpAttribute &warp, bool sourceReady);
}
