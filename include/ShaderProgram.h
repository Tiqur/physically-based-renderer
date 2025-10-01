#pragma once

#include "Shader.h"
#include <GL/glew.h>

class ShaderProgram {
  public:
    ShaderProgram(Shader &&vertexShader, Shader &&fragmentShader);
    void use();
    ~ShaderProgram();
    GLuint id();

  private:
    GLuint m_id{};
    Shader m_vertexShader;
    Shader m_fragmentShader;
};
