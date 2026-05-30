#pragma once

#include "ooey/platform/wayland/window_backend.hpp"
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.h>

namespace ooey::wayland {

class VulkanWindowBackend : public WindowBackend {
public:
    VulkanWindowBackend();
    ~VulkanWindowBackend() override;

protected:
    bool init_graphics_context() override;
    void cleanup_graphics_context() override;
    void recreate_render_target(int width, int height) override;

private:
    VkInstance instance_{VK_NULL_HANDLE};
    VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
    VkDevice device_{VK_NULL_HANDLE};
    VkQueue graphics_queue_{VK_NULL_HANDLE};
    uint32_t queue_family_index_{0};
    VkSurfaceKHR vk_surface_{VK_NULL_HANDLE};
};

} // namespace ooey::wayland
