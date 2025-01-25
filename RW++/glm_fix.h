#pragma once

#include <glm/mat4x4.hpp>

inline glm::mat4 ortho(const float left, const float right, const float bottom, const float top, const float near_plane, const float far_plane) {
    glm::mat4 result(1.0f);  // Start with identity matrix

    result[0][0] = 2.0f / (right - left);
    result[1][1] = 2.0f / (top - bottom);
    result[2][2] = 1.0f / (near_plane - far_plane);

    result[3][0] = (left + right) / (left - right);
    result[3][1] = (top + bottom) / (bottom - top);
    result[3][2] = near_plane / (near_plane - far_plane);
    result[3][3] = 1.0f;

    return result;
}
