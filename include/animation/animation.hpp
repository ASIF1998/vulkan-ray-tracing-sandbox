#pragma once

#include <base/raytracing_base.hpp>
#include <base/scene/scene.hpp>

using namespace sample_vk;

class Animation final :
    public RayTracingBase
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

            count
        };
    };

    void init() override;
    void show() override;

    void resizeWindow() override;

    void initScene();
    void initCamera();

    void createPipelineLayout();
    void createPipeline();

    void createShaderBindingTable();

    void destroyPipelineLayout();
    void destroyPipeline();

    bool processEvents();

    void swapchainImageToPresentUsgae(uint32_t image_index);
    
public:
    Animation() = default;

    Animation(Animation&& animation)        = delete;
    Animation(const Animation& animation)   = delete;

    ~Animation();

    Animation& operator = (Animation&& animation)       = delete;
    Animation& operator = (const Animation& animation)  = delete;

private:
    std::optional<Scene> _scene;

    VkPipeline          _pipeline_handle        = VK_NULL_HANDLE;
    VkPipelineLayout    _pipeline_layout_handle = VK_NULL_HANDLE;

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

    constexpr static uint32_t _max_ray_tracing_recursive = 1u;
};