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

class Shape {
public:
  glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
  
  virtual ~Shape() = default;
  virtual std::vector<float> getVertices() const = 0;
  
  // Transform to world space
  glm::mat4 getModelMatrix() const {
    return glm::translate(glm::mat4(1.0f), position);
  }
};

class Triangle : public Shape {
public:
  glm::vec3 v0, v1, v2;
  
  Triangle(glm::vec3 vertex0 = glm::vec3(-0.5f, -0.5f, 0.0f),
           glm::vec3 vertex1 = glm::vec3(0.5f, -0.5f, 0.0f),
           glm::vec3 vertex2 = glm::vec3(0.0f, 0.5f, 0.0f))
    : v0(vertex0), v1(vertex1), v2(vertex2) {}
  
  std::vector<float> getVertices() const override {
    return {
      v0.x, v0.y, v0.z,
      v1.x, v1.y, v1.z,
      v2.x, v2.y, v2.z
    };
  }
};


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

class Camera {
  public:
    // Camera Settings
    glm::vec3 camPosition;
    float camYaw;
    float camPitch;
    float rotationSpeed;
    float moveSpeed;

    // Ghost mode settings
    bool ghostMode;
    glm::vec3 ghostQuadPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    float ghostQuadYaw = 0.0f;
    float ghostQuadPitch = 0.0f;
    glm::vec3 savedCamPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    float savedCamYaw = 0.0f;
    float savedCamPitch = 0.0f;

    // Projection settings
    float fov;
    float nearPlane;
    float farPlane;

    Camera() {
     camPosition = glm::vec3(0.0f, 0.0f, 0.0f);
     camYaw = 0.0f;
     camPitch = 0.0f;
     rotationSpeed = 5.0f;
     moveSpeed = 0.5f;

     ghostMode = false;
     ghostQuadPosition = glm::vec3(0.0f, 0.0f, 0.0f);
     ghostQuadYaw = 0.0f;
     ghostQuadPitch = 0.0f;
     savedCamPosition = glm::vec3(0.0f, 0.0f, 0.0f);
     savedCamYaw = 0.0f;
     savedCamPitch = 0.0f;

     fov = 45.0f;
     nearPlane = 0.1f;
     farPlane = 100.0f;
    }

    void toggleGhostMode() {
      if (!ghostMode) {
        // Entering ghost mode
        ghostQuadPosition = camPosition;
        ghostQuadYaw = camYaw;
        ghostQuadPitch = camPitch;
        
        savedCamPosition = camPosition;
        savedCamYaw = camYaw;
        savedCamPitch = camPitch;
        
        // Move camera behind quad
        camPosition = ghostQuadPosition + glm::vec3(0.0f, 0.0f, 1.0f);
      } else {
        // Exiting ghost mode - teleport back
        camPosition = ghostQuadPosition;
        camYaw = ghostQuadYaw;
        camPitch = ghostQuadPitch;
      }
      ghostMode = !ghostMode;
    }
};

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
    forward.x = sin(glm::radians(cam.camYaw)) * cos(glm::radians(cam.camPitch));
    forward.y = -sin(glm::radians(cam.camPitch));
    forward.z = -cos(glm::radians(cam.camYaw)) * cos(glm::radians(cam.camPitch));
    forward = glm::normalize(forward);
    
    //glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    //glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    //glm::vec3 up = worldUp;

    // View matrix
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::rotate(view, glm::radians(-cam.camYaw), glm::vec3(0.0f, 1.0f, 0.0f));
    view = glm::rotate(view, glm::radians(-cam.camPitch), glm::vec3(1.0f, 0.0f, 0.0f));
    view = glm::translate(view, -cam.camPosition);

    glm::mat4 projection = glm::perspective(glm::radians(cam.fov), 800.0f / 600.0f, cam.nearPlane, cam.farPlane);

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
    if (ImGui::Button(cam.ghostMode ? "Disable Ghost" : "Enable Ghost")) {
      cam.toggleGhostMode();
    }
    
    ImGui::End();

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (cam.ghostMode) {
      glm::mat4 quadModel = glm::mat4(1.0f);
      quadModel = glm::translate(quadModel, cam.ghostQuadPosition);
      quadModel = glm::rotate(quadModel, glm::radians(cam.ghostQuadYaw), glm::vec3(0.0f, 1.0f, 0.0f));
      quadModel = glm::rotate(quadModel, glm::radians(cam.ghostQuadPitch), glm::vec3(1.0f, 0.0f, 0.0f));
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
