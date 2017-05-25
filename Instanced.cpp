#include "Instanced.h"

#include <vector>

using namespace glm;
using namespace std;

#pragma pack(push, 1)
struct InstanceInfo {
    vec3 position;
    PackedColor color;
    PackedVec normal;
};

struct QuadBaseVertex {
    QuadBaseVertex(float x, float y) {
        weights.x = x;
        weights.y = y;
    }

    vec2 weights;
};
#pragma pack(pop)

// Buffers a single face of a voxel in the given vector
void BufferFaceInstanced(vec3 center, vec3 normal, vec3 color, vector<InstanceInfo>& vertices, int& nextIdx) {
    
    InstanceInfo instance;

    // Center of voxel to center of face
    instance.position = center + vec3(normal) * VOXEL_SIZE / 2.0f;

    float extentsCo = -1.0f;

    if (normal.x < 0 || normal.y < 0 || normal.z < 0) {
        extentsCo = 1.0f;
    }

    instance.normal = PackVec4(vec4(normal, extentsCo));
    instance.color = PackColor(color);

    if (nextIdx >= vertices.size()) {
        vertices.push_back(instance);
    } else {
        vertices[nextIdx] = instance;
    }
    nextIdx++;
}

// Buffers all faces of a voxel in the given vector
void BufferVoxelInstanced(VoxelSet& voxels, vec3 offset, ivec3 idx, vector<InstanceInfo>& vertices, int& nextIdx) {
    if (!voxels.IsSolid(idx)) {
        return;
    }

    vec3 color = voxels.At(idx);
    vec3 center = offset + vec3(idx) * VOXEL_SIZE + vec3(VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE) / 2.0f;

    vector<ivec3> normals = {
        { 1, 0, 0 },
        { -1, 0, 0 },
        { 0, 1, 0 },
        { 0,-1, 0 },
        { 0, 0, 1 },
        { 0, 0,-1 },
    };

    for (auto& n : normals) {
        if (voxels.IsSolid(idx + n)) {
            continue;
        }
        BufferFaceInstanced(center, vec3(n), color, vertices, nextIdx);
    }
}

// Buffers an entire voxel model in the given vector
void BufferVoxelSetInstanced(VoxelSet& voxels, vec3 offset, vector<InstanceInfo>& vertices) {
    int nextIdx = 0;

    for (int z = 0; z < voxels.size.z; ++z) {
        for (int y = 0; y < voxels.size.y; ++y) {
            for (int x = 0; x < voxels.size.x; ++x) {
                BufferVoxelInstanced(voxels, offset, ivec3(x, y, z), vertices, nextIdx);
            }
        }
    }
}

static GLuint MakeQuadVbo() {
    vector<QuadBaseVertex> weights = {
        {-1, -1 },
        { 1, -1 },
        { 1,  1 },
        {-1,  1 },
    };

    for (int i = 0; i < 4; ++i) {
        // Pre-multiply voxel size with weights
        weights[i].weights *= VOXEL_SIZE / 2.0f;
    }

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QuadBaseVertex) * weights.size(), &weights[0], GL_STATIC_DRAW);
    CheckGLErrors();

    return vbo;
}

static void EnableQuadAttributes(GLuint program, GLuint vbo) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    GLint vWeights = glGetAttribLocation(program, "vWeights");
    glEnableVertexAttribArray(vWeights);
    glVertexAttribPointer(vWeights, 2, GL_FLOAT, GL_FALSE,
                          sizeof(QuadBaseVertex), (void*)0);
    CheckGLErrors();
}

size_t MakeInstancedGrid(VoxelSet& model, ivec3 dimensions, vec3 spacing, std::vector<GLuint>& vaos,
                   std::vector<GLuint>& vbos, GLuint program) {
    vbos.resize(dimensions.x * dimensions.y * dimensions.z);
    vaos.resize(dimensions.x * dimensions.y * dimensions.z);

    glGenBuffers(vbos.size(), &vbos[0]);
    CheckGLErrors();

    glGenVertexArrays(vaos.size(), &vaos[0]);
    CheckGLErrors();

    int nextVbo = 0;

    GLuint quadVbo = MakeQuadVbo();

    vector<InstanceInfo> vertices;

    for (int z = 0; z < dimensions.z; ++z) {
        for (int y = 0; y < dimensions.y; ++y) {
            for (int x = 0; x < dimensions.x; ++x) {
                ivec3 idx(x, y, z);
                vec3 offset = vec3(idx) * spacing;
                offset -= vec3(0, dimensions.y, 0) * spacing / 2.0f;
                BufferVoxelSetInstanced(model, offset, vertices);

                glBindVertexArray(vaos[nextVbo]);
                EnableQuadAttributes(program, quadVbo);
                glBindBuffer(GL_ARRAY_BUFFER, vbos[nextVbo]);
                glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceInfo) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

                GLint vPosLoc = glGetAttribLocation(program, "vCenter");
                glEnableVertexAttribArray(vPosLoc);
                glVertexAttribPointer(vPosLoc, 3, GL_FLOAT, GL_FALSE,
                                      sizeof(InstanceInfo),
                                      (void*)offsetof(InstanceInfo, position));
                glVertexAttribDivisor(vPosLoc, 1);
                CheckGLErrors();

                GLint vColorPos = glGetAttribLocation(program, "vColor");
                glEnableVertexAttribArray(vColorPos);
                glVertexAttribPointer(vColorPos, 4, GL_UNSIGNED_INT_2_10_10_10_REV, GL_TRUE,
                                      sizeof(InstanceInfo),
                                      (void*)offsetof(InstanceInfo, color));
                glVertexAttribDivisor(vColorPos, 1);
                CheckGLErrors();

                GLint vNormal = glGetAttribLocation(program, "vNormal");
                glEnableVertexAttribArray(vNormal);
                glVertexAttribPointer(vNormal, 4, GL_INT_2_10_10_10_REV, GL_TRUE,
                                      sizeof(InstanceInfo),
                                      (void*)offsetof(InstanceInfo, normal));
                glVertexAttribDivisor(vNormal, 1);
                CheckGLErrors();

                nextVbo++;
            }
        }
    }

    return vertices.size();
}

PerfRecord RunInstancedTest(VoxelSet& model, glm::ivec3 gridSize, glm::vec3 voxelSpacing) {
    GLuint program;
    GLint mvpLoc;
    vector<GLuint> vaos;
    vector<GLuint> vbos;
    size_t vertexCount;

    PerfRecord record = RunPerf(
        [&]() {
        program = MakeShaderProgram({
            { "Shaders/instanced.vert", GL_VERTEX_SHADER },
            { "Shaders/instanced.frag", GL_FRAGMENT_SHADER },
        });
        mvpLoc = glGetUniformLocation(program, "mvp");
        vertexCount = MakeInstancedGrid(model, gridSize, voxelSpacing, vaos, vbos, program);
    },
        [&]() {
        mat4 mvp = MakeMvp();

        glUseProgram(program);
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, (const GLfloat*)&mvp);

        for (GLuint vao : vaos) {
            glBindVertexArray(vao);
            glDrawArraysInstanced(GL_QUADS, 0, 4, vertexCount);
        }
    },
        [&]() {
        glDeleteBuffers(vbos.size(), &vbos[0]);
        CheckGLErrors();

        glDeleteVertexArrays(vaos.size(), &vaos[0]);
        CheckGLErrors();

        glDeleteProgram(program);
        CheckGLErrors();
    }
    );

    return record;
}
