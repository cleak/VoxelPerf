#include "QuadGeom.h"

#include <iostream>

#include <cstddef>
#include <vector>

using namespace glm;
using namespace std;

#pragma pack(push, 1)
struct PointQuad {
    //vec3 position;
    uint8_t x;
    uint8_t y;
    uint8_t z;
    //uint8_t pad0;

    //PackedColor color;
    uint8_t r;
    uint8_t g;
    uint8_t b;

    uint8_t faceIdx;
};
#pragma pack(pop)

// Stores information about the location and size of each set of layers of a voxel object
struct LayersInfo {
    uint16_t layerStartIdx[6];
    uint16_t layerSize[6];
};

struct LayeredVertexBuffer {
    int nextLayerIdx[6] = {};
    vector<PointQuad> layer[6] = {};
};

int FloatToPackedInt(float f) {
    if (f == 1.0f) {
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

uint8_t RoundColor(float c) {
    return (uint8_t)round(c * 0xff);
}

void BufferPointQuadFace(ivec3 voxIdx, ivec3 normal, vec3 color, vector<PointQuad>& vertices, int& nextIdx,
                         uint8_t faceIdx) {

    if (normal.x > 0 || normal.y > 0 || normal.z > 0) {
        voxIdx += normal;
    }

    PointQuad p;
    p.r = RoundColor(color.r);
    p.g = RoundColor(color.g);
    p.b = RoundColor(color.b);
    
    p.x = voxIdx.x;
    p.y = voxIdx.y;
    p.z = voxIdx.z;

    p.faceIdx = faceIdx;
    
    if (nextIdx >= vertices.size()) {
        vertices.push_back(p);
    } else {
        vertices[nextIdx] = p;
    }
    nextIdx++;
}

// Buffers all faces of a voxel in the given vector
void BufferPointQuadVoxel(VoxelSet& voxels, vec3 offset, ivec3 idx, LayeredVertexBuffer& vertexBuffers) {
    if (!voxels.IsSolid(idx)) {
        return;
    }

    vec3 color = voxels.At(idx);

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
        BufferPointQuadFace(idx, normals[i], color, vertexBuffers.layer[i], vertexBuffers.nextLayerIdx[i], (uint8_t)i);
    }
}

// Buffers an entire voxel model in the given vector
void BufferVoxelSetPointQuads(VoxelSet& voxels, vec3 offset, LayeredVertexBuffer& vertexBuffers) {

    for (int i = 0; i < 6; ++i) {
        vertexBuffers.nextLayerIdx[i] = 0;
    }

    for (int z = 0; z < voxels.size.z; ++z) {
        for (int y = 0; y < voxels.size.y; ++y) {
            for (int x = 0; x < voxels.size.x; ++x) {
                BufferPointQuadVoxel(voxels, offset, ivec3(x, y, z), vertexBuffers);
            }
        }
    }
}

void MakeGridPointQuads(VoxelSet& model, ivec3 dimensions, vec3 spacing, std::vector<GLuint>& vaos,
                      std::vector<GLuint>& vbos, std::vector<LayersInfo>& layersInfo, GLuint program) {

    size_t modelCount = dimensions.x * dimensions.y * dimensions.z;

    vbos.resize(modelCount);
    vaos.resize(modelCount);
    layersInfo.resize(modelCount);
    
    glGenBuffers(vbos.size(), &vbos[0]);
    CheckGLErrors();

    glGenVertexArrays(vaos.size(), &vaos[0]);
    CheckGLErrors();

    int nextVbo = 0;

    LayeredVertexBuffer vertexBuffers;

    for (int z = 0; z < dimensions.z; ++z) {
        for (int y = 0; y < dimensions.y; ++y) {
            for (int x = 0; x < dimensions.x; ++x) {

                ivec3 idx(x, y, z);
                vec3 offset = vec3(idx) * spacing;
                offset -= vec3(0, dimensions.y, 0) * spacing / 2.0f;

                BufferVoxelSetPointQuads(model, offset, vertexBuffers);

                glBindVertexArray(vaos[nextVbo]);
                glBindBuffer(GL_ARRAY_BUFFER, vbos[nextVbo]);
                
                int totalQuads = 0;
                for (int i = 0; i < 6; ++i) {
                    totalQuads += vertexBuffers.layer[i].size();
                }

                glBufferData(GL_ARRAY_BUFFER, sizeof(PointQuad) * totalQuads, nullptr, GL_STATIC_DRAW);

                layersInfo[nextVbo].layerStartIdx[0] = 0;

                for (int i = 0; i < 6; ++i) {
                    layersInfo[nextVbo].layerSize[i] = vertexBuffers.layer[i].size();
                    if (i > 0) {
                        layersInfo[nextVbo].layerStartIdx[i] = layersInfo[nextVbo].layerStartIdx[i - 1] + layersInfo[nextVbo].layerSize[i];
                    }

                    glBufferSubData(GL_ARRAY_BUFFER, layersInfo[nextVbo].layerStartIdx[i] * sizeof(PointQuad),
                                    layersInfo[nextVbo].layerSize[i] * sizeof(PointQuad), &vertexBuffers.layer[i][0]);
                }

                GLint vPosLoc = glGetAttribLocation(program, "vPos");
                glEnableVertexAttribArray(vPosLoc);
                glVertexAttribPointer(vPosLoc, 3, GL_BYTE, GL_FALSE,
                                      sizeof(PointQuad),
                                      (void*)offsetof(PointQuad, x));
                CheckGLErrors();

                GLint vColorPos = glGetAttribLocation(program, "vColor");
                glEnableVertexAttribArray(vColorPos);
                glVertexAttribPointer(vColorPos, 3, GL_UNSIGNED_BYTE, GL_TRUE,
                sizeof(PointQuad),
                (void*)offsetof(PointQuad, r));
                CheckGLErrors();

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
}

PerfRecord RunQuadGeometryShaderTest(VoxelSet & model, glm::ivec3 gridSize, glm::vec3 voxelSpacing) {
    GLuint program;
    
    GLint mvpLoc;
    GLint offsetLoc;

    vector<GLuint> vaos;
    vector<GLuint> vbos;

    vector<vec3> offsets;

    std::vector<LayersInfo> layersInfo;

    vector<ivec3> normals = {
        { 1, 0, 0 },
        { -1, 0, 0 },

        { 0, 1, 0 },
        { 0,-1, 0 },

        { 0, 0, 1 },
        { 0, 0,-1 },
    };

    PerfRecord record = RunPerf(
        [&]() {
        // Setup
        program = MakeShaderProgram({
            { "Shaders/point_quads.vert", GL_VERTEX_SHADER },
            { "Shaders/point_voxels.frag", GL_FRAGMENT_SHADER },
            { "Shaders/point_quads.geom", GL_GEOMETRY_SHADER },
        });
        mvpLoc = glGetUniformLocation(program, "mvp");
        offsetLoc = glGetUniformLocation(program, "vOffset");
        MakeGridPointQuads(model, gridSize, voxelSpacing, vaos, vbos, layersInfo, program);

        offsets.resize(vaos.size());
        int nextOffsetIdx = 0;
        for (int z = 0; z < gridSize.z; ++z) {
            for (int y = 0; y < gridSize.y; ++y) {
                for (int x = 0; x < gridSize.x; ++x) {
                    ivec3 idx(x, y, z);
                    vec3 offset = vec3(idx) * voxelSpacing;
                    offset -= vec3(0, gridSize.y, 0) * voxelSpacing / 2.0f;
                    offsets[nextOffsetIdx] = offset;
                    nextOffsetIdx++;
                }
            }
        }
    },
    [&]() {
        // Draw
        mat4 mvp = MakeMvp();
        //PrintMatrix(mvp);

        glUseProgram(program);
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, (const GLfloat*)&mvp);

        for (int i = 0; i < vaos.size(); ++i) {
            vec3 offset = offsets[i];
            glUniform3fv(offsetLoc, 1, (const GLfloat*)&offset);
            glBindVertexArray(vaos[i]);

            for (int j = 0; j < 6; ++j) {
				// Kludge to iterate over x twice, y twice, and z twice
                int compareIdx = j / 2;

				// Visibility threshold along a particular axis
                float thresh = offset[compareIdx] + voxelSpacing[compareIdx] / 2.0f;
                
				// Direction of normal (used for deciding how to compare)
                float normalDir = normals[j][compareIdx];
                thresh += normalDir * voxelSpacing[compareIdx] / 2.0f;

                // Cull layers that are entirely backfacing
                if (normalDir * thresh > normalDir * CameraPosition()[compareIdx]) {
                    continue;
                }

                glDrawArrays(GL_POINTS, layersInfo[i].layerStartIdx[j], layersInfo[i].layerSize[j]);
            }
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
