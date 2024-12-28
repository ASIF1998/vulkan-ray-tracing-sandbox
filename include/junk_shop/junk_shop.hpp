#pragma once

#include <base/raytracing_base.hpp>

#include <base/scene/scene.hpp>

#include <array>

using namespace vrts;

namespace vrts::junk_shop
{
    struct CameraData
    {
        glm::mat4 inv_view_matrix       = glm::mat4(1.0f);
        glm::mat4 inv_projection_matrix = glm::mat4(1.0f);
    };

    enum class DrawStay :
        uint32_t
    {
        draw,
        clear
    };

    struct PushConstants
    {
        struct RayGen
        {
            CameraData  camera_data;
            uint32_t    accumulated_frames_count    = 0;
        } rgen_consts;

        struct ClosestHit
        {
            float eye_to_pixel_cone_spread_angle = 0.0f;
        } chit_consts;
    };

    struct ShaderId
    {
        enum : size_t
        {
            ray_gen,
            chit,
            miss,

            count
        };
    };

    struct DescriptorSets
    {
        enum : size_t
        {
            acceleration_structure,

            storage_image,
            accumulated_buffer,

            ssbo,

            albedos,
            normal_maps,
            metallic,
            roughness,
            emissive,

            count
        };
    };

    using PoolSizes                 = std::array<VkDescriptorPoolSize, junk_shop::DescriptorSets::count>;
    using DescriptorSetsBindings    = std::array<VkDescriptorSetLayoutBinding, junk_shop::DescriptorSets::count>;
}

class JunkShop final :
    public RayTracingBase
{
    void init() override;
    void show() override;

    void resizeWindow() override;

public:
    ~JunkShop();

private:
    void createPipelineLayout();
    void createPipeline();
    void compileShaders();
    void createShaderBindingTable();
    void createDescriptorSets();
    void createAS();
    void createAccumulationBuffer();

    void importScene();
    void initVertexBuffersReferences();
    void initCamera();

    void destroyDescriptorSets();
    void destroyShaders();
    void destroyPipeline();

    void buildCommandBuffers();

    void updateDescriptorSet(uint32_t image_index);

    [[nodiscard]]
    junk_shop::PushConstants getPushConstantData();

    [[nodiscard]]
    bool processEvents();

    [[nodiscard]] 
    junk_shop::DescriptorSetsBindings getPipelineDescriptorSetsBindings() const;

    [[nodiscard]] 
    junk_shop::PoolSizes getPoolSizes() const;

    [[nodiscard]]
    static VkDescriptorImageInfo createDescriptorImageInfo(const Image& image);

    [[nodiscard]]
    static VkDescriptorImageInfo createDescriptorImageInfo(VkImageView image_view_handle);

    [[nodiscard]]
    static VkDescriptorBufferInfo createDescriptorBufferInfo(VkBuffer buffer_handle, VkDeviceSize size, VkDeviceSize offset = 0);

private:
    std::optional<Scene> _scene;

    std::array<VkShaderModule, junk_shop::ShaderId::count> _shader_modules;

    VkPipelineLayout    _pipeline_layout    = VK_NULL_HANDLE;
    VkPipeline          _pipeline           = VK_NULL_HANDLE;

    struct 
    {
        std::optional<Buffer> raygen;
        std::optional<Buffer> closest_hit;
        std::optional<Buffer> miss;

        VkStridedDeviceAddressRegionKHR raygen_region = { };
        VkStridedDeviceAddressRegionKHR chit_region = { };
        VkStridedDeviceAddressRegionKHR miss_region = { };
        VkStridedDeviceAddressRegionKHR callable_region = { };
    } _sbt;

    VkDescriptorPool        _descriptor_pool                = VK_NULL_HANDLE;
    VkDescriptorSetLayout   _descriptor_set_layout_handle   = VK_NULL_HANDLE;
    VkDescriptorSet         _descriptor_set                 = VK_NULL_HANDLE;

    uint32_t _accumulated_frames_count = 0;

    struct 
    {
        std::optional<Buffer> scene_geometries_ref;
        std::optional<Buffer> scene_indices_ref;

        std::optional<Buffer> scene_info_reference;
    } _vertex_buffers_references;

    junk_shop::DrawStay _draw_stay = junk_shop::DrawStay::draw;
    
    std::optional<Image> _accumulation_buffer;

    struct
    {
        std::array<std::optional<CommandBuffer>, NUM_IMAGES_IN_SWAPCHAIN> general_to_present_layout;
        std::array<std::optional<CommandBuffer>, NUM_IMAGES_IN_SWAPCHAIN> present_layout_to_general;
    } _swapchain;
};