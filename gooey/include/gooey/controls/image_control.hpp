#pragma once

#include "gooey/mvvmc/view.hpp"
#include <memory>
#include <string>

namespace ooey {
class Image;
}

namespace gooey::controls {
    using namespace ooey;

class ImageControl : public View {
public:
    // Create an image control and decode the file path automatically using the matching decoder
    ImageControl(Rect bounds, const std::string& image_path);

    // Create an image control from a pre-loaded Image object
    ImageControl(Rect bounds, std::shared_ptr<Image> image);

    Rect bounds() const;
    void set_image(std::shared_ptr<Image> image);
    std::shared_ptr<Image> get_image() const { return image_; }

    // Layout support
    Size measure(Size constraints) override;
    void layout(Rect bounds) override;
    void draw(IRenderTarget& target) const override;

private:
    Rect bounds_;
    std::shared_ptr<Image> image_;
};

} // namespace gooey::controls

namespace gooey {
using gooey::controls::ImageControl;
}
