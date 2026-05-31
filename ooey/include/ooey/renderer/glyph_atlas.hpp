#pragma once

#include "ooey/types.hpp"
#include "ooey/renderer/image.hpp"
#include <unordered_map>
#include <memory>
#include <string>

namespace ooey {

class IFontBackend;

struct GlyphMetrics {
    int x{0};             // X coordinate in the atlas image
    int y{0};             // Y coordinate in the atlas image
    int width{0};         // Width in the atlas image (including padding)
    int height{0};        // Height in the atlas image (including padding)
    int offset_x{0};      // X offset relative to the pen position
    int offset_y{0};      // Y offset relative to the pen position
    int advance{0};       // Advance width
};

class GlyphAtlas {
public:
    GlyphAtlas(const Font& font, IFontBackend* backend);

    std::shared_ptr<Image> get_image() const { return image_; }
    bool get_glyph_metrics(char c, GlyphMetrics& metrics) const;
    int get_line_height() const { return line_height_; }
    int get_ascender() const { return ascender_; }

private:
    void generate(const Font& font, IFontBackend* backend);

    std::shared_ptr<Image> image_;
    std::unordered_map<char, GlyphMetrics> glyphs_;
    int line_height_{0};
    int ascender_{0};
};

class GlyphAtlasManager {
public:
    static std::shared_ptr<GlyphAtlas> get_atlas(const Font& font, IFontBackend* backend);
    static void clear();

private:
    static std::unordered_map<std::string, std::shared_ptr<GlyphAtlas>> cache_;
};

} // namespace ooey
