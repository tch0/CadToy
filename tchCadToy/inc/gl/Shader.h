#pragma once
#include <string>
#include <source_location>

#include <glad/gl.h>
#include <glm/glm.hpp>

class Shader
{
private:
    GLuint m_Id;
public:
    Shader();
    Shader(const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader = "",
           const std::source_location& loc = std::source_location::current());
    Shader(const std::string& vertexShader, const std::string tessellationCtrlShader, const std::string& tessellationEvalShader,
           const std::string& fragmentShader, const std::string& geometryShader = "",
           const std::source_location& loc = std::source_location::current());
    Shader(const Shader& shader);
    Shader& operator=(const Shader& shader);
    void setShaderSource(const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader = "",
                         const std::source_location& loc = std::source_location::current());
    void setShaderSource(const std::string& vertexShader, const std::string tessellationCtrlShader, const std::string& tessellationEvalShader,
                         const std::string& fragmentShader, const std::string& geometryShader = "",
                         const std::source_location& loc = std::source_location::current());
    GLuint getShaderId() const;
    void use() const;
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, GLint value) const;
    void setUint(const std::string& name, GLuint value) const;
    void setFloat(const std::string& name, GLfloat value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec2(const std::string& name, GLfloat x, GLfloat y) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec3(const std::string& name, GLfloat x, GLfloat y, GLfloat z) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setVec4(const std::string& name, GLfloat x, GLfloat y, GLfloat z, GLfloat w) const;
    void setMat2(const std::string& name, const glm::mat2& mat) const;
    void setMat3(const std::string& name, const glm::mat3& mat) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;
};
