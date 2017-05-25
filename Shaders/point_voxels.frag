#version 410
flat in lowp vec3 fColor;
//varying int fEnabledFaces;

void main() {
    gl_FragColor = vec4(fColor, 1.0);// + vec4(enabledFaces / 1024.0);
}