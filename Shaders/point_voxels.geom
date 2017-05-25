#version 410

layout(points) in;
layout(triangle_strip, max_vertices = 12) out;

flat in lowp vec3 gColor[];
flat in int gEnabledFaces[];

flat out lowp vec3 fColor;

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

bool IsCulled(vec4 normal) {
    return normal.z > 0;
}

void main() {
    vec4 center = gl_in[0].gl_Position;
    
    vec4 dx = mvp[0] / 2.0f * voxSize;
    vec4 dy = mvp[1] / 2.0f * voxSize;
    vec4 dz = mvp[2] / 2.0f * voxSize;

    if ((gEnabledFaces[0] & 0x01) != 0 && !IsCulled(dx))
        AddQuad(center + dx, dy, dz);
    
    if ((gEnabledFaces[0] & 0x02) != 0 && !IsCulled(-dx))
        AddQuad(center - dx, dz, dy);

    if ((gEnabledFaces[0] & 0x04) != 0 && !IsCulled(dy))
        AddQuad(center + dy, dz, dx);

    if ((gEnabledFaces[0] & 0x08) != 0 && !IsCulled(-dy))
        AddQuad(center - dy, dx, dz);

    if ((gEnabledFaces[0] & 0x10) != 0 && !IsCulled(dz))
        AddQuad(center + dz, dx, dy);

    if ((gEnabledFaces[0] & 0x20) != 0 && !IsCulled(-dz))
        AddQuad(center - dz, dy, dx);
}
