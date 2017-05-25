#pragma once

#include "PerfFramework.h"
#include "Voxels.h"

#include <GL/glew.h>
#include <glm/glm.hpp>

// Runs a performance test using a geometry shader that generates multiple quads per primitive
PerfRecord RunGeometryShaderTest(VoxelSet& model, glm::ivec3 gridSize, glm::vec3 voxelSpacing);
