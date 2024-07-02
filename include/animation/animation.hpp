#pragma once

#include <base/raytracing_base.hpp>
#include <base/scene/scene.hpp>

using namespace sample_vk;

class Animation final :
    public RayTracingBase
{
    void init() override;
    void show() override;

    void resizeWindow() override;

    void initScene();

public:
    Animation() = default;

    Animation(Animation&& animation)        = delete;
    Animation(const Animation& animation)   = delete;

    ~Animation();

    Animation& operator = (Animation&& animation)       = delete;
    Animation& operator = (const Animation& animation)  = delete;

private:
    std::optional<Scene> _scene;
};