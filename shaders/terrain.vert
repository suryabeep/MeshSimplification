#version 460 core

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec2 texCoord;

out vec3 heightMapColor;
out vec2 texcoord;
out float percent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float maxHeight;
uniform float minHeight;

uniform sampler2D heightmap;


void main() {
    texcoord = texCoord;

    float texHeight = texture(heightmap, texcoord).r ;
    vec3 pos = vec3(vertexPosition.x, texHeight, vertexPosition.z);
	gl_Position = projection * view * model * vec4(pos, 1.0f);

    percent = (pos.y - minHeight) / (maxHeight - minHeight);
    heightMapColor = texture(heightmap, texcoord).rgb;
}