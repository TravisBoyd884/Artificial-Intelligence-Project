#pragma once
#include <vector>
#include <cstdint>

class FBOCapture {
public:
    FBOCapture(int width, int height);
    ~FBOCapture();

    void bind();
    void unbind();
    std::vector<uint8_t> readPixels();

    int width()  const { return m_width; }
    int height() const { return m_height; }

private:
    int          m_width, m_height;
    unsigned int m_fbo      = 0;
    unsigned int m_colorTex = 0;
    unsigned int m_depthRbo = 0;
};
