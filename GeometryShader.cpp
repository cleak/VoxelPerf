#include "GeometryShader.h"

#include <vector>

using namespace glm;
using namespace std;

#pragma pack(push, 1)
struct PointVertex {
    vec3 position;
    vec3 color;
    uint8_t enabledFaces;
};
#pragma pack(pop)

// Buffers all faces of a voxel in the given vector
void BufferVoxelPoint(VoxelSet& voxels, vec3 offset, ivec3 idx, vector<PointVertex>& vertices, int& nextIdx) {
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

    PointVertex p;
    p.enabledFaces = 0;

    for (int i = 0; i < normals.size(); ++i) {
        if (voxels.IsSolid(idx + normals[i])) {
            continue;
        }

        p.enabledFaces |= (1 << i);
    }

    p.position = center;
    p.color = color;

    if (nextIdx >= vertices.size()) {
        vertices.push_back(p);
    } else {
        vertices[nextIdx] = p;
    }
    nextIdx++;
}

// Buffers an entire voxel model in the given vector
void BufferVoxelSetPoints(VoxelSet& voxels, vec3 offset, vector<PointVertex>& vertices) {
    int nextIdx = 0;

    for (int z = 0; z < voxels.size.z; ++z) {
        for (int y = 0; y < voxels.size.y; ++y) {
            for (int x = 0; x < voxels.size.x; ++x) {
                BufferVoxelPoint(voxels, offset, ivec3(x, y, z), vertices, nextIdx);
            }
        }
    }
}

size_t MakeGridPoints(VoxelSet& model, ivec3 dimensions, vec3 spacing, std::vector<GLuint>& vaos,
                   std::vector<GLuint>& vbos, GLuint program) {
    vbos.resize(dimensions.x * dimensions.y * dimensions.z);
    vaos.resize(dimensions.x * dimensions.y * dimensions.z);

    glGenBuffers(vbos.size(), &vbos[0]);
    CheckGLErrors();

    glGenVertexArrays(vaos.size(), &vaos[0]);
    CheckGLErrors();

    int nextVbo = 0;

    vector<PointVertex> vertices;

    for (int z = 0; z < dimensions.z; ++z) {
        for (int y = 0; y < dimensions.y; ++y) {
            for (int x = 0; x < dimensions.x; ++x) {
                ivec3 idx(x, y, z);
                vec3 offset = vec3(idx) * spacing;
                offset -= vec3(0, dimensions.y, 0) * spacing / 2.0f;
                BufferVoxelSetPoints(model, offset, vertices);

                glBindVertexArray(vaos[nextVbo]);
                glBindBuffer(GL_ARRAY_BUFFER, vbos[nextVbo]);
                glBufferData(GL_ARRAY_BUFFER, sizeof(PointVertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

                GLint vPosLoc = glGetAttribLocation(program, "vPos");
                glEnableVertexAttribArray(vPosLoc);
                glVertexAttribPointer(vPosLoc, 3, GL_FLOAT, GL_FALSE,
                                      sizeof(float) * 6 + 1, (void*)0);
                CheckGLErrors();

                GLint vColorPos = glGetAttribLocation(program, "vColor");
                glEnableVertexAttribArray(vColorPos);
                glVertexAttribPointer(vColorPos, 3, GL_FLOAT, GL_FALSE,
                                      sizeof(float) * 6 + 1, (void*)(sizeof(float) * 3));
                CheckGLErrors();

                GLint vEnabledFaces = glGetAttribLocation(program, "vEnabledFaces");
                glEnableVertexAttribArray(vEnabledFaces);
                glVertexAttribIPointer(vEnabledFaces, 1, GL_UNSIGNED_BYTE, 
                                       sizeof(float) * 6 + 1, (void*)(sizeof(float) * 6));
                CheckGLErrors();

                nextVbo++;
            }
        }
    }

    return vertices.size();
}

PerfRecord RunGeometryShaderTest(VoxelSet & model, glm::ivec3 gridSize, glm::vec3 voxelSpacing) {
    GLuint program;
    GLint mvpLoc;
    vector<GLuint> vaos;
    vector<GLuint> vbos;
    size_t vertexCount;

    PerfRecord record = RunPerf(
    [&]() {
        // Setup
        program = MakeShaderProgram({
            { "Shaders/point_voxels.vert", GL_VERTEX_SHADER },
            { "Shaders/point_voxels.frag", GL_FRAGMENT_SHADER },
            { "Shaders/point_voxels.geom", GL_GEOMETRY_SHADER },
        });
        mvpLoc = glGetUniformLocation(program, "mvp");
        vertexCount = MakeGridPoints(model, gridSize, voxelSpacing, vaos, vbos, program);
        //glPointSize(4.0f);
    },
    [&]() {
        // Draw
        mat4 mvp = MakeMvp();
        //PrintMatrix(mvp);

        glUseProgram(program);
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, (const GLfloat*)&mvp);

        for (GLuint vao : vaos) {
            glBindVertexArray(vao);
            glDrawArrays(GL_POINTS, 0, vertexCount);
        }
    },
    [&]() {
        // Teardown
        glDeleteBuffers(vbos.size(), &vbos[0]);
        CheckGLErrors();

        glDeleteVertexArrays(vaos.size(), &vaos[0]);
        CheckGLErrors();

        glDeleteProgram(program);
        CheckGLErrors();
    });

    return record;
}
