#include "Shader.h"
#include "ShaderProgram.h"
#include "VAO.h"
#include "VBO.h"
#include "Camera.h"
#include "Shape.h"
#include "Triangle.h"
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

Camera cam = Camera(); 

// TODO refactor/move 
// Renderer Settings
static int threadCount = 1;


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

  // --- SETUP WORLD OBJECTS ---
  std::vector<Shape*> worldObjects;
  
  for (int i=0; i<32; i++) {
    Triangle* triangle = new Triangle();
    triangle->position = glm::vec3(0.0f, 0, (float)(-i)*0.1);
    worldObjects.push_back(triangle);
  }
  
  // Setup VBO/VAO for each shape
  std::vector<VBO*> shapeVBOs;
  std::vector<VAO*> shapeVAOs;
  
  for (Shape* shape : worldObjects) {
    std::vector<float> shapeVerts = shape->getVertices();
    VBO* shapeVBO = new VBO(&shapeVerts);
    VAO* shapeVAO = new VAO();
    
    shapeVAO->bind();
    shapeVAO->setAttribPointer(0, 3, GL_FLOAT, false, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    
    shapeVBOs.push_back(shapeVBO);
    shapeVAOs.push_back(shapeVAO);
  }

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
    GLint colorLoc = glGetUniformLocation(program.id(), "uColor");

    // Model matrix
    glm::mat4 model = glm::mat4(1.0f);

    // Calculate camera forward, right, up vectors
    glm::vec3 forward;
    forward.x = sin(glm::radians(cam.getCamYaw())) * cos(glm::radians(cam.getCamPitch()));
    forward.y = -sin(glm::radians(cam.getCamPitch()));
    forward.z = -cos(glm::radians(cam.getCamYaw())) * cos(glm::radians(cam.getCamPitch()));
    forward = glm::normalize(forward);
    
    //glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    //glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    //glm::vec3 up = worldUp;

    // View matrix
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::rotate(view, glm::radians(-cam.getCamYaw()), glm::vec3(0.0f, 1.0f, 0.0f));
    view = glm::rotate(view, glm::radians(-cam.getCamPitch()), glm::vec3(1.0f, 0.0f, 0.0f));
    view = glm::translate(view, -cam.getCamPos());

    glm::mat4 projection = glm::perspective(glm::radians(cam.getFov()), 800.0f / 600.0f, cam.getNearPlane(), cam.getFarPlane());

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
    
    // Ghost mode toggle
    if (ImGui::Button(cam.getGhostMode() ? "Disable Ghost" : "Enable Ghost")) {
      cam.toggleGhostMode();
    }
    
    ImGui::End();

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (cam.getGhostMode()) {
      glm::mat4 quadModel = glm::mat4(1.0f);
      quadModel = glm::translate(quadModel, cam.getGhostPos());
      quadModel = glm::rotate(quadModel, glm::radians(cam.getGhostYaw()), glm::vec3(0.0f, 1.0f, 0.0f));
      quadModel = glm::rotate(quadModel, glm::radians(cam.getGhostPitch()), glm::vec3(1.0f, 0.0f, 0.0f));
      quadModel = glm::translate(quadModel, glm::vec3(0.0f, 0.0f, -1.0f));
      quadModel = glm::scale(quadModel, glm::vec3(1.0f, 1.0f, 1.0f));
      
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(quadModel));
      glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
      glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
      glUniform4f(colorLoc, 0.0f, 1.0f, 1.0f, 0.3f);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    } else {
      glm::mat4 orthoProjection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 10.0f);
      glm::mat4 hudView = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.0f));
      
      glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(orthoProjection));
      glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(hudView));
      glUniform4f(colorLoc, 0.0f, 1.0f, 1.0f, 0.3f);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // Restore perspective projection and view matrix for world objects
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    
    // Draw all world objects
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    for (size_t i = 0; i < worldObjects.size(); i++) {
      glm::mat4 shapeModel = worldObjects[i]->getModelMatrix();
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(shapeModel));
      glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
      glBindVertexArray(shapeVAOs[i]->id());
      glDrawArrays(GL_TRIANGLES, 0, worldObjects[i]->getVertices().size() / 3);
    }

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
  for (Shape* shape : worldObjects) {
    delete shape;
  }
  for (VBO* vbo : shapeVBOs) {
    delete vbo;
  }
  for (VAO* vao : shapeVAOs) {
    delete vao;
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  return 0;
}
