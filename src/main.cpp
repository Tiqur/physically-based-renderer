#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

std::string vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 transform;
    void main() {
      gl_Position = transform*vec4(aPos.x, aPos.y, aPos.z, 1.0);
    }
  )";
std::string fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    void main() {
      FragColor = vec4(0.5f, 0.2f, 0.8f, 1.0f);
    }
  )";

std::vector<float> vertices = {
    -1.0f, -1.0f, 0.0f, // ***
    -1.0f, 1.0f,  0.0f, // **
    1.0f,  1.0f,  0.0f, // *

    -1.0f, -1.0f, 0.0f, //   *
    1.0f,  1.0f,  0.0f, //  **
    1.0f,  -1.0f, 0.0f, // ***
};

using std::cout, std::endl;
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  (void)window;
  glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

class Shader {
public:
  Shader(std::string *shaderSource, GLenum shaderType) {
    m_id = glCreateShader(shaderType);
    const char *p = shaderSource->c_str();
    glShaderSource(m_id, 1, &p, NULL);
    glCompileShader(m_id);
    checkErrors();
  }
  void checkErrors() {
    int success;
    char infoLog[512];
    glGetShaderiv(m_id, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(m_id, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::COMPILATION_FAILED\n"
                << infoLog << std::endl;
    };
  }
  ~Shader() { glDeleteShader(m_id); }
  GLuint id() { return m_id; }

private:
  GLuint m_id{};
};

class ShaderProgram {
public:
  ShaderProgram(Shader &&vertexShader, Shader &&fragmentShader)
      : m_vertexShader(std::move(vertexShader)),
        m_fragmentShader(std::move(fragmentShader)) {
    m_id = glCreateProgram();
    glAttachShader(m_id, vertexShader.id());
    glAttachShader(m_id, fragmentShader.id());
    glLinkProgram(m_id);
  }
  void use() { glUseProgram(m_id); }
  ~ShaderProgram() { glDeleteProgram(m_id); }
  GLuint id() { return m_id; }

private:
  GLuint m_id{};
  Shader m_vertexShader;
  Shader m_fragmentShader;
};

class VAO {
public:
  VAO() {
    glGenVertexArrays(1, &m_id);
    if (m_id == 0) {
      cout << "Failed to generate Vertex Array Object" << endl;
      return;
    }
  }
  ~VAO() { glDeleteVertexArrays(1, &m_id); }
  void setAttribPointer(GLuint index, GLuint size, GLenum type,
                        GLboolean normalized, GLsizei stride,
                        const void *pointer) {
    bind();
    glVertexAttribPointer(index, size, type, normalized, stride, pointer);
  }
  void bind() { glBindVertexArray(m_id); }
  void unbind() { glBindVertexArray(0); }
  GLuint id() { return m_id; }

private:
  GLuint m_id{};
};

class VBO {
public:
  VBO(const std::vector<float> *vertices) {
    glGenBuffers(1, &m_id);
    if (m_id == 0) {
      cout << "Failed to generate Vertex Buffer Object" << endl;
      return;
    }
    bind();
    glBufferData(GL_ARRAY_BUFFER, vertices->size() * sizeof(float),
                 vertices->data(), GL_STATIC_DRAW);
  }
  ~VBO() { glDeleteBuffers(1, &m_id); }
  void bind() { glBindBuffer(GL_ARRAY_BUFFER, m_id); }
  void unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }
  GLuint id() { return m_id; }

private:
  GLuint m_id{};
};

int main() {
  // Initialize ImGui
  std::cout << "Initializing ImGui..." << std::endl;
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  std::cout << "Initializing ImGui GLFW backend..." << std::endl;
  if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
    std::cerr << "Failed to initialize ImGui GLFW backend!" << std::endl;
    return -1;
  }

  std::cout << "Initializing ImGui OpenGL backend..." << std::endl;
  if (!ImGui_ImplOpenGL3_Init("#version 330")) {
    std::cerr << "Failed to initialize ImGui OpenGL backend!" << std::endl;
    return -1;
  }

  // Initialize GLEW
  glewExperimental = GL_TRUE; // Ensure GLEW uses modern OpenGL techniques
  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to initialize GLEW" << std::endl;
    return -1;
  }

  // Set the viewport
  glViewport(0, 0, 800, 600);

  // Register the framebuffer size callback
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  Shader vertexShader(&vertexShaderSource, GL_VERTEX_SHADER);
  Shader fragmentShader(&fragmentShaderSource, GL_FRAGMENT_SHADER);
  ShaderProgram program(std::move(vertexShader), std::move(fragmentShader));

  VBO vbo(&vertices);
  VAO vao;

  vao.bind();
  vao.setAttribPointer(0, 3, GL_FLOAT, false, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  static int threadCount = 1;

  // Main render loop
  while (!glfwWindowShouldClose(window)) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Render OpenGL
    glClearColor(0.2f, 0.4f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    program.use();

    GLuint transformLoc = glGetUniformLocation(program.id(), "transform");
    glm::mat4 trans = glm::mat4(1.0f);

    glBindVertexArray(vao.id());

    // ImGui TODO
    ImGui::Begin("Scene Settings");
    ImGui::Button("Load/Select Scene");
    ImGui::Button("Start");
    ImGui::Button("Stop");
    ImGui::Button("Reset");
    ImGui::SliderInt("Thread Count", &threadCount, 1, 16);
    ImGui::End();

    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Process user input
    processInput(window);

    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Clean up and exit
  glfwDestroyWindow(window);
  glfwTerminate();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  return 0;
}
