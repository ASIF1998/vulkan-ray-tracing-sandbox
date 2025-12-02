#pragma once

#include <base/raytracing_base.hpp>
#include <base/scene/scene.hpp>

#include <optional>

using namespace vrts;

class GaussianSplatting final :
    public RayTracingBase
{
    void init() override;
    void show() override;

    void resizeWindow() override;

    void initScene();

    std::optional<Scene> _scene;
};