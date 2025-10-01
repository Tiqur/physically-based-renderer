#include "ShaderProgram.h"
#include <iostream>

ShaderProgram::ShaderProgram(Shader &&vertexShader, Shader &&fragmentShader)
    : m_vertexShader(std::move(vertexShader)), m_fragmentShader(std::move(fragmentShader)) {
    m_id = glCreateProgram();
    glAttachShader(m_id, vertexShader.id());
    glAttachShader(m_id, fragmentShader.id());
    glLinkProgram(m_id);
};

void ShaderProgram::use() { glUseProgram(m_id); }
ShaderProgram::~ShaderProgram() { glDeleteProgram(m_id); }
GLuint ShaderProgram::id() { return m_id; }
