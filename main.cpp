#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <functional>
#include <stdlib.h>
#include <stdio.h>
#include <vector>

#include <glm/glm.hpp>

#include <fstream>
#include <iostream>

#include "PerfFramework.h"
#include "PerfTimer.h"
#include "Voxels.h"

#include "DisplayLists.h"
#include "Vaos.h"
#include "GeometryShader.h"
#include "QuadGeom.h"
#include "Instanced.h"
#include "CompactDisplayLists.h"
#include "HybridInstanced.h"

using namespace glm;
using namespace std;

void Usage() {
    cerr << "VoxelPerf [dl|cdl|vao|gs|qgs|inst] <width> <height> <depth>" << endl;
}

int main(int argc, char** argv) {
    if (argc != 5) {
        Usage();
        exit(EXIT_FAILURE);
    }

    string testType = string(argv[1]);
    if (testType != "dl" && testType != "vao" && testType != "gs"
        && testType != "qgs" && testType != "inst" && testType != "cdl"
        && testType != "hyi") {
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

    bool runDlPerf = (testType == "dl");
    bool runCdlPerf = (testType == "cdl");
    bool runVaoPerf = (testType == "vao");
    bool runGsPerf = (testType == "gs");
    bool runQgsPerf = (testType == "qgs");
    bool runInstancedPerf = (testType == "inst");
    bool runHyInstancedPerf = (testType == "hyi");

    ivec3 voxelGrid(w, h, d);

    vec3 voxelSpacing(32, 32, 32);
    voxelSpacing *= VOXEL_SIZE;

    int numObjects = voxelGrid.x * voxelGrid.y * voxelGrid.z;

    PerfRecord record;

    // Test display list perf
    if (runDlPerf) {
        record = RunDisplayListsTest(sphere, voxelGrid, voxelSpacing);
    }

    // Test compact display list perf
    if (runCdlPerf) {
        record = RunCompactDisplayListsTest(sphere, voxelGrid, voxelSpacing);
    }

    // Test VAO perf
    if (runVaoPerf) {
        record = RunVaosTest(sphere, voxelGrid, voxelSpacing);
    }

    // Test geometry shader perf
    if (runGsPerf) {
        record = RunGeometryShaderTest(sphere, voxelGrid, voxelSpacing);
    }

    // Test quad-only geometry shader perf
    if (runQgsPerf) {
        record = RunQuadGeometryShaderTest(sphere, voxelGrid, voxelSpacing);
    }

    // Test quad-only geometry shader perf
    if (runInstancedPerf) {
        record = RunInstancedTest(sphere, voxelGrid, voxelSpacing);
    }

    // Test quad-only geometry shader perf
    if (runHyInstancedPerf) {
        record = RunHybridInstancedTest(sphere, voxelGrid, voxelSpacing);
    }

    cout << testType << ", " << numObjects << ", " << record.averageFrameTimeMs << ", " << record.gpuMemUsed << ", " << record.mainMemUsed << endl;
    exit(EXIT_SUCCESS);
}
