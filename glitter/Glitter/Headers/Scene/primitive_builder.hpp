#pragma once

#include "Rendering/mesh.hpp"
#include <glm/glm.hpp>
#include <string>

namespace PrimitiveBuilder {

Mesh buildFloor    (float halfSize, float uvScale, unsigned int textureID);
Mesh buildWalls    (float width, float depth, float height, unsigned int textureID);
Mesh buildCeiling  (float halfSize, float height, unsigned int textureID);
Mesh buildBackboard(glm::vec3 centre, float width, float height, unsigned int textureID);
Mesh buildHoop     (glm::vec3 centre, float radius, float tubeRadius, int segments, unsigned int textureID);
Mesh buildSphere   (float radius, int stacks, int slices, unsigned int textureID);

unsigned int solidColorTexture(unsigned char r, unsigned char g, unsigned char b);
unsigned int loadTexture(const std::string& path);

} // namespace PrimitiveBuilder
