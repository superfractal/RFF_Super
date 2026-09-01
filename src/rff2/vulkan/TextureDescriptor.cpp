//
// Created by Opus 5 on 2026-08-05
// Modified by Opus 5 on 2026-08-07, 2026-08-13, 2026-08-15, 2026-08-17, 2026-08-18, 2026-08-27
//

#include "TextureDescriptor.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <vector>

#include "../../vulkan_helper/core/logger.hpp"
#include "../../vulkan_helper/impl/Uniform.hpp"
#include "../../vulkan_helper/util/BufferImageContextUtils.hpp"

namespace merutilm::rff2::TextureDescriptor {
    namespace {
        constexpr VkFormat TEXTURE_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;

        // stbi_load takes a narrow path, which mangles non-ASCII names on Windows, so the UTF-8 path
        // is widened through std::filesystem and the image is decoded from memory instead.
        std::vector<std::byte> readFileBytes(const std::string &path) {
            const std::filesystem::path fsPath{
                std::u8string(reinterpret_cast<const char8_t *>(path.data()), path.size())
            };
            std::ifstream in(fsPath, std::ios::in | std::ios::binary | std::ios::ate);
            if (!in.is_open()) {
                return {};
            }
            const auto size = static_cast<std::streamsize>(in.tellg());
            if (size <= 0) {
                return {};
            }
            in.seekg(0, std::ios::beg);
            std::vector<std::byte> bytes(static_cast<size_t>(size));
            in.read(reinterpret_cast<char *>(bytes.data()), size);
            if (in.gcount() != size) {
                return {};
            }
            return bytes;
        }
    }

    vkh::DescriptorManager createManager(const vkh::CoreRef core, const vkh::SamplerRef sampler,
                                         const VkShaderStageFlags stage) {
        auto descManager = vkh::factory::create<vkh::DescriptorManager>();
        for (uint32_t layer = 0; layer < TEXTURE_LAYER_COUNT; ++layer) {
            descManager->appendCombinedImgSampler(samplerBinding(layer), stage,
                                                  vkh::factory::create<vkh::CombinedImageSampler>(
                                                      core, sampler, false));
        }

        auto bufferManager = vkh::factory::create<vkh::HostDataObjectManager>();
        bufferManager->reserve<uint32_t>(TARGET_TEXTURE_ENABLED);
        bufferManager->reserve<uint32_t>(TARGET_TEXTURE_UV_MODE);
        bufferManager->reserve<uint32_t>(TARGET_TEXTURE_BLEND_MODE);
        bufferManager->reserve<float>(TARGET_TEXTURE_OPACITY);
        bufferManager->reserve<float>(TARGET_TEXTURE_SCALE_U);
        bufferManager->reserve<float>(TARGET_TEXTURE_SCALE_V);
        bufferManager->reserve<float>(TARGET_TEXTURE_SCROLL_U);
        bufferManager->reserve<float>(TARGET_TEXTURE_SCROLL_V);
        bufferManager->reserve<float>(TARGET_TEXTURE_PALETTE_FOLLOW);
        bufferManager->reserve<float>(TARGET_TEXTURE_PERIOD);
        bufferManager->reserve<uint32_t>(TARGET_PATTERN_ENABLED);
        bufferManager->reserve<uint32_t>(TARGET_PATTERN_TYPE);
        bufferManager->reserve<uint32_t>(TARGET_PATTERN_UV_MODE);
        bufferManager->reserve<uint32_t>(TARGET_PATTERN_BLEND_MODE);
        bufferManager->reserve<float>(TARGET_PATTERN_OPACITY);
        bufferManager->reserve<float>(TARGET_PATTERN_SCALE_U);
        bufferManager->reserve<float>(TARGET_PATTERN_SCALE_V);
        bufferManager->reserve<float>(TARGET_PATTERN_SCROLL_U);
        bufferManager->reserve<float>(TARGET_PATTERN_SCROLL_V);
        bufferManager->reserve<float>(TARGET_PATTERN_PALETTE_FOLLOW);
        bufferManager->reserve<float>(TARGET_PATTERN_PERIOD);
        bufferManager->reserve<float>(TARGET_PATTERN_SHARPNESS);
        bufferManager->reserve<float>(TARGET_PATTERN_COLOR_R);
        bufferManager->reserve<float>(TARGET_PATTERN_COLOR_G);
        bufferManager->reserve<float>(TARGET_PATTERN_COLOR_B);
        bufferManager->reserve<uint32_t>(TARGET_PATTERN_INK_MODE);
        bufferManager->reserve<float>(TARGET_PATTERN_PALETTE_SHIFT);
        // Layers 1 and up, appended behind the pattern block in the same field order as layer 0.
        for (uint32_t layer = 1; layer < TEXTURE_LAYER_COUNT; ++layer) {
            bufferManager->reserve<uint32_t>(textureTarget(layer, TARGET_TEXTURE_ENABLED));
            bufferManager->reserve<uint32_t>(textureTarget(layer, TARGET_TEXTURE_UV_MODE));
            bufferManager->reserve<uint32_t>(textureTarget(layer, TARGET_TEXTURE_BLEND_MODE));
            bufferManager->reserve<float>(textureTarget(layer, TARGET_TEXTURE_OPACITY));
            bufferManager->reserve<float>(textureTarget(layer, TARGET_TEXTURE_SCALE_U));
            bufferManager->reserve<float>(textureTarget(layer, TARGET_TEXTURE_SCALE_V));
            bufferManager->reserve<float>(textureTarget(layer, TARGET_TEXTURE_SCROLL_U));
            bufferManager->reserve<float>(textureTarget(layer, TARGET_TEXTURE_SCROLL_V));
            bufferManager->reserve<float>(textureTarget(layer, TARGET_TEXTURE_PALETTE_FOLLOW));
            bufferManager->reserve<float>(textureTarget(layer, TARGET_TEXTURE_PERIOD));
        }
        // Domain warp, appended behind the last layer.
        bufferManager->reserve<uint32_t>(TARGET_WARP_ENABLED);
        bufferManager->reserve<uint32_t>(TARGET_WARP_SOURCE);
        bufferManager->reserve<uint32_t>(TARGET_WARP_UV_MODE);
        bufferManager->reserve<float>(TARGET_WARP_AMOUNT);
        bufferManager->reserve<float>(TARGET_WARP_OCTAVES);
        bufferManager->reserve<float>(TARGET_WARP_SCALE_U);
        bufferManager->reserve<float>(TARGET_WARP_SCALE_V);
        bufferManager->reserve<float>(TARGET_WARP_SCROLL_U);
        bufferManager->reserve<float>(TARGET_WARP_SCROLL_V);
        bufferManager->reserve<float>(TARGET_WARP_PALETTE_FOLLOW);
        bufferManager->reserve<float>(TARGET_WARP_PERIOD);
        // Pattern outline, appended behind the warp so the offsets above it are untouched.
        bufferManager->reserve<uint32_t>(TARGET_PATTERN_EDGE_ENABLED);
        bufferManager->reserve<float>(TARGET_PATTERN_EDGE_COLOR_R);
        bufferManager->reserve<float>(TARGET_PATTERN_EDGE_COLOR_G);
        bufferManager->reserve<float>(TARGET_PATTERN_EDGE_COLOR_B);
        bufferManager->reserve<float>(TARGET_PATTERN_EDGE_WIDTH);
        bufferManager->reserve<float>(TARGET_PATTERN_EDGE_OPACITY);
        bufferManager->reserve<uint32_t>(TARGET_PATTERN_EDGE_RELATIVE);
        // Pattern layers 1 and up, appended behind everything else. Unlike layer 0 each is one
        // contiguous run, so the shape and the outline of one layer sit together.
        for (uint32_t layer = 1; layer < PATTERN_LAYER_COUNT; ++layer) {
            bufferManager->reserve<uint32_t>(patternTarget(layer, TARGET_PATTERN_F_ENABLED));
            bufferManager->reserve<uint32_t>(patternTarget(layer, TARGET_PATTERN_F_TYPE));
            bufferManager->reserve<uint32_t>(patternTarget(layer, TARGET_PATTERN_F_UV_MODE));
            bufferManager->reserve<uint32_t>(patternTarget(layer, TARGET_PATTERN_F_BLEND_MODE));
            bufferManager->reserve<float>(patternTarget(layer, TARGET_PATTERN_F_OPACITY));
            bufferManager->reserve<float>(patternTarget(layer, TARGET_PATTERN_F_SCALE_U));
            bufferManager->reserve<float>(patternTarget(layer, TARGET_PATTERN_F_SCALE_V));
            bufferManager->reserve<float>(patternTarget(layer, TARGET_PATTERN_F_SCROLL_U));
            bufferManager->reserve<float>(patternTarget(layer, TARGET_PATTERN_F_SCROLL_V));
            bufferManager->reserve<float>(patternTarget(layer, TARGET_PATTERN_F_PALETTE_FOLLOW));
            bufferManager->reserve<float>(patternTarget(layer, TARGET_PATTERN_F_PERIOD));
            bufferManager->reserve<float>(patternTarget(layer, TARGET_PATTERN_F_SHARPNESS));
            bufferManager->reserve<float>(patternTarget(layer, TARGET_PATTERN_F_COLOR_R));
            bufferManager->reserve<float>(patternTarget(layer, TARGET_PATTERN_F_COLOR_G));
            bufferManager->reserve<float>(patternTarget(layer, TARGET_PATTERN_F_COLOR_B));
            bufferManager->reserve<uint32_t>(patternTarget(layer, TARGET_PATTERN_F_INK_MODE));
            bufferManager->reserve<float>(patternTarget(layer, TARGET_PATTERN_F_PALETTE_SHIFT));
            bufferManager->reserve<uint32_t>(patternTarget(layer, TARGET_PATTERN_F_EDGE_ENABLED));
            bufferManager->reserve<float>(patternTarget(layer, TARGET_PATTERN_F_EDGE_COLOR_R));
            bufferManager->reserve<float>(patternTarget(layer, TARGET_PATTERN_F_EDGE_COLOR_G));
            bufferManager->reserve<float>(patternTarget(layer, TARGET_PATTERN_F_EDGE_COLOR_B));
            bufferManager->reserve<float>(patternTarget(layer, TARGET_PATTERN_F_EDGE_WIDTH));
            bufferManager->reserve<float>(patternTarget(layer, TARGET_PATTERN_F_EDGE_OPACITY));
            bufferManager->reserve<uint32_t>(patternTarget(layer, TARGET_PATTERN_F_EDGE_RELATIVE));
        }
        // Every layer's Size and Keep Aspect, appended behind everything else.
        for (uint32_t layer = 0; layer < TEXTURE_LAYER_COUNT; ++layer) {
            bufferManager->reserve<float>(textureTarget(layer, TARGET_TEXTURE_SIZE));
            bufferManager->reserve<uint32_t>(textureTarget(layer, TARGET_TEXTURE_KEEP_ASPECT));
        }
        auto ubo = vkh::factory::create<vkh::Uniform>(core, std::move(bufferManager), vkh::BufferLock::LOCK_UNLOCK,
                                                      false);
        descManager->appendUBO(BINDING_UBO_TEXTURE, stage, std::move(ubo));
        return descManager;
    }

    void uploadPlaceholder(const vkh::CoreRef core, const vkh::CommandPoolRef commandPool, const vkh::DescriptorRef desc,
                           const uint32_t layer) {
        constexpr std::array<std::byte, 4> white = {
            std::byte{255}, std::byte{255}, std::byte{255}, std::byte{255}
        };
        const auto ctx = vkh::BufferImageContextUtils::imageFromByteColorArray(
            core, commandPool, TEXTURE_FORMAT, 1, 1, 4, 8, false, white.data());
        desc.get<vkh::CombinedImageSampler>(0, samplerBinding(layer))->setUniqueImageContext(ctx);
    }

    bool uploadImage(const vkh::CoreRef core, const vkh::CommandPoolRef commandPool, const vkh::DescriptorRef desc,
                     const uint32_t layer, const std::string &path, std::string &cachedPath) {
        if (path == cachedPath) {
            return false;
        }

        if (path.empty()) {
            uploadPlaceholder(core, commandPool, desc, layer);
            cachedPath.clear();
            return true;
        }

        const std::vector<std::byte> bytes = readFileBytes(path);
        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc *pixels = bytes.empty()
                              ? nullptr
                              : stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(bytes.data()),
                                                      static_cast<int>(bytes.size()), &width, &height, &channels,
                                                      STBI_rgb_alpha);
        if (pixels == nullptr || width <= 0 || height <= 0) {
            if (pixels != nullptr) {
                stbi_image_free(pixels);
            }
            vkh::logger::w_log(L"ERROR : Cannot load texture image");
            uploadPlaceholder(core, commandPool, desc, layer);
            cachedPath.clear();
            return true;
        }

        const auto ctx = vkh::BufferImageContextUtils::imageFromByteColorArray(
            core, commandPool, TEXTURE_FORMAT, static_cast<uint32_t>(width), static_cast<uint32_t>(height), 4, 8,
            false, reinterpret_cast<std::byte *>(pixels));
        stbi_image_free(pixels);
        desc.get<vkh::CombinedImageSampler>(0, samplerBinding(layer))->setUniqueImageContext(ctx);
        cachedPath = path;
        return true;
    }

    void updateParams(const vkh::DescriptorRef desc, const uint32_t layer, const ShdTextureAttribute &texture,
                      const bool textureReady) {
        const auto &ubo = *desc.get<vkh::Uniform>(0, BINDING_UBO_TEXTURE);
        auto &host = ubo.getHostObject();
        host.set<uint32_t>(textureTarget(layer, TARGET_TEXTURE_ENABLED),
                           texture.enabled && textureReady ? 1u : 0u);
        host.set<uint32_t>(textureTarget(layer, TARGET_TEXTURE_UV_MODE), static_cast<uint32_t>(texture.uvMode));
        host.set<uint32_t>(textureTarget(layer, TARGET_TEXTURE_BLEND_MODE),
                           static_cast<uint32_t>(texture.blendMode));
        host.set<float>(textureTarget(layer, TARGET_TEXTURE_OPACITY), texture.opacity);
        host.set<float>(textureTarget(layer, TARGET_TEXTURE_SCALE_U), texture.scaleU);
        host.set<float>(textureTarget(layer, TARGET_TEXTURE_SCALE_V), texture.scaleV);
        host.set<float>(textureTarget(layer, TARGET_TEXTURE_SCROLL_U), texture.scrollU);
        host.set<float>(textureTarget(layer, TARGET_TEXTURE_SCROLL_V), texture.scrollV);
        host.set<float>(textureTarget(layer, TARGET_TEXTURE_PALETTE_FOLLOW), texture.paletteFollow);
        host.set<float>(textureTarget(layer, TARGET_TEXTURE_PERIOD), texture.periodIterations);
        host.set<float>(textureTarget(layer, TARGET_TEXTURE_SIZE), texture.size);
        host.set<uint32_t>(textureTarget(layer, TARGET_TEXTURE_KEEP_ASPECT), texture.keepAspect ? 1u : 0u);
        ubo.update();
    }

    void updatePatternParams(const vkh::DescriptorRef desc, const uint32_t layer,
                             const ShdPatternAttribute &pattern) {
        const auto &ubo = *desc.get<vkh::Uniform>(0, BINDING_UBO_TEXTURE);
        auto &host = ubo.getHostObject();
        auto u32 = [&host, layer](const uint32_t field, const uint32_t value) {
            host.set<uint32_t>(patternTarget(layer, field), value);
        };
        auto f32 = [&host, layer](const uint32_t field, const float value) {
            host.set<float>(patternTarget(layer, field), value);
        };
        u32(TARGET_PATTERN_F_ENABLED, pattern.enabled ? 1u : 0u);
        u32(TARGET_PATTERN_F_TYPE, static_cast<uint32_t>(pattern.type));
        u32(TARGET_PATTERN_F_UV_MODE, static_cast<uint32_t>(pattern.uvMode));
        u32(TARGET_PATTERN_F_BLEND_MODE, static_cast<uint32_t>(pattern.blendMode));
        f32(TARGET_PATTERN_F_OPACITY, pattern.opacity);
        f32(TARGET_PATTERN_F_SCALE_U, pattern.scaleU);
        f32(TARGET_PATTERN_F_SCALE_V, pattern.scaleV);
        f32(TARGET_PATTERN_F_SCROLL_U, pattern.scrollU);
        f32(TARGET_PATTERN_F_SCROLL_V, pattern.scrollV);
        f32(TARGET_PATTERN_F_PALETTE_FOLLOW, pattern.paletteFollow);
        f32(TARGET_PATTERN_F_PERIOD, pattern.periodIterations);
        f32(TARGET_PATTERN_F_SHARPNESS, pattern.sharpness);
        f32(TARGET_PATTERN_F_COLOR_R, pattern.color.r);
        f32(TARGET_PATTERN_F_COLOR_G, pattern.color.g);
        f32(TARGET_PATTERN_F_COLOR_B, pattern.color.b);
        u32(TARGET_PATTERN_F_INK_MODE, static_cast<uint32_t>(pattern.inkMode));
        f32(TARGET_PATTERN_F_PALETTE_SHIFT, pattern.paletteShift);
        u32(TARGET_PATTERN_F_EDGE_ENABLED, pattern.edgeEnabled ? 1u : 0u);
        f32(TARGET_PATTERN_F_EDGE_COLOR_R, pattern.edgeColor.r);
        f32(TARGET_PATTERN_F_EDGE_COLOR_G, pattern.edgeColor.g);
        f32(TARGET_PATTERN_F_EDGE_COLOR_B, pattern.edgeColor.b);
        f32(TARGET_PATTERN_F_EDGE_WIDTH, pattern.edgeWidth);
        f32(TARGET_PATTERN_F_EDGE_OPACITY, pattern.edgeOpacity);
        u32(TARGET_PATTERN_F_EDGE_RELATIVE, pattern.edgeRelative ? 1u : 0u);
        ubo.update();
    }

    void updateWarpParams(const vkh::DescriptorRef desc, const ShdWarpAttribute &warp, const bool sourceReady) {
        const auto &ubo = *desc.get<vkh::Uniform>(0, BINDING_UBO_TEXTURE);
        auto &host = ubo.getHostObject();
        host.set<uint32_t>(TARGET_WARP_ENABLED, warp.enabled && sourceReady ? 1u : 0u);
        host.set<uint32_t>(TARGET_WARP_SOURCE, static_cast<uint32_t>(warp.source));
        host.set<uint32_t>(TARGET_WARP_UV_MODE, static_cast<uint32_t>(warp.uvMode));
        host.set<float>(TARGET_WARP_AMOUNT, warp.amount);
        host.set<float>(TARGET_WARP_OCTAVES, warp.octaves);
        host.set<float>(TARGET_WARP_SCALE_U, warp.scaleU);
        host.set<float>(TARGET_WARP_SCALE_V, warp.scaleV);
        host.set<float>(TARGET_WARP_SCROLL_U, warp.scrollU);
        host.set<float>(TARGET_WARP_SCROLL_V, warp.scrollV);
        host.set<float>(TARGET_WARP_PALETTE_FOLLOW, warp.paletteFollow);
        host.set<float>(TARGET_WARP_PERIOD, warp.periodIterations);
        ubo.update();
    }
}
