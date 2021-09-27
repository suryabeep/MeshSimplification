#version 460 core

in vec3 vpoint;
out vec2 uv;

void main() {
    gl_Position = vec4(vpoint, 1.0);
    uv = gl_Position.xy * 0.5 + 0.5;
}