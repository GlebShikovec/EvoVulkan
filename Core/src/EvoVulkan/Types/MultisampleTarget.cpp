//
// Created by Nikita on 11.06.2021.
//

#include <EvoVulkan/Types/MultisampleTarget.h>
#include <EvoVulkan/Tools/VulkanTools.h>
#include <EvoVulkan/Memory/Allocator.h>

EvoVulkan::Types::MultisampleTarget *EvoVulkan::Types::MultisampleTarget::Create(
        EvoVulkan::Types::Device *device,
        Memory::Allocator* allocator,
        Swapchain* swapchain,
        uint32_t w, uint32_t h,
        const std::vector<VkFormat>& formats,
        bool multisampling)
{
    auto multisample = new MultisampleTarget();
    multisample->m_device        = device;
    multisample->m_allocator     = allocator;
    multisample->m_swapchain     = swapchain;
    multisample->m_countResolves = formats.size();
    multisample->m_formats       = formats;
    multisample->m_multisampling = multisampling;

    if (!multisample->ReCreate(w, h)) {
        VK_ERROR("MultisampleTarget::Create() : failed to re-create multisample!");
        return nullptr;
    }

    return multisample;
}

bool EvoVulkan::Types::MultisampleTarget::ReCreate(uint32_t w, uint32_t h) {
    this->Destroy();

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(*m_device, &deviceProperties);

    if (!((deviceProperties.limits.framebufferColorSampleCounts >= m_device->GetMSAASamples()) && (deviceProperties.limits.framebufferDepthSampleCounts >= m_device->GetMSAASamples()))) {
        VK_ERROR("MultisampleTarget::ReCreate() : multisampling is unsupported!");
        return false;
    }

    //! ----------------------------------- Color target -----------------------------------

    m_resolves = (Image*)malloc(sizeof(Image) * m_countResolves);

    auto imageCI = Types::ImageCreateInfo(
            m_device, m_allocator, w, h,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            m_multisampling);

    for (uint32_t i = 0; i < m_countResolves; ++i) {
        imageCI.format = m_formats[i];

        if (!(m_resolves[i].m_image = Types::Image::Create(imageCI)).Valid()) {
            VK_ERROR("MultisampleTarget::ReCreate() : failed to create resolve image!");
            return false;
        }

        m_resolves[i].m_view = Tools::CreateImageView(*m_device, m_resolves[i].m_image, m_formats[i], 1, VK_IMAGE_ASPECT_COLOR_BIT);

        if (m_resolves[i].m_view == VK_NULL_HANDLE) {
            VK_ERROR("MultisampleTarget::ReCreate() : failed to create resolve image view!");
            return false;
        }
    }

    //! ----------------------------------- Depth target -----------------------------------

    imageCI.format = m_swapchain->GetDepthFormat();
    imageCI.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    if (!(m_depth.m_image = Types::Image::Create(imageCI)).Valid()) {
        VK_ERROR("MultisampleTarget::ReCreate() : failed to create depth image!");
        return false;
    }

    m_depth.m_view = Tools::CreateImageView(*m_device, m_depth.m_image, m_swapchain->GetDepthFormat(), 1, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    if (m_depth.m_view == VK_NULL_HANDLE) {
        VK_ERROR("MultisampleTarget::ReCreate() : failed to create depth image view!");
        return false;
    }

    return true;
}

void EvoVulkan::Types::MultisampleTarget::Destroy() {
    // Destroy MSAA target
    if (m_resolves) {
        for (uint32_t i = 0; i < m_countResolves; ++i) {
            if (m_resolves[i].m_image && m_resolves[i].m_view && m_resolves[i].m_image.Valid()) {
                vkDestroyImageView(*m_device, m_resolves[i].m_view, nullptr);
                m_allocator->FreeImage(m_resolves[i].m_image);
                m_resolves[i].m_view = VK_NULL_HANDLE;
            }
        }
        free(m_resolves);
        this->m_resolves = nullptr;
        //m_countResolves = dont touch
    }

    if (m_depth.m_image && m_depth.m_view && m_depth.m_image.Valid()) {
        vkDestroyImageView(*m_device, m_depth.m_view, nullptr);
        m_allocator->FreeImage(m_depth.m_image);
        m_depth.m_view = VK_NULL_HANDLE;
    }
}
