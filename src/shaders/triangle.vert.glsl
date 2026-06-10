#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

layout(location = 0) out vec4 color;

void main()
{
    gl_Position = vec4(position + vec3(0.0f, 0.0f, 0.5f), 1.0f);

    color = vec4(normal * 0.5f + vec3(0.5f), 1.0f);
}
