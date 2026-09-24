//
// Created by Merutilm on 2025-07-13.
// Modified by GPT-6 on 2026-09-18, 2026-09-23
//

#pragma once
#include <memory>
#include <string>

#include "../core/vkh_base.hpp"
#include "../handle/CoreHandler.hpp"

namespace merutilm::vkh {
    class ShaderModuleImpl final : public CoreHandler {

        static constexpr auto SHADER_PATH_PREFIX = "../shaders/";

        VkShaderStageFlagBits shaderStage;
        VkShaderModule shaderModule = nullptr;
        const std::string filename;

    public:
        explicit ShaderModuleImpl(CoreRef core, const std::string &filename);

        ~ShaderModuleImpl() override;

        ShaderModuleImpl(const ShaderModuleImpl &) = delete;

        ShaderModuleImpl &operator=(const ShaderModuleImpl &) = delete;

        ShaderModuleImpl(ShaderModuleImpl &&) = delete;

        ShaderModuleImpl &operator=(ShaderModuleImpl &&) = delete;

        [[nodiscard]] VkShaderStageFlagBits getShaderStage() const { return shaderStage; }

        [[nodiscard]] VkShaderModule getShaderModuleHandle() const { return shaderModule; }

        [[nodiscard]] const std::string &getFilename() const { return filename; }

    private:
        [[nodiscard]] static VkShaderStageFlagBits getShaderStage(const std::string &filename);

        void init() override;

        void destroy() override;
    };

    
    using ShaderModule = std::unique_ptr<ShaderModuleImpl>;
    using ShaderModulePtr = ShaderModuleImpl *;
    using ShaderModuleRef = ShaderModuleImpl &;
}
