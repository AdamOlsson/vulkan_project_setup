#pragma once

#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vulkan/vulkan_beta.h"

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

const int MAX_FRAMES_IN_FLIGHT = 2;

struct UniformBufferObject {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
  }
  static std::array<VkVertexInputAttributeDescription, 2>
  getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);
    return attributeDescriptions;
  }
};

const std::vector<Vertex> vertices = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                      {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                                      {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
                                      {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};

const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {

  std::cout << "validation layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

class GraphicsContext {
public:
  GraphicsContext(GLFWwindow *window) : enableValidationLayers(true) {
    instance = createInstance(enableValidationLayers);
    if (enableValidationLayers) {
      debugMessenger = setupDebugMessenger(instance, enableValidationLayers);
    }
    // Create surface can affect the device selection, so we create it before
    // selecting device
    surface = createSurface(&instance, window);
    physicalDevice = pickPhysicalDevice(instance);
    device = createLogicalDevice(physicalDevice, deviceExtensions);

    auto [_swapChain, _swapChainImages, _swapChainImageFormat,
          _swapChainExtent] = createSwapChain(physicalDevice, device);
    swapChain = _swapChain;
    swapChainImages = _swapChainImages;
    swapChainImageFormat = _swapChainImageFormat;
    swapChainExtent = _swapChainExtent;
    swapChainImageViews = createImageViews(device, swapChainImages);
    renderPass = createRenderPass(device, swapChainImageFormat);
    descriptorSetLayout = createDescriptorSetLayout(device);

    graphicsPipeline = createGraphicsPipeline(device, "src/shaders/vert.spv",
                                              "src/shaders/frag.spv");

    swapChainFramebuffers = createFramebuffers(device, swapChainImageViews);
    commandPool = createCommandPool(physicalDevice, device);

    auto [_vertexBuffer, _vertexBufferMemory] =
        createVertexBuffer(device, vertices);
    vertexBuffer = _vertexBuffer;
    vertexBufferMemory = _vertexBufferMemory;

    auto [_indexBuffer, _indexBufferMemory] =
        createIndexBuffer(device, indices);
    indexBuffer = _indexBuffer;
    indexBufferMemory = _indexBufferMemory;
    auto [_uniformBuffers, _uniformBuffersMemory, _uniformBuffersMapped] =
        createUniformBuffers(device, MAX_FRAMES_IN_FLIGHT);
    uniformBuffers = _uniformBuffers;
    uniformBuffersMemory = _uniformBuffersMemory;
    uniformBuffersMapped = _uniformBuffersMapped;

    descriptorPool = createDescriptorPool(device, MAX_FRAMES_IN_FLIGHT);
    descriptorSets =
        createDescriptorSets(device, descriptorSetLayout, MAX_FRAMES_IN_FLIGHT);
    commandBuffers = createCommandBuffers(device, MAX_FRAMES_IN_FLIGHT);

    auto [_imageAvailableSemaphores, _renderFinishedSemaphores,
          _inFlightFences] = createSyncObjects(device, MAX_FRAMES_IN_FLIGHT);
    imageAvailableSemaphores = _imageAvailableSemaphores;
    renderFinishedSemaphores = _renderFinishedSemaphores;
    inFlightFences = _inFlightFences;
    this->window = window;
  }

  ~GraphicsContext() {
    // Order of these functions matter
    cleanupSwapChain(device);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroyBuffer(device, uniformBuffers[i], nullptr);
      vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
      vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
      vkDestroyFence(device, inFlightFences[i], nullptr);
    }
    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    vkDestroyDevice(device, nullptr);
    if (enableValidationLayers) {
      DestroyDebugUtilsMessengerEXT(instance, debugMessenger.value(), nullptr);
    }
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
  }

  void waitIdle();
  void render();

private:
  uint32_t currentFrame = 0;
  bool enableValidationLayers;
  GLFWwindow *window;
  VkInstance instance;
  std::optional<VkDebugUtilsMessengerEXT> debugMessenger;

  VkSurfaceKHR surface;

  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device;
  VkQueue graphicsQueue;
  VkQueue presentQueue;

  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkImageView> swapChainImageViews;

  VkRenderPass renderPass;

  VkDescriptorSetLayout descriptorSetLayout;

  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
  std::vector<VkFramebuffer> swapChainFramebuffers;
  VkCommandPool commandPool;
  std::vector<VkCommandBuffer> commandBuffers;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;

  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkBuffer indexBuffer;
  VkDeviceMemory indexBufferMemory;

  std::vector<VkBuffer> uniformBuffers;
  std::vector<VkDeviceMemory> uniformBuffersMemory;
  std::vector<void *> uniformBuffersMapped;

  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;

  const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};

  bool framebufferResized = false;

  void recreateSwapChain(VkDevice &device);

  void populateDebugMessengerCreateInfo(
      VkDebugUtilsMessengerCreateInfoEXT &createInfo);

  bool checkValidationLayerSupport();

  std::vector<const char *> getRequiredExtensions(bool enableValidationLayers);

  VkInstance createInstance(bool enableValidationLayers);

  std::optional<VkDebugUtilsMessengerEXT>
  setupDebugMessenger(VkInstance &instance, bool enableValidationLayers);

  VkResult CreateDebugUtilsMessengerEXT(
      VkInstance instance,
      const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
      const VkAllocationCallbacks *pAllocator,
      VkDebugUtilsMessengerEXT *pDebugMessenger);

  VkPhysicalDevice pickPhysicalDevice(VkInstance &instance);
  bool isDeviceSuitable(const VkPhysicalDevice &physicalDevice);

  VkDevice
  createLogicalDevice(VkPhysicalDevice &physicalDevice,
                      const std::vector<const char *> &deviceExtensions);

  std::vector<VkImageView>
  createImageViews(VkDevice &device, std::vector<VkImage> &swapChainImages);

  VkRenderPass createRenderPass(VkDevice &device,
                                VkFormat &swapChainImageFormat);

  VkDescriptorSetLayout createDescriptorSetLayout(VkDevice &device);

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);

  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);

  VkExtent2D chooseSwapExtent(GLFWwindow &window,
                              const VkSurfaceCapabilitiesKHR &capabilities);

  std::tuple<VkSwapchainKHR, std::vector<VkImage>, VkFormat, VkExtent2D>
  createSwapChain(VkPhysicalDevice &physicalDevice, VkDevice &device);

  VkSurfaceKHR createSurface(VkInstance *instance, GLFWwindow *window);
  bool checkDeviceExtensionSupport(const VkPhysicalDevice &physicalDevice);

  SwapChainSupportDetails
  querySwapChainSupport(const VkPhysicalDevice &physicalDevice);

  QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice &device);

  VkPipeline createGraphicsPipeline(VkDevice &device,
                                    const std::string vertex_shader_path,
                                    const std::string fragment_shader_path);
  std::vector<VkCommandBuffer> createCommandBuffers(VkDevice &device,
                                                    const int capacity);

  std::vector<VkFramebuffer>
  createFramebuffers(VkDevice &device,
                     std::vector<VkImageView> &swapChainImageViews);

  VkCommandPool createCommandPool(VkPhysicalDevice &physicalDevice,
                                  VkDevice &device);

  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex,
                           uint32_t currentFrame,
                           const std::vector<uint16_t> indices);

  std::tuple<std::vector<VkSemaphore>, std::vector<VkSemaphore>,
             std::vector<VkFence>>
  createSyncObjects(VkDevice &device, const int capacity);

  uint32_t findMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties);

  std::tuple<VkBuffer, VkDeviceMemory>
  createVertexBuffer(VkDevice &device, const std::vector<Vertex> &vertices);

  std::tuple<VkBuffer, VkDeviceMemory>
  createIndexBuffer(VkDevice &device, const std::vector<uint16_t> &indices);

  void copyBuffer(VkDevice &device, VkBuffer srcBuffer, VkBuffer dstBuffer,
                  VkDeviceSize size);

  void createBuffer(VkDevice &device, VkDeviceSize size,
                    VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                    VkBuffer &buffer, VkDeviceMemory &bufferMemory);

  void updateUniformBuffer(uint32_t currentImage);
  std::tuple<std::vector<VkBuffer>, std::vector<VkDeviceMemory>,
             std::vector<void *>>
  createUniformBuffers(VkDevice &device, const int capacity);

  std::vector<VkDescriptorSet>
  createDescriptorSets(VkDevice &device,
                       VkDescriptorSetLayout &descriptorSetLayout,
                       const int capacity);

  VkDescriptorPool createDescriptorPool(VkDevice &device, const int capacity);

  void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                     VkDebugUtilsMessengerEXT debugMessenger,
                                     const VkAllocationCallbacks *pAllocator);

  void cleanupSwapChain(VkDevice &device);
};
