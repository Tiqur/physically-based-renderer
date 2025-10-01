#include "Shader.h"
#include "ShaderProgram.h"
#include "VAO.h"
#include "VBO.h"
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
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
      gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
  )";
std::string fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    uniform vec4 uColor;

    void main() {
      FragColor = uColor;
    }
  )";

std::vector<float> vertices = {
    -1.0f, -1.0f, 0.0f, // ***
    -1.0f, 1.0f,  0.0f, // **
    1.0f,  1.0f,  0.0f, // *

    -1.0f, -1.0f, 0.0f, //   *
    1.0f,  1.0f,  0.0f, //  **
    1.0f,  -1.0f, 0.0f, // ***
                        //
    -2.0f, -2.0f, 0.0f, //   *
    2.0f,  2.0f,  0.0f, //  **
    2.0f,  -2.0f, 0.0f, // ***
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

// TODO refactor/move 
// Renderer Settings
static int threadCount = 1;

// Camera Settings
static float camYaw = 0.0f;
static float camPitch = 0.0f;
float rotationSpeed = 5.0f;

// Projection settings
static float fov = 45.0f;
static float nearPlane = 0.1f;
static float farPlane = 100.0f;

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
  glewExperimental = GL_TRUE;
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

  // Enable blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Main render loop
  while (!glfwWindowShouldClose(window)) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Render OpenGL
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    program.use();

    // Get uniform locations
    GLuint modelLoc = glGetUniformLocation(program.id(), "model");
    GLuint viewLoc = glGetUniformLocation(program.id(), "view");
    GLuint projectionLoc = glGetUniformLocation(program.id(), "projection");

    // Set Color
    GLint colorLoc = glGetUniformLocation(program.id(), "uColor");

    // Model matrix
    glm::mat4 model = glm::mat4(1.0f);

    // View matrix
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::rotate(view, glm::radians(-camYaw), glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw
    view = glm::rotate(view, glm::radians(-camPitch), glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -5.0f));

    // Projection matrix
    glm::mat4 projection = glm::perspective(glm::radians(fov), 800.0f / 600.0f, nearPlane, farPlane);

    glBindVertexArray(vao.id());

    // ImGui TODO
    ImGui::Begin("Scene Settings");
    ImGui::Button("Load/Select Scene");
    ImGui::Button("Start");
    ImGui::Button("Stop");
    ImGui::Button("Reset");
    ImGui::SliderInt("Thread Count", &threadCount, 1, 16);
    ImGui::End();

    ImGui::Begin("Camera");
    if (ImGui::Button("Left")) {
      camYaw -= rotationSpeed;
    }
    ImGui::SameLine();
    if (ImGui::Button("Right")) {
      camYaw += rotationSpeed;
    }
    if (ImGui::Button("Up")) {
      camPitch += rotationSpeed;
    }
    ImGui::SameLine();
    if (ImGui::Button("Down")) {
      camPitch -= rotationSpeed;
    }
    ImGui::SliderFloat("Rotation Speed", &rotationSpeed, 1.0f, 45.0f);
    ImGui::Text("Yaw:   %.2f", camYaw);
    ImGui::Text("Pitch: %.2f", camPitch);
    ImGui::SliderFloat("FOV", &fov, 10.0f, 120.0f);
    ImGui::SliderFloat("Near Plane", &nearPlane, 0.01f, 10.0f);
    ImGui::SliderFloat("Far Plane", &farPlane, 10.0f, 500.0f);
    ImGui::End();

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Raycast quad (fixed to camera)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glm::mat4 orthoProjection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 10.0f);
    glm::mat4 hudView = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(orthoProjection));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(hudView));
    glUniform4f(colorLoc, 0.0f, 1.0f, 1.0f, 0.0f);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Wireframe objects
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 0.4f);
    glDrawArrays(GL_TRIANGLES, 6, 9-6);

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
