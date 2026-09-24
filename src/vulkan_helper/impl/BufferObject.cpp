//
// Created by Merutilm on 2025-07-10.
// Modified by GPT-6 on 2026-09-23
// Modified by Opus 5.5 on 2026-09-23
//

#include "BufferObject.hpp"

#include "../executor/ScopedNewCommandBufferExecutor.hpp"
#include "../util/BufferImageUtils.hpp"
#include "../core/logger.hpp"
#include "../util/BarrierUtils.hpp"
#include "../util/BufferImageContextUtils.hpp"

namespace merutilm::vkh {
    namespace {
        void waitForBufferCopy(CoreRef core, FencePtr fence) {
            if (fence == nullptr) {
                core.getLogicalDevice().waitDeviceIdle();
            } else {
                fence->wait();
            }
        }
    }

    BufferObjectAbstract::BufferObjectAbstract(CoreRef core, HostDataObjectManager &&dataManager,
                                               const VkBufferUsageFlags bufferUsage,
                                               const BufferLock bufferLock,
                                               const bool multiframeEnabled) : CoreHandler(core),
                                                                               hostDataObject(
                                                                                   factory::create<HostDataObject>(
                                                                                       std::move(dataManager))),
                                                                               bufferUsage(bufferUsage),
                                                                               bufferLock(bufferLock),
                                                                               multiframeEnabled(multiframeEnabled) {
        BufferObjectAbstract::init();
    }

    BufferObjectAbstract::~BufferObjectAbstract() {
        BufferObjectAbstract::destroy();
    }


    void BufferObjectAbstract::reloadBuffer() {
        BufferObjectAbstract::destroy();
        locked = false;
        BufferObjectAbstract::init();
    }

    void BufferObjectAbstract::update() const {
        checkFinalizedBeforeUpdate();
        memcpy(getBufferContext().mappedMemory, hostDataObject->data.data(), hostDataObject->getTotalSizeByte());
    }

    void BufferObjectAbstract::update(const uint32_t target) const {
        checkFinalizedBeforeUpdate();
        const uint32_t offset = hostDataObject->getOffset(target);
        const uint32_t size = hostDataObject->getSizeByte(target);
        memcpy(getBufferContext().mappedMemory + offset, hostDataObject->data.data() + offset, size);
    }

    void BufferObjectAbstract::updateMF(const uint32_t frameIndex) const {
        checkFinalizedBeforeUpdate();
        memcpy(getBufferContextMF(frameIndex).mappedMemory, hostDataObject->data.data(),
               hostDataObject->getTotalSizeByte());
    }

    void BufferObjectAbstract::updateMF(const uint32_t frameIndex, const uint32_t target) const {
        checkFinalizedBeforeUpdate();
        const uint32_t offset = hostDataObject->getOffset(target);
        const uint32_t size = hostDataObject->getSizeByte(target);
        memcpy(getBufferContextMF(frameIndex).mappedMemory + offset, hostDataObject->data.data() + offset, size);
    }

    void BufferObjectAbstract::upload() const {
        if (multiframeEnabled) {
            for (uint32_t i = 0; i < getBufferContextMF().size(); ++i) {
                updateMF(i);
            }
        } else {
            update();
        }
    }

    void BufferObjectAbstract::checkFinalizedBeforeUpdate() const {
        if (locked) {
            throw exception_invalid_state(
                "BufferObjectAbstract::updateMF() This bufferObject is already been finalized. It cannot be modified.");
        }
    }


    void BufferObjectAbstract::init() {
        const uint32_t size = hostDataObject->getTotalSizeByte();

        VkBufferUsageFlags lockFlags = 0;

        switch (bufferLock) {
                using enum BufferLock;
            case LOCK_UNLOCK:
            case LOCK_ONLY: lockFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                break;
            case ALWAYS_MUTABLE: lockFlags = 0;
                break;
        }

        const BufferInitInfo info{
            .size = size,
            .usage = bufferUsage | lockFlags,
            .properties =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

        if (multiframeEnabled) {
            auto context = BufferContext::createMultiframeContext(core, info);
            size_t mappedCount = 0;
            try {
                for (auto &frame : context) {
                    BufferContext::mapMemory(core, frame);
                    ++mappedCount;
                }
            } catch (...) {
                for (size_t i = 0; i < mappedCount; ++i) {
                    BufferContext::unmapMemory(core, context[i]);
                }
                BufferContext::destroyContext(core, context);
                throw;
            }
            bufferContext = std::move(context);
        } else {
            auto context = BufferContext::createContext(core, info);
            try {
                BufferContext::mapMemory(core, context);
            } catch (...) {
                BufferContext::destroyContext(core, context);
                throw;
            }
            bufferContext = context;
        }
    }

    void BufferObjectAbstract::lock(CommandPoolRef commandPool, FencePtr const fence) {
        if (locked) {
            logger::log_err_silent("Double-call of BufferObjectAbstract::lock()");
            return;
        }

        VkBufferUsageFlags lockFlags = 0;

        switch (bufferLock) {
                using enum BufferLock;
            case LOCK_UNLOCK: lockFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                break;
            case LOCK_ONLY: lockFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                break;
            case ALWAYS_MUTABLE: {
                logger::log_err_silent("Cannot lock object because the given BufferLock is {}",
                                       static_cast<uint32_t>(bufferLock));
                return;
            }
        }


        const VkBufferCopy copyRegion = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = hostDataObject->getTotalSizeByte(),
        };
        const BufferInitInfo info{
            .size = hostDataObject->getTotalSizeByte(),
            .usage = bufferUsage | lockFlags,
            .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        };

        if (multiframeEnabled) {
            MultiframeBufferContext lockedBuffer = BufferContext::createMultiframeContext(core, info);
            //NEW COMMAND BUFFER
            try {
                const uint32_t maxFramesInFlight = core.getPhysicalDevice().getMaxFramesInFlight();
                auto cex = ScopedNewCommandBufferExecutor(core, commandPool);

                for (uint32_t i = 0; i < maxFramesInFlight; ++i) {
                    BarrierUtils::cmdBufferMemoryBarrier(cex.getCommandBufferHandle(), VK_ACCESS_HOST_WRITE_BIT,
                                                         VK_ACCESS_TRANSFER_READ_BIT, getBufferContextMF(i).buffer, 0,
                                                         hostDataObject->getTotalSizeByte(), VK_PIPELINE_STAGE_HOST_BIT,
                                                         VK_PIPELINE_STAGE_TRANSFER_BIT);
                    vkCmdCopyBuffer(cex.getCommandBufferHandle(), getBufferContextMF(i).buffer, lockedBuffer[i].buffer,
                                    1,
                                    &copyRegion);
                }
                cex.finish();
            } catch (...) {
                BufferContext::destroyContext(core, lockedBuffer);
                throw;
            }

            waitForBufferCopy(core, fence);

            BufferContext::unmapMemory(core, getBufferContextMF());
            BufferContext::destroyContext(core, getBufferContextMF());
            bufferContext = std::move(lockedBuffer);
        } else {
            BufferContext lockedBuffer = BufferContext::createContext(core, info);
            //NEW COMMAND BUFFER
            try {
                auto cex = ScopedNewCommandBufferExecutor(core, commandPool);
                BarrierUtils::cmdBufferMemoryBarrier(cex.getCommandBufferHandle(), VK_ACCESS_HOST_WRITE_BIT,
                                                     VK_ACCESS_TRANSFER_READ_BIT, getBufferContext().buffer, 0,
                                                     hostDataObject->getTotalSizeByte(), VK_PIPELINE_STAGE_HOST_BIT,
                                                     VK_PIPELINE_STAGE_TRANSFER_BIT);
                vkCmdCopyBuffer(cex.getCommandBufferHandle(), getBufferContext().buffer, lockedBuffer.buffer, 1,
                                &copyRegion);
                cex.finish();
            } catch (...) {
                BufferContext::destroyContext(core, lockedBuffer);
                throw;
            }

            waitForBufferCopy(core, fence);

            BufferContext::unmapMemory(core, getBufferContext());
            BufferContext::destroyContext(core, getBufferContext());
            bufferContext = std::move(lockedBuffer);
        }
        locked = true;
    }


    void BufferObjectAbstract::unlock(CommandPoolRef commandPool, FencePtr const fence) {
        if (!locked) {
            logger::w_log_err_silent(L"Double-call of BufferObjectAbstract::unlock()");
            return;
        }
        VkBufferUsageFlags lockFlags = 0;

        switch (bufferLock) {
                using enum BufferLock;
            case LOCK_UNLOCK: lockFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                break;
            case LOCK_ONLY:
            case ALWAYS_MUTABLE: {
                logger::w_log_err_silent(L"Unlock is not allowed : BufferLock is not LOCK_UNLOCK");
                return;
            }
        }


        const BufferInitInfo info {
            .size = hostDataObject->getTotalSizeByte(),
            .usage = bufferUsage | lockFlags,
            .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

        const VkBufferCopy copyRegion = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = hostDataObject->getTotalSizeByte(),
        };
        //NEW COMMAND BUFFER
        if (multiframeEnabled) {

            MultiframeBufferContext unlockedBuffer = BufferContext::createMultiframeContext(core, info);
            size_t mappedCount = 0;
            try {
                const uint32_t maxFramesInFlight = core.getPhysicalDevice().getMaxFramesInFlight();
                auto cex = ScopedNewCommandBufferExecutor(core, commandPool);
                for (uint32_t i = 0; i < maxFramesInFlight; ++i) {
                    BarrierUtils::cmdBufferMemoryBarrier(cex.getCommandBufferHandle(), VK_ACCESS_SHADER_WRITE_BIT,
                                                         VK_ACCESS_TRANSFER_READ_BIT, getBufferContextMF(i).buffer, 0,
                                                         hostDataObject->getTotalSizeByte(),
                                                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                                         VK_PIPELINE_STAGE_TRANSFER_BIT);
                    vkCmdCopyBuffer(cex.getCommandBufferHandle(), getBufferContextMF(i).buffer,
                                    unlockedBuffer[i].buffer, 1,
                                    &copyRegion);
                }
                cex.finish();

                waitForBufferCopy(core, fence);
                for (auto &frame : unlockedBuffer) {
                    BufferContext::mapMemory(core, frame);
                    ++mappedCount;
                }
            } catch (...) {
                for (size_t i = 0; i < mappedCount; ++i) {
                    BufferContext::unmapMemory(core, unlockedBuffer[i]);
                }
                BufferContext::destroyContext(core, unlockedBuffer);
                throw;
            }
            BufferContext::destroyContext(core, getBufferContextMF());
            bufferContext = std::move(unlockedBuffer);
        } else {

            BufferContext unlockedBuffer = BufferContext::createContext(core, info);
            bool mapped = false;
            try {
                auto cex = ScopedNewCommandBufferExecutor(core, commandPool);
                BarrierUtils::cmdBufferMemoryBarrier(cex.getCommandBufferHandle(), VK_ACCESS_SHADER_WRITE_BIT,
                                                     VK_ACCESS_TRANSFER_READ_BIT, getBufferContext().buffer, 0,
                                                     hostDataObject->getTotalSizeByte(),
                                                     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                                     VK_PIPELINE_STAGE_TRANSFER_BIT);
                vkCmdCopyBuffer(cex.getCommandBufferHandle(), getBufferContext().buffer, unlockedBuffer.buffer, 1,
                                &copyRegion);
                cex.finish();

                waitForBufferCopy(core, fence);
                BufferContext::mapMemory(core, unlockedBuffer);
                mapped = true;
            } catch (...) {
                if (mapped) {
                    BufferContext::unmapMemory(core, unlockedBuffer);
                }
                BufferContext::destroyContext(core, unlockedBuffer);
                throw;
            }
            BufferContext::destroyContext(core, getBufferContext());
            bufferContext = std::move(unlockedBuffer);
        }

        locked = false;
    }


    void BufferObjectAbstract::destroy() {
        if (multiframeEnabled) {
            auto &context = getBufferContextMF();
            BufferContext::destroyContext(core, context);
            context.clear();
        } else {
            auto &context = getBufferContext();
            BufferContext::destroyContext(core, context);
            context = {};
        }
    }
}
