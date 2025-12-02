#include <gaussian_splatting/gaussian_splatting.hpp>

void GaussianSplatting::init()
{
    RayTracingBase::init("GaussianSplatting");

    initScene();
}

void GaussianSplatting::show()
{
    /// @todo
}

void GaussianSplatting::resizeWindow()
{
    /// @todo
}

void GaussianSplatting::initScene()
{
    auto [width, height] = _window->getSize();
    _scene = Scene::Importer(getContext())
        .path(project_dir / "content/Tree.ply")
        .vkMemoryTypeIndex(MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        .viewport(width, height)
        .import();
}