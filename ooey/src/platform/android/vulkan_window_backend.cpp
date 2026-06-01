#include "ooey/platform/android/vulkan_window_backend.hpp"
#include "ooey/renderer/vulkan_render_target.hpp"
#include <android/native_window.h>
#include <android/log.h>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>

#define LOG_TAG "OOEY_VULKAN"
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

namespace ooey::android {

VulkanWindowBackend::VulkanWindowBackend(struct android_app* app) : WindowBackend(app) {}

VulkanWindowBackend::~VulkanWindowBackend() {
    VulkanWindowBackend::cleanup_graphics_context();
}

bool VulkanWindowBackend::init_graphics_context() {
    std::vector<const char*> validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };
    bool enable_validation = false;

    if (!create_instance(enable_validation, validation_layers) ||
        !pick_physical_device() ||
        !find_graphics_queue_family() ||
        !create_logical_device(enable_validation, validation_layers)) {
        LOGE("Vulkan Android: Vulkan initialization failed. Falling back to Software rendering.");
        use_software_fallback_ = true;
        return WindowBackend::init_graphics_context();
    }

    // Get Graphics Queue handle
    vkGetDeviceQueue(device_, queue_family_index_, 0, &graphics_queue_);
    return true;
}

bool VulkanWindowBackend::create_instance(bool& enable_validation, const std::vector<const char*>& validation_layers) {
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "ooey";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "ooey";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    std::vector<const char*> extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
    };

    enable_validation = false;
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
            LOGI("Vulkan Android: Enabling Vulkan validation layers...");
        } else {
            LOGE("Vulkan Android: VK_LAYER_KHRONOS_validation layer requested but not found.");
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
        LOGE("Vulkan Android: Failed to create Vulkan instance (error: %d)", res);
        return false;
    }
    return true;
}

bool VulkanWindowBackend::pick_physical_device() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
    if (device_count == 0) {
        LOGE("Vulkan Android: Failed to find GPUs with Vulkan support.");
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
        return false;
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());
    physical_device_ = devices[0];

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(physical_device_, &device_properties);
    LOGI("Vulkan Android: Using Vulkan Physical Device: %s (Driver version: %d.%d.%d)",
         device_properties.deviceName,
         VK_API_VERSION_MAJOR(device_properties.driverVersion),
         VK_API_VERSION_MINOR(device_properties.driverVersion),
         VK_API_VERSION_PATCH(device_properties.driverVersion));
    return true;
}

bool VulkanWindowBackend::find_graphics_queue_family() {
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
        LOGE("Vulkan Android: Failed to find a graphics queue family");
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
        return false;
    }
    return true;
}

bool VulkanWindowBackend::create_logical_device(bool enable_validation, const std::vector<const char*>& validation_layers) {
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

    VkResult res = vkCreateDevice(physical_device_, &device_create_info, nullptr, &device_);
    if (res != VK_SUCCESS) {
        LOGE("Vulkan Android: Failed to create Vulkan logical device (error: %d)", res);
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
        return false;
    }
    return true;
}

void VulkanWindowBackend::cleanup_graphics_context() {
    if (use_software_fallback_) {
        WindowBackend::cleanup_graphics_context();
        return;
    }
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
    if (use_software_fallback_) {
        WindowBackend::recreate_render_target(width, height);
        return;
    }
    if (vk_surface_ == VK_NULL_HANDLE) {
        if (!native_window_) {
            LOGE("Vulkan Android: Cannot recreate render target, native_window_ is null");
            return;
        }

        VkAndroidSurfaceCreateInfoKHR create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
        create_info.pNext = nullptr;
        create_info.flags = 0;
        create_info.window = native_window_;

        VkResult res = vkCreateAndroidSurfaceKHR(instance_, &create_info, nullptr, &vk_surface_);
        if (res != VK_SUCCESS) {
            LOGE("Vulkan Android: Failed to create Android window surface (error: %d)", res);
            return;
        }
        LOGI("Vulkan Android: Created Android Vulkan surface successfully");
    }

    if (render_target_) {
        render_target_->resize(width, height);
    } else {
        render_target_ = std::make_unique<VulkanRenderTarget>(
            width, height, instance_, physical_device_, device_, 
            graphics_queue_, queue_family_index_, vk_surface_
        );
    }
}

} // namespace ooey::android

