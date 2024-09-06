#include <animation/animation.hpp>

#include <base/vulkan/memory.hpp>

#include <base/scene/visitors/acceleration_structure_builder.hpp>

#include <base/shader_compiler.hpp>

Animation::~Animation()
{
    destroyPipelineLayout();
    destroyPipeline();
}

void Animation::initScene()
{
    if (auto memory_index = MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
    {
        auto [width, height] = _window->getSize();

        _scene = Scene::Importer(getContext())
            .path(project_dir / "content/dancing_penguin.glb")
            .vkMemoryTypeIndex(*memory_index)
            .viewport(width, height)
            .import();

        auto ptr_as_builder = std::make_unique<ASBuilder>(getContext());

        auto& model = _scene->getModel();
        model.visit(ptr_as_builder);
    }
}

void Animation::initCamera()
{
    auto& camera = _scene->getCameraController().getCamera();

    camera.setDepthRange(0.0f, 1000.0f);
    camera.lookaAt(glm::vec3(50, 1823.898, 5133.947), glm::vec3(-0.5, 0, 1));
}

void Animation::createPipelineLayout()
{
    auto ptr_context = getContext();

    std::vector<VkDescriptorSetLayoutBinding> bindings (2);
    bindings[0].binding         = Animation::Bindings::result;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR ;
    
    bindings[1].binding         = Animation::Bindings::acceleration_structure;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    bindings[1].stageFlags  = VK_SHADER_STAGE_RAYGEN_BIT_KHR ;

    VkDescriptorSetLayout set_layout_handle = VK_NULL_HANDLE;
    VkDescriptorSetLayoutCreateInfo set_layout_info = { };
    set_layout_info.sType           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set_layout_info.pBindings       = bindings.data();
    set_layout_info.bindingCount    = static_cast<uint32_t>(bindings.size());

    VK_CHECK(vkCreateDescriptorSetLayout(ptr_context->device_handle, &set_layout_info, nullptr, &set_layout_handle));

    std::vector<VkDescriptorSetLayout> set_layouts;

    VkPipelineLayoutCreateInfo pipeline_layout_info = { };
    pipeline_layout_info.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts    = &set_layout_handle;

    VK_CHECK(vkCreatePipelineLayout(ptr_context->device_handle, &pipeline_layout_info, nullptr, &_pipeline_layout_handle));

    vkDestroyDescriptorSetLayout(ptr_context->device_handle, set_layout_handle, nullptr);
}

void Animation::createPipeline()
{
    auto ptr_context = getContext();

    const auto animation_shaders_root = project_dir / "shaders/animation";

    std::array<VkShaderModule, Animation::StageId::count> shader_modules = { VK_NULL_HANDLE };
    shader_modules[Animation::StageId::ray_gen] = shader::Compiler::createShaderModule(ptr_context->device_handle, animation_shaders_root / "animation.glsl.rgen", shader::Type::raygen);
    shader_modules[Animation::StageId::miss] = shader::Compiler::createShaderModule(ptr_context->device_handle, animation_shaders_root / "animation.glsl.rmiss", shader::Type::miss);
    shader_modules[Animation::StageId::chit] = shader::Compiler::createShaderModule(ptr_context->device_handle, animation_shaders_root / "animation.glsl.rchit", shader::Type::closesthit);

    std::array<VkShaderStageFlagBits, Animation::StageId::count> stages = { };
    stages[Animation::StageId::ray_gen] = VK_SHADER_STAGE_RAYGEN_BIT_KHR; 
    stages[Animation::StageId::miss] = VK_SHADER_STAGE_MISS_BIT_KHR; 
    stages[Animation::StageId::chit] = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR; 

    std::array<VkPipelineShaderStageCreateInfo, Animation::StageId::count> shader_stage_create_infos = { };
    for (size_t i = 0; i < Animation::StageId::count; ++i)
    {
        shader_stage_create_infos[i].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage_create_infos[i].module = shader_modules[i];
        shader_stage_create_infos[i].pName  = "main";
        shader_stage_create_infos[i].stage  = stages[i];
    }
    
    std::array<VkRayTracingShaderGroupCreateInfoKHR, Animation::StageId::count> groups = { };

    groups[Animation::StageId::ray_gen].sType               = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groups[Animation::StageId::ray_gen].generalShader       = Animation::StageId::ray_gen;
    groups[Animation::StageId::ray_gen].anyHitShader        = VK_SHADER_UNUSED_KHR;
    groups[Animation::StageId::ray_gen].closestHitShader    = VK_SHADER_UNUSED_KHR;
    groups[Animation::StageId::ray_gen].intersectionShader  = VK_SHADER_UNUSED_KHR;
    groups[Animation::StageId::ray_gen].type                = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;

    groups[Animation::StageId::miss].sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groups[Animation::StageId::miss].generalShader      = Animation::StageId::miss;
    groups[Animation::StageId::miss].anyHitShader       = VK_SHADER_UNUSED_KHR;
    groups[Animation::StageId::miss].closestHitShader   = VK_SHADER_UNUSED_KHR;
    groups[Animation::StageId::miss].intersectionShader = VK_SHADER_UNUSED_KHR;
    groups[Animation::StageId::miss].type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    
    groups[Animation::StageId::chit].sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groups[Animation::StageId::chit].generalShader      = VK_SHADER_UNUSED_KHR;
    groups[Animation::StageId::chit].anyHitShader       = VK_SHADER_UNUSED_KHR;
    groups[Animation::StageId::chit].closestHitShader   = Animation::StageId::chit;
    groups[Animation::StageId::chit].intersectionShader = VK_SHADER_UNUSED_KHR;
    groups[Animation::StageId::chit].type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;

    VkRayTracingPipelineCreateInfoKHR pipeline_info = { };
    pipeline_info.sType                          = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipeline_info.stageCount                     = Animation::StageId::count;
    pipeline_info.pStages                        = shader_stage_create_infos.data();
    pipeline_info.groupCount                     = Animation::StageId::count;
    pipeline_info.pGroups                        = groups.data();
    pipeline_info.maxPipelineRayRecursionDepth   = max_ray_tracing_recursive;
    pipeline_info.layout                         = _pipeline_layout_handle;

    auto func_table = VkUtils::getVulkanFunctionPointerTable();

    VK_CHECK(
         func_table.vkCreateRayTracingPipelinesKHR(
            ptr_context->device_handle, 
            VK_NULL_HANDLE, 
            VK_NULL_HANDLE, 
            1, &pipeline_info, 
            nullptr, 
            &_pipeline_handle
        )
    );

    for (auto shader_module: shader_modules)
        vkDestroyShaderModule(ptr_context->device_handle, shader_module, nullptr);
}

void Animation::destroyPipelineLayout()
{
    if (_pipeline_layout_handle != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(getContext()->device_handle, _pipeline_layout_handle, nullptr);
}

void Animation::destroyPipeline()
{
    if (_pipeline_handle != VK_NULL_HANDLE)
        vkDestroyPipeline(getContext()->device_handle, _pipeline_handle, nullptr);
}

void Animation::init() 
{
    RayTracingBase::init("Animation");

    initScene();
    initCamera();
    createPipelineLayout();
    createPipeline();
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

void Animation::swapchainImageToPresentUsgae(uint32_t image_index)
{
    auto command_buffer = getCommandBuffer();

    command_buffer.write([this, image_index](VkCommandBuffer command_buffer_handle)
    {
        VkImageSubresourceRange range = { };
        range.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseArrayLayer    = 0;
        range.layerCount        = 1;
        range.baseMipLevel      = 0;
        range.levelCount        = 1;

        VkImageMemoryBarrier image_memory_barrier = { };
        image_memory_barrier.sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.srcAccessMask          = VK_ACCESS_SHADER_WRITE_BIT;
        image_memory_barrier.dstAccessMask          = VK_ACCESS_NONE_KHR;
        image_memory_barrier.oldLayout              = VK_IMAGE_LAYOUT_GENERAL;
        image_memory_barrier.newLayout              = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        image_memory_barrier.srcQueueFamilyIndex    = VK_QUEUE_FAMILY_EXTERNAL_KHR;
        image_memory_barrier.dstQueueFamilyIndex    = VK_QUEUE_FAMILY_EXTERNAL_KHR;
        image_memory_barrier.image                  = _swapchain_image_handles[image_index];
        image_memory_barrier.subresourceRange       = range;

        vkCmdPipelineBarrier(
            command_buffer_handle, 
            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 
            0,
            0, nullptr,
            0, nullptr, 
            1, &image_memory_barrier
        );
    });

    command_buffer.upload(getContext());
}

void Animation::show()
{
    /// @todo 
    for (uint32_t i = 0; i < NUM_IMAGES_IN_SWAPCHAIN; ++i)
        swapchainImageToPresentUsgae(i);

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