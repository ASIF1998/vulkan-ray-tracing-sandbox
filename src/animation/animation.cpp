#include <animation/animation.hpp>

#include <base/vulkan/memory.hpp>

#include <base/scene/visitors/acceleration_structure_builder.hpp>

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

        auto ptr_as_builder = std::make_unique<ASBuilder>(getContext());

        auto& model = _scene->getModel();
        model.visit(ptr_as_builder);
    }
}

void Animation::init() 
{
    RayTracingBase::init("Animation");

    initScene();
}

bool Animation::processEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        _scene->processEvent(&event);

        switch(event.type)
        {
            case SDL_QUIT:
                return false;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    return false;
                break;
        }
    }

    return true;
}



void Animation::show()
{
    while (processEvents())
    {
        auto image_index = getNextImageIndex();

        VkResult result = VK_RESULT_MAX_ENUM;

		VkPresentInfoKHR present_info = { };
		present_info.sType			= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.pImageIndices	= &image_index;
		present_info.pResults		= &result;
		present_info.pSwapchains	= &_swapchain_handle;
		present_info.swapchainCount = 1;

        auto present_result = vkQueuePresentKHR(_context.queue.handle, &present_info);
        VK_CHECK(present_result);
        VK_CHECK(result);
    }
} 

void Animation::resizeWindow()
{

} 