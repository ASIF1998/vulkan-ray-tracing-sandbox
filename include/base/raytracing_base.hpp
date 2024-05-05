
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <string_view>

#include <memory>
#include <array>
#include <vector>
#include <span>

#include <numeric>

#include <filesystem>

#include <optional>

#include <functional>

#include <base/vulkan/utils.hpp>
#include <base/vulkan/command_buffer.hpp>

#include <base/vulkan/context.hpp>

namespace sample_vk
{
    class Window
    {
    public:
        struct Size
        {
            uint32_t width;
            uint32_t height;
        };

    public:
        Window(const std::string_view title, uint32_t width, uint32_t height);

        Window(Window&& window);
        Window(const Window& window) = delete;

        ~Window();

        Window& operator = (Window&& window);
        Window& operator = (const Window& window) = delete;

        explicit operator SDL_Window* ()
        {
            return _ptr_window;
        }

        [[nodiscard]]
        Size getSize() const;

        [[nodiscard]]
        std::vector<const char*> getVulkanExtensions() const;

        [[nodiscard]] std::string getTitle() const;
        void setTitle(const std::string_view title);

    private:
        SDL_Window* _ptr_window = nullptr;

        uint32_t _width     = 0;
        uint32_t _height    = 0;
    };

    class RayTracingBase
    {
    public:
        enum : uint32_t
        {
            NUM_IMAGES_IN_SWAPCHAIN = 2
        };

    public:
        RayTracingBase() = default;

        RayTracingBase(const RayTracingBase& app)   = delete;
        RayTracingBase(RayTracingBase&& app)        = delete;

        virtual ~RayTracingBase();

        RayTracingBase& operator = (const RayTracingBase& app)  = delete;
        RayTracingBase& operator = (RayTracingBase&& app)       = delete;

        virtual void init() = 0;
        virtual void show() = 0;

        [[nodiscard]] uint32_t getNextImageIndex() const;
        
    protected:
        [[nodiscard]]
        CommandBuffer getCommandBuffer() const;

        void init(const std::string_view app_name);

        [[nodiscard]]
        const Context* getContext() const noexcept;

    private:
        void getPhysicalDevice();
        void getQueue();
        void getSwapchainImages();

        void createContext();

        void createInstance();
        void createDevice();
        void createCommandPool();
        void createSurface();
        void createSwapchain();
        void createSwapchainImageViews();

        void destroySwapchainImageViews();
        void destroySwapchain();
        void destroySurface();
        void destroyCommandPool();
        void destroyDevice();
        void destroyInstance();

        void destroyContext();

        [[nodiscard]]
        bool isRayTracingSupported(std::span<const VkExtensionProperties> properties) const;

        [[nodiscard]]
        std::vector<VkSurfaceFormatKHR> getSurfaceFormats() const;

    protected:
        std::optional<Window> _window;

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR _ray_tracing_pipeline_properties = { };

        Context _context;

        VkSurfaceKHR                                        _surface_handle                 = VK_NULL_HANDLE;
        VkSwapchainKHR                                      _swapchain_handle               = VK_NULL_HANDLE;
        VkSurfaceFormatKHR                                  _surface_format                 = { };
        std::array<VkImage, NUM_IMAGES_IN_SWAPCHAIN>        _swapchain_image_handles        = { VK_NULL_HANDLE };
        std::array<VkImageView, NUM_IMAGES_IN_SWAPCHAIN>    _swapchain_image_view_handles   = { VK_NULL_HANDLE };
    };
}