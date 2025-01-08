#include <base/scene/visitors/acceleration_structure_builder.hpp>
#include <base/scene/node.hpp>

#include <base/vulkan/context.hpp>

#include <base/vulkan/buffer.hpp>

#include <ranges>

namespace vrts
{
    ASBuilder::ASBuilder(const Context* ptr_context) :
        _ptr_context (ptr_context)
    {
        if (!ptr_context)
            log::error("[ASBuilder]: ptr_context is null.");

        auto identity_matrix = VkUtils::cast(glm::mat4(1.0f));

        if (auto memory_type_index = MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        {
            constexpr auto usage = 
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT 
                |   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT 
                |   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT 
                |   VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

            _identity_matrix = Buffer::Builder(_ptr_context)
                .vkSize(sizeof(identity_matrix))
                .vkUsage(usage)
                .name("[ASBuilder] Identity matrix for AS build")
                .build();
        }
        else
            log::error("[ASBuilder]: Not memory index for create identity matrix.");

        Buffer::writeData(_identity_matrix.value(), identity_matrix);
    }

    void ASBuilder::process(Node* ptr_node)
    {
        if (ptr_node->children.empty())
            return ;

        for (auto& child: ptr_node->children)
            child->visit(this);

        const auto tlas_name = std::format("[TLAS] '{}'", ptr_node->name);

        const auto func_table = VkUtils::getVulkanFunctionPointerTable();

        std::vector<VkAccelerationStructureInstanceKHR> instances;
        instances.reserve(ptr_node->children.size() - 1);

        VkAccelerationStructureKHR acceleration_structure_handle = VK_NULL_HANDLE;

        auto get_instance = [this, &func_table, ptr_node] (const auto& child)
        {
            VkAccelerationStructureDeviceAddressInfoKHR address_info = { };
            address_info.sType                  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
            address_info.accelerationStructure  = child->acceleation_structure->vk_handle;

            VkAccelerationStructureInstanceKHR instance = { };
            instance.transform                              = VkUtils::cast(child->transform * ptr_node->transform);
            instance.instanceCustomIndex                    = _custom_index++; /// gl_InstanceCustomIndexEXT 
            instance.accelerationStructureReference         = func_table.vkGetAccelerationStructureDeviceAddressKHR(_ptr_context->device_handle, &address_info);
            instance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
            instance.mask                                   = 0xff;
            instance.instanceShaderBindingTableRecordOffset = 0;

            return instance;
        };

        auto is_valid = [] (const auto& child) 
        {
            return child->acceleation_structure != std::nullopt;
        };

        std::ranges::for_each(
                ptr_node->children 
            |   std::views::filter(is_valid) 
            |   std::views::transform(get_instance), 
            [&instances](auto instance)
            {
                instances.push_back(instance);
            }
        );

        if (instances.empty())
            return ;

        const auto primitive_count = static_cast<uint32_t>(instances.size());

        auto instances_buffer = Buffer::Builder(_ptr_context)
            .vkSize(sizeof(VkAccelerationStructureInstanceKHR) * instances.size())
            .vkUsage(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
            .name(std::format("[TLAS] Instance buffer: {}", ptr_node->name))
            .build();

        Buffer::writeData(instances_buffer, std::span(instances));

        VkAccelerationStructureGeometryKHR geometry = { };
        geometry.sType          = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.geometryType   = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.flags          = VK_GEOMETRY_OPAQUE_BIT_KHR;

        {
            auto& geometry_instances = geometry.geometry.instances;
            geometry_instances.sType                = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
            geometry_instances.data.deviceAddress   = instances_buffer.getAddress();
            geometry_instances.arrayOfPointers      = VK_FALSE;
        }

        VkAccelerationStructureBuildGeometryInfoKHR geometry_build_info = { };
        geometry_build_info.sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;   
        geometry_build_info.geometryCount   = 1;
        geometry_build_info.pGeometries     = &geometry;
        geometry_build_info.mode            = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        geometry_build_info.type            = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        geometry_build_info.flags           = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;

        VkAccelerationStructureBuildSizesInfoKHR tlas_size = { };
        tlas_size.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        
        func_table.vkGetAccelerationStructureBuildSizesKHR(
            _ptr_context->device_handle,
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &geometry_build_info,
            &primitive_count,
            &tlas_size
        );

        auto tlas_buffer = Buffer::Builder(_ptr_context)
            .vkSize(tlas_size.accelerationStructureSize)
            .vkUsage(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
            .name(std::format("[TLAS] Buffer: {}", ptr_node->name))
            .build();

        VkAccelerationStructureCreateInfoKHR tlas_info = { };
        tlas_info.sType     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        tlas_info.size      = tlas_size.accelerationStructureSize;
        tlas_info.buffer    = tlas_buffer.vk_handle;
        tlas_info.type      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

        VK_CHECK(
            func_table.vkCreateAccelerationStructureKHR(
                _ptr_context->device_handle,
                &tlas_info,
                nullptr,
                &acceleration_structure_handle
            )
        );

        VkUtils::setName(
            _ptr_context->device_handle, 
            acceleration_structure_handle, 
            VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, 
            tlas_name
        );

        auto scratch_buffer = Buffer::Builder(_ptr_context)
            .vkSize(tlas_size.buildScratchSize)
            .vkUsage(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
            .name(std::format("[TLAS] Scratch buffer: {}", ptr_node->name))
            .build();

        geometry_build_info.scratchData.deviceAddress   = scratch_buffer.getAddress();
        geometry_build_info.dstAccelerationStructure    = acceleration_structure_handle;

        auto command_buffer_for_build = VkUtils::getCommandBuffer(_ptr_context);

        command_buffer_for_build.write([primitive_count, &func_table, &geometry_build_info] (VkCommandBuffer vk_handle)
        {
            VkAccelerationStructureBuildRangeInfoKHR build_range = { };
            build_range.primitiveCount = primitive_count;
            
            auto ptr_build_range = &build_range;

            func_table.vkCmdBuildAccelerationStructuresKHR(
                vk_handle, 
                1, &geometry_build_info,
                &ptr_build_range
            );
        });

        command_buffer_for_build.upload(_ptr_context);

        ptr_node->acceleation_structure = std::make_optional<AccelerationStructure>(_ptr_context, acceleration_structure_handle, std::move(tlas_buffer));
    }

    void ASBuilder::process(MeshNode* ptr_node)
    {
        ptr_node->acceleation_structure = buildBLAS(
            ptr_node->name, 
            *ptr_node->mesh.vertex_buffer,
            static_cast<uint32_t>(ptr_node->mesh.vertex_count),
            *ptr_node->mesh.index_buffer,
            static_cast<uint32_t>(ptr_node->mesh.index_count)
        );
    }
    
    void ASBuilder::process(SkinnedMeshNode* ptr_node)
    {
        ptr_node->acceleation_structure = buildBLAS(
            ptr_node->name, 
            *ptr_node->mesh.processed_vertex_buffer,
            static_cast<uint32_t>(ptr_node->mesh.vertex_count),
            *ptr_node->mesh.index_buffer,
            static_cast<uint32_t>(ptr_node->mesh.index_count)
        );
    }

    AccelerationStructure ASBuilder::buildBLAS(
        std::string_view    name,
        const Buffer&       vertex_buffer,
        uint32_t            vertex_count,
        const Buffer&       index_buffer,
        uint32_t            index_count
    )
    {
        VkAccelerationStructureGeometryKHR mesh_info = { };
        mesh_info.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        mesh_info.flags         = VK_GEOMETRY_OPAQUE_BIT_KHR;
        mesh_info.geometryType  = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        
        {
            auto& triangles = mesh_info.geometry.triangles;

            triangles = { };
            triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;

            triangles.indexData.deviceAddress   = index_buffer.getAddress();
            triangles.indexType                 = VK_INDEX_TYPE_UINT32;

            triangles.maxVertex                 = vertex_count;
            triangles.vertexData.deviceAddress  = vertex_buffer.getAddress();
            triangles.vertexFormat              = VK_FORMAT_R32G32B32_SFLOAT;
            triangles.vertexStride              = sizeof(Mesh::Attributes);

            triangles.transformData.deviceAddress = _identity_matrix->getAddress();
        }

        VkAccelerationStructureBuildGeometryInfoKHR build_geometry_info = { };
        build_geometry_info.sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        build_geometry_info.geometryCount   = 1;
        build_geometry_info.pGeometries     = &mesh_info;
        build_geometry_info.mode            = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        build_geometry_info.type            = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

        const auto primitive_count =  index_count / 3;

        VkAccelerationStructureBuildSizesInfoKHR as_size = { };
        as_size.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

        auto func_table = VkUtils::getVulkanFunctionPointerTable();

        func_table.vkGetAccelerationStructureBuildSizesKHR(
            _ptr_context->device_handle,
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &build_geometry_info,
            &primitive_count,
            &as_size
        );

        VkAccelerationStructureKHR acceleration_structure_handle = VK_NULL_HANDLE;

        auto blas_buffer = Buffer::Builder(_ptr_context)
            .vkSize(as_size.accelerationStructureSize)
            .vkUsage(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
            .name(std::format("[BLAS] Buffer: {}", name))
            .build();

        VkAccelerationStructureCreateInfoKHR as_info = { };
        as_info.sType   = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        as_info.buffer  = blas_buffer.vk_handle;
        as_info.size    = as_size.accelerationStructureSize;
        as_info.type    = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        
        VK_CHECK(
            func_table.vkCreateAccelerationStructureKHR(
                _ptr_context->device_handle,
                &as_info,
                nullptr,
                &acceleration_structure_handle
            )
        );

        VkUtils::setName(
            _ptr_context->device_handle, 
            acceleration_structure_handle, 
            VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, 
            std::format("[BLAS] {}", name)
        );

        auto scratch_buffer = Buffer::Builder(_ptr_context)
            .vkSize(as_size.buildScratchSize)
            .vkUsage(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
            .name(std::format("[BLAS] Scratch buffer: {}", name))
            .build();

        VkAccelerationStructureBuildGeometryInfoKHR build_info = { };
        build_info.sType                        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        build_info.type                         = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        build_info.mode                         = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        build_info.flags                        = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
        build_info.geometryCount                = 1;
        build_info.pGeometries                  = &mesh_info;
        build_info.scratchData.deviceAddress    = scratch_buffer.getAddress();
        build_info.dstAccelerationStructure     = acceleration_structure_handle;

        auto command_buffer_for_build = VkUtils::getCommandBuffer(_ptr_context);

        command_buffer_for_build.write([&build_info, &func_table, primitive_count, this](VkCommandBuffer vk_handle)
        {
            VkAccelerationStructureBuildRangeInfoKHR range_info = { };
            range_info.primitiveCount = primitive_count;

            auto ptr_range_info = &range_info;

            func_table.vkCmdBuildAccelerationStructuresKHR(
                vk_handle,
                1, &build_info,
                &ptr_range_info
            );
        });

        command_buffer_for_build.upload(_ptr_context);

        return AccelerationStructure(_ptr_context, acceleration_structure_handle, std::move(blas_buffer));
    }
}