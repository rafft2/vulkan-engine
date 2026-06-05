#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#include <vulkan/vulkan.h>

#include "stdio.h"
#include <cstdlib>

// TODO: replace asserts and VK_CHECK for proper error handling and fallbacks
#include "assert.h"
#define VK_CHECK(x)\
    do\
    {\
        VkResult result = (x);\
        assert(result == VK_SUCCESS);\
    } while(0)

#define ARRAY_COUNT(arr) sizeof(arr) / sizeof(arr[0])

typedef unsigned int u32;
typedef signed int s32;
typedef float f32;

static VkInstance CreateInstance()
{
    VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.apiVersion = VK_API_VERSION_1_4;
    app_info.applicationVersion = 1u;
    app_info.engineVersion = 1u;

    VkInstanceCreateInfo create_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    create_info.pApplicationInfo = &app_info;
#ifdef DEBUG_BUILD
    const char *debug_layers[] =
    {
        "VK_LAYER_KHRONOS_validation",
    };
    create_info.ppEnabledLayerNames = debug_layers;
    create_info.enabledLayerCount = ARRAY_COUNT(debug_layers);
#endif

    const char *extensions[] =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,

    #ifdef VK_USE_PLATFORM_WIN32_KHR
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    #endif
    };
    create_info.ppEnabledExtensionNames = extensions;
    create_info.enabledExtensionCount = ARRAY_COUNT(extensions);

    VkInstance instance = 0;
    VK_CHECK(vkCreateInstance(&create_info, 0, &instance));
    return(instance);
}

static VkPhysicalDevice CreatePhysicalDevice(VkInstance instance)
{
    VkPhysicalDevice physical_devices[16];
    u32 physical_device_count = ARRAY_COUNT(physical_devices);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices));
    if(physical_device_count == 0)
    {
        printf("Error. Driver or GPU doesn't support Vulkan.\n");
    }
    for(u32 i = 0; i < physical_device_count; i++)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(physical_devices[i], &props);
        if(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            printf("Using discrete GPU: %s.\n", props.deviceName);
            return(physical_devices[i]);
        }
    }
    return(physical_devices[0]);
}

static VkDevice CreateDevice(VkPhysicalDevice physical_device, u32 *family_index)
{
    f32 queue_priorities[] = {1.0f};
    VkDeviceQueueCreateInfo queue_create_info = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    *family_index = 0;
    queue_create_info.queueFamilyIndex = *family_index;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = queue_priorities;

    const char *extensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    VkDeviceCreateInfo create_device_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    create_device_info.queueCreateInfoCount = 1;
    create_device_info.pQueueCreateInfos = &queue_create_info;
    create_device_info.ppEnabledExtensionNames = extensions;
    create_device_info.enabledExtensionCount = ARRAY_COUNT(extensions);

    VkDevice device = 0;
    VK_CHECK(vkCreateDevice(physical_device, &create_device_info, 0, &device));
    return(device);
}

static VkSurfaceKHR CreateSurface(VkInstance instance, GLFWwindow* window)
{
    VkWin32SurfaceCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    create_info.hinstance = GetModuleHandle(0);
    create_info.hwnd = glfwGetWin32Window(window);

    VkSurfaceKHR surface = 0;
    VK_CHECK(vkCreateWin32SurfaceKHR(instance, &create_info, 0, &surface));
    return(surface);
}

static VkSwapchainKHR CreateSwapchain(VkDevice device, VkSurfaceKHR surface, u32 family_index, u32 width, u32 height)
{
    VkSwapchainCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    create_info.surface = surface;
    create_info.minImageCount = 2;
    create_info.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    create_info.imageExtent = {width, height};
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &family_index;
    create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    VkSwapchainKHR swapchain = 0;
    VK_CHECK(vkCreateSwapchainKHR(device, &create_info, 0, &swapchain));
    return(swapchain);
}

static VkSemaphore CreateSemaphore(VkDevice device)
{
    VkSemaphoreCreateInfo create_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkSemaphore semaphore = 0;
    VK_CHECK(vkCreateSemaphore(device, &create_info, 0, &semaphore));
    return(semaphore);
}

static VkCommandPool CreateCommandPool(VkDevice device, u32 family_index)
{
    VkCommandPoolCreateInfo create_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    create_info.queueFamilyIndex = family_index;

    VkCommandPool command_pool = 0;
    vkCreateCommandPool(device, &create_info, 0, &command_pool);
    return(command_pool);
}

int main(int argc, char **argv)
{
    glfwInit(); GLFWwindow *window = glfwCreateWindow(1366, 768, "veng", 0, 0);

    VkInstance instance = CreateInstance();

    VkPhysicalDevice physical_device = CreatePhysicalDevice(instance);

    u32 family_index = 0;
    VkDevice device = CreateDevice(physical_device, &family_index);

    VkSurfaceKHR surface = CreateSurface(instance, window);
    assert(surface);

    s32 swapchain_width = 0; s32 swapchain_height = 0;
    glfwGetWindowSize(window, &swapchain_width, &swapchain_height);
    VkSwapchainKHR swapchain = CreateSwapchain(device, surface, family_index, (u32)swapchain_width, (u32)swapchain_height);

    VkSemaphore acquire_semaphore = CreateSemaphore(device);
    VkSemaphore release_semaphore = CreateSemaphore(device);

    VkQueue queue = 0;
    vkGetDeviceQueue(device, family_index, 0, &queue);

    VkImage swapchain_images[16];
    u32 swapchain_image_count = ARRAY_COUNT(swapchain_images);
    vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images);

    VkCommandPool command_pool = CreateCommandPool(device, family_index);
    
    VkCommandBufferAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocate_info.commandPool = command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer = 0;
    vkAllocateCommandBuffers(device, &allocate_info, &command_buffer);

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        u32 image_index = 0;
        unsigned long long timeout = ~0ull - 50ull;
        VK_CHECK(vkAcquireNextImageKHR(device, swapchain, timeout, acquire_semaphore, VK_NULL_HANDLE, &image_index));

        vkResetCommandPool(device, command_pool, 0);

        VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(command_buffer, &begin_info);

        VkClearColorValue color = {0.0f, 0.0f, 1.0f, 1.0f};
        VkImageSubresourceRange range = {};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.levelCount = 1;
        range.layerCount = 1;

        vkCmdClearColorImage(command_buffer, swapchain_images[image_index], VK_IMAGE_LAYOUT_GENERAL, &color, 1, &range);
        vkEndCommandBuffer(command_buffer);

        VkPipelineStageFlags submit_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &acquire_semaphore;
        submit_info.pWaitDstStageMask = &submit_stage_mask;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &release_semaphore;

        vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);

        VkPresentInfoKHR present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &release_semaphore;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &image_index;
        VK_CHECK(vkQueuePresentKHR(queue, &present_info));

        VK_CHECK(vkDeviceWaitIdle(device));
    }

    glfwDestroyWindow(window);
    exit(0);
}