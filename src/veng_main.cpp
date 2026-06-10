#include "glfw3.h"
#include "glfw3native.h"

#define VOLK_IMPLEMENTATION
#include <volk.h>

#define FAST_OBJ_IMPLEMENTATION
#pragma warning(push, 0)
#include <fast_obj.h>
#pragma warning(pop)
#include <meshoptimizer.h>

#include "stdio.h"
#include <cstdlib>

#include "veng_math.h"

// TODO: replace asserts and VK_CHECK for proper error handling and fallbacks
#include "assert.h"
#define VK_CHECK(x)\
    do\
    {\
        VkResult VK_CHECK_RESULT = (x);\
        assert(VK_CHECK_RESULT == VK_SUCCESS);\
    } while(0)

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
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    #endif
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
    };
    create_info.ppEnabledExtensionNames = extensions;
    create_info.enabledExtensionCount = ARRAY_COUNT(extensions);

    VkInstance instance = 0;
    VK_CHECK(vkCreateInstance(&create_info, 0, &instance));
    return(instance);
}

static VkBool32 DebugReportCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t object,
    size_t location,
    int32_t messageCode,
    const char* pLayerPrefix,
    const char* pMessage,
    void* pUserData)
{
    const char* type = (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) ? "ERROR" :
                       (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) ? "WARNING" :
                       "INFO";
    printf("%s: %s\n", type, pMessage);
    if(flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        assert(0);
    }

    return(VK_FALSE);
}

static VkDebugReportCallbackEXT RegisterDebugCallback(VkInstance instance)
{
    VkDebugReportCallbackCreateInfoEXT create_info = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
    create_info.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;
    create_info.pfnCallback = DebugReportCallback;

    VkDebugReportCallbackEXT callback = 0;
    vkCreateDebugReportCallbackEXT(instance, &create_info, 0, &callback);
    return(callback);
}

b32 SupportsPresentation(VkPhysicalDevice physical_device, u32 family_index)
{
#ifdef VK_USE_PLATFORM_WIN32_KHR
    return(vkGetPhysicalDeviceWin32PresentationSupportKHR(physical_device, family_index));
#else
    return(true);
#endif
}

static u32 GetGraphicsFamilyIndex(VkPhysicalDevice physical_device)
{
    VkQueueFamilyProperties queue_family_properties[64];
    u32 queue_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_count, 0);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_count, queue_family_properties);

    for(u32 i = 0; i < queue_count; i++)
    {
        if(queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            return(i);
        }
    }
    return(VK_QUEUE_FAMILY_IGNORED);
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
    VkPhysicalDevice discrete = 0;
    VkPhysicalDevice fallback = 0;
    for(u32 i = 0; i < physical_device_count; i++)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(physical_devices[i], &props);
        printf("GPU %d: %s.\n", i, props.deviceName);
        u32 family_index = GetGraphicsFamilyIndex(physical_devices[i]);
        if(family_index == VK_QUEUE_FAMILY_IGNORED)
        {
            continue;
        }
        if(!SupportsPresentation(physical_devices[i], family_index))
        {
            continue;
        }
        if(!discrete && props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            discrete = physical_devices[i];
        }
        if(!fallback)
        {
            fallback = physical_devices[i];
        }
    }
    assert(discrete || fallback);
    VkPhysicalDeviceProperties props;
    if(discrete)
    {
        vkGetPhysicalDeviceProperties(discrete, &props);
        printf("Picked GPU: %s.\n", props.deviceName);
        return(discrete);
    }
    else
    {
        vkGetPhysicalDeviceProperties(fallback, &props);
        printf("Picked GPU: %s.\n", props.deviceName);
        return(fallback);
    }
}

static VkDevice CreateDevice(VkPhysicalDevice physical_device, u32 family_index)
{
    f32 queue_priorities[] = {1.0f};
    VkDeviceQueueCreateInfo queue_create_info = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queue_create_info.queueFamilyIndex = family_index;
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

static VkFormat GetSwapchainFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    VkSurfaceFormatKHR formats[16];
    u32 format_count = ARRAY_COUNT(formats);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats));

    return(formats[0].format);
}

static VkSwapchainKHR CreateSwapchain(VkDevice device, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR surface_caps, 
                                      u32 family_index, u32 width, u32 height, VkFormat format, VkSwapchainKHR old_swapchain = 0)
{
    VkCompositeAlphaFlagBitsKHR surface_composite =
        (surface_caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR :
        (surface_caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR :
        (surface_caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) ? VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR :
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

    VkSwapchainCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    create_info.surface = surface;
    create_info.minImageCount = MAXIMUM(2u, surface_caps.minImageCount);
    create_info.imageFormat = format;
    create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    create_info.imageExtent = {width, height};
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &family_index;
    create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    create_info.compositeAlpha = surface_composite;
    create_info.oldSwapchain = old_swapchain;

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

static VkRenderPass CreateRenderPass(VkDevice device, VkFormat format)
{
    VkAttachmentDescription attachments[1] = {};
    attachments[0].format = format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_attachments = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachments;

    VkRenderPassCreateInfo create_info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    create_info.attachmentCount = ARRAY_COUNT(attachments);
    create_info.pAttachments = attachments;
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass;

    VkRenderPass render_pass = 0;
    VK_CHECK(vkCreateRenderPass(device, &create_info, 0, &render_pass));
    return(render_pass);
}

VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format)
{
    VkImageViewCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    create_info.image = image;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = format;
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.layerCount = 1;

    VkImageView view;
    VK_CHECK(vkCreateImageView(device, &create_info, 0, &view));
    return(view);
}

VkFramebuffer CreateFramebuffer(VkDevice device, VkRenderPass render_pass, VkImageView image_view, u32 width, u32 height)
{
    VkFramebufferCreateInfo create_info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    create_info.renderPass = render_pass;
    create_info.attachmentCount = 1;
    create_info.pAttachments = &image_view;
    create_info.width = width;
    create_info.height = height;
    create_info.layers = 1;

    VkFramebuffer framebuffer = 0;
    VK_CHECK(vkCreateFramebuffer(device, &create_info, 0, &framebuffer));
    return(framebuffer);
}

VkShaderModule LoadShader(VkDevice device, char *path)
{
    FILE *file = fopen(path, "rb");
    assert(file);
    fseek(file, 0, SEEK_END);
    u64 length = (u64)ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char*)malloc(length);
    fread(buffer, 1, length, file);
    fclose(file);

    VkShaderModuleCreateInfo create_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    create_info.codeSize = length;
    create_info.pCode = (u32*)buffer;

    VkShaderModule shader_module = 0;
    VK_CHECK(vkCreateShaderModule(device, &create_info, 0, &shader_module));
    return(shader_module);
}

VkPipelineLayout CreatePipelineLayout(VkDevice device)
{
    VkPipelineLayoutCreateInfo create_info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

    VkPipelineLayout layout = 0;
    VK_CHECK(vkCreatePipelineLayout(device, &create_info, 0, &layout));
    return(layout);
}

VkPipeline CreateGraphicsPipeline(VkDevice device, VkPipelineCache pipeline_cache, VkRenderPass render_pass, VkPipelineLayout layout, VkShaderModule vert, VkShaderModule frag)
{
    VkGraphicsPipelineCreateInfo create_info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag;
    stages[1].pName = "main";

    create_info.stageCount = ARRAY_COUNT(stages);
    create_info.pStages = stages;

    VkPipelineVertexInputStateCreateInfo vertex_input = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    create_info.pVertexInputState = &vertex_input;

    VkPipelineInputAssemblyStateCreateInfo input_assembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    create_info.pInputAssemblyState = &input_assembly;

    VkPipelineViewportStateCreateInfo viewport_state = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;
    create_info.pViewportState = &viewport_state;
    
    VkPipelineRasterizationStateCreateInfo raster_state = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    raster_state.lineWidth = 1.0f;
    create_info.pRasterizationState = &raster_state;

    VkPipelineMultisampleStateCreateInfo multisample_state = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    create_info.pMultisampleState = &multisample_state;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    create_info.pDepthStencilState = &depth_stencil_state;

    VkPipelineColorBlendAttachmentState color_attachment_state = {};
    color_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blend_state = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    color_blend_state.attachmentCount = 1;
    color_blend_state.pAttachments = &color_attachment_state;
    create_info.pColorBlendState = &color_blend_state;


    VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    
    VkPipelineDynamicStateCreateInfo dynamic_state = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamic_state.dynamicStateCount = ARRAY_COUNT(dynamic_states);
    dynamic_state.pDynamicStates = dynamic_states;
    create_info.pDynamicState = &dynamic_state;

    create_info.layout = layout;
    create_info.renderPass = render_pass;

    VkPipeline pipeline = 0;
    VK_CHECK(vkCreateGraphicsPipelines(device, pipeline_cache, 1, &create_info, 0, &pipeline));
    return(pipeline);
}

static VkImageMemoryBarrier CreateImageMemoryBarrier(VkImage image, VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask,
                                                                    VkImageLayout old_layout, VkImageLayout new_layout)
{
    VkImageMemoryBarrier memory_barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };

    memory_barrier.srcAccessMask = src_access_mask;
    memory_barrier.dstAccessMask = dst_access_mask;
    memory_barrier.oldLayout = old_layout;
    memory_barrier.newLayout = new_layout;
    memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memory_barrier.image = image;
    memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    memory_barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    memory_barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return(memory_barrier);
}

struct veng_swapchain
{
    VkSwapchainKHR swapchain;
    VkImage *images;
    VkImageView *image_views;
    VkFramebuffer *framebuffers;
    u32 image_count;
    u32 width;
    u32 height;
};

static void CreateOrResizeSwapchain(veng_swapchain *result, 
                    VkDevice device, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR surface_caps, 
                    u32 family_index, u32 width, u32 height, VkFormat swapchain_format, VkRenderPass render_pass, VkSwapchainKHR old_swapchain = 0)
{
    VkSwapchainKHR swapchain = CreateSwapchain(device, surface, surface_caps, family_index, width, height, swapchain_format, old_swapchain);
    if(old_swapchain)
    {
        VK_CHECK(vkDeviceWaitIdle(device));
        vkDestroySwapchainKHR(device, old_swapchain, 0);
    }
    result->swapchain = swapchain;

    u32 image_count = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, 0);
    result->image_count = image_count;
    
    result->images = (VkImage*)malloc(sizeof(VkImage)*image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, result->images);

    result->image_views = (VkImageView*)malloc(sizeof(VkImageView)*image_count);
    for(u32 i = 0; i < image_count; i++)
    {
        result->image_views[i] = CreateImageView(device, result->images[i], swapchain_format);
    }

    result->framebuffers = (VkFramebuffer*)malloc(sizeof(VkFramebuffer)*image_count);
    for(u32 i = 0; i < image_count; i++)
    {
        result->framebuffers[i] = CreateFramebuffer(device, render_pass, result->image_views[i], width, height);
    }
    result->width = width;
    result->height = height;
}

struct veng_vertex
{
    vec3 pos;
    vec3 normal;
    // TODO: vec2
    f32 tu, tv;
};

struct veng_mesh
{
    veng_vertex *vertices;
    u32 vertex_count;

    u32 *indices;
    u32 index_count;
};

b32 LoadMesh(veng_mesh *inout, const char *path)
{
    fastObjMesh *mesh = fast_obj_read(path);
    if(!mesh)
    {
        return(false);
    }
    inout->vertex_count = mesh->position_count;
    inout->vertices = (veng_vertex*)malloc(inout->vertex_count * sizeof(veng_vertex));

    for(u32 i = 0; i < inout->vertex_count; i++)
    {
        inout->vertices[i].pos = *(vec3*)(&mesh->positions[i * 3]);

        if(mesh->normals && i < mesh->normal_count)
        {
            inout->vertices[i].normal = *(vec3*)(&mesh->normals[i * 3]);
        }
        else
        {
            inout->vertices[i].normal = {0.0f, 0.0f, 1.0f};
        }

        if(mesh->texcoords && i < mesh->texcoord_count)
        {
            inout->vertices[i].tu = mesh->texcoords[i * 2 + 0];
            inout->vertices[i].tv = mesh->texcoords[i * 2 + 1];
        }
        else
        {
            inout->vertices[i].tu = 0.0f;
            inout->vertices[i].tv = 0.0f;
        }
    }

    u32 index_count = mesh->face_count * 3;
    inout->index_count = index_count;
    inout->indices = (u32*)malloc(index_count*sizeof(u32));
    for(u32 i = 1; i < index_count; i++)
    {
        inout->indices[i] = mesh->indices[i].p;
    }

    fast_obj_destroy(mesh);
    return(true);
}

struct veng_buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    void *data;
    u32 size;
};

u32 SelectMemoryType(VkPhysicalDeviceMemoryProperties *memory_props, u32 memory_type_bits, VkMemoryPropertyFlags flags)
{
    for(u32 i = 0; i < memory_props->memoryTypeCount; i++)
    {
        if((memory_type_bits & (1 << i)) != 0 && (memory_props->memoryTypes[i].propertyFlags & flags) == flags)
        {
            return(i);
        }
    }
    printf("No compatible memory type found.\n");
    return(~0u);
}

void CreateBuffer(veng_buffer *result, VkDevice device, VkPhysicalDeviceMemoryProperties *memory_props, u32 size, VkBufferUsageFlags usage)
{
    VkBufferCreateInfo create_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    create_info.size = size;
    create_info.usage = usage;

    VkBuffer buffer = 0;
    vkCreateBuffer(device, &create_info, 0, &buffer);

    VkMemoryRequirements memory_reqs;
    vkGetBufferMemoryRequirements(device, buffer, &memory_reqs);

    u32 memory_type_index = SelectMemoryType(memory_props, memory_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkMemoryAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocate_info.allocationSize = memory_reqs.size;
    allocate_info.memoryTypeIndex = memory_type_index;

    VkDeviceMemory memory = 0;
    vkAllocateMemory(device, &allocate_info, 0, &memory);

    VK_CHECK(vkBindBufferMemory(device, buffer, memory, 0));
    void *data = 0;
    VK_CHECK(vkMapMemory(device, memory, 0, size, 0, &data));

    result->buffer = buffer;
    result->memory = memory;
    result->data = data;
    result->size = size;
}

void DestroyBuffer(veng_buffer *buffer, VkDevice device)
{
    vkFreeMemory(device, buffer->memory, 0);
    vkDestroyBuffer(device, buffer->buffer, 0);
}

int main(int argc, char **argv)
{
    s32 window_width = 1366; s32 window_height = 768;
    glfwInit(); GLFWwindow *window = glfwCreateWindow(window_width, window_height, "veng", 0, 0);
    
    volkInitialize();
    VkInstance instance = CreateInstance();
    volkLoadInstance(instance);

    VkDebugReportCallbackEXT debug_callback = RegisterDebugCallback(instance);

    VkPhysicalDevice physical_device = CreatePhysicalDevice(instance);

    u32 family_index = GetGraphicsFamilyIndex(physical_device);
    assert(family_index != VK_QUEUE_FAMILY_IGNORED);
    VkDevice device = CreateDevice(physical_device, family_index);
    volkLoadDevice(device);

    VkSurfaceKHR surface = CreateSurface(instance, window);
    assert(surface);
    VkBool32 present_supported = 0;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, family_index, surface, &present_supported);
    assert(present_supported);

    VkFormat swapchain_format = GetSwapchainFormat(physical_device, surface);

    VkSurfaceCapabilitiesKHR surface_caps;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_caps));

    VkSemaphore acquire_semaphore = CreateSemaphore(device);
    VkSemaphore release_semaphore = CreateSemaphore(device);

    VkQueue queue = 0;
    vkGetDeviceQueue(device, family_index, 0, &queue);

    VkRenderPass render_pass = CreateRenderPass(device, swapchain_format);
    assert(render_pass);

    VkShaderModule triangle_vert = LoadShader(device, "./src/shaders/triangle.vert.spv");
    VkShaderModule triangle_frag = LoadShader(device, "./src/shaders/triangle.frag.spv");

    VkPipelineCache pipeline_cache = 0;
    VkPipelineLayout layout = CreatePipelineLayout(device);
    VkPipeline triangle_pipeline = CreateGraphicsPipeline(device, pipeline_cache, render_pass, layout, triangle_vert, triangle_frag);

    veng_swapchain swapchain;
    CreateOrResizeSwapchain(&swapchain, device, surface, surface_caps, family_index, (u32)window_width, (u32)window_height, swapchain_format, render_pass);

    VkCommandPool command_pool = CreateCommandPool(device, family_index);
    
    VkCommandBufferAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocate_info.commandPool = command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer = 0;
    vkAllocateCommandBuffers(device, &allocate_info, &command_buffer);

    VkPhysicalDeviceMemoryProperties memory_props;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_props);

    veng_mesh mesh;
    if(!LoadMesh(&mesh, "assets/sponza.obj"))
    {
        printf("error");
    }

    veng_buffer vertex_buffer = {}; 
    CreateBuffer(&vertex_buffer, device, &memory_props, Megabytes(128), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    veng_buffer index_buffer = {};
    CreateBuffer(&index_buffer, device, &memory_props, Megabytes(128), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    assert(vertex_buffer.size >= mesh.vertex_count * sizeof(veng_vertex));
    memcpy(vertex_buffer.data, mesh.vertices, mesh.vertex_count * sizeof(veng_vertex));

    assert(index_buffer.size >= mesh.index_count * sizeof(u32));
    memcpy(index_buffer.data, mesh.indices, mesh.index_count * sizeof(u32));

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_caps));

        if(swapchain.width != surface_caps.currentExtent.width || swapchain.height != surface_caps.currentExtent.height)
        {
            CreateOrResizeSwapchain(&swapchain, device, surface, surface_caps, family_index, surface_caps.currentExtent.width, surface_caps.currentExtent.height, swapchain_format, render_pass, swapchain.swapchain);
        }

        u32 image_index = 0;
        u64 timeout = ~0ull - 50ull;
        VK_CHECK(vkAcquireNextImageKHR(device, swapchain.swapchain, timeout, acquire_semaphore, VK_NULL_HANDLE, &image_index));

        vkResetCommandPool(device, command_pool, 0);

        VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        VK_CHECK(vkBeginCommandBuffer(command_buffer, &begin_info));

            VkImageMemoryBarrier render_begin_barrier = 
                CreateImageMemoryBarrier(swapchain.images[image_index],
                                         0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
                                 VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &render_begin_barrier);

            VkClearColorValue color = {0.4f, 0.25f, 0.4f, 1.0f};
            VkClearValue clear_color = {color};

            VkRenderPassBeginInfo pass_begin_info = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
            pass_begin_info.renderPass = render_pass;
            pass_begin_info.framebuffer = swapchain.framebuffers[image_index];
            pass_begin_info.renderArea.extent.width = swapchain.width;
            pass_begin_info.renderArea.extent.height = swapchain.height;
            pass_begin_info.clearValueCount = 1;
            pass_begin_info.pClearValues = &clear_color;

            vkCmdBeginRenderPass(command_buffer, &pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

                VkViewport viewport = {0, (f32)window_height, (f32)swapchain.width, -(f32)swapchain.height, 0, 1};
                VkRect2D scissor = {{0, 0}, {swapchain.width, swapchain.height}};

                vkCmdSetViewport(command_buffer, 0, 1, &viewport);
                vkCmdSetScissor(command_buffer, 0, 1, &scissor);
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle_pipeline);
                vkCmdDraw(command_buffer, 3, 1, 0, 0);
            
            vkCmdEndRenderPass(command_buffer);
            
            VkImageMemoryBarrier render_end_barrier = 
                CreateImageMemoryBarrier(swapchain.images[image_index],
                                         VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0,
                                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &render_end_barrier);

        VK_CHECK(vkEndCommandBuffer(command_buffer));

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
        present_info.pSwapchains = &swapchain.swapchain;
        present_info.pImageIndices = &image_index;
        VK_CHECK(vkQueuePresentKHR(queue, &present_info));

        VK_CHECK(vkDeviceWaitIdle(device));
    }

    glfwDestroyWindow(window);
    exit(0);
}