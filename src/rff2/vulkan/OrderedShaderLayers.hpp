//
// Modified by GPT-6 on 2026-09-16, 2026-09-17, 2026-09-19, 2026-09-20, 2026-09-21, 2026-09-23
//

#pragma once
#include "ShaderLayerControl.hpp"
#include "../../vulkan_helper/util/BarrierUtils.hpp"

namespace merutilm::rff2 {
    static_assert(uint32_t(ShdLayer::TEXTURE_4) - uint32_t(ShdLayer::TEXTURE_1) + 1 == TEXTURE_LAYER_COUNT);
    static_assert(uint32_t(ShdLayer::PATTERN_4) - uint32_t(ShdLayer::PATTERN_1) + 1 == PATTERN_LAYER_COUNT);
    static_assert(uint32_t(ShdLayer::EFFECT_4) - uint32_t(ShdLayer::EFFECT_1) + 1 == EFFECT_LAYER_COUNT);

    inline bool shaderLayerActive(const ShaderAttribute &shader, ShdLayer layer,
                                  bool respectVisibility = true) {
        if (respectVisibility && shader.layerOrder.enabled && !shader.layerOrder.visible(layer)) {
            return false;
        }
        const auto id = uint32_t(layer);
        if (layer >= ShdLayer::TEXTURE_1 && layer <= ShdLayer::TEXTURE_4) {
            const auto &texture = shader.textures[id - uint32_t(ShdLayer::TEXTURE_1)];
            return texture.enabled && texture.opacity > 0;
        }
        if (layer >= ShdLayer::PATTERN_1 && layer <= ShdLayer::PATTERN_4) {
            const auto &pattern = shader.patterns[id - uint32_t(ShdLayer::PATTERN_1)];
            return pattern.enabled && pattern.opacity > 0;
        }
        if (layer >= ShdLayer::EFFECT_1 && layer <= ShdLayer::EFFECT_4) {
            const auto &effect = shader.effects[id - uint32_t(ShdLayer::EFFECT_1)];
            return effect.enabled && effect.opacity > 0;
        }
        const auto &slope = shader.slope;
        const bool material =
            slope.studio.use && slope.depth != 0 && slope.opacity > 0 && slope.chromeStrength > 0;
        switch (layer) {
        case ShdLayer::BAND_LINE:
            return shader.palette.bandLineEnabled;
        case ShdLayer::STRIPE:
            return shader.stripe.stripeType != ShdStripeType::NONE && shader.stripe.opacity > 0;
        case ShdLayer::SURFACE:
            return (slope.depth != 0 && slope.opacity > 0) ||
                   (slope.studio.use && slope.replaceSurfaceStyle &&
                    slope.surfaceStyle != ShdSurfaceStyle::ORIGINAL);
        case ShdLayer::METAL:
            return material && slope.layerMetal > 0;
        case ShdLayer::SIGIL:
            return material && slope.layerSigil > 0;
        case ShdLayer::PHONK:
            return material && slope.layerPhonk > 0;
        case ShdLayer::FROST:
            return material && slope.layerFrost > 0;
        case ShdLayer::SEA:
            return material && slope.layerSea > 0;
        case ShdLayer::PRINT:
            return material && slope.layerPrint > 0;
        case ShdLayer::SURFACE_COLOR:
            return material;
        case ShdLayer::COLOR:
            return true;
        case ShdLayer::FOG:
            return shader.fog.opacity > 0 || shader.fog.focusAmount > 0 || shader.fog.chaosAmount > 0;
        case ShdLayer::BLOOM:
            return shader.bloom.intensity != 0;
        case ShdLayer::TONE_MAP:
            return true;
        case ShdLayer::PRINT_FINISH:
            return slope.layerQuantize > 0;
        case ShdLayer::VHS:
            return slope.layerVhs > 0;
        case ShdLayer::MONO:
            return slope.layerMono > 0;
        default:
            return false;
        }
    }

    // Copy between the existing full-size buffers so every movable layer starts from PRIMARY.
    inline void copyShaderLayerImage(VkCommandBuffer command, const vkh::ImageContext &source,
                                     const vkh::ImageContext &destination) {
        vkh::BarrierUtils::cmdImageMemoryBarrier(
            command, source.image, VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,
            VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 1, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkh::BarrierUtils::cmdImageMemoryBarrier(
            command, destination.image, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
            1, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        const VkImageCopy region{{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
                                 {0, 0, 0},
                                 {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
                                 {0, 0, 0},
                                 {source.extent.width, source.extent.height, 1}};
        vkCmdCopyImage(command, source.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination.image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        vkh::BarrierUtils::cmdImageMemoryBarrier(
            command, source.image, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        vkh::BarrierUtils::cmdImageMemoryBarrier(
            command, destination.image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }
} // namespace merutilm::rff2
