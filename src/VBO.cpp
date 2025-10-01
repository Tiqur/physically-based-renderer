#include "VBO.h"
#include <iostream>

using std::cout, std::endl;

VBO::VBO(const std::vector<float> *vertices) {
  glGenBuffers(1, &m_id);
  if (m_id == 0) {
    cout << "Failed to generate Vertex Buffer Object" << endl;
    return;
  }
  bind();
  glBufferData(GL_ARRAY_BUFFER, vertices->size() * sizeof(float),
               vertices->data(), GL_STATIC_DRAW);
}

VBO::~VBO() { glDeleteBuffers(1, &m_id); }

void VBO::bind() { glBindBuffer(GL_ARRAY_BUFFER, m_id); }

void VBO::unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }

GLuint VBO::id() { return m_id; }
