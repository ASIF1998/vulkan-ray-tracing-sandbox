#include <base/raytracing_base.hpp>
#include <base/vulkan/memory.hpp>
#include <base/shader_compiler.hpp>

#include <base/vulkan/image.hpp>

#include <stdexcept>

#include <algorithm>

#include <ranges>

namespace sample_vk
{
    Window::Window(const std::string_view title, uint32_t width, uint32_t height)
    {
        log::info("Create window.");

         _ptr_window = SDL_CreateWindow(
            title.data(), 
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
            static_cast<int>(width), static_cast<int>(height), 
            SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
        );

        if (!_ptr_window)
            log::error("Failed create window.");
    }

    Window::Window(Window&& window)
    {
        std::swap(window._ptr_window, _ptr_window);
    }

    Window::~Window()
    {
        if (_ptr_window)
            SDL_DestroyWindow(_ptr_window);
    }

    Window& Window::operator = (Window&& window)
    {
        std::swap(window._ptr_window, _ptr_window);
        return *this;
    }

    Window::Size Window::getSize() const
    {
        int width   = 0;
        int height  = 0;

        SDL_GetWindowSize(_ptr_window, &width, &height);

        return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    }

    std::vector<const char*> Window::getVulkanExtensions() const
    {
        uint32_t extension_count = 0u;

        SDL_CHECK(
            SDL_Vulkan_GetInstanceExtensions(
                _ptr_window,
                &extension_count, nullptr
            )
        );

        std::vector<const char*> extensions(extension_count);

        SDL_CHECK(
            SDL_Vulkan_GetInstanceExtensions(
                _ptr_window,
                &extension_count, extensions.data()
            )   
        );

        return extensions;
    }

    std::string Window::getTitle() const
    {
        return SDL_GetWindowTitle(_ptr_window);
    }

    void Window::setTitle(const std::string_view title)
    {
        SDL_SetWindowTitle(_ptr_window, title.data());
    }
}

namespace sample_vk
{
    RayTracingBase::~RayTracingBase()
    {
        shader::Compiler::finalize();
        destroySwapchainImageViews();
        destroySwapchain();
        destroySurface();
        destroyCommandPool();
        destroyContext();
    }

    void RayTracingBase::destroyContext()
    {
        destroyDevice();
        destroyInstance();
    }
}

namespace sample_vk
{
    enum : uint32_t
    {
        DEFAULT_WINDOW_WIDTH    = 2500,
        DEFAULT_WDINDOW_HEIGHT  = 1200
    };

    void RayTracingBase::createContext()
    {
        createInstance();
        getPhysicalDevice();
        createDevice();
    }

    void RayTracingBase::init(const std::string_view app_name)
    {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        {
            std::string error_msg = SDL_GetError();
            SDL_ClearError();
            log::error("SDL init error: {}.", error_msg);
        }

        _window = Window(app_name, DEFAULT_WINDOW_WIDTH, DEFAULT_WDINDOW_HEIGHT);
        
        createContext();

        VkUtils::init(getContext());
        MemoryProperties::init(_context.physical_device_handle);
        Image::init();
        shader::Compiler::init();
        
        getQueue();
        createCommandPool();
        createSurface();
        createSwapchain();
        getSwapchainImages();
        createSwapchainImageViews();
    }
}

namespace sample_vk
{
    void RayTracingBase::createInstance()
    {
        std::vector<const char*> layers;

        if constexpr (enable_vk_profiling)
        {
            layers.resize(2);
            layers[0] = "VK_LAYER_LUNARG_api_dump";
            layers[1] = "VK_LAYER_KHRONOS_validation";
        }

        auto extensions = _window->getVulkanExtensions();

        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        for (auto extension: extensions)
            log::info("Instance extension: {}", extension);

#if 0
        VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = { };
        debug_messenger_create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_messenger_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_messenger_create_info.pfnUserCallback = VkUtils::debugCallback;
#endif

        VkApplicationInfo app_info = { };
        app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.apiVersion         = VK_API_VERSION_1_3;
        app_info.pApplicationName   = "Ray Tracing Sample";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName        = "No Engine";
        app_info.engineVersion      = VK_MAKE_VERSION(0, 0, 0);

        VkInstanceCreateInfo instance_create_info = { };
        instance_create_info.sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        //instance_create_info.pNext                      = &debug_messenger_create_info;
        instance_create_info.enabledExtensionCount      = static_cast<uint32_t>(extensions.size());
        instance_create_info.ppEnabledExtensionNames    = extensions.data();
        instance_create_info.pApplicationInfo           = &app_info;VK_MAKE_VERSION(0, 0, 1);
        instance_create_info.ppEnabledLayerNames        = layers.data();
        instance_create_info.enabledLayerCount          = static_cast<uint32_t>(layers.size());

        log::info("Create instance");

        VK_CHECK(vkCreateInstance(&instance_create_info, nullptr, &_context.instance_handle));
    }

    void RayTracingBase::destroyInstance()
    {
        if (_context.instance_handle != VK_NULL_HANDLE)
            vkDestroyInstance(_context.instance_handle, nullptr);        
    }
}

namespace sample_vk
{
    void RayTracingBase::getPhysicalDevice()
    {
        uint32_t physical_device_count = 0u;

        std::vector<VkPhysicalDevice> handles;

        VK_CHECK(
            vkEnumeratePhysicalDevices(
                _context.instance_handle, 
                &physical_device_count, nullptr
            )
        );

        handles.resize(physical_device_count);

        VK_CHECK(
            vkEnumeratePhysicalDevices(
                _context.instance_handle, 
                &physical_device_count, handles.data()
            ) 
        );

        VkPhysicalDeviceProperties physical_device_props = { };

        vkGetPhysicalDeviceProperties(handles[0], &physical_device_props);

        for (auto handle: handles)
        {
            uint32_t num_properties = 0u;

            vkEnumerateDeviceExtensionProperties(
                handle, 
                nullptr, 
                &num_properties, nullptr
            );

            std::vector<VkExtensionProperties> extension_properties (num_properties);

            vkEnumerateDeviceExtensionProperties(
                handle, 
                nullptr, 
                &num_properties, extension_properties.data()
            );

            if (isRayTracingSupported(extension_properties))
            {
                _context.physical_device_handle = handle;
                break;
            }
        }

        if (_context.physical_device_handle == VK_NULL_HANDLE)
            log::error("Ray tracing is not supported.");

        _ray_tracing_pipeline_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

        VkPhysicalDeviceProperties2 phyical_device_properties = { };
        phyical_device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        phyical_device_properties.pNext = &_ray_tracing_pipeline_properties;

        vkGetPhysicalDeviceProperties2(_context.physical_device_handle, &phyical_device_properties);

        log::info("Use GPU: {}", phyical_device_properties.properties.deviceName);
    }

    bool RayTracingBase::isRayTracingSupported(std::span<const VkExtensionProperties> properties) const
    {
        constexpr std::array<const char*, 3> device_extensions
        {
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
        };

        for (const auto& property: properties)
        {
            for (auto extension_name: device_extensions)
            {
                if (!strcmp(property.extensionName, extension_name))
                    return true;
            }
        }

        return false;
    }
}

namespace sample_vk
{
    uint32_t getQueueFamilyIndex(
        const std::vector<VkQueueFamilyProperties>& families, 
        VkQueueFlags                                type
    )    
    {
        for (auto family_index: std::views::iota(0u, families.size()))
        {
            if (families[family_index].queueFlags & type)
                return family_index;
        }

        log::error("Failed find queue family index");
        return std::numeric_limits<uint32_t>::max();
    }

    void RayTracingBase::createDevice()
    {
        uint32_t num_families = 0u;

        vkGetPhysicalDeviceQueueFamilyProperties(
            _context.physical_device_handle, 
            &num_families, nullptr
        );

        std::vector<VkQueueFamilyProperties> families_properties (num_families);
        
        vkGetPhysicalDeviceQueueFamilyProperties(
            _context.physical_device_handle, 
            &num_families, families_properties.data()
        );

        constexpr float priority = 1.0f;

        _context.queue.family_index = getQueueFamilyIndex(
            families_properties, 
            VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT
        );

        constexpr uint32_t queue_count = 1;

        VkDeviceQueueCreateInfo device_queue_create_info = { };
        device_queue_create_info.sType               = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        device_queue_create_info.pQueuePriorities    = &priority;
        device_queue_create_info.queueCount          = queue_count;
        device_queue_create_info.queueFamilyIndex    = _context.queue.family_index;

        std::vector<const char*> extensions
        {
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
        };

        for (auto extension: extensions)
            log::info("Device extension: {}", extension);

        VkPhysicalDeviceVulkan12Features device_vulkan12_features = { };
        device_vulkan12_features.sType                  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        device_vulkan12_features.runtimeDescriptorArray = VK_TRUE;
        device_vulkan12_features.bufferDeviceAddress    = VK_TRUE;

        VkPhysicalDeviceSynchronization2Features synchronization_2_features = { };
        synchronization_2_features.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
        synchronization_2_features.pNext            = &device_vulkan12_features;
        synchronization_2_features.synchronization2 = VK_TRUE;

        VkPhysicalDeviceAccelerationStructureFeaturesKHR as_features = { };
        as_features.sType                                                   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        as_features.pNext                                                   = &synchronization_2_features;
        as_features.accelerationStructure                                   = VK_TRUE;
        as_features.accelerationStructureCaptureReplay                      = VK_TRUE;
        as_features.descriptorBindingAccelerationStructureUpdateAfterBind   = VK_TRUE;

        VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_pipeline_features = { };
        ray_tracing_pipeline_features.sType                                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        ray_tracing_pipeline_features.pNext                                 = &as_features;
        ray_tracing_pipeline_features.rayTracingPipeline                    = VK_TRUE;
        ray_tracing_pipeline_features.rayTracingPipelineTraceRaysIndirect   = VK_TRUE;
        ray_tracing_pipeline_features.rayTraversalPrimitiveCulling          = VK_TRUE;

        VkPhysicalDeviceFeatures2 features2 = { };
        features2.sType                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features2.pNext                 = &ray_tracing_pipeline_features;
        features2.features.shaderInt64  = VK_TRUE;

        vkGetPhysicalDeviceFeatures2(_context.physical_device_handle, &features2);

        VkDeviceCreateInfo device_create_info = { };
        device_create_info.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.pNext                    = &features2;
        device_create_info.queueCreateInfoCount     = 1;
        device_create_info.pQueueCreateInfos        = &device_queue_create_info;
        device_create_info.enabledExtensionCount    = static_cast<uint32_t>(extensions.size());
        device_create_info.ppEnabledExtensionNames  = extensions.data();

        log::info("Create device");

        VK_CHECK(
            vkCreateDevice(
                _context.physical_device_handle, 
                &device_create_info, 
                nullptr, 
                &_context.device_handle
            )
        );
    }

    void RayTracingBase::destroyDevice()
    {
        if (_context.device_handle != VK_NULL_HANDLE)
            vkDestroyDevice(_context.device_handle, nullptr);
    }
}

namespace sample_vk
{
    void RayTracingBase::getQueue()
    {
        vkGetDeviceQueue(_context.device_handle, _context.queue.family_index, 0, &_context.queue.handle);
    }
}

namespace sample_vk
{
    void RayTracingBase::createCommandPool()
    {
        VkCommandPoolCreateInfo command_pool_create_info = { };
        command_pool_create_info.sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_create_info.queueFamilyIndex   = _context.queue.family_index;

        VK_CHECK(
            vkCreateCommandPool(
                _context.device_handle, 
                &command_pool_create_info, 
                nullptr, 
                &_context.command_pool_handle
            )
        );
    }

    void RayTracingBase::destroyCommandPool()
    {
        if (_surface_handle != VK_NULL_HANDLE)
            vkDestroyCommandPool(_context.device_handle, _context.command_pool_handle, nullptr);
    }

    CommandBuffer RayTracingBase::getCommandBuffer() const
    {
        return VkUtils::getCommandBuffer(getContext());
    }
}

namespace sample_vk
{
    void RayTracingBase::createSurface()
    {
        log::info("Create surface");

        SDL_CHECK( 
            SDL_Vulkan_CreateSurface(
                static_cast<SDL_Window*>(*_window),
                _context.instance_handle,
                &_surface_handle
            )
        );

        VkBool32 is_supported = VK_FALSE;

        VK_CHECK(
            vkGetPhysicalDeviceSurfaceSupportKHR(
                _context.physical_device_handle, 
                _context.queue.family_index, 
                _surface_handle, 
                &is_supported
            )
        );

        if (is_supported == VK_FALSE)
            log::error("Surface is not support.");
    }

    void RayTracingBase::destroySurface()
    {
        if (_surface_handle != VK_NULL_HANDLE)
            vkDestroySurfaceKHR(_context.instance_handle, _surface_handle, nullptr);
    }
}

namespace sample_vk
{
    std::vector<VkSurfaceFormatKHR> RayTracingBase::getSurfaceFormats() const
    {
        uint32_t num_formats = 0;

        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(_context.physical_device_handle, _surface_handle, &num_formats, nullptr));

        std::vector<VkSurfaceFormatKHR> formats(num_formats);

        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(_context.physical_device_handle, _surface_handle, &num_formats, formats.data()));

        return formats;
    }


    bool formatIsRgb(VkFormat format)
    {
        switch(format)
        {
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_USCALED:
        case VK_FORMAT_R8G8B8_SSCALED:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_R16G16B16_UNORM:
        case VK_FORMAT_R16G16B16_SNORM:
        case VK_FORMAT_R16G16B16_USCALED:
        case VK_FORMAT_R16G16B16_SSCALED:
        case VK_FORMAT_R16G16B16_UINT:
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16_SFLOAT:
        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
        case VK_FORMAT_R64G64B64_UINT:
        case VK_FORMAT_R64G64B64_SINT:
        case VK_FORMAT_R64G64B64_SFLOAT:
            return true;

        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_USCALED:
        case VK_FORMAT_R8G8B8A8_SSCALED:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_USCALED:
        case VK_FORMAT_R16G16B16A16_SSCALED:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        case VK_FORMAT_R64G64B64A64_UINT:
        case VK_FORMAT_R64G64B64A64_SINT:
        case VK_FORMAT_R64G64B64A64_SFLOAT:
            return true;    

        default:
            return false;
        }
    }

    bool formatIsBgr(VkFormat format)
    {
        switch(format)
        {
            case VK_FORMAT_B8G8R8_UNORM:
            case VK_FORMAT_B8G8R8_SNORM:
            case VK_FORMAT_B8G8R8_USCALED:
            case VK_FORMAT_B8G8R8_SSCALED:
            case VK_FORMAT_B8G8R8_UINT:
            case VK_FORMAT_B8G8R8_SINT:
            case VK_FORMAT_B8G8R8_SRGB:
                return true;

            case VK_FORMAT_B8G8R8A8_UNORM:
            case VK_FORMAT_B8G8R8A8_SNORM:
            case VK_FORMAT_B8G8R8A8_USCALED:
            case VK_FORMAT_B8G8R8A8_SSCALED:
            case VK_FORMAT_B8G8R8A8_UINT:
            case VK_FORMAT_B8G8R8A8_SINT:
            case VK_FORMAT_B8G8R8A8_SRGB:
                return true;
            
            default:
                return false;
        }
    }

    VkSurfaceFormatKHR getFormat(const std::vector<VkSurfaceFormatKHR>& formats)
    {
        auto is_valid_format = [] (const auto& format)
        {
            return formatIsRgb(format.format) || formatIsBgr(format.format);
        };
        
        for(const auto& format: formats | std::views::filter(is_valid_format))
            return format;

        log::error("Failed find surface format.");
        return { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_MAX_ENUM_KHR };
    }

    void RayTracingBase::createSwapchain()
    {
        VkSurfaceCapabilitiesKHR capabilities = { };

        VK_CHECK(
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                _context.physical_device_handle, 
                _surface_handle, 
                &capabilities
            )
        );

        _surface_format = getFormat(getSurfaceFormats());

        auto [width, height] = _window->getSize();

        VkSwapchainCreateInfoKHR swapchian_create_info = { };
        swapchian_create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchian_create_info.imageArrayLayers      = 1;
        swapchian_create_info.imageColorSpace       = _surface_format.colorSpace;
        swapchian_create_info.imageFormat           = _surface_format.format;
        swapchian_create_info.imageExtent           = { width, height };
        swapchian_create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        swapchian_create_info.imageUsage            = capabilities.supportedUsageFlags;
        swapchian_create_info.minImageCount         = NUM_IMAGES_IN_SWAPCHAIN;
        swapchian_create_info.queueFamilyIndexCount = 1;
        swapchian_create_info.pQueueFamilyIndices   = &_context.queue.family_index;
        swapchian_create_info.presentMode           = VK_PRESENT_MODE_IMMEDIATE_KHR;
        swapchian_create_info.preTransform          = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchian_create_info.surface               = _surface_handle;
        swapchian_create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        VK_CHECK(
            vkCreateSwapchainKHR(
                _context.device_handle, 
                &swapchian_create_info, 
                nullptr, 
                &_swapchain_handle
            )
        );
    }

    void RayTracingBase::destroySwapchain()
    {
        if (_swapchain_handle != VK_NULL_HANDLE)
            vkDestroySwapchainKHR(_context.device_handle, _swapchain_handle, nullptr);
    }

    void RayTracingBase::getSwapchainImages()
    {
        uint32_t num_images = NUM_IMAGES_IN_SWAPCHAIN;

        VK_CHECK(
            vkGetSwapchainImagesKHR(
                _context.device_handle, 
                _swapchain_handle, 
                &num_images, _swapchain_image_handles.data()
            )
        );

        for (auto i: std::views::iota(0u, num_images))
        {
            VkUtils::setName(
                _context.device_handle, 
                _swapchain_image_handles[i], 
                VK_OBJECT_TYPE_IMAGE, 
                "Swapchain image " + std::to_string(i)
            );
        }

        auto command_buffer = getCommandBuffer();
        
        command_buffer.write([this] (VkCommandBuffer vk_handle)
        {
            VkImageSubresourceRange image_range = { };
            image_range.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
            image_range.baseArrayLayer  = 0;
            image_range.layerCount      = 1;
            image_range.baseMipLevel    = 0;
            image_range.levelCount      = 1;

            VkImageMemoryBarrier image_memory_barrier = { };
            image_memory_barrier.sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            image_memory_barrier.srcAccessMask          = VK_ACCESS_NONE_KHR;
            image_memory_barrier.dstAccessMask          = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
            image_memory_barrier.oldLayout              = VK_IMAGE_LAYOUT_UNDEFINED;
            image_memory_barrier.newLayout              = VK_IMAGE_LAYOUT_GENERAL;
            image_memory_barrier.srcQueueFamilyIndex    = _context.queue.family_index;
            image_memory_barrier.dstQueueFamilyIndex    = _context.queue.family_index;
            image_memory_barrier.subresourceRange       = image_range;

            for(const auto swapchain_image_handle: _swapchain_image_handles)
            {
                image_memory_barrier.image = swapchain_image_handle;

                vkCmdPipelineBarrier(
                    vk_handle, 
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 
                    VK_DEPENDENCY_BY_REGION_BIT, 
                    0, nullptr, 
                    0, nullptr, 
                    1, &image_memory_barrier
                );
            }
        });

        command_buffer.upload(getContext());
    }

    void RayTracingBase::createSwapchainImageViews()
    {
        VkImageViewCreateInfo image_view_create_info = { };
        image_view_create_info.sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.format       = _surface_format.format;
        image_view_create_info.viewType     = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.components   = 
        { 
            VK_COMPONENT_SWIZZLE_IDENTITY, 
            VK_COMPONENT_SWIZZLE_IDENTITY, 
            VK_COMPONENT_SWIZZLE_IDENTITY, 
            VK_COMPONENT_SWIZZLE_IDENTITY 
        };

        image_view_create_info.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseArrayLayer  = 0;
        image_view_create_info.subresourceRange.baseMipLevel    = 0;
        image_view_create_info.subresourceRange.layerCount      = 1;
        image_view_create_info.subresourceRange.levelCount      = 1;
        
        for (auto i: std::views::iota(0u, NUM_IMAGES_IN_SWAPCHAIN))
        {
            image_view_create_info.image = _swapchain_image_handles[i];

            VK_CHECK(
                vkCreateImageView(
                    _context.device_handle, 
                    &image_view_create_info, 
                    nullptr, 
                    &_swapchain_image_view_handles[i]
                )
            );

            VkUtils::setName(
                _context.device_handle, 
                _swapchain_image_view_handles[i], 
                VK_OBJECT_TYPE_IMAGE_VIEW, 
                "Swapchain image view " + std::to_string(i)
            );
        }
    }

    void RayTracingBase::destroySwapchainImageViews()
    {
        for (auto image_view_handle: _swapchain_image_view_handles)
        {
            if (image_view_handle != VK_NULL_HANDLE)
                vkDestroyImageView(_context.device_handle, image_view_handle, nullptr);
        }
    }
}

namespace sample_vk
{
    const Context* RayTracingBase::getContext() const noexcept
    {
        return &_context;
    }
}

namespace sample_vk
{
    uint32_t RayTracingBase::getNextImageIndex() const
    {
        const auto time = std::numeric_limits<uint64_t>::max();

        uint32_t image_index = 0;

        VkFence fence_handle = VK_NULL_HANDLE;
        VkFenceCreateInfo fence_create_info = { };
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        VK_CHECK(vkCreateFence(_context.device_handle, &fence_create_info, nullptr, &fence_handle));

        auto result = vkAcquireNextImageKHR(
            _context.device_handle, 
            _swapchain_handle, 
            time, 
            VK_NULL_HANDLE, 
            fence_handle,
            &image_index
        );

        vkWaitForFences(_context.device_handle, 1, &fence_handle, VK_TRUE, time);
        vkDestroyFence(_context.device_handle, fence_handle, nullptr);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
            return std::numeric_limits<uint32_t>::max();
        else 
            VK_CHECK(result);

        return image_index;
    }
}