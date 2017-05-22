#include "DisplayLists.h"

#include <vector>

using namespace glm;
using namespace std;

// Draws a single face of a voxel
void DrawFace(vec3 center, vec3 normal) {
    // Center of voxel to center of face
    center += vec3(normal) * VOXEL_SIZE / 2.0f;

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
        vec3 position = center + (d1 * w.x + d2 * w.y) / 2.0f * VOXEL_SIZE;
        glVertex3fv((float*)&position);
    }
}

// Draws a single voxel
void DrawVoxel(VoxelSet& voxels, vec3 offset, ivec3 idx) {
    if (!voxels.IsSolid(idx)) {
        return;
    }

    vec3 color = voxels.At(idx);
    glColor3fv((float*)&color);
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
        DrawFace(center, vec3(n));
    }
}

// Converts a voxel set to a display list
GLuint VoxelsToDisplayList(VoxelSet& voxels, vec3 offset) {
    GLuint displayIdx = glGenLists(1);
    glNewList(displayIdx, GL_COMPILE);

    glBegin(GL_QUADS);
    for (int z = 0; z < voxels.size.z; ++z) {
        for (int y = 0; y < voxels.size.y; ++y) {
            for (int x = 0; x < voxels.size.x; ++x) {
                DrawVoxel(voxels, offset, ivec3(x, y, z));
            }
        }
    }
    glEnd();
    glEndList();

    return displayIdx;
}

// Makes a grid of voxel objects from the given model
void MakeDisplayListGrid(VoxelSet& model, ivec3 dimensions, vec3 spacing, std::vector<GLuint>& displayLists) {
    displayLists.resize(dimensions.x * dimensions.y * dimensions.z);
    int nextListIdx = 0;

    for (int z = 0; z < dimensions.z; ++z) {
        for (int y = 0; y < dimensions.y; ++y) {
            for (int x = 0; x < dimensions.x; ++x) {
                ivec3 idx(x, y, z);
                vec3 offset = vec3(idx) * spacing;
                offset -= vec3(0, dimensions.y, 0) * spacing / 2.0f;
                displayLists[nextListIdx] = VoxelsToDisplayList(model, offset);
                nextListIdx++;
            }
        }
    }
}

PerfRecord RunDrawListsTest(VoxelSet& model, glm::ivec3 gridSize, glm::vec3 voxelSpacing) {
    std::vector<GLuint> displayLists;
    PerfRecord record = RunPerf(
        [&]() {
            glUseProgram(0);
            MakeDisplayListGrid(model, gridSize, voxelSpacing, displayLists);
        },
        [&]() {
            mat4 mv = MakeModelView();
            mat4 p = MakeProjection();

            glMatrixMode(GL_PROJECTION);
            glLoadMatrixf((float*)&p);
            glMatrixMode(GL_MODELVIEW);
            glLoadMatrixf((float*)&mv);

            for (auto& list : displayLists) {
                glCallList(list);
            }
        },
        [&]() {
            for (auto& list : displayLists) {
                glDeleteLists(list, 1);
            }
            CheckGLErrors();
        }
    );

    return record;
}
