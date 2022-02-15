#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <vector>
#include <string>
#include <iostream>

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cerr << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int loadCubemap(std::vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cerr << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

// simple OBJ file loader modified from http://www.opengl-tutorial.org/beginners-tutorials/tutorial-7-model-loading/
bool loadObj (const char * path, std::vector < glm::vec3 > & out_vertices, std::vector < glm::ivec3 > & out_faces) {
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        fprintf(stderr, "Unable to open the file! \n");
        return false;
    }

    // read line by line until EOF
    while (true) {
        char lineHeader[128];
        int res = fscanf(file, "%s", lineHeader);
        if (res == EOF) {
            break;
        }

        if (strcmp(lineHeader, "v") == 0) {
            glm::vec3 vertex;
            fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
            out_vertices.push_back(vertex);
        }
        else if (strcmp(lineHeader, "f") == 0) {
            glm::ivec3 face;
            fscanf(file, "%d %d %d\n", &face.x, &face.y, &face.z);
            // adjust indexing
            face -= glm::ivec3(1, 1, 1);
            out_faces.push_back(face);
        }
    }
    return true;
}

// math from https://math.stackexchange.com/a/2686620
glm::vec4 computePlaneCoeffs(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
    glm::vec3 first_3 = glm::cross(b - a, c - a);
    float k = -glm::dot(first_3, a);
    return glm::vec4(first_3, k);
}

bool loadObj (const char* path, std::vector<glm::vec3> &out_vertices, std::vector<glm::ivec3> &out_faces, std::vector<glm::vec3> &out_normals) {
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        fprintf(stderr, "Unable to open the file! \n");
        return false;
    }

    bool normalsInFile = false;

    // read line by line until EOF
    while (true) {
        char lineHeader[128];
        int res = fscanf(file, "%s", lineHeader);
        if (res == EOF) {
            break;
        }

        if (strcmp(lineHeader, "v") == 0) {
            glm::vec3 vertex;
            fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
            out_vertices.push_back(vertex);
        }
        else if (strcmp(lineHeader, "f") == 0) {
            glm::ivec3 face;
            fscanf(file, "%d %d %d\n", &face.x, &face.y, &face.z);
            // adjust indexing
            face -= glm::ivec3(1, 1, 1);
            out_faces.push_back(face);
        }
        else if (strcmp(lineHeader, "vn") == 0) {
            normalsInFile = true;
            glm::vec3 normal;
            fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
            out_normals.push_back(normal);
        }
    }

    if (!normalsInFile) {
        // have to compute normals manually
        out_normals.resize(out_vertices.size(), glm::vec3(0.0f));
        for (size_t i = 0; i < out_faces.size(); i++) {
            glm::ivec3 face = out_faces[i];
            glm::vec3 normal = glm::cross(out_vertices[face[1]] - out_vertices[face[0]], 
                                        out_vertices[face[2]] - out_vertices[face[0]]);
            out_normals[face[0]] += normal;
            out_normals[face[1]] += normal;
            out_normals[face[2]] += normal;
        }
        for (size_t i = 0; i < out_normals.size(); i++) {
            out_normals[i] = glm::normalize(out_normals[i]);
        }
    }

    return true;
}
