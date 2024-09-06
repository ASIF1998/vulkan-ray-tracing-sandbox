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

    VkPipeline _pipeline_handle = VK_NULL_HANDLE;
    VkPipelineLayout _pipeline_layout_handle = VK_NULL_HANDLE;    

    constexpr static uint32_t max_ray_tracing_recursive = 1u;
};