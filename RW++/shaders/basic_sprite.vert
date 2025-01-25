#version 450

layout(binding = 0, set = 0) uniform SceneInfo {
    mat4 transform;
} scene;

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec4 v_col;

layout(location = 0) out vec2 f_uv;
layout(location = 1) out vec4 f_col;

void main() {
    gl_Position = scene.transform * vec4(v_pos, 1);
    f_uv = v_uv;
    f_col = v_col;
}
