#pragma once

#include "utilities.h"
#include "drawable.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Skybox : public Drawable{
public:
    Skybox(std::vector<std::string> faces, Shader *shader) {
        loadVertices();
        _cubemapHandle = loadCubemap(faces);
        _skyboxShader = shader;
    }

    void setupBuffers();
    void draw(glm::mat4 projection, glm::mat4 view, glm::mat4 model);
    void deleteGLResources();

    Shader *_skyboxShader;

private:
    std::vector<glm::vec3> _vertices;
    GLuint _vao;
    GLuint _vertexBufferHandle;
    GLuint _cubemapHandle;

    void loadVertices();
};


void Skybox::loadVertices() {
    // from https://stackoverflow.com/questions/28375338/cube-using-single-gl-triangle-strip
    float cube_strip[] = {
        -1.f, 1.f, 1.f,     // Front-top-left
        1.f, 1.f, 1.f,      // Front-top-right
        -1.f, -1.f, 1.f,    // Front-bottom-left
        1.f, -1.f, 1.f,     // Front-bottom-right
        1.f, -1.f, -1.f,    // Back-bottom-right
        1.f, 1.f, 1.f,      // Front-top-right
        1.f, 1.f, -1.f,     // Back-top-right
        -1.f, 1.f, 1.f,     // Front-top-left
        -1.f, 1.f, -1.f,    // Back-top-left
        -1.f, -1.f, 1.f,    // Front-bottom-left
        -1.f, -1.f, -1.f,   // Back-bottom-left
        1.f, -1.f, -1.f,    // Back-bottom-right
        -1.f, 1.f, -1.f,    // Back-top-left
        1.f, 1.f, -1.f      // Back-top-right
    };

    for (int i = 0; i < 14; ++i) {
        _vertices.push_back(glm::vec3(cube_strip[3 * i], cube_strip[3 * i + 1], cube_strip[3 * i + 2]));
    }
}

void Skybox::setupBuffers() {
    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    glGenBuffers(1, &_vertexBufferHandle);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferHandle);
    glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(glm::vec3), _vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(glm::vec3), (void *) 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Skybox::draw(glm::mat4 projection, glm::mat4 view, glm::mat4 model) {
    glDepthMask(GL_FALSE);
    _skyboxShader->use();
    _skyboxShader->setMat4("projection", projection);
    _skyboxShader->setMat4("view", glm::mat4(glm::mat3(view)));
    _skyboxShader->setMat4("model", model);
    glBindVertexArray(_vao);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _cubemapHandle);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferHandle);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
    glDepthMask(GL_TRUE);
}

void Skybox::deleteGLResources() {
    glDeleteTextures(1, &_cubemapHandle);
    glDeleteBuffers(1, &_vertexBufferHandle);
    glDeleteVertexArrays(1, &_vao);
}