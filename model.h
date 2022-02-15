#pragma once

#include "utilities.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <unordered_map>
#include <map>
#include <queue>

#define DIM 256

class Model {
public:
    Model(const char* path) {
        loadObj(path, _vertices, _faces, _normals);

        fprintf(stderr, "Vertices size is %lu\n", _vertices.size());
        fprintf(stderr, "Normals size is %lu\n", _normals.size());
        fprintf(stderr, "Faces size is %lu\n", _faces.size());

        computeQEM();
    }

    void setupBuffers();
    void draw();
    void deleteGLResources();
    void collapseMesh();
    void computeQEM();
    void collapseMeshQEM();

private:
    std::vector<glm::vec3> _vertices;
    std::vector<glm::vec3> _normals;
    std::vector<glm::ivec3> _faces;

    // Quadric Error Metric simplification data structures
    std::unordered_multimap<int, int> _vertexFaceAdjacency;
    std::unordered_multimap<int, int> _edges;
    std::unordered_map<int, glm::mat4> _quadrics;
    std::multimap<float, std::pair<int, int>> _pairs;

    GLuint _vao;
    GLuint _vertexBuffer;
    GLuint _normalBuffer;
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

    // normal buffer
    glGenBuffers(1, &_normalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _normalBuffer);
    glBufferData(GL_ARRAY_BUFFER, _normals.size() * sizeof(_normals.at(0)), _normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(glm::vec3), (void *) 0);
    glEnableVertexAttribArray(1);

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

glm::mat4 computeKp(glm::vec4 plane) {
    return glm::outerProduct(plane, plane);
}

void Model::computeQEM() {
    _vertexFaceAdjacency.clear();
    _edges.clear();
    _quadrics.clear();
    _pairs.clear();

    // compute vertex to face adjacency and vertex-vertex adjacency
    // TODO: this can be moved into the file parsing function.
    for (int i = 0; i < _faces.size(); i++) {
        glm::ivec3 face = _faces[i];
        _vertexFaceAdjacency.emplace(face[0], i);
        _vertexFaceAdjacency.emplace(face[1], i);
        _vertexFaceAdjacency.emplace(face[2], i);

        _edges.emplace(face[0], face[1]);
        _edges.emplace(face[0], face[2]);
        _edges.emplace(face[1], face[2]);

    }

    for (auto kv : _vertexFaceAdjacency) {
        int vertexIndex = kv.first;
        int faceIndex = kv.second;

        glm::mat4 Kp = computeKp(computePlaneCoeffs(_vertices[_faces[faceIndex][0]], 
                                                    _vertices[_faces[faceIndex][1]], 
                                                    _vertices[_faces[faceIndex][2]]));
        
        auto quadric_it = _quadrics.find(vertexIndex);
        if ( quadric_it == _quadrics.end()) {
            _quadrics.emplace(vertexIndex, Kp);
        } else {
            _quadrics[vertexIndex] += Kp;
        }        
    }

    for (auto kv : _quadrics) {
        int vertexIndex = kv.first;
        glm::mat4 q = kv.second;
        kv.second = glm::outerProduct(glm::vec4(_vertices[vertexIndex], 1), q * glm::vec4(_vertices[vertexIndex], 1));
    }

    // for every edge, compute the error of the pair
    for (auto it = _edges.begin(); it != _edges.end(); it++) {
        int v1 = it->first;
        int v2 = it->second;
        glm::mat4 Q = _quadrics.at(v1) + _quadrics.at(v2);
        glm::vec3 midpoint = (_vertices[v1] + _vertices[v2]) / 2.0f;
        float error = glm::dot(glm::vec4(midpoint, 1.0f), Q * glm::vec4(midpoint, 1.0f));
        _pairs.emplace(error, std::make_pair(v1, v2));
    }
}

void Model::collapseMeshQEM() {
    if (_pairs.empty()) {
        return;
    }

    auto smallestEdge = _pairs.begin();
    int v1 = smallestEdge->second.first;
    int v2 = smallestEdge->second.second;

    size_t v1_count = _vertexFaceAdjacency.count(v1);
    size_t v2_count = _vertexFaceAdjacency.count(v2);

    int toKeep = v1_count > v2_count ? v2 : v1;
    int toRemove = v1_count > v2_count ? v1 : v2;

    // change all instances of the more-frequent vertex to the less-frequent vertex
    for (size_t i = 0; i < _faces.size(); i++) {
        for (int j = 0; j < 3; j++) {
            if (_faces[i][j] == toRemove) {
                _faces[i][j] = toKeep;
            }
        }
    }

    for (auto it = _faces.begin(); it != _faces.end(); ) {
        if (it->x == it->y || it->x == it->z || it->y == it->z) {
            it = _faces.erase(it);
        }
        else {
            it++;
        }
    }
    fprintf(stderr, "Collapsed mesh now has %lu vertices and %lu faces\n", _vertices.size(), _faces.size());

    // update GL buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _faceBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, _faces.size() * sizeof(_faces.at(0)), _faces.data(), GL_STATIC_DRAW);

    computeQEM();

    // TODO: actually update the QEM datastructure neatly instead of tossing and recomputing.

    // add edges from v1 to all new vertices that were originally connected to v2. Check if they already exist first though.

    // update the vertex-face adjacency map --> remove key v2 and add all the new faces to key v1

    // update quadrics multimap. Do this by removing key v2, then iterate through all faces adjacent to v1.

    // for each adjacent face, recalculate the error quadrics for every vertex in the face
    
    // then remove all key-value pairs in _pairs that use v2 i
}