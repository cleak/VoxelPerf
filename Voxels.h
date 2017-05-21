#pragma once

#include <vector>

#include <glm/glm.hpp>

#define VOXEL_SIZE (0.25f)

struct VoxelSet {
    glm::ivec3 size;
    std::vector<glm::vec4> colors;

    // Constructs a voxel set of the given dimensions
    inline VoxelSet(glm::ivec3 size) {
        int elemCount = size.x * size.y * size.z;
        this->size = size;
        colors.resize(elemCount);
        for (int i = 0; i < elemCount; ++i) {
            colors[i] = glm::vec4(0, 0, 0, 0);
        }
    }

    // Returns true if the given index is valid for the voxel set
    inline bool IsValid(glm::ivec3 idx) {
        return idx.x >= 0 && idx.y >= 0 && idx.z >= 0
            && idx.x < size.x && idx.y < size.y && idx.z < size.z;
    }

    // Returns true if idx is a valid index and represents a solid voxel; false otherwise.
    inline bool IsSolid(glm::ivec3 idx) {
        return IsValid(idx) && At(idx).a > 0.1f;
    }

    // Access the voxel at the given index
    inline glm::vec4& At(glm::ivec3 idx) {
        return colors[idx.z * (size.x * size.y) + idx.y * size.x + idx.x];
    }

    // Make the voxel set into a colored sphere
    inline void MakeSphere() {
        float radius = size.x / 2.0f - 0.5f;
        glm::vec3 color = 1.0f / (glm::vec3(size) - glm::vec3(1, 1, 1));
        glm::vec3 center = glm::vec3(size) / 2.0f;

        for (int z = 0; z < size.z; ++z) {
            for (int y = 0; y < size.y; ++y) {
                for (int x = 0; x < size.x; ++x) {

                    glm::ivec3 idx(x, y, z);

                    glm::vec3 delta = (glm::vec3(idx) + glm::vec3(0.5f, 0.5f, 0.5f)) - center;

                    if ((glm::dot(delta, delta) - 0.1f) <= radius * radius) {
                        // Inside sphere
                        At(idx) = glm::vec4(color * glm::vec3(idx), 1.0f);
                    } else {
                        // Outside sphere
                        At(idx) = glm::vec4(0, 0, 0, 0);
                    }
                }
            }
        }
    }
};

