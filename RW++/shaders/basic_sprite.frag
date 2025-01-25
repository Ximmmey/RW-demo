#version 450

layout(location = 0) in vec2 f_uv;
layout(location = 1) in vec4 f_color;

layout(location = 0) out vec4 out_color;

layout(binding = 0, set = 1) uniform sampler2D tex;

void main() {
    out_color = texture(tex, f_uv) * f_color;
    if (out_color.a == 0) discard;
}
