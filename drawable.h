#pragma once

#include "utilities.h"
#include "shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


class Drawable {
public:
    virtual void setupBuffers() = 0;
    virtual void draw(glm::mat4 projection, glm::mat4 view, glm::mat4 model) = 0;
    virtual void deleteGLResources() = 0;

    Shader *shader;
};