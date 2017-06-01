#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <functional>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "PerfFramework.h"
#include "PerfTimer.h"
#include "Voxels.h"

// Techniquess
#include "DisplayLists.h"
#include "Vaos.h"
#include "GeometryShader.h"
#include "QuadGeom.h"
#include "Instanced.h"
#include "CompactDisplayLists.h"
#include "HybridInstanced.h"
#include "SignedDistanceFields.h"
#include "LayerMarching.h"
#include "SdfShape.h"

using namespace glm;
using namespace std;

typedef PerfRecord(*PerfTestFn)(VoxelSet& model, glm::ivec3 gridSize, glm::vec3 voxelSpacing);

static map<string, PerfTestFn> tests = {
	{ "dl", RunDisplayListsTest },
	{ "cdl", RunCompactDisplayListsTest },
	{ "vao", RunVaosTest },
	{ "gs", RunGeometryShaderTest },
	{ "qgs", RunQuadGeometryShaderTest },
	{ "inst", RunInstancedTest },
	{ "hyi", RunHybridInstancedTest },
    { "sdf", RunSdfTest },
    { "sdfs", RunSdfShapeTest },
    { "lm", RunLayerMarchingTest },
};

template <typename T>
string Join(T iterable, string delim) {
	int count = 0;
	string result = "";
	for (auto& t : iterable) {
		if (count > 0) {
			result += delim;
		}
		result += t;
		++count;
	}
	return result;
}

template <typename T>
string JoinFirst(T iterable, string delim) {
	int count = 0;
	string result = "";
	for (auto& t : iterable) {
		if (count > 0) {
			result += delim;
		}
		result += t.first;
		++count;
	}
	return result;
}

void Usage() {
	cerr << "VoxelPerf [" << JoinFirst(tests, "|") << "] <width> <height> <depth>" << endl;
}

int main(int argc, char** argv) {
    if (argc != 5) {
        Usage();
        exit(EXIT_FAILURE);
    }

    string testType = string(argv[1]);
    if (!tests.count(testType)) {
        Usage();
        exit(EXIT_FAILURE);
    }

    int w = atoi(argv[2]);
    int h = atoi(argv[3]);
    int d = atoi(argv[4]);

    if (w <= 0 || h <= 0 || d <= 0) {
        Usage();
        exit(EXIT_FAILURE);
    }

    VoxelSet sphere({ 32,32,32 });
    sphere.MakeSphere();

    ivec3 voxelGrid(w, h, d);

    vec3 voxelSpacing(32, 32, 32);
    voxelSpacing *= VOXEL_SIZE;

    int numObjects = voxelGrid.x * voxelGrid.y * voxelGrid.z;

    PerfRecord record = tests[testType](sphere, voxelGrid, voxelSpacing);

    cout << testType << ", " << numObjects << ", " << record.averageFrameTimeMs << ", " << record.gpuMemUsed << ", " << record.mainMemUsed << endl;
    exit(EXIT_SUCCESS);
}
