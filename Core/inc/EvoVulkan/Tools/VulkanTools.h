//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_VULKANTOOLS_H
#define EVOVULKAN_VULKANTOOLS_H

#include <EvoVulkan/Types/Device.h>
#include <EvoVulkan/Types/Surface.h>
#include <EvoVulkan/Types/CmdPool.h>
#include <EvoVulkan/Types/CmdBuffer.h>
#include <EvoVulkan/Types/Swapchain.h>
#include <EvoVulkan/Types/DepthStencil.h>
#include <EvoVulkan/Types/Synchronization.h>
#include <EvoVulkan/Types/Instance.h>
#include <EvoVulkan/Types/Pipeline.h>

#include <EvoVulkan/Tools/VulkanInitializers.h>
#include <EvoVulkan/Tools/VulkanHelper.h>

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <array>
#include <mutex>

#include <EvoVulkan/Tools/VulkanConverter.h>

#include <functional>
#include <fstream>
#include <EvoVulkan/Types/VulkanBuffer.h>
#include "DeviceTools.h"
#include "EvoVulkan/Types/Instance.h"

namespace EvoVulkan::Tools {
    VkAttachmentDescription CreateColorAttachmentDescription(VkFormat format,
                                                             VkSampleCountFlagBits samples,
                                                             VkImageLayout init,
                                                             VkImageLayout final);

    VkShaderModule LoadShaderModule(const char *fileName, VkDevice device);

    VkPipelineLayout CreatePipelineLayout(const VkDevice& device, VkDescriptorSetLayout descriptorSetLayout);

    static VkDescriptorSetLayout CreateDescriptorLayout(
            const VkDevice& device,
            const std::vector<VkDescriptorSetLayoutBinding>& setLayoutBindings)
    {
        auto descriptorSetLayoutCreateInfo = Initializers::DescriptorSetLayoutCreateInfo(
                setLayoutBindings.data(),
                static_cast<uint32_t>(setLayoutBindings.size()));

        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        auto result = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout);
        if (result != VK_SUCCESS) {
            VK_ERROR("Tools::CreateDescriptorLayout() : failed to create descriptor set layout!");
            return VK_NULL_HANDLE;
        }
        else
            return descriptorSetLayout;
    }

    Types::Pipeline* CreateStandardGeometryPipeLine(
            const Types::Device* device,
            const std::vector<VkDynamicState>& dynamicStateEnables,
            std::vector<VkPipelineShaderStageCreateInfo> shaderStages,
            VkVertexInputBindingDescription vertexInputBinding,
            std::vector<VkVertexInputAttributeDescription> vertexInputAttributes,
            VkPipelineCache pipelineCache);

    static VkCommandBuffer* AllocateCommandBuffers(const VkDevice& device, VkCommandBufferAllocateInfo allocInfo) {
        auto cmdBuffs = (VkCommandBuffer*)malloc(sizeof(VkCommandBuffer) * allocInfo.commandBufferCount);

        auto result = vkAllocateCommandBuffers(device, &allocInfo, cmdBuffs);
        if (result != VK_SUCCESS) {
            VK_ERROR("Tools::AllocateCommandBuffers() : failed to allocate vulkan command buffer!");
            return nullptr;
        }

        return cmdBuffs;
    }

    static void FreeCommandBuffers(const VkDevice& device, const VkCommandPool& cmdPool, VkCommandBuffer** cmdBuffs, uint32_t count) {
        if (cmdBuffs && *cmdBuffs) {
            vkFreeCommandBuffers(device, cmdPool, count, *cmdBuffs);

            free(*cmdBuffs);
            *cmdBuffs = nullptr;
        } else
            VK_ERROR("Tools::FreeCommandBuffers() : command buffers in nullptr!");
    }

    void DestroyPipelineCache(const VkDevice& device, VkPipelineCache* cache);

    VkPipelineCache CreatePipelineCache(const VkDevice& device);

    void DestroySynchronization(const VkDevice& device, Types::Synchronization* sync);

    Types::Synchronization CreateSynchronization(const VkDevice& device);

    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static VkDebugUtilsMessengerEXT SetupDebugMessenger(const VkInstance& instance) {
        Tools::VkDebug::Graph("VulkanTools::SetupDebugMessenger() : setup vulkan debug messenger...");

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        Tools::PopulateDebugMessengerCreateInfo(createInfo);

        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        auto result = CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger);
        if (result != VK_SUCCESS) {
            Tools::VkDebug::Error("VulkanTools::SetupDebugMessenger() : failed to set up debug messenger! Reason: "
                + Tools::Convert::result_to_description(result));
            return VK_NULL_HANDLE;
        }

        return debugMessenger;
    }

    static Types::Surface* CreateSurface(const VkInstance& instance, const std::function<VkSurfaceKHR(const VkInstance&)>& platformCreate, void* windowHandle) {
        VkSurfaceKHR surfaceKhr = platformCreate(instance);
        if (surfaceKhr == VK_NULL_HANDLE) {
            Tools::VkDebug::Error("VulkanTools::CreateSurface() : failed platform-create vulkan surface!");
            return nullptr;
        }

        Types::Surface* surface = Types::Surface::Create(
                surfaceKhr,
                instance,
                windowHandle);

        return surface;
    }

    static VkDevice CreateLogicalDevice(
            VkPhysicalDevice physicalDevice,
            Types::FamilyQueues *pQueues,
            const std::vector<const char *> &extensions,
            const std::vector<const char *> &validLayers,
            VkPhysicalDeviceFeatures deviceFeatures)
    {
        Tools::VkDebug::Graph("VulkanTools::CreateLogicalDevice() : create vulkan logical device...");

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { pQueues->GetGraphicsIndex(), pQueues->GetPresentIndex() };

        std::vector<float_t> queuePriorities = { 1.0f }; //, 1.0f
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = queuePriorities.size();
            queueCreateInfo.pQueuePriorities = queuePriorities.data();
            queueCreateInfos.push_back(queueCreateInfo);
        }

        //!=============================================================================================================

        /*VkPhysicalDeviceImagelessFramebufferFeatures physicalDeviceImagelessFramebufferFeatures = {};
        physicalDeviceImagelessFramebufferFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES;
        physicalDeviceImagelessFramebufferFeatures.imagelessFramebuffer = true;

        VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = (void*)&physicalDeviceImagelessFramebufferFeatures;
        deviceFeatures2.features = deviceFeatures;*/

        //!==========

        VkDeviceCreateInfo createInfo      = {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        //createInfo.pNext                   = (void*)&deviceFeatures2;

        createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos       = queueCreateInfos.data();

        createInfo.pEnabledFeatures        = &deviceFeatures;

        createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        if (validLayers.empty()) {
            Tools::VkDebug::Graph("VulkanTools::CreateLogicalDevice() : validation layers disabled.");
            createInfo.enabledLayerCount = 0;
        } else {
            Tools::VkDebug::Graph("VulkanTools::CreateLogicalDevice() : validation layers enabled.");

            createInfo.enabledLayerCount   = static_cast<uint32_t>(validLayers.size());
            createInfo.ppEnabledLayerNames = validLayers.data();
        }

        VkDevice device = VK_NULL_HANDLE;
        auto result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
        if (result != VK_SUCCESS) {
            Tools::VkDebug::Graph("VulkanTools::CreateLogicalDevice() : failed to create logical device! \n\tReason: "
                + Tools::Convert::result_to_string(result) + "\n\tDescription: " + Tools::Convert::result_to_description(result));
            return VK_NULL_HANDLE;
        }

        return device;
    }

    static bool CopyBufferToImage(Types::CmdBuffer* copyCmd, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        if (!copyCmd->IsBegin())
            copyCmd->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VkBufferImageCopy region = {};
        region.bufferOffset      = 0;
        region.bufferRowLength   = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
                width,
                height,
                1
        };

        vkCmdCopyBufferToImage(*copyCmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        copyCmd->End();

        return true;
    }

    static VkSampler CreateSampler(
            const Types::Device* device,
            uint32_t mipLevels,
            VkFilter minFilter,
            VkFilter magFilter,
            VkSamplerAddressMode addressMode,
            VkCompareOp compareOp)
    {
        VkSamplerCreateInfo samplerIC = Tools::Initializers::SamplerCreateInfo();

        samplerIC.magFilter    = magFilter;
        samplerIC.minFilter    = minFilter;
        samplerIC.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerIC.addressModeU = addressMode;
        samplerIC.addressModeV = addressMode;
        samplerIC.addressModeW = addressMode;
        samplerIC.mipLodBias   = 0.0f;
        samplerIC.compareOp    = compareOp;
        samplerIC.minLod       = 0.0f;
        // Set max level-of-detail to mip level count of the texture
        samplerIC.maxLod = mipLevels;
        // Enable anisotropic filtering
        // This feature is optional, so we must check if it's supported on the device
        if (device->SamplerAnisotropyEnabled()) {
            // Use max. level of anisotropy for this example
            samplerIC.maxAnisotropy    = device->GetMaxSamplerAnisotropy();
            samplerIC.anisotropyEnable = VK_TRUE;
        } else {
            // The device does not support anisotropic filtering
            samplerIC.maxAnisotropy = 1.0;
            samplerIC.anisotropyEnable = VK_FALSE;
        }
        samplerIC.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        VkSampler sampler = VK_NULL_HANDLE;
        if (vkCreateSampler(*device, &samplerIC, nullptr, &sampler) != VK_SUCCESS) {
            VK_ERROR("Tools::CreateSampler() : failed to create vulkan sampler!");
            return VK_NULL_HANDLE;
        }

        return sampler;
    }

    static std::set<std::string> GetSupportedExtensions() {
        uint32_t count;
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr); //get number of extensions
        std::vector<VkExtensionProperties> extensions(count);
        vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()); //populate buffer
        std::set<std::string> results;
        for (auto & extension : extensions) {
            results.insert(extension.extensionName);
        }
        return results;
    }

    static VkImageView CreateImageView(
            const VkDevice& device,
            VkImage image,
            VkFormat format,
            uint32_t mipLevels,
            VkImageAspectFlags imageAspectFlags,
            bool cubeMap = false) {
        VkImageView view = VK_NULL_HANDLE;

        VkImageViewCreateInfo viewCI           = Tools::Initializers::ImageViewCreateInfo();
        viewCI.image                           = image;
        viewCI.viewType                        = cubeMap ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
        viewCI.format                          = format;
        viewCI.components                      = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        //viewCI.components                      = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
        viewCI.subresourceRange.aspectMask     = imageAspectFlags; //VK_IMAGE_ASPECT_COLOR_BIT;
        viewCI.subresourceRange.baseMipLevel   = 0;
        viewCI.subresourceRange.baseArrayLayer = 0;
        viewCI.subresourceRange.layerCount     = cubeMap ? 6 : 1;
        viewCI.subresourceRange.levelCount     = mipLevels;

        if (vkCreateImageView(device, &viewCI, nullptr, &view) != VK_SUCCESS) {
            VK_ERROR("Tools::CreateImageView() : failed to create image view!");
            return VK_NULL_HANDLE;
        }

        return view;
    }

    /*
    static VkImage CreateImage(
            Types::Device* device,
            uint32_t width, uint32_t height,
            uint32_t mipLevels,
            VkFormat format, VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties,
            Types::DeviceMemory* imageMemory,
            bool multisampling = true,
            VkImageCreateFlagBits createFlagBits = VK_IMAGE_CREATE_FLAG_BITS_MAX_ENUM,
            uint32_t arrayLayers = 1)
    {
        if (width == 0 || height == 0) {
            VK_ERROR("VulkanTools::CreateImage() : width or height equals zero!");
            return VK_NULL_HANDLE;
        }

        VkImageCreateInfo imageInfo = {};
        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth  = 1;
        imageInfo.mipLevels     = mipLevels;
        imageInfo.arrayLayers   = arrayLayers;
        imageInfo.format        = format;
        imageInfo.tiling        = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = usage;
        imageInfo.samples       = (mipLevels > 1 || !multisampling) ? VK_SAMPLE_COUNT_1_BIT : device->GetMSAASamples();
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        if (createFlagBits != VK_IMAGE_CREATE_FLAG_BITS_MAX_ENUM)
            imageInfo.flags = createFlagBits;

        VkImage image = VK_NULL_HANDLE;
        if (vkCreateImage(*device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            VK_ERROR("Tools::CreateImage() : failed to create vulkan image!");
            return VK_NULL_HANDLE;
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(*device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memRequirements.size;

        VkBool32 found;
        allocInfo.memoryTypeIndex = device->GetMemoryType(memRequirements.memoryTypeBits, properties, &found);
        if (!found) { // TODO
            allocInfo.memoryTypeIndex = device->GetMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &found);
            if (!found) {
                VK_ERROR("Tools::CreateImage() : memory is not support this properties!");
                return VK_NULL_HANDLE;
            }
        }

        *imageMemory = device->AllocateMemory(allocInfo);
        if (!imageMemory->Ready()) {
            VK_ERROR("Tools::CreateImage() : failed to allocate device memory!");
            return VK_NULL_HANDLE;
        }

        if (vkBindImageMemory(*device, image, *imageMemory, 0) != VK_SUCCESS) {
            VK_ERROR("Tools::CreateImage() : failed to bind vulkan image memory!");
            return VK_NULL_HANDLE;
        }

        return image;
    }*/

    static bool TransitionImageLayout(
            Types::CmdBuffer* copyCmd,
            VkImage image,
            VkImageLayout oldLayout,
            VkImageLayout newLayout,
            uint32_t mipLevels,
            uint32_t layerCount = 1)
    {
        if (!copyCmd->IsBegin())
            copyCmd->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VkImageMemoryBarrier barrier = {};
        barrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout            = oldLayout;
        barrier.newLayout            = newLayout;
        barrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                = image;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = layerCount;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
           VK_ERROR("Tools::TransitionImageLayout() : Unsupported layout transition!");
           return false;
        }

        vkCmdPipelineBarrier(
                *copyCmd,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
        );

        return copyCmd->End();
    }

    static Types::Device* CreateDevice(
            Types::Instance* instance, const Types::Surface* surface,
            const std::vector<const char*>& extensions,
            const std::vector<const char*>& validationLayers,
            const bool& enableSampleShading,
            bool multisampling,
            uint32_t sampleCount)
    {
        Tools::VkDebug::Graph("VulkanTools::CreateDevice() : create vulkan device...");

        Types::FamilyQueues* queues         = nullptr;

        VkPhysicalDevice     physicalDevice = VK_NULL_HANDLE;
        VkDevice             logicalDevice  = VK_NULL_HANDLE;

        //!=============================================================================================================

        auto devices = Tools::GetAllDevices(*instance);
        if (devices.empty()) {
            Tools::VkDebug::Error("VulkanTools::CreateDevice() : not found device with vulkan support!");
            return nullptr;
        }

        for (const auto& device : devices)
            Tools::VkDebug::Log("VulkanTools::CreateDevice() : found device - " + Tools::GetDeviceName(device));

        for (auto physDev : devices) {
            if (Tools::IsDeviceSuitable(physDev, (*surface), extensions)) {
                if (physicalDevice == VK_NULL_HANDLE) {
                    physicalDevice = physDev;
                    continue;
                }

                if (Tools::IsBetterThan(physDev, physicalDevice))
                    physicalDevice = physDev;
            }
            else
                Tools::VkDebug::Warn("VulkanTools::CreateDevice() : device \"" + Tools::GetDeviceName(physDev) + "\" isn't suitable!");
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            std::string msg = std::string();

            for (auto extension : extensions) {
                msg += "\n\t";
                msg += extension;
            }

            Tools::VkDebug::Error("VulkanTools::CreateDevice() : not found suitable device! \nExtensions: " + msg);

            return nullptr;
        }
        else
            Tools::VkDebug::Log("VulkanTools::CreateDevice() : select \"" + Tools::GetDeviceName(physicalDevice) + "\" device.");

        queues = Types::FamilyQueues::Find(physicalDevice, surface);
        if (!queues->IsComplete()) {
            Tools::VkDebug::Error("VulkanTools::CreateDevice() : family queues isn't complete!");
            return nullptr;
        }

        //!=============================================================================================================

        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.fillModeNonSolid  = true;
        deviceFeatures.samplerAnisotropy = true;
        //deviceFeatures.textureCompressionASTC_LDR = true;
        deviceFeatures.textureCompressionBC       = true;
        //deviceFeatures.textureCompressionETC2     = true;

        logicalDevice = Tools::CreateLogicalDevice(
                physicalDevice,
                queues,
                extensions,
                validationLayers,
                deviceFeatures);

        if (logicalDevice == VK_NULL_HANDLE) {
            Tools::VkDebug::Error("VulkanTools::CreateDevice() : failed create logical device!");
            return nullptr;
        }

        Tools::VkDebug::Graph("VulkanTools::CreateDevice() : set logical device queues...");
        {
            VkQueue graphics = VK_NULL_HANDLE;
            //VkQueue present  = VK_NULL_HANDLE;

            vkGetDeviceQueue(logicalDevice, queues->GetGraphicsIndex(), 0, &graphics);
            //vkGetDeviceQueue(logicalDevice, queues->GetPresentIndex(), 1, &present);

            queues->SetQueue(graphics);
            //queues->SetPresentQueue(present);
        }

        Types::EvoDeviceCreateInfo createInfo = {
                physicalDevice,
                logicalDevice,
                instance,
                queues,
                enableSampleShading,
                multisampling,
                static_cast<int32_t>(sampleCount)
        };

        if (auto finallyDevice = Types::Device::Create(createInfo)) {
            Tools::VkDebug::Log("VulkanTools::CreateDevice() : device successfully created!");
            return finallyDevice;
        }

        Tools::VkDebug::Error("VulkanTools::CreateDevice() : failed to create device!");
        return nullptr;
    }

    //VkRenderPass CreateRenderPass(const Types::Device* device, const Types::Swapchain* swapchain, std::vector<VkAttachmentDescription> attachments = {});
}

#endif //EVOVULKAN_VULKANTOOLS_H
