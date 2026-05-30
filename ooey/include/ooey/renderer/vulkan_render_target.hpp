#pragma once

#include "ooey/renderer/i_render_target.hpp"
#include <vulkan/vulkan.h>
#include <functional>
#include <vector>

namespace ooey {

class VulkanRenderTarget : public IRenderTarget {
public:
    // Initialize Vulkan render target with window surface
    VulkanRenderTarget(int width, int height, VkInstance instance, VkPhysicalDevice physical_device, 
                       VkDevice device, VkQueue graphics_queue, uint32_t queue_family_index,
                       VkSurfaceKHR surface, std::function<void()>&& present_callback = nullptr);

    // Headless / buffer-only constructor
    VulkanRenderTarget(int width, int height, VkInstance instance, VkPhysicalDevice physical_device, 
                       VkDevice device, VkQueue graphics_queue, uint32_t queue_family_index,
                       std::function<void()>&& present_callback = nullptr);

    ~VulkanRenderTarget() override;

    void resize(int width, int height) override;

    // IRenderTarget implementation
    void clear(Color color) override;
    void draw_geometry(const Geometry& geometry) override;
    Size measure_text(const std::string& text, const Font& font) override;
    void draw_text(const std::string& text, const Font& font, const Point& position, Color color) override;
    void present() override;

    // Vulkan resource getters
    VkInstance get_instance() const { return instance_; }
    VkDevice get_device() const { return device_; }
    VkRenderPass get_render_pass() const { return render_pass_; }

private:
    void init_swapchain();
    void cleanup_swapchain();
    void create_command_buffers();
    void create_sync_objects();
    void create_render_pass();
    void create_framebuffers();

    int width_{0};
    int height_{0};
    std::function<void()> present_callback_;

    // Core Vulkan handles
    VkInstance instance_{VK_NULL_HANDLE};
    VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
    VkDevice device_{VK_NULL_HANDLE};
    VkQueue graphics_queue_{VK_NULL_HANDLE};
    uint32_t queue_family_index_{0};
    VkSurfaceKHR surface_{VK_NULL_HANDLE};

    // Swapchain handles
    VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
    VkFormat swapchain_image_format_{VK_FORMAT_B8G8R8A8_UNORM};
    VkExtent2D swapchain_extent_{0, 0};
    std::vector<VkImage> swapchain_images_;
    std::vector<VkImageView> swapchain_image_views_;
    std::vector<VkFramebuffer> swapchain_framebuffers_;

    // Presentation pipeline
    VkRenderPass render_pass_{VK_NULL_HANDLE};
    VkCommandPool command_pool_{VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> command_buffers_;

    // Synchronization
    std::vector<VkSemaphore> image_available_semaphores_;
    std::vector<VkSemaphore> render_finished_semaphores_;
    std::vector<VkFence> in_flight_fences_;
    uint32_t current_frame_{0};
    bool headless_{true};
    
    // Clear color cached
    Color clear_color_{0, 0, 0, 255};

    // Shaders & Pipelines
    VkShaderModule vert_shader_module_{VK_NULL_HANDLE};
    VkShaderModule frag_shader_module_{VK_NULL_HANDLE};
    VkPipelineLayout pipeline_layout_{VK_NULL_HANDLE};
    VkPipeline triangle_pipeline_{VK_NULL_HANDLE};
    VkPipeline line_pipeline_{VK_NULL_HANDLE};

    // Vertex & Index buffers
    VkBuffer vertex_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory vertex_buffer_memory_{VK_NULL_HANDLE};
    VkBuffer index_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory index_buffer_memory_{VK_NULL_HANDLE};

    struct DrawCall {
        uint32_t first_index;
        uint32_t index_count;
        int32_t vertex_offset;
        PrimitiveType type;
    };
    std::vector<DrawCall> draw_calls_;
    std::vector<Vertex> frame_vertices_;
    std::vector<uint32_t> frame_indices_;

    void create_pipelines();
    void create_vertex_buffers();
};

} // namespace ooey
