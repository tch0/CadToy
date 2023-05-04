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
    public:
        std::vector<glm::vec3> m_Vertices;
        std::vector<glm::vec4> m_Colors;
        // sizes are in pixels
        int m_PickBoxSize {20};
        int m_CursorSize {80};
    public:
        CustomCursor();
        void setPickBoxSize(int size);  // range: 0 ~ 50
        void setCursorSize(int size);   // range: 10 ~ 200
        const std::vector<glm::vec3>& vertices();
        const std::vector<glm::vec4>& colors();
        void updateVertices();
    };
    class Grid
    {
    private:
        std::vector<glm::vec3> m_MainGridVertices;
        std::vector<glm::vec3> m_SubGridVertices;

        std::vector<glm::vec3> m_Vertices; // include main grid and subgrid
        std::vector<glm::vec4> m_Colors;
    public:
        Grid();
        void updateVertices();
        const std::vector<glm::vec3>& vertices();
        const std::vector<glm::vec4>& colors();
    };
    class Axises
    {
    private:
        std::vector<glm::vec3> m_XAxisVertices;
        std::vector<glm::vec3> m_YAxisVertices;
        std::vector<glm::vec3> m_Vertices;
        std::vector<glm::vec4> m_Colors;
        float m_Width {0.0f};
        float m_Height {0.0f};
    public:
        Axises();
        void updateVertices();
        const std::vector<glm::vec3>& vertices();
        const std::vector<glm::vec4>& colors();
    };
private:
    Shader m_BasicPureColorShader;
    glm::mat4 m_ModelMatrix {0.0f};
    glm::mat4 m_ViewMatrix {0.0f};
    glm::mat4 m_ProjMatrix {0.0f};
    // cursor
    CustomCursor m_Cursor;
    GLuint m_CursorVao {0};
    GLuint m_CursorPosVbo {0};
    GLuint m_CursorColorVbo {0};
    // grid
    Grid m_Grid;
    GLuint m_GridVao {0};
    GLuint m_GridPosVbo {0};
    GLuint m_GridColorVbo {0};
    bool m_bGridUpdatedFirstTime {false};
    // axises
    Axises m_Axises;
    GLuint m_AxisesVao {0};
    GLuint m_AxisesPosVbo {0};
    GLuint m_AxisesColorVbo {0};
public:
    Canvas();
    // compile opengl shader, register callbacks
    void init();
    // update cursor hover point / grid / matrices, etc
    void update();
    // draw entities
    void draw();
    // cursor size and pick box size
    int getCursorSize();
    void setCursorSize(int size);
    int getPickBoxSize();
    void setPickBoxSize(int size);
private:
    // generate vao, generate vbo, and bind data to vertex buffer
    void generateVaoVbo(GLuint& vao, GLuint& posVbo, GLuint& colorVbo, const std::vector<glm::vec3>& vertices, const std::vector<glm::vec4>& colors);
    // bind new data to existing vertex buffer
    void updateVertexArrayBufferData(GLuint& vao, GLuint& posVbo, GLuint& colorVbo, const std::vector<glm::vec3>& vertices, const std::vector<glm::vec4>& colors);
};
