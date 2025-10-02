#include "Shader.h"
#include "ShaderProgram.h"
#include "VAO.h"
#include "VBO.h"
#include "Camera.h"
#include "Shape.h"
#include "Triangle.h"
#include "Square.h"
#include "Cube.h"
#include "Ray.h"
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

using std::cout, std::endl;

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

Camera cam = Camera(); 

// Mouse state
static bool isDragging = false;
static double lastMouseX = 0.0;
static double lastMouseY = 0.0;
static float mouseSensitivity = 0.1f;

// Delta time
static float deltaTime = 0.0f;
static float lastFrame = 0.0f;

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  (void)window;
  glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return;
    }

    float currentFov = cam.getFov();
    currentFov -= (float)yoffset * 2.0f;
    cam.setFov(currentFov);
}


void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
  ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
  ImGuiIO& io = ImGui::GetIO();
  
  bool shouldDrag = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !io.WantCaptureMouse;
  
  if (shouldDrag) {
    if (!isDragging) {
      isDragging = true;
      lastMouseX = xpos;
      lastMouseY = ypos;
    } else {
      // Continue dragging
      double xoffset = xpos - lastMouseX;
      double yoffset = lastMouseY - ypos;
      
      lastMouseX = xpos;
      lastMouseY = ypos;
      
      cam.setCamYaw(cam.getCamYaw() + xoffset * mouseSensitivity);
      cam.setCamPitch(cam.getCamPitch() + yoffset * mouseSensitivity);
      
      if (cam.getCamPitch() > 89.0f) cam.setCamPitch(89.0f);
      if (cam.getCamPitch() < -89.0f) cam.setCamPitch(-89.0f);
    }
  } else {
    isDragging = false;
  }
}

void processInput(GLFWwindow *window, float deltaTime, const glm::vec3& forward, const glm::vec3& right, const glm::vec3& up) {
  ImGuiIO& io = ImGui::GetIO();
  
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
  
  if (io.WantCaptureKeyboard) {
    return;
  }
  
  float velocity = cam.getMoveSpeed() * deltaTime * 10.0f;
  
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    cam.setCamPos(cam.getCamPos() + forward * velocity);
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    cam.setCamPos(cam.getCamPos() - forward * velocity);
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    cam.setCamPos(cam.getCamPos() - right * velocity);
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    cam.setCamPos(cam.getCamPos() + right * velocity);
  }
  
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    cam.setCamPos(cam.getCamPos() + up * velocity);
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
      glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
    cam.setCamPos(cam.getCamPos() - up * velocity);
  }
}

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

  // Register callbacks
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);

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

  // --- SETUP RAYS ---
  std::vector<Ray*> rays;
  for (int i=0; i<=20; i++) {
    Ray* ray = new Ray(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.1f*i, 1.0f));
    rays.push_back(ray);
  }

  // Setup VBO/VAO for each ray
  std::vector<VBO*> rayVBOs;
  std::vector<VAO*> rayVAOs;

  float rayLength = 10.0f;

  for (Ray* ray : rays) {
      glm::vec3 p0 = ray->origin;
      glm::vec3 p1 = ray->at(rayLength);

      std::vector<float> vertices = {
          p0.x, p0.y, p0.z,
          p1.x, p1.y, p1.z
      };

      VBO* rayVBO = new VBO(&vertices);
      VAO* rayVAO = new VAO();

      rayVAO->bind();
      rayVBO->bind();
      rayVAO->setAttribPointer(0, 3, GL_FLOAT, false, 3 * sizeof(float), (void*)0);
      glEnableVertexAttribArray(0);

      rayVBOs.push_back(rayVBO);
      rayVAOs.push_back(rayVAO);
  }

  // --- SETUP WORLD OBJECTS ---
  std::vector<Shape*> worldObjects;
  

  Cube* cube = new Cube();
  cube->position = glm::vec3(0.0f, 0.0f, -10.0f);
  worldObjects.push_back(cube);

  Square* square = new Square();
  square->position = glm::vec3(0.0f, 0.0f, -5.0f);
  worldObjects.push_back(square);

  for (int i=0; i<32; i++) {
    Triangle* triangle = new Triangle();
    triangle->position = glm::vec3(0.0f, 0.0f, (float)(-i)*0.1);
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

    // Calculate delta time
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

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
    forward.x = cos(glm::radians(cam.getCamYaw())) * cos(glm::radians(cam.getCamPitch()));
    forward.y = sin(glm::radians(cam.getCamPitch()));
    forward.z = sin(glm::radians(cam.getCamYaw())) * cos(glm::radians(cam.getCamPitch()));
    forward = glm::normalize(forward);

    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    // View matrix
    glm::mat4 view = glm::lookAt(cam.getCamPos(), cam.getCamPos() + forward, up);

    // Projection matrix
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
      glm::vec3 ghostForward = cam.getGhostForward();

      glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
      glm::vec3 ghostRight = glm::normalize(glm::cross(ghostForward, worldUp));
      glm::vec3 ghostUp = glm::normalize(glm::cross(ghostRight, ghostForward));

      glm::mat4 ghostViewMatrix = glm::lookAt(cam.getGhostPos(), cam.getGhostPos() + ghostForward, ghostUp);
      glm::mat4 quadModel = glm::inverse(ghostViewMatrix);
 
      glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(quadModel));
      glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
      glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    } else {
      glm::mat4 orthoProjection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 10.0f);
      glm::mat4 hudView = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.0f));
      
      glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(orthoProjection));
      glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(hudView));
    }
    glUniform4f(colorLoc, 0.0f, 1.0f, 1.0f, 0.3f);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Restore perspective projection and view matrix for world objects
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // Draw all rays
    for (size_t i = 0; i < rays.size(); i++) {
        glm::mat4 identity = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(identity));
        glUniform4f(colorLoc, 1.0f, 0.0f, 0.0f, 1.0f);

        rayVAOs[i]->bind();
        glDrawArrays(GL_LINES, 0, 2);
    }
    
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
    processInput(window, deltaTime, forward, right, up);

    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Clean up and exit
  for (Shape* shape : worldObjects) {
    delete shape;
  }
  for (Ray* ray : rays) {
    delete ray;
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
