//
// Created by Merutilm on 2025-08-15.
// Modified by GPT-6 on 2026-09-10, 2026-09-16, 2026-09-20, 2026-09-23
//

#include "GPCColor.hpp"

#include "SharedDescriptorTemplate.hpp"
#include "SharedImageContextIndices.hpp"
#include "../constants/VulkanWindowConstants.hpp"

namespace merutilm::rff2 {
    void GPCColor::updateQueue(vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
        //no operation
    }

    void GPCColor::setColor(const ShdColorAttribute &color, const bool sceneLinear) const {
        using namespace SharedDescriptorTemplate;
        auto &colorDescriptor = getDescriptor(SET_COLOR);
        const auto &colorUniform = *colorDescriptor.get<vkh::Uniform>(0, DescColor::BINDING_UBO_COLOR);
        auto &colorParameters = colorUniform.getHostObject();
        colorParameters.set<float>(DescColor::TARGET_COLOR_GAMMA, color.gamma);
        colorParameters.set<float>(DescColor::TARGET_COLOR_EXPOSURE, color.exposure);
        colorParameters.set<float>(DescColor::TARGET_COLOR_HUE, color.hue);
        colorParameters.set<float>(DescColor::TARGET_COLOR_SATURATION, color.saturation);
        colorParameters.set<float>(DescColor::TARGET_COLOR_BRIGHTNESS, color.brightness);
        colorParameters.set<float>(DescColor::TARGET_COLOR_CONTRAST, color.contrast);
        colorParameters.set<float>(DescColor::TARGET_COLOR_SCENE_LINEAR, sceneLinear ? 1.0f : 0.0f);
        colorUniform.update();
    }

    void GPCColor::pipelineInitialized() {
        using namespace SharedDescriptorTemplate;
        writeDescriptorMF([this](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
            getDescriptor(SET_COLOR).queue(queue, frameIndex, {}, {DescColor::BINDING_UBO_COLOR});
        });
    }

    void GPCColor::renderContextRefreshed() {
        auto &sic = wc.getSharedImageContext();
        auto &inputDesc = getDescriptor(SET_PREV_RESULT);

        switch (wc.getAttachmentIndex()) {
            case Constants::VulkanWindow::MAIN_WINDOW_ATTACHMENT_INDEX: {
                const auto &input =  sic.getImageContextMF(SharedImageContextIndices::MF_MAIN_RENDER_IMAGE_SECONDARY);
                inputDesc.get<vkh::InputAttachment>(0, BINDING_PREV_RESULT_INPUT).ctx = input;
                break;
            }
            case Constants::VulkanWindow::VIDEO_PREPARATION_WINDOW_ATTACHMENT_INDEX:
            case Constants::VulkanWindow::VIDEO_WINDOW_ATTACHMENT_INDEX: {
                const auto &input =  sic.getImageContextMF(SharedImageContextIndices::MF_VIDEO_RENDER_IMAGE_SECONDARY);
                inputDesc.get<vkh::InputAttachment>(0, BINDING_PREV_RESULT_INPUT).ctx = input;
                break;
            }
            default: {
                //noop
            }
        }
        writeDescriptorMF([&inputDesc](vkh::DescriptorUpdateQueue &queue, const uint32_t frameIndex) {
            inputDesc.queue(queue, frameIndex, {}, {BINDING_PREV_RESULT_INPUT});
        });
    }

    void GPCColor::configurePushConstant(vkh::PipelineLayoutManagerRef pipelineLayoutManager) {
        ShaderLayerControl::configure(layerPush, pipelineLayoutManager);
        //noop
    }

    void GPCColor::configureDescriptors(std::vector<vkh::DescriptorPtr> &descriptors) {
        using namespace SharedDescriptorTemplate;
        auto descManager = vkh::factory::create<vkh::DescriptorManager>();
        descManager->appendInputAttachment(BINDING_PREV_RESULT_INPUT, VK_SHADER_STAGE_FRAGMENT_BIT);

        appendUniqueDescriptor(SET_PREV_RESULT, descriptors, std::move(descManager));
        appendDescriptor<DescColor>(SET_COLOR, descriptors);
    }
}
