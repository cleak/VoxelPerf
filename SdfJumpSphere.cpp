#include "SdfJumpSphere.h"

#include <array>
#include <iostream>
#include <fstream>

#include <cstddef>
#include <vector>

using namespace glm;
using namespace std;

#pragma pack(push, 1)
static struct SdfVertex {
	SdfVertex(vec3 p) : position(p) {}

	vec3 position;
};
#pragma pack(pop)

// Buffers a single face of a voxel in the given vector
static void BufferCubeFace(vec3 center, vec3 normal, float cubeWidth, vector<SdfVertex>& vertices) {
	// Center of voxel to center of face
	center += vec3(normal) * cubeWidth / 2.0f;

	vec3 d1 = -vec3(normal.z, normal.x, normal.y);
	vec3 d2 = -vec3(normal.y, normal.z, normal.x);

	vector<vec2> weights = {
		{ -1, -1 },
		{ 1, -1 },
		{ 1,  1 },
		{ -1,  1 },
	};

	// Reverse winding order for negative normals
	if (normal.x < 0 || normal.y < 0 || normal.z < 0) {
		std::swap(d1, d2);
	}

	for (int i = 0; i < 4; ++i) {
		vec2 w = weights[i];

		SdfVertex v(center + (d1 * w.x + d2 * w.y) / 2.0f * cubeWidth);
		vertices.push_back(v);
	}
}

// Buffers all faces of a voxel in the given vector
static GLuint BufferCube(float cubeWidth, GLuint program) {
	vec3 center = vec3(cubeWidth, cubeWidth, cubeWidth) / 2.0f;

	std::vector<SdfVertex> vertices;

	vector<ivec3> normals = {
		{ 1, 0, 0 },
		{ -1, 0, 0 },
		{ 0, 1, 0 },
		{ 0,-1, 0 },
		{ 0, 0, 1 },
		{ 0, 0,-1 },
	};

	for (auto& n : normals) {
		BufferCubeFace(center, vec3(n), cubeWidth, vertices);
	}

	GLuint vao;
	GLuint vbo;
	glGenBuffers(1, &vbo);
	CheckGLErrors();

	glGenVertexArrays(1, &vao);
	CheckGLErrors();

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(SdfVertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

	GLint vPosLoc = glGetAttribLocation(program, "vPos");
	glEnableVertexAttribArray(vPosLoc);
	glVertexAttribPointer(vPosLoc, 3, GL_FLOAT, GL_FALSE,
						  sizeof(SdfVertex),
						  (void*)offsetof(SdfVertex, position));
	CheckGLErrors();

	return vao;
}

static void MakeSdfQuad(VoxelSet& model, ivec3 dimensions, vec3 spacing, GLuint& vao,
						GLuint& vbo, GLuint program) {

	size_t modelCount = dimensions.x * dimensions.y * dimensions.z;

	CheckGLErrors();

	glGenBuffers(1, &vbo);
	CheckGLErrors();

	glGenVertexArrays(1, &vao);
	CheckGLErrors();

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	array<SdfVertex, 4> vertices = { {
		{ vec3(-1, -1, 0) },
		{ vec3(1, -1, 0) },
		{ vec3(1,  1, 0) },
		{ vec3(-1,  1, 0) },
		} };

	glBufferData(GL_ARRAY_BUFFER, sizeof(SdfVertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

	GLint vPosLoc = glGetAttribLocation(program, "vPos");
	glEnableVertexAttribArray(vPosLoc);
	glVertexAttribPointer(vPosLoc, 3, GL_FLOAT, GL_FALSE,
						  sizeof(SdfVertex),
						  (void*)offsetof(SdfVertex, position));
	CheckGLErrors();
}

static uint8_t RoundByteF(float f) {
	return (uint8_t)round(f * 255.0f);
}

struct Box {
	Box(ivec3 idx) {
		minPoint = vec3(idx);
		maxPoint = vec3(idx) + vec3(1);
	}

	vec3 minPoint;
	vec3 maxPoint;
};

float BoxDistance2(Box a, Box b) {
	//vec3 d = glm::max(a.maxPoint - b.minPoint, b.maxPoint - a.minPoint);
	vec3 d = glm::max(a.minPoint - b.maxPoint, b.minPoint - a.maxPoint);
	d = max(d, vec3(0));
	return dot(d, d);
}

static bool IsEmpty(VoxelSet& voxels, ivec3 idx, float distance) {

	ivec3 startIdx = idx - ivec3(ceil(distance) + 1);
	ivec3 endIdx = idx + ivec3(ceil(distance) + 1);

	Box center(idx);

	for (int z = startIdx.z; z <= endIdx.z; ++z) {
		for (int y = startIdx.y; y <= endIdx.y; ++y) {
			for (int x = startIdx.x; x <= endIdx.x; ++x) {
				ivec3 checkIdx(x, y, z);

				Box checkBox(checkIdx);

				if (BoxDistance2(center, checkBox) > distance * distance) {
					continue;
				}

				if (voxels.IsSolid(checkIdx)) {
					return false;
				}
			}
		}
	}

	return true;
}

static uint8_t FindMaxJump(VoxelSet& voxels, ivec3 idx) {
	uint8_t jumpSize = 0;

	//while (IsEmpty(voxels, idx - ivec3(jumpSize + 1), idx + ivec3(jumpSize + 1)) && jumpSize < 32) {
	while (IsEmpty(voxels, idx, jumpSize + 0.005f)) {
		jumpSize++;
	}

	return jumpSize;
}

static void MakeJumpTexture(VoxelSet& voxels, vector<uint8_t>& jumpTexture) {
	int jumpIdx = 0;
	int shapeSize = voxels.size.x * voxels.size.y * voxels.size.z;
	jumpTexture.resize(shapeSize);

	if (IsOptionSet("loadShape")) {
		ifstream fin(GetOption("loadShape").c_str());

		for (int i = 0; i < shapeSize; ++i) {
			int val;
			fin >> val;
			jumpTexture[i] = val;
		}

		fin.close();
		return;
	}

	for (int z = 0; z < voxels.size.z; ++z) {
		for (int y = 0; y < voxels.size.y; ++y) {
			for (int x = 0; x < voxels.size.x; ++x) {
				ivec3 idx(x, y, z);
				int jumpIdx = z * voxels.size.x * voxels.size.y
					+ y * voxels.size.x
					+ x;

				if (voxels.IsSolid(idx)) {
					jumpTexture[jumpIdx] = 255;
				} else {
					jumpTexture[jumpIdx] = FindMaxJump(voxels, idx);
				}
			}
		}
	}

	if (IsOptionSet("saveShape")) {
		ofstream fout(GetOption("saveShape").c_str());

		for (int i = 0; i < shapeSize; ++i) {
			if (i > 0) {
				fout << " ";
			}
			fout << (int)jumpTexture[i];
		}

		fout << endl;

		fout.close();
	}
}

static void VoxelsToTexture(VoxelSet & voxels, GLuint& colorTexture, GLuint& shapeTexture, GLuint& colorSampler, GLuint& shapeSampler, vector<uint8_t>& shape) {
	int colorsSize = voxels.size.x * voxels.size.y * voxels.size.z * 4;
	int shapeSize = voxels.size.x * voxels.size.y * voxels.size.z;

	vector<uint8_t> colors(colorsSize);

	for (int z = 0; z < voxels.size.z; ++z) {
		for (int y = 0; y < voxels.size.y; ++y) {
			for (int x = 0; x < voxels.size.x; ++x) {
				ivec3 idx(x, y, z);
				int shapeIdx = z * voxels.size.x * voxels.size.y
					+ y * voxels.size.x
					+ x;
				int colorIdx = shapeIdx * 3;

				vec4 c = voxels.At(idx);

				colors[colorIdx + 0] = RoundByteF(c.r);
				colors[colorIdx + 1] = RoundByteF(c.g);
				colors[colorIdx + 2] = RoundByteF(c.b);
			}
		}
	}

	// Color texture
	glGenTextures(1, &colorTexture);
	glBindTexture(GL_TEXTURE_3D, colorTexture);

	glTexImage3D(GL_TEXTURE_3D,
				 0,
				 GL_RGB8,
				 voxels.size.x,
				 voxels.size.y,
				 voxels.size.z,
				 0,
				 GL_RGB,
				 GL_UNSIGNED_BYTE,
				 &colors[0]);

	glGenSamplers(1, &colorSampler);
	glSamplerParameteri(colorSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(colorSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(colorSampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(colorSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glSamplerParameteri(colorSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	CheckGLErrors();

	// Shape texture
	glGenTextures(1, &shapeTexture);
	glBindTexture(GL_TEXTURE_3D, shapeTexture);
	CheckGLErrors();

	glTexImage3D(GL_TEXTURE_3D,
				 0,
				 GL_R8UI,
				 voxels.size.x,
				 voxels.size.y,
				 voxels.size.z,
				 0,
				 GL_RED_INTEGER,
				 GL_UNSIGNED_BYTE,
				 &shape[0]);
	CheckGLErrors();

	glGenSamplers(1, &shapeSampler);
	glSamplerParameteri(shapeSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(shapeSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(shapeSampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(shapeSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glSamplerParameteri(shapeSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	CheckGLErrors();
}

struct VoxObjInfo {
	vec3 offset;
	GLuint colorTexture;
	GLuint shapeTexture;

	GLuint colorSampler;
	GLuint shapeSampler;
};

PerfRecord RunSdfJumpSphereTest(VoxelSet & model, glm::ivec3 gridSize, glm::vec3 voxelSpacing) {
	GLuint program;

	GLint mvpLoc;
	GLint mvInvLoc;
	GLint mvLoc;
	GLint nearDistLoc;
	GLint nearDimLoc;
	GLint offsetLoc;

	GLuint vao;
	GLuint vbo;

	GLuint cubeVao;

	GLuint colorTexLoc;
	GLuint shapeTexLoc;

	vector<VoxObjInfo> voxObjs;

	PerfRecord record = RunPerf(
		[&]() {
		// Setup
		vector<uint8_t> shape;
		MakeJumpTexture(model, shape);

		program = MakeShaderProgram({
			{ "Shaders/sdf.vert", GL_VERTEX_SHADER },
			{ "Shaders/sdf_jump.frag", GL_FRAGMENT_SHADER },
		});
		CheckGLErrors();

		mvpLoc = glGetUniformLocation(program, "mvp");
		CheckGLErrors();
		mvInvLoc = glGetUniformLocation(program, "mvInv");
		CheckGLErrors();
		mvLoc = glGetUniformLocation(program, "mv");
		CheckGLErrors();

		// Assign texture slots
		colorTexLoc = glGetUniformLocation(program, "voxelColor");
		CheckGLErrors();

		shapeTexLoc = glGetUniformLocation(program, "voxelShape");
		CheckGLErrors();

		nearDimLoc = glGetUniformLocation(program, "nearPlaneDim");
		nearDistLoc = glGetUniformLocation(program, "nearPlaneDist");
		offsetLoc = glGetUniformLocation(program, "offset");
		CheckGLErrors();
		MakeSdfQuad(model, gridSize, voxelSpacing, vao, vbo, program);
		cubeVao = BufferCube(32.0f * VOXEL_SIZE, program);

		int nextOffsetIdx = 0;
		int totalModels = gridSize.x * gridSize.y * gridSize.z;
		voxObjs.resize(totalModels);
		for (int z = 0; z < gridSize.z; ++z) {
			for (int y = 0; y < gridSize.y; ++y) {
				for (int x = 0; x < gridSize.x; ++x) {
					ivec3 idx(x, y, z);
					voxObjs[nextOffsetIdx].offset = vec3(idx) * voxelSpacing;
					voxObjs[nextOffsetIdx].offset -= vec3(0, gridSize.y, 0) * voxelSpacing / 2.0f;
					//offsets[nextOffsetIdx] = offset;
					VoxelsToTexture(model, voxObjs[nextOffsetIdx].colorTexture, voxObjs[nextOffsetIdx].shapeTexture,
									voxObjs[nextOffsetIdx].colorSampler, voxObjs[nextOffsetIdx].shapeSampler, shape);
					nextOffsetIdx++;
				}
			}
		}
	},
		[&]() {
		// Draw
		mat4 mvp = MakeMvp();
		mat4 mv = MakeModelView();
		mat4 mvInv = glm::inverse(mv);

		vec3 nearPlane = GetNearPlane();

		glUseProgram(program);
		glUniform2fv(nearDimLoc, 1, &nearPlane.x);
		glUniform1fv(nearDistLoc, 1, &nearPlane.z);
		glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, (const GLfloat*)&mvp);

		glUniformMatrix4fv(mvInvLoc, 1, GL_FALSE, (const GLfloat*)&mvInv);
		glUniformMatrix4fv(mvLoc, 1, GL_FALSE, (const GLfloat*)&mv);

		glUniform1i(colorTexLoc, 0);
		glUniform1i(shapeTexLoc, 1);

		for (int i = 0; i < voxObjs.size(); ++i) {
			vec3 offset = voxObjs[i].offset;
			glUniform3fv(offsetLoc, 1, (const GLfloat*)&offset);
			glBindVertexArray(cubeVao);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_3D, voxObjs[i].colorTexture);
			glBindSampler(0, voxObjs[i].colorSampler);

			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_3D, voxObjs[i].shapeTexture);
			glBindSampler(1, voxObjs[i].shapeSampler);

			glDrawArrays(GL_QUADS, 0, 24);
		}
	},
		[&]() {
		// Teardown
		glDeleteBuffers(1, &vbo);
		CheckGLErrors();

		glDeleteVertexArrays(1, &vao);
		CheckGLErrors();

		glDeleteProgram(program);
		CheckGLErrors();

		//glDeleteTextures(textures.size(), &textures[0]);
		CheckGLErrors();
	});

	return record;
}
