#include "ooey/renderer/software_render_target.hpp"
#include "ooey/renderer/font_engine.hpp"
#include "ooey/renderer/glyph_atlas.hpp"
#include "ooey/renderer/image.hpp"
#include <cmath>
#include <algorithm>
#include <cstddef>

namespace ooey {

/**
 * @brief Fast, bit-exact division by 255 for 16-bit values.
 * 
 * Traditional integer division (val / 255) compiles to a division instruction or 
 * a combination of multiplication and shifts, which can be relatively slow.
 * This bitwise approximation: (val + 1 + (val >> 8)) >> 8 is mathematically 
 * identical to floor(val / 255.0) for all input values in the range [0, 65535].
 * 
 * Since our color values are 8-bit channels multiplied by alpha, the maximum
 * value is 255 * 255 = 65025, which fits comfortably within this range.
 * This optimization avoids expensive divisions during alpha blending operations.
 */
static inline uint8_t div255(uint32_t val) {
    return (val + 1 + (val >> 8)) >> 8;
}



SoftwareRenderTarget::SoftwareRenderTarget(uint8_t* data, int width, int height, int stride, std::function<void()>&& present_callback)
    : data_(data), width_(width), height_(height), stride_(stride), present_callback_(std::move(present_callback)) {}

void SoftwareRenderTarget::initialize_buffer(uint8_t* data, int width, int height, int stride, std::function<void()>&& present_callback) {
    data_ = data;
    width_ = width;
    height_ = height;
    stride_ = stride;
    present_callback_ = std::move(present_callback);
}

void SoftwareRenderTarget::clear(Color color) {
    clip_stack_.clear();
    if (!data_) {
        return;
    }
    uint8_t r = color.r;
    uint8_t g = color.g;
    uint8_t b = color.b;
    uint8_t a = color.a;
    uint32_t pixel = (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | (static_cast<uint32_t>(b));
    for (int y = 0; y < height_; ++y) {
        auto* row = reinterpret_cast<uint32_t*>(data_ + static_cast<ptrdiff_t>(y * stride_));
        for (int x = 0; x < width_; ++x) {
            row[x] = pixel;
        }
    }
}

void SoftwareRenderTarget::draw_geometry(const Geometry& geometry) {
    if (!data_ || geometry.vertices.empty()) {
        return;
    }

    if (geometry.type == PrimitiveType::Triangles) {
        if (!geometry.indices.empty()) {
            for (size_t i = 0; i + 2 < geometry.indices.size(); i += 3) {
                unsigned int i0 = geometry.indices[i];
                unsigned int i1 = geometry.indices[i + 1];
                unsigned int i2 = geometry.indices[i + 2];
                if (i0 < geometry.vertices.size() && i1 < geometry.vertices.size() && i2 < geometry.vertices.size()) {
                    draw_triangle(geometry.vertices[i0], geometry.vertices[i1], geometry.vertices[i2], geometry.vertices[i0].color);
                }
            }
        } else {
            for (size_t i = 0; i + 2 < geometry.vertices.size(); i += 3) {
                draw_triangle(geometry.vertices[i], geometry.vertices[i + 1], geometry.vertices[i + 2], geometry.vertices[i].color);
            }
        }
    } else if (geometry.type == PrimitiveType::Lines) {
        if (!geometry.indices.empty()) {
            for (size_t i = 0; i + 1 < geometry.indices.size(); i += 2) {
                unsigned int ia = geometry.indices[i];
                unsigned int ib = geometry.indices[i + 1];
                if (ia < geometry.vertices.size() && ib < geometry.vertices.size()) {
                    const auto& a = geometry.vertices[ia];
                    const auto& b = geometry.vertices[ib];
                    draw_line(static_cast<int>(a.x), static_cast<int>(a.y), static_cast<int>(b.x), static_cast<int>(b.y), a.color);
                }
            }
        } else {
            for (size_t i = 0; i + 1 < geometry.vertices.size(); i += 2) {
                const auto& a = geometry.vertices[i];
                const auto& b = geometry.vertices[i + 1];
                draw_line(static_cast<int>(a.x), static_cast<int>(a.y), static_cast<int>(b.x), static_cast<int>(b.y), a.color);
            }
        }
    }
}

void SoftwareRenderTarget::draw_geometry(const Geometry& geometry, const void* /*cache_key*/, bool /*is_dirty*/) {
    draw_geometry(geometry);
}

Size SoftwareRenderTarget::measure_text(const std::string& text, const Font& font) {
    return FontEngine::measure_text(text, font);
}

void SoftwareRenderTarget::draw_text(const std::string& text, const Font& font, const Point& position, Color color) {
    if (text.empty() || !data_) {
        return;
    }
    auto atlas = FontEngine::get_glyph_atlas(font);
    if (!atlas || !atlas->get_image()) {
        return;
    }
    const Image& image = *atlas->get_image();
    const uint8_t* pixels = image.data().data();
    int atlas_w = image.width();

    int pen_x = position.x;
    int pen_y = position.y;

    for (char c : text) {
        GlyphMetrics metrics;
        if (atlas->get_glyph_metrics(c, metrics)) {
            if (metrics.width > 0 && metrics.height > 0) {
                int x0 = pen_x + metrics.offset_x;
                int y0 = pen_y + metrics.offset_y;

                Rect clip = get_current_clip();
                int start_y = std::max(clip.y, y0);
                int end_y = std::min(clip.y + clip.height, y0 + metrics.height);
                int start_x = std::max(clip.x, x0);
                int end_x = std::min(clip.x + clip.width, x0 + metrics.width);

                for (int yy = start_y; yy < end_y; ++yy) {
                    auto* row = reinterpret_cast<uint32_t*>(data_ + static_cast<ptrdiff_t>(yy * stride_));
                    int atlas_y = metrics.y + (yy - y0);

                    for (int xx = start_x; xx < end_x; ++xx) {
                        int atlas_x = metrics.x + (xx - x0);
                        int src_idx = (atlas_y * atlas_w + atlas_x) * 4;
                        uint8_t alpha_coverage = pixels[src_idx + 3];
                        auto alpha = static_cast<uint8_t>((static_cast<uint32_t>(color.a) * alpha_coverage) / 255);

                        if (alpha > 0) {
                            if (alpha == 255) {
                                uint32_t src_pixel = (static_cast<uint32_t>(255) << 24) |
                                                     (static_cast<uint32_t>(color.r) << 16) |
                                                     (static_cast<uint32_t>(color.g) << 8) |
                                                     (static_cast<uint32_t>(color.b));
                                row[xx] = src_pixel;
                            } else {
                                uint32_t dest_pixel = row[xx];
                                uint8_t dest_b = dest_pixel & 0xFF;
                                uint8_t dest_g = (dest_pixel >> 8) & 0xFF;
                                uint8_t dest_r = (dest_pixel >> 16) & 0xFF;
                                uint8_t dest_a = (dest_pixel >> 24) & 0xFF;

                                uint32_t src_a = alpha;
                                uint32_t inv_a = 255 - src_a;

                                uint8_t r = div255(color.r * src_a + dest_r * inv_a);
                                uint8_t g = div255(color.g * src_a + dest_g * inv_a);
                                uint8_t b = div255(color.b * src_a + dest_b * inv_a);
                                uint8_t a = div255(src_a * 255 + dest_a * inv_a);

                                row[xx] = (static_cast<uint32_t>(a) << 24) |
                                          (static_cast<uint32_t>(r) << 16) |
                                          (static_cast<uint32_t>(g) << 8) |
                                          (static_cast<uint32_t>(b));
                            }
                        }
                    }
                }
            }
            pen_x += metrics.advance;
        }
    }
}

void SoftwareRenderTarget::present() {
    if (present_callback_) {
        present_callback_();
    }
}

void SoftwareRenderTarget::push_clip(const Rect& rect) {
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

void SoftwareRenderTarget::pop_clip() {
    if (!clip_stack_.empty()) {
        clip_stack_.pop_back();
    }
}

Rect SoftwareRenderTarget::get_current_clip() const {
    if (clip_stack_.empty()) {
        return Rect{0, 0, width_, height_};
    }
    return clip_stack_.back();
}

void SoftwareRenderTarget::draw_filled_rect(int x, int y, int w, int h, Color color) {
    if (!data_) {
        return;
    }
    // Performance optimization: Pre-clamp rect bounds to the render target resolution.
    // This allows us to avoid expensive per-pixel clipping checks inside the nested rendering loop.
    Rect clip = get_current_clip();
    int start_y = std::max(clip.y, y);
    int end_y = std::min(clip.y + clip.height, y + h);
    int start_x = std::max(clip.x, x);
    int end_x = std::min(clip.x + clip.width, x + w);
    if (start_x >= end_x || start_y >= end_y) {
        return;
    }
    uint32_t pixel = (static_cast<uint32_t>(color.a) << 24) | (static_cast<uint32_t>(color.r) << 16) | (static_cast<uint32_t>(color.g) << 8) | (static_cast<uint32_t>(color.b));
    
    // Performance optimization: Cache the pointer to the start of the row outside the inner loop.
    // This reduces pointer arithmetic to simple offsets/increments in the tight inner loop.
    for (int yy = start_y; yy < end_y; ++yy) {
        auto* row = reinterpret_cast<uint32_t*>(data_ + static_cast<ptrdiff_t>(yy * stride_));
        for (int xx = start_x; xx < end_x; ++xx) {
            row[xx] = pixel;
        }
    }
}

void SoftwareRenderTarget::draw_filled_rect_blended(int x, int y, int w, int h, Color color) {
    if (!data_ || color.a == 0) {
        return;
    }
    Rect clip = get_current_clip();
    int start_y = std::max(clip.y, y);
    int end_y = std::min(clip.y + clip.height, y + h);
    int start_x = std::max(clip.x, x);
    int end_x = std::min(clip.x + clip.width, x + w);
    if (start_x >= end_x || start_y >= end_y) {
        return;
    }

    if (color.a == 255) {
        uint32_t pixel = (static_cast<uint32_t>(255) << 24) | (static_cast<uint32_t>(color.r) << 16) | (static_cast<uint32_t>(color.g) << 8) | (static_cast<uint32_t>(color.b));
        for (int yy = start_y; yy < end_y; ++yy) {
            auto* row = reinterpret_cast<uint32_t*>(data_ + static_cast<ptrdiff_t>(yy * stride_));
            for (int xx = start_x; xx < end_x; ++xx) {
                row[xx] = pixel;
            }
        }
    } else {
        uint32_t src_a = color.a;
        uint32_t inv_a = 255 - src_a;
        for (int yy = start_y; yy < end_y; ++yy) {
            auto* row = reinterpret_cast<uint32_t*>(data_ + static_cast<ptrdiff_t>(yy * stride_));
            for (int xx = start_x; xx < end_x; ++xx) {
                uint32_t dest_pixel = row[xx];
                uint8_t dest_b = dest_pixel & 0xFF;
                uint8_t dest_g = (dest_pixel >> 8) & 0xFF;
                uint8_t dest_r = (dest_pixel >> 16) & 0xFF;
                uint8_t dest_a = (dest_pixel >> 24) & 0xFF;

                uint8_t r = div255(color.r * src_a + dest_r * inv_a);
                uint8_t g = div255(color.g * src_a + dest_g * inv_a);
                uint8_t b = div255(color.b * src_a + dest_b * inv_a);
                uint8_t a = div255(src_a * 255 + dest_a * inv_a);

                row[xx] = (static_cast<uint32_t>(a) << 24) |
                          (static_cast<uint32_t>(r) << 16) |
                          (static_cast<uint32_t>(g) << 8) |
                          (static_cast<uint32_t>(b));
            }
        }
    }
}

void SoftwareRenderTarget::draw_line(int start_x, int start_y, int end_x, int end_y, Color color) {
    if (!data_) {
        return;
    }
    int delta_x = std::abs(end_x - start_x);
    int step_x = start_x < end_x ? 1 : -1;
    int delta_y = -std::abs(end_y - start_y);
    int step_y = start_y < end_y ? 1 : -1;
    int error = delta_x + delta_y;
    uint32_t pixel = (static_cast<uint32_t>(color.a) << 24) | (static_cast<uint32_t>(color.r) << 16) | (static_cast<uint32_t>(color.g) << 8) | (static_cast<uint32_t>(color.b));

    Rect clip = get_current_clip();
    while (true) {
        if (start_x >= clip.x && start_x < clip.x + clip.width && start_y >= clip.y && start_y < clip.y + clip.height) {
            auto* row = reinterpret_cast<uint32_t*>(data_ + static_cast<ptrdiff_t>(start_y * stride_));
            row[start_x] = pixel;
        }
        if (start_x == end_x && start_y == end_y) {
            break;
        }
        int error2 = 2 * error;
        if (error2 >= delta_y) {
            error += delta_y;
            start_x += step_x;
        }
        if (error2 <= delta_x) {
            error += delta_x;
            start_y += step_y;
        }
    }
}

void SoftwareRenderTarget::draw_pixel(int x, int y, Color color) {
    Rect clip = get_current_clip();
    if (x >= clip.x && x < clip.x + clip.width && y >= clip.y && y < clip.y + clip.height) {
        auto* row = reinterpret_cast<uint32_t*>(data_ + static_cast<ptrdiff_t>(y * stride_));
        uint32_t pixel = (static_cast<uint32_t>(color.a) << 24) | (static_cast<uint32_t>(color.r) << 16) | (static_cast<uint32_t>(color.g) << 8) | (static_cast<uint32_t>(color.b));
        row[x] = pixel;
    }
}

void SoftwareRenderTarget::draw_flat_bottom_triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Color color) {
    if (std::abs(v1.y - v0.y) < 1e-5f) {
        return;
    }
    // Performance optimization: Calculate reciprocal slopes once to allow incremental
    // updates of x-coordinates (curx1, curx2) per scanline using addition instead of multiplication.
    float invslope1 = (v1.x - v0.x) / (v1.y - v0.y);
    float invslope2 = (v2.x - v0.x) / (v2.y - v0.y);

    float curx1 = v0.x;
    float curx2 = v0.x;

    int y_start = static_cast<int>(std::round(v0.y));
    int y_end = static_cast<int>(std::round(v1.y));

    uint32_t pixel = (static_cast<uint32_t>(color.a) << 24) | (static_cast<uint32_t>(color.r) << 16) | (static_cast<uint32_t>(color.g) << 8) | (static_cast<uint32_t>(color.b));

    Rect clip = get_current_clip();
    for (int scanline_y = y_start; scanline_y < y_end; scanline_y++) {
        // Vertical clipping check at scanline level (outer loop)
        if (scanline_y >= clip.y && scanline_y < clip.y + clip.height) {
            int x1 = static_cast<int>(std::round(curx1));
            int x2 = static_cast<int>(std::round(curx2));
            if (x1 > x2) {
                std::swap(x1, x2);
            }
            // Performance optimization: Clamp the horizontal scanline bounds to target width.
            // Avoids per-pixel clipping checks inside the inner loop.
            int start_x = std::max(clip.x, x1);
            int end_x = std::min(clip.x + clip.width - 1, x2);
            if (start_x <= end_x) {
                // Performance optimization: Cache row pointer per scanline (outer loop).
                // Minimizes address calculation overhead for individual pixel writes.
                auto* row = reinterpret_cast<uint32_t*>(data_ + static_cast<ptrdiff_t>(scanline_y * stride_));
                for (int x = start_x; x <= end_x; ++x) {
                    row[x] = pixel;
                }
            }
        }
        curx1 += invslope1;
        curx2 += invslope2;
    }
}

void SoftwareRenderTarget::draw_flat_top_triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Color color) {
    if (std::abs(v2.y - v0.y) < 1e-5f) {
        return;
    }
    // Performance optimization: Calculate reciprocal slopes once to allow incremental
    // updates of x-coordinates (curx1, curx2) per scanline using subtraction instead of multiplication.
    float invslope1 = (v2.x - v0.x) / (v2.y - v0.y);
    float invslope2 = (v2.x - v1.x) / (v2.y - v1.y);

    float curx1 = v2.x;
    float curx2 = v2.x;

    int y_start = static_cast<int>(std::round(v2.y));
    int y_end = static_cast<int>(std::round(v0.y));

    uint32_t pixel = (static_cast<uint32_t>(color.a) << 24) | (static_cast<uint32_t>(color.r) << 16) | (static_cast<uint32_t>(color.g) << 8) | (static_cast<uint32_t>(color.b));

    Rect clip = get_current_clip();
    for (int scanline_y = y_start; scanline_y > y_end; scanline_y--) {
        // Vertical clipping check at scanline level (outer loop)
        if (scanline_y >= clip.y && scanline_y < clip.y + clip.height) {
            int x1 = static_cast<int>(std::round(curx1));
            int x2 = static_cast<int>(std::round(curx2));
            if (x1 > x2) {
                std::swap(x1, x2);
            }
            // Performance optimization: Clamp the horizontal scanline bounds to target width.
            // Avoids per-pixel clipping checks inside the inner loop.
            int start_x = std::max(clip.x, x1);
            int end_x = std::min(clip.x + clip.width - 1, x2);
            if (start_x <= end_x) {
                // Performance optimization: Cache row pointer per scanline (outer loop).
                // Minimizes address calculation overhead for individual pixel writes.
                auto* row = reinterpret_cast<uint32_t*>(data_ + static_cast<ptrdiff_t>(scanline_y * stride_));
                for (int x = start_x; x <= end_x; ++x) {
                    row[x] = pixel;
                }
            }
        }
        curx1 -= invslope1;
        curx2 -= invslope2;
    }
}

void SoftwareRenderTarget::draw_triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, Color color) {
    Vertex sorted_v0 = v0;
    Vertex sorted_v1 = v1;
    Vertex sorted_v2 = v2;

    if (sorted_v0.y > sorted_v1.y) {
        std::swap(sorted_v0, sorted_v1);
    }
    if (sorted_v0.y > sorted_v2.y) {
        std::swap(sorted_v0, sorted_v2);
    }
    if (sorted_v1.y > sorted_v2.y) {
        std::swap(sorted_v1, sorted_v2);
    }

    if (std::abs(sorted_v1.y - sorted_v2.y) < 1e-5f) {
        draw_flat_bottom_triangle(sorted_v0, sorted_v1, sorted_v2, color);
    } else if (std::abs(sorted_v0.y - sorted_v1.y) < 1e-5f) {
        draw_flat_top_triangle(sorted_v0, sorted_v1, sorted_v2, color);
    } else {
        Vertex v3;
        v3.y = sorted_v1.y;
        v3.x = sorted_v0.x + ((sorted_v1.y - sorted_v0.y) / (sorted_v2.y - sorted_v0.y)) * (sorted_v2.x - sorted_v0.x);
        v3.color = color;
        draw_flat_bottom_triangle(sorted_v0, sorted_v1, v3, color);
        draw_flat_top_triangle(sorted_v1, v3, sorted_v2, color);
    }
}

void SoftwareRenderTarget::draw_image(const Image& image, const Rect& dest_rect) {
    if (!data_ || image.width() <= 0 || image.height() <= 0) {
        return;
    }

    const uint8_t* img_pixels = image.data().data();
    int img_w = image.width();
    int img_h = image.height();

    Rect clip = get_current_clip();
    int start_y = std::max(0, clip.y - dest_rect.y);
    int end_y = std::min(dest_rect.height, clip.y + clip.height - dest_rect.y);
    int start_x = std::max(0, clip.x - dest_rect.x);
    int end_x = std::min(dest_rect.width, clip.x + clip.width - dest_rect.x);

    if (start_x >= end_x || start_y >= end_y) {
        return;
    }

    for (int y = start_y; y < end_y; ++y) {
        int yy = dest_rect.y + y;
        int src_y = (y * img_h) / dest_rect.height;
        if (src_y < 0 || src_y >= img_h) continue;

        auto* row = reinterpret_cast<uint32_t*>(data_ + static_cast<ptrdiff_t>(yy * stride_));

        for (int x = start_x; x < end_x; ++x) {
            int xx = dest_rect.x + x;
            int src_x = (x * img_w) / dest_rect.width;
            if (src_x < 0 || src_x >= img_w) continue;

            int src_idx = (src_y * img_w + src_x) * 4;
            uint8_t src_r = img_pixels[src_idx + 0];
            uint8_t src_g = img_pixels[src_idx + 1];
            uint8_t src_b = img_pixels[src_idx + 2];
            uint8_t src_a = img_pixels[src_idx + 3];

            if (src_a == 255) {
                row[xx] = (static_cast<uint32_t>(255) << 24) |
                          (static_cast<uint32_t>(src_r) << 16) |
                          (static_cast<uint32_t>(src_g) << 8) |
                          (static_cast<uint32_t>(src_b));
            } else if (src_a > 0) {
                uint32_t dest_pixel = row[xx];
                uint8_t dest_b = dest_pixel & 0xFF;
                uint8_t dest_g = (dest_pixel >> 8) & 0xFF;
                uint8_t dest_r = (dest_pixel >> 16) & 0xFF;
                uint8_t dest_a = (dest_pixel >> 24) & 0xFF;

                uint32_t inv_a = 255 - src_a;
                uint8_t r = div255(src_r * src_a + dest_r * inv_a);
                uint8_t g = div255(src_g * src_a + dest_g * inv_a);
                uint8_t b = div255(src_b * src_a + dest_b * inv_a);
                uint8_t a = div255(src_a * 255 + dest_a * inv_a);

                row[xx] = (static_cast<uint32_t>(a) << 24) |
                          (static_cast<uint32_t>(r) << 16) |
                          (static_cast<uint32_t>(g) << 8) |
                          (static_cast<uint32_t>(b));
            }
        }
    }
}

} // namespace ooey
