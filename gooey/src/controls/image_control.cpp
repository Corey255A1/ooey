#include "gooey/controls/image_control.hpp"
#include "ooey/renderer/image.hpp"
#include "ooey/renderer/i_render_target.hpp"

namespace gooey::controls {
    using namespace ooey;

ImageControl::ImageControl(Rect bounds, const std::string& image_path) : bounds_(bounds) {
    width = {SizePolicy::Fixed, static_cast<float>(bounds.width)};
    height = {SizePolicy::Fixed, static_cast<float>(bounds.height)};
    is_absolute = true;
    absolute_bounds = bounds;

    // Load and decode format automatically via registry
    image_ = Image::load_from_file(image_path);
}

ImageControl::ImageControl(Rect bounds, std::shared_ptr<Image> image) : bounds_(bounds), image_(std::move(image)) {
    width = {SizePolicy::Fixed, static_cast<float>(bounds.width)};
    height = {SizePolicy::Fixed, static_cast<float>(bounds.height)};
    is_absolute = true;
    absolute_bounds = bounds;
}

Rect ImageControl::bounds() const {
    return bounds_;
}

void ImageControl::set_image(std::shared_ptr<Image> image) {
    image_ = std::move(image);
}

Size ImageControl::measure(Size constraints) {
    int w = resolve_width(constraints.width, absolute_bounds.width);
    int h = resolve_height(constraints.height, absolute_bounds.height);
    return Size{w, h};
}

void ImageControl::layout(Rect bounds) {
    bounds_ = bounds;
    View::layout(bounds);
}

void ImageControl::draw(IRenderTarget& target) const {
    if (image_) {
        target.draw_image(*image_, bounds_);
    }
    View::draw(target);
}

} // namespace gooey::controls
