#version 410

in vec3 fEye;
in vec3 fPos;

uniform sampler3D voxels;

uniform float voxelSize = 0.25;
uniform float voxelGridSize = 32;

float maxcomp(in vec3 p ) { return max(p.x,max(p.y,p.z));}
int maxcomp(in ivec3 p ) { return max(p.x,max(p.y,p.z));}

float maxcomp(in vec2 p ) { return max(p.x,p.y);}

float mincomp(in vec3 p ) { return min(p.x,min(p.y,p.z));}
int mincomp(in ivec3 p ) { return min(p.x,min(p.y,p.z));}

vec3 lowest(vec3 v) {
	return vec3(
		v.x <= v.y && v.x <= v.z,
		v.y < v.x && v.y <= v.z,
		v.z < v.x && v.z < v.y
	);
}

int lowestIdx(vec3 v) {
	if (v.x <= v.y && v.x <= v.z)
		return 0;
	
	if (v.y < v.x && v.y <= v.z)
		return 1;
	
	return 2;
}

const float eps = 0.0005;

vec4 march(vec3 startPoint, vec3 dir) {
	vec3 p0 = startPoint / voxelSize;
	vec3 d = dir / voxelSize;
	float endT = mincomp(max((vec3(voxelGridSize) - p0) / dir, -p0 / dir));

	float t = 0;
	while (t <= endT) {
		vec3 p = p0 + dir * t;
		vec4 c = textureLod(voxels, p / voxelGridSize, 0);
		if (c.a > 0) {
			return c;
		}

		// TODO: For negative directions, we're probably taking twice as many steps as necessary
		// since it'll go n => n + eps => n + 1 rather than n => n + 1
		vec3 deltas = (step(0, dir) - fract(p)) / dir;
		t += max(mincomp(deltas), eps);
	}

	return vec4(0);
}

void main() {
	// TODO: Doesn't need to be normalized?
	vec3 eyeNormal = normalize(fEye);

	vec4 color = march(fPos, eyeNormal);
	if (color.a == 0) {
		discard;
	}

	gl_FragColor = color;
}