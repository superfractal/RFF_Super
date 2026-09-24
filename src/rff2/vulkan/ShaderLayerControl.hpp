//
// Modified by GPT-6 on 2026-09-16, 2026-09-17, 2026-09-19, 2026-09-21, 2026-09-23
//

#pragma once
#include <cstdint>

#include "../attr/ShaderAttribute.h"
#include "../../vulkan_helper/configurator/PipelineConfigurator.hpp"

namespace merutilm::rff2 {
    struct ShaderLayerControl {
        static constexpr int INITIALIZE = 100;
        static constexpr int FINALIZE = 101;

    private:
        static constexpr uint32_t SLOT_CONTROL = 0;
        static constexpr uint32_t SLOT_LINE_COLOR = 1;
        static constexpr uint32_t SLOT_LINE_PARAMS = 2;
        static constexpr uint32_t SLOT_LINE_SPINES = 3;
        static constexpr uint32_t SLOT_LINE_ORNAMENTS = 4;
        static constexpr uint32_t SLOT_LINE_GLOW = 5;
        static constexpr uint32_t SLOT_LINE_GLOW_COLOR = 6;

    public:
        static void configure(vkh::PushConstant &storage, vkh::PipelineLayoutManagerRef layout,
                              VkShaderStageFlags stage = VK_SHADER_STAGE_FRAGMENT_BIT) {
            auto manager = vkh::factory::create<vkh::HostDataObjectManager>();
            manager->reserve<glm::ivec4>(SLOT_CONTROL);
            manager->reserve<glm::vec4>(SLOT_LINE_COLOR);
            manager->reserve<glm::vec4>(SLOT_LINE_PARAMS);
            manager->reserve<glm::vec4>(SLOT_LINE_SPINES);
            manager->reserve<glm::vec4>(SLOT_LINE_ORNAMENTS);
            manager->reserve<glm::vec4>(SLOT_LINE_GLOW);
            manager->reserve<glm::vec4>(SLOT_LINE_GLOW_COLOR);
            storage = vkh::factory::create<vkh::PushConstant>(stage, std::move(manager));
            auto &host = storage->getHostObject();
            host.set(SLOT_CONTROL, glm::ivec4(0));
            host.set(SLOT_LINE_COLOR, glm::vec4(0));
            host.set(SLOT_LINE_PARAMS, glm::vec4(0));
            host.set(SLOT_LINE_SPINES, glm::vec4(0));
            host.set(SLOT_LINE_ORNAMENTS, glm::vec4(0));
            host.set(SLOT_LINE_GLOW, glm::vec4(0));
            host.set(SLOT_LINE_GLOW_COLOR, glm::vec4(0));
            layout.appendPushConstantManager(storage.get());
        }
        static void set(vkh::PipelineConfiguratorAbstract &pipeline, int stage,
                        const ShaderAttribute &shader) {
            auto &host = pipeline.getPushConstant(0).getHostObject();
            const auto &palette = shader.palette;
            host.set(SLOT_CONTROL, glm::ivec4(stage, shader.slope.studio.use ? 1 : 0, palette.bandLineEnabled ? 1 : 0,
                                   palette.bandLineGroove ? 1 : 0));
            host.set<glm::vec4>(SLOT_LINE_COLOR, palette.bandLineColor);
            host.set(SLOT_LINE_PARAMS, glm::vec4(float(palette.bandLineCount), palette.bandLineWidth,
                                  palette.bandLineOpacity, palette.bandLineSoftness));
            host.set(SLOT_LINE_SPINES, glm::vec4(palette.bandSpineAmount, palette.bandSpineLength, palette.bandSpineDensity,
                                  palette.bandBranchAmount));
            host.set(SLOT_LINE_ORNAMENTS, glm::vec4(palette.bandOrnamentAmount, palette.bandOrnamentSize,
                                  palette.bandOrnamentDensity, palette.bandOrnamentInset));
            host.set(SLOT_LINE_GLOW, glm::vec4(shader.slope.styleRimStrength, shader.slope.styleRimWidth, 0, 0));
            host.set<glm::vec4>(SLOT_LINE_GLOW_COLOR, shader.slope.styleRimColor);
        }
    };
} // namespace merutilm::rff2
