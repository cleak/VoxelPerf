//uniform mat4 mvp;

//attribute vec3 vColor;
//attribute vec3 vPos;

varying vec3 color;

void main() {
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    color = gl_Color.rgb;
}
