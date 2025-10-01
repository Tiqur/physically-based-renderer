#include "VAO.h"
#include <iostream>

using std::cout, std::endl;

VAO::VAO() {
  glGenVertexArrays(1, &m_id);
  if (m_id == 0) {
    cout << "Failed to generate Vertex Array Object" << endl;
    return;
  }
}

VAO::~VAO() { glDeleteVertexArrays(1, &m_id); }

void VAO::setAttribPointer(GLuint index, GLuint size, GLenum type,
                           GLboolean normalized, GLsizei stride,
                           const void *pointer) {
  bind();
  glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

void VAO::bind() { glBindVertexArray(m_id); }

void VAO::unbind() { glBindVertexArray(0); }

GLuint VAO::id() { return m_id; }
