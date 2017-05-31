#version 410

uniform mat4 mvInv;
uniform mat4 mv;
uniform mat4 mvp;

uniform vec3 offset;
uniform vec2 nearPlaneDim;
uniform float nearPlaneDist;

in vec3 vPos;
out vec3 fPos;
out vec3 fEye;

void main() {
	// Screen space position
    gl_Position = mvp * vec4(vPos + offset, 1.0);

    // Voxel space position
    fPos = vPos;

    // World space position
    vec3 pos = vPos + offset;
    
    fEye = pos - (mvInv * vec4(vec3(0), 1)).xyz;
}
