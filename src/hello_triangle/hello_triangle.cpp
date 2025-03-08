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

        const VkRayTracingShaderGroupCreateInfoKHR ray_tracing_shader_group_create_info 
        { 
            .sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
            .type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
            .generalShader      = VK_SHADER_UNUSED_KHR,
            .closestHitShader   = VK_SHADER_UNUSED_KHR,
            .anyHitShader       = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
        };

        group.push_back(ray_tracing_shader_group_create_info);
        group.back().generalShader = ShaderID::ray_gen;

        group.push_back(ray_tracing_shader_group_create_info);
        group.back().generalShader = ShaderID::miss;

        group.push_back(ray_tracing_shader_group_create_info);
        group.back().generalShader      = VK_SHADER_UNUSED_KHR;
        group.back().closestHitShader   = ShaderID::chit;
        group.back().type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    }

    const VkPipelineLayoutCreateInfo  pipeline_layout_create_info 
    { 
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts    = &_descriptor_set_layout_handle
    };

    log::info("Create pipeline layout");

    VK_CHECK(
        vkCreatePipelineLayout(
            _context.device_handle, 
            &pipeline_layout_create_info, 
            nullptr, 
            &_pipeline_layout_handle
        )
    );

    const VkRayTracingPipelineCreateInfoKHR pipeline_create_info 
    { 
        .sType                          = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
        .stageCount                     = ShaderID::count,
        .pStages                        = shaders_stage_infos.data(),
        .groupCount                     = static_cast<uint32_t>(group.size()),
        .pGroups                        = group.data(),
        .maxPipelineRayRecursionDepth   = 1,
        .layout                         = _pipeline_layout_handle
    };

    auto func_table = VkUtils::getVulkanFunctionPointerTable();

    log::info("Create ray tracing pipeline");

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
    auto transform_matrix_buffer = Buffer::Builder(getContext())
        .vkSize(sizeof(VkTransformMatrixKHR))
        .vkUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR)
        .name("Transform matrix buffer")
        .build();

    Buffer::writeData(transform_matrix_buffer, VkUtils::getVulkanIdentityMatrix());

    VkAccelerationStructureGeometryKHR blas_geometry 
    { 
        .sType          = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType   = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
        .flags          = VK_GEOMETRY_OPAQUE_BIT_KHR
    };

    auto& geometry = blas_geometry.geometry;
    geometry.triangles.sType                          = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.triangles.vertexFormat                   = VK_FORMAT_R32G32B32_SFLOAT;
    geometry.triangles.vertexData.deviceAddress       = _mesh.vertex_buffer->getAddress();
    geometry.triangles.maxVertex                      = 3;
    geometry.triangles.vertexStride                   = sizeof(glm::vec3);
    geometry.triangles.indexType                      = VK_INDEX_TYPE_UINT32;
    geometry.triangles.indexData.deviceAddress        = _mesh.index_buffer->getAddress();
    geometry.triangles.transformData.deviceAddress    = transform_matrix_buffer.getAddress();

    const VkAccelerationStructureBuildGeometryInfoKHR blas_build_geometry_info 
    { 
        .sType          = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type           = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags          = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .geometryCount  = 1,
        .pGeometries    = &blas_geometry
    };

    const uint32_t num_triangles = 1;

    VkAccelerationStructureBuildSizesInfoKHR blas_build_sizes 
    { 
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
    };

    auto func_table = VkUtils::getVulkanFunctionPointerTable();

    func_table.vkGetAccelerationStructureBuildSizesKHR(
        _context.device_handle, 
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, 
        &blas_build_geometry_info, 
        &num_triangles, 
        &blas_build_sizes
    );

    auto blas_buffer = Buffer::Builder(getContext())
        .vkSize(blas_build_sizes.accelerationStructureSize)
        .vkUsage(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        .name("Blas buffer")
        .build();

    const VkAccelerationStructureCreateInfoKHR blas_create_info 
    { 
        .sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = blas_buffer.vk_handle,
        .size   = blas_build_sizes.accelerationStructureSize,
        .type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
    };

    VkAccelerationStructureKHR blas_handle = VK_NULL_HANDLE;

    log::info("Create BLAS");

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

    auto scratch_buffer = Buffer::Builder(getContext())
        .vkSize(blas_build_sizes.buildScratchSize)
        .vkUsage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        .name("Blas scratch buffer")
        .build();

    command_buffer.write([&scratch_buffer, &func_table, &blas_geometry, &blas_handle] (VkCommandBuffer vk_handle)
    {
        constexpr VkAccelerationStructureBuildRangeInfoKHR blas_build_range_info 
        { 
            .primitiveCount = 1
        };

        const auto ptr_blas_build_range_info = &blas_build_range_info;

        const VkAccelerationStructureBuildGeometryInfoKHR blas_build_geometry_info 
        { 
            .sType                      = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .type                       = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
            .flags                      = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
            .mode                       = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
            .dstAccelerationStructure   = blas_handle,
            .geometryCount              = 1,
            .pGeometries                = &blas_geometry,
            .scratchData                = {.deviceAddress  = scratch_buffer.getAddress()}
        };

        func_table.vkCmdBuildAccelerationStructuresKHR(
            vk_handle, 
            1, 
            &blas_build_geometry_info, 
            &ptr_blas_build_range_info
        );
    }, "Build BLAS", GpuMarkerColors::build_blas);

    command_buffer.upload(getContext());

    _blas = AccelerationStructure(getContext(), blas_handle, std::move(blas_buffer));
}

void HelloTriangle::createTLAS()
{
    auto instance_buffer = Buffer::Builder(getContext())
        .vkSize(sizeof(VkAccelerationStructureInstanceKHR))
        .vkUsage(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
        .name("Instance buffer")
        .build();

    auto func_table = VkUtils::getVulkanFunctionPointerTable();;

    {
        const VkAccelerationStructureDeviceAddressInfoKHR blas_address_info 
        { 
            .sType                  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
            .accelerationStructure  = _blas->vk_handle,
        };

        const VkAccelerationStructureInstanceKHR as_instance 
        { 
            .transform = VkUtils::getVulkanIdentityMatrix(),
            .instanceCustomIndex                     = 0,
            .mask                                    = 0xff,             // Попадание только в том случае, если rayMask и instance.mask != 0
            .instanceShaderBindingTableRecordOffset  = 0,                // Мы будем использовать одну и ту же группу хитов для всех объектов
            .flags                                   = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
            .accelerationStructureReference          = func_table.vkGetAccelerationStructureDeviceAddressKHR(_context.device_handle, &blas_address_info)
        };

        Buffer::writeData(instance_buffer, as_instance);
    }

    const auto instance_buffer_address = instance_buffer.getAddress();

    const VkAccelerationStructureGeometryInstancesDataKHR as_geometry_instance_data 
    { 
        .sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
        .data   = {.deviceAddress = instance_buffer_address}
    };

    const VkAccelerationStructureGeometryKHR as_geometry 
    { 
        .sType          = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType   = VK_GEOMETRY_TYPE_INSTANCES_KHR,
        .geometry       = {.instances = as_geometry_instance_data}
    };

    constexpr uint32_t geomtry_count = 1;
    
    VkAccelerationStructureBuildGeometryInfoKHR geomerty_build_info 
    { 
        .sType          = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type           = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .mode           = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .geometryCount  = geomtry_count,
        .pGeometries    = &as_geometry,
    };

    VkAccelerationStructureBuildSizesInfoKHR tlas_build_sizes 
    { 
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
    };

    func_table.vkGetAccelerationStructureBuildSizesKHR(
        _context.device_handle, 
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, 
        &geomerty_build_info, 
        &geomtry_count, 
        &tlas_build_sizes
    );

    auto tlas_buffer = Buffer::Builder(getContext())
        .vkSize(tlas_build_sizes.accelerationStructureSize)
        .vkUsage(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        .name("Tlas buffer")
        .build();

    const VkAccelerationStructureCreateInfoKHR tlas_create_info 
    { 
        .sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = tlas_buffer.vk_handle,
        .size   = tlas_build_sizes.accelerationStructureSize,
        .type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR
    };

    VkAccelerationStructureKHR tlas_handle = VK_NULL_HANDLE;
    
    log::info("Create TLAS");

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

    auto scratch_buffer = Buffer::Builder(getContext())
        .vkSize(tlas_build_sizes.buildScratchSize)
        .vkUsage(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        .name("Tlas scratch buffer")
        .build();

    geomerty_build_info.dstAccelerationStructure    = tlas_handle;
    geomerty_build_info.scratchData.deviceAddress   = scratch_buffer.getAddress();

    auto command_buffer_for_build_tlas = getCommandBuffer();

    command_buffer_for_build_tlas.write([&func_table, geomerty_build_info] (VkCommandBuffer vk_handle)
    {
        constexpr uint32_t primitive_count = 1;

        constexpr VkAccelerationStructureBuildRangeInfoKHR tlas_build_range_info 
        { 
            .primitiveCount    = primitive_count,
            .primitiveOffset   = 0,
            .firstVertex       = 0,
            .transformOffset   = 0
        };

        const auto ptr_tlas_build_range_info = &tlas_build_range_info;

        func_table.vkCmdBuildAccelerationStructuresKHR(
            vk_handle, 
            1, &geomerty_build_info, 
            &ptr_tlas_build_range_info
        );
    }, "Build TLAS", GpuMarkerColors::build_tlas);

    command_buffer_for_build_tlas.upload(getContext());

    _tlas = std::make_optional<AccelerationStructure>(getContext(), tlas_handle, std::move(tlas_buffer));
}

void HelloTriangle::compileShaders()
{
    const std::array shaders
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
    log::info("Create SBT");

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

    auto createBufferForSBT = [this] (
		std::optional<Buffer>&      buffer, 
		const std::span<uint8_t> 	data,
		const std::string_view 		name
	)
	{
        buffer = Buffer::Builder(getContext())
            .vkSize(_ray_tracing_pipeline_properties.shaderGroupHandleSize)
            .vkUsage(VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
            .name(std::format("[SBT]: {}", name))
            .build();

		Buffer::writeData(buffer.value(), data);
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
    constexpr std::array bindings
    {
        VkDescriptorSetLayoutBinding
        {
            .binding         = 0,
            .descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .descriptorCount = 1,
            .stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR
        },
        VkDescriptorSetLayoutBinding
        { 
            .binding         = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR
        }
    };

    const VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info 
    { 
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount  = static_cast<uint32_t>(bindings.size()),
        .pBindings     = bindings.data()
    };

    VK_CHECK(
        vkCreateDescriptorSetLayout(
            _context.device_handle, 
            &descriptor_set_layout_create_info, 
            nullptr, 
            &_descriptor_set_layout_handle
        )
    );

    constexpr std::array pool_sizes 
    { 
        VkDescriptorPoolSize
        {
            .type              = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .descriptorCount   = 1
        },
        VkDescriptorPoolSize
        {
            .type              = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount   = 1
        }
    };

    const VkDescriptorPoolCreateInfo descriptor_pool_create_info 
    { 
        .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets        = static_cast<uint32_t>(_descriptor_set_handles.size()),
        .poolSizeCount  = static_cast<uint32_t>(pool_sizes.size()),
        .pPoolSizes     = pool_sizes.data()
    };

    VK_CHECK(
        vkCreateDescriptorPool(
            _context.device_handle, 
            &descriptor_pool_create_info, 
            nullptr, 
            &_descriptor_pool_handle
        )
    );

    const VkDescriptorSetAllocateInfo descriptor_set_allocate_info 
    { 
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = _descriptor_pool_handle,
        .descriptorSetCount = 1,
        .pSetLayouts        = &_descriptor_set_layout_handle,
    };

    for (auto i: std::views::iota(0u, _descriptor_set_handles.size()))
        VK_CHECK(vkAllocateDescriptorSets(_context.device_handle, &descriptor_set_allocate_info, &_descriptor_set_handles[i]));
}

void HelloTriangle::updateDescriptorSets()
{
    VkDescriptorImageInfo storage_image_descriptor
    {
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };

    VkWriteDescriptorSet result_image_write 
    { 
        .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstBinding       = 1,
        .descriptorCount  = 1,
        .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo       = &storage_image_descriptor
    };

    for (auto i: std::views::iota(0u, _descriptor_set_handles.size()))
    {
        storage_image_descriptor.imageView  = _swapchain_image_view_handles[i];
        result_image_write.dstSet           = _descriptor_set_handles[i];

        vkUpdateDescriptorSets(_context.device_handle, 1, &result_image_write, 0, VK_NULL_HANDLE);
    }

    const VkWriteDescriptorSetAccelerationStructureKHR as_write_info 
    { 
        .sType                         = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .accelerationStructureCount    = 1,
        .pAccelerationStructures       = &_tlas->vk_handle
    };

    VkWriteDescriptorSet write_descriptor_set_info 
    { 
        .sType             = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext             = &as_write_info,
        .dstBinding        = 0,
        .descriptorCount   = 1,
        .descriptorType    = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
    };

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
    log::info("Create triangle");
    
    _mesh.vertices.emplace_back(1, 1, 0);
    _mesh.vertices.emplace_back(-1, 1, 0);
    _mesh.vertices.emplace_back(0, -1, 0);

    _mesh.indices.push_back(0);
    _mesh.indices.push_back(1);
    _mesh.indices.push_back(2);

    const auto vertex_buffer_size = static_cast<uint32_t>(sizeof(glm::vec3) * _mesh.vertices.size());
    const auto index_buffer_size  = static_cast<uint32_t>(sizeof(uint32_t) * _mesh.indices.size());

    auto shared_usage_flags = 
            VK_BUFFER_USAGE_TRANSFER_DST_BIT 
        |   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT 
        |   VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

    _mesh.vertex_buffer = Buffer::Builder(getContext())
        .vkSize(vertex_buffer_size)
        .vkUsage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | shared_usage_flags)
        .name("Vertex buffer")
        .build();

    _mesh.index_buffer = Buffer::Builder(getContext())
        .vkSize(index_buffer_size)
        .vkUsage(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | shared_usage_flags)
        .name("Index buffer")
        .build();

    Buffer::writeData(_mesh.vertex_buffer.value(), std::span(_mesh.vertices));
    Buffer::writeData(_mesh.index_buffer.value(), std::span(_mesh.indices));
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
        }, "Run drawing", GpuMarkerColors::run_ray_tracing_pipeline);
    }

    /// build command buffers for switch images layout from swapchain
    VkImageMemoryBarrier image_barrier 
    { 
        .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout              = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout              = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex    = 0,
        .dstQueueFamilyIndex    = 0,
        .subresourceRange       = 
        {
            .aspectMask       = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel     = 0,
            .levelCount       = 1,
            .baseArrayLayer   = 0,
            .layerCount       = 1
        }
    };

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
        }, "Change swapchain image layout to present", GpuMarkerColors::change_image_layout);

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
        }, "Change swapchain image layout to general", GpuMarkerColors::change_image_layout);

        std::swap(image_barrier.oldLayout, image_barrier.newLayout);
    }
}

void HelloTriangle::show()
{
    bool stay = true;

    SDL_Event event = { };
    
    VkResult result = VK_SUCCESS;

    VkPresentInfoKHR present_info 
    { 
        .sType          = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount = 1,
        .pSwapchains    = &_swapchain_handle,
        .pResults       = &result
    };

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

        const auto image_index = getNextImageIndex();
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