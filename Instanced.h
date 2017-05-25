#pragma once

#include "PerfFramework.h"
#include "Voxels.h"

#include <GL/glew.h>
#include <glm/glm.hpp>

PerfRecord RunInstancedTest(VoxelSet& model, glm::ivec3 gridSize, glm::vec3 voxelSpacing);
