#pragma once

#include <vector>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <Shader.h>

// represent the painting area
class Canvas
{
private:
    // todo: maybe draw it in the form of pixels directly to frame buffer instead of a OpenGL 3D object.
    class CustomCursor
    {
    private:
        std::vector<glm::vec3> m_Vertices;
        // sizes are in pixels
        int m_PickBoxSize {20};
        int m_CursorSize {80};
    public:
        CustomCursor();
        // range: 0 ~ 50
        void setPickBoxSize(int size);
        // range: 10 ~ 200
        void setCursorSize(int size);
        // get vertices array
        const std::vector<glm::vec3>& vertices();
        // update cursor datas
        void updateVertices();
    };
    class Grid
    {
    private:
        std::vector<glm::vec3> m_MainGridVertices;
        std::vector<glm::vec3> m_SubGridVertices;
    public:
        Grid();
        void updateVertices();
        const std::vector<glm::vec3>& mainGridVertices();
        const std::vector<glm::vec3>& subGridVertices();
    };
    class Axises
    {
    private:
        std::vector<glm::vec3> m_XAxisVertices;
        std::vector<glm::vec3> m_YAxisVertices;
        float m_Width {0.0f};
        float m_Height {0.0f};
    public:
        Axises();
        void updateVertices();
        const std::vector<glm::vec3>& xVertices();
        const std::vector<glm::vec3>& yVertices();
    };
private:
    Shader m_BasicPureColorShader;
    glm::mat4 m_ModelMatrix {0.0f};
    glm::mat4 m_ViewMatrix {0.0f};
    glm::mat4 m_ProjMatrix {0.0f};
    // cursor
    CustomCursor m_Cursor;
    GLuint m_CursorVao {0};
    GLuint m_CursorVbo {0};
    // grid
    Grid m_Grid;
    GLuint m_MainGridVao {0};
    GLuint m_MainGridVbo {0};
    GLuint m_SubGridVao {0};
    GLuint m_SubGridVbo {0};
    bool m_bGridUpdatedFirstTime {false};
    // axises
    Axises m_Axises;
    GLuint m_XAxisVao {0};
    GLuint m_XAxisVbo {0};
    GLuint m_YAxisVao {0};
    GLuint m_YAxisVbo {0};
public:
    Canvas();
    // compile opengl shader, register callbacks
    void init();
    // update cursor hover point / grid / matrices, etc
    void update();
    // draw entities
    void draw();
private:
    // generate vao, generate vbo, and bind data to vertex buffer
    void generateVaoVbo(GLuint& vao, GLuint& vbo, const std::vector<glm::vec3>& data);
    // bind new data to existing vertex buffer
    void updateVertexArrayBufferData(GLuint& vao, GLuint& vbo, const std::vector<glm::vec3>& data);
};
