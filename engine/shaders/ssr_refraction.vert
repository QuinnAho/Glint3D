#version 330 core
// WebGL2: auto-translated

layout(location = 0) in vec3 aPos;

void main()
{
    // Fullscreen pass will use a screen-space triangle/quad; placeholder position passthrough
    gl_Position = vec4(aPos, 1.0);
}

