cmake_minimum_required(VERSION 3.9)
project(NovaEngine)

set(VK_INCLUDE)
set(VK_LIB)
set(VKW_INCLUDE)
set(VKW_LIB)
set(GLFW_INCLUDE)
set(GLFW_LIB)

add_library(NovaEngine
    "src/Engine.cpp"
    "src/Renderer.cpp"
    "src/Window.cpp"
    "src/QueueGraph.cpp"
    "src/RenderGraph.cpp"
    "src/Memory.cpp"
    "src/IAllocator.cpp"
    "src/LinearAllocator.cpp"
    "src/StackAllocator.cpp"
    "src/FreeListAllocator.cpp"
    "src/IRawAllocator.cpp"
    "src/RawAllocator.cpp"
    "src/IResourceAllocator.cpp"
    "src/Allocator.cpp"
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