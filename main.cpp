
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <fstream>
#include <iostream>

#include "PerfTimer.h"

using namespace glm;
using namespace std;

static const struct {
    float x, y;
    float r, g, b;
} vertices[3] =
{
    { -0.6f, -0.4f, 1.f, 0.f, 0.f },
    { 0.6f, -0.4f, 0.f, 1.f, 0.f },
    { 0.f,  0.6f, 0.f, 0.f, 1.f }
};
static const char* vertex_shader_text =
"uniform mat4 MVP;\n"
"attribute vec3 vColor;\n"
"attribute vec2 vPos;\n"
"varying vec3 color;\n"
"void main()\n"
"{\n"
"    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
"    color = vColor;\n"
"}\n";
static const char* fragment_shader_text =
"varying vec3 color;\n"
"void main()\n"
"{\n"
"    gl_FragColor = vec4(color, 1.0);\n"
"}\n";

GLFWwindow* window;

void CheckGLErrors() {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        cerr << "OpenGL error: " << err << endl;
    }
}

mat4 MakeModelView() {
    mat4 view = lookAt(vec3(-16, 0, -16), vec3(0, 0, 0), vec3(0, 1, 0));
        //* glm::rotate((float)glfwGetTime(), vec3(0, 1, 0));
    return view;
}

mat4 MakeProjection() {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    float fovY = radians(45.0f);
    mat4 projection = perspective(fovY, (float)width / (float)height, 1.0f, 10000.0f);
    return projection;
}

mat4 MakeMvp() {
    return MakeProjection() * MakeModelView();
}

struct VoxelSet {
    VoxelSet(ivec3 size) {
        int elemCount = size.x * size.y * size.z;
        this->size = size;
        colors.resize(elemCount);
        for (int i = 0; i < elemCount; ++i) {
            colors[i] = vec4(0, 0, 0, 0);
        }
    }

    ivec3 size;
    vector<vec4> colors;

    inline bool IsValid(ivec3 idx) {
        return idx.x >= 0 && idx.y >= 0 && idx.z >= 0
            && idx.x < size.x && idx.y < size.y && idx.z < size.z;
    }

    inline bool IsSolid(ivec3 idx) {
        return IsValid(idx) && At(idx).a > 0.1f;
    }

    inline vec4& At(ivec3 idx) {
        return colors[idx.z * (size.x * size.y) + idx.y * size.x + idx.x];
    }
};

GLuint MakeShader() {
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vertex_shader_text, NULL);
    glCompileShader(vShader);

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fragment_shader_text, NULL);
    glCompileShader(fShader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vShader);
    glAttachShader(program, fShader);
    glLinkProgram(program);

    return program;
}

float voxelSize = 0.25f;

void MakeSphere(VoxelSet& voxels) {
    float radius = voxels.size.x / 2.0f - 0.5f;
    vec3 color = 1.0f / (vec3(voxels.size) - vec3(1, 1, 1));
    vec3 center = vec3(voxels.size) / 2.0f;

    for (int z = 0; z < voxels.size.z; ++z) {
        for (int y = 0; y < voxels.size.y; ++y) {
            for (int x = 0; x < voxels.size.x; ++x) {

                ivec3 idx(x, y, z);

                vec3 delta = (vec3(idx) + vec3(0.5f, 0.5f, 0.5f)) - center;

                if ((glm::dot(delta, delta) - 0.1f) <= radius * radius) {
                    // Inside sphere
                    voxels.At(idx) = vec4(color * vec3(idx), 1.0f);
                } else {
                    // Outside sphere
                    voxels.At(idx) = vec4(0, 0, 0, 0);
                }
            }
        }
    }
}

void DrawFace(vec3 center, vec3 normal) {
    // Center of voxel to center of face
    center += vec3(normal) * voxelSize / 2.0f;

    vec3 d1 = -vec3(normal.z, normal.x, normal.y);
    vec3 d2 = -vec3(normal.y, normal.z, normal.x);

    vector<vec2> weights = {
        { -1, -1 },
        {  1, -1 },
        {  1,  1 },
        { -1,  1 },
    };

    // Reverse winding order for negative normals
    if (normal.x < 0 || normal.y < 0 || normal.z < 0) {
        std::swap(d1, d2);
    }

    for (int i = 0; i < 4; ++i) {
        vec2 w = weights[i];

        //ivec3 idxOffset1 = d1 * w.x;
        //ivec3 idxOffset2 = d2 * w.y;
        //vec3 position = center + (vec3(idxOffset1 + idxOffset2) / 2.0f) * voxelSize;
        vec3 position = center + (d1 * w.x + d2 * w.y) / 2.0f * voxelSize;

        glVertex3fv((float*)&position);
    }
}

void DrawVoxel(VoxelSet& voxels, vec3 offset, ivec3 idx) {
    if (!voxels.IsSolid(idx)) {
        return;
    }

    vec3 color = voxels.At(idx);
    glColor3fv((float*)&color);
    vec3 center = offset + vec3(idx) * voxelSize + vec3(voxelSize, voxelSize, voxelSize) / 2.0f;

    vector<ivec3> normals = {
        { 1, 0, 0 },
        {-1, 0, 0 },
        { 0, 1, 0 },
        { 0,-1, 0 },
        { 0, 0, 1 },
        { 0, 0,-1 },
    };

    for (auto& n : normals) {
        if (voxels.IsSolid(idx + n)) {
            continue;
        }
        DrawFace(center, vec3(n));
    }
}

GLuint VoxelsToDisplayList(VoxelSet& voxels, vec3 offset) {
    GLuint displayIdx = glGenLists(1);
    glNewList(displayIdx, GL_COMPILE);
    
    glBegin(GL_QUADS);
    for (int z = 0; z < voxels.size.z; ++z) {
        for (int y = 0; y < voxels.size.y; ++y) {
            for (int x = 0; x < voxels.size.x; ++x) {
                DrawVoxel(voxels, offset, ivec3(x, y, z));
            }
        }
    }
    glEnd();
    glEndList();

    return displayIdx;
}

void MakeDisplayListGrid(VoxelSet& model, ivec3 dimensions, vec3 spacing, std::vector<GLuint>& displayLists) {
    displayLists.resize(dimensions.x * dimensions.y * dimensions.z);
    int nextListIdx = 0;

    for (int z = 0; z < dimensions.z; ++z) {
        for (int y = 0; y < dimensions.y; ++y) {
            for (int x = 0; x < dimensions.x; ++x) {
                ivec3 idx(x, y, z);
                vec3 offset = vec3(idx) * spacing;
                offset -= vec3(0, dimensions.y, 0) * spacing / 2.0f;
                displayLists[nextListIdx] = VoxelsToDisplayList(model, offset);
                nextListIdx++;
            }
        }
    }
}

int main(void) {
    // Number of frames to delay before recording perf
    int frameDelay = 32;
    int samples = 1024;

    ivec3 voxelGrid(32, 8, 32);
    vec3 voxelSpacing(32, 32, 32);
    voxelSpacing *= voxelSize;

    GLuint vertex_buffer;
    GLint mvp_location, vpos_location, vcol_location;
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    window = glfwCreateWindow(1920, 1080, "Voxel Perf", NULL, NULL);

    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glewInit();
    glfwSwapInterval(1);

    GLuint program = MakeShader();
    GLint mvpLoc = glGetUniformLocation(program, "MVP");

    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    CheckGLErrors();

    GLint vPosLoc = glGetAttribLocation(program, "vPos");
    glEnableVertexAttribArray(vPosLoc);
    glVertexAttribPointer(vPosLoc, 2, GL_FLOAT, GL_FALSE,
                          sizeof(float) * 5, (void*)0);
    CheckGLErrors();

    GLint vColorPos = glGetAttribLocation(program, "vColor");
    glEnableVertexAttribArray(vColorPos);
    glVertexAttribPointer(vColorPos, 3, GL_FLOAT, GL_FALSE,
                          sizeof(float) * 5, (void*)(sizeof(float) * 2));
    CheckGLErrors();

    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    VoxelSet sphere({ 32,32,32 });
    MakeSphere(sphere);
    CheckGLErrors();
    GLuint displayList = VoxelsToDisplayList(sphere, vec3(0, -sphere.size.y * voxelSize / 2.0f, 0));
    CheckGLErrors();

    std::vector<GLuint> displayLists;
    MakeDisplayListGrid(sphere, voxelGrid, voxelSpacing, displayLists);

    ofstream dlResults("DisplayListResults.txt");

    PerfTimer timer;
    timer.Start();
    while (!glfwWindowShouldClose(window)) {
        double frameTime = timer.Stop();
        timer.Start();
        if (frameDelay == 0) {
            if (samples > 0) {
                dlResults << frameTime << endl;
                samples--;
                if (samples == 0) {
                    dlResults.close();
                }
            }
        } else {
            frameDelay--;
        }

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        float t = (float)(cos(glfwGetTime()) + 1) / 2;
        glClearColor(0.1f, 0.12f, t * 0.8f + 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mat4 mvp = MakeMvp();
        mat4 mv = MakeModelView();
        mat4 p = MakeProjection();
        glUseProgram(program);
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, (const GLfloat*)&mvp);

        //glDrawArrays(GL_TRIANGLES, 0, 3);

        glUseProgram(0);
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf((float*)&p);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf((float*)&mv);

        //glCallList(displayList);
        //CheckGLErrors();

        for (auto& list : displayLists) {
            glCallList(list);
        }
        CheckGLErrors();

        

        glFlush();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    dlResults.close();

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}