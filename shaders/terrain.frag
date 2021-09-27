#version 460 core

in vec3 heightMapColor;
in vec2 texcoord;
in float percent;

out vec4 FragColor;

uniform sampler2D waterTexture;
uniform sampler2D grassTexture;
uniform sampler2D rockTexture;
uniform sampler2D snowTexture;

void main() {
    vec4 color;
    if (percent < 0.2) {
        color = texture(waterTexture, texcoord);
    } 
    else if (0.2 < percent && percent < 0.3) {
        color = mix(texture(waterTexture, texcoord), texture(grassTexture, texcoord), (percent - 0.3) / (0.3 - 0.2));
    }
    else if (percent < 0.5) {
        color = texture(grassTexture, texcoord);
    }
    else if (0.5 < percent && percent < 0.6) {
        color = mix(texture(grassTexture, texcoord), texture(rockTexture, texcoord), (percent - 0.5) / (0.6 - 0.5));
    }
    else if (percent < 0.8) {
        color = texture(rockTexture, texcoord);
    }
    else if (0.8 < percent && percent < 0.9) {
        color = mix(texture(rockTexture, texcoord), texture(snowTexture, texcoord), (percent - 0.8) / (0.9 - 0.8));
    }
    else {
        color = texture(snowTexture, texcoord);
    }
    FragColor = vec4(heightMapColor, 1);
}
