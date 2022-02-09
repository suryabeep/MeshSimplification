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
        loadObj(path, _vertices, _faces);
        computeQEM();
        // fprintf(stderr, "loaded %lu vertices and %lu faces\n", _vertices.size(), _faces.size());
        auto it = _quadrics.begin();
        for (int i = 0; i < 4; i++) {
            std::cerr << it->first << ": " << std::endl;
            glm::mat4 q = it->second;
            for (int row = 0; row < 4; row++) {
                for (int col = 0; col < 4; col++) {
                    std::cerr << it->second[row][col] << " ";
                }
                std::cerr << std::endl;
            }

            it++;
        }
        // print edge errors
        auto edge_it = _pairs.begin();
        for (int i = 0; i < 4; i++) {
            if (edge_it == _pairs.end())
                break;
            // fprintf(stderr, "Error for vertices %d -- %d : %f\n", edge_it->second.first, edge_it->second.second, edge_it->first);
            edge_it++;
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    void setupBuffers();
    void draw();
    void deleteGLResources();
    void collapseMesh();
    void computeQEM();
    void collapseMeshQEM();

private:
    std::vector<glm::vec3> _vertices;
    std::vector<glm::ivec3> _faces;

    // Quadric Error Metric simplification data structures
    std::unordered_multimap<int, int> _vertexFaceAdjacency;
    std::unordered_multimap<int, int> _edges;
    std::unordered_map<int, glm::mat4> _quadrics;
    std::multimap<float, std::pair<int, int>> _pairs;

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

// math from https://math.stackexchange.com/a/2686620
glm::vec4 computePlaneCoeffs(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
    glm::vec3 first_3 = glm::cross(b - a, c - a);
    float k = -glm::dot(first_3, a);
    return glm::vec4(first_3, k);
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

    std::vector<int> toErase;

    if (v2_count > v1_count) {
        // get all faces that use v2
        auto v2_faces_range = _vertexFaceAdjacency.equal_range(v2);
        for (auto it = v2_faces_range.first; it != v2_faces_range.second; it++) {

            // change v2 in the face to v1
            if (_faces[it->second][0] == v2) _faces[it->second][0] = v1;
            if (_faces[it->second][1] == v2) _faces[it->second][1] = v1;
            if (_faces[it->second][2] == v2) _faces[it->second][2] = v1;

            // remove face if it has become degenerate
            if ( _faces[it->second][0] == _faces[it->second][1] 
            || _faces[it->second][1] == _faces[it->second][2] 
            || _faces[it->second][0] == _faces[it->second][2]) {
                toErase.push_back(it->second);
            } 
        }
        // change v1's coordinate to (v1 + v2) / 2.0
        _vertices[v1] = (_vertices[v1] + _vertices[v2]) / 2.0f;
    }
    else {
        // get all faces that use v1
        auto v1_faces_range = _vertexFaceAdjacency.equal_range(v1);
        for (auto it = v1_faces_range.first; it != v1_faces_range.second; it++) {

            // change v1 in the face to v2
            if (_faces[it->second][0] == v1) _faces[it->second][0] = v2;
            if (_faces[it->second][1] == v1) _faces[it->second][1] = v2;
            if (_faces[it->second][2] == v1) _faces[it->second][2] = v2;

            // remove face if it has become degenerate
            if ( _faces[it->second][0] == _faces[it->second][1] 
            || _faces[it->second][1] == _faces[it->second][2] 
            || _faces[it->second][0] == _faces[it->second][2]) {
                toErase.push_back(it->second);
            }
        }

        // change v2's coordinate to (v1 + v2) / 2.0
        _vertices[v2] = (_vertices[v1] + _vertices[v2]) / 2.0f;
        // fprintf(stderr, "v2 coords changed to: [ %f, %f, %f]\n", _vertices[v2][0], _vertices[v2][1], _vertices[v2][2]);

    }

    for (int i = 0; i < toErase.size(); i++) {
        // fprintf(stderr, "erasing item at index %d\n", toErase[i]);
        _faces.erase(_faces.begin() + toErase[i]);
    }

    computeQEM();

    // fprintf(stderr, "Collapsed mesh now has %lu vertices and %lu faces\n", _vertices.size(), _faces.size());

    // update GL buffers
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(_vertices.at(0)), _vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _faceBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, _faces.size() * sizeof(_faces.at(0)), _faces.data(), GL_STATIC_DRAW);


    // add edges from v1 to all new vertices that were originally connected to v2. Check if they already exist first though.

    // update the vertex-face adjacency map --> remove key v2 and add all the new faces to key v1

    // update quadrics multimap. Do this by removing key v2, then iterate through all faces adjacent to v1.

    // for each adjacent face, recalculate the error quadrics for every vertex in the face
    
    // then remove all key-value pairs in _pairs that use v2 i
}


//// BASIC EDGE COLLAPSE ALGORITHM BELOW

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

    // fprintf(stderr, "Vertex1 index = %d, occurrences = %d\nVertex2 index = %d, occurrences = %d\n", vertexIndex1, v1Occurrences, vertexIndex2, v2Occurrences);

    if (v1Occurrences > v2Occurrences) {
        // replace all occurrences of v1 with v2
        for (int i = 0; i < _faces.size(); i++) {
            if (_faces[i][0] == vertexIndex1) _faces[i][0] = vertexIndex2;
            if (_faces[i][1] == vertexIndex1) _faces[i][1] = vertexIndex2;
            if (_faces[i][2] == vertexIndex1) _faces[i][2] = vertexIndex2;
        }
        // move v2 to the midpoint
        _vertices[vertexIndex2] = (_vertices[vertexIndex1] + _vertices[vertexIndex2]) / 2.0f;
    }
    else {
        // replace all occurrences of v2 with v1
        for (int i = 0; i < _faces.size(); i++) {
            if (_faces[i][0] == vertexIndex2) _faces[i][0] = vertexIndex1;
            if (_faces[i][1] == vertexIndex2) _faces[i][1] = vertexIndex1;
            if (_faces[i][2] == vertexIndex2) _faces[i][2] = vertexIndex1;
        }
        // move v1 to the midpoint
        _vertices[vertexIndex1] = (_vertices[vertexIndex1] + _vertices[vertexIndex2]) / 2.0f;
    }

    // remove degenerate triangles
    for (int i = 0; i < _faces.size(); i++) {
        if (_faces[i][0] == _faces[i][1] || _faces[i][0] == _faces[i][2] || _faces[i][1] == _faces[i][2]) {
            _faces.erase(_faces.begin() + i);
        }
    }

    // fprintf(stderr, "Collapsed mesh now has %lu vertices and %lu faces\n", _vertices.size(), _faces.size());

    // update GL buffers
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(_vertices.at(0)), _vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _faceBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, _faces.size() * sizeof(_faces.at(0)), _faces.data(), GL_STATIC_DRAW);
}