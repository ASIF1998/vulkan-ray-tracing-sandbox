#pragma once

#include <base/raytracing_base.hpp>

#include <base/vulkan/acceleration_structure.hpp>

using namespace sample_vk;

namespace sample_vk::hello_triangle
{
    struct Mesh
    {
        std::vector<glm::vec3>  vertices;
        std::vector<uint32_t>   indices;

        std::optional<Buffer> vertex_buffer;
        std::optional<Buffer> index_buffer;
    };

    struct ShaderID
    {
        enum : size_t
        {
            ray_gen,
            miss,
            chit,

            count
        };
    };
}

class HelloTriangle final :
    public RayTracingBase
{
    void init() override;
    void show() override;

    void resizeWindow() override;

    void createPipeline();
    void compileShaders();
    void createShaderBindingTable();
    void creareDescriptorSets();
    void createAS();

    void initMesh();

    void createBLAS();
    void createTLAS();

    void destroyDescriptorSets();
    void destroyShaders();
    void destroyPipeline();

    void updateDescriptorSets();

    void buildCommandBuffers();

public:
    HelloTriangle() = default;

    HelloTriangle(HelloTriangle&& sample)       = delete;
    HelloTriangle(const HelloTriangle& sample)  = delete;

    ~HelloTriangle();

    HelloTriangle& operator = (HelloTriangle&& sample)      = delete;
    HelloTriangle& operator = (const HelloTriangle& sample) = delete;

private:
    hello_triangle::Mesh _mesh;

    std::optional<AccelerationStructure> _blas;
    std::optional<AccelerationStructure> _tlas;

    VkDescriptorSetLayout                                   _descriptor_set_layout_handle   = VK_NULL_HANDLE;
    VkDescriptorPool                                        _descriptor_pool_handle         = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, NUM_IMAGES_IN_SWAPCHAIN>    _descriptor_set_handles         = { VK_NULL_HANDLE };

    std::array<VkShaderModule, hello_triangle::ShaderID::count> _shader_modules = { VK_NULL_HANDLE };

    VkPipeline          _pipeline_handle        = VK_NULL_HANDLE;
    VkPipelineLayout    _pipeline_layout_handle = VK_NULL_HANDLE;

    struct 
    {
        std::optional<Buffer> _raygen_shader_binding_table;
        std::optional<Buffer> _chit_shader_binding_table;
        std::optional<Buffer> _miss_shader_binding_table;

        VkStridedDeviceAddressRegionKHR raygen_region = { };
        VkStridedDeviceAddressRegionKHR chit_region = { };
        VkStridedDeviceAddressRegionKHR miss_region = { };
        VkStridedDeviceAddressRegionKHR callable_region = { };
    } _sbt;


    std::array<std::optional<CommandBuffer>, NUM_IMAGES_IN_SWAPCHAIN> _ray_tracing_launch;

    struct 
    {
        std::array<std::optional<CommandBuffer>, NUM_IMAGES_IN_SWAPCHAIN> general_to_present_layout;
        std::array<std::optional<CommandBuffer>, NUM_IMAGES_IN_SWAPCHAIN> present_layout_to_general;
    } _swapchain;
};