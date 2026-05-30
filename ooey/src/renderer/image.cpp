#include "ooey/renderer/image.hpp"
#include "ooey/renderer/image_decoder_registry.hpp"

namespace ooey::renderer {

std::shared_ptr<Image> Image::load_from_file(const std::string& path) {
    return ImageDecoderRegistry::decode(path);
}

} // namespace ooey::renderer
