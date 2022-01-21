#pragma once

#include "utilities.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#define DIM 256

class Model {
public:
    Model(const char* path) {
        loadObj(path, _vertices, _faces);
        fprintf(stderr, "loaded %lu vertices and %lu faces\n", _vertices.size(), _faces.size());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    void setupBuffers();
    void draw();
    void deleteGLResources();

private:
    std::vector<glm::vec3> _vertices;
    std::vector<glm::ivec3> _faces;

    GLuint _vao;
    GLuint _vertexBuffer;
    GLuint _faceBuffer;
};

void Model::setupBuffers() {
    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    // vertex buffer
    glGenBuffers(1, &_vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(_vertices.at(0)), _vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(glm::vec3), (void *) 0);
    glEnableVertexAttribArray(0);

    // vertex index buffer for drawing faces
    glGenBuffers(1, &_faceBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _faceBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, _faces.size() * sizeof(_faces.at(0)), _faces.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Model::deleteGLResources() {
    glDeleteBuffers(1, &_vertexBuffer);
    glDeleteBuffers(1, &_faceBuffer);
    glDeleteVertexArrays(1, &_vao);
}

void Model::draw() {
    glBindVertexArray(_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _faceBuffer);
    glDrawElements(GL_TRIANGLES, _faces.size() * 3, GL_UNSIGNED_INT, (void *) 0);
}