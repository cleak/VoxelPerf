#version 410

in vec3 fEye;
in vec3 fPos;

uniform sampler3D voxels;
uniform float voxelSize = 0.25;
uniform float voxelGridSize = 32;

float maxcomp(in vec3 p ) { return max(p.x,max(p.y,p.z));}
float maxcomp(in vec2 p ) { return max(p.x,p.y);}

float mincomp(in vec3 p ) { return min(p.x,min(p.y,p.z));}

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

vec4 march(vec3 startPoint, vec3 dir) {
	float t = 0;
	float lastT = 0;
	vec3 tStep = abs(voxelSize / dir);

	vec3 tVec = -mod(abs(startPoint) / abs(dir), tStep);
	float endT = mincomp(max((vec3(voxelGridSize * voxelSize) - startPoint) / dir, -startPoint / dir));

	while (t <= endT) {
		int idx = lowestIdx(tVec + tStep);
		tVec[idx] += tStep[idx];
		t = tVec[idx];

		// Check the midpoint between the last point and the current one
		vec3 p = startPoint + (t + lastT) / 2.0 * dir;
		vec4 c = textureLod(voxels, p / (voxelSize * voxelGridSize), 0);
		if (c.a > 0) {
			return c;
		}

		// TODO: Could potentially poke through corners without colliding with sides
		lastT = t;
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
