#version 410

uniform mat4 mvp;

attribute lowp vec3 vColor;
attribute lowp vec3 vPos;

varying lowp vec3 color;

void main() {
    gl_Position = mvp * vec4(vPos, 1.0);
    color = vColor;
}
