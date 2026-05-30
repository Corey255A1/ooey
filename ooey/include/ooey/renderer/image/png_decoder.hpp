#pragma once

#include "ooey/renderer/i_image_decoder.hpp"

namespace ooey::renderer {

class PngDecoder : public IImageDecoder {
public:
    PngDecoder() = default;
    virtual ~PngDecoder() override = default;

    bool can_decode(const std::string& path, const std::vector<uint8_t>& header) override;
    std::shared_ptr<Image> decode(const std::string& path) override;
};

} // namespace ooey::renderer
