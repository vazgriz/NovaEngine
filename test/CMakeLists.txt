cmake_minimum_required(VERSION 3.9)
project(Test)

set(SHADER_SOURCES
    "${PROJECT_SOURCE_DIR}/shader.vert" ;
    "${PROJECT_SOURCE_DIR}/shader.frag" ;
)
set(SPIRV_BINARY_FILES)

foreach(SHADER_SOURCE ${SHADER_SOURCES})
    get_filename_component(FILE_NAME ${SHADER_SOURCE} NAME)
    set(SPIRV "${PROJECT_BINARY_DIR}/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSL_VALIDATOR} ${SHADER_SOURCE} -o ${SPIRV}
        DEPENDS ${SHADER_SOURCE})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(SHADER_SOURCE)

add_custom_target(
    Shaders
    DEPENDS ${SPIRV_BINARY_FILES}
)

add_executable(Test main.cpp)
target_link_libraries(Test NovaEngine)
target_include_directories(Test
    PUBLIC ${GLM_INCLUDE}
)
add_dependencies(Test Shaders)