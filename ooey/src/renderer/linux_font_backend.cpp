#include "linux_font_backend.hpp"
#include <iostream>
#include <unordered_map>
#include <dlfcn.h>
#include <cmath>

namespace ooey {

// --- Fontconfig Typedefs ---
typedef struct _FcConfig FcConfig;
typedef struct _FcPattern FcPattern;
typedef int FcBool;
typedef enum _FcResult {
    FcResultMatch, FcResultNoMatch, FcResultTypeMismatch, FcResultNoId, FcResultOutOfMemory
} FcResult;

// --- FreeType Typedefs ---
typedef void* FT_Library;
typedef void* FT_Face;
typedef int FT_Error;

typedef struct FT_Vector_ {
    long x;
    long y;
} FT_Vector;

typedef struct FT_Glyph_Metrics_ {
    long width;
    long height;
    long horiBearingX;
    long horiBearingY;
    long horiAdvance;
    long vertBearingX;
    long vertBearingY;
    long vertAdvance;
} FT_Glyph_Metrics;

typedef struct FT_Bitmap_ {
    unsigned int rows;
    unsigned int width;
    int pitch;
    unsigned char* buffer;
    unsigned short num_grays;
    unsigned char pixel_mode;
    unsigned char palette_mode;
    void* palette;
} FT_Bitmap;

typedef struct FT_GlyphSlotRec_ {
    void* library;
    void* face;
    void* next;
    unsigned int reserved;
    void* generic[2];
    FT_Glyph_Metrics metrics;
    long linearHoriAdvance;
    long linearVertAdvance;
    FT_Vector advance;
    unsigned int format;
    FT_Bitmap bitmap;
    int bitmap_left;
    int bitmap_top;
} FT_GlyphSlotRec, *FT_GlyphSlot;

typedef struct FT_Size_Metrics_ {
    unsigned short x_ppem;
    unsigned short y_ppem;
    long x_scale;
    long y_scale;
    long ascender;
    long descender;
    long height;
    long max_advance;
} FT_Size_Metrics;

typedef struct FT_SizeRec_ {
    void* face;
    void* generic[2];
    FT_Size_Metrics metrics;
    void* internal;
} FT_SizeRec;

typedef struct FT_FaceRec_ {
    long num_faces;
    long face_index;
    long face_flags;
    long style_flags;
    long num_glyphs;
    char* family_name;
    char* style_name;
    int num_fixed_sizes;
    void* available_sizes;
    int num_charmaps;
    void* charmaps;
    void* generic[2];
    long bbox_xMin, bbox_yMin, bbox_xMax, bbox_yMax;
    unsigned short units_per_EM;
    short ascender;
    short descender;
    short height;
    short max_advance_width;
    short max_advance_height;
    short underline_position;
    short underline_thickness;
    FT_GlyphSlot glyph;
    FT_SizeRec* size;
} FT_FaceRec;

struct LinuxFontBackend::Impl {
    void* fontconfig_lib{nullptr};
    void* freetype_lib{nullptr};
    bool loaded{false};

    // Fontconfig dynamic symbols
    FcConfig* (*FcInitLoadConfigAndFonts)(){nullptr};
    FcPattern* (*FcPatternCreate)(){nullptr};
    FcBool (*FcPatternAddString)(FcPattern* p, const char* object, const char* s){nullptr};
    FcBool (*FcPatternAddInteger)(FcPattern* p, const char* object, int i){nullptr};
    FcBool (*FcConfigSubstitute)(FcConfig* config, FcPattern* p, int kind){nullptr};
    void (*FcDefaultSubstitute)(FcPattern* p){nullptr};
    FcPattern* (*FcFontMatch)(FcConfig* config, FcPattern* p, FcResult* result){nullptr};
    FcResult (*FcPatternGetString)(const FcPattern* p, const char* object, int id, char** s){nullptr};
    void (*FcPatternDestroy)(FcPattern* p){nullptr};

    // FreeType dynamic symbols
    FT_Error (*FT_Init_FreeType)(FT_Library* alibrary){nullptr};
    FT_Error (*FT_New_Face)(FT_Library library, const char* filepathname, long face_index, FT_Face* aface){nullptr};
    FT_Error (*FT_Set_Pixel_Sizes)(FT_Face face, unsigned int pixel_width, unsigned int pixel_height){nullptr};
    FT_Error (*FT_Load_Char)(FT_Face face, unsigned long char_code, int load_flags){nullptr};
    FT_Error (*FT_Done_Face)(FT_Face face){nullptr};
    FT_Error (*FT_Done_FreeType)(FT_Library library){nullptr};

    FT_Library ft_library{nullptr};

    struct LoadedFace {
        FT_Face face{nullptr};
        std::string path;
    };
    std::unordered_map<std::string, LoadedFace> loaded_faces_;

    ~Impl() {
        cleanup();
    }

    void cleanup() {
        for (auto& pair : loaded_faces_) {
            if (pair.second.face && FT_Done_Face) {
                FT_Done_Face(pair.second.face);
            }
        }
        loaded_faces_.clear();

        if (ft_library && FT_Done_FreeType) {
            FT_Done_FreeType(ft_library);
            ft_library = nullptr;
        }

        if (freetype_lib) {
            dlclose(freetype_lib);
            freetype_lib = nullptr;
        }
        if (fontconfig_lib) {
            dlclose(fontconfig_lib);
            fontconfig_lib = nullptr;
        }
        loaded = false;
    }

    bool load_symbols() {
        fontconfig_lib = dlopen("libfontconfig.so.1", RTLD_LAZY);
        if (!fontconfig_lib) {
            fontconfig_lib = dlopen("libfontconfig.so", RTLD_LAZY);
        }
        if (!fontconfig_lib) return false;

        freetype_lib = dlopen("libfreetype.so.6", RTLD_LAZY);
        if (!freetype_lib) {
            freetype_lib = dlopen("libfreetype.so", RTLD_LAZY);
        }
        if (!freetype_lib) {
            dlclose(fontconfig_lib);
            fontconfig_lib = nullptr;
            return false;
        }

        // Fontconfig Resolve
        *(void**)(&FcInitLoadConfigAndFonts) = dlsym(fontconfig_lib, "FcInitLoadConfigAndFonts");
        *(void**)(&FcPatternCreate) = dlsym(fontconfig_lib, "FcPatternCreate");
        *(void**)(&FcPatternAddString) = dlsym(fontconfig_lib, "FcPatternAddString");
        *(void**)(&FcPatternAddInteger) = dlsym(fontconfig_lib, "FcPatternAddInteger");
        *(void**)(&FcConfigSubstitute) = dlsym(fontconfig_lib, "FcConfigSubstitute");
        *(void**)(&FcDefaultSubstitute) = dlsym(fontconfig_lib, "FcDefaultSubstitute");
        *(void**)(&FcFontMatch) = dlsym(fontconfig_lib, "FcFontMatch");
        *(void**)(&FcPatternGetString) = dlsym(fontconfig_lib, "FcPatternGetString");
        *(void**)(&FcPatternDestroy) = dlsym(fontconfig_lib, "FcPatternDestroy");

        // FreeType Resolve
        *(void**)(&FT_Init_FreeType) = dlsym(freetype_lib, "FT_Init_FreeType");
        *(void**)(&FT_New_Face) = dlsym(freetype_lib, "FT_New_Face");
        *(void**)(&FT_Set_Pixel_Sizes) = dlsym(freetype_lib, "FT_Set_Pixel_Sizes");
        *(void**)(&FT_Load_Char) = dlsym(freetype_lib, "FT_Load_Char");
        *(void**)(&FT_Done_Face) = dlsym(freetype_lib, "FT_Done_Face");
        *(void**)(&FT_Done_FreeType) = dlsym(freetype_lib, "FT_Done_FreeType");

        if (!FcInitLoadConfigAndFonts || !FcPatternCreate || !FcPatternAddString || 
            !FcPatternAddInteger || !FcConfigSubstitute || !FcDefaultSubstitute || 
            !FcFontMatch || !FcPatternGetString || !FcPatternDestroy ||
            !FT_Init_FreeType || !FT_New_Face || !FT_Set_Pixel_Sizes || 
            !FT_Load_Char || !FT_Done_Face || !FT_Done_FreeType) {
            cleanup();
            return false;
        }

        if (FT_Init_FreeType(&ft_library) != 0) {
            cleanup();
            return false;
        }

        loaded = true;
        return true;
    }

    std::string match_font(const std::string& family, FontWeight weight, FontStyle style) {
        if (!loaded) return "";

        FcConfig* config = FcInitLoadConfigAndFonts();
        FcPattern* pat = FcPatternCreate();
        if (!pat) return "";

        FcPatternAddString(pat, "family", family.c_str());

        int fc_weight = 100; // FcWeightNormal
        if (weight == FontWeight::Bold) {
            fc_weight = 200; // FcWeightBold
        }
        FcPatternAddInteger(pat, "weight", fc_weight);

        int fc_slant = 0; // FcSlantRoman
        if (style == FontStyle::Italic) {
            fc_slant = 100; // FcSlantItalic
        }
        FcPatternAddInteger(pat, "slant", fc_slant);

        FcConfigSubstitute(config, pat, 0); // FcMatchPattern
        FcDefaultSubstitute(pat);

        FcResult result;
        FcPattern* match = FcFontMatch(config, pat, &result);
        std::string font_path = "";
        if (match) {
            char* file_path = nullptr;
            if (FcPatternGetString(match, "file", 0, &file_path) == FcResultMatch && file_path) {
                font_path = file_path;
            }
            FcPatternDestroy(match);
        }
        FcPatternDestroy(pat);
        return font_path;
    }

    FT_Face get_face(const Font& font) {
        if (!loaded) return nullptr;

        std::string key = std::string(font.family ? font.family : "") + "_" + 
                          std::to_string(static_cast<int>(font.weight)) + "_" + 
                          std::to_string(static_cast<int>(font.style));
        
        auto it = loaded_faces_.find(key);
        if (it != loaded_faces_.end()) {
            return it->second.face;
        }

        std::string path = match_font(font.family ? font.family : "sans-serif", font.weight, font.style);
        if (path.empty()) {
            return nullptr;
        }

        FT_Face face = nullptr;
        if (FT_New_Face(ft_library, path.c_str(), 0, &face) == 0) {
            loaded_faces_[key] = {face, path};
            return face;
        }
        return nullptr;
    }
};

LinuxFontBackend::LinuxFontBackend() : impl_(std::make_unique<Impl>()) {}
LinuxFontBackend::~LinuxFontBackend() = default;

bool LinuxFontBackend::initialize() {
    return impl_->load_symbols();
}

bool LinuxFontBackend::load_font(const Font& font) {
    return impl_->get_face(font) != nullptr;
}

Size LinuxFontBackend::measure_text(const std::string& text, const Font& font) {
    FT_Face face = impl_->get_face(font);
    if (!face) {
        return Size{0, 0};
    }

    impl_->FT_Set_Pixel_Sizes(face, 0, font.size);

    FT_FaceRec* face_rec = reinterpret_cast<FT_FaceRec*>(face);
    int total_width = 0;
    
    // Set standard line height derived from font size
    int line_height = static_cast<int>(face_rec->size->metrics.height >> 6);
    if (line_height <= 0) {
        line_height = font.size;
    }

    for (char c : text) {
        if (impl_->FT_Load_Char(face, static_cast<unsigned char>(c), 0x0) == 0) { // FT_LOAD_DEFAULT
            FT_GlyphSlot slot = face_rec->glyph;
            total_width += static_cast<int>(slot->advance.x >> 6);
        }
    }

    return Size{total_width, line_height};
}

void LinuxFontBackend::draw_text(const std::string& text, const Font& font, const Point& position, const DrawCallback& callback) {
    FT_Face face = impl_->get_face(font);
    if (!face) {
        return;
    }

    impl_->FT_Set_Pixel_Sizes(face, 0, font.size);

    FT_FaceRec* face_rec = reinterpret_cast<FT_FaceRec*>(face);

    int pen_x = position.x;
    int ascender = static_cast<int>(face_rec->size->metrics.ascender >> 6);
    int pen_y = position.y + ascender;

    for (char c : text) {
        if (impl_->FT_Load_Char(face, static_cast<unsigned char>(c), 0x4) == 0) { // FT_LOAD_RENDER
            FT_GlyphSlot slot = face_rec->glyph;
            int glyph_x = pen_x + slot->bitmap_left;
            int glyph_y = pen_y - slot->bitmap_top;

            int w = slot->bitmap.width;
            int h = slot->bitmap.rows;
            const unsigned char* buffer = slot->bitmap.buffer;

            if (buffer && w > 0 && h > 0) {
                // FT_PIXEL_MODE_GRAY = 2
                if (slot->bitmap.pixel_mode == 2) {
                    for (int r = 0; r < h; ++r) {
                        for (int col = 0; col < w; ++col) {
                            uint8_t alpha = buffer[r * slot->bitmap.pitch + col];
                            if (alpha > 0) {
                                callback(glyph_x + col, glyph_y + r, 1, 1, alpha);
                            }
                        }
                    }
                }
            }

            pen_x += static_cast<int>(slot->advance.x >> 6);
        }
    }
}

std::vector<std::string> LinuxFontBackend::get_available_fonts() {
    // Return standard system font families. Fontconfig automatically maps each to system font files.
    return {
        "Liberation Sans", 
        "Liberation Serif", 
        "Liberation Mono", 
        "DejaVu Sans", 
        "DejaVu Serif", 
        "DejaVu Sans Mono", 
        "Ubuntu", 
        "sans-serif", 
        "serif", 
        "monospace"
    };
}

} // namespace ooey
