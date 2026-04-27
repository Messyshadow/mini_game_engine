#version 330 core
layout (location = 0) in vec2 aPos;

uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uParticlePos;
uniform float uSize;
uniform vec4 uColor;

out vec4 vColor;
out vec2 vUV;

void main() {
    vec3 camRight = vec3(uView[0][0], uView[1][0], uView[2][0]);
    vec3 camUp    = vec3(uView[0][1], uView[1][1], uView[2][1]);
    vec3 worldPos = uParticlePos + camRight * aPos.x * uSize + camUp * aPos.y * uSize;
    gl_Position = uProjection * uView * vec4(worldPos, 1.0);
    vColor = uColor;
    vUV = aPos + 0.5;
}
