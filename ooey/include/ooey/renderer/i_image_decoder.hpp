#pragma once

#include <string>
#include <memory>
#include <vector>
#include <cstdint>

namespace ooey::renderer {

class Image;

class IImageDecoder {
public:
    virtual ~IImageDecoder() = default;

    // Returns true if this decoder can process the image format, based on file extension/signature sniff
    virtual bool can_decode(const std::string& path, const std::vector<uint8_t>& header) = 0;

    // Decodes the image file to a standard RGBA8888 Image object
    virtual std::shared_ptr<Image> decode(const std::string& path) = 0;
};

} // namespace ooey::renderer
