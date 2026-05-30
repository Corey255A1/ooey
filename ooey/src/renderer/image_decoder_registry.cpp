#include "ooey/renderer/image_decoder_registry.hpp"
#include "ooey/renderer/i_image_decoder.hpp"
#include "ooey/renderer/image.hpp"
#include <fstream>
#include <iostream>

// Include built-in format decoders
#include "ooey/renderer/image/bmp_decoder.hpp"

#ifdef OOEY_HAS_PNG
#include "ooey/renderer/image/png_decoder.hpp"
#endif

namespace ooey::renderer {

void ImageDecoderRegistry::register_decoder(std::unique_ptr<IImageDecoder>&& decoder) {
    decoders().push_back(std::move(decoder));
}

std::vector<std::unique_ptr<IImageDecoder>>& ImageDecoderRegistry::decoders() {
    static std::vector<std::unique_ptr<IImageDecoder>> decs;
    return decs;
}

void ImageDecoderRegistry::register_default_decoders() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    // Register built-in decoders
    register_decoder(std::make_unique<BmpDecoder>());

#ifdef OOEY_HAS_PNG
    register_decoder(std::make_unique<PngDecoder>());
#endif
}

std::shared_ptr<Image> ImageDecoderRegistry::decode(const std::string& path) {
    register_default_decoders();

    // Sniff first 16 bytes for checking format signature
    std::ifstream file(path, std::ios::binary);
    std::vector<uint8_t> header(16, 0);
    if (file) {
        file.read(reinterpret_cast<char*>(header.data()), 16);
        header.resize(file.gcount());
    }

    for (auto& decoder : decoders()) {
        if (decoder->can_decode(path, header)) {
            auto img = decoder->decode(path);
            if (img) {
                return img;
            }
        }
    }
    return nullptr;
}

} // namespace ooey::renderer
