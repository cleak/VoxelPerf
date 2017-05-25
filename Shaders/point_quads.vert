#version 410

uniform mat4 mvp;

attribute vec4 vColor;
attribute vec3 vPos;

//attribute vec4 vExtent1;
//attribute vec4 vExtent2;
attribute int vFaceIdx;

//attribute vec3 vNormal;

flat out lowp vec3 gColor;

//varying vec4 gExtent1;
//varying vec4 gExtent2;

flat out int gFaceIdx;

uniform float voxSize = 0.25;

void main() {
    gl_Position = mvp * vec4(vPos, 1.0);

    //gExtent1 = mvp * vExtent1 / 2.0 * voxSize;
    //gExtent2 = mvp * vExtent2 / 2.0 * voxSize;
    gFaceIdx = vFaceIdx;
    
    gColor = vColor.rgb;
}
