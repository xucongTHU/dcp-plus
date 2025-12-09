#ifndef IMAGE_ENCODER_H
#define IMAGE_ENCODER_H

#include <string>
#include <vector>

namespace senseauto {

class ImageEncoder {
public:
    ImageEncoder();
    ~ImageEncoder();

    // JPEG encoding
    bool EncodeJPEG(const unsigned char* data, int width, int height, int channels, 
                    std::vector<unsigned char>& output, int quality = 90);

    // PNG encoding
    bool EncodePNG(const unsigned char* data, int width, int height, int channels,
                   std::vector<unsigned char>& output);

private:
    // Private implementation details
    class Impl;
    Impl* impl_;
};

} // namespace senseauto

#endif // IMAGE_ENCODER_H