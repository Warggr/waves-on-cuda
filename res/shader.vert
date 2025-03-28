#version 420 core
layout (location = 0) in vec3 aPos;

layout(binding = 0) uniform Projection {
    mat4 model, view, projection;
} projection;

void main()
{
   gl_Position = projection.projection * projection.view * projection.model * vec4(aPos, 1.0);
}
