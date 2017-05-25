uniform mat4 mvp;

// Per vertex attributes
attribute vec2 vWeights;

// Instanced attributes
attribute vec4 vColor;
attribute vec3 vCenter;
attribute vec4 vNormal;

varying vec3 color;

void main() {
    /*vec3 d1 = -vec3(vNormal.z, vNormal.x, vNormal.y);
    vec3 d2 = -vec3(vNormal.y, vNormal.z, vNormal.x);*/

    //vec3 d1 = vNormal.w * vec3(vNormal.z, vNormal.x, vNormal.y);
    //vec3 d2 = vNormal.w * vec3(vNormal.y, vNormal.z, vNormal.x);

    vec3 d1 = vNormal.w * vec3(vNormal.z, vNormal.x, vNormal.y);
    vec3 d2 = -vec3(vNormal.y, vNormal.z, vNormal.x);

    vec3 pos = vCenter + d1 * vWeights.x + d2 * vWeights.y;

    gl_Position = mvp * vec4(pos, 1.0);
    color = vColor.rgb;
}
