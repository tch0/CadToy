#include <glm/ext.hpp>

#include "gl/Shader.h"
#include "gl/GLFuncs.h"


Shader::Shader() : m_Id(0)
{
}
Shader::Shader(const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader, const std::source_location& loc)
{
    m_Id = createShaderProgramFromSource(vertexShader, fragmentShader, geometryShader, loc);
}
Shader::Shader(const std::string& vertexShader, const std::string tessellationCtrlShader, const std::string& tessellationEvalShader,
               const std::string& fragmentShader, const std::string& geometryShader,
               const std::source_location& loc)
{
    m_Id = createShaderProgramFromSource(vertexShader, tessellationCtrlShader, tessellationEvalShader, fragmentShader, geometryShader, loc);
}

Shader::Shader(const Shader& shader) : m_Id(shader.m_Id)
{
}
Shader& Shader::operator=(const Shader& shader)
{
    m_Id = shader.m_Id;
    return *this;
}
void Shader::setShaderSource(const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader, const std::source_location& loc)
{
    *this = Shader(vertexShader, fragmentShader, geometryShader, loc);
}
void Shader::setShaderSource(const std::string& vertexShader, const std::string tessellationCtrlShader, const std::string& tessellationEvalShader,
                     const std::string& fragmentShader, const std::string& geometryShader,
                     const std::source_location& loc)
{
    *this = Shader(vertexShader, tessellationCtrlShader, tessellationEvalShader, fragmentShader, geometryShader, loc);
}

GLuint Shader::getShaderId() const
{
    return m_Id;
}

void Shader::use() const
{
    glUseProgram(m_Id);
}

void Shader::setBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(m_Id, name.c_str()), GLint(value));
}
void Shader::setInt(const std::string& name, GLint value) const
{
    glUniform1i(glGetUniformLocation(m_Id, name.c_str()), value);
}
void Shader::setUint(const std::string& name, GLuint value) const
{
    glUniform1ui(glGetUniformLocation(m_Id, name.c_str()), GLint(value));
}
void Shader::setFloat(const std::string& name, GLfloat value) const
{
    glUniform1f(glGetUniformLocation(m_Id, name.c_str()), value);
}
void Shader::setVec2(const std::string& name, const glm::vec2& value) const
{
    glUniform2fv(glGetUniformLocation(m_Id, name.c_str()), 1, glm::value_ptr(value));
}
void Shader::setVec2(const std::string& name, GLfloat x, GLfloat y) const
{
    glUniform2f(glGetUniformLocation(m_Id, name.c_str()), x, y);
}
void Shader::setVec3(const std::string& name, const glm::vec3& value) const
{
    glUniform3fv(glGetUniformLocation(m_Id, name.c_str()), 1, glm::value_ptr(value));
}
void Shader::setVec3(const std::string& name, GLfloat x, GLfloat y, GLfloat z) const
{
    glUniform3f(glGetUniformLocation(m_Id, name.c_str()), x, y, z);
}
void Shader::setVec4(const std::string& name, const glm::vec4& value) const
{
    glUniform4fv(glGetUniformLocation(m_Id, name.c_str()), 1, glm::value_ptr(value));
}
void Shader::setVec4(const std::string& name, GLfloat x, GLfloat y, GLfloat z, GLfloat w) const
{
    glUniform4f(glGetUniformLocation(m_Id, name.c_str()), x, y, z, w);
}
void Shader::setMat2(const std::string& name, const glm::mat2& mat) const
{
    glUniformMatrix2fv(glGetUniformLocation(m_Id, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}
void Shader::setMat3(const std::string& name, const glm::mat3& mat) const
{
    glUniformMatrix3fv(glGetUniformLocation(m_Id, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}
void Shader::setMat4(const std::string& name, const glm::mat4& mat) const
{
    glUniformMatrix4fv(glGetUniformLocation(m_Id, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}