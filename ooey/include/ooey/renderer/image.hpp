#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <memory>

namespace ooey::renderer {

class Image {
public:
    Image(int width, int height, std::vector<uint8_t>&& data)
        : width_(width), height_(height), data_(std::move(data)) {}

    int width() const { return width_; }
    int height() const { return height_; }
    const std::vector<uint8_t>& data() const { return data_; }

    static std::shared_ptr<Image> load_from_file(const std::string& path);

private:
    int width_{0};
    int height_{0};
    std::vector<uint8_t> data_;
};

} // namespace ooey::renderer

namespace ooey {
using renderer::Image;
}
