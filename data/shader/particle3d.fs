#version 330 core
in vec4 vColor;
in vec2 vUV;
out vec4 FragColor;

void main() {
    float dist = length(vUV - vec2(0.5));
    float alpha = 1.0 - smoothstep(0.3, 0.5, dist);
    FragColor = vec4(vColor.rgb, vColor.a * alpha);
}
