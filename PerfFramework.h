#pragma once

#include <functional>
#include <string>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

#pragma pack(push, 1)
struct PackedVec {
    int x : 10;
    int y : 10;
    int z : 10;
    int w : 2;
};

struct PackedColor {
    unsigned int r : 10;
    unsigned int g : 10;
    unsigned int b : 10;
    unsigned int a : 2;
};
#pragma pack(pop)

PackedVec PackVec3(glm::vec3 v);
PackedVec PackVec4(glm::vec4 v);
PackedColor PackColor(glm::vec3 color);

// How many frames to discard before recording starts
#define FRAMES_TO_DISCARD 32

// How many frames to record and average for a single sample
#define FRAMES_TO_RECORD 128

#define ROTATE false
// Record of a single performance sample
struct PerfRecord {
    size_t  gpuMemUsed;
    size_t  mainMemUsed;
    double averageFrameTimeMs;
};

// Gets the amount of free VRAM for nvidia cards
size_t GetFreeMemNvidia();

// Gets the amount of main memory usage
// NOTE: Right now this includes VRAM used, so the amount of used VRAM must be subtracted off
size_t GetMainMemUsage();

// Runs a performance test with the given setup and draw functions
PerfRecord RunPerf(std::function<void()> setupFn, std::function<void()> drawFn,
                   std::function<void()> teardownFn);

void CheckGLErrors();

glm::vec3 CameraPosition();

// View/projection/model matrix methods
glm::mat4 MakeModelView();
glm::mat4 MakeProjection();
glm::mat4 MakeMvp();

void PrintMatrix(glm::mat4 matrix);

// Compiles a shader program from the given list of shader files
GLuint MakeShaderProgram(std::vector<std::pair<std::string, GLenum>> shaders);

// Reads in the entire contents of the specified text file
std::string ReadTextFile(std::string filename);