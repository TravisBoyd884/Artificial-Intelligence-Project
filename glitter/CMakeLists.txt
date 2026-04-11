cmake_minimum_required(VERSION 4.1.4)
project(Glitter LANGUAGES C CXX)

# -----------------------------
# Enable compile_commands.json
# -----------------------------
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------
# Add Vendor Libraries
# -----------------------------
# GLFW
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
add_subdirectory(Glitter/Vendor/glfw)

# Assimp
option(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
option(ASSIMP_BUILD_SAMPLES OFF)
option(ASSIMP_BUILD_TESTS OFF)
add_subdirectory(Glitter/Vendor/assimp)

# Bullet
option(BUILD_BULLET2_DEMOS OFF)
option(BUILD_CPU_DEMOS OFF)
option(BUILD_EXTRAS OFF)
option(BUILD_OPENGL3_DEMOS OFF)
option(BUILD_UNIT_TESTS OFF)
add_subdirectory(Glitter/Vendor/bullet)

# -----------------------------
# Set Compile Flags
# -----------------------------
if(MSVC)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
else()
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(GLAD_LIBRARIES dl) # Linux specific
endif()

# -----------------------------
# Project Sources
# -----------------------------
# Headers and source files
file(GLOB_RECURSE PROJECT_HEADERS Glitter/Headers/*.hpp)
file(GLOB_RECURSE PROJECT_SOURCES Glitter/Sources/*.cpp)
file(GLOB VENDORS_SOURCES Glitter/Vendor/glad/src/glad.c)
file(GLOB PROJECT_SHADERS Glitter/Shaders/*.comp
                          Glitter/Shaders/*.frag
                          Glitter/Shaders/*.geom
                          Glitter/Shaders/*.vert)
file(GLOB PROJECT_CONFIGS CMakeLists.txt
                          Readme.md
                          .gitattributes
                          .gitignore
                          .gitmodules)

# Group files in IDEs
source_group("Headers" FILES ${PROJECT_HEADERS})
source_group("Sources" FILES ${PROJECT_SOURCES})
source_group("Vendors" FILES ${VENDORS_SOURCES})
source_group("Shaders" FILES ${PROJECT_SHADERS})

# -----------------------------
# Create Executable
# -----------------------------
add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
    ${PROJECT_SHADERS}
    ${PROJECT_CONFIGS}
    ${VENDORS_SOURCES}
)

# -----------------------------
# Target Include Directories
# -----------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    Glitter/Headers/
    Glitter/Vendor/assimp/include/
    Glitter/Vendor/bullet/src/
    Glitter/Vendor/glad/include/
    Glitter/Vendor/glfw/include/
    Glitter/Vendor/glm/
    Glitter/Vendor/stb/
)

# -----------------------------
# Compile Definitions
# -----------------------------
target_compile_definitions(${PROJECT_NAME} PRIVATE
    GLFW_INCLUDE_NONE
    PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

# -----------------------------
# Compile Options
# -----------------------------
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# -----------------------------
# Link Libraries
# -----------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    assimp glfw ${GLFW_LIBRARIES} ${GLAD_LIBRARIES}
    BulletDynamics BulletCollision LinearMath
)

# -----------------------------
# Output Directory
# -----------------------------
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}
)

# -----------------------------
# Post-build: copy shaders
# -----------------------------
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${CMAKE_SOURCE_DIR}/Glitter/Shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
