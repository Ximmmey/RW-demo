#version 450

layout (binding = 0, set = 0) uniform SceneInfo {
    mat4x4 transform;
} scene;

layout (binding = 0, set = 1) uniform LevelInfo {
    // Joar stuff
    ivec2 texelSize;

    float rain;
    float light;
    vec2  screenOffset;

    vec4  lightDirAndPixelSize;
    float fogAmount;
    float waterLevel;
    float Grime;
    float SwarmRoom;
    float WetTerrain;
    float cloudsSpeed;
    float darkness;
    float contrast;
    float saturation;
    float hue;
    float brightness;
    vec4  aboveCloudsAtmosphereColor;
    float rimFix;
} level;

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec4 v_col;

layout(location = 0) out vec2 f_uv;
layout(location = 1) out vec2 f_uv2; // what?

void main() {
    gl_Position = scene.transform * vec4(v_pos, 1);

    f_uv = v_uv;
    f_uv2 = f_uv - level.texelSize * .5 * level.rimFix;
}
