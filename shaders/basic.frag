#version 460 core

layout(location = 0) in vec3 vertexPositionView;
layout(location = 1) in vec3 vertexNormalView;

out vec4 FragColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 lightPos;
uniform vec3 eyePos;

void main() {

    float shininess = 100;
    vec3 ambient = vec3(0.82, 0.93, 0.99);
    vec3 diffuse = vec3(1, 0, 0);
    vec3 specular = vec3(1, 1, 1);

    float ambientWeight = 0.1;

    vec3 camPos = vec3(0, 0, 0);
    vec3 lightVector = normalize(lightPos - vertexPositionView);
    vec3 reflectionVector = normalize(reflect(-lightVector, vertexNormalView));
    vec3 viewVector = normalize(camPos - vertexPositionView);

    vec3 halfway = normalize(lightVector + viewVector);
    float n_dot_h = max(dot(halfway, vertexNormalView), 0.0);
    float blinn_spec_weight = pow(n_dot_h, shininess);

    // Calculate diffuse light weighting: (n dot l)
    float diffuseWeight = max(dot(vertexNormalView, lightVector), 0.0);

    // Interpolate the computed vertex color for each fragment.
    FragColor = vec4((  ambient * ambientWeight
                          + diffuse * diffuseWeight
                          +specular * blinn_spec_weight), 1.0);
}
