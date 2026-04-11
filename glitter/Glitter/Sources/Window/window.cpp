#include "Window/window.hpp"
#include "GLFW/glfw3.h"

Window::Window(int screenWidth, int screenHeight) {
  init_GLFW();
  m_screenHeight = screenHeight;
  m_screenWidth = screenWidth;
  m_Window = glfwCreateWindow(m_screenWidth, m_screenHeight, "OpenGL", nullptr,
                              nullptr);
  glfwMakeContextCurrent(m_Window);
  gladLoadGL();
  fprintf(stderr, "OpenGL %s\n", glGetString(GL_VERSION));
  glViewport(0, 0, m_screenWidth, m_screenHeight);
  glfwSetFramebufferSizeCallback(m_Window, framebuffer_size_callback);
};
Window::~Window() { glfwTerminate(); }

void Window::framebuffer_size_callback(GLFWwindow *window, int width,
                                       int height) {
  glViewport(0, 0, width, height);
}

void Window::init_GLFW() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
};

void Window::handle_input() {
  if (glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(m_Window, true);

  if (glfwGetKey(m_Window, GLFW_KEY_0) == GLFW_PRESS)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  if (glfwGetKey(m_Window, GLFW_KEY_1) == GLFW_PRESS)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
