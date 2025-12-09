#include "image_encoder.h"

namespace senseauto {

class ImageEncoder::Impl {
public:
    Impl() = default;
    ~Impl() = default;
};

ImageEncoder::ImageEncoder() : impl_(new Impl()) {}

ImageEncoder::~ImageEncoder() {
    delete impl_;
}

bool ImageEncoder::EncodeJPEG(const unsigned char* data, int width, int height, int channels, 
                              std::vector<unsigned char>& output, int quality) {
    // TODO: 实现JPEG编码功能
    // 当前为占位实现，防止编译错误
    return false;
}

bool ImageEncoder::EncodePNG(const unsigned char* data, int width, int height, int channels,
                             std::vector<unsigned char>& output) {
    // TODO: 实现PNG编码功能
    // 当前为占位实现，防止编译错误
    return false;
}

} // namespace senseauto