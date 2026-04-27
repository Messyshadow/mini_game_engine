#version 330 core
layout (location = 0) in vec3 aPos;
out vec3 vTexCoord;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    vTexCoord = aPos;
    mat4 viewNoTranslation = mat4(mat3(uView));
    vec4 pos = uProjection * viewNoTranslation * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
