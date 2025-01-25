#pragma once

#include <glm/glm.hpp>

// Standard scene info
struct UniformSceneInfo {
    glm::mat4 transform;
};

// Uniform for SceneLevel objects
struct UniformLevelInfo {
    glm::ivec2 texelSize;

    float     rain;
    float     light;
    glm::vec2 screenOffset;

    glm::vec2 PADDING1;

    glm::vec4 lightDirAndPixelSize;
    float     fogAmount;
    float     waterLevel;
    float     Grime;
    float     SwarmRoom;
    float     WetTerrain;
    float     cloudsSpeed;
    float     darkness;
    float     contrast;
    float     saturation;
    float     hue;
    float     brightness;

    float PADDING2;

    glm::vec4 aboveCloudsAtmosphereColor;
    float     rimFix;
};
