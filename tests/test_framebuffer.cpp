#include <gtest/gtest.h>
#include "ooey/platform/framebuffer/window_backend.hpp"
#include "ooey/renderer/software_render_target.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <cstdlib>

class TempFile {
public:
    explicit TempFile(size_t size) {
        char path_template[] = "/tmp/ooey_fb_test_XXXXXX";
        fd = mkstemp(path_template);
        if (fd >= 0) {
            path = path_template;
            if (ftruncate(fd, static_cast<off_t>(size)) < 0) {
                close(fd);
                fd = -1;
            }
        }
    }

    ~TempFile() {
        if (fd >= 0) {
            close(fd);
        }
        if (!path.empty()) {
            unlink(path.c_str());
        }
    }

    int fd{-1};
    std::string path;
};

TEST(FramebufferRenderTarget, RotationMapping0) {
    TempFile temp(1536000);
    ASSERT_GE(temp.fd, 0);

    ooey::framebuffer::WindowBackend backend(0, temp.path);
    ASSERT_TRUE(backend.create({480, 800}, "Test"));

    EXPECT_EQ(backend.get_logical_width(), 480);
    EXPECT_EQ(backend.get_logical_height(), 800);

    auto* target = backend.get_render_target();
    ASSERT_NE(target, nullptr);

    ooey::Color clear_color{10, 20, 30, 255};
    target->clear(clear_color);
    target->present();

    uint32_t expected_pixel = (255 << 24) | (10 << 16) | (20 << 8) | 30;

    std::vector<uint32_t> buffer(480 * 800);
    lseek(temp.fd, 0, SEEK_SET);
    ASSERT_EQ(read(temp.fd, buffer.data(), buffer.size() * 4), static_cast<ssize_t>(buffer.size() * 4));

    EXPECT_EQ(buffer[0], expected_pixel);
}

TEST(FramebufferRenderTarget, RotationMapping90) {
    TempFile temp(1536000);
    ASSERT_GE(temp.fd, 0);

    ooey::framebuffer::WindowBackend backend(90, temp.path);
    ASSERT_TRUE(backend.create({800, 480}, "Test"));

    EXPECT_EQ(backend.get_logical_width(), 800);
    EXPECT_EQ(backend.get_logical_height(), 480);

    auto* target = backend.get_render_target();
    ASSERT_NE(target, nullptr);

    target->clear(ooey::Color{0, 0, 0, 0});

    ooey::Color draw_color{255, 128, 64, 255};
    target->draw_geometry({
        {
            {10.0f, 20.0f, draw_color},
            {11.0f, 20.0f, draw_color},
            {11.0f, 21.0f, draw_color},
            {10.0f, 21.0f, draw_color}
        },
        {0, 1, 2, 0, 2, 3},
        ooey::PrimitiveType::Triangles
    });
    target->present();

    std::vector<uint32_t> buffer(480 * 800);
    lseek(temp.fd, 0, SEEK_SET);
    ASSERT_EQ(read(temp.fd, buffer.data(), buffer.size() * 4), static_cast<ssize_t>(buffer.size() * 4));

    uint32_t expected_pixel = (255 << 24) | (255 << 16) | (128 << 8) | 64;
    EXPECT_EQ(buffer[10 * 480 + 459], expected_pixel);
}

TEST(FramebufferRenderTarget, RotationMapping180) {
    TempFile temp(1536000);
    ASSERT_GE(temp.fd, 0);

    ooey::framebuffer::WindowBackend backend(180, temp.path);
    ASSERT_TRUE(backend.create({480, 800}, "Test"));

    EXPECT_EQ(backend.get_logical_width(), 480);
    EXPECT_EQ(backend.get_logical_height(), 800);

    auto* target = backend.get_render_target();
    ASSERT_NE(target, nullptr);

    target->clear(ooey::Color{0, 0, 0, 0});

    ooey::Color draw_color{255, 128, 64, 255};
    target->draw_geometry({
        {
            {10.0f, 20.0f, draw_color},
            {11.0f, 20.0f, draw_color},
            {11.0f, 21.0f, draw_color},
            {10.0f, 21.0f, draw_color}
        },
        {0, 1, 2, 0, 2, 3},
        ooey::PrimitiveType::Triangles
    });
    target->present();

    std::vector<uint32_t> buffer(480 * 800);
    lseek(temp.fd, 0, SEEK_SET);
    ASSERT_EQ(read(temp.fd, buffer.data(), buffer.size() * 4), static_cast<ssize_t>(buffer.size() * 4));

    uint32_t expected_pixel = (255 << 24) | (255 << 16) | (128 << 8) | 64;
    EXPECT_EQ(buffer[779 * 480 + 469], expected_pixel);
}

TEST(FramebufferRenderTarget, RotationMapping270) {
    TempFile temp(1536000);
    ASSERT_GE(temp.fd, 0);

    ooey::framebuffer::WindowBackend backend(270, temp.path);
    ASSERT_TRUE(backend.create({800, 480}, "Test"));

    EXPECT_EQ(backend.get_logical_width(), 800);
    EXPECT_EQ(backend.get_logical_height(), 480);

    auto* target = backend.get_render_target();
    ASSERT_NE(target, nullptr);

    target->clear(ooey::Color{0, 0, 0, 0});

    ooey::Color draw_color{255, 128, 64, 255};
    target->draw_geometry({
        {
            {10.0f, 20.0f, draw_color},
            {11.0f, 20.0f, draw_color},
            {11.0f, 21.0f, draw_color},
            {10.0f, 21.0f, draw_color}
        },
        {0, 1, 2, 0, 2, 3},
        ooey::PrimitiveType::Triangles
    });
    target->present();

    std::vector<uint32_t> buffer(480 * 800);
    lseek(temp.fd, 0, SEEK_SET);
    ASSERT_EQ(read(temp.fd, buffer.data(), buffer.size() * 4), static_cast<ssize_t>(buffer.size() * 4));

    uint32_t expected_pixel = (255 << 24) | (255 << 16) | (128 << 8) | 64;
    EXPECT_EQ(buffer[789 * 480 + 20], expected_pixel);
}
