#pragma once

#include <string_view>

namespace lumen::render::builtin {

// Vertex layout: vec2 position (location 0), vec3 color (location 1). Uniform: float u_angle (radians).
inline constexpr std::string_view kColorVertex = R"GLSL(#version 300 es
precision highp float;
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec3 a_color;
uniform float u_angle;
out vec3 v_color;
void main() {
    float c = cos(u_angle);
    float s = sin(u_angle);
    vec2 p = vec2(c * a_pos.x - s * a_pos.y, s * a_pos.x + c * a_pos.y);
    gl_Position = vec4(p, 0.0, 1.0);
    v_color = a_color;
}
)GLSL";

inline constexpr std::string_view kColorFragment = R"GLSL(#version 300 es
precision highp float;
in vec3 v_color;
out vec4 o_color;
void main() {
    o_color = vec4(v_color, 1.0);
}
)GLSL";

}  // namespace lumen::render::builtin
