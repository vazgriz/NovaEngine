#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;

layout(location = 0) out vec3 vColor;

layout(binding = 0) uniform Camera {
    mat4 view;
    mat4 rotationView;
    mat4 proj;
} camera;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = camera.proj * camera.view * vec4(pos, 1.0);
    vColor = col;
}