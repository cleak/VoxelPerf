#pragma once

#include "PerfFramework.h"
#include "Voxels.h"

#include <GL/glew.h>
#include <glm/glm.hpp>

PerfRecord RunVaosTest(VoxelSet& model, glm::ivec3 gridSize, glm::vec3 voxelSpacing);
