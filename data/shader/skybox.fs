#version 330 core
in vec3 vTexCoord;
out vec4 FragColor;

uniform vec3 uTopColor;
uniform vec3 uBottomColor;
uniform vec3 uHorizonColor;

void main() {
    vec3 dir = normalize(vTexCoord);
    float t = dir.y * 0.5 + 0.5;
    vec3 color = mix(uBottomColor, uTopColor, t);
    float horizonFactor = 1.0 - abs(dir.y);
    horizonFactor = pow(horizonFactor, 4.0);
    color = mix(color, uHorizonColor, horizonFactor * 0.6);
    FragColor = vec4(color, 1.0);
}
