#version 410

in vec3 fEye;
in vec3 fPos;

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

vec4 march(vec3 startPoint, vec3 dir) {
	//vec3 tPerUnit = 1.0 / dir;
	vec3 tPerUnit = voxelSize / abs(dir);
	//tPerUnit = 

	vec3 t0 = startPoint / voxelSize / abs(dir);
	//vec3 t0 = startPoint / dir;
	//t0 = step(-1, t0) * t0;

	float t = 0;
	float lastT = 0;


	float endT = mincomp(max((vec3(voxelGridSize * voxelSize) - startPoint) / dir, -startPoint / dir));

	while (t <= endT) {

		vec3 p = startPoint + dir * (t + lastT) / 2.0;
		/*if (textureLod(voxelShape, p / (voxelSize * voxelGridSize), 0).r > 0) {
			return textureLod(voxelColor, p / (voxelSize * voxelGridSize), 0);
		}*/

		if (textureLod(voxelShape, p / (voxelGridSize * voxelSize), 0).r > 0) {
			return textureLod(voxelColor, p / (voxelGridSize * voxelSize), 0);
		}

		//ivec3 lastThresh = ivec3((vec3(t) - t0) / tPerUnit);
		vec3 lastThresh = (vec3(t) - t0) / tPerUnit;
		lastThresh -= fract(lastThresh);
		vec3 tUnitIntersect = (lastThresh + vec3(1)) * tPerUnit + t0;

		lastT = t;
		t = max(mincomp(tUnitIntersect), t + 0.00001);

		//t += voxelSize - mincomp(abs(mod(p, voxelSize)));
	}

	return vec4(0);
}

const float eps = 0.0005;
//const float eps = 0.00000025;

vec4 march2(dvec3 startPoint, dvec3 dir) {
	double endT = mincomp(max((dvec3(voxelGridSize * voxelSize) - startPoint) / dir, -startPoint / dir)) + eps;
	dvec3 tPerVoxel = voxelSize / abs(dir);
	dvec3 t0 = startPoint / abs(dir);

	ivec3 voxelCoords = ivec3(0);
	double t = 0;
	double lastT = 0;

	while (t <= endT) {

		/*dvec3 p = startPoint + dir * t;
		if (textureLod(voxelShape, vec3(p / (voxelGridSize * voxelSize)), 0).r > 0) {
			return textureLod(voxelColor, vec3(p / (voxelGridSize * voxelSize)), 0);
		}

		lastT = t;*/

		int minComp = -1;
		//float minT = 1 / 0;
		//float minT = 1000000.0;

		voxelCoords = ivec3((t - t0) / tPerVoxel + eps);
		//voxelCoords -= fract(voxelCoords) ;
		dvec3 tVec = t0 + tPerVoxel * (voxelCoords + ivec3(1));
		double minT = mincomp(tVec);

		/*if (tVec[0] < minT) {
			minT = tVec[0];
			minComp = 0;
		}

		if (tVec[1] < minT) {
			minT = tVec[1];
			minComp = 1;
		}

		if (tVec[2] < minT) {
			minT = tVec[2];
			minComp = 2;
		}

		if (minT < t) {
			return vec4(1);
			//t += eps;
		} else {
			t = minT;
		}

		voxelCoords[minComp] += 1;*/

		if (minT < t) {
			return vec4(1);
			//t += eps;
		} else {
			t = minT;
		}

		//vec3 p = startPoint + dir * (t + lastT) / 2.0;
		dvec3 p = startPoint + dir * t;
		if (textureLod(voxelShape, vec3(p / (voxelGridSize * voxelSize)), 0).r > 0) {
			return textureLod(voxelColor, vec3(p / (voxelGridSize * voxelSize)), 0);
		}

		lastT = t;
	}

	return vec4(0);
}

vec4 march4(vec3 startPoint, vec3 dir) {
	vec3 p0 = startPoint / voxelSize;
	vec3 d = dir / voxelSize;
	float endT = mincomp(max((vec3(voxelGridSize) - p0) / dir, -p0 / dir));

	float t = 0;
	while (t <= endT) {
		vec3 p = p0 + dir * t;
		//vec4 c = textureLod(voxels, p / voxelGridSize, 0);
		if (textureLod(voxelShape, vec3(p / (voxelGridSize)), 0).r > 0) {
		//if (c.a > 0) {
			return textureLod(voxelColor, vec3(p / (voxelGridSize)), 0);
			//return c;
		}
		//vec3 deltas = step(0, dir) - fract(p) * sign(dir);
		//deltas = (deltas + step(0, -deltas)) / abs(dir);

		//vec3 deltas = step(0, dir) - fract(p) * sign(dir);
		// /deltas = (deltas + step(0, -deltas)) / abs(dir);
		//deltas = deltas / abs(dir);
		vec3 deltas = (step(0, dir) - fract(p)) / dir;

		t += max(mincomp(deltas), eps);
		//t += mincomp(deltas);
	}

	return vec4(0);
}

void main() {
	vec3 eyeNormal = normalize(fEye);

	//vec4 color = march4(dvec3(fPos), dvec3(eyeNormal));
	vec4 color = march4(fPos, eyeNormal);
	if (color.a == 0) {
		discard;
	}

	gl_FragColor = color;
}
