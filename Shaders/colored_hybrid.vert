#version 410

uniform mat4 mvp;
uniform vec3 offset;
uniform float voxelSize = 0.25;

attribute lowp vec4 vColor;
attribute lowp vec3 vPos;

varying lowp vec3 color;

void main() {
    gl_Position = mvp * vec4(vPos * voxelSize + offset, 1.0);
    color = vColor.rgb;
}
