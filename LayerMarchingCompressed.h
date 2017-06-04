#pragma once

#include "PerfFramework.h"
#include "Voxels.h"

#include <GL/glew.h>
#include <glm/glm.hpp>

// Runs a performance test that uses plane marching and compressed textures
PerfRecord RunLayerMarchingCompressedTest(VoxelSet& model, glm::ivec3 gridSize, glm::vec3 voxelSpacing);
