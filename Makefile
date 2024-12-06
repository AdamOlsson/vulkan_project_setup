CXX = g++
VULKAN_SDK = /Users/adamolsson/VulkanSDK/1.3.296.0/macOS
VULKAN_LIBRARY_PATH = $(VULKAN_SDK)/lib
VULKAN_INCLUDE = $(VULKAN_SDK)/include
VULKAN_LIBRARY = vulkan.1

CXX_INCLUDE = -I$(VULKAN_INCLUDE)

TARGET = main
SRC = src/main.cpp

CXXFLAGS = $(CXX_INCLUDE) -std=c++17 -Wall -O2

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o bin/$(TARGET) \
		-L$(VULKAN_LIBRARY_PATH) \
		-Wl,-rpath,$(VULKAN_LIBRARY_PATH) \
		-l$(VULKAN_LIBRARY)

