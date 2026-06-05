#include "GLFW/glfw3.h"

#include <vulkan/vulkan.h>

#include "stdio.h"
#include <cstdlib>
#include "assert.h"

#define VENG_CHECK(x)\
    do\
    {\
        VkResult result = (x);\
        assert(result == VK_SUCCESS);\
    } while(0)

int main(int argc, char **argv)
{
    glfwInit(); GLFWwindow *window = glfwCreateWindow(1366, 768, "veng", 0, 0);

    VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.apiVersion = VK_API_VERSION_1_4;

    VkInstanceCreateInfo create_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    create_info.pApplicationInfo = &app_info;

    VkInstance instance = 0;
    VENG_CHECK(vkCreateInstance(&create_info, 0, &instance));

    #ifdef DEBUG_BUILD
        printf("this is a debug build.\n");
    #endif

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    exit(0);
}