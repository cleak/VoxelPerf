#version 410

in vec3 fEye;
in vec3 fPos;

uniform sampler3D voxels;
uniform float voxelSize = 0.25;
uniform float voxelGridSize = 32;

float maxcomp(in vec3 p ) { return max(p.x,max(p.y,p.z));}
float maxcomp(in vec2 p ) { return max(p.x,p.y);}

float mincomp(in vec3 p ) { return min(p.x,min(p.y,p.z));}

vec4 march(vec3 startPoint, vec3 dir) {
	float t = 0;

	for (int i = 0; i < 64; ++i) {
		vec3 p = startPoint + dir * t;
		vec4 c = textureLod(voxels, p / (voxelSize * voxelGridSize), 0);
		if (c.a > 0) {
			return c;
		}
		t += voxelSize - mincomp(abs(mod(p, voxelSize)));
	}

	return vec4(0);
}

void main() {
	vec3 eyeNormal = normalize(fEye);

	vec4 color = march(fPos, eyeNormal);
	if (color.a == 0) {
		discard;
	}

	gl_FragColor = color;
}
