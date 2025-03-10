#pragma once

#include <base/raytracing_base.hpp>
#include <base/scene/scene.hpp>

#include <dancing_penguin/animation_pass.hpp>

using namespace vrts;

namespace vrts::dancing_penguin
{
    struct StageId
    {
        enum 
        {
            ray_gen,
            chit,
            miss,

            count
        };
    };
    
    struct Bindings
    {
        enum
        {
            result,
            acceleration_structure,
            scene_geometry,
            albedos,

            count
        };
    };

    struct PushConstants
    {
        struct RayGen
        {
            struct CameraData
            {
                glm::mat4 inv_view          = glm::mat4(1.0f);
                glm::mat4 inv_projection    = glm::mat4(1.0f);
            } camera_data;
        } rgen;

        struct ClosestHit
        {
            float eye_to_pixel_cone_spread_angle = 0.0f;
        } rchit;
    };
}

class DancingPenguin final :
    public RayTracingBase
{
    void init() override;
    void show() override;

    void resizeWindow() override;

    void initScene();
    void initCamera();

    void createPipelineLayout();
    void createPipeline();
    void createDescriptorSets();

    void createShaderBindingTable();

    void updateVertexBufferReferences();

    void destroyDescriptorSets();
    void destroyPipelineLayout();
    void destroyPipeline();

    bool processEvents();
    void processPushConstants();

    void swapchainImageToPresentUsage(uint32_t image_index);
    void swapchainImageToGeneralUsage(uint32_t image_index);

    void updateDescriptorSets(uint32_t image_index);

    void updateTime();

    void animationPass();

    void bindAlbedos();
    
public:
    DancingPenguin() = default;

    DancingPenguin(DancingPenguin&& animation)      = delete;
    DancingPenguin(const DancingPenguin& animation) = delete;

    ~DancingPenguin();

    DancingPenguin& operator = (DancingPenguin&& animation)         = delete;
    DancingPenguin& operator = (const DancingPenguin& animation)    = delete;

private:
    std::optional<Scene> _scene;

    VkPipeline              _pipeline_handle                = VK_NULL_HANDLE;
    VkPipelineLayout        _pipeline_layout_handle         = VK_NULL_HANDLE;
    VkDescriptorSetLayout   _descriptor_set_layout_handle   = VK_NULL_HANDLE;
    VkDescriptorSet         _descriptor_set_handle          = VK_NULL_HANDLE;
    VkDescriptorPool        _descriptor_pool_handle         = VK_NULL_HANDLE;

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

    struct 
    {
        std::optional<Buffer> scene_geometries_ref;
        std::optional<Buffer> scene_indices_ref;

        std::optional<Buffer> scene_info_reference;
    } _vertex_buffers_references;

    float _delta_time = 0.0f;

    dancing_penguin::PushConstants _push_constants;

    std::optional<dancing_penguin::AnimationPass> _animation_pass;

    constexpr static uint32_t _max_ray_tracing_recursive = 1u;
};
