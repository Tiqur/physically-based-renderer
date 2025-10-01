#pragma once

#include <GL/glew.h>
#include <vector>

class VBO {
  public:
    VBO(const std::vector<float> *vertices);
    ~VBO();
    void bind();
    void unbind();
    GLuint id();

  private:
    GLuint m_id{};
};
