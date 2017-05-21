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

using namespace glm;
using namespace std;

void Usage() {
    cerr << "VoxelPerf [dl|vao] <width> <height> <depth>" << endl;
}

int main(int argc, char** argv) {
    if (argc != 5) {
        Usage();
        exit(EXIT_FAILURE);
    }

    string testType = string(argv[1]);
    if (testType != "dl" && testType != "vao") {
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

    int discardFrames = 32;
    int recordFrames = 128;

    VoxelSet sphere({ 32,32,32 });
    sphere.MakeSphere();

    bool runDlPerf = (testType == "dl");
    bool runVaoPerf = (testType == "vao");

    ivec3 voxelGrid(w, h, d);

    vec3 voxelSpacing(32, 32, 32);
    voxelSpacing *= VOXEL_SIZE;

    int numObjects = voxelGrid.x * voxelGrid.y * voxelGrid.z;

    PerfRecord record;

    // Test display list perf
    if (runDlPerf) {
        record = RunDrawListsTest(sphere, voxelGrid, voxelSpacing);
    }

    // Test VAO perf
    if (runVaoPerf) {
        record = RunVaosTest(sphere, voxelGrid, voxelSpacing);
    }

    cout << testType << ", " << numObjects << ", " << record.averageFrameTimeMs << ", " << record.gpuMemUsed << ", " << record.mainMemUsed << endl;
    exit(EXIT_SUCCESS);
}