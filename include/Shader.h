#pragma once

#include <GL/glew.h>
#include <string>

class Shader {
  public:
	Shader(std::string* shaderSource, GLenum shaderType);
	void checkErrors();
	~Shader();
	GLuint id();

  private:
	GLuint m_id{};
};
