#include "graphics_context.h"
#include <memory>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const char WINDOW_TITLE[] = "Vulkan Tutorial";

GLFWwindow *initWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  // For a non-resizeable window, use:
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  GLFWwindow *window =
      glfwCreateWindow(WIDTH, HEIGHT, WINDOW_TITLE, nullptr, nullptr);
  return window;
}

int main() {
  GLFWwindow *window = initWindow();
  auto ctx = std::make_unique<GraphicsContext>(window);
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    ctx->render();
  }
  ctx->waitIdle();
}
