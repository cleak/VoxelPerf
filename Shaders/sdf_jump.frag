#version 410

in vec3 fEye;
in vec3 fPos;

//uniform sampler3D voxelColor;
//uniform usampler3D voxelShape;


uniform sampler3D voxelColor;
uniform usampler3D voxelShape;

uniform float voxelSize = 0.25;
uniform float voxelGridSize = 32;

float maxcomp(in vec3 p ) { return max(p.x,max(p.y,p.z));}
double maxcomp(in dvec3 p ) { return max(p.x,max(p.y,p.z));}
float maxcomp(in vec2 p ) { return max(p.x,p.y);}
double maxcomp(in dvec2 p ) { return max(p.x,p.y);}

float mincomp(in vec3 p ) { return min(p.x,min(p.y,p.z));}
double mincomp(in dvec3 p ) { return min(p.x,min(p.y,p.z));}

const float eps = 0.0005;

vec4 march(vec3 startPoint, vec3 dir) {
	// Start point in voxel space
	vec3 p0 = startPoint / voxelSize;
	float endT = mincomp(max((vec3(voxelGridSize) - p0) / dir, -p0 / dir));

	vec3 p0abs = (1 - step(0, dir)) * voxelGridSize + sign(dir) * p0;
	vec3 dirAbs = abs(dir);

	float t = 0;
	while (t <= endT) {
		// Next point to check
		vec3 p = p0 + dir * t;
		uint jump = textureLod(voxelShape, vec3(p / (voxelGridSize)), 0).r;

		// Stop if voxel is solid
		if (jump == 255) {
			return textureLod(voxelColor, vec3(p / (voxelGridSize)), 0);
		}

		vec3 pAbs = p0abs + dirAbs * t;
		vec3 deltas = (1 - fract(p)) / dirAbs;
		t += max(mincomp(deltas), eps) + jump;
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
