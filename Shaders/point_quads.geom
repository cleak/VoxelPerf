#version 150

layout(points) in;
//layout(triangle_strip, max_vertices = 24) out;
layout(triangle_strip, max_vertices = 4) out;

in vec3 gColor[];
in vec4 gExtent1[];
in vec4 gExtent2[];

out vec3 fColor;

uniform float voxSize = 0.25;
uniform mat4 mvp;

void AddQuad(vec4 center, vec4 dy, vec4 dx) {
    fColor = gColor[0];
    gl_Position = center + (dx - dy);
    EmitVertex();

    fColor = gColor[0];
    gl_Position = center + (-dx - dy);
    EmitVertex();

    fColor = gColor[0];
    gl_Position = center + (dx + dy);
    EmitVertex();

    fColor = gColor[0];
    gl_Position = center + (-dx + dy);
    EmitVertex();

    //EndPrimitive();
}

void main() {
    vec4 center = gl_in[0].gl_Position;
    AddQuad(center, gExtent1[0], gExtent2[0]);
}
