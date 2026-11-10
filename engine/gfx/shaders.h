// G-Engine — gfx/shaders.h
// GLSL ES 3.00 shaders of the renderer (work unchanged on WebGL2 and OpenGL ES 3).
#pragma once

namespace ge::shaders {

// ------------------------------------------------------------------------------------------
// 1. Sprites -> G-buffer (colour, normal + emissive)
// ------------------------------------------------------------------------------------------
inline constexpr const char* kSpriteVertex = R"(#version 300 es
layout(location = 0) in vec2 aCorner;
layout(location = 1) in vec4 aPosSize;   // xy = centre, zw = size
layout(location = 2) in vec4 aRotMisc;   // x = cos, y = sin, z = emissive, w = flip (+1 / -1)
layout(location = 3) in vec4 aUv;        // u0, v0, u1, v1
layout(location = 4) in vec4 aTint;
uniform vec4 uView;                      // xy = camera centre, zw = 2 / view size
out vec2 vUv;
out vec4 vTint;
flat out vec4 vRotMisc;
void main() {
    vec2 local = (aCorner - 0.5) * aPosSize.zw;
    vec2 world = aPosSize.xy + vec2(local.x * aRotMisc.x - local.y * aRotMisc.y, local.x * aRotMisc.y + local.y * aRotMisc.x);
    gl_Position = vec4((world - uView.xy) * uView.zw, 0.0, 1.0);
    vec2 t = vec2(aRotMisc.w < 0.0 ? 1.0 - aCorner.x : aCorner.x, 1.0 - aCorner.y);
    vUv = mix(aUv.xy, aUv.zw, t);
    vTint = aTint;
    vRotMisc = aRotMisc;
}
)";

inline constexpr const char* kSpriteFragment = R"(#version 300 es
precision highp float;   // mediump is too coarse for atlas coordinates on phone GPUs
precision highp sampler2D;
uniform sampler2D uAlbedo;
uniform sampler2D uNormal;
in vec2 vUv;
in vec4 vTint;
flat in vec4 vRotMisc;
layout(location = 0) out vec4 oAlbedo;
layout(location = 1) out vec4 oNormal;
void main() {
    vec4 colour = texture(uAlbedo, vUv) * vTint;
    if (colour.a < 0.01) discard;
    vec4 data = texture(uNormal, vUv);
    vec2 n = data.rg * 2.0 - 1.0;
    n.x *= vRotMisc.w;                                                  // mirrored sprite
    n = vec2(n.x * vRotMisc.x - n.y * vRotMisc.y, n.x * vRotMisc.y + n.y * vRotMisc.x);  // rotated sprite
    float emissive = vRotMisc.z >= 0.0 ? data.b * vRotMisc.z : -vRotMisc.z;
    oAlbedo = vec4(colour.rgb * colour.a, colour.a);                    // premultiplied
    oNormal = vec4((n * 0.5 + 0.5) * colour.a, clamp(emissive, 0.0, 1.0) * colour.a, colour.a);
}
)";

// ------------------------------------------------------------------------------------------
// 2. Shadow map: one row per light, x = direction, depth = distance to the nearest occluder
// ------------------------------------------------------------------------------------------
inline constexpr const char* kShadowVertex = R"(#version 300 es
layout(location = 0) in vec2 aCorner;
layout(location = 1) in vec4 aSegment;   // a.xy, b.xy (world)
layout(location = 2) in vec4 aLight;     // xy = position, z = radius, w = row
layout(location = 3) in vec4 aCopy;      // x = 0 or 1: segments crossing the +-180 degree seam are drawn twice
uniform vec4 uShadowInfo;                // x = 1 / width, y = 1 / rows
flat out vec2 vA;
flat out vec2 vB;
flat out float vRadius;
out float vAngle;
const float PI = 3.14159265359;
void main() {
    vec2 a = aSegment.xy - aLight.xy;
    vec2 b = aSegment.zw - aLight.xy;
    float ta = atan(a.y, a.x);
    float tb = atan(b.y, b.x);
    if (tb - ta > PI) tb -= 2.0 * PI;
    else if (tb - ta < -PI) tb += 2.0 * PI;
    float lo = min(ta, tb), hi = max(ta, tb);
    float shift = 0.0;
    bool hidden = false;
    if (aCopy.x > 0.5) {
        if (hi > PI) shift = -2.0 * PI;
        else if (lo < -PI) shift = 2.0 * PI;
        else hidden = true;
    }
    float t = aCorner.x < 0.5 ? lo : hi;
    float x = (t + shift) / PI + (aCorner.x < 0.5 ? -1.0 : 1.0) * uShadowInfo.x;   // half a texel wider: no gaps
    float y = (aLight.w + aCorner.y) * uShadowInfo.y * 2.0 - 1.0;
    gl_Position = hidden ? vec4(2.0, 2.0, 2.0, 1.0) : vec4(x, y, 0.0, 1.0);
    vA = a;
    vB = b;
    vRadius = aLight.z;
    vAngle = t + (aCorner.x < 0.5 ? -1.0 : 1.0) * uShadowInfo.x * PI;
}
)";

inline constexpr const char* kShadowFragment = R"(#version 300 es
precision highp float;
flat in vec2 vA;
flat in vec2 vB;
flat in float vRadius;
in float vAngle;
void main() {
    // Exact distance from the light to the segment along this direction (no interpolation error).
    vec2 dir = vec2(cos(vAngle), sin(vAngle));
    vec2 e = vB - vA;
    float denom = dir.x * e.y - dir.y * e.x;
    float hit = abs(denom) > 1e-6 ? (vA.x * e.y - vA.y * e.x) / denom : min(length(vA), length(vB));
    if (hit <= 0.0) hit = min(length(vA), length(vB));
    gl_FragDepth = clamp(hit / vRadius, 0.0, 1.0);
}
)";

// ------------------------------------------------------------------------------------------
// 3. Lights: every light is one quad, added on top of each other
// ------------------------------------------------------------------------------------------
inline constexpr const char* kLightVertex = R"(#version 300 es
layout(location = 0) in vec2 aCorner;
layout(location = 1) in vec4 aLight;     // xy = position, z = height, w = radius
layout(location = 2) in vec4 aColor;     // rgb, w = shadow row or -1
layout(location = 3) in vec4 aMisc;      // x = source size
uniform vec4 uView;                      // xy = camera centre, zw = 2 / view size
flat out vec4 vLight;
flat out vec4 vColor;
flat out float vSize;
void main() {
    vec2 world = aLight.xy + (aCorner * 2.0 - 1.0) * aLight.w;
    gl_Position = vec4((world - uView.xy) * uView.zw, 0.0, 1.0);
    vLight = aLight;
    vColor = aColor;
    vSize = aMisc.x;
}
)";

inline constexpr const char* kLightFragment = R"(#version 300 es
precision highp float;
precision highp sampler2D;   // HDR targets must not be read at low precision
uniform sampler2D uNormalTex;
uniform highp sampler2D uShadowTex;
uniform vec4 uView;         // xy = camera centre, zw = 2 / view size
uniform vec2 uInvRes;       // 1 / light buffer size
uniform vec2 uShadowInfo;   // x = shadow map width, y = penetration depth
flat in vec4 vLight;
flat in vec4 vColor;
flat in float vSize;
out vec4 oColor;
const float TAU = 6.28318530718;

float visibilityAt(float texel, int row, float dist, float radius) {
    float w = uShadowInfo.x;
    float t = texel - 0.5;
    float base = floor(t);
    float f = t - base;
    float d0 = texelFetch(uShadowTex, ivec2(int(mod(base, w)), row), 0).r * radius;
    float d1 = texelFetch(uShadowTex, ivec2(int(mod(base + 1.0, w)), row), 0).r * radius;
    // Light reaches a little way into solid things (lit rims on walls) and is then cut off.
    float v0 = 1.0 - smoothstep(0.0, uShadowInfo.y, dist - d0);
    float v1 = 1.0 - smoothstep(0.0, uShadowInfo.y, dist - d1);
    return mix(v0, v1, f);
}

void main() {
    vec2 uv = gl_FragCoord.xy * uInvRes;
    vec2 world = uView.xy + (uv * 2.0 - 1.0) / uView.zw;
    vec2 toLight = vLight.xy - world;
    float dist = length(toLight);
    float radius = vLight.w;
    float x = dist / radius;
    if (x >= 1.0) discard;
    float window = 1.0 - x * x;
    float attenuation = window * window / (1.0 + 5.0 * x * x);

    vec4 g = texture(uNormalTex, uv);
    vec2 nxy = g.xy * 2.0 - 1.0;
    vec3 n = vec3(nxy, sqrt(max(1.0 - dot(nxy, nxy), 0.0)));
    vec3 l = normalize(vec3(toLight, vLight.z));
    float diffuse = max(dot(n, l), 0.0) * 0.9 + 0.1;

    float visibility = 1.0;
    if (vColor.w >= 0.0) {
        int row = int(vColor.w + 0.5);
        float w = uShadowInfo.x;
        float texel = (atan(-toLight.y, -toLight.x) / TAU + 0.5) * w;
        float safeDist = max(dist, 0.05);
        // Blocker search: how far in front of this pixel are the things that block the light?
        float search = clamp(vSize / safeDist * w / TAU, 1.0, 12.0);
        float blockers = 0.0;
        float blockerDist = 0.0;
        for (int i = 0; i < 5; ++i) {
            float o = (float(i) - 2.0) * 0.5 * search;
            float d = texelFetch(uShadowTex, ivec2(int(mod(floor(texel + o), w)), row), 0).r * radius;
            if (d < dist - 0.02) {
                blockers += 1.0;
                blockerDist += d;
            }
        }
        if (blockers > 0.0) {
            blockerDist /= blockers;
            // The penumbra widens with the gap between blocker and receiver: contact-hardening shadows.
            float penumbra = vSize * (dist - blockerDist) / max(blockerDist, 0.05);
            float spread = clamp(penumbra / safeDist * w / TAU, 0.6, 20.0);
            float sum = 0.0;
            for (int i = 0; i < 8; ++i) {
                float o = ((float(i) + 0.5) / 8.0 * 2.0 - 1.0) * spread;
                sum += visibilityAt(texel + o, row, dist, radius);
            }
            visibility = sum / 8.0;
        }
    }
    oColor = vec4(vColor.rgb * (attenuation * diffuse * visibility), 1.0);
}
)";

// ------------------------------------------------------------------------------------------
// Full-screen passes
// ------------------------------------------------------------------------------------------
inline constexpr const char* kFullscreenVertex = R"(#version 300 es
out vec2 vUv;
void main() {
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    vUv = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)";

// 4. colour x (ambient + light) + emissive + haze
inline constexpr const char* kCompositeFragment = R"(#version 300 es
precision highp float;
precision highp sampler2D;   // HDR targets must not be read at low precision
uniform sampler2D uAlbedoTex;
uniform sampler2D uNormalTex;
uniform sampler2D uLightTex;
uniform vec4 uAmbient;   // rgb, a = lighting enabled (1) or not (0)
uniform vec4 uParams;    // x = emissive gain, y = haze
in vec2 vUv;
out vec4 oColor;
void main() {
    vec3 albedo = texture(uAlbedoTex, vUv).rgb;
    float emissive = texture(uNormalTex, vUv).b;
    vec3 light = texture(uLightTex, vUv).rgb;
    vec3 lit = albedo * (uAmbient.rgb + light) + albedo * emissive * uParams.x + light * uParams.y;
    oColor = vec4(mix(albedo, lit, uAmbient.a), 1.0);
}
)";

// 5. bloom: blur while shrinking ...
inline constexpr const char* kBloomDownFragment = R"(#version 300 es
precision highp float;
precision highp sampler2D;   // HDR targets must not be read at low precision
uniform sampler2D uTex;
uniform vec2 uTexel;       // 1 / source size
uniform float uThreshold;  // > 0 on the first step only
in vec2 vUv;
out vec4 oColor;
vec3 pick(vec2 uv) {
    vec3 c = texture(uTex, uv).rgb;
    if (uThreshold > 0.0) {
        float brightness = max(c.r, max(c.g, c.b));
        c *= max(brightness - uThreshold, 0.0) / max(brightness, 1e-4);
        c = min(c, vec3(12.0));
    }
    return c;
}
void main() {
    vec3 c = pick(vUv) * 4.0;
    c += pick(vUv + uTexel * vec2(-1.0, -1.0));
    c += pick(vUv + uTexel * vec2(1.0, -1.0));
    c += pick(vUv + uTexel * vec2(-1.0, 1.0));
    c += pick(vUv + uTexel * vec2(1.0, 1.0));
    oColor = vec4(c / 8.0, 1.0);
}
)";

// ... and while growing again (added on top of the next larger level)
inline constexpr const char* kBloomUpFragment = R"(#version 300 es
precision highp float;
precision highp sampler2D;   // HDR targets must not be read at low precision
uniform sampler2D uTex;
uniform vec2 uTexel;
in vec2 vUv;
out vec4 oColor;
void main() {
    vec3 c = texture(uTex, vUv).rgb * 4.0;
    c += texture(uTex, vUv + uTexel * vec2(-1.0, 0.0)).rgb * 2.0;
    c += texture(uTex, vUv + uTexel * vec2(1.0, 0.0)).rgb * 2.0;
    c += texture(uTex, vUv + uTexel * vec2(0.0, -1.0)).rgb * 2.0;
    c += texture(uTex, vUv + uTexel * vec2(0.0, 1.0)).rgb * 2.0;
    c += texture(uTex, vUv + uTexel * vec2(-1.0, -1.0)).rgb;
    c += texture(uTex, vUv + uTexel * vec2(1.0, -1.0)).rgb;
    c += texture(uTex, vUv + uTexel * vec2(-1.0, 1.0)).rgb;
    c += texture(uTex, vUv + uTexel * vec2(1.0, 1.0)).rgb;
    oColor = vec4(c / 16.0, 1.0);
}
)";

// 6. tone mapping, vignette, dithering
inline constexpr const char* kOutputFragment = R"(#version 300 es
precision highp float;
precision highp sampler2D;   // HDR targets must not be read at low precision
uniform sampler2D uSceneTex;
uniform sampler2D uBloomTex;
uniform vec4 uParams;   // x = exposure, y = bloom strength, z = vignette, w = lighting enabled
in vec2 vUv;
out vec4 oColor;
vec3 aces(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}
void main() {
    vec3 c = texture(uSceneTex, vUv).rgb;
    c += texture(uBloomTex, vUv).rgb * uParams.y;
    c *= uParams.x;
    vec2 q = vUv - 0.5;
    c *= 1.0 - uParams.z * smoothstep(0.25, 1.1, dot(q, q) * 2.4);
    c = mix(clamp(c, 0.0, 1.0), aces(c), uParams.w);
    c = pow(c, vec3(1.0 / 2.2));
    // A little noise hides the colour banding that dark gradients would otherwise show.
    float noise = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
    oColor = vec4(c + (noise - 0.5) / 255.0, 1.0);
}
)";

}  // namespace ge::shaders
