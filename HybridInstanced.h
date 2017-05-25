#pragma once

#include "PerfFramework.h"
#include "Voxels.h"

#include <GL/glew.h>
#include <glm/glm.hpp>

// Runs a performance test that uses a full set of positions, but attribute dividers for color
PerfRecord RunHybridInstancedTest(VoxelSet& model, glm::ivec3 gridSize, glm::vec3 voxelSpacing);
