cmake_minimum_required(VERSION 4.00)
project(Church-of-hopper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(Church-of-hopper
        src/main.cpp
        src/resource/Material.cpp
        src/resource/Mesh.cpp
        src/resource/stb_image.cpp
        include/camera/Camera.h
        src/camera/Camera.cpp
        include/camera/Camera.h
        src/camera/Camera.cpp
        include/renderer/Shader.h
        src/renderer/Shader.cpp
        src/resource/texture.cpp
        include/renderer/Renderer.h
        src/renderer/Renderer.cpp
        include/camera/CameraControl.h
        src/camera/CameraControl.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        OpenGL::GL
        glfw
        glad::glad
        glm::glm
)