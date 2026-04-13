#include "RL/fbo_capture.hpp"
#include <glad/glad.h>
#include <stdexcept>

FBOCapture::FBOCapture(int width, int height) : m_width(width), m_height(height) {
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glGenTextures(1, &m_colorTex);
    glBindTexture(GL_TEXTURE_2D, m_colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, m_colorTex, 0);

    glGenRenderbuffers(1, &m_depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, m_depthRbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("FBOCapture: framebuffer incomplete");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FBOCapture::~FBOCapture() {
    glDeleteFramebuffers(1, &m_fbo);
    glDeleteTextures(1, &m_colorTex);
    glDeleteRenderbuffers(1, &m_depthRbo);
}

void FBOCapture::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void FBOCapture::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::vector<uint8_t> FBOCapture::readPixels() {
    std::vector<uint8_t> pixels(static_cast<size_t>(m_width * m_height * 3));
    glReadPixels(0, 0, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    return pixels;
}
