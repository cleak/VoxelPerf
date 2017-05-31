#include "SignedDistanceFields.h"

#include <array>
#include <iostream>

#include <cstddef>
#include <vector>

using namespace glm;
using namespace std;

#pragma pack(push, 1)
struct SdfVertex {
	SdfVertex(vec3 p) : position(p)
	{}

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

void MakeSdfQuad(VoxelSet& model, ivec3 dimensions, vec3 spacing, GLuint& vao,
	GLuint& vbo, GLuint program) {

	size_t modelCount = dimensions.x * dimensions.y * dimensions.z;


	glGenBuffers(1, &vbo);
	CheckGLErrors();

	glGenVertexArrays(1, &vao);
	CheckGLErrors();

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	
	array<SdfVertex, 4> vertices = { {
		{ vec3(-1, -1, 0) },
		{ vec3( 1, -1, 0) },
		{ vec3( 1,  1, 0) },
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

static GLuint VoxelsToTexture(VoxelSet & voxels) {
    int totalSize = voxels.size.x * voxels.size.y * voxels.size.z * 4;
    vector<uint8_t> colors(totalSize);

    for (int z = 0; z < voxels.size.z; ++z) {
        for (int y = 0; y < voxels.size.y; ++y) {
            for (int x = 0; x < voxels.size.x; ++x) {
                ivec3 idx(x, y, z);
                int colorIdx = z * voxels.size.x * voxels.size.y
                    + y * voxels.size.x
                    + x;
                colorIdx *= 4;

                vec4 c = voxels.At(idx);

                colors[colorIdx + 0] = RoundByteF(c.r);
                colors[colorIdx + 1] = RoundByteF(c.g);
                colors[colorIdx + 2] = RoundByteF(c.b);

                if (voxels.IsSolid(idx)) {
                    colors[colorIdx + 3] = 255;
                } else {
                    colors[colorIdx + 3] = 0;
                }
            }
        }
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_3D, tex);

    glTexImage3D(GL_TEXTURE_3D,
        0,
        GL_RGBA8,
        voxels.size.x,
        voxels.size.y,
        voxels.size.z,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        &colors[0]);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    CheckGLErrors();

    return tex;
}

PerfRecord RunSdfTest(VoxelSet & model, glm::ivec3 gridSize, glm::vec3 voxelSpacing) {
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

    vector<GLuint> textures;
    vector<vec3> offsets;

	PerfRecord record = RunPerf(
		[&]() {
		// Setup
		program = MakeShaderProgram({
			{ "Shaders/sdf.vert", GL_VERTEX_SHADER },
			{ "Shaders/sdf.frag", GL_FRAGMENT_SHADER },
		});
        mvpLoc = glGetUniformLocation(program, "mvp");
        mvInvLoc = glGetUniformLocation(program, "mvInv");
        mvLoc = glGetUniformLocation(program, "mv");
        nearDimLoc = glGetUniformLocation(program, "nearPlaneDim");
        nearDistLoc = glGetUniformLocation(program, "nearPlaneDist");
        offsetLoc = glGetUniformLocation(program, "offset");
		MakeSdfQuad(model, gridSize, voxelSpacing, vao, vbo, program);
        cubeVao = BufferCube(32.0f * VOXEL_SIZE, program);

        int nextOffsetIdx = 0;
        int totalModels = gridSize.x * gridSize.y * gridSize.z;
        offsets.resize(totalModels);
        textures.resize(totalModels);
        for (int z = 0; z < gridSize.z; ++z) {
            for (int y = 0; y < gridSize.y; ++y) {
                for (int x = 0; x < gridSize.x; ++x) {
                    ivec3 idx(x, y, z);
                    vec3 offset = vec3(idx) * voxelSpacing;
                    offset -= vec3(0, gridSize.y, 0) * voxelSpacing / 2.0f;
                    offsets[nextOffsetIdx] = offset;
                    textures[nextOffsetIdx] = VoxelsToTexture(model);
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

        for (int i = 0; i < offsets.size(); ++i) {
            vec3 offset = offsets[i];
            glUniform3fv(offsetLoc, 1, (const GLfloat*)&offset);
            glBindVertexArray(cubeVao);
            glBindTexture(GL_TEXTURE_3D, textures[i]);
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

        glDeleteTextures(textures.size(), &textures[0]);
        CheckGLErrors();
    });

	return record;
}
