#include "ooey/renderer/glyph_atlas.hpp"
#include "ooey/renderer/font_backend.hpp"
#include "ooey/renderer/bitmap_font.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace ooey {

std::unordered_map<std::string, std::shared_ptr<GlyphAtlas>> GlyphAtlasManager::cache_;

std::shared_ptr<GlyphAtlas> GlyphAtlasManager::get_atlas(const Font& font, IFontBackend* backend) {
    std::string key = std::string(font.family) + "_" + std::to_string(font.size) + "_" +
                      std::to_string(static_cast<int>(font.weight)) + "_" +
                      std::to_string(static_cast<int>(font.style));
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        return it->second;
    }
    auto atlas = std::make_shared<GlyphAtlas>(font, backend);
    cache_[key] = atlas;
    return atlas;
}

void GlyphAtlasManager::clear() {
    cache_.clear();
}

GlyphAtlas::GlyphAtlas(const Font& font, IFontBackend* backend) {
    generate(font, backend);
}

bool GlyphAtlas::get_glyph_metrics(char c, GlyphMetrics& metrics) const {
    auto it = glyphs_.find(c);
    if (it != glyphs_.end()) {
        metrics = it->second;
        return true;
    }
    return false;
}

void GlyphAtlas::generate(const Font& font, IFontBackend* backend) {
    int atlas_w = 512;
    int atlas_h = 512;
    std::vector<uint8_t> data(atlas_w * atlas_h * 4, 0);
    // Initialize color channels to white, alpha to 0 (transparent)
    for (int i = 0; i < atlas_w * atlas_h; ++i) {
        data[i * 4 + 0] = 255;
        data[i * 4 + 1] = 255;
        data[i * 4 + 2] = 255;
        data[i * 4 + 3] = 0;
    }

    int current_x = 2;
    int current_y = 2;
    int shelf_height = 0;
    int padding = 1;

    // Estimate line height and ascender
    if (backend && backend->load_font(font)) {
        Size sz = backend->measure_text("Ag", font);
        line_height_ = sz.height;
        ascender_ = static_cast<int>(sz.height * 0.8f);
    } else {
        line_height_ = font.size;
        ascender_ = static_cast<int>(font.size * 0.8f);
    }

    struct TempPixel {
        int x, y;
        uint8_t alpha;
    };

    for (char c = 32; c < 127; ++c) {
        std::vector<TempPixel> collected;
        int min_x = 9999, max_x = -9999;
        int min_y = 9999, max_y = -9999;

        auto collect_cb = [&](int x, int y, int w, int h, uint8_t alpha) {
            for (int dy = 0; dy < h; ++dy) {
                for (int dx = 0; dx < w; ++dx) {
                    int px = x + dx;
                    int py = y + dy;
                    collected.push_back({px, py, alpha});
                    if (px < min_x) min_x = px;
                    if (px > max_x) max_x = px;
                    if (py < min_y) min_y = py;
                    if (py > max_y) max_y = py;
                }
            }
        };

        if (backend && backend->load_font(font)) {
            backend->draw_text(std::string(1, c), font, Point{0, 0}, collect_cb);
        } else {
            BitmapFont::draw_text(std::string(1, c), font.size, Point{0, 0}, [&](int x, int y, int w, int h) {
                collect_cb(x, y, w, h, 255);
            });
        }

        int advance = 0;
        if (backend && backend->load_font(font)) {
            advance = backend->measure_text(std::string(1, c), font).width;
        } else {
            advance = font.size * 3 / 5; // fallback estimate
        }

        if (collected.empty()) {
            GlyphMetrics gm{};
            gm.advance = advance;
            glyphs_[c] = gm;
            continue;
        }

        int W = max_x - min_x + 1;
        int H = max_y - min_y + 1;

        std::vector<uint8_t> gray_buf(W * H, 0);
        for (const auto& p : collected) {
            int gx = p.x - min_x;
            int gy = p.y - min_y;
            if (gx >= 0 && gx < W && gy >= 0 && gy < H) {
                gray_buf[gy * W + gx] = p.alpha;
            }
        }

        // Copy exact alpha coverage into padded buffer
        int pw = W + 2 * padding;
        int ph = H + 2 * padding;
        std::vector<uint8_t> padded_buf(pw * ph, 0);

        for (int gy = 0; gy < H; ++gy) {
            for (int gx = 0; gx < W; ++gx) {
                int px = gx + padding;
                int py = gy + padding;
                padded_buf[py * pw + px] = gray_buf[gy * W + gx];
            }
        }

        // Shelf Packing
        if (current_x + pw + 2 > atlas_w) {
            current_x = 2;
            current_y += shelf_height + 2;
            shelf_height = 0;
        }

        if (current_y + ph > atlas_h) {
            std::cerr << "OOEY Font Engine: Warning, glyph atlas is full!\n";
            break;
        }

        // Write to master image buffer
        for (int py = 0; py < ph; ++py) {
            for (int px = 0; px < pw; ++px) {
                int dest_idx = ((current_y + py) * atlas_w + (current_x + px)) * 4;
                uint8_t val = padded_buf[py * pw + px];
                data[dest_idx + 0] = 255;
                data[dest_idx + 1] = 255;
                data[dest_idx + 2] = 255;
                data[dest_idx + 3] = val; // store distance field in alpha
            }
        }

        GlyphMetrics gm;
        gm.x = current_x;
        gm.y = current_y;
        gm.width = pw;
        gm.height = ph;
        gm.offset_x = min_x - padding;
        gm.offset_y = min_y - padding;
        gm.advance = advance;
        glyphs_[c] = gm;

        current_x += pw + 2;
        shelf_height = std::max(shelf_height, ph);
    }

    image_ = std::make_shared<Image>(atlas_w, atlas_h, std::move(data));
}

} // namespace ooey
