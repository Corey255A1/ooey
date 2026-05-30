#pragma once

#include "ooey/renderer/i_image_decoder.hpp"

namespace ooey::renderer {

class BmpDecoder : public IImageDecoder {
public:
    BmpDecoder() = default;
    virtual ~BmpDecoder() override = default;

    bool can_decode(const std::string& path, const std::vector<uint8_t>& header) override;
    std::shared_ptr<Image> decode(const std::string& path) override;
};

} // namespace ooey::renderer
