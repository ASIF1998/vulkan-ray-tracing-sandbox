#include <animation/animation.hpp>

#include <base/vulkan/memory.hpp>

Animation::~Animation()
{

}

void Animation::initScene()
{
    if (auto memory_index = MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
    {
        auto [width, height] = _window->getSize();

        _scene = Scene::Importer(getContext())
            .path(project_dir / "content/mech-drone/source/Drone.FBX")
            .vkMemoryTypeIndex(*memory_index)
            .viewport(width, height)
            .import();
    }
}

void Animation::init() 
{
    RayTracingBase::init("Animation");

    initScene();
}

void Animation::show()
{

} 

void Animation::resizeWindow()
{

} 