#include "QuadGeom.h"

#include <iostream>

#include <cstddef>
#include <vector>

using namespace glm;
using namespace std;

#pragma pack(push, 1)
struct PointQuad {
    vec3 position;
    //vec3 color;
    PackedColor color;

    //PackedVec extent1;
    //PackedVec extent2;
    uint8_t faceIdx;
};
#pragma pack(pop)

int FloatToPackedInt(float f) {
    if (f == 1.0f) {
        //return 511;
        return 511;
    }

    if (f == -1.0f) {
        return -512;
    }

    return 0;
}

PackedVec PackVec(vec3 v) {
    PackedVec p;
    p.w = 0;
    p.x = FloatToPackedInt(v.x);
    p.y = FloatToPackedInt(v.y);
    p.z = FloatToPackedInt(v.z);

    return p;
}

void BufferPointQuadFace(vec3 center, vec3 normal, vec3 color, vector<PointQuad>& vertices, int& nextIdx,
                         uint8_t faceIdx) {
    // Center of voxel to center of face
    center += vec3(normal) * VOXEL_SIZE / 2.0f;

    vec3 d1 = -vec3(normal.z, normal.x, normal.y);
    vec3 d2 = -vec3(normal.y, normal.z, normal.x);

    vector<vec2> weights = {
        {-1, -1 },
        { 1, -1 },
        { 1,  1 },
        {-1,  1 },
    };

    // Reverse winding order for negative normals
    if (normal.x < 0 || normal.y < 0 || normal.z < 0) {
        std::swap(d1, d2);
    }

    PointQuad p;
    p.color = PackColor(color);
    p.position = center;
    p.faceIdx = faceIdx;
    //p.extent1 = d1 / 2.0f * VOXEL_SIZE;
    //p.extent2 = d2 / 2.0f * VOXEL_SIZE;

    //p.extent1 = PackWXYZ(d1);
    //p.extent2 = PackWXYZ(d2);
    //p.extent1 = PackVec(d1);
    //p.extent2 = PackVec(d2);
    
    if (nextIdx >= vertices.size()) {
        vertices.push_back(p);
    } else {
        vertices[nextIdx] = p;
    }
    nextIdx++;
}

// Buffers all faces of a voxel in the given vector
void BufferPointQuadVoxel(VoxelSet& voxels, vec3 offset, ivec3 idx, vector<PointQuad>& vertices, int& nextIdx) {
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

    PointQuad p;
    int enabledFaces = 0;

    for (int i = 0; i < normals.size(); ++i) {
        if (voxels.IsSolid(idx + normals[i])) {
            continue;
        }
        BufferPointQuadFace(center, vec3(normals[i]), color, vertices, nextIdx, (uint8_t)i);
    }
}

// Buffers an entire voxel model in the given vector
void BufferVoxelSetPointQuads(VoxelSet& voxels, vec3 offset, vector<PointQuad>& vertices) {
    int nextIdx = 0;

    for (int z = 0; z < voxels.size.z; ++z) {
        for (int y = 0; y < voxels.size.y; ++y) {
            for (int x = 0; x < voxels.size.x; ++x) {
                BufferPointQuadVoxel(voxels, offset, ivec3(x, y, z), vertices, nextIdx);
            }
        }
    }
}

size_t MakeGridPointQuads(VoxelSet& model, ivec3 dimensions, vec3 spacing, std::vector<GLuint>& vaos,
                      std::vector<GLuint>& vbos, GLuint program) {
    vbos.resize(dimensions.x * dimensions.y * dimensions.z);
    vaos.resize(dimensions.x * dimensions.y * dimensions.z);

    glGenBuffers(vbos.size(), &vbos[0]);
    CheckGLErrors();

    glGenVertexArrays(vaos.size(), &vaos[0]);
    CheckGLErrors();

    int nextVbo = 0;

    vector<PointQuad> vertices;

    for (int z = 0; z < dimensions.z; ++z) {
        for (int y = 0; y < dimensions.y; ++y) {
            for (int x = 0; x < dimensions.x; ++x) {
                ivec3 idx(x, y, z);
                vec3 offset = vec3(idx) * spacing;
                offset -= vec3(0, dimensions.y, 0) * spacing / 2.0f;
                BufferVoxelSetPointQuads(model, offset, vertices);

                glBindVertexArray(vaos[nextVbo]);
                glBindBuffer(GL_ARRAY_BUFFER, vbos[nextVbo]);
                glBufferData(GL_ARRAY_BUFFER, sizeof(PointQuad) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

                GLint vPosLoc = glGetAttribLocation(program, "vPos");
                glEnableVertexAttribArray(vPosLoc);
                glVertexAttribPointer(vPosLoc, 3, GL_FLOAT, GL_FALSE,
                                      sizeof(PointQuad),
                                      (void*)offsetof(PointQuad, position));
                CheckGLErrors();

                GLint vColorPos = glGetAttribLocation(program, "vColor");
                glEnableVertexAttribArray(vColorPos);
                glVertexAttribPointer(vColorPos, 4, GL_UNSIGNED_INT_2_10_10_10_REV, GL_TRUE,
                                      sizeof(PointQuad),
                                      (void*)offsetof(PointQuad, color));
                CheckGLErrors();

                /*GLint vExtent1 = glGetAttribLocation(program, "vExtent1");
                glEnableVertexAttribArray(vExtent1);
                glVertexAttribPointer(vExtent1, 4, GL_INT_2_10_10_10_REV, GL_TRUE,
                                       sizeof(PointQuad),
                                       (void*)offsetof(PointQuad, extent1));
                CheckGLErrors();

                GLint vExtent2 = glGetAttribLocation(program, "vExtent2");
                glEnableVertexAttribArray(vExtent2);
                glVertexAttribPointer(vExtent2, 4, GL_INT_2_10_10_10_REV, GL_TRUE,
                                      sizeof(PointQuad),
                                      (void*)offsetof(PointQuad, extent2));
                CheckGLErrors();*/

                GLint vExtent1 = glGetAttribLocation(program, "vFaceIdx");
                glEnableVertexAttribArray(vExtent1);
                glVertexAttribIPointer(vExtent1, 1, GL_BYTE,
                                      sizeof(PointQuad),
                                      (void*)offsetof(PointQuad, faceIdx));
                CheckGLErrors();

                nextVbo++;
            }
        }
    }

    return vertices.size();
}

PerfRecord RunQuadGeometryShaderTest(VoxelSet & model, glm::ivec3 gridSize, glm::vec3 voxelSpacing) {
    GLuint program;
    GLint mvpLoc;
    vector<GLuint> vaos;
    vector<GLuint> vbos;
    size_t vertexCount;

    GLuint displayList = 0xffffffff;
    PerfRecord record = RunPerf(
        [&]() {
        // Setup
        program = MakeShaderProgram({
            { "Shaders/point_quads.vert", GL_VERTEX_SHADER },
            { "Shaders/point_voxels.frag", GL_FRAGMENT_SHADER },
            { "Shaders/point_quads.geom", GL_GEOMETRY_SHADER },
        });
        mvpLoc = glGetUniformLocation(program, "mvp");
        vertexCount = MakeGridPointQuads(model, gridSize, voxelSpacing, vaos, vbos, program);
        //glPointSize(4.0f);
    },
        [&]() {
        // Draw
        mat4 mvp = MakeMvp();
        //PrintMatrix(mvp);

        /*if (displayList == 0xffffffff) {
            displayList = glGenLists(1);
            glNewList(displayList, GL_COMPILE);*/

            glUseProgram(program);
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, (const GLfloat*)&mvp);

            for (GLuint vao : vaos) {
                glBindVertexArray(vao);
                glDrawArrays(GL_POINTS, 0, vertexCount);
                //glDrawArrays(GL_POINTS, 0, 3);
            }
        /*}

        glCallList(displayList);*/
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
