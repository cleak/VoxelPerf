uniform mat4 mvp;

attribute vec3 vColor;
attribute vec3 vPos;

varying vec3 color;

void main() {
    gl_Position = mvp * vec4(vPos, 1.0);
    color = vColor;
}
