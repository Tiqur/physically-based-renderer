#pragma once

#include <GL/glew.h>

class VAO {
public:
  VAO();
  ~VAO();
  void setAttribPointer(GLuint index, GLuint size, GLenum type,
                        GLboolean normalized, GLsizei stride,
                        const void *pointer);
  void bind();
  void unbind();
  GLuint id();

private:
  GLuint m_id{};
};
