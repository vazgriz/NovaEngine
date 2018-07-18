cmake_minimum_required(VERSION 3.9)
project(Test)

add_executable(Test main.cpp)
target_link_libraries(Test NovaEngine)
target_include_directories(Test
    PUBLIC ${GLM_INCLUDE}
)