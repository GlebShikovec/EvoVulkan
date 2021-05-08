//
// Created by Nikita on 06.05.2021.
//

#ifndef EVOVULKAN_DESCRIPTORMANAGER_H
#define EVOVULKAN_DESCRIPTORMANAGER_H

#include <EvoVulkan/Types/Device.h>

namespace EvoVulkan::Core {
    class DescriptorPool {
    public:
        inline static const std::vector<std::pair<VkDescriptorType, float>> g_poolSizes =
                {
                        {VK_DESCRIPTOR_TYPE_SAMPLER,                0.5f},
                        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f},
                        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          4.f},
                        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1.f},
                        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1.f},
                        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1.f},
                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         2.f},
                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         2.f},
                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f},
                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f},
                        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       0.5f}
                };

        static bool Contains(const std::set<VkDescriptorType>& types, const VkDescriptorType& type) {
            std::set<VkDescriptorType>::iterator it;
            for (it = types.begin(); it != types.end(); ++it)
                if (type == *it)
                    return true;

            return false;
        }
    public:
        static DescriptorPool* Create(
                const uint32_t maxSets,
                VkDescriptorSetLayout layout,
                VkDevice device, const std::set<VkDescriptorType>& requestTypes)
        {
            auto* pool = new DescriptorPool(maxSets);

            pool->m_layout = layout;
            pool->m_device = device;

            pool->m_requestTypes = requestTypes;

            pool->m_descriptorSets = (VkDescriptorSet *) malloc(sizeof(VkDescriptorSet) * maxSets);
            for (uint32_t i = 0; i < maxSets; i++)
                pool->m_descriptorSets[i] = VK_NULL_HANDLE;

            std::vector<VkDescriptorPoolSize> sizes = {};
            sizes.reserve(g_poolSizes.size());
            for (auto sz : g_poolSizes)
                if (Contains(requestTypes, sz.first))
                    sizes.push_back({ sz.first, uint32_t(sz.second * maxSets) });

            VkDescriptorPoolCreateInfo descriptorPoolCI =
                    Tools::Initializers::DescriptorPoolCreateInfo(sizes.size(), sizes.data(), maxSets);

            VkResult vkRes = vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &pool->m_pool);
            if (vkRes != VK_NULL_HANDLE) {
                VK_ERROR("DescriptorPool::Create() : failed to create vulkan descriptor pool!");
                return VK_NULL_HANDLE;
            }

            return pool;
        }
    private:
        DescriptorPool(const uint32_t maxSets) : m_maxSets(maxSets) {

        }
    public:
        ~DescriptorPool() {
            if (m_descriptorSets) {
                free(m_descriptorSets);
                m_descriptorSets = nullptr;
            }

            if (m_pool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(m_device, m_pool, nullptr);
                m_pool = VK_NULL_HANDLE;
            }
        }
    public:
        bool Equal(const std::set<VkDescriptorType>& requestTypes) {
            return requestTypes.size() == m_requestTypes.size()
                   && std::equal(requestTypes.begin(), requestTypes.end(),
                                 m_requestTypes.begin());
        }

        [[nodiscard]] VkDescriptorSet* FindFree() const {
            for (uint32_t t = 0; t < m_maxSets; t++)
                if (m_descriptorSets[t] == VK_NULL_HANDLE)
                    return &m_descriptorSets[t];

            return nullptr;
        }
    public:
        std::set<VkDescriptorType> m_requestTypes   = std::set<VkDescriptorType>();

        // for check equal alloc request (reference)
        VkDescriptorSetLayout      m_layout         = VK_NULL_HANDLE;

        VkDevice                   m_device         = VK_NULL_HANDLE;
        VkDescriptorPool           m_pool           = VK_NULL_HANDLE;

        uint32_t                   m_used           = 0;
        const uint32_t             m_maxSets        = 0;
        VkDescriptorSet*           m_descriptorSets = nullptr;
    };

    class DescriptorManager {
    public:
        DescriptorManager(const DescriptorManager&) = delete;
    private:
        DescriptorManager()  = default;
        ~DescriptorManager() = default;
    private:
        uint32_t                        m_countDescriptorsAllocate = 1000;
        const EvoVulkan::Types::Device* m_device                   = nullptr;
        std::vector<DescriptorPool*>    m_pools                    = std::vector<DescriptorPool*>();
    public:
        static DescriptorManager* Create(const EvoVulkan::Types::Device* device) {
            auto manager = new DescriptorManager();
            manager->m_device = device;
            return manager;
        }

        void Free() {
            VK_LOG("DescriptorManager::Free() : free descriptor manager pointer...");
            this->Reset();
            delete this;
        }
    public:
        void Reset();
    public:
        VkDescriptorSet AllocateDescriptorSets(VkDescriptorSetLayout layout, const std::set<VkDescriptorType>& requestTypes);
    };
}

#endif //EVOVULKAN_DESCRIPTORMANAGER_H
