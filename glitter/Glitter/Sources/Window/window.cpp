#include "Window/window.hpp"

Window::Window(int screenWidth, int screenHeight) {
  init_GLFW();
  m_screenHeight = screenHeight;
  m_screenWidth  = screenWidth;
  m_Window = glfwCreateWindow(m_screenWidth, m_screenHeight, "OpenGL", nullptr, nullptr);
  glfwMakeContextCurrent(m_Window);
  gladLoadGL();
  fprintf(stderr, "OpenGL %s\n", glGetString(GL_VERSION));
  glViewport(0, 0, m_screenWidth, m_screenHeight);
  glfwSetFramebufferSizeCallback(m_Window, framebuffer_size_callback);
}

Window::~Window() { glfwTerminate(); }

void Window::init_GLFW() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
}

void Window::framebuffer_size_callback(GLFWwindow * /*window*/, int width, int height) {
  glViewport(0, 0, width, height);
}

void Window::handle_input() {
  if (glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(m_Window, true);

  if (glfwGetKey(m_Window, GLFW_KEY_0) == GLFW_PRESS)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  if (glfwGetKey(m_Window, GLFW_KEY_1) == GLFW_PRESS)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Window::captureMouse() {
  glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetWindowUserPointer(m_Window, this);
  glfwSetCursorPosCallback(m_Window, mouse_callback);
}

// Static GLFW callback — retrieves the Window instance via the user pointer.
void Window::mouse_callback(GLFWwindow *window, double xpos, double ypos) {
  Window *self = static_cast<Window *>(glfwGetWindowUserPointer(window));

  // On the first event, seed the last position so there is no initial jump.
  if (self->m_firstMouse) {
    self->m_lastMouseX = xpos;
    self->m_lastMouseY = ypos;
    self->m_firstMouse  = false;
  }

  // Accumulate offset; Y is inverted so that moving the mouse up is positive.
  self->m_mouseOffsetX += static_cast<float>(xpos - self->m_lastMouseX);
  self->m_mouseOffsetY += static_cast<float>(self->m_lastMouseY - ypos);
  self->m_lastMouseX    = xpos;
  self->m_lastMouseY    = ypos;
}

glm::vec2 Window::getMouseDelta() {
  glm::vec2 delta(m_mouseOffsetX, m_mouseOffsetY);
  m_mouseOffsetX = 0.0f;
  m_mouseOffsetY = 0.0f;
  return delta;
}
