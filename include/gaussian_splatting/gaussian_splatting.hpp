#pragma once

#include <base/raytracing_base.hpp>

using namespace vrts;

class GaussianSplatting final :
    public RayTracingBase
{
    void init() override;
    void show() override;

    void resizeWindow() override;
};