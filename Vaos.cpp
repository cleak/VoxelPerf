#include "Vaos.h"

#include <vector>

using namespace glm;
using namespace std;

#pragma pack(push, 1)
struct Vertex {
    vec3 position;
    PackedColor color;
};
#pragma pack(pop)

// Buffers a single face of a voxel in the given vector
static void BufferFace(vec3 center, vec3 normal, vec3 color, vector<Vertex>& vertices, int& nextIdx) {
    // Center of voxel to center of face
    center += vec3(normal) * VOXEL_SIZE / 2.0f;

    vec3 d1 = -vec3(normal.z, normal.x, normal.y);
    vec3 d2 = -vec3(normal.y, normal.z, normal.x);

    vector<vec2> weights = {
        { -1, -1 },
        {  1, -1 },
        {  1,  1 },
        { -1,  1 },
    };

    // Reverse winding order for negative normals
    if (normal.x < 0 || normal.y < 0 || normal.z < 0) {
        std::swap(d1, d2);
    }

    for (int i = 0; i < 4; ++i) {
        vec2 w = weights[i];

        Vertex v;
        v.color = PackColor(color);
        v.position = center + (d1 * w.x + d2 * w.y) / 2.0f * VOXEL_SIZE;

        if (nextIdx >= vertices.size()) {
            vertices.push_back(v);
        } else {
            vertices[nextIdx] = v;
        }
        nextIdx++;
    }
}

// Buffers all faces of a voxel in the given vector
static void BufferVoxel(VoxelSet& voxels, vec3 offset, ivec3 idx, vector<Vertex>& vertices, int& nextIdx) {
    if (!voxels.IsSolid(idx)) {
        return;
    }

    vec3 color = voxels.At(idx);
    vec3 center = offset + vec3(idx) * VOXEL_SIZE + vec3(VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE) / 2.0f;

    vector<ivec3> normals = {
        { 1, 0, 0 },
        {-1, 0, 0 },
        { 0, 1, 0 },
        { 0,-1, 0 },
        { 0, 0, 1 },
        { 0, 0,-1 },
    };

    for (auto& n : normals) {
        if (voxels.IsSolid(idx + n)) {
            continue;
        }
        BufferFace(center, vec3(n), color, vertices, nextIdx);
    }
}

// Buffers an entire voxel model in the given vector
static void BufferVoxelSet(VoxelSet& voxels, vec3 offset, vector<Vertex>& vertices) {
    int nextIdx = 0;

    for (int z = 0; z < voxels.size.z; ++z) {
        for (int y = 0; y < voxels.size.y; ++y) {
            for (int x = 0; x < voxels.size.x; ++x) {
                BufferVoxel(voxels, offset, ivec3(x, y, z), vertices, nextIdx);
            }
        }
    }
}

static size_t MakeVaoGrid(VoxelSet& model, ivec3 dimensions, vec3 spacing, std::vector<GLuint>& vaos,
                   std::vector<GLuint>& vbos, GLuint program) {
    vbos.resize(dimensions.x * dimensions.y * dimensions.z);
    vaos.resize(dimensions.x * dimensions.y * dimensions.z);

    glGenBuffers(vbos.size(), &vbos[0]);
    CheckGLErrors();

    glGenVertexArrays(vaos.size(), &vaos[0]);
    CheckGLErrors();

    int nextVbo = 0;

    vector<Vertex> vertices;

    for (int z = 0; z < dimensions.z; ++z) {
        for (int y = 0; y < dimensions.y; ++y) {
            for (int x = 0; x < dimensions.x; ++x) {
                ivec3 idx(x, y, z);
                vec3 offset = vec3(idx) * spacing;
                offset -= vec3(0, dimensions.y, 0) * spacing / 2.0f;
                BufferVoxelSet(model, offset, vertices);

                glBindVertexArray(vaos[nextVbo]);
                glBindBuffer(GL_ARRAY_BUFFER, vbos[nextVbo]);
                glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

                GLint vPosLoc = glGetAttribLocation(program, "vPos");
                glEnableVertexAttribArray(vPosLoc);
                glVertexAttribPointer(vPosLoc, 3, GL_FLOAT, GL_FALSE,
                                      sizeof(Vertex), (void*)0);
                CheckGLErrors();

                GLint vColorPos = glGetAttribLocation(program, "vColor");
                glEnableVertexAttribArray(vColorPos);
                glVertexAttribPointer(vColorPos, 4, GL_UNSIGNED_INT_2_10_10_10_REV, GL_TRUE,
                                      sizeof(Vertex),
                                      (void*)offsetof(Vertex, color));
                CheckGLErrors();

                nextVbo++;
            }
        }
    }

    return vertices.size();
}

PerfRecord RunVaosTest(VoxelSet & model, glm::ivec3 gridSize, glm::vec3 voxelSpacing) {
    GLuint program;
    GLint mvpLoc;
    vector<GLuint> vaos;
    vector<GLuint> vbos;
    size_t vertexCount;

    PerfRecord record = RunPerf(
        [&]() {
            program = MakeShaderProgram({
                { "Shaders/colored.vert", GL_VERTEX_SHADER },
                { "Shaders/colored.frag", GL_FRAGMENT_SHADER },
            });
            mvpLoc = glGetUniformLocation(program, "mvp");
            vertexCount = MakeVaoGrid(model, gridSize, voxelSpacing, vaos, vbos, program);
        },
        [&]() {
            mat4 mvp = MakeMvp();

            glUseProgram(program);
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, (const GLfloat*)&mvp);

            for (GLuint vao : vaos) {
                glBindVertexArray(vao);
                glDrawArrays(GL_QUADS, 0, vertexCount);
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
