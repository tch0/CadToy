#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <Shader.h>

// represent the painting area
class Canvas
{
private:
    Shader m_BasicPureColorShader;
    glm::mat4 m_ModelMatrix;
    glm::mat4 m_ViewMatrix;
    glm::mat4 m_ProjMatrix;
public:
    Canvas();
    // compile opengl shader, register callbacks
    void init();
    // update cursor hover point / grid / matrices, etc
    void update();
    // draw entities
    void draw();
};
