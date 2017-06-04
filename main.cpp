#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <functional>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
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
#include "LayerMarchingCompressed.h"
#include "SdfShape.h"
#include "SdfJump.h"
#include "SdfJumpSphere.h"

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
	{ "sdfj", RunSdfJumpTest },
	{ "sdfjs", RunSdfJumpSphereTest },
	{ "lm", RunLayerMarchingTest },
	{ "lmc", RunLayerMarchingCompressedTest },
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
	cerr << "VoxelPerf [-t] <" << JoinFirst(tests, "|") << "> <width> <height> <depth>" << endl;
}

extern std::map<string, string> g_commandLineOptions;

int main(int argc, char** argv) {
	//map<string, string> options;
	vector<string> arguments;

	for (int i = 0; i < argc; ++i) {
		string arg = argv[i];
		if (arg.size() > 0 && arg[0] == '-') {
			if (arg.find('=') != string::npos) {
				size_t splitIdx = arg.find('=');
				string key = arg.substr(1, splitIdx - 1);
				string val = arg.substr(splitIdx + 1, arg.size() - splitIdx - 1);
				g_commandLineOptions.insert({ key, val });
			} else {
				g_commandLineOptions.insert({ arg.substr(1, arg.size() - 1), "" });
			}
		} else {
			arguments.push_back(arg);
		}
	}

    if (arguments.size() != 5) {
        Usage();
        exit(EXIT_FAILURE);
    }

    string testType = arguments[1];
    if (!tests.count(testType)) {
        Usage();
        exit(EXIT_FAILURE);
    }

    int w = atoi(arguments[2].c_str());
    int h = atoi(arguments[3].c_str());
    int d = atoi(arguments[4].c_str());

    if (w <= 0 || h <= 0 || d <= 0) {
        Usage();
        exit(EXIT_FAILURE);
    }

    VoxelSet model({ 32,32,32 });

	if (IsOptionSet("t")) {
		// Make a trivial example
		model.At({31, 31, 31}) = vec4(0.8f, 0.6f, 0.2f, 1);
	} else {
		model.MakeSphere();
	}

    ivec3 voxelGrid(w, h, d);

    vec3 voxelSpacing(32, 32, 32);
    voxelSpacing *= VOXEL_SIZE;

    int numObjects = voxelGrid.x * voxelGrid.y * voxelGrid.z;

    PerfRecord record = tests[testType](model, voxelGrid, voxelSpacing);

    cout << testType << ", " << numObjects << ", " << record.averageFrameTimeMs << ", " << record.gpuMemUsed << ", " << record.mainMemUsed << endl;
    exit(EXIT_SUCCESS);
}
