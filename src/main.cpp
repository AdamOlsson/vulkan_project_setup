#include <print>
#include <vulkan/vulkan.h>

int main() {
    //Get Extension Count
    ::uint32_t extensionCount{};
    ::vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    //Debug Extensions
    std::printf("Found {%d} extensions!", extensionCount);

    return 0;
}
