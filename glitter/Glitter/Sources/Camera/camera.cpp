#include "Camera/camera.hpp"

Camera::Camera(glm::vec3 position, glm::vec3 worldUp, float yaw, float pitch)
    : m_position(position)
    , m_worldUp(worldUp)
    , m_yaw(yaw)
    , m_pitch(pitch)
    , m_movementSpeed(DEFAULT_SPEED)
    , m_mouseSensitivity(DEFAULT_SENSITIVITY)
    , m_zoom(DEFAULT_ZOOM)
{
  updateVectors();
}

glm::mat4 Camera::getViewMatrix() const {
  return glm::lookAt(m_position, m_position + m_front, m_up);
}

void Camera::processKeyboard(CameraMovement direction, float deltaTime) {
  float velocity = m_movementSpeed * deltaTime;
  switch (direction) {
    case CameraMovement::FORWARD:  m_position += m_front * velocity;  break;
    case CameraMovement::BACKWARD: m_position -= m_front * velocity;  break;
    case CameraMovement::LEFT:     m_position -= m_right * velocity;  break;
    case CameraMovement::RIGHT:    m_position += m_right * velocity;  break;
  }
}

void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
  m_yaw   += xoffset * m_mouseSensitivity;
  m_pitch += yoffset * m_mouseSensitivity;

  if (constrainPitch) {
    if (m_pitch >  89.0f) m_pitch =  89.0f;
    if (m_pitch < -89.0f) m_pitch = -89.0f;
  }

  updateVectors();
}

void Camera::updateVectors() {
  glm::vec3 front;
  front.x = glm::cos(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch));
  front.y = glm::sin(glm::radians(m_pitch));
  front.z = glm::sin(glm::radians(m_yaw)) * glm::cos(glm::radians(m_pitch));

  m_front = glm::normalize(front);
  m_right = glm::normalize(glm::cross(m_front, m_worldUp));
  m_up    = glm::normalize(glm::cross(m_right, m_front));
}
