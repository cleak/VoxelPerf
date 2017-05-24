uniform mat4 mvp;

attribute vec3 vColor;
attribute vec3 vPos;

attribute vec4 vExtent1;
attribute vec4 vExtent2;
attribute vec3 vNormal;

varying vec3 gColor;

varying vec4 gExtent1;
varying vec4 gExtent2;

uniform float voxSize = 0.25;

void main() {
    gl_Position = mvp * vec4(vPos, 1.0);

    gExtent1 = mvp * vExtent1 / 2.0 * voxSize;
    gExtent2 = mvp * vExtent2 / 2.0 * voxSize;
    
    gColor = vColor;
}
