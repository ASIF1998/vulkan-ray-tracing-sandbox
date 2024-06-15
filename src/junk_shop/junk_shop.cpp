#include <junk_shop/junk_shop.hpp>
#include <base/scene/visitors/acceleration_structure_builder.hpp>
#include <base/scene/visitors/scene_geometry_references_getter.hpp>

#include <base/shader_compiler.hpp>

#include <ranges>

JunkShop::~JunkShop()
{
	destroyPipeline();
	destroyDescriptorSets();
	destroyShaders();
}

void JunkShop::init()
{
	RayTracingBase::init("JunkShop");

	compileShaders();
	
	importScene();
	initCamera();

	createAccumulationBuffer();
	createPipeline();
	createShaderBindingTable();
	createDescriptorSets();

	buildCommandBuffers();
}

void JunkShop::resizeWindow()
{
	VK_CHECK(vkDeviceWaitIdle(_context.device_handle));

	auto [width, height] = _window->getSize();
	_scene->getCameraController().getCamera().setSize(static_cast<float>(width), static_cast<float>(height));
	
	destroySwapchainImageViews();
	destroySwapchain();

	createSwapchain();
	getSwapchainImages();
	createSwapchainImageViews();
	createAccumulationBuffer();

	buildCommandBuffers();
}

VkDescriptorImageInfo JunkShop::createDescriptorImageInfo(const Image& image)
{
	VkDescriptorImageInfo image_info = { };
	image_info.imageLayout 	= VK_IMAGE_LAYOUT_GENERAL;
	image_info.imageView 	= image.view_handle;
	image_info.sampler 		= image.sampler_handle;

	return image_info;
}

VkDescriptorImageInfo JunkShop::createDescriptorImageInfo(VkImageView image_view_handle)
{
	VkDescriptorImageInfo image_info = { };
	image_info.imageLayout 	= VK_IMAGE_LAYOUT_GENERAL;
	image_info.imageView 	= image_view_handle;
	image_info.sampler 		= VK_NULL_HANDLE;

	return image_info;
}

auto JunkShop::createDescriptorBufferInfo(VkBuffer buffer_handle, VkDeviceSize size, VkDeviceSize offset)
	-> VkDescriptorBufferInfo
{
	VkDescriptorBufferInfo 	buffer_info = { };
	buffer_info.buffer 	= buffer_handle;
	buffer_info.offset 	= offset;
	buffer_info.range 	= size;

	return buffer_info;
}

void JunkShop::updateDescriptorSet(uint32_t image_index)
{
	auto& material_manager = _scene->getModel().getMaterialManager();

	VkWriteDescriptorSetAccelerationStructureKHR write_acceleration_structure_info = { };
	write_acceleration_structure_info.sType 						= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	write_acceleration_structure_info.accelerationStructureCount 	= 1;

	if (auto root_tlas_handle = _scene->getModel().getRootTLAS())
		write_acceleration_structure_info.pAccelerationStructures = &(*root_tlas_handle);
	else 
		log::appError("Acceleartion structure not created.");
	
	/*	--------------- rendering result ----------------------	*/
	auto result_image_info = createDescriptorImageInfo(_swapchain_image_view_handles[image_index]);

	/*	------------------------------------------------------	*/
	/*	--------------- accumulated buffer ----------------------	*/
	auto accumulated_buffer_info = createDescriptorImageInfo(_accumulation_buffer->view_handle);

	/*	------------------------------------------------------	*/
	/*	---------------- scene geometry	----------------------	*/
	auto buffer_info = createDescriptorBufferInfo(_vertex_buffers_references.scene_info_reference->vk_handle, sizeof(VkDeviceAddress) * 2);

	/*	------------------------------------------------------	*/
	/*	--------------------	materials	------------------	*/
	const auto& materials = material_manager.getMaterials();

	std::vector<VkDescriptorImageInfo> albedos_infos 		(materials.size());
	std::vector<VkDescriptorImageInfo> normal_maps_infos 	(materials.size());
	std::vector<VkDescriptorImageInfo> metallic_infos 		(materials.size());
	std::vector<VkDescriptorImageInfo> roughness_infos 		(materials.size());
	std::vector<VkDescriptorImageInfo> emissive_infos 		(materials.size());

	for (auto i: std::views::iota(0u, materials.size()))
	{
		albedos_infos[i] 		= createDescriptorImageInfo(materials[i].albedo.value());
		normal_maps_infos[i] 	= createDescriptorImageInfo(materials[i].normal_map.value());
		metallic_infos[i] 		= createDescriptorImageInfo(materials[i].metallic.value());
		roughness_infos[i] 		= createDescriptorImageInfo(materials[i].roughness.value());
		emissive_infos[i] 		= createDescriptorImageInfo(materials[i].emissive.value());
	}

	/*	------------------------------------------------------	*/
	std::array<VkWriteDescriptorSet, DescriptorSets::count> write_infos;

    for (auto i: std::views::iota(0u, DescriptorSets::count))
	{
		write_infos[i] = { };
		write_infos[i].sType 			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_infos[i].descriptorCount	= 1;
		write_infos[i].dstArrayElement 	= 0;
		write_infos[i].dstBinding 		= i;
		write_infos[i].dstSet 			= _descriptor_set;
	}

	write_infos[DescriptorSets::acceleration_structure].descriptorType 	= VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	write_infos[DescriptorSets::acceleration_structure].pNext 			= &write_acceleration_structure_info;

	write_infos[DescriptorSets::storage_image].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	write_infos[DescriptorSets::storage_image].pImageInfo 		= &result_image_info;
	
	write_infos[DescriptorSets::accumulated_buffer].descriptorType	= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	write_infos[DescriptorSets::accumulated_buffer].pImageInfo		= &accumulated_buffer_info;

	write_infos[DescriptorSets::ssbo].descriptorType 	= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write_infos[DescriptorSets::ssbo].pBufferInfo		= &buffer_info;

	write_infos[DescriptorSets::albedos].descriptorCount	= static_cast<uint32_t>(albedos_infos.size());
	write_infos[DescriptorSets::albedos].pImageInfo 		= albedos_infos.data();
	write_infos[DescriptorSets::albedos].descriptorType 	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	write_infos[DescriptorSets::normal_maps].descriptorCount	= static_cast<uint32_t>(normal_maps_infos.size());
	write_infos[DescriptorSets::normal_maps].pImageInfo 		= normal_maps_infos.data();
	write_infos[DescriptorSets::normal_maps].descriptorType 	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	write_infos[DescriptorSets::metallic].descriptorCount	= static_cast<uint32_t>(metallic_infos.size());
	write_infos[DescriptorSets::metallic].pImageInfo 		= metallic_infos.data();
	write_infos[DescriptorSets::metallic].descriptorType 	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	write_infos[DescriptorSets::roughness].descriptorCount	= static_cast<uint32_t>(roughness_infos.size());
	write_infos[DescriptorSets::roughness].pImageInfo 		= roughness_infos.data();
	write_infos[DescriptorSets::roughness].descriptorType 	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	
	write_infos[DescriptorSets::emissive].descriptorCount	= static_cast<uint32_t>(emissive_infos.size());
	write_infos[DescriptorSets::emissive].pImageInfo 		= emissive_infos.data();
	write_infos[DescriptorSets::emissive].descriptorType 	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	
	vkUpdateDescriptorSets(
		_context.device_handle, 
		static_cast<uint32_t>(write_infos.size()), write_infos.data(), 
		0, nullptr
	);
}

bool JunkShop::processEvents()
{
	SDL_Event event = { };

	auto is_move = [] (auto current_scancode)
	{
		constexpr std::array scancodes = 
		{
			SDL_SCANCODE_W,
			SDL_SCANCODE_A,
			SDL_SCANCODE_S,
			SDL_SCANCODE_D,
			SDL_SCANCODE_SPACE,
			SDL_SCANCODE_LSHIFT
		};

		for (auto scancode: scancodes)
		{
			if (current_scancode == scancode)
				return true;
		}

		return false;
	};

	while (SDL_PollEvent(&event))
	{
		_scene->processEvent(&event);

		switch (event.type)
		{
			case SDL_QUIT:
				return false;
			case SDL_KEYUP:
				if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
					return false;
				_draw_stay = DrawStay::draw;
				break;
			case SDL_MOUSEBUTTONUP:
				_draw_stay = DrawStay::draw;
				break;
			case SDL_KEYDOWN:
			case SDL_MOUSEBUTTONDOWN:
				_draw_stay = DrawStay::clear;
				break;
			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					resizeWindow();
					_draw_stay = DrawStay::clear;
				}
				break;
		}
	}

	return true;
}

void JunkShop::show()
{
	log::appInfo("Sample running !!!");

	while (processEvents())
	{
		if (_draw_stay == DrawStay::clear)
		{
			_accumulated_frames_count = 0;
			_draw_stay = DrawStay::draw;
		}
		else
			++_accumulated_frames_count;

		_scene->updateCamera();
		
		auto image_index = getNextImageIndex();
		if (auto is_resize_window = image_index == std::numeric_limits<uint32_t>::max(); is_resize_window)
			continue;

		updateDescriptorSet(image_index);

		auto command_buffer_for_trace_ray = getCommandBuffer();

		command_buffer_for_trace_ray.write([this](VkCommandBuffer command_buffer_handle)
		{
			auto push_constant_data = getPushConstantData();

			auto func_table = VkUtils::getVulkanFunctionPointerTable();

			auto [width, height] = _window->getSize();

			vkCmdBindPipeline(
				command_buffer_handle,
				VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
				_pipeline
			);

			vkCmdBindDescriptorSets(
				command_buffer_handle,
				VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
				_pipeline_layout,
				0, 1, &_descriptor_set,
				0, nullptr
			);

			vkCmdPushConstants(
				command_buffer_handle,
				_pipeline_layout, VK_SHADER_STAGE_RAYGEN_BIT_KHR,
				0, sizeof(PushConstants::RayGen), &push_constant_data.rgen_consts
			);

			vkCmdPushConstants(
				command_buffer_handle,
				_pipeline_layout, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
				sizeof(PushConstants::RayGen), sizeof(PushConstants::ClosestHit), &push_constant_data.chit_consts
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

		command_buffer_for_trace_ray.upload(getContext());

		VkResult result = VK_RESULT_MAX_ENUM;

		VkPresentInfoKHR present_info = { };
		present_info.sType			= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.pImageIndices	= &image_index;
		present_info.pResults		= &result;
		present_info.pSwapchains	= &_swapchain_handle;
		present_info.swapchainCount = 1;

		_swapchain.general_to_present_layout[image_index]->upload(getContext());

		auto present_result = vkQueuePresentKHR(_context.queue.handle, &present_info);

		if (auto is_resize_window = present_result == VK_ERROR_OUT_OF_DATE_KHR; is_resize_window)
			continue;
		else 
		{
			VK_CHECK(present_result);
			VK_CHECK(result);
		}

		_swapchain.present_layout_to_general[image_index]->upload(getContext());
	}
}

auto JunkShop::getPipelineDescriptorSetsBindings() const
	-> std::array<VkDescriptorSetLayoutBinding, DescriptorSets::count>
{
	auto& materials = _scene->getModel().getMaterialManager().getMaterials();

	std::array<VkDescriptorSetLayoutBinding, DescriptorSets::count> bindings;

	bindings[DescriptorSets::acceleration_structure] = { };
	bindings[DescriptorSets::acceleration_structure].binding 			= DescriptorSets::acceleration_structure;
	bindings[DescriptorSets::acceleration_structure].descriptorCount 	= 1;
	bindings[DescriptorSets::acceleration_structure].descriptorType 	= VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	bindings[DescriptorSets::acceleration_structure].stageFlags 		= VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	bindings[DescriptorSets::storage_image] = { };
	bindings[DescriptorSets::storage_image].binding 			= DescriptorSets::storage_image;
	bindings[DescriptorSets::storage_image].descriptorCount 	= 1;
	bindings[DescriptorSets::storage_image].descriptorType 		= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	bindings[DescriptorSets::storage_image].stageFlags 			= VK_SHADER_STAGE_RAYGEN_BIT_KHR;

	bindings[DescriptorSets::accumulated_buffer] = { };
	bindings[DescriptorSets::accumulated_buffer].binding 			= DescriptorSets::accumulated_buffer;
	bindings[DescriptorSets::accumulated_buffer].descriptorCount 	= 1;
	bindings[DescriptorSets::accumulated_buffer].descriptorType		= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	bindings[DescriptorSets::accumulated_buffer].stageFlags			= VK_SHADER_STAGE_RAYGEN_BIT_KHR;

	bindings[DescriptorSets::ssbo] = { };
	bindings[DescriptorSets::ssbo].binding 				= DescriptorSets::ssbo;
	bindings[DescriptorSets::ssbo].descriptorCount 		= 1;
	bindings[DescriptorSets::ssbo].descriptorType 		= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[DescriptorSets::ssbo].stageFlags 			= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	bindings[DescriptorSets::albedos] = { };
	bindings[DescriptorSets::albedos].binding			= DescriptorSets::albedos;
	bindings[DescriptorSets::albedos].descriptorCount	= static_cast<uint32_t>(materials.size());
	bindings[DescriptorSets::albedos].descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[DescriptorSets::albedos].stageFlags		= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	bindings[DescriptorSets::normal_maps] = { };
	bindings[DescriptorSets::normal_maps].binding			= DescriptorSets::normal_maps;
	bindings[DescriptorSets::normal_maps].descriptorCount	= static_cast<uint32_t>(materials.size());
	bindings[DescriptorSets::normal_maps].descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[DescriptorSets::normal_maps].stageFlags		= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	bindings[DescriptorSets::metallic] = { };
	bindings[DescriptorSets::metallic].binding			= DescriptorSets::metallic;
	bindings[DescriptorSets::metallic].descriptorCount	= static_cast<uint32_t>(materials.size());
	bindings[DescriptorSets::metallic].descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[DescriptorSets::metallic].stageFlags		= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	bindings[DescriptorSets::roughness] = { };
	bindings[DescriptorSets::roughness].binding			= DescriptorSets::roughness;
	bindings[DescriptorSets::roughness].descriptorCount	= static_cast<uint32_t>(materials.size());
	bindings[DescriptorSets::roughness].descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[DescriptorSets::roughness].stageFlags		= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	
	bindings[DescriptorSets::emissive] = { };
	bindings[DescriptorSets::emissive].binding			= DescriptorSets::emissive;
	bindings[DescriptorSets::emissive].descriptorCount	= static_cast<uint32_t>(materials.size());
	bindings[DescriptorSets::emissive].descriptorType	= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[DescriptorSets::emissive].stageFlags		= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	return bindings;
}

void JunkShop::createPipelineLayout()
{
	auto bindings = getPipelineDescriptorSetsBindings();

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = { };
	descriptor_set_layout_create_info.sType 		= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_set_layout_create_info.bindingCount 	= static_cast<uint32_t>(bindings.size());
	descriptor_set_layout_create_info.pBindings 	= bindings.data();

	VK_CHECK(
		vkCreateDescriptorSetLayout(
			_context.device_handle,
			&descriptor_set_layout_create_info,
			nullptr,
			&_descriptor_set_layout_handle
		)
	);

	std::array<VkPushConstantRange, 2> push_constants_ranges;

	push_constants_ranges[ShaderId::ray_gen] = { };
	push_constants_ranges[ShaderId::ray_gen].size 			= sizeof(PushConstants::RayGen);
	push_constants_ranges[ShaderId::ray_gen].stageFlags 	= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	push_constants_ranges[ShaderId::ray_gen].offset 		= 0;

	push_constants_ranges[ShaderId::chit] = { };
	push_constants_ranges[ShaderId::chit].size 			= sizeof(PushConstants::ClosestHit);
	push_constants_ranges[ShaderId::chit].stageFlags 	= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	push_constants_ranges[ShaderId::chit].offset 		= sizeof(PushConstants::RayGen);

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = { };
	pipeline_layout_create_info.sType 					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount 			= 1;
	pipeline_layout_create_info.pSetLayouts 			= &_descriptor_set_layout_handle;
	pipeline_layout_create_info.pushConstantRangeCount 	= static_cast<uint32_t>(push_constants_ranges.size());
	pipeline_layout_create_info.pPushConstantRanges 	= push_constants_ranges.data();

	VK_CHECK(
		vkCreatePipelineLayout(
			_context.device_handle,
			&pipeline_layout_create_info,
			nullptr,
			&_pipeline_layout
		)
	);
}

void JunkShop::createPipeline()
{
	createPipelineLayout();

	std::array<VkPipelineShaderStageCreateInfo, ShaderId::count> shader_stages_info = { };

	shader_stages_info[ShaderId::ray_gen] = { };
	shader_stages_info[ShaderId::ray_gen].sType 	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages_info[ShaderId::ray_gen].module 	= _shader_modules[ShaderId::ray_gen];
	shader_stages_info[ShaderId::ray_gen].pName	 	= "main";
	shader_stages_info[ShaderId::ray_gen].stage		= VK_SHADER_STAGE_RAYGEN_BIT_KHR;

	shader_stages_info[ShaderId::chit] = { };
	shader_stages_info[ShaderId::chit].sType 	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages_info[ShaderId::chit].module 	= _shader_modules[ShaderId::chit];
	shader_stages_info[ShaderId::chit].pName 	= "main";
	shader_stages_info[ShaderId::chit].stage 	= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	shader_stages_info[ShaderId::miss] = { };
	shader_stages_info[ShaderId::miss].sType 	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages_info[ShaderId::miss].module 	= _shader_modules[ShaderId::miss];
	shader_stages_info[ShaderId::miss].pName 	= "main";
	shader_stages_info[ShaderId::miss].stage 	= VK_SHADER_STAGE_MISS_BIT_KHR;

	std::array<VkRayTracingShaderGroupCreateInfoKHR, ShaderId::count> groups;

	groups[ShaderId::ray_gen] = { };
	groups[ShaderId::ray_gen].sType					= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	groups[ShaderId::ray_gen].generalShader 		= ShaderId::ray_gen;
	groups[ShaderId::ray_gen].closestHitShader		= VK_SHADER_UNUSED_KHR;
	groups[ShaderId::ray_gen].anyHitShader			= VK_SHADER_UNUSED_KHR;
	groups[ShaderId::ray_gen].intersectionShader	= VK_SHADER_UNUSED_KHR;
	groups[ShaderId::ray_gen].type 					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;

	groups[ShaderId::miss] = { };
	groups[ShaderId::miss].sType				= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	groups[ShaderId::miss].generalShader  		= ShaderId::miss;
	groups[ShaderId::miss].closestHitShader		= VK_SHADER_UNUSED_KHR;
	groups[ShaderId::miss].anyHitShader			= VK_SHADER_UNUSED_KHR;
	groups[ShaderId::miss].intersectionShader	= VK_SHADER_UNUSED_KHR;
	groups[ShaderId::miss].type 				= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;

	groups[ShaderId::chit] = { };
	groups[ShaderId::chit].sType 				= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	groups[ShaderId::chit].closestHitShader		= ShaderId::chit;
	groups[ShaderId::chit].anyHitShader			= VK_SHADER_UNUSED_KHR;
	groups[ShaderId::chit].intersectionShader	= VK_SHADER_UNUSED_KHR;
	groups[ShaderId::chit].generalShader		= VK_SHADER_UNUSED_KHR;
	groups[ShaderId::chit].type 				= VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;

	VkRayTracingPipelineCreateInfoKHR pipeline_create_info = { };
	pipeline_create_info.sType 							= VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	pipeline_create_info.layout 						= _pipeline_layout;
	pipeline_create_info.maxPipelineRayRecursionDepth 	= 6;
	pipeline_create_info.stageCount 					= static_cast<uint32_t>(ShaderId::count);
	pipeline_create_info.pStages 						= shader_stages_info.data();
	pipeline_create_info.groupCount						= static_cast<uint32_t>(groups.size());
	pipeline_create_info.pGroups						= groups.data();

	auto func_table = VkUtils::getVulkanFunctionPointerTable();

	VK_CHECK(
		func_table.vkCreateRayTracingPipelinesKHR(
			_context.device_handle,
			VK_NULL_HANDLE,
			VK_NULL_HANDLE,
			1, &pipeline_create_info,
			nullptr,
			&_pipeline
		)
	);
}

void JunkShop::compileShaders()
{
	std::array<std::filesystem::path, ShaderId::count> shaders;
	shaders[ShaderId::ray_gen] 	= project_dir / "shaders/junk_shop/junk_shop.glsl.rgen";
	shaders[ShaderId::chit] 	= project_dir / "shaders/junk_shop/junk_shop.glsl.rchit";
	shaders[ShaderId::miss] 	= project_dir / "shaders/junk_shop/junk_shop.glsl.rmiss";

	std::array<shader::Type, ShaderId::count> types;
	types[ShaderId::ray_gen] 	= shader::Type::raygen;
	types[ShaderId::chit] 		= shader::Type::closesthit;
	types[ShaderId::miss] 		= shader::Type::miss;

    for (auto i: std::views::iota(0u, ShaderId::count))
	{
		_shader_modules[i] = shader::Compiler::createShaderModule(_context.device_handle, shaders[i], types[i]);

		VkUtils::setName(
			_context.device_handle, 
			_shader_modules[i],
			VK_OBJECT_TYPE_SHADER_MODULE, 
			shaders[i].filename().string()
		);
	}
}

void JunkShop::createShaderBindingTable()
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
			_pipeline,
			0, ShaderId::count,
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
			command_buffer, 
			_context.queue.handle
		);
	};

	auto raygen_data 		= std::span(&raw_data[handle_size_aligned * ShaderId::ray_gen], _ray_tracing_pipeline_properties.shaderGroupHandleSize);
	auto closest_hit_data 	= std::span(&raw_data[handle_size_aligned * ShaderId::chit], _ray_tracing_pipeline_properties.shaderGroupHandleSize); 
	auto miss_data 			= std::span(&raw_data[handle_size_aligned * ShaderId::miss], _ray_tracing_pipeline_properties.shaderGroupHandleSize);

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

auto JunkShop::getPoolSizes() const
	-> std::array<VkDescriptorPoolSize, DescriptorSets::count>
{
	const auto& materials = _scene->getModel().getMaterialManager().getMaterials();

	std::array<VkDescriptorPoolSize, DescriptorSets::count> pool_sizes;

	pool_sizes[DescriptorSets::storage_image] = { };
	pool_sizes[DescriptorSets::storage_image].type				= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	pool_sizes[DescriptorSets::storage_image].descriptorCount 	= 1;
	
	pool_sizes[DescriptorSets::accumulated_buffer] = { };
	pool_sizes[DescriptorSets::accumulated_buffer].type				= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	pool_sizes[DescriptorSets::accumulated_buffer].descriptorCount 	= 1;

	pool_sizes[DescriptorSets::acceleration_structure] = { };
	pool_sizes[DescriptorSets::acceleration_structure].type				= VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	pool_sizes[DescriptorSets::acceleration_structure].descriptorCount 	= 1;

	pool_sizes[DescriptorSets::ssbo] = { };
	pool_sizes[DescriptorSets::ssbo].type				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_sizes[DescriptorSets::ssbo].descriptorCount 	= 1;

	pool_sizes[DescriptorSets::albedos] = { };
	pool_sizes[DescriptorSets::albedos].type			= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[DescriptorSets::albedos].descriptorCount	= static_cast<uint32_t>(materials.size());

	pool_sizes[DescriptorSets::normal_maps] = { };
	pool_sizes[DescriptorSets::normal_maps].type			= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[DescriptorSets::normal_maps].descriptorCount	= static_cast<uint32_t>(materials.size());

	pool_sizes[DescriptorSets::metallic] = { };
	pool_sizes[DescriptorSets::metallic].type				= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[DescriptorSets::metallic].descriptorCount	= static_cast<uint32_t>(materials.size());

	pool_sizes[DescriptorSets::roughness] = { };
	pool_sizes[DescriptorSets::roughness].type				= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[DescriptorSets::roughness].descriptorCount	= static_cast<uint32_t>(materials.size());
	
	pool_sizes[DescriptorSets::emissive] = { };
	pool_sizes[DescriptorSets::emissive].type				= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[DescriptorSets::emissive].descriptorCount	= static_cast<uint32_t>(materials.size());

	return pool_sizes;
}

void JunkShop::createDescriptorSets()
{
	auto pool_sizes = getPoolSizes();

	VkDescriptorPoolCreateInfo descriptor_pool_create_info = { };
	descriptor_pool_create_info.sType 			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_create_info.maxSets 		= 1;
	descriptor_pool_create_info.poolSizeCount 	= static_cast<uint32_t>(pool_sizes.size());
	descriptor_pool_create_info.pPoolSizes 		= pool_sizes.data();

	VK_CHECK(
		vkCreateDescriptorPool(
			_context.device_handle,
			&descriptor_pool_create_info,
			nullptr,
			&_descriptor_pool
		)
	);

	VkDescriptorSetAllocateInfo descriptor_set_allocate_info = { };
	descriptor_set_allocate_info.sType 				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_allocate_info.descriptorPool 	= _descriptor_pool;
	descriptor_set_allocate_info.descriptorSetCount = 1;
	descriptor_set_allocate_info.pSetLayouts  		= &_descriptor_set_layout_handle;
	
	VK_CHECK(
		vkAllocateDescriptorSets(
			_context.device_handle,
			&descriptor_set_allocate_info,
			&_descriptor_set
		)
	);
}

void JunkShop::createAS()
{
	auto ptr_as_builder = std::make_unique<ASBuilder>(getContext());

	auto& model = _scene->getModel();
	model.visit(ptr_as_builder);

	initVertexBuffersReferences();
}

void JunkShop::createAccumulationBuffer()
{
	auto [width, height] = _window->getSize();

	auto command_buffer = getCommandBuffer();

	_accumulation_buffer = Image::Builder(getContext())
		.generateMipmap(false)
		.size(width, height)
		.vkFormat(VK_FORMAT_R32G32B32A32_SFLOAT)
		.build();

	VkUtils::setName(
		_context.device_handle, 
		*_accumulation_buffer,
		VK_OBJECT_TYPE_IMAGE, 
		"Accumulation buffer"
	);
}

void JunkShop::importScene()
{
	auto [width, height] = _window->getSize();

	if (auto memory_index = MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
	{
		_scene = Scene::Importer(getContext())
			.path(project_dir / "content/Blender 2.glb")
			.vkMemoryTypeIndex(*memory_index)
			.viewport(width, height)
			.import();

		_scene->addRect(
			getContext(), 
			glm::vec2(30, 10), 
			glm::vec3(1, 1, 0), 
			glm::vec3(2500), 
			glm::translate(glm::mat4(1), glm::vec3(-20, 10, 20)) *
			glm::rotate(glm::mat4(1), glm::radians(-45.0f), glm::vec3(1, 0, 0))
		);
	}
	else 
		log::appError("Not memory index for scene importer.");

	_scene->applyTransform(glm::scale(glm::mat4(1), glm::vec3(100)));

	createAS();
}

void JunkShop::initVertexBuffersReferences()
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
		command_buffer, _context.queue.handle
	);
}

void JunkShop::initCamera()
{
	auto& camera = _scene->getCameraController().getCamera();

	camera.setDepthRange(0.01f, 1000.0f);
	camera.lookaAt(glm::vec3(50, 1823.898, 5133.947),glm::vec3(-0.5, 0, 1));
}

PushConstants JunkShop::getPushConstantData()
{
	PushConstants constants;

	const auto& camera = _scene->getCameraController().getCamera();

	constants.rgen_consts.camera_data.inv_view_matrix 		= camera.getInvViewMatrix();
	constants.rgen_consts.camera_data.inv_projection_matrix = camera.getInvProjection();
	constants.rgen_consts.accumulated_frames_count			= _accumulated_frames_count;

	constants.chit_consts.eye_to_pixel_cone_spread_angle = camera.getEyeToPixelConeSpreadAngle();

	return constants; 
}

void JunkShop::buildCommandBuffers()
{
	/// build command buffers for switch images layout from swapchain
    VkImageMemoryBarrier image_barrier = { };
    image_barrier.sType                             = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier.oldLayout                         = VK_IMAGE_LAYOUT_GENERAL;
    image_barrier.newLayout                         = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    image_barrier.srcQueueFamilyIndex               = 0;
    image_barrier.dstQueueFamilyIndex               = 0;
    image_barrier.subresourceRange.aspectMask       = VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange.baseArrayLayer   = 0;
    image_barrier.subresourceRange.layerCount       = 1;
    image_barrier.subresourceRange.baseMipLevel     = 0;
    image_barrier.subresourceRange.levelCount       = 1;

    for (size_t image_index = 0; image_index < NUM_IMAGES_IN_SWAPCHAIN; ++image_index)
    {
        _swapchain.general_to_present_layout[image_index] = getCommandBuffer();
        _swapchain.present_layout_to_general[image_index] = getCommandBuffer();

        image_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        image_barrier.dstAccessMask = 0;
        image_barrier.image         = _swapchain_image_handles[image_index];

        _swapchain.general_to_present_layout[image_index]->write([&image_barrier] (VkCommandBuffer handle)
        {
            vkCmdPipelineBarrier(
                handle,
                VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 
                0, nullptr,
                0, nullptr,
                1, &image_barrier 
            );
        });

        std::swap(image_barrier.oldLayout, image_barrier.newLayout);
        image_barrier.srcAccessMask = 0;
        image_barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        _swapchain.present_layout_to_general[image_index]->write([&image_barrier] (VkCommandBuffer handle)
        {
            vkCmdPipelineBarrier(
                handle,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 
                0, nullptr,
                0, nullptr,
                1, &image_barrier 
            );
        });

        std::swap(image_barrier.oldLayout, image_barrier.newLayout);
    }
}

void JunkShop::destroyDescriptorSets()
{
	if (_descriptor_pool != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(_context.device_handle, _descriptor_pool, nullptr);
}

void JunkShop::destroyShaders()
{
	for (auto shader_module_handle: _shader_modules)
	{
		if (shader_module_handle != VK_NULL_HANDLE)
			vkDestroyShaderModule(_context.device_handle, shader_module_handle, nullptr);
	}
}

void JunkShop::destroyPipeline()
{
	if (_descriptor_set_layout_handle != VK_NULL_HANDLE)
		vkDestroyDescriptorSetLayout(_context.device_handle, _descriptor_set_layout_handle, nullptr);

	if (_pipeline_layout != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(_context.device_handle, _pipeline_layout, nullptr);

	if (_pipeline != VK_NULL_HANDLE)
		vkDestroyPipeline(_context.device_handle, _pipeline, nullptr);
}