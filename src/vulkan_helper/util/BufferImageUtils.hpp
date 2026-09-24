//
// Created by Merutilm on 2025-07-10.
// Modified by Opus 5 on 2026-08-31
// Modified by GPT-5 on 2026-09-01
// Modified by GPT-6 on 2026-09-22
//

#pragma once
#include "../struct/ImageInitInfo.hpp"
#include "../impl/Core.hpp"
#include "../struct/BufferInitInfo.hpp"
#include "../core/vkh_core.hpp"

namespace merutilm::vkh {
    struct BufferImageUtils {
        BufferImageUtils() = delete;


        static void initImage(CoreRef core,
                              const ImageInitInfo &iii, VkImage *image,
                              VkDeviceMemory *imageMemory,
                              VkImageView *imageView, VkImageView *mipmappedImageView, VkDeviceSize *capacity) {
            initImage(core.getLogicalDevice().getLogicalDeviceHandle(),
                      core.getPhysicalDevice().getPhysicalDeviceMemoryProperties(), iii, image, imageMemory, imageView, mipmappedImageView, capacity);
        }


        static void initImage(const VkDevice device, const VkPhysicalDeviceMemoryProperties &memProperties,
                              const ImageInitInfo &iii, VkImage *image,
                              VkDeviceMemory *imageMemory,
                              VkImageView *imageView, VkImageView *mipmappedImageView, VkDeviceSize *capacity) {
            *image = VK_NULL_HANDLE;
            *imageMemory = VK_NULL_HANDLE;
            *imageView = VK_NULL_HANDLE;
            *mipmappedImageView = VK_NULL_HANDLE;
            *capacity = 0;
            const uint32_t mipLevels = genMipLevels(iii);
            VkImage createdImage = VK_NULL_HANDLE;
            VkDeviceMemory allocatedMemory = VK_NULL_HANDLE;
            VkImageView createdView = VK_NULL_HANDLE;
            VkImageView createdMipmappedView = VK_NULL_HANDLE;
            VkDeviceSize allocatedCapacity = 0;
            createImage(device, iii, mipLevels, &createdImage);
            bool memoryAllocated = false;
            bool viewCreated = false;
            try {
                allocateImageMemory(device, memProperties, iii, createdImage,
                                    &allocatedMemory, &allocatedCapacity);
                memoryAllocated = true;
                // Every other step here refuses to go on when it fails; an image left with no memory
                // behind it is the one that goes on to be rendered into.
                if (allocator::invoke(vkBindImageMemory, device, createdImage, allocatedMemory, 0) != VK_SUCCESS) {
                    throw exception_init("Failed to bind image memory!");
                }
                createImageView(device, createdImage, iii.imageViewType, iii.imageFormat, &createdView);
                viewCreated = true;
                if (mipLevels == 1) {
                    createdMipmappedView = createdView;
                } else {
                    createMipmappedImageView(device, createdImage, iii.imageViewType, iii.imageFormat, mipLevels,
                                            &createdMipmappedView);
                }
            } catch (...) {
                if (viewCreated) {
                    allocator::invoke(vkDestroyImageView, device, createdView, nullptr);
                }
                allocator::invoke(vkDestroyImage, device, createdImage, nullptr);
                if (memoryAllocated) {
                    allocator::invoke(vkFreeMemory, device, allocatedMemory, nullptr);
                }
                throw;
            }
            *image = createdImage;
            *imageMemory = allocatedMemory;
            *imageView = createdView;
            *mipmappedImageView = createdMipmappedView;
            *capacity = allocatedCapacity;
        }


        static void createImage(const VkDevice device, const ImageInitInfo &iii, const uint32_t mipLevels, VkImage *image) {
            const VkImageCreateInfo imageInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .imageType = iii.imageType,
                .format = iii.imageFormat,
                .extent = iii.extent,
                .mipLevels = mipLevels,
                .arrayLayers = iii.arrayLayers,
                .samples = iii.samples,
                .tiling = iii.imageTiling,
                .usage = iii.usage,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices = nullptr,
                .initialLayout = iii.initialLayout,
            };
            if (allocator::invoke(vkCreateImage, device, &imageInfo, nullptr, image)) {
                throw exception_init("Failed to create image!");
            }
        }


        static void allocateImageMemory(const VkDevice device, const VkPhysicalDeviceMemoryProperties &memProperties,
                                        const ImageInitInfo &iii,
                                        const VkImage image, VkDeviceMemory *imageMemory, VkDeviceSize *capacity) {
            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(device, image, &memRequirements);
            const VkMemoryAllocateInfo allocInfo = {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = nullptr,
                .allocationSize = memRequirements.size,
                .memoryTypeIndex = findMemoryTypeIndex(memProperties, memRequirements.memoryTypeBits, iii.properties),
            };
            *capacity = memRequirements.size;
            if (allocator::invoke(vkAllocateMemory, device, &allocInfo, nullptr,
                                  imageMemory)) {
                throw exception_init("Failed to allocate memory!");
            }
        }

        static void createImageView(const VkDevice device, const VkImage image,
                                    const VkImageViewType imageViewType,
                                    const VkFormat imageFormat, VkImageView *writeImageView) {
            const VkImageViewCreateInfo viewInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = image,
                .viewType = imageViewType,
                .format = imageFormat,
                .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
                .subresourceRange = {
                    .aspectMask = getAspectMask(imageFormat),
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
            if (allocator::invoke(vkCreateImageView, device, &viewInfo, nullptr, writeImageView)) {
                throw exception_init("Failed to create image view!");
            }
        }

        static void createMipmappedImageView(const VkDevice device, const VkImage image,
                                    const VkImageViewType imageViewType,
                                    const VkFormat imageFormat, const uint32_t mipLevels, VkImageView *mipmappedImageView) {
            const VkImageViewCreateInfo viewInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = image,
                .viewType = imageViewType,
                .format = imageFormat,
                .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
                .subresourceRange = {
                    .aspectMask = getAspectMask(imageFormat),
                    .baseMipLevel = 0,
                    .levelCount = mipLevels,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
            if (allocator::invoke(vkCreateImageView, device, &viewInfo, nullptr, mipmappedImageView)) {
                throw exception_init("Failed to create image view!");
            }
        }

        static VkImageAspectFlags getAspectMask(const VkFormat format) {
            if (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D16_UNORM) {
                return VK_IMAGE_ASPECT_DEPTH_BIT;
            }
            if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
                return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            }

            return VK_IMAGE_ASPECT_COLOR_BIT;
        }

        static void initBuffer(CoreRef core, const BufferInitInfo &bii, VkBuffer *buffer, VkDeviceMemory *bufferMemory) {
            initBuffer(core.getLogicalDevice().getLogicalDeviceHandle(),
                       core.getPhysicalDevice().getPhysicalDeviceMemoryProperties(), bii, buffer, bufferMemory);
        }

        static void initBuffer(const VkDevice device, const VkPhysicalDeviceMemoryProperties &memProperties,
                               const BufferInitInfo &bii, VkBuffer *buffer, VkDeviceMemory *bufferMemory) {
            *buffer = VK_NULL_HANDLE;
            *bufferMemory = VK_NULL_HANDLE;
            VkBuffer createdBuffer = VK_NULL_HANDLE;
            VkDeviceMemory allocatedMemory = VK_NULL_HANDLE;
            createBuffer(device, bii.size, bii.usage, &createdBuffer);
            bool memoryAllocated = false;
            try {
                allocateBufferMemory(device, memProperties, bii.properties, createdBuffer,
                                     &allocatedMemory);
                memoryAllocated = true;
                if (allocator::invoke(vkBindBufferMemory, device, createdBuffer, allocatedMemory, 0) != VK_SUCCESS) {
                    throw exception_init("Failed to bind buffer memory!");
                }
            } catch (...) {
                allocator::invoke(vkDestroyBuffer, device, createdBuffer, nullptr);
                if (memoryAllocated) {
                    allocator::invoke(vkFreeMemory, device, allocatedMemory, nullptr);
                }
                throw;
            }
            *buffer = createdBuffer;
            *bufferMemory = allocatedMemory;
        }

        static void createBuffer(const VkDevice device, const VkDeviceSize size, const VkBufferUsageFlags usage,
                                 VkBuffer *buffer) {
            const VkBufferCreateInfo bufferInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .size = size,
                .usage = usage,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices = nullptr
            };
            if (allocator::invoke(vkCreateBuffer, device, &bufferInfo, nullptr, buffer) !=
                VK_SUCCESS) {
                throw exception_init("Failed to create buffer!");
            }
        }


        static void allocateBufferMemory(const VkDevice device, const VkPhysicalDeviceMemoryProperties &memProperties,
                                         const VkMemoryPropertyFlags properties,
                                         const VkBuffer buffer, VkDeviceMemory *bufferMemory) {
            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
            const VkMemoryAllocateInfo allocInfo = {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = nullptr,
                .allocationSize = memRequirements.size,
                .memoryTypeIndex = findMemoryTypeIndex(memProperties, memRequirements.memoryTypeBits,
                                                       properties)
            };
            if (allocator::invoke(vkAllocateMemory, device, &allocInfo, nullptr,
                                  bufferMemory) != VK_SUCCESS) {
                throw exception_init("failed to allocate memory!");
            }
        }


        static uint32_t findMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties &memProperties,
                                            const uint32_t memoryTypeBits,
                                            const VkMemoryPropertyFlags properties) {
            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
                if ((memoryTypeBits & 1 << i) != 0 && //check memory type is equal
                    (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                    //check the "propertyFlags" is completely contains "properties"
                    return i;
                }
            }

            throw exception_init("failed to find suitable memory type!");
        }

        static uint32_t genMipLevels(const ImageInitInfo &iii) {
            if (!iii.useMipmap) return 1;
            return getAvailableMipLevels({iii.extent.width, iii.extent.height});
        }

        static uint32_t getAvailableMipLevels(const VkExtent2D &extent) {
            return static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height))) + 1);
        }
    };
}
