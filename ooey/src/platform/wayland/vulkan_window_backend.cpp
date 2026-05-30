#include "ooey/platform/wayland/vulkan_window_backend.hpp"
#include "ooey/renderer/vulkan_render_target.hpp"
#include "ooey/renderer/window_chrome.hpp"
#include <wayland-client.h>
#include <iostream>
#include <vector>

namespace ooey::wayland {

VulkanWindowBackend::VulkanWindowBackend() = default;

VulkanWindowBackend::~VulkanWindowBackend() {
    VulkanWindowBackend::cleanup_graphics_context();
}

#include <cstring>

bool VulkanWindowBackend::init_graphics_context() {
    // 1. Create Vulkan Instance with surface extensions
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "ooey";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "ooey";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    std::vector<const char*> extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
    };

    std::vector<const char*> validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    bool enable_validation = false;
    const char* validation_env = std::getenv("OOEY_VULKAN_VALIDATION");
    if (validation_env != nullptr && std::string(validation_env) == "1") {
        uint32_t layer_count = 0;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        for (const auto& layer : available_layers) {
            if (std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
                enable_validation = true;
                break;
            }
        }
        if (enable_validation) {
            std::cout << "Vulkan Wayland: Enabling Vulkan validation layers...\n";
        } else {
            std::cerr << "Vulkan Wayland: VK_LAYER_KHRONOS_validation layer requested but not found on system.\n";
        }
    }

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
    if (enable_validation) {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
    }

    VkResult res = vkCreateInstance(&create_info, nullptr, &instance_);
    if (res != VK_SUCCESS) {
        std::cerr << "Vulkan Wayland: Failed to create Vulkan instance (error: " << res << "). Check if Vulkan works via vkcube or vulkaninfo.\n";
        return false;
    }

    // 2. Pick Physical Device
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
    if (device_count == 0) {
        std::cerr << "Vulkan Wayland: Failed to find GPUs with Vulkan support. Ensure a Vulkan driver/ICD is installed.\n";
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
        return false;
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());
    physical_device_ = devices[0];

    // Log physical device properties to help the user diagnose Crostini setup
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(physical_device_, &device_properties);
    std::cout << "Vulkan Wayland: Using Vulkan Physical Device: " << device_properties.deviceName 
              << " (Driver version: " << VK_API_VERSION_MAJOR(device_properties.driverVersion) << "."
              << VK_API_VERSION_MINOR(device_properties.driverVersion) << "."
              << VK_API_VERSION_PATCH(device_properties.driverVersion) << ")\n";

    // 3. Find Graphics Queue Family
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, queue_families.data());

    bool found = false;
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queue_family_index_ = i;
            found = true;
            break;
        }
    }

    if (!found) {
        std::cerr << "Vulkan Wayland: Failed to find a graphics queue family\n";
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
        return false;
    }

    // 4. Create Logical Device
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family_index_;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;

    std::vector<const char*> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    device_create_info.ppEnabledExtensionNames = device_extensions.data();
    if (enable_validation) {
        device_create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        device_create_info.ppEnabledLayerNames = validation_layers.data();
    }

    res = vkCreateDevice(physical_device_, &device_create_info, nullptr, &device_);
    if (res != VK_SUCCESS) {
        std::cerr << "Vulkan Wayland: Failed to create Vulkan logical device (error: " << res << ")\n";
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
        return false;
    }

    // 5. Get Graphics Queue handle
    vkGetDeviceQueue(device_, queue_family_index_, 0, &graphics_queue_);
    return true;
}

void VulkanWindowBackend::cleanup_graphics_context() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
        render_target_.reset(); // Destroy target before device/instance teardown
        
        if (vk_surface_ != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(instance_, vk_surface_, nullptr);
            vk_surface_ = VK_NULL_HANDLE;
        }
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

void VulkanWindowBackend::recreate_render_target(int width, int height) {
    if (vk_surface_ == VK_NULL_HANDLE) {
        VkWaylandSurfaceCreateInfoKHR create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        create_info.display = display_;
        create_info.surface = surface_;

        VkResult res = vkCreateWaylandSurfaceKHR(instance_, &create_info, nullptr, &vk_surface_);
        if (res != VK_SUCCESS) {
            std::cerr << "Vulkan Wayland: Failed to create Wayland window surface (error: " << res << ")\n";
            WindowBackend::recreate_render_target(width, height);
            return;
        }
    }

    if (render_target_) {
        render_target_->resize(width, height);
    } else {
        render_target_ = std::make_unique<VulkanRenderTarget>(
            width, height, instance_, physical_device_, device_, 
            graphics_queue_, queue_family_index_, vk_surface_
        );
    }

    if (window_chrome_) {
        decorated_render_target_ = std::make_unique<ooey::ChromeRenderTarget>(render_target_.get(), window_chrome_, Size{width, height});
    } else {
        decorated_render_target_.reset();
    }
}

} // namespace ooey::wayland
