#include <iostream>

// Window.h `#include`s ImGui, GLFW, and glad in correct order.
#include "589-689-skeleton/Window.h"

#include "589-689-skeleton/GLDebug.h"
#include "589-689-skeleton/Geometry.h"
#include "589-689-skeleton/Log.h"
#include "589-689-skeleton/Shader.h"
#include "589-689-skeleton/ShaderProgram.h"
#include "bubbles.hpp"

// CALLBACKS
class MyCallbacks : public CallbackInterface {

public:
  // Constructor. We use values of -1 for attributes that, at the start of
  // the program, have no meaningful/"true" value.
  MyCallbacks(ShaderProgram &shader, int screenWidth, int screenHeight)
      : shader(shader), currentFrame(0), leftMouseActiveVal(false),
        lastLeftPressedFrame(-1), lastRightPressedFrame(-1), screenMouseX(-1.0),
        screenMouseY(-1.0), screenWidth(screenWidth),
        screenHeight(screenHeight) {}

  virtual void keyCallback(int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
      shader.recompile();
    }
  }

  virtual void mouseButtonCallback(int button, int action, int mods) {
    // If we click the mouse on the ImGui window, we don't want to log that
    // here. But if we RELEASE the mouse over the window, we do want to
    // know that!
    auto &io = ImGui::GetIO();
    if (io.WantCaptureMouse && action == GLFW_PRESS)
      return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
      leftMouseActiveVal = true;
      lastLeftPressedFrame = currentFrame;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
      lastRightPressedFrame = currentFrame;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
      leftMouseActiveVal = false;
    }
  }

  // Updates the screen width and height, in screen coordinates
  // (not necessarily the same as pixels)
  virtual void windowSizeCallback(int width, int height) {
    screenWidth = width;
    screenHeight = height;
  }

  // Sets the new cursor position, in screen coordinates
  virtual void cursorPosCallback(double xpos, double ypos) {
    screenMouseX = xpos;
    screenMouseY = ypos;
  }

  // Whether the left mouse was pressed down this frame.
  bool leftMouseJustPressed() { return lastLeftPressedFrame == currentFrame; }

  // Whether the left mouse button is being pressed down at all.
  bool leftMouseActive() { return leftMouseActiveVal; }

  // Whether the right mouse button was pressed down this frame.
  bool rightMouseJustPressed() { return lastRightPressedFrame == currentFrame; }

  // Tell the callbacks object a new frame has begun.
  void incrementFrameCount() { currentFrame++; }

  // Converts the cursor position from screen coordinates to GL coordinates
  // and returns the result.
  glm::vec2 getCursorPosGL() {
    glm::vec2 screenPos(screenMouseX, screenMouseY);
    // Interpret click as at centre of pixel.
    glm::vec2 centredPos = screenPos + glm::vec2(0.5f, 0.5f);
    // Scale cursor position to [0, 1] range.
    glm::vec2 scaledToZeroOne =
        centredPos / glm::vec2(screenWidth, screenHeight);

    glm::vec2 flippedY = glm::vec2(scaledToZeroOne.x, 1.0f - scaledToZeroOne.y);

    // Go from [0, 1] range to [-1, 1] range.
    return 2.f * flippedY - glm::vec2(1.f, 1.f);
  }

private:
  int screenWidth;
  int screenHeight;

  double screenMouseX;
  double screenMouseY;

  int currentFrame;

  bool leftMouseActiveVal;

  int lastLeftPressedFrame;
  int lastRightPressedFrame;

  ShaderProgram &shader;

  // Converts GL coordinates to screen coordinates.
  glm::vec2 glPosToScreenCoords(glm::vec2 glPos) {
    // Convert the [-1, 1] range to [0, 1]
    glm::vec2 scaledZeroOne = 0.5f * (glPos + glm::vec2(1.f, 1.f));

    glm::vec2 flippedY = glm::vec2(scaledZeroOne.x, 1.0f - scaledZeroOne.y);
    glm::vec2 screenPos = flippedY * glm::vec2(screenWidth, screenHeight);
    return screenPos;
  }
};

int main() {
  Log::debug("Starting main");

  Simulation::Params params;
  int x_max = 100;
  int y_max = 100;
  params.bubbles = {Coord(50, 50), Coord(20, 20), Coord(30, 30), Coord(70, 10)};
  params.radii = {10.0, 15.0, 10, 10};
  params.colors = {0, 1, 2, 4, 4};
  params.pressures = {.5, .7, 1, 1};
  params.targets = {0, 3000, 1000, 2000, 1000};
  Simulation sim(x_max, y_max, params);

  // WINDOW
  glfwInit();
  Window window(
      800, 800,
      "CPSC 589/689"); // could set callbacks at construction if desired

  GLDebug::enable();

  // SHADERS
  ShaderProgram shader("shaders/test.vert", "shaders/test.frag");

  auto cb = std::make_shared<MyCallbacks>(shader, window.getWidth(),
                                          window.getHeight());
  // CALLBACKS
  window.setCallbacks(cb);

  window.setupImGui(); // Make sure this call comes AFTER GLFW callbacks set.

  // GEOMETRY
  CPU_Geometry cpuGeom;
  GPU_Geometry gpuGeom;

  std::vector<glm::vec3> color_map = {glm::vec3(0.f, 0.f, 1.f),
                                      glm::vec3(1.f, 0.f, 0.f),
                                  	  glm::vec3(0.f, 1.f, 0.f),
                                  	  glm::vec3(.7f, 0.3, .45f),
                                  	  glm::vec3(0.1f, 0.1f, 0.4f)
                                  	 };
  float scale = .005;
  float DX = 0;
  float DY = 0;

  // Variables for the commented-out widgets.
  /*
  int sampleInt = 0;
  float sampleFloat = 0.f;
  float sampleDragFloat = 0.f;
  float sampleAngle = 0.f;
  float sampleFloatPair[2] = { 1.f, 2.f };
  */

  bool go = false;
  // RENDER LOOP
  while (!window.shouldClose()) {

    // Tell callbacks object a new frame's begun BEFORE polling events!
    cb->incrementFrameCount();
    glfwPollEvents();

    bool change = false; // Whether any ImGui variable's changed.

    // Three functions that must be called each new frame.
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Sample window.");

    ImGui::Text("Sample text.");

    if (!go) {
    	if(ImGui::Button("Go")) go = true;
  	} else {
  		if (ImGui::Button("Stop"))
  			go = false;
  	}

    ImGui::Text("Average %.1f ms/frame (%.1f fps)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    // if(ImGui::Button("Run Sim Iteration"))
     if(go) sim.run_sim_iteration();

    // Commented out ImGui widgets from tutorial video.
    // Check ImGui samples/GitHub/etc. for more cool options, like graphs!
    /*
    change |= ImGui::InputInt("sample int", &sampleInt, 2);
    change |= ImGui::InputFloat("sample float", &sampleFloat, 0.5f);
    change |= ImGui::DragFloat("sample dragger:", &sampleDragFloat, 1.f, 0.f,
    100.f); bool changeAngle = false; change |= changeAngle =
    ImGui::SliderAngle("sample angle", &sampleAngle); change |=
    ImGui::SliderFloat2("sample float pair", (float*)&sampleFloatPair, -15, 15);
    if (change) {
            std::cout << "Change detected! " << std::endl;
            if (changeAngle) std::cout << "Sample angle: " << sampleAngle <<
    std::endl;
    }
    */

    ImGui::End();
    ImGui::Render();

    for (int x = 0; x < sim.x_max; x++)
      for (int y = 0; y < sim.y_max; y++) {

        auto cell = sim.grid[x + sim.x_max * y];
        glm::vec3 p1{scale * x + DX, scale * y + DY, 0};
        glm::vec3 p2{scale * (x + 1) + DX, scale * y + DY, 0};
        glm::vec3 p3{scale * (x + 1) + DX, scale * (y + 1) + DY, 0};
        glm::vec3 p4{scale * x + DX, scale * (y + 1) + DY, 0};

        glm::vec3 color = color_map[cell.color];

        // if (cell.boundary_index >= 0) color = glm::vec3(0, 1, 0);

        cpuGeom.verts.push_back(p1);
        cpuGeom.verts.push_back(p2);
        cpuGeom.verts.push_back(p3);
        cpuGeom.verts.push_back(p3);
        cpuGeom.verts.push_back(p4);
        cpuGeom.verts.push_back(p1);
        cpuGeom.cols.push_back(color);
        cpuGeom.cols.push_back(color);
        cpuGeom.cols.push_back(color);
        cpuGeom.cols.push_back(color);
        cpuGeom.cols.push_back(color);
        cpuGeom.cols.push_back(color);
      }

    gpuGeom.setVerts(cpuGeom.verts);
    gpuGeom.setCols(cpuGeom.cols);

    shader.use();
    gpuGeom.bind();

    glEnable(GL_FRAMEBUFFER_SRGB);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, GLsizei(cpuGeom.verts.size()));
    glDisable(GL_FRAMEBUFFER_SRGB); // disable sRGB for things like imgui

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    window.swapBuffers();
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwTerminate();
  return 0;
}
