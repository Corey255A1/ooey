#include "ooey/renderer/image/bmp_decoder.hpp"
#include "ooey/renderer/image.hpp"
#include <fstream>
#include <vector>

namespace ooey::renderer {

bool BmpDecoder::can_decode(const std::string& /*path*/, const std::vector<uint8_t>& header) {
    if (header.size() < 2) return false;
    return header[0] == 'B' && header[1] == 'M';
}

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t file_type{0x4D42}; // BM
    uint32_t file_size{0};
    uint16_t reserved1{0};
    uint16_t reserved2{0};
    uint32_t offset_data{0};
};

struct BMPInfoHeader {
    uint32_t size{0};
    int32_t width{0};
    int32_t height{0};
    uint16_t planes{1};
    uint16_t bit_count{0};
    uint32_t compression{0};
    uint32_t size_image{0};
    int32_t x_pixels_per_meter{0};
    int32_t y_pixels_per_meter{0};
    uint32_t colors_used{0};
    uint32_t colors_important{0};
};
#pragma pack(pop)

std::shared_ptr<Image> BmpDecoder::decode(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return nullptr;

    BMPFileHeader file_header;
    file.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    if (file_header.file_type != 0x4D42) return nullptr;

    BMPInfoHeader info_header;
    file.read(reinterpret_cast<char*>(&info_header), sizeof(info_header));

    // Support only uncompressed or bitfields 24-bit/32-bit BMP
    if (info_header.compression != 0 && info_header.compression != 3) {
        return nullptr;
    }

    if (info_header.bit_count != 24 && info_header.bit_count != 32) {
        return nullptr;
    }

    int width = info_header.width;
    int height = info_header.height;
    bool flip = true;
    if (height < 0) {
        flip = false;
        height = -height;
    }

    file.seekg(file_header.offset_data, std::ios::beg);

    std::vector<uint8_t> pixels(width * height * 4, 0);

    if (info_header.bit_count == 24) {
        int row_stride = (width * 3 + 3) & ~3; // aligned to 4 bytes
        std::vector<uint8_t> row_buffer(row_stride);

        for (int y = 0; y < height; ++y) {
            file.read(reinterpret_cast<char*>(row_buffer.data()), row_stride);
            int target_y = flip ? (height - 1 - y) : y;
            uint8_t* dest_row = pixels.data() + target_y * width * 4;

            for (int x = 0; x < width; ++x) {
                dest_row[x * 4 + 0] = row_buffer[x * 3 + 2]; // R
                dest_row[x * 4 + 1] = row_buffer[x * 3 + 1]; // G
                dest_row[x * 4 + 2] = row_buffer[x * 3 + 0]; // B
                dest_row[x * 4 + 3] = 255;                  // A
            }
        }
    } else if (info_header.bit_count == 32) {
        int row_stride = width * 4;
        std::vector<uint8_t> row_buffer(row_stride);

        for (int y = 0; y < height; ++y) {
            file.read(reinterpret_cast<char*>(row_buffer.data()), row_stride);
            int target_y = flip ? (height - 1 - y) : y;
            uint8_t* dest_row = pixels.data() + target_y * width * 4;

            for (int x = 0; x < width; ++x) {
                dest_row[x * 4 + 0] = row_buffer[x * 4 + 2]; // R
                dest_row[x * 4 + 1] = row_buffer[x * 4 + 1]; // G
                dest_row[x * 4 + 2] = row_buffer[x * 4 + 0]; // B
                dest_row[x * 4 + 3] = row_buffer[x * 4 + 3]; // A
            }
        }
    }

    return std::make_shared<Image>(width, height, std::move(pixels));
}

} // namespace ooey::renderer
