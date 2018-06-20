cmake_minimum_required(VERSION 3.9)
project(NovaEngine)

set(VK_INCLUDE)
set(VK_LIB)
set(VKW_INCLUDE)
set(VKW_LIB)
set(GLFW_INCLUDE)
set(GLFW_LIB)

add_library(NovaEngine
    "src/engine.cpp"
    "src/renderer.cpp"
)
target_include_directories(NovaEngine
    PUBLIC "${PROJECT_SOURCE_DIR}/include"
    PUBLIC "${VK_INCLUDE}"
    PUBLIC "${VKW_INCLUDE}"
    PUBLIC "${GLFW_INCLUDE}"
)
target_link_libraries(NovaEngine
    ${VK_LIB}
    ${VKW_LIB}
    ${GLFW_LIB}
)

add_subdirectory("test")