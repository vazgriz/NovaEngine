cmake_minimum_required(VERSION 3.10)
project(NovaEngine)

set(VK_INCLUDE)
set(VK_LIB)
set(VKW_INCLUDE)
set(VKW_LIB)
set(GLFW_INCLUDE)
set(GLFW_LIB)
set(GLM_INCLUDE)
set(BOOST_INCLUDE)

add_library(NovaEngine
    "src/Engine.cpp"
    "src/Renderer.cpp"
    "src/Window.cpp"
    "src/FrameGraph.cpp"
    "src/Memory.cpp"
    "src/IGenericAllocator.cpp"
    "src/LinearAllocator.cpp"
    "src/StackAllocator.cpp"
    "src/FreeListAllocator.cpp"
    "src/IRawAllocator.cpp"
    "src/RawAllocator.cpp"
    "src/IResourceAllocator.cpp"
    "src/Allocator.cpp"
    "src/StagingAllocator.cpp"
    "src/TransferNode.cpp"
    "src/CameraManager.cpp"
    "src/Camera.cpp"
    "src/PerspectiveCamera.cpp"
    "src/Input.cpp"
    "src/FreeCam.cpp"
    "src/FrameGraph.cpp"
    "src/Texture.cpp"
    "src/Mesh.cpp"
    "src/Clock.cpp"
)
target_include_directories(NovaEngine
    PUBLIC "${PROJECT_SOURCE_DIR}/include"
    PUBLIC "${VK_INCLUDE}"
    PUBLIC "${VKW_INCLUDE}"
    PUBLIC "${GLFW_INCLUDE}"
    PUBLIC "${GLM_INCLUDE}"
    PUBLIC "${BOOST_INCLUDE}"
)
target_link_libraries(NovaEngine
    ${VK_LIB}
    ${VKW_LIB}
    ${GLFW_LIB}
)

set_target_properties(NovaEngine PROPERTIES CXX_STANDARD 17)

add_subdirectory("test")