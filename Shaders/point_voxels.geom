#version 150

layout(points) in;
layout(triangle_strip, max_vertices = 24) out;

in vec3 gColor[];
in int gEnabledFaces[];

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

    EndPrimitive();
}

void main() {
    vec4 center = gl_in[0].gl_Position;

    vec4 dx = mvp[0] / 2.0f * voxSize;
    vec4 dy = mvp[1] / 2.0f * voxSize;
    vec4 dz = mvp[2] / 2.0f * voxSize;

    if ((gEnabledFaces[0] & 0x01) != 0)
        AddQuad(center + dx, dy, dz);
    
    if ((gEnabledFaces[0] & 0x02) != 0)
        AddQuad(center - dx, dz, dy);

    if ((gEnabledFaces[0] & 0x04) != 0)
        AddQuad(center + dy, dz, dx);

    if ((gEnabledFaces[0] & 0x08) != 0)
        AddQuad(center - dy, dx, dz);

    if ((gEnabledFaces[0] & 0x10) != 0)
        AddQuad(center + dz, dx, dy);

    if ((gEnabledFaces[0] & 0x20) != 0)
        AddQuad(center - dz, dy, dx);
}
