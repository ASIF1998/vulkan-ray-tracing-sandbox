#include <animation/animation.hpp>

#include <base/vulkan/memory.hpp>

#include <base/scene/visitors/acceleration_structure_builder.hpp>
#include <base/scene/visitors/scene_geometry_references_getter.hpp>

#include <base/shader_compiler.hpp>

#include <animation/animation_pass.hpp>

using namespace animation;

Animation::~Animation()
{
    destroyPipelineLayout();
    destroyPipeline();
    destroyDescriptorSets();
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

        auto ptr_animation_pass_builder = std::make_unique<AnimationPass::Builder>(getContext());

        _scene->getModel().visit(ptr_animation_pass_builder);

        _animation_pass = ptr_animation_pass_builder->build();
    }
}

void Animation::initCamera()
{
    auto& camera = _scene->getCameraController().getCamera();
    camera.setDepthRange(0.0001f, 1000.0f);
    camera.lookaAt(glm::vec3(0, 200, 500), glm::vec3(0, 0, 1));
}

void Animation::createPipelineLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> bindings (3);
    bindings[0].binding         = Bindings::result;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR ;
    
    bindings[1].binding         = Bindings::acceleration_structure;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    bindings[1].stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR ;
    
    bindings[2].binding         = Bindings::scene_geometry;
    bindings[2].descriptorCount = 1;
    bindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    VkDescriptorSetLayoutCreateInfo set_layout_info = { };
    set_layout_info.sType           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set_layout_info.pBindings       = bindings.data();
    set_layout_info.bindingCount    = static_cast<uint32_t>(bindings.size());

    VK_CHECK(vkCreateDescriptorSetLayout(_context.device_handle, &set_layout_info, nullptr, &_descriptor_set_layout_handle));

    VkPushConstantRange push_constant_range = { };
    push_constant_range.offset      = 0;
    push_constant_range.size        = sizeof(PushConstants);
    push_constant_range.stageFlags  = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkPipelineLayoutCreateInfo pipeline_layout_info = { };
    pipeline_layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount         = 1;
    pipeline_layout_info.pSetLayouts            = &_descriptor_set_layout_handle;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges    = &push_constant_range;

    VK_CHECK(vkCreatePipelineLayout(_context.device_handle, &pipeline_layout_info, nullptr, &_pipeline_layout_handle));
}

void Animation::createDescriptorSets()
{
    std::vector<VkDescriptorPoolSize> pool_sizes;
    pool_sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1});
    pool_sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1});

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = { };
    descriptor_pool_create_info.sType           = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.maxSets         = 1;
    descriptor_pool_create_info.poolSizeCount   = static_cast<uint32_t>(pool_sizes.size());
    descriptor_pool_create_info.pPoolSizes      = pool_sizes.data();

    VK_CHECK(vkCreateDescriptorPool(_context.device_handle, &descriptor_pool_create_info, nullptr, &_descriptor_pool_handle));

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = { };
    descriptor_set_allocate_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.descriptorPool     = _descriptor_pool_handle;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts        = &_descriptor_set_layout_handle;

    VK_CHECK(vkAllocateDescriptorSets(_context.device_handle, &descriptor_set_allocate_info, &_descriptor_set_handle));

    VkUtils::setName(_context.device_handle, _descriptor_set_handle, VK_OBJECT_TYPE_DESCRIPTOR_SET, "Descritor set for ray tracing");
}

void Animation::createPipeline()
{
    const auto ptr_context = getContext();

    const auto animation_shaders_root = project_dir / "shaders/animation";

    std::array<VkShaderModule, StageId::count> shader_modules = { VK_NULL_HANDLE };
    shader_modules[StageId::ray_gen] = shader::Compiler::createShaderModule(ptr_context->device_handle, animation_shaders_root / "animation.glsl.rgen", shader::Type::raygen);
    shader_modules[StageId::miss]    = shader::Compiler::createShaderModule(ptr_context->device_handle, animation_shaders_root / "animation.glsl.rmiss", shader::Type::miss);
    shader_modules[StageId::chit]    = shader::Compiler::createShaderModule(ptr_context->device_handle, animation_shaders_root / "animation.glsl.rchit", shader::Type::closesthit);

    std::array<VkShaderStageFlagBits, StageId::count> stages = { };
    stages[StageId::ray_gen] = VK_SHADER_STAGE_RAYGEN_BIT_KHR; 
    stages[StageId::miss]    = VK_SHADER_STAGE_MISS_BIT_KHR; 
    stages[StageId::chit]    = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR; 

    std::array<VkPipelineShaderStageCreateInfo, StageId::count> shader_stage_create_infos = { };
    for (size_t i = 0; i < StageId::count; ++i)
    {
        shader_stage_create_infos[i].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage_create_infos[i].module = shader_modules[i];
        shader_stage_create_infos[i].pName  = "main";
        shader_stage_create_infos[i].stage  = stages[i];
    }
    
    std::array<VkRayTracingShaderGroupCreateInfoKHR, StageId::count> groups = { };

    groups[StageId::ray_gen].sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groups[StageId::ray_gen].generalShader      = StageId::ray_gen;
    groups[StageId::ray_gen].anyHitShader       = VK_SHADER_UNUSED_KHR;
    groups[StageId::ray_gen].closestHitShader   = VK_SHADER_UNUSED_KHR;
    groups[StageId::ray_gen].intersectionShader = VK_SHADER_UNUSED_KHR;
    groups[StageId::ray_gen].type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;

    groups[StageId::miss].sType                 = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groups[StageId::miss].generalShader         = StageId::miss;
    groups[StageId::miss].anyHitShader          = VK_SHADER_UNUSED_KHR;
    groups[StageId::miss].closestHitShader      = VK_SHADER_UNUSED_KHR;
    groups[StageId::miss].intersectionShader    = VK_SHADER_UNUSED_KHR;
    groups[StageId::miss].type                  = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    
    groups[StageId::chit].sType                 = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groups[StageId::chit].generalShader         = VK_SHADER_UNUSED_KHR;
    groups[StageId::chit].anyHitShader          = VK_SHADER_UNUSED_KHR;
    groups[StageId::chit].closestHitShader      = StageId::chit;
    groups[StageId::chit].intersectionShader    = VK_SHADER_UNUSED_KHR;
    groups[StageId::chit].type                  = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;

    VkRayTracingPipelineCreateInfoKHR pipeline_info = { };
    pipeline_info.sType                          = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipeline_info.stageCount                     = StageId::count;
    pipeline_info.pStages                        = shader_stage_create_infos.data();
    pipeline_info.groupCount                     = StageId::count;
    pipeline_info.pGroups                        = groups.data();
    pipeline_info.maxPipelineRayRecursionDepth   = _max_ray_tracing_recursive;
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

void Animation::createShaderBindingTable()
{
    constexpr uint32_t miss_count 	= 1;
	constexpr uint32_t hit_count 	= 1;

	constexpr uint32_t handle_count = 1 + miss_count + hit_count;

	uint32_t handle_size_aligned = VkUtils::getAlignedSize(
		_ray_tracing_pipeline_properties.shaderGroupHandleSize, 
		_ray_tracing_pipeline_properties.shaderGroupHandleAlignment	
	);

	uint32_t data_size = handle_count * handle_size_aligned;

	std::vector<uint8_t> raw_data (data_size);

	auto func_table = VkUtils::getVulkanFunctionPointerTable();

	VK_CHECK(
		func_table.vkGetRayTracingShaderGroupHandlesKHR(
			_context.device_handle,
			_pipeline_handle,
			0, StageId::count,
			data_size, raw_data.data()
		)
	);

	auto memory_type_index = MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (!memory_type_index.has_value())
		log::appError("Not memory index for create SBT.");

	auto createBufferForSBT = [this, &memory_type_index] (
		std::optional<Buffer>&		buffer, 
		const std::span<uint8_t> 	data,
		const std::string_view 		name
	)
	{
		buffer = Buffer::make(
			getContext(),
			_ray_tracing_pipeline_properties.shaderGroupHandleSize,
			*memory_type_index,
			VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			std::format("[SBT]: {}", name)
		);

		auto command_buffer = getCommandBuffer();

		Buffer::writeData(
			buffer.value(), 
			data, 
			command_buffer
		);
	};

	auto raygen_data 		= std::span(&raw_data[handle_size_aligned * StageId::ray_gen], _ray_tracing_pipeline_properties.shaderGroupHandleSize);
	auto closest_hit_data 	= std::span(&raw_data[handle_size_aligned * StageId::chit], _ray_tracing_pipeline_properties.shaderGroupHandleSize); 
	auto miss_data 			= std::span(&raw_data[handle_size_aligned * StageId::miss], _ray_tracing_pipeline_properties.shaderGroupHandleSize);

	createBufferForSBT(_sbt.raygen, raygen_data, "raygen");
	createBufferForSBT(_sbt.closest_hit, closest_hit_data, "closest hit");
	createBufferForSBT(_sbt.miss, miss_data, "miss");

	_sbt.raygen_region.deviceAddress 	= _sbt.raygen->getAddress();
	_sbt.raygen_region.stride 			= handle_size_aligned;
	_sbt.raygen_region.size 			= handle_size_aligned;

	_sbt.chit_region.deviceAddress 		= _sbt.closest_hit->getAddress();
	_sbt.raygen_region.stride 			= handle_size_aligned;
	_sbt.raygen_region.size 			= handle_size_aligned;
	
	_sbt.miss_region.deviceAddress 		= _sbt.miss->getAddress();
	_sbt.raygen_region.stride 			= handle_size_aligned;
	_sbt.raygen_region.size 			= handle_size_aligned;
}

void Animation::destroyPipelineLayout()
{
    if (_pipeline_layout_handle != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(getContext()->device_handle, _pipeline_layout_handle, nullptr);
}

void Animation::destroyDescriptorSets()
{
    if (_descriptor_set_layout_handle != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(_context.device_handle, _descriptor_set_layout_handle, nullptr);

    if (_descriptor_pool_handle != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(_context.device_handle, _descriptor_pool_handle, nullptr);
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
    createDescriptorSets();
    createShaderBindingTable();
    initVertexBufferReferences();
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

void Animation::processPushConstants()
{
    const auto& camera = _scene->getCameraController().getCamera();

    _push_constants.rgen.camera_data.inv_view       = camera.getInvViewMatrix();
    _push_constants.rgen.camera_data.inv_projection = camera.getInvProjection();
}

void Animation::swapchainImageToPresentUsage(uint32_t image_index)
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

void Animation::swapchainImageToGeneralUsage(uint32_t image_index)
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
        image_memory_barrier.srcAccessMask          = VK_ACCESS_NONE_KHR;
        image_memory_barrier.dstAccessMask          = VK_ACCESS_SHADER_WRITE_BIT;
        image_memory_barrier.oldLayout              = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        image_memory_barrier.newLayout              = VK_IMAGE_LAYOUT_GENERAL;
        image_memory_barrier.srcQueueFamilyIndex    = VK_QUEUE_FAMILY_EXTERNAL_KHR;
        image_memory_barrier.dstQueueFamilyIndex    = VK_QUEUE_FAMILY_EXTERNAL_KHR;
        image_memory_barrier.image                  = _swapchain_image_handles[image_index];
        image_memory_barrier.subresourceRange       = range;

        vkCmdPipelineBarrier(
            command_buffer_handle, 
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 
            0,
            0, nullptr,
            0, nullptr, 
            1, &image_memory_barrier
        );
    });

    command_buffer.upload(getContext());
}

void Animation::updateDescriptorSets(uint32_t image_index)
{
    std::array<VkWriteDescriptorSet, 3> write_infos = { };

    VkDescriptorImageInfo image_info = { };
    image_info.imageLayout  = VK_IMAGE_LAYOUT_GENERAL;
    image_info.imageView    = _swapchain_image_view_handles[image_index];

    write_infos[0] = { }; 
    write_infos[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_infos[0].descriptorCount  = 1;
    write_infos[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write_infos[0].dstArrayElement  = 0;
    write_infos[0].dstBinding       = Bindings::result;
    write_infos[0].dstSet           = _descriptor_set_handle;
    write_infos[0].dstSet           = _descriptor_set_handle;
    write_infos[0].pImageInfo       = &image_info;

    auto acceleration_structure_handle = _scene->getModel().getRootTLAS().value();

    VkWriteDescriptorSetAccelerationStructureKHR write_acceleration_structure_info = { };
	write_acceleration_structure_info.sType 						= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	write_acceleration_structure_info.accelerationStructureCount 	= 1;
    write_acceleration_structure_info.pAccelerationStructures       = &acceleration_structure_handle;

    write_infos[1] = { }; 
    write_infos[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_infos[1].pNext           = &write_acceleration_structure_info;
    write_infos[1].descriptorCount = 1;
    write_infos[1].descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    write_infos[1].dstArrayElement = 0;
    write_infos[1].dstBinding      = Bindings::acceleration_structure;
    write_infos[1].dstSet          = _descriptor_set_handle;

    VkDescriptorBufferInfo buffer_info = { };
	buffer_info.buffer 	= _vertex_buffers_references.scene_info_reference->vk_handle;
	buffer_info.offset 	= 0;
	buffer_info.range 	= sizeof(VkDeviceAddress) * 2;

    write_infos[2] = { };
    write_infos[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_infos[2].descriptorCount  = 1;
    write_infos[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write_infos[2].dstBinding       = Bindings::scene_geometry;
    write_infos[2].dstSet           = _descriptor_set_handle;
    write_infos[2].pBufferInfo      = &buffer_info;

    vkUpdateDescriptorSets(
        _context.device_handle,
        static_cast<uint32_t>(write_infos.size()), write_infos.data(),
        0, nullptr
    );
}

void Animation::initVertexBufferReferences()
{
    auto ptr_visitor = std::make_unique<SceneGeometryReferencesGetter>(getContext());

	_scene->getModel().visit(ptr_visitor);

	_vertex_buffers_references.scene_geometries_ref	= ptr_visitor->getVertexBuffersReferences();
	_vertex_buffers_references.scene_indices_ref	= ptr_visitor->getIndexBuffersReferences();
	
	if (auto memory_index = MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
	{
		_vertex_buffers_references.scene_info_reference = Buffer::make(
			getContext(),
			sizeof(VkDeviceAddress) * 2,
			*memory_index,
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			"Pointer to vertex buffers references"
		);
	}
	else 
		log::appError("Not memory index for create vertex buffers references.");

	auto command_buffer = getCommandBuffer();

	std::array<VkDeviceAddress, 2> references
	{
		_vertex_buffers_references.scene_geometries_ref->getAddress(),
		_vertex_buffers_references.scene_indices_ref->getAddress()
	};

	Buffer::writeData<VkDeviceAddress>(
		*_vertex_buffers_references.scene_info_reference, 
		std::span(references),
		command_buffer
	);
}

void Animation::updateTime()
{
    static std::chrono::high_resolution_clock timer;

    static auto begin   = timer.now();
    static auto end     = begin;

    end = timer.now();

    _time.delta = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(end - begin).count();

    begin = end;
}

void Animation::animationPass()
{
    _scene->getAnimator().update(_time.delta);

    _animation_pass->process();

    /// @todo build AS from skinned mesh
    auto ptr_as_builder = std::make_unique<ASBuilder>(getContext());

    auto& model = _scene->getModel();
    model.visit(ptr_as_builder);
}

void Animation::show()
{
    while (processEvents())
    {
        updateTime();

        _window->setTitle(std::format("Animation, {}ms", _time.delta));

        animationPass();

        auto image_index = getNextImageIndex();

        processPushConstants();

        swapchainImageToGeneralUsage(image_index);
        updateDescriptorSets(image_index);
        _scene->updateCamera();

        auto draw_command_buffer = getCommandBuffer();
        draw_command_buffer.write([this](VkCommandBuffer command_buffer_handle) 
        {
            auto func_table = VkUtils::getVulkanFunctionPointerTable();
            auto [width, height] = _window->getSize();

            vkCmdBindPipeline(
				command_buffer_handle,
				VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
				_pipeline_handle
			);

            vkCmdBindDescriptorSets(
                command_buffer_handle, 
                VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, 
                _pipeline_layout_handle, 
                0, 1, &_descriptor_set_handle, 
                0, nullptr
            );

            vkCmdPushConstants(
				command_buffer_handle,
				_pipeline_layout_handle, VK_SHADER_STAGE_RAYGEN_BIT_KHR,
				0, sizeof(PushConstants::RayGen), &_push_constants.rgen
			);

            func_table.vkCmdTraceRaysKHR(
				command_buffer_handle,
				&_sbt.raygen_region,
				&_sbt.miss_region,
				&_sbt.chit_region,
				&_sbt.callable_region,
				width, height, 1
			);
        });

        draw_command_buffer.upload(getContext());

        swapchainImageToPresentUsage(image_index);

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
    /// @todo
} 