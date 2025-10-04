#include "Shader.h"
#include <iostream>

Shader::Shader(std::string* shaderSource, GLenum shaderType) {
	m_id = glCreateShader(shaderType);
	const char* p = shaderSource->c_str();
	glShaderSource(m_id, 1, &p, NULL);
	glCompileShader(m_id);
	checkErrors();
}

void Shader::checkErrors() {
	int success;
	char infoLog[512];
	glGetShaderiv(m_id, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(m_id, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::COMPILATION_FAILED\n"
		          << infoLog << std::endl;
	};
}

Shader::~Shader() { glDeleteShader(m_id); }

GLuint Shader::id() { return m_id; }
