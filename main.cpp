//
// Created by Nikita on 12.04.2021.
//

#include "UnitTests/Example.h"

int main() {
    auto* kernel = new VulkanExample();

    auto printMemory = [kernel]() -> std::string {
        if (!kernel->GetDevice())
            return std::string();
        else
            return "{" + std::to_string(kernel->GetDevice()->GetAllocatedMemorySize() / 1024 / 1024) + "MB} ";
    };

    EvoVulkan::Tools::VkDebug::Error = std::function<void(const std::string& msg)>([printMemory](const std::string& msg) {
        std::cout << printMemory() << "[Error] "  << msg << std::endl;
    });

    EvoVulkan::Tools::VkDebug::Graph = std::function<void(const std::string& msg)>([printMemory](const std::string& msg) {
        std::cout << printMemory() << "[Graph] "  << msg << std::endl;
    });

    EvoVulkan::Tools::VkDebug::Log = std::function<void(const std::string& msg)>([printMemory](const std::string& msg) {
        std::cout << printMemory() << "[Log] "  << msg << std::endl;
    });

    EvoVulkan::Tools::VkDebug::Warn = std::function<void(const std::string& msg)>([printMemory](const std::string& msg) {
        std::cout << printMemory() << "[Warn] "  << msg << std::endl;
    });

    //!=================================================================================================================

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    unsigned int width     = 600;  //1280
    unsigned int height    = 600;  //820
    bool validationEnabled = true;

    auto window = glfwCreateWindow((int)width, (int)height, "Vulkan application", nullptr, nullptr); //1280, 1024
    glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height) {
        auto kernel = static_cast<Core::VulkanKernel*>(glfwGetWindowUserPointer(window));
        kernel->SetSize(width, height);
    });

    //!=================================================================================================================

    glfwSetWindowUserPointer(window, (void*)kernel);

    kernel->SetValidationLayersEnabled(validationEnabled);
    kernel->SetSize(width, height);
    kernel->SetMultisampling(8);

    std::vector<const char*> extensions;
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

    if (validationEnabled)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    if (!kernel->PreInit("Simple engine", "NoEngine",
            R"(C:\VulkanSDK\1.2.170.0\Bin\glslc.exe)",
            { extensions },
            { "VK_LAYER_KHRONOS_validation" }))
    {
        std::cout << "Failed to pre-initialize Evo Vulkan!\n";
        return -1;
    }

    std::function<VkSurfaceKHR(const VkInstance& instance)> surfCreate = [window](const VkInstance& instance) -> VkSurfaceKHR {
        VkSurfaceKHR surfaceKhr = {};
        if (glfwCreateWindowSurface(instance, window, nullptr, &surfaceKhr) != VK_SUCCESS) {
            EvoVulkan::Tools::VkDebug::Error("VulkanKernel::Init(lambda) : failed to create glfw window surface!");
            return VK_NULL_HANDLE;
        } else
            return surfaceKhr;
    };

    if (!kernel->Init(
            surfCreate,
            { VK_KHR_SWAPCHAIN_EXTENSION_NAME },
            true, // sample shading
            true  // vsync
    )) {
        std::cout << "Failed to initialize Evo Vulkan!\n";
        return -1;
    }

    if (!kernel->PostInit()) {
        std::cout << "Failed to post-initialize Evo Vulkan!\n";
        return -1;
    }

    //!=================================================================================================================

    if (!kernel->LoadTexture())
        return -1;

    if (!kernel->LoadCubeMap())
        return -1;

    if (!kernel->SetupShader())
        return -1;

    if (!kernel->SetupUniforms())
        return -1;

    if (!kernel->GenerateGeometry())
        return -1;

    std::vector<EvoVulkan::Core::DescriptorSet> descriptors;
    for (uint32_t i = 0; i < 100000; i++)
        descriptors.emplace_back(kernel->GetDescriptorManager()->AllocateDescriptorSets(
                kernel->m_geometry->GetDescriptorSetLayout(),
                { VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }));

    for (auto& descriptor : descriptors)
        kernel->GetDescriptorManager()->FreeDescriptorSet(descriptor);

    /*std::array<Types::Buffer*, 4000> buffers = { };
    for (auto& buffer : buffers) {
        buffer = Types::Buffer::Create(
                kernel->GetDevice(),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                16);
    }*/

#ifdef EVOVULKAN_EXAMPLE_H
    kernel->LoadSkybox();
#endif

    kernel->BuildCmdBuffers();

    std::cout << kernel->GetDevice()->GetAllocatedHeapsCount() << std::endl;

    while (!glfwWindowShouldClose(window) && !kernel->HasErrors()) {
        glfwPollEvents();

        kernel->NextFrame();

        kernel->UpdateUBO();
    }

    kernel->Destroy();

    delete kernel;

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}