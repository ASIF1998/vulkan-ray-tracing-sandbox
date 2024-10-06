#include <hello_triangle/hello_triangle.hpp>

#include <base/vulkan/utils.hpp>
#include <base/vulkan/memory.hpp>
#include <base/shader_compiler.hpp>

#include <ranges>

using namespace hello_triangle;

HelloTriangle::~HelloTriangle()
{
    destroyDescriptorSets();
    destroyShaders();
    destroyPipeline();
}

void HelloTriangle::createPipeline()
{
    std::array<VkPipelineShaderStageCreateInfo, ShaderID::count> shaders_stage_infos = { };

    shaders_stage_infos[ShaderID::ray_gen] = { };
    shaders_stage_infos[ShaderID::ray_gen].sType   = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaders_stage_infos[ShaderID::ray_gen].stage   = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    shaders_stage_infos[ShaderID::ray_gen].module  = _shader_modules[ShaderID::ray_gen];
    shaders_stage_infos[ShaderID::ray_gen].pName   = "main";

    shaders_stage_infos[ShaderID::chit] = { };
    shaders_stage_infos[ShaderID::chit].sType   = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaders_stage_infos[ShaderID::chit].stage   = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    shaders_stage_infos[ShaderID::chit].module  = _shader_modules[ShaderID::chit];
    shaders_stage_infos[ShaderID::chit].pName   = "main";

    shaders_stage_infos[ShaderID::miss] = { };
    shaders_stage_infos[ShaderID::miss].sType   = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaders_stage_infos[ShaderID::miss].stage   = VK_SHADER_STAGE_MISS_BIT_KHR;
    shaders_stage_infos[ShaderID::miss].module  = _shader_modules[ShaderID::miss];
    shaders_stage_infos[ShaderID::miss].pName   = "main";
 
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> group;
    {
        group.reserve(ShaderID::count);

        VkRayTracingShaderGroupCreateInfoKHR ray_tracing_shader_group_create_info = { };
        ray_tracing_shader_group_create_info.sType               = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        ray_tracing_shader_group_create_info.type                = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        ray_tracing_shader_group_create_info.generalShader       = VK_SHADER_UNUSED_KHR;
        ray_tracing_shader_group_create_info.closestHitShader    = VK_SHADER_UNUSED_KHR;
        ray_tracing_shader_group_create_info.anyHitShader        = VK_SHADER_UNUSED_KHR;
        ray_tracing_shader_group_create_info.intersectionShader  = VK_SHADER_UNUSED_KHR;

        group.push_back(ray_tracing_shader_group_create_info);
        group.back().generalShader = ShaderID::ray_gen;

        group.push_back(ray_tracing_shader_group_create_info);
        group.back().generalShader  = ShaderID::miss;

        group.push_back(ray_tracing_shader_group_create_info);
        group.back().generalShader      = VK_SHADER_UNUSED_KHR;
        group.back().closestHitShader   = ShaderID::chit;
        group.back().type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;

    }

    VkPipelineLayoutCreateInfo  pipeline_layout_create_info = { };
    pipeline_layout_create_info.sType           = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount  = 1;
    pipeline_layout_create_info.pSetLayouts     = &_descriptor_set_layout_handle;

    log::appInfo("Create pipeline layout");

    VK_CHECK(
        vkCreatePipelineLayout(
            _context.device_handle, 
            &pipeline_layout_create_info, 
            nullptr, 
            &_pipeline_layout_handle
        )
    );

    VkRayTracingPipelineCreateInfoKHR pipeline_create_info = { };
    pipeline_create_info.sType                          = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipeline_create_info.stageCount                     = ShaderID::count;
    pipeline_create_info.pStages                        = shaders_stage_infos.data();
    pipeline_create_info.groupCount                     = static_cast<uint32_t>(group.size());
    pipeline_create_info.pGroups                        = group.data();
    pipeline_create_info.maxPipelineRayRecursionDepth   = 1;
    pipeline_create_info.layout                         = _pipeline_layout_handle;

    auto func_table = VkUtils::getVulkanFunctionPointerTable();

    log::appInfo("Create ray tracing pipeline");

    VK_CHECK(
        func_table.vkCreateRayTracingPipelinesKHR(
            _context.device_handle, 
            VK_NULL_HANDLE, 
            VK_NULL_HANDLE, 
            1, &pipeline_create_info, 
            nullptr, 
            &_pipeline_handle
        )
    );
}

void HelloTriangle::createAS()
{
    createBLAS();
    createTLAS();
}

void HelloTriangle::createBLAS()
{
    auto memory_type_index = MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (!memory_type_index.has_value())
        log::appError("Not memory index for create buffers.");

    auto transform_matrix_buffer = Buffer::make(
        getContext(), 
        sizeof(VkTransformMatrixKHR), 
        *memory_type_index, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, 
        "Transform matrix buffer"
    );

    {
        auto transform_matrix = VkUtils::getVulkanIdentityMatrix();

        auto command_buffer = getCommandBuffer();

        Buffer::writeData(transform_matrix_buffer, transform_matrix, command_buffer);
    }

    VkAccelerationStructureGeometryKHR blas_geometry = { };
    blas_geometry.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    blas_geometry.flags         = VK_GEOMETRY_OPAQUE_BIT_KHR;
    blas_geometry.geometryType  = VK_GEOMETRY_TYPE_TRIANGLES_KHR;

    auto& geometry = blas_geometry.geometry;
    geometry.triangles.sType                          = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.triangles.vertexFormat                   = VK_FORMAT_R32G32B32_SFLOAT;
    geometry.triangles.vertexData.deviceAddress       = _mesh.vertex_buffer->getAddress();
    geometry.triangles.maxVertex                      = 3;
    geometry.triangles.vertexStride                   = sizeof(glm::vec3);
    geometry.triangles.indexType                      = VK_INDEX_TYPE_UINT32;
    geometry.triangles.indexData.deviceAddress        = _mesh.index_buffer->getAddress();
    geometry.triangles.transformData.deviceAddress    = transform_matrix_buffer.getAddress();

    VkAccelerationStructureBuildGeometryInfoKHR blas_build_geometry_info = { };
    blas_build_geometry_info.sType          = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    blas_build_geometry_info.type           = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    blas_build_geometry_info.flags          = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    blas_build_geometry_info.geometryCount  = 1;
    blas_build_geometry_info.pGeometries    = &blas_geometry;

    const uint32_t num_triangles = 1;

    VkAccelerationStructureBuildSizesInfoKHR blas_build_sizes = { };
    blas_build_sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    auto func_table = VkUtils::getVulkanFunctionPointerTable();

    func_table.vkGetAccelerationStructureBuildSizesKHR(
        _context.device_handle, 
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, 
        &blas_build_geometry_info, 
        &num_triangles, 
        &blas_build_sizes
    );

    auto blas_buffer = Buffer::make(
        getContext(), 
        blas_build_sizes.accelerationStructureSize, 
        *memory_type_index, 
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
        "Blas buffer"
    );

    VkAccelerationStructureCreateInfoKHR blas_create_info = { };
    blas_create_info.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    blas_create_info.buffer = blas_buffer.vk_handle;
    blas_create_info.size   = blas_build_sizes.accelerationStructureSize;
    blas_create_info.type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    VkAccelerationStructureKHR blas_handle = VK_NULL_HANDLE;

    log::appInfo("Create BLAS");

    VK_CHECK(
        func_table.vkCreateAccelerationStructureKHR(
            _context.device_handle, 
            &blas_create_info, 
            nullptr, 
            &blas_handle
        )
    );

    VkUtils::setName(
        _context.device_handle,
        blas_handle, 
        VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, 
        "Triangle BLAS"
    );

    auto command_buffer = getCommandBuffer();

    auto scratch_buffer = Buffer::make(
        getContext(), 
        blas_build_sizes.buildScratchSize, 
        *memory_type_index, 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
        "Blas scratch buffer"
    );

    command_buffer.write([&scratch_buffer, &func_table, &blas_geometry, &blas_handle] (VkCommandBuffer vk_handle)
    {
        VkAccelerationStructureBuildRangeInfoKHR blas_build_range_info = { };
        blas_build_range_info.primitiveCount = 1;

        const auto ptr_blas_build_range_info = &blas_build_range_info;

        VkAccelerationStructureBuildGeometryInfoKHR blas_build_geometry_info = { };
        blas_build_geometry_info.sType                      = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        blas_build_geometry_info.type                       = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        blas_build_geometry_info.flags                      = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        blas_build_geometry_info.mode                       = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        blas_build_geometry_info.dstAccelerationStructure   = blas_handle;
        blas_build_geometry_info.geometryCount              = 1;
        blas_build_geometry_info.pGeometries                = &blas_geometry;

        blas_build_geometry_info.scratchData.deviceAddress  = scratch_buffer.getAddress();

        func_table.vkCmdBuildAccelerationStructuresKHR(
            vk_handle, 
            1, 
            &blas_build_geometry_info, 
            &ptr_blas_build_range_info
        );
    });

    command_buffer.upload(getContext());

    _blas = AccelerationStructure(getContext(), blas_handle, std::move(blas_buffer));
}

void HelloTriangle::createTLAS()
{
    auto memory_type_index = MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (!memory_type_index.has_value())
        log::appError("Not memory index for create buffers.");
    
    auto instance_buffer = Buffer::make(
        getContext(), 
        sizeof(VkAccelerationStructureInstanceKHR), 
        *memory_type_index,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
        "Instance buffer"
    );

    auto func_table = VkUtils::getVulkanFunctionPointerTable();;

    {
        VkAccelerationStructureDeviceAddressInfoKHR blas_address_info = { };
        blas_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        blas_address_info.accelerationStructure = _blas->vk_handle;

        VkAccelerationStructureInstanceKHR as_instance = { };
        as_instance.transform = VkUtils::getVulkanIdentityMatrix();
        as_instance.instanceCustomIndex                     = 0;
        as_instance.accelerationStructureReference          = func_table.vkGetAccelerationStructureDeviceAddressKHR(_context.device_handle, &blas_address_info);
        as_instance.flags                                   = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        as_instance.mask                                    = 0xff;             // Попадание только в том случае, если rayMask и instance.mask != 0
        as_instance.instanceShaderBindingTableRecordOffset  = 0;                // Мы будем использовать одну и ту же группу хитов для всех объектов

        auto command_buffer = getCommandBuffer();

        Buffer::writeData(instance_buffer, as_instance, command_buffer);
    }

    auto instance_buffer_address = instance_buffer.getAddress();

    VkAccelerationStructureGeometryInstancesDataKHR as_geometry_instance_data = { };
    as_geometry_instance_data.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    as_geometry_instance_data.data.deviceAddress    = instance_buffer_address;

    VkAccelerationStructureGeometryKHR as_geometry = { };
    as_geometry.sType               = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    as_geometry.geometryType        = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    as_geometry.geometry.instances  = as_geometry_instance_data;

    constexpr uint32_t geomtry_count = 1;
    
    VkAccelerationStructureBuildGeometryInfoKHR geomerty_build_info = { };
    geomerty_build_info.sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    geomerty_build_info.geometryCount   = geomtry_count;
    geomerty_build_info.pGeometries     = &as_geometry;
    geomerty_build_info.mode            = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    geomerty_build_info.type            = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

    VkAccelerationStructureBuildSizesInfoKHR tlas_build_sizes = { };
    tlas_build_sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    func_table.vkGetAccelerationStructureBuildSizesKHR(
        _context.device_handle, 
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, 
        &geomerty_build_info, 
        &geomtry_count, 
        &tlas_build_sizes
    );

    auto tlas_buffer = Buffer::make(
        getContext(), 
        tlas_build_sizes.accelerationStructureSize, 
        *memory_type_index,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
        "Tlas buffer"
    );

    VkAccelerationStructureCreateInfoKHR tlas_create_info = { };
    tlas_create_info.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    tlas_create_info.size   = tlas_build_sizes.accelerationStructureSize;
    tlas_create_info.type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    tlas_create_info.buffer = tlas_buffer.vk_handle;

    VkAccelerationStructureKHR tlas_handle = VK_NULL_HANDLE;
    
    log::appInfo("Create TLAS");

    VK_CHECK(
        func_table.vkCreateAccelerationStructureKHR(
            _context.device_handle, 
            &tlas_create_info, 
            nullptr, 
            &tlas_handle
        )
    );

    VkUtils::setName(
        _context.device_handle, 
        tlas_handle, 
        VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, 
        "TLAS"
    );

    auto scratch_buffer = Buffer::make(
        getContext(), 
        tlas_build_sizes.buildScratchSize, 
        *memory_type_index,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
        "Tlas scratch buffer"
    );

    geomerty_build_info.dstAccelerationStructure    = tlas_handle;
    geomerty_build_info.scratchData.deviceAddress   = scratch_buffer.getAddress();

    auto command_buffer_for_build_tlas = getCommandBuffer();

    command_buffer_for_build_tlas.write([&func_table, geomerty_build_info] (VkCommandBuffer vk_handle)
    {
        const uint32_t primitive_count = 1;

        VkAccelerationStructureBuildRangeInfoKHR tlas_build_range_info = { };
        tlas_build_range_info.firstVertex       = 0;
        tlas_build_range_info.primitiveCount    = primitive_count;
        tlas_build_range_info.primitiveOffset   = 0;
        tlas_build_range_info.transformOffset   = 0;

        const auto ptr_tlas_build_range_info = &tlas_build_range_info;

        func_table.vkCmdBuildAccelerationStructuresKHR(
            vk_handle, 
            1, &geomerty_build_info, 
            &ptr_tlas_build_range_info
        );
    });

    command_buffer_for_build_tlas.upload(getContext());

    _tlas = std::make_optional<AccelerationStructure>(getContext(), tlas_handle, std::move(tlas_buffer));
}

void HelloTriangle::compileShaders()
{
    std::array shaders
    {
        project_dir / "shaders/hello_triangle/hello_triangle.glsl.rgen",
        project_dir / "shaders/hello_triangle/hello_triangle.glsl.rmiss",
        project_dir / "shaders/hello_triangle/hello_triangle.glsl.rchit"
    };

    constexpr std::array shader_types 
    {
        shader::Type::raygen,
        shader::Type::miss,
        shader::Type::closesthit
    };

    for (auto i: std::views::iota(0u, ShaderID::count))
    {
        _shader_modules[i] = shader::Compiler::createShaderModule(_context.device_handle, shaders[i], shader_types[i]);

        VkUtils::setName(
            _context.device_handle, 
            _shader_modules[i],
            VK_OBJECT_TYPE_SHADER_MODULE, 
            shaders[i].filename().string()
        );
    }
}

void HelloTriangle::createShaderBindingTable()
{
    log::appInfo("Create SBT");

    auto handle_size_aligned = VkUtils::getAlignedSize(
        _ray_tracing_pipeline_properties.shaderGroupHandleSize, 
        _ray_tracing_pipeline_properties.shaderGroupHandleAlignment
    );

    constexpr uint32_t miss_count   = 1;
    constexpr uint32_t hit_count    = 1;
        
    constexpr uint32_t handle_count = 1 + miss_count + hit_count;
    
    const uint32_t data_size = handle_count * handle_size_aligned;

    std::vector<uint8_t> handles (data_size);

    auto func_table = VkUtils::getVulkanFunctionPointerTable();

    VK_CHECK(
        func_table.vkGetRayTracingShaderGroupHandlesKHR(
            _context.device_handle, 
            _pipeline_handle, 
            0, 
            handle_count, 
            data_size, 
            handles.data()
        )
    );

    auto memory_type_index = MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (!memory_type_index.has_value())
        log::appError("Not memory index for create SBT.");

    auto createBufferForSBT = [this, &memory_type_index] (
		std::optional<Buffer>&      buffer, 
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

    auto raygen_data        = std::span(&handles[0], _ray_tracing_pipeline_properties.shaderGroupHandleSize);
    auto miss_data          = std::span(&handles[handle_size_aligned], _ray_tracing_pipeline_properties.shaderGroupHandleSize);
    auto closest_hit_data   = std::span(&handles[handle_size_aligned * 2], _ray_tracing_pipeline_properties.shaderGroupHandleSize);

    createBufferForSBT(_sbt._raygen_shader_binding_table, raygen_data, "raygen");
    createBufferForSBT(_sbt._miss_shader_binding_table, miss_data, "miss");
    createBufferForSBT(_sbt._chit_shader_binding_table, closest_hit_data, "closest hit");

    _sbt.raygen_region.deviceAddress 	= _sbt._raygen_shader_binding_table->getAddress();
	_sbt.raygen_region.stride 			= handle_size_aligned;
	_sbt.raygen_region.size 			= handle_size_aligned;

	_sbt.chit_region.deviceAddress 		= _sbt._chit_shader_binding_table->getAddress();
	_sbt.raygen_region.stride 			= handle_size_aligned;
	_sbt.raygen_region.size 			= handle_size_aligned;
	
	_sbt.miss_region.deviceAddress 		= _sbt._miss_shader_binding_table->getAddress();
	_sbt.raygen_region.stride 			= handle_size_aligned;
	_sbt.raygen_region.size 			= handle_size_aligned;
}

void HelloTriangle::creareDescriptorSets()
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    
    bindings.push_back({ });
    bindings.back().binding         = 0;
    bindings.back().descriptorCount = 1;
    bindings.back().descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    bindings.back().stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    bindings.push_back({ });
    bindings.back().binding         = 1;
    bindings.back().descriptorCount = 1;
    bindings.back().descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings.back().stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = { };
    descriptor_set_layout_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.bindingCount  = static_cast<uint32_t>(bindings.size());
    descriptor_set_layout_create_info.pBindings     = bindings.data();

    VK_CHECK(
        vkCreateDescriptorSetLayout(
            _context.device_handle, 
            &descriptor_set_layout_create_info, 
            nullptr, 
            &_descriptor_set_layout_handle
        )
    );

    std::array<VkDescriptorPoolSize, 2> pool_sizes = { };

    pool_sizes[0].type              = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    pool_sizes[0].descriptorCount   = 1;
    
    pool_sizes[1].type              = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    pool_sizes[1].descriptorCount   = 1;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = { };
    descriptor_pool_create_info.sType           = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.poolSizeCount   = static_cast<uint32_t>(pool_sizes.size());
    descriptor_pool_create_info.pPoolSizes      = pool_sizes.data();
    descriptor_pool_create_info.maxSets         = static_cast<uint32_t>(_descriptor_set_handles.size());

    VK_CHECK(
        vkCreateDescriptorPool(
            _context.device_handle, 
            &descriptor_pool_create_info, 
            nullptr, 
            &_descriptor_pool_handle
        )
    );

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = { };
    descriptor_set_allocate_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.descriptorPool     = _descriptor_pool_handle;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts        = &_descriptor_set_layout_handle;

    for (auto i: std::views::iota(0u, _descriptor_set_handles.size()))
        VK_CHECK(vkAllocateDescriptorSets(_context.device_handle, &descriptor_set_allocate_info, &_descriptor_set_handles[i]));
}

void HelloTriangle::updateDescriptorSets()
{
    VkDescriptorImageInfo storage_image_descriptor{};
    storage_image_descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet result_image_write = { };
    result_image_write.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    result_image_write.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    result_image_write.dstBinding       = 1;
    result_image_write.pImageInfo       = &storage_image_descriptor;
    result_image_write.descriptorCount  = 1;

    for (auto i: std::views::iota(0u, _descriptor_set_handles.size()))
    {
        storage_image_descriptor.imageView  = _swapchain_image_view_handles[i];
        result_image_write.dstSet           = _descriptor_set_handles[i];

        vkUpdateDescriptorSets(_context.device_handle, 1, &result_image_write, 0, VK_NULL_HANDLE);
    }

    VkWriteDescriptorSetAccelerationStructureKHR as_write_info = { };
    as_write_info.sType                         = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    as_write_info.accelerationStructureCount    = 1;
    as_write_info.pAccelerationStructures       = &_tlas->vk_handle;

    VkWriteDescriptorSet write_descriptor_set_info = { };
    write_descriptor_set_info.sType             = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set_info.pNext             = &as_write_info;
    write_descriptor_set_info.dstBinding        = 0;
    write_descriptor_set_info.descriptorCount   = 1;
    write_descriptor_set_info.descriptorType    = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    for (auto descriptor_set_handle: _descriptor_set_handles)
    {
        write_descriptor_set_info.dstSet = descriptor_set_handle;
        vkUpdateDescriptorSets(_context.device_handle, 1, &write_descriptor_set_info, 0, nullptr);
    }
}

struct RgbaChannelIds
{
    enum
    {
        R,
        G,
        B,
        A
    };
};

void HelloTriangle::initMesh()
{
    log::appInfo("Create triangle");
    
    _mesh.vertices.push_back(glm::vec3(1, 1, 0));
    _mesh.vertices.push_back(glm::vec3(-1, 1, 0));
    _mesh.vertices.push_back(glm::vec3(0, -1, 0));

    _mesh.indices.push_back(0);
    _mesh.indices.push_back(1);
    _mesh.indices.push_back(2);

    if (auto memory_type_index = MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
    {
        uint32_t vertex_buffer_size = static_cast<uint32_t>(sizeof(glm::vec3) * _mesh.vertices.size());
        uint32_t index_buffer_size  = static_cast<uint32_t>(sizeof(uint32_t) * _mesh.indices.size());

        auto shared_usage_flags = 
                VK_BUFFER_USAGE_TRANSFER_DST_BIT 
            |   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT 
            |   VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

        _mesh.vertex_buffer = Buffer::make(
            getContext(),
            vertex_buffer_size, 
            *memory_type_index,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | shared_usage_flags,
            "Vertex buffer"
        );

        _mesh.index_buffer = Buffer::make(
            getContext(),
            index_buffer_size, 
            *memory_type_index,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | shared_usage_flags,
            "Index buffer"
        );
    }
    else 
        log::appError("Not memory index for create mesh index and vertex buffers.");

    auto command_buffer_for_copy_vertices   = getCommandBuffer();
    auto command_buffer_for_copy_indices    = getCommandBuffer();

    Buffer::writeData(_mesh.vertex_buffer.value(), std::span(_mesh.vertices), command_buffer_for_copy_vertices);
    Buffer::writeData(_mesh.index_buffer.value(), std::span(_mesh.indices), command_buffer_for_copy_indices);
}

void HelloTriangle::init()
{
    RayTracingBase::init("Hello Triangle");

    initMesh();
    createAS();
    compileShaders();
    creareDescriptorSets();
    createPipeline();
    createShaderBindingTable();

    updateDescriptorSets();
    buildCommandBuffers();
}

void HelloTriangle::resizeWindow()
{
    VK_CHECK(vkDeviceWaitIdle(_context.device_handle));

    destroySwapchainImageViews();
    destroySwapchain();

    createSwapchain();
    getSwapchainImages();
    createSwapchainImageViews();

    updateDescriptorSets();
    buildCommandBuffers();
}

void HelloTriangle::buildCommandBuffers()
{
    /// build command buffers for ray tracing
    for (auto image_index: std::views::iota(0u, NUM_IMAGES_IN_SWAPCHAIN))
    {
        _ray_tracing_launch[image_index] = getCommandBuffer();

        _ray_tracing_launch[image_index]->write([this, image_index] (VkCommandBuffer command_buffer_handle)
        {
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
                0, 1, 
                &_descriptor_set_handles[image_index], 
                0, nullptr
            );

            auto func_table = VkUtils::getVulkanFunctionPointerTable();

            func_table.vkCmdTraceRaysKHR(
                command_buffer_handle, 
                &_sbt.raygen_region,
				&_sbt.miss_region,
				&_sbt.chit_region,
				&_sbt.callable_region,
                width, height, 1
            );
        });
    }

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

    for (auto image_index: std::views::iota(0u, NUM_IMAGES_IN_SWAPCHAIN))
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

void HelloTriangle::show()
{
    bool stay = true;

    SDL_Event event = { };
    
    VkResult result = VK_SUCCESS;

    VkPresentInfoKHR present_info = { };
    present_info.sType          = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pResults       = &result;
    present_info.pSwapchains    = &_swapchain_handle;
    present_info.swapchainCount = 1;

    while (stay)
    {
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_QUIT:
                    stay = false;
                    break;   
                case SDL_KEYUP:
                    if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                        stay = false;
                    break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOW_RESIZABLE)
                        resizeWindow();
                    break;
            }
        }

        auto image_index = getNextImageIndex();
        if (auto is_resize_window = image_index == std::numeric_limits<uint32_t>::max(); is_resize_window)
            continue;

        _ray_tracing_launch[image_index]->upload(getContext());

        _swapchain.general_to_present_layout[image_index]->upload(getContext());

        present_info.pImageIndices = &image_index;

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

void HelloTriangle::destroyDescriptorSets()
{
    if (_descriptor_pool_handle != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(_context.device_handle, _descriptor_pool_handle, nullptr);

    if (_descriptor_set_layout_handle != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(_context.device_handle, _descriptor_set_layout_handle, nullptr);
}

void HelloTriangle::destroyShaders()
{
    for (auto shader_module: _shader_modules)
    {
        if (shader_module != VK_NULL_HANDLE)
            vkDestroyShaderModule(_context.device_handle, shader_module, nullptr);
    }
}

void HelloTriangle::destroyPipeline()
{
    if (_pipeline_layout_handle != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(_context.device_handle, _pipeline_layout_handle, nullptr);

    if (_pipeline_handle != VK_NULL_HANDLE)
        vkDestroyPipeline(_context.device_handle, _pipeline_handle, nullptr);
}