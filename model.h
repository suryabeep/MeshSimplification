#pragma once

#include "utilities.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>

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
    void collapseMesh();

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

bool compare(const std::vector<float> &a, const std::vector<float> &b) {
    return a[0] < b[0];
}

// basic edge-collapse taken from https://github.com/inessadl/mesh-simplification/blob/master/sources/meshsimplification.cpp
void Model::collapseMesh() {
    if (_faces.size() <= 1) {
        return;
    }
    std::vector<std::vector<float>> edges;
    std::vector<float> x;

    for (int i = 0; i < _faces.size(); i++) {
        float edge1 = glm::distance(_vertices[_faces[i][0]], _vertices[_faces[i][1]]);
        edges.push_back(x);
        edges[3*i].push_back(edge1);
        edges[3*i].push_back((float) _faces[i][0]);
        edges[3*i].push_back((float) _faces[i][1]);

        float edge2 = glm::distance(_vertices[_faces[i][0]], _vertices[_faces[i][2]]);
        edges.push_back(x);
        edges[3*i + 1].push_back(edge2);
        edges[3*i + 1].push_back((float) _faces[i][0]);
        edges[3*i + 1].push_back((float) _faces[i][2]);

        float edge3 = glm::distance(_vertices[_faces[i][1]], _vertices[_faces[i][2]]);
        edges.push_back(x);
        edges[3*i + 2].push_back(edge3);
        edges[3*i + 2].push_back((float) _faces[i][1]);
        edges[3*i + 2].push_back((float) _faces[i][2]);
    }

    std::sort(edges.begin(), edges.end(), compare);

    int vertexIndex1 = (int) edges[0][1];
    int vertexIndex2 = (int) edges[0][2];
    int v1Occurrences = 0;
    int v2Occurrences = 0;

    for (int i = 0; i < _faces.size(); i++) {
        if (_faces[i][0] == vertexIndex1) v1Occurrences++;
        if (_faces[i][1] == vertexIndex1) v1Occurrences++;
        if (_faces[i][2] == vertexIndex1) v1Occurrences++;

        if (_faces[i][0] == vertexIndex2) v2Occurrences++;
        if (_faces[i][1] == vertexIndex2) v2Occurrences++;
        if (_faces[i][2] == vertexIndex2) v2Occurrences++;
    }

    fprintf(stderr, "Vertex1 index = %d, occurrences = %d\nVertex2 index = %d, occurrences = %d\n", vertexIndex1, v1Occurrences, vertexIndex2, v2Occurrences);

    if (v1Occurrences > v2Occurrences) {
        // replace all occurrences of v1 with v2
        for (int i = 0; i < _faces.size(); i++) {
            if (_faces[i][0] == vertexIndex1) _faces[i][0] = vertexIndex2;
            if (_faces[i][1] == vertexIndex1) _faces[i][1] = vertexIndex2;
            if (_faces[i][2] == vertexIndex1) _faces[i][2] = vertexIndex2;
        }
    }
    else {
        // replace all occurrences of v2 with v1
        for (int i = 0; i < _faces.size(); i++) {
            if (_faces[i][0] == vertexIndex2) _faces[i][0] = vertexIndex1;
            if (_faces[i][1] == vertexIndex2) _faces[i][1] = vertexIndex1;
            if (_faces[i][2] == vertexIndex2) _faces[i][2] = vertexIndex1;
        }
    }

    // remove degenerate triangles
    for (int i = 0; i < _faces.size(); i++) {
        if (_faces[i][0] == _faces[i][1] || _faces[i][0] == _faces[i][2] || _faces[i][1] == _faces[i][2]) {
            _faces.erase(_faces.begin() + i);
        }
    }

    fprintf(stderr, "Collapsed mesh now has %lu vertices and %lu faces\n", _vertices.size(), _faces.size());

    // update GL buffers
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(_vertices.at(0)), _vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _faceBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, _faces.size() * sizeof(_faces.at(0)), _faces.data(), GL_STATIC_DRAW);
}