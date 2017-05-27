#version 410

in vec3 vColor;
in vec3 vPos;
in int vFaceIdx;

flat out lowp vec3 gColor;
flat out int gFaceIdx;

uniform mat4 mvp;
uniform float voxSize = 0.25;
uniform vec3 vOffset = vec3(0.0, 0.0, 0.0);

void main() {
    gl_Position = mvp * vec4(vPos * voxSize + vOffset, 1.0);
    gFaceIdx = vFaceIdx;
    gColor = vColor;
}
