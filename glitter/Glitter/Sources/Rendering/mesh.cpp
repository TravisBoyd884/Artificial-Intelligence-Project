#include "Rendering/mesh.hpp"

#include <utility> // std::move

Mesh::Mesh(std::vector<Vertex>      vertices,
           std::vector<unsigned int> indices,
           std::vector<MeshTexture>  textures)
    : m_vertices(std::move(vertices))
    , m_indices(std::move(indices))
    , m_textures(std::move(textures))
{
  setupMesh();
}

Mesh::~Mesh() {
  if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
  if (m_VBO) glDeleteBuffers(1, &m_VBO);
  if (m_EBO) glDeleteBuffers(1, &m_EBO);
}

Mesh::Mesh(Mesh &&other) noexcept
    : m_VAO(other.m_VAO)
    , m_VBO(other.m_VBO)
    , m_EBO(other.m_EBO)
    , m_vertices(std::move(other.m_vertices))
    , m_indices(std::move(other.m_indices))
    , m_textures(std::move(other.m_textures))
{
  other.m_VAO = 0;
  other.m_VBO = 0;
  other.m_EBO = 0;
}

Mesh &Mesh::operator=(Mesh &&other) noexcept {
  if (this != &other) {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
    if (m_EBO) glDeleteBuffers(1, &m_EBO);

    m_VAO = other.m_VAO;
    m_VBO = other.m_VBO;
    m_EBO = other.m_EBO;
    other.m_VAO = 0;
    other.m_VBO = 0;
    other.m_EBO = 0;

    m_vertices = std::move(other.m_vertices);
    m_indices  = std::move(other.m_indices);
    m_textures = std::move(other.m_textures);
  }
  return *this;
}

void Mesh::setupMesh() {
  glGenVertexArrays(1, &m_VAO);
  glGenBuffers(1, &m_VBO);
  glGenBuffers(1, &m_EBO);

  glBindVertexArray(m_VAO);

  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)),
               m_vertices.data(),
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(m_indices.size() * sizeof(unsigned int)),
               m_indices.data(),
               GL_STATIC_DRAW);

  // position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        reinterpret_cast<void *>(offsetof(Vertex, position)));
  // normal
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        reinterpret_cast<void *>(offsetof(Vertex, normal)));
  // texture coordinates
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        reinterpret_cast<void *>(offsetof(Vertex, texCoords)));

  glBindVertexArray(0);
}

void Mesh::draw(const Shader &shader) const {
  unsigned int diffuseNr  = 0;
  unsigned int specularNr = 0;

  for (unsigned int i = 0; i < m_textures.size(); i++) {
    glActiveTexture(GL_TEXTURE0 + i);

    // Build the uniform name: texture_diffuse0, texture_specular0, etc.
    unsigned int nr = 0;
    const std::string &type = m_textures[i].type;
    if (type == "texture_diffuse")       nr = diffuseNr++;
    else if (type == "texture_specular") nr = specularNr++;

    shader.setInt(type + std::to_string(nr), static_cast<int>(i));
    glBindTexture(GL_TEXTURE_2D, m_textures[i].id);
  }

  glBindVertexArray(m_VAO);
  glDrawElements(GL_TRIANGLES,
                 static_cast<GLsizei>(m_indices.size()),
                 GL_UNSIGNED_INT,
                 nullptr);

  glBindVertexArray(0);
  glActiveTexture(GL_TEXTURE0);
}
