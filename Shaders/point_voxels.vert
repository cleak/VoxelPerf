uniform mat4 mvp;

attribute vec3 vColor;
attribute vec3 vPos;
attribute int vEnabledFaces;

varying vec3 gColor;
varying int gEnabledFaces;

void main() {
    gl_Position = mvp * vec4(vPos, 1.0);
    
    gColor = vColor;
    gEnabledFaces = vEnabledFaces;
}
