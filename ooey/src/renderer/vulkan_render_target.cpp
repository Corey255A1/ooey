#ifndef __EMSCRIPTEN__
#include "ooey/renderer/vulkan_render_target.hpp"
#include "ooey/renderer/font_engine.hpp"
#include "ooey/renderer/glyph_atlas.hpp"
#include "ooey/renderer/vulkan_shaders.hpp"
#include "ooey/renderer/image.hpp"
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <limits>
#include <cstring>

namespace ooey {

struct PushConstants {
    float width;
    float height;
};

// Static helper functions for Vulkan resource creation
static uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);
    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("Vulkan: Failed to find suitable memory type!");
}

static void create_buffer(VkDevice device, VkPhysicalDevice physical_device, VkDeviceSize size, VkBufferUsageFlags usage, 
                          VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory) {
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to create buffer!");
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(physical_device, mem_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, buffer_memory, 0);
}

static VkShaderModule create_shader_module(VkDevice device, const std::vector<uint32_t>& code) {
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size() * sizeof(uint32_t);
    create_info.pCode = code.data();

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to create shader module!");
    }
    return shader_module;
}

VulkanRenderTarget::VulkanRenderTarget(int width, int height, VkInstance instance, VkPhysicalDevice physical_device, 
                                       VkDevice device, VkQueue graphics_queue, uint32_t queue_family_index,
                                       VkSurfaceKHR surface, std::function<void()>&& present_callback)
    : width_(width), height_(height), present_callback_(std::move(present_callback)),
      instance_(instance), physical_device_(physical_device), device_(device),
      graphics_queue_(graphics_queue), queue_family_index_(queue_family_index),
      surface_(surface), headless_(false) {
    
    init_swapchain();
    create_render_pass();
    create_framebuffers();
    create_pipelines();
    create_vertex_buffers();
    create_command_buffers();
    create_sync_objects();
}

VulkanRenderTarget::VulkanRenderTarget(int width, int height, VkInstance instance, VkPhysicalDevice physical_device, 
                                       VkDevice device, VkQueue graphics_queue, uint32_t queue_family_index,
                                       std::function<void()>&& present_callback)
    : width_(width), height_(height), present_callback_(std::move(present_callback)),
      instance_(instance), physical_device_(physical_device), device_(device),
      graphics_queue_(graphics_queue), queue_family_index_(queue_family_index) {
    
    create_vertex_buffers();
    create_command_buffers();
    create_sync_objects();
}

VulkanRenderTarget::~VulkanRenderTarget() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
        
        cleanup_swapchain();
        
        if (triangle_pipeline_ != VK_NULL_HANDLE) vkDestroyPipeline(device_, triangle_pipeline_, nullptr);
        if (line_pipeline_ != VK_NULL_HANDLE) vkDestroyPipeline(device_, line_pipeline_, nullptr);
        if (pipeline_layout_ != VK_NULL_HANDLE) vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
        if (vert_shader_module_ != VK_NULL_HANDLE) vkDestroyShaderModule(device_, vert_shader_module_, nullptr);
        if (frag_shader_module_ != VK_NULL_HANDLE) vkDestroyShaderModule(device_, frag_shader_module_, nullptr);
        
        if (vertex_buffer_ != VK_NULL_HANDLE) vkDestroyBuffer(device_, vertex_buffer_, nullptr);
        if (vertex_buffer_memory_ != VK_NULL_HANDLE) vkFreeMemory(device_, vertex_buffer_memory_, nullptr);
        if (index_buffer_ != VK_NULL_HANDLE) vkDestroyBuffer(device_, index_buffer_, nullptr);
        if (index_buffer_memory_ != VK_NULL_HANDLE) vkFreeMemory(device_, index_buffer_memory_, nullptr);
        
        for (auto& pair : geometry_cache_) {
            if (pair.second.vertex_buffer != VK_NULL_HANDLE) vkDestroyBuffer(device_, pair.second.vertex_buffer, nullptr);
            if (pair.second.vertex_memory != VK_NULL_HANDLE) vkFreeMemory(device_, pair.second.vertex_memory, nullptr);
            if (pair.second.index_buffer != VK_NULL_HANDLE) vkDestroyBuffer(device_, pair.second.index_buffer, nullptr);
            if (pair.second.index_memory != VK_NULL_HANDLE) vkFreeMemory(device_, pair.second.index_memory, nullptr);
        }
        geometry_cache_.clear();

        if (render_pass_ != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device_, render_pass_, nullptr);
        }
        
        for (size_t i = 0; i < 2; ++i) {
            if (i < image_available_semaphores_.size()) {
                vkDestroySemaphore(device_, image_available_semaphores_[i], nullptr);
            }
            if (i < render_finished_semaphores_.size()) {
                vkDestroySemaphore(device_, render_finished_semaphores_[i], nullptr);
            }
            if (i < in_flight_fences_.size()) {
                vkDestroyFence(device_, in_flight_fences_[i], nullptr);
            }
        }
        
        if (command_pool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device_, command_pool_, nullptr);
        }
    }
}

void VulkanRenderTarget::resize(int width, int height) {
    width_ = width;
    height_ = height;
    
    if (headless_) {
        return;
    }
    
    vkDeviceWaitIdle(device_);
    cleanup_swapchain();
    init_swapchain();
    create_framebuffers();
}

void VulkanRenderTarget::clear(Color color) {
    clear_color_ = color;
    clip_stack_.clear();
}

void VulkanRenderTarget::draw_geometry(const Geometry& geometry) {
    draw_geometry(geometry, nullptr, true);
}

void VulkanRenderTarget::draw_geometry(const Geometry& geometry, const void* cache_key, bool is_dirty) {
    if (geometry.vertices.empty()) {
        return;
    }

    if (cache_key != nullptr) {
        if (current_frame_keys_.count(cache_key) > 0) {
            // Collision detected! This is a stack-reused address (a temporary object).
            // Bypass the cache and use the transient (per-frame) buffer.
            cache_key = nullptr;
        } else {
            current_frame_keys_.insert(cache_key);
        }
    }

    if (cache_key != nullptr) {
        auto it = geometry_cache_.find(cache_key);
        if (it != geometry_cache_.end()) {
            it->second.epoch = current_epoch_;
            if (is_dirty) {
                if (it->second.vertex_count < geometry.vertices.size() || it->second.index_count < geometry.indices.size()) {
                    if (it->second.vertex_buffer != VK_NULL_HANDLE) vkDestroyBuffer(device_, it->second.vertex_buffer, nullptr);
                    if (it->second.vertex_memory != VK_NULL_HANDLE) vkFreeMemory(device_, it->second.vertex_memory, nullptr);
                    if (it->second.index_buffer != VK_NULL_HANDLE) vkDestroyBuffer(device_, it->second.index_buffer, nullptr);
                    if (it->second.index_memory != VK_NULL_HANDLE) vkFreeMemory(device_, it->second.index_memory, nullptr);

                    VkDeviceSize needed_vertex_size = geometry.vertices.size() * sizeof(Vertex);
                    VkDeviceSize needed_index_size = geometry.indices.size() * sizeof(uint32_t);

                    create_buffer(device_, physical_device_, needed_vertex_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                  it->second.vertex_buffer, it->second.vertex_memory);
                    create_buffer(device_, physical_device_, needed_index_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                  it->second.index_buffer, it->second.index_memory);
                }

                it->second.vertex_count = static_cast<uint32_t>(geometry.vertices.size());
                it->second.index_count = static_cast<uint32_t>(geometry.indices.size());

                void* data;
                vkMapMemory(device_, it->second.vertex_memory, 0, it->second.vertex_count * sizeof(Vertex), 0, &data);
                std::memcpy(data, geometry.vertices.data(), it->second.vertex_count * sizeof(Vertex));
                vkUnmapMemory(device_, it->second.vertex_memory);

                vkMapMemory(device_, it->second.index_memory, 0, it->second.index_count * sizeof(uint32_t), 0, &data);
                std::memcpy(data, geometry.indices.data(), it->second.index_count * sizeof(uint32_t));
                vkUnmapMemory(device_, it->second.index_memory);
            }

            DrawCall call;
            call.first_index = 0;
            call.index_count = it->second.index_count;
            call.vertex_offset = 0;
            call.type = geometry.type;
            call.vertex_buffer = it->second.vertex_buffer;
            call.index_buffer = it->second.index_buffer;
            call.scissor_rect = get_current_clip();
            draw_calls_.push_back(call);
            return;
        } else {
            VulkanGeometryCacheEntry entry;
            entry.epoch = current_epoch_;
            entry.vertex_count = static_cast<uint32_t>(geometry.vertices.size());
            entry.index_count = static_cast<uint32_t>(geometry.indices.size());

            VkDeviceSize needed_vertex_size = entry.vertex_count * sizeof(Vertex);
            VkDeviceSize needed_index_size = entry.index_count * sizeof(uint32_t);

            create_buffer(device_, physical_device_, needed_vertex_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          entry.vertex_buffer, entry.vertex_memory);
            create_buffer(device_, physical_device_, needed_index_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          entry.index_buffer, entry.index_memory);

            void* data;
            vkMapMemory(device_, entry.vertex_memory, 0, needed_vertex_size, 0, &data);
            std::memcpy(data, geometry.vertices.data(), needed_vertex_size);
            vkUnmapMemory(device_, entry.vertex_memory);

            vkMapMemory(device_, entry.index_memory, 0, needed_index_size, 0, &data);
            std::memcpy(data, geometry.indices.data(), needed_index_size);
            vkUnmapMemory(device_, entry.index_memory);

            geometry_cache_[cache_key] = entry;

            DrawCall call;
            call.first_index = 0;
            call.index_count = entry.index_count;
            call.vertex_offset = 0;
            call.type = geometry.type;
            call.vertex_buffer = entry.vertex_buffer;
            call.index_buffer = entry.index_buffer;
            call.scissor_rect = get_current_clip();
            draw_calls_.push_back(call);
            return;
        }
    }

    DrawCall call;
    call.first_index = static_cast<uint32_t>(frame_indices_.size());
    call.index_count = static_cast<uint32_t>(geometry.indices.size());
    call.vertex_offset = static_cast<int32_t>(frame_vertices_.size());
    call.type = geometry.type;
    call.vertex_buffer = VK_NULL_HANDLE;
    call.index_buffer = VK_NULL_HANDLE;
    call.scissor_rect = get_current_clip();

    frame_vertices_.insert(frame_vertices_.end(), geometry.vertices.begin(), geometry.vertices.end());
    frame_indices_.insert(frame_indices_.end(), geometry.indices.begin(), geometry.indices.end());

    draw_calls_.push_back(call);
}

Size VulkanRenderTarget::measure_text(const std::string& text, const Font& font) {
    return FontEngine::measure_text(text, font);
}

void VulkanRenderTarget::draw_text(const std::string& text, const Font& font, const Point& position, Color color) {
    if (text.empty()) {
        return;
    }
    auto atlas = FontEngine::get_glyph_atlas(font);
    if (!atlas || !atlas->get_image()) {
        return;
    }

    size_t start_vertices = frame_vertices_.size();
    size_t start_indices = frame_indices_.size();

    DrawCall call;
    call.first_index = static_cast<uint32_t>(start_indices);
    call.vertex_offset = static_cast<int32_t>(start_vertices);
    call.type = PrimitiveType::Triangles;
    call.scissor_rect = get_current_clip();

    uint32_t quad_count = 0;

    int pen_x = position.x;
    int pen_y = position.y;

    const Image& image = *atlas->get_image();
    const uint8_t* pixels = image.data().data();
    int atlas_w = image.width();

    for (char c : text) {
        GlyphMetrics metrics;
        if (atlas->get_glyph_metrics(c, metrics)) {
            if (metrics.width > 0 && metrics.height > 0) {
                int x0 = pen_x + metrics.offset_x;
                int y0 = pen_y + metrics.offset_y;

                for (int r = 0; r < metrics.height; ++r) {
                    int atlas_y = metrics.y + r;
                    for (int col = 0; col < metrics.width; ++col) {
                        int atlas_x = metrics.x + col;
                        int src_idx = (atlas_y * atlas_w + atlas_x) * 4;
                        uint8_t alpha_coverage = pixels[src_idx + 3];
                        auto alpha = static_cast<uint8_t>((static_cast<uint32_t>(color.a) * alpha_coverage) / 255);

                        if (alpha > 0) {
                            auto fx = static_cast<float>(x0 + col);
                            auto fy = static_cast<float>(y0 + r);
                            float fw = 1.0f;
                            float fh = 1.0f;

                            uint32_t base = quad_count * 4;
                            Color blended_color = color;
                            blended_color.a = alpha;

                            frame_vertices_.push_back(Vertex{.x=fx, .y=fy, .color=blended_color});
                            frame_vertices_.push_back(Vertex{.x=fx + fw, .y=fy, .color=blended_color});
                            frame_vertices_.push_back(Vertex{.x=fx + fw, .y=fy + fh, .color=blended_color});
                            frame_vertices_.push_back(Vertex{.x=fx, .y=fy + fh, .color=blended_color});

                            frame_indices_.push_back(base + 0);
                            frame_indices_.push_back(base + 1);
                            frame_indices_.push_back(base + 2);
                            frame_indices_.push_back(base + 2);
                            frame_indices_.push_back(base + 3);
                            frame_indices_.push_back(base + 0);

                            quad_count++;
                        }
                    }
                }
            }
            pen_x += metrics.advance;
        }
    }

    if (quad_count > 0) {
        call.index_count = quad_count * 6;
        draw_calls_.push_back(call);
    }
}


void VulkanRenderTarget::present_headless() {
    frame_vertices_.clear();
    frame_indices_.clear();
    draw_calls_.clear();
    current_frame_keys_.clear();
    if (present_callback_) {
        present_callback_();
    }
}

bool VulkanRenderTarget::acquire_next_image(uint32_t& image_index) {
    vkWaitForFences(device_, 1, &in_flight_fences_[current_frame_], VK_TRUE, UINT64_MAX);
    
    VkResult result = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, 
                                            image_available_semaphores_[current_frame_], 
                                            VK_NULL_HANDLE, &image_index);
                                             
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        resize(width_, height_);
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Vulkan: Failed to acquire swapchain image!");
    }
    return true;
}

void VulkanRenderTarget::copy_vertex_and_index_data() {
    // Performance & Stability Optimization: Dynamically resize vertex and index buffers.
    // If the current frame requires more vertex/index capacity than allocated, we free the old
    // buffers and allocate larger ones, rounding up to MB boundaries to avoid recurring reallocation thrashing.
    if (!frame_vertices_.empty()) {
        VkDeviceSize needed_vertex_size = frame_vertices_.size() * sizeof(Vertex);
        if (needed_vertex_size > vertex_buffer_size_) {
            if (vertex_buffer_ != VK_NULL_HANDLE) {
                vkDestroyBuffer(device_, vertex_buffer_, nullptr);
            }
            if (vertex_buffer_memory_ != VK_NULL_HANDLE) {
                vkFreeMemory(device_, vertex_buffer_memory_, nullptr);
            }
            // Round up to next MB boundary to reduce future reallocations
            vertex_buffer_size_ = ((needed_vertex_size + (static_cast<VkDeviceSize>(1024 * 1024)) - 1) / (static_cast<VkDeviceSize>(1024 * 1024))) * (static_cast<VkDeviceSize>(1024 * 1024));
            create_buffer(device_, physical_device_, vertex_buffer_size_, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          vertex_buffer_, vertex_buffer_memory_);
        }

        VkDeviceSize needed_index_size = frame_indices_.size() * sizeof(uint32_t);
        if (needed_index_size > index_buffer_size_) {
            if (index_buffer_ != VK_NULL_HANDLE) {
                vkDestroyBuffer(device_, index_buffer_, nullptr);
            }
            if (index_buffer_memory_ != VK_NULL_HANDLE) {
                vkFreeMemory(device_, index_buffer_memory_, nullptr);
            }
            // Round up to next MB boundary
            index_buffer_size_ = ((needed_index_size + (static_cast<VkDeviceSize>(1024 * 1024)) - 1) / (static_cast<VkDeviceSize>(1024 * 1024))) * (static_cast<VkDeviceSize>(1024 * 1024));
            create_buffer(device_, physical_device_, index_buffer_size_, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          index_buffer_, index_buffer_memory_);
        }

        void* data;
        vkMapMemory(device_, vertex_buffer_memory_, 0, needed_vertex_size, 0, &data);
        std::memcpy(data, frame_vertices_.data(), needed_vertex_size);
        vkUnmapMemory(device_, vertex_buffer_memory_);

        vkMapMemory(device_, index_buffer_memory_, 0, needed_index_size, 0, &data);
        std::memcpy(data, frame_indices_.data(), needed_index_size);
        vkUnmapMemory(device_, index_buffer_memory_);
    }
}


void VulkanRenderTarget::record_render_commands(VkCommandBuffer cmd, uint32_t image_index) {
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &begin_info);

    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass_;
    render_pass_info.framebuffer = swapchain_framebuffers_[image_index];
    render_pass_info.renderArea.offset = {.x=0, .y=0};
    render_pass_info.renderArea.extent = swapchain_extent_;

    VkClearValue clear_value{};
    clear_value.color = {{clear_color_.r / 255.0f, clear_color_.g / 255.0f, clear_color_.b / 255.0f, clear_color_.a / 255.0f}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_value;

    vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain_extent_.width);
    viewport.height = static_cast<float>(swapchain_extent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {.x=0, .y=0};
    scissor.extent = swapchain_extent_;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // If we have geometry to draw, record pipeline bindings and draws
    if (!draw_calls_.empty()) {
        PushConstants push_constants{ .width=static_cast<float>(width_), .height=static_cast<float>(height_) };
        vkCmdPushConstants(cmd, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &push_constants);

        VkPipeline last_pipeline = VK_NULL_HANDLE;
        VkBuffer last_vertex_buffer = VK_NULL_HANDLE;
        VkBuffer last_index_buffer = VK_NULL_HANDLE;

        for (const auto& call : draw_calls_) {
            VkRect2D vk_scissor{};
            vk_scissor.offset.x = std::max(0, call.scissor_rect.x);
            vk_scissor.offset.y = std::max(0, call.scissor_rect.y);
            vk_scissor.extent.width = std::max(0, call.scissor_rect.width);
            vk_scissor.extent.height = std::max(0, call.scissor_rect.height);
            vkCmdSetScissor(cmd, 0, 1, &vk_scissor);

            VkPipeline active_pipeline = (call.type == PrimitiveType::Triangles) ? triangle_pipeline_ : line_pipeline_;
            if (active_pipeline != last_pipeline) {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, active_pipeline);
                last_pipeline = active_pipeline;
            }

            VkBuffer active_vertex = (call.vertex_buffer != VK_NULL_HANDLE) ? call.vertex_buffer : vertex_buffer_;
            VkBuffer active_index = (call.index_buffer != VK_NULL_HANDLE) ? call.index_buffer : index_buffer_;

            if (active_vertex != last_vertex_buffer && active_vertex != VK_NULL_HANDLE) {
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(cmd, 0, 1, &active_vertex, offsets);
                last_vertex_buffer = active_vertex;
            }
            if (active_index != last_index_buffer && active_index != VK_NULL_HANDLE) {
                vkCmdBindIndexBuffer(cmd, active_index, 0, VK_INDEX_TYPE_UINT32);
                last_index_buffer = active_index;
            }

            vkCmdDrawIndexed(cmd, call.index_count, 1, call.first_index, call.vertex_offset, 0);
        }
    }
    
    vkCmdEndRenderPass(cmd);
    
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to record command buffer!");
    }
}

void VulkanRenderTarget::submit_and_present(VkCommandBuffer cmd, uint32_t image_index) {
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {image_available_semaphores_[current_frame_]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;

    VkSemaphore signal_semaphores[] = {render_finished_semaphores_[current_frame_]};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    if (vkQueueSubmit(graphics_queue_, 1, &submit_info, in_flight_fences_[current_frame_]) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapchains[] = {swapchain_};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &image_index;

    VkResult result = vkQueuePresentKHR(graphics_queue_, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        resize(width_, height_);
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to present swapchain image!");
    }
}

void VulkanRenderTarget::push_clip(const Rect& rect) {
    Rect current = rect;
    if (!clip_stack_.empty()) {
        const Rect& parent = clip_stack_.back();
        int x1 = std::max(parent.x, rect.x);
        int y1 = std::max(parent.y, rect.y);
        int x2 = std::min(parent.x + parent.width, rect.x + rect.width);
        int y2 = std::min(parent.y + parent.height, rect.y + rect.height);
        int w = std::max(0, x2 - x1);
        int h = std::max(0, y2 - y1);
        current = Rect{x1, y1, w, h};
    }
    clip_stack_.push_back(current);
}

void VulkanRenderTarget::pop_clip() {
    if (!clip_stack_.empty()) {
        clip_stack_.pop_back();
    }
}

Rect VulkanRenderTarget::get_current_clip() const {
    if (clip_stack_.empty()) {
        return Rect{0, 0, width_, height_};
    }
    return clip_stack_.back();
}

void VulkanRenderTarget::present() {
    if (headless_) {
        present_headless();
        return;
    }

    if (swapchain_ == VK_NULL_HANDLE) {
        return;
    }

    uint32_t image_index = 0;
    if (!acquire_next_image(image_index)) {
        return;
    }

    copy_vertex_and_index_data();

    vkResetFences(device_, 1, &in_flight_fences_[current_frame_]);

    VkCommandBuffer cmd = command_buffers_[image_index];
    vkResetCommandBuffer(cmd, 0);

    record_render_commands(cmd, image_index);

    submit_and_present(cmd, image_index);

    current_frame_ = (current_frame_ + 1) % 2;

    // Garbage collect unused cached geometry buffers (epoch-based)
    current_epoch_++;
    std::vector<const void*> keys_to_remove;
    for (const auto& pair : geometry_cache_) {
        if (current_epoch_ - pair.second.epoch > 300) {
            keys_to_remove.push_back(pair.first);
        }
    }
    for (const void* key : keys_to_remove) {
        auto& entry = geometry_cache_[key];
        if (entry.vertex_buffer != VK_NULL_HANDLE) vkDestroyBuffer(device_, entry.vertex_buffer, nullptr);
        if (entry.vertex_memory != VK_NULL_HANDLE) vkFreeMemory(device_, entry.vertex_memory, nullptr);
        if (entry.index_buffer != VK_NULL_HANDLE) vkDestroyBuffer(device_, entry.index_buffer, nullptr);
        if (entry.index_memory != VK_NULL_HANDLE) vkFreeMemory(device_, entry.index_memory, nullptr);
        geometry_cache_.erase(key);
    }

    frame_vertices_.clear();
    frame_indices_.clear();
    draw_calls_.clear();
    current_frame_keys_.clear();

    if (present_callback_) {
        present_callback_();
    }
}

void VulkanRenderTarget::init_swapchain() {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_, &capabilities);

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &format_count, formats.data());

    VkSurfaceFormatKHR surface_format = formats[0];
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surface_format = format;
            break;
        }
    }
    swapchain_image_format_ = surface_format.format;

    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        swapchain_extent_ = capabilities.currentExtent;
    } else {
        VkExtent2D actual_extent = {static_cast<uint32_t>(width_), static_cast<uint32_t>(height_)};
        actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        swapchain_extent_ = actual_extent;
    }

    uint32_t image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
        image_count = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface_;
    create_info.minImageCount = image_count;
    create_info.imageFormat = swapchain_image_format_;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = swapchain_extent_;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.preTransform = capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device_, &create_info, nullptr, &swapchain_) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to create swapchain!");
    }

    vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, nullptr);
    swapchain_images_.resize(image_count);
    vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, swapchain_images_.data());

    swapchain_image_views_.resize(swapchain_images_.size());
    for (size_t i = 0; i < swapchain_images_.size(); ++i) {
        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = swapchain_images_[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = swapchain_image_format_;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device_, &view_info, nullptr, &swapchain_image_views_[i]) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create swapchain image views!");
        }
    }
}

void VulkanRenderTarget::cleanup_swapchain() {
    for (auto framebuffer : swapchain_framebuffers_) {
        vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }
    swapchain_framebuffers_.clear();
    
    for (auto image_view : swapchain_image_views_) {
        vkDestroyImageView(device_, image_view, nullptr);
    }
    swapchain_image_views_.clear();
    
    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
}

void VulkanRenderTarget::create_render_pass() {
    if (headless_) {
        return;
    }
    
    VkAttachmentDescription color_attachment{};
    color_attachment.format = swapchain_image_format_;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(device_, &render_pass_info, nullptr, &render_pass_) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to create render pass!");
    }
}

void VulkanRenderTarget::create_framebuffers() {
    if (headless_) {
        return;
    }
    
    swapchain_framebuffers_.resize(swapchain_image_views_.size());
    
    for (size_t i = 0; i < swapchain_image_views_.size(); ++i) {
        VkImageView attachments[] = {swapchain_image_views_[i]};

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_pass_;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = swapchain_extent_.width;
        framebuffer_info.height = swapchain_extent_.height;
        framebuffer_info.layers = 1;

        if (vkCreateFramebuffer(device_, &framebuffer_info, nullptr, &swapchain_framebuffers_[i]) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create framebuffers!");
        }
    }
}

void VulkanRenderTarget::create_command_buffers() {
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = queue_family_index_;

    if (vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to create command pool!");
    }

    uint32_t count = headless_ ? 1 : static_cast<uint32_t>(swapchain_images_.size());
    command_buffers_.resize(count);

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool_;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = count;

    if (vkAllocateCommandBuffers(device_, &alloc_info, command_buffers_.data()) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to allocate command buffers!");
    }
}

void VulkanRenderTarget::create_sync_objects() {
    image_available_semaphores_.resize(2);
    render_finished_semaphores_.resize(2);
    in_flight_fences_.resize(2);

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < 2; ++i) {
        if (vkCreateSemaphore(device_, &semaphore_info, nullptr, &image_available_semaphores_[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device_, &semaphore_info, nullptr, &render_finished_semaphores_[i]) != VK_SUCCESS ||
            vkCreateFence(device_, &fence_info, nullptr, &in_flight_fences_[i]) != VK_SUCCESS) {
            throw std::runtime_error("Vulkan: Failed to create synchronization structures!");
        }
    }
}

void VulkanRenderTarget::create_vertex_buffers() {
    // Allocate 1 MB for vertex and index buffers
    VkDeviceSize vertex_buffer_size = static_cast<VkDeviceSize>(1024 * 1024);
    VkDeviceSize index_buffer_size = static_cast<VkDeviceSize>(1024 * 1024);

    create_buffer(device_, physical_device_, vertex_buffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  vertex_buffer_, vertex_buffer_memory_);

    create_buffer(device_, physical_device_, index_buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  index_buffer_, index_buffer_memory_);
}

void VulkanRenderTarget::create_pipeline_layout() {
    VkPushConstantRange push_constant_range{};
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;

    if (vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &pipeline_layout_) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to create pipeline layout!");
    }
}

VkPipeline VulkanRenderTarget::create_graphics_pipeline_with_topology(VkPrimitiveTopology topology) {
    VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_module_;
    vert_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module_;
    frag_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

    VkVertexInputBindingDescription binding_description{};
    binding_description.binding = 0;
    binding_description.stride = sizeof(Vertex);
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attribute_descriptions[2]{};
    attribute_descriptions[0].binding = 0;
    attribute_descriptions[0].location = 0;
    attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions[0].offset = offsetof(Vertex, x);

    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format = VK_FORMAT_R8G8B8A8_UNORM;
    attribute_descriptions[1].offset = offsetof(Vertex, color);

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.vertexAttributeDescriptionCount = 2;
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions;

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = topology;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;

    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = pipeline_layout_;
    pipeline_info.renderPass = render_pass_;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to create graphics pipeline!");
    }
    return pipeline;
}

void VulkanRenderTarget::create_pipelines() {
    if (headless_) {
        return;
    }

    vert_shader_module_ = create_shader_module(device_, vulkan_vert_shader);
    frag_shader_module_ = create_shader_module(device_, vulkan_frag_shader);

    create_pipeline_layout();

    triangle_pipeline_ = create_graphics_pipeline_with_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    line_pipeline_ = create_graphics_pipeline_with_topology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
}

void VulkanRenderTarget::draw_image(const Image& image, const Rect& dest_rect) {
    // Performance Optimization: Cache unit-sized geometry of downsampled image pixels.
    // Rather than rebuilding the image quads and allocating dynamic structures every frame,
    // we query our cache. If the image is not cached yet, we generate its unit-sized vertices
    // and indices once.
    auto it = image_geometry_cache_.find(&image);
    if (it == image_geometry_cache_.end()) {
        CachedImageGeometry cached;
        cached.orig_width = image.width();
        cached.orig_height = image.height();

        int ds_w = std::min(image.width(), 32);
        int ds_h = std::min(image.height(), 32);
        float qw = 1.0f / ds_w; // Unit coordinates relative to dest_rect size
        float qh = 1.0f / ds_h;
        const uint8_t* pixels = image.data().data();
        int orig_w = image.width();
        int orig_h = image.height();

        uint32_t quad_count = 0;
        for (int y = 0; y < ds_h; ++y) {
            int src_y = (y * orig_h) / ds_h;
            for (int x = 0; x < ds_w; ++x) {
                int src_x = (x * orig_w) / ds_w;
                int src_idx = (src_y * orig_w + src_x) * 4;
                Color color{pixels[src_idx], pixels[src_idx+1], pixels[src_idx+2], pixels[src_idx+3]};

                if (color.a == 0) continue; // skip fully transparent

                float fx = x * qw;
                float fy = y * qh;

                uint32_t base = quad_count * 4;
                cached.vertices.push_back(Vertex{.x=fx, .y=fy, .color=color});
                cached.vertices.push_back(Vertex{.x=fx + qw, .y=fy, .color=color});
                cached.vertices.push_back(Vertex{.x=fx + qw, .y=fy + qh, .color=color});
                cached.vertices.push_back(Vertex{.x=fx, .y=fy + qh, .color=color});

                cached.indices.push_back(base + 0);
                cached.indices.push_back(base + 1);
                cached.indices.push_back(base + 2);
                cached.indices.push_back(base + 2);
                cached.indices.push_back(base + 3);
                cached.indices.push_back(base + 0);

                quad_count++;
            }
        }
        image_geometry_cache_[&image] = std::move(cached);
        it = image_geometry_cache_.find(&image);
    }

    const auto& cached = it->second;
    if (cached.vertices.empty()) {
        return;
    }

    size_t start_vertices = frame_vertices_.size();
    size_t start_indices = frame_indices_.size();

    // Performance Optimization: Batch all quads representing the image pixels into a single draw call.
    // This reduces Vulkan drawing commands from thousands per image to exactly one draw command.
    DrawCall call;
    call.first_index = static_cast<uint32_t>(start_indices);
    call.vertex_offset = static_cast<int32_t>(start_vertices);
    call.type = PrimitiveType::Triangles;
    call.index_count = static_cast<uint32_t>(cached.indices.size());
    call.scissor_rect = get_current_clip();

    // Copy indices direct with offset adjustments
    frame_indices_.insert(frame_indices_.end(), cached.indices.begin(), cached.indices.end());

    // Scale and shift vertices to dest_rect (extremely fast linear projection, zero heap allocations)
    auto rx = static_cast<float>(dest_rect.x);
    auto ry = static_cast<float>(dest_rect.y);
    auto rw = static_cast<float>(dest_rect.width);
    auto rh = static_cast<float>(dest_rect.height);

    for (const auto& v : cached.vertices) {
        frame_vertices_.push_back(Vertex{
            .x=rx + v.x * rw,
            .y=ry + v.y * rh,
            .color=v.color
        });
    }

    draw_calls_.push_back(call);
}



} // namespace ooey

#endif // __EMSCRIPTEN__
