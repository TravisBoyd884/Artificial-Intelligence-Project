#include "Scene/primitive_builder.hpp"

#include <glad/glad.h>
#include <glm/gtc/constants.hpp>
#include <stb_image.h>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

static Mesh meshFromArrays(const std::vector<float>& verts,
                           const std::vector<unsigned int>& indices,
                           unsigned int textureID) {
  std::vector<Vertex> vertices;
  vertices.reserve(verts.size() / 8);
  for (size_t i = 0; i + 7 < verts.size(); i += 8) {
    Vertex v;
    v.position  = {verts[i + 0], verts[i + 1], verts[i + 2]};
    v.normal    = {verts[i + 3], verts[i + 4], verts[i + 5]};
    v.texCoords = {verts[i + 6], verts[i + 7]};
    vertices.push_back(v);
  }

  std::vector<MeshTexture> textures;
  if (textureID != 0) {
    MeshTexture t;
    t.id   = textureID;
    t.type = "texture_diffuse";
    t.path = "";
    textures.push_back(t);
  }

  return Mesh(std::move(vertices), indices, std::move(textures));
}

namespace PrimitiveBuilder {

Mesh buildFloor(float halfSize, float uvScale, unsigned int textureID) {
  float h = halfSize;
  float u = uvScale;
  std::vector<float> verts = {
    -h, 0.0f, -h,   0.0f, 1.0f, 0.0f,   0.0f, u,
     h, 0.0f, -h,   0.0f, 1.0f, 0.0f,   u,    u,
     h, 0.0f,  h,   0.0f, 1.0f, 0.0f,   u,    0.0f,
    -h, 0.0f,  h,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
  };
  return meshFromArrays(verts, {0, 1, 2,  2, 3, 0}, textureID);
}

Mesh buildCeiling(float halfSize, float height, unsigned int textureID) {
  float h = halfSize;
  std::vector<float> verts = {
    -h, height, -h,   0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
     h, height, -h,   0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
     h, height,  h,   0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
    -h, height,  h,   0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
  };
  return meshFromArrays(verts, {0, 3, 2,  2, 1, 0}, textureID);
}

Mesh buildWalls(float width, float depth, float height, unsigned int textureID) {
  float hw = width * 0.5f;
  float hd = depth * 0.5f;

  std::vector<float> verts;
  std::vector<unsigned int> indices;
  unsigned int base = 0;

  auto addWall = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d,
                     glm::vec3 normal) {
    float tw = glm::length(b - a);
    float th = height;
    for (auto [pos, uv] : std::initializer_list<std::pair<glm::vec3, glm::vec2>>{
        {a, {0.0f, 0.0f}}, {b, {tw, 0.0f}}, {c, {tw, th}}, {d, {0.0f, th}}}) {
      verts.insert(verts.end(), {pos.x, pos.y, pos.z,
                                 normal.x, normal.y, normal.z,
                                 uv.x, uv.y});
    }
    indices.insert(indices.end(), {base, base+1, base+2,  base+2, base+3, base});
    base += 4;
  };

  addWall({-hw, 0.0f,  hd}, { hw, 0.0f,  hd}, { hw, height,  hd}, {-hw, height,  hd}, {0.0f,  0.0f, -1.0f});
  addWall({ hw, 0.0f, -hd}, {-hw, 0.0f, -hd}, {-hw, height, -hd}, { hw, height, -hd}, {0.0f,  0.0f,  1.0f});
  addWall({-hw, 0.0f, -hd}, {-hw, 0.0f,  hd}, {-hw, height,  hd}, {-hw, height, -hd}, {1.0f,  0.0f,  0.0f});
  addWall({ hw, 0.0f,  hd}, { hw, 0.0f, -hd}, { hw, height, -hd}, { hw, height,  hd}, {-1.0f, 0.0f,  0.0f});

  return meshFromArrays(verts, indices, textureID);
}

Mesh buildBackboard(glm::vec3 centre, float width, float height, unsigned int textureID) {
  float hw = width  * 0.5f;
  float hh = height * 0.5f;
  std::vector<float> verts = {
    centre.x - hw, centre.y - hh, centre.z,   0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
    centre.x + hw, centre.y - hh, centre.z,   0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
    centre.x + hw, centre.y + hh, centre.z,   0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
    centre.x - hw, centre.y + hh, centre.z,   0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
  };
  return meshFromArrays(verts, {0, 1, 2,  2, 3, 0}, textureID);
}

Mesh buildHoop(glm::vec3 centre, float radius, float tubeRadius,
               int segments, unsigned int textureID) {
  std::vector<float> verts;
  std::vector<unsigned int> indices;
  unsigned int base = 0;

  for (int i = 0; i < segments; i++) {
    float a0 = glm::two_pi<float>() * static_cast<float>(i)     / static_cast<float>(segments);
    float a1 = glm::two_pi<float>() * static_cast<float>(i + 1) / static_cast<float>(segments);

    glm::vec3 p0(centre.x + radius * std::cos(a0), centre.y, centre.z + radius * std::sin(a0));
    glm::vec3 p1(centre.x + radius * std::cos(a1), centre.y, centre.z + radius * std::sin(a1));

    glm::vec3 outward = glm::normalize(p0 + p1 - 2.0f * centre);
    outward.y = 0.0f;
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    std::vector<glm::vec3> corners = {
      p0 - up * tubeRadius, p1 - up * tubeRadius,
      p1 + up * tubeRadius, p0 + up * tubeRadius,
    };
    std::vector<glm::vec2> uvs = {{0,0},{1,0},{1,1},{0,1}};

    for (int k = 0; k < 4; k++) {
      verts.insert(verts.end(), {
        corners[k].x, corners[k].y, corners[k].z,
        outward.x, outward.y, outward.z,
        uvs[k].x, uvs[k].y
      });
    }
    indices.insert(indices.end(), {base, base+1, base+2,  base+2, base+3, base});
    base += 4;
  }

  return meshFromArrays(verts, indices, textureID);
}

Mesh buildSphere(float radius, int stacks, int slices, unsigned int textureID) {
  std::vector<float> verts;
  std::vector<unsigned int> indices;

  for (int i = 0; i <= stacks; i++) {
    float phi = glm::pi<float>() * static_cast<float>(i) / static_cast<float>(stacks);
    for (int j = 0; j <= slices; j++) {
      float theta = glm::two_pi<float>() * static_cast<float>(j) / static_cast<float>(slices);
      glm::vec3 n(std::sin(phi) * std::cos(theta),
                  std::cos(phi),
                  std::sin(phi) * std::sin(theta));
      glm::vec3 p = n * radius;
      verts.insert(verts.end(), {
        p.x, p.y, p.z, n.x, n.y, n.z,
        static_cast<float>(j) / static_cast<float>(slices),
        static_cast<float>(i) / static_cast<float>(stacks)
      });
    }
  }

  for (int i = 0; i < stacks; i++) {
    for (int j = 0; j < slices; j++) {
      unsigned int a = static_cast<unsigned int>(i * (slices + 1) + j);
      unsigned int b = a + static_cast<unsigned int>(slices + 1);
      indices.insert(indices.end(), {a, b, a+1,  b, b+1, a+1});
    }
  }

  return meshFromArrays(verts, indices, textureID);
}

unsigned int solidColorTexture(unsigned char r, unsigned char g, unsigned char b) {
  unsigned char pixel[] = {r, g, b};
  unsigned int id;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, pixel);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  return id;
}

unsigned int loadTexture(const std::string& path) {
  int w, h, ch;
  unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 0);
  if (!data) {
    std::cerr << "loadTexture: not found: " << path << '\n';
    return solidColorTexture(128, 128, 128);
  }

  GLenum fmt = (ch == 4) ? GL_RGBA : (ch == 1) ? GL_RED : GL_RGB;
  unsigned int id;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(fmt), w, h, 0,
               fmt, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  stbi_image_free(data);
  return id;
}

} // namespace PrimitiveBuilder
