#version 460 core

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;

layout (location = 0) out vec3 vertexPositionView;
layout (location = 1) out vec3 vertexNormalView;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 normalMatrix;

uniform vec3 lightPos;
uniform vec3 eyePos;

void main() {
	vertexPositionView = (view * model * vec4(vertexPosition, 1.0f)).xyz;
	vertexNormalView = normalize((normalMatrix * vec4(vertexNormal, 0.0f)).xyz);

	gl_Position = projection * view * model * vec4(vertexPosition, 1.0f);
}