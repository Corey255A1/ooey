#pragma once

#include <vector>
#include <memory>
#include <string>

namespace ooey::renderer {

class IImageDecoder;
class Image;

class ImageDecoderRegistry {
public:
    static void register_decoder(std::unique_ptr<IImageDecoder>&& decoder);
    static std::shared_ptr<Image> decode(const std::string& path);
    static void register_default_decoders();

private:
    static std::vector<std::unique_ptr<IImageDecoder>>& decoders();
};

} // namespace ooey::renderer
