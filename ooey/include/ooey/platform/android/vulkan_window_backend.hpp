#pragma once

#include "ooey/platform/android/window_backend.hpp"
#define VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan.h>
#include <vector>

namespace ooey::android {

class VulkanWindowBackend : public WindowBackend {
public:
    VulkanWindowBackend(struct android_app* app);
    ~VulkanWindowBackend() override;

protected:
    bool init_graphics_context() override;
    void cleanup_graphics_context() override;
    void recreate_render_target(int width, int height) override;

private:
    bool create_instance(bool& enable_validation, const std::vector<const char*>& validation_layers);
    bool pick_physical_device();
    bool find_graphics_queue_family();
    bool create_logical_device(bool enable_validation, const std::vector<const char*>& validation_layers);

    VkInstance instance_{VK_NULL_HANDLE};
    VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
    VkDevice device_{VK_NULL_HANDLE};
    VkQueue graphics_queue_{VK_NULL_HANDLE};
    uint32_t queue_family_index_{0};
    VkSurfaceKHR vk_surface_{VK_NULL_HANDLE};
    bool use_software_fallback_{false};
};

} // namespace ooey::android

