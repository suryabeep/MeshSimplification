#pragma once

#include "utilities.h"
#include "drawable.h"
#include "convolution.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <time.h>
#include <random>     // mt19937 and uniform_int_distribution
#include <algorithm>  // generate
#include <iterator>   // begin, end, and ostream_iterator
#include <functional> // bind

#define DIM 256

class Terrain : public Drawable {
public:
    Terrain(Shader *shader, unsigned int texHandle, const int divisions, const float dimension) {
        _terrainShader = shader;
        _heightmapHandle = texHandle;
        
        _div = divisions;
        _dim = dimension;
        _minX = 0;
        _maxX = _dim;
        _minY = 0;
        _maxY = _dim;

        _heightValues.resize(DIM * DIM, 0.0f);
        //fillHeightWithNoise();
        // for (int i = 0; i < 10; i++) {
        //     std::cerr << _heightValues[i] << ", ";
        // }

        // std::cerr << std::endl;
        // convolveHeightmap();
        // for (int i = 0; i < 10; i++) {
        //     std::cerr << _heightValues[i] << ", ";
        // }
        // std::cerr << std::endl;

        _vertices = new std::vector<glm::vec3> ();
        _normals  = new std::vector<glm::vec3> ();
        _faces    = new std::vector<glm::ivec3>();
        _texCoords = new std::vector<glm::vec2>();

        generateTriangles();

        //fprintf(stderr, "maxheight = %f, minHeight = %f\n", _maxHeight, _minHeight);

        _waterTexHandle = loadTexture("resources/water.jpg");
        _grassTexHandle = loadTexture("resources/grass.jpg");
        _rockTexHandle = loadTexture("resources/rock.jpg");
        _snowTexHandle = loadTexture("resources/snow.jpg");

        setupTiles();

        // unsigned int textureID;
        // glGenTextures(1, &textureID);
        // glBindTexture(GL_TEXTURE_2D, textureID);
        // glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, DIM, DIM, 0, GL_RED, GL_UNSIGNED_BYTE, _heightValues.data());
        // glGenerateMipmap(GL_TEXTURE_2D);

        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // _heightmapHandle = textureID;
    }

    ~Terrain() {
        delete _vertices;
        delete _normals;
        delete _texCoords;
        delete _faces;
    }

    void setupBuffers();
    void draw(glm::mat4 projection, glm::mat4 view, glm::mat4 model);
    void deleteGLResources();
    float getMaxHeight();
    float getMinHeight();

    void updateTilePositions(glm::vec3 cameraPos);
    void addTile(const glm::vec2 &pos);
    std::vector<glm::vec2> getTiles() {return _tilePositions;}
    Shader *_terrainShader;

private:
    int _div;
    float _dim;
    float _maxX;
    float _minX;
    float _maxY;
    float _minY;
    float _minHeight = 100000;
    float _maxHeight = -100000;

    std::vector<float> _heightValues;

    size_t _numVertices;
    size_t _numFaces;

    std::vector<glm::vec3>  *_vertices;
    std::vector<glm::vec3>  *_normals;
    std::vector<glm::ivec3> *_faces;
    std::vector<glm::vec2>  *_texCoords;

    // each element is a vec2 representing the bottom-left corner of the tile
    std::vector<glm::vec2> _tilePositions;

    GLuint _vao;
    GLuint _vertexBuffer;
    GLuint _normalBuffer;
    GLuint _texCoordBuffer;
    GLuint _faceBuffer;
    GLuint _heightmapHandle;
    GLuint _waterTexHandle;
    GLuint _grassTexHandle;
    GLuint _rockTexHandle;
    GLuint _snowTexHandle;

    GLuint tempHeightHandle;

    float distanceToFaultPlane(glm::vec3 b, glm::vec3 p, glm::vec3 rand_normal);
    void generateTriangles();
    void shapeTerrain();
    void calculateNormals();

    void setupTiles();
    void fillHeightWithNoise();
    void convolveHeightmap();
};

void Terrain::generateTriangles() {
    float deltaX = (_maxX - _minX) / _div;
    float deltaY = (_maxY - _minY) / _div;

    fprintf(stderr, "deltaX: %f, deltaY: %f \n", deltaX, deltaY);

    for (size_t i = 0; i <= _div; ++i) {
        for (size_t j = 0; j <= _div; ++j) {
            _vertices->push_back(glm::vec3(_minX + deltaX * j, 0, _minY + deltaY * i));
            _normals->push_back(glm::ivec3(0));
            _texCoords->push_back(glm::vec2((float)i / (float)_div, (float)j / (float)_div));
        }
    }

    for (size_t i = 0; i <= _div - 1; ++i) {
        for (size_t j = 0; j <= _div - 1; ++j) {
            int vertex_num = i * (_div + 1) + j;
            _faces->push_back(glm::ivec3(vertex_num, vertex_num + 1, vertex_num + _div + 1));
            _faces->push_back(glm::ivec3(vertex_num + 1, vertex_num + _div + 2, vertex_num + _div + 1));
        }
    }
}

void Terrain::setupBuffers() {
    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    // vertex buffer
    glGenBuffers(1, &_vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, _vertices->size() * sizeof(_vertices->at(0)), _vertices->data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(glm::vec3), (void *) 0);
    glEnableVertexAttribArray(0);

    // normal buffer
    glGenBuffers(1, &_normalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _normalBuffer);
    glBufferData(GL_ARRAY_BUFFER, _normals->size() * sizeof(_normals->at(0)), _normals->data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(glm::vec3), (void *) 0);
    glEnableVertexAttribArray(1);

    // texCoord buffer
    glGenBuffers(1, &_texCoordBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _texCoordBuffer);
    glBufferData(GL_ARRAY_BUFFER, _texCoords->size() * sizeof(_texCoords->at(0)), _texCoords->data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(glm::vec2), (void *) 0);
    glEnableVertexAttribArray(2);

    // vertex index buffer for drawing faces
    glGenBuffers(1, &_faceBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _faceBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, _faces->size() * sizeof(_faces->at(0)), _faces->data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Terrain::deleteGLResources() {
    glDeleteBuffers(1, &_vertexBuffer);
    glDeleteBuffers(1, &_normalBuffer);
    glDeleteBuffers(1, &_texCoordBuffer);
    glDeleteBuffers(1, &_faceBuffer);
    glDeleteVertexArrays(1, &_vao);
}

void Terrain::draw(glm::mat4 projection, glm::mat4 view, glm::mat4 model) {
    _terrainShader->use();
    _terrainShader->setMat4("projection", projection);
    _terrainShader->setMat4("view", view);
    _terrainShader->setFloat("maxHeight", _maxHeight);
    _terrainShader->setFloat("minHeight", _minHeight);
    _terrainShader->setInt("heightmap", 0);
    _terrainShader->setInt("waterTexture", 1);
    _terrainShader->setInt("grassTexture", 2);
    _terrainShader->setInt("rockTexture", 3);
    _terrainShader->setInt("snowTexture", 4);

    static bool first = true;
    if (first) {
        fprintf(stderr, "heightmap handle is: %d\n", _heightmapHandle);
        first = false;
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _heightmapHandle);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _waterTexHandle);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _grassTexHandle);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, _rockTexHandle);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, _snowTexHandle);

    glBindVertexArray(_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _faceBuffer);

    for (size_t i = 0; i < _tilePositions.size(); ++i) {
        glm::mat4 model = glm::mat4(1);
        model = glm::translate(model, glm::vec3(_tilePositions[i][0], 0, _tilePositions[i][1]));
        _terrainShader->setMat4("model", model);
        glDrawElements(GL_TRIANGLES, _faces->size() * 3, GL_UNSIGNED_INT, (void *) 0);
    }
}


float Terrain::getMaxHeight() {
    return _maxHeight;
}

float Terrain::getMinHeight() {
    return _minHeight;
}

void Terrain::setupTiles() {
    for (int i = -_dim * 1.5; i <= _dim / 2; i+= _dim) {
        for (int j = -_dim * 1.5; j <= _dim / 2; j+= _dim) {
            _tilePositions.push_back(glm::vec2(i, j));
        }
    }
}

void Terrain::addTile(const glm::vec2 &pos) {
    _tilePositions.push_back(pos);
}

void Terrain::updateTilePositions(glm::vec3 cameraPos) {
    for (size_t i = 0; i < _tilePositions.size(); i++) {
        glm::vec2 camPos = glm::vec2(cameraPos.x, cameraPos.z);
        glm::vec2 tilePos = _tilePositions[i];
        glm::vec2 tileCenter = tilePos + glm::vec2(_dim / 2);
        glm::vec2 difference = camPos - tileCenter;

        if (abs(difference.x) >= 1.5 * _dim) {
            tilePos.x += 3 * _dim * glm::sign(difference.x);
        }
        if (abs(difference.y) >= 1.5 * _dim) {
            tilePos.y += 3 * _dim * glm::sign(difference.y);
        }
        
        _tilePositions[i] = tilePos;
    }
}

void Terrain::convolveHeightmap() {
    HeightmapGenerator generator(_heightValues.data());
    generator.run();
    float* convolved = generator.get();
    std::vector<float> temp = std::vector<float> {convolved, convolved + DIM * DIM};
    _heightValues.assign(temp.begin(), temp.end());
    _minHeight = generator.getMinHeight();
    _maxHeight = generator.getMaxHeight();
}