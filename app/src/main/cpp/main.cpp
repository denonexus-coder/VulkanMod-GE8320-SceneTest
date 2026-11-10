#include <android/log.h>
#include <android_native_app_glue.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>
#include <string.h>
#include <stdlib.h>

#define LOG_TAG "VulkanSceneTest"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define VK_CHECK(r) do { VkResult _r = (r); if(_r != VK_SUCCESS){ LOGE("VK error %d at line %d", _r, __LINE__); return; } } while(0)

/* ───────────── SPIR-V shaders (compiled with glslangValidator 16.6.0) ─────── */

static const uint32_t vert_spv[] = {
    0x07230203, 0x00010000, 0x0008000B, 0x00000036, 0x00000000, 0x00020011, 0x00000001, 0x0006000B,
    0x00000001, 0x4C534C47, 0x6474732E, 0x3035342E, 0x00000000, 0x0003000E, 0x00000000, 0x00000001,
    0x0008000F, 0x00000000, 0x00000004, 0x6E69616D, 0x00000000, 0x00000022, 0x00000026, 0x00000031,
    0x00030003, 0x00000002, 0x000001C2, 0x00040005, 0x00000004, 0x6E69616D, 0x00000000, 0x00050005,
    0x0000000C, 0x69736F70, 0x6E6F6974, 0x00000073, 0x00040005, 0x00000017, 0x6F6C6F63, 0x00007372,
    0x00060005, 0x00000020, 0x505F6C67, 0x65567265, 0x78657472, 0x00000000, 0x00060006, 0x00000020,
    0x00000000, 0x505F6C67, 0x7469736F, 0x006E6F69, 0x00070006, 0x00000020, 0x00000001, 0x505F6C67,
    0x746E696F, 0x657A6953, 0x00000000, 0x00070006, 0x00000020, 0x00000002, 0x435F6C67, 0x4470696C,
    0x61747369, 0x0065636E, 0x00070006, 0x00000020, 0x00000003, 0x435F6C67, 0x446C6C75, 0x61747369,
    0x0065636E, 0x00030005, 0x00000022, 0x00000000, 0x00060005, 0x00000026, 0x565F6C67, 0x65747265,
    0x646E4978, 0x00007865, 0x00050005, 0x00000031, 0x67617266, 0x6F6C6F43, 0x00000072, 0x00030047,
    0x00000020, 0x00000002, 0x00050048, 0x00000020, 0x00000000, 0x0000000B, 0x00000000, 0x00050048,
    0x00000020, 0x00000001, 0x0000000B, 0x00000001, 0x00050048, 0x00000020, 0x00000002, 0x0000000B,
    0x00000003, 0x00050048, 0x00000020, 0x00000003, 0x0000000B, 0x00000004, 0x00040047, 0x00000026,
    0x0000000B, 0x0000002A, 0x00040047, 0x00000031, 0x0000001E, 0x00000000, 0x00020013, 0x00000002,
    0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007,
    0x00000006, 0x00000002, 0x00040015, 0x00000008, 0x00000020, 0x00000000, 0x0004002B, 0x00000008,
    0x00000009, 0x00000003, 0x0004001C, 0x0000000A, 0x00000007, 0x00000009, 0x00040020, 0x0000000B,
    0x00000006, 0x0000000A, 0x0004003B, 0x0000000B, 0x0000000C, 0x00000006, 0x0004002B, 0x00000006,
    0x0000000D, 0x00000000, 0x0004002B, 0x00000006, 0x0000000E, 0xBF000000, 0x0005002C, 0x00000007,
    0x0000000F, 0x0000000D, 0x0000000E, 0x0004002B, 0x00000006, 0x00000010, 0x3F000000, 0x0005002C,
    0x00000007, 0x00000011, 0x00000010, 0x00000010, 0x0005002C, 0x00000007, 0x00000012, 0x0000000E,
    0x00000010, 0x0006002C, 0x0000000A, 0x00000013, 0x0000000F, 0x00000011, 0x00000012, 0x00040017,
    0x00000014, 0x00000006, 0x00000003, 0x0004001C, 0x00000015, 0x00000014, 0x00000009, 0x00040020,
    0x00000016, 0x00000006, 0x00000015, 0x0004003B, 0x00000016, 0x00000017, 0x00000006, 0x0004002B,
    0x00000006, 0x00000018, 0x3F800000, 0x0006002C, 0x00000014, 0x00000019, 0x00000018, 0x0000000D,
    0x0000000D, 0x0006002C, 0x00000014, 0x0000001A, 0x0000000D, 0x00000018, 0x0000000D, 0x0006002C,
    0x00000014, 0x0000001B, 0x0000000D, 0x0000000D, 0x00000018, 0x0006002C, 0x00000015, 0x0000001C,
    0x00000019, 0x0000001A, 0x0000001B, 0x00040017, 0x0000001D, 0x00000006, 0x00000004, 0x0004002B,
    0x00000008, 0x0000001E, 0x00000001, 0x0004001C, 0x0000001F, 0x00000006, 0x0000001E, 0x0006001E,
    0x00000020, 0x0000001D, 0x00000006, 0x0000001F, 0x0000001F, 0x00040020, 0x00000021, 0x00000003,
    0x00000020, 0x0004003B, 0x00000021, 0x00000022, 0x00000003, 0x00040015, 0x00000023, 0x00000020,
    0x00000001, 0x0004002B, 0x00000023, 0x00000024, 0x00000000, 0x00040020, 0x00000025, 0x00000001,
    0x00000023, 0x0004003B, 0x00000025, 0x00000026, 0x00000001, 0x00040020, 0x00000028, 0x00000006,
    0x00000007, 0x00040020, 0x0000002E, 0x00000003, 0x0000001D, 0x00040020, 0x00000030, 0x00000003,
    0x00000014, 0x0004003B, 0x00000030, 0x00000031, 0x00000003, 0x00040020, 0x00000033, 0x00000006,
    0x00000014, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200F8, 0x00000005,
    0x0003003E, 0x0000000C, 0x00000013, 0x0003003E, 0x00000017, 0x0000001C, 0x0004003D, 0x00000023,
    0x00000027, 0x00000026, 0x00050041, 0x00000028, 0x00000029, 0x0000000C, 0x00000027, 0x0004003D,
    0x00000007, 0x0000002A, 0x00000029, 0x00050051, 0x00000006, 0x0000002B, 0x0000002A, 0x00000000,
    0x00050051, 0x00000006, 0x0000002C, 0x0000002A, 0x00000001, 0x00070050, 0x0000001D, 0x0000002D,
    0x0000002B, 0x0000002C, 0x0000000D, 0x00000018, 0x00050041, 0x0000002E, 0x0000002F, 0x00000022,
    0x00000024, 0x0003003E, 0x0000002F, 0x0000002D, 0x0004003D, 0x00000023, 0x00000032, 0x00000026,
    0x00050041, 0x00000033, 0x00000034, 0x00000017, 0x00000032, 0x0004003D, 0x00000014, 0x00000035,
    0x00000034, 0x0003003E, 0x00000031, 0x00000035, 0x000100FD, 0x00010038,
};
static const size_t vert_spv_size = sizeof(vert_spv);

static const uint32_t frag_spv[] = {
    0x07230203, 0x00010000, 0x0008000B, 0x00000013, 0x00000000, 0x00020011, 0x00000001, 0x0006000B,
    0x00000001, 0x4C534C47, 0x6474732E, 0x3035342E, 0x00000000, 0x0003000E, 0x00000000, 0x00000001,
    0x0007000F, 0x00000004, 0x00000004, 0x6E69616D, 0x00000000, 0x00000009, 0x0000000C, 0x00030010,
    0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001C2, 0x00040005, 0x00000004, 0x6E69616D,
    0x00000000, 0x00050005, 0x00000009, 0x4374756F, 0x726F6C6F, 0x00000000, 0x00050005, 0x0000000C,
    0x67617266, 0x6F6C6F43, 0x00000072, 0x00040047, 0x00000009, 0x0000001E, 0x00000000, 0x00040047,
    0x0000000C, 0x0000001E, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002,
    0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020,
    0x00000008, 0x00000003, 0x00000007, 0x0004003B, 0x00000008, 0x00000009, 0x00000003, 0x00040017,
    0x0000000A, 0x00000006, 0x00000003, 0x00040020, 0x0000000B, 0x00000001, 0x0000000A, 0x0004003B,
    0x0000000B, 0x0000000C, 0x00000001, 0x0004002B, 0x00000006, 0x0000000E, 0x3F800000, 0x00050036,
    0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200F8, 0x00000005, 0x0004003D, 0x0000000A,
    0x0000000D, 0x0000000C, 0x00050051, 0x00000006, 0x0000000F, 0x0000000D, 0x00000000, 0x00050051,
    0x00000006, 0x00000010, 0x0000000D, 0x00000001, 0x00050051, 0x00000006, 0x00000011, 0x0000000D,
    0x00000002, 0x00070050, 0x00000007, 0x00000012, 0x0000000F, 0x00000010, 0x00000011, 0x0000000E,
    0x0003003E, 0x00000009, 0x00000012, 0x000100FD, 0x00010038,
};
static const size_t frag_spv_size = sizeof(frag_spv);

/* ───────────── State ──────────────────────────────────────────────────────── */

struct VulkanState {
    VkInstance       instance;
    VkPhysicalDevice physDev;
    VkDevice         device;
    VkQueue          graphicsQueue;
    uint32_t         graphicsFamily;
    VkSurfaceKHR     surface;
    VkSwapchainKHR   swapchain;
    VkFormat         swapFormat;
    VkExtent2D       swapExtent;
    uint32_t         imageCount;
    VkImage*         images;
    VkImageView*     imageViews;
    VkRenderPass     renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline       pipeline;
    VkFramebuffer*   framebuffers;
    VkCommandPool    cmdPool;
    VkCommandBuffer* cmdBuffers;
    VkSemaphore      imageAvailable;
    VkSemaphore      renderFinished;
    VkFence          inFlight;
    bool             ready;
};

static VulkanState g_vk = {};

/* ───────────── Helpers ────────────────────────────────────────────────────── */

static VkShaderModule createShader(VkDevice dev, const uint32_t* code, size_t size) {
    VkShaderModuleCreateInfo ci = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    ci.codeSize = size;
    ci.pCode    = code;
    VkShaderModule mod;
    if (vkCreateShaderModule(dev, &ci, nullptr, &mod) != VK_SUCCESS) {
        LOGE("Failed to create shader module");
        return VK_NULL_HANDLE;
    }
    return mod;
}

/* ───────────── Init ───────────────────────────────────────────────────────── */

static void vulkanInit(struct android_app* app) {
    /* 1. Instance */
    const char* instExts[] = {
        "VK_KHR_surface",
        "VK_KHR_android_surface"
    };
    VkInstanceCreateInfo instCI = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    VkApplicationInfo appInfo   = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.pApplicationName    = "VulkanSceneTest";
    appInfo.apiVersion          = VK_API_VERSION_1_1;
    instCI.pApplicationInfo        = &appInfo;
    instCI.enabledExtensionCount   = 2;
    instCI.ppEnabledExtensionNames = instExts;
    VK_CHECK(vkCreateInstance(&instCI, nullptr, &g_vk.instance));
    LOGI("Instance created");

    /* 2. Physical device */
    uint32_t cnt = 1;
    vkEnumeratePhysicalDevices(g_vk.instance, &cnt, &g_vk.physDev);
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(g_vk.physDev, &props);
    LOGI("GPU: %s", props.deviceName);

    /* 3. Queue family */
    uint32_t qfc = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(g_vk.physDev, &qfc, nullptr);
    VkQueueFamilyProperties* qfp = (VkQueueFamilyProperties*)malloc(qfc * sizeof(*qfp));
    vkGetPhysicalDeviceQueueFamilyProperties(g_vk.physDev, &qfc, qfp);
    g_vk.graphicsFamily = UINT32_MAX;
    for (uint32_t i = 0; i < qfc; i++) {
        if (qfp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { g_vk.graphicsFamily = i; break; }
    }
    free(qfp);
    if (g_vk.graphicsFamily == UINT32_MAX) { LOGE("No graphics queue!"); return; }

    /* 4. Logical device */
    float prio = 1.0f;
    VkDeviceQueueCreateInfo qCI = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    qCI.queueFamilyIndex = g_vk.graphicsFamily;
    qCI.queueCount       = 1;
    qCI.pQueuePriorities = &prio;
    const char* devExts[] = { "VK_KHR_swapchain" };
    VkDeviceCreateInfo devCI = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    devCI.queueCreateInfoCount    = 1;
    devCI.pQueueCreateInfos       = &qCI;
    devCI.enabledExtensionCount   = 1;
    devCI.ppEnabledExtensionNames = devExts;
    VK_CHECK(vkCreateDevice(g_vk.physDev, &devCI, nullptr, &g_vk.device));
    vkGetDeviceQueue(g_vk.device, g_vk.graphicsFamily, 0, &g_vk.graphicsQueue);
    LOGI("Device created");

    /* 5. Surface */
    PFN_vkCreateAndroidSurfaceKHR pfnCreateSurface =
        (PFN_vkCreateAndroidSurfaceKHR)vkGetInstanceProcAddr(g_vk.instance, "vkCreateAndroidSurfaceKHR");
    VkAndroidSurfaceCreateInfoKHR surfCI = { VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR };
    surfCI.window = app->window;
    VK_CHECK(pfnCreateSurface(g_vk.instance, &surfCI, nullptr, &g_vk.surface));
    LOGI("Surface created");

    /* 6. Swapchain format — query and pick best */
    uint32_t fmtCnt = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(g_vk.physDev, g_vk.surface, &fmtCnt, nullptr);
    VkSurfaceFormatKHR* fmts = (VkSurfaceFormatKHR*)malloc(fmtCnt * sizeof(*fmts));
    vkGetPhysicalDeviceSurfaceFormatsKHR(g_vk.physDev, g_vk.surface, &fmtCnt, fmts);
    g_vk.swapFormat = fmts[0].format; /* fallback: first available */
    VkColorSpaceKHR colorSpace = fmts[0].colorSpace;
    for (uint32_t i = 0; i < fmtCnt; i++) {
        LOGI("Surface format[%u]: %d  colorspace: %d", i, fmts[i].format, fmts[i].colorSpace);
        /* Prefer BGRA8_UNORM (PowerVR native) */
        if (fmts[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
            fmts[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            g_vk.swapFormat = fmts[i].format;
            colorSpace       = fmts[i].colorSpace;
        }
    }
    free(fmts);
    LOGI("Selected swapchain format: %d", g_vk.swapFormat);

    /* Surface capabilities / extent */
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_vk.physDev, g_vk.surface, &caps);
    g_vk.swapExtent = caps.currentExtent;
    if (g_vk.swapExtent.width == 0xFFFFFFFF) {
        g_vk.swapExtent.width  = 720;
        g_vk.swapExtent.height = 1280;
    }

    uint32_t imgCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imgCount > caps.maxImageCount)
        imgCount = caps.maxImageCount;

    VkSwapchainCreateInfoKHR swapCI = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapCI.surface               = g_vk.surface;
    swapCI.minImageCount         = imgCount;
    swapCI.imageFormat           = g_vk.swapFormat;
    swapCI.imageColorSpace       = colorSpace;
    swapCI.imageExtent           = g_vk.swapExtent;
    swapCI.imageArrayLayers      = 1;
    swapCI.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapCI.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    swapCI.preTransform          = caps.currentTransform;
    swapCI.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapCI.presentMode           = VK_PRESENT_MODE_FIFO_KHR;
    swapCI.clipped               = VK_TRUE;
    VK_CHECK(vkCreateSwapchainKHR(g_vk.device, &swapCI, nullptr, &g_vk.swapchain));
    LOGI("Swapchain created (%ux%u)", g_vk.swapExtent.width, g_vk.swapExtent.height);

    /* 7. Swapchain images / views */
    vkGetSwapchainImagesKHR(g_vk.device, g_vk.swapchain, &g_vk.imageCount, nullptr);
    g_vk.images     = (VkImage*)    malloc(g_vk.imageCount * sizeof(VkImage));
    g_vk.imageViews = (VkImageView*)malloc(g_vk.imageCount * sizeof(VkImageView));
    vkGetSwapchainImagesKHR(g_vk.device, g_vk.swapchain, &g_vk.imageCount, g_vk.images);
    for (uint32_t i = 0; i < g_vk.imageCount; i++) {
        VkImageViewCreateInfo ivCI = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        ivCI.image    = g_vk.images[i];
        ivCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ivCI.format   = g_vk.swapFormat;
        ivCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ivCI.subresourceRange.levelCount = 1;
        ivCI.subresourceRange.layerCount = 1;
        VK_CHECK(vkCreateImageView(g_vk.device, &ivCI, nullptr, &g_vk.imageViews[i]));
    }
    LOGI("Image views created: %u", g_vk.imageCount);

    /* 8. Render pass */
    VkAttachmentDescription att = {};
    att.format         = g_vk.swapFormat;
    att.samples        = VK_SAMPLE_COUNT_1_BIT;
    att.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    att.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    att.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    att.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference attRef = {};
    attRef.attachment = 0;
    attRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &attRef;

    VkSubpassDependency dep = {};
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rpCI = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    rpCI.attachmentCount = 1;
    rpCI.pAttachments    = &att;
    rpCI.subpassCount    = 1;
    rpCI.pSubpasses      = &subpass;
    rpCI.dependencyCount = 1;
    rpCI.pDependencies   = &dep;
    VK_CHECK(vkCreateRenderPass(g_vk.device, &rpCI, nullptr, &g_vk.renderPass));

    /* 9. Pipeline layout (empty — no uniforms) */
    VkPipelineLayoutCreateInfo plCI = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    VK_CHECK(vkCreatePipelineLayout(g_vk.device, &plCI, nullptr, &g_vk.pipelineLayout));

    /* 10. Shaders */
    VkShaderModule vertMod = createShader(g_vk.device, vert_spv, vert_spv_size);
    VkShaderModule fragMod = createShader(g_vk.device, frag_spv, frag_spv_size);
    if (vertMod == VK_NULL_HANDLE || fragMod == VK_NULL_HANDLE) return;

    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertMod;
    stages[0].pName  = "main";
    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragMod;
    stages[1].pName  = "main";

    /* 11. Fixed-function state */
    VkPipelineVertexInputStateCreateInfo viCI = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    /* No vertex buffers — positions hardcoded in shader via gl_VertexIndex */

    VkPipelineInputAssemblyStateCreateInfo iaCI = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    iaCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport = { 0, 0, (float)g_vk.swapExtent.width, (float)g_vk.swapExtent.height, 0.0f, 1.0f };
    VkRect2D   scissor  = { {0,0}, g_vk.swapExtent };
    VkPipelineViewportStateCreateInfo vpCI = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    vpCI.viewportCount = 1; vpCI.pViewports = &viewport;
    vpCI.scissorCount  = 1; vpCI.pScissors  = &scissor;

    VkPipelineRasterizationStateCreateInfo rsCI = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rsCI.polygonMode  = VK_POLYGON_MODE_FILL;
    rsCI.cullMode     = VK_CULL_MODE_NONE;   /* no culling for sanity check */
    rsCI.frontFace    = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rsCI.lineWidth    = 1.0f;

    VkPipelineMultisampleStateCreateInfo msCI = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    msCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState cbAtt = {};
    cbAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    cbAtt.blendEnable    = VK_FALSE;   /* plain opaque — no dual-src needed */

    VkPipelineColorBlendStateCreateInfo cbCI = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    cbCI.logicOpEnable   = VK_FALSE;
    cbCI.attachmentCount = 1;
    cbCI.pAttachments    = &cbAtt;

    /* 12. Graphics pipeline */
    VkGraphicsPipelineCreateInfo gpCI = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    gpCI.stageCount          = 2;
    gpCI.pStages             = stages;
    gpCI.pVertexInputState   = &viCI;
    gpCI.pInputAssemblyState = &iaCI;
    gpCI.pViewportState      = &vpCI;
    gpCI.pRasterizationState = &rsCI;
    gpCI.pMultisampleState   = &msCI;
    gpCI.pColorBlendState    = &cbCI;
    gpCI.layout              = g_vk.pipelineLayout;
    gpCI.renderPass          = g_vk.renderPass;
    gpCI.subpass             = 0;
    VK_CHECK(vkCreateGraphicsPipelines(g_vk.device, VK_NULL_HANDLE, 1, &gpCI, nullptr, &g_vk.pipeline));
    LOGI("Graphics pipeline created");

    vkDestroyShaderModule(g_vk.device, vertMod, nullptr);
    vkDestroyShaderModule(g_vk.device, fragMod, nullptr);

    /* 13. Framebuffers */
    g_vk.framebuffers = (VkFramebuffer*)malloc(g_vk.imageCount * sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < g_vk.imageCount; i++) {
        VkFramebufferCreateInfo fbCI = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        fbCI.renderPass      = g_vk.renderPass;
        fbCI.attachmentCount = 1;
        fbCI.pAttachments    = &g_vk.imageViews[i];
        fbCI.width           = g_vk.swapExtent.width;
        fbCI.height          = g_vk.swapExtent.height;
        fbCI.layers          = 1;
        VK_CHECK(vkCreateFramebuffer(g_vk.device, &fbCI, nullptr, &g_vk.framebuffers[i]));
    }

    /* 14. Command pool + buffers */
    VkCommandPoolCreateInfo cpCI = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    cpCI.queueFamilyIndex = g_vk.graphicsFamily;
    cpCI.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(g_vk.device, &cpCI, nullptr, &g_vk.cmdPool));

    g_vk.cmdBuffers = (VkCommandBuffer*)malloc(g_vk.imageCount * sizeof(VkCommandBuffer));
    VkCommandBufferAllocateInfo cbAI = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cbAI.commandPool        = g_vk.cmdPool;
    cbAI.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbAI.commandBufferCount = g_vk.imageCount;
    VK_CHECK(vkAllocateCommandBuffers(g_vk.device, &cbAI, g_vk.cmdBuffers));

    /* 15. Sync objects */
    VkSemaphoreCreateInfo semCI = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo     fnCI  = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fnCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CHECK(vkCreateSemaphore(g_vk.device, &semCI, nullptr, &g_vk.imageAvailable));
    VK_CHECK(vkCreateSemaphore(g_vk.device, &semCI, nullptr, &g_vk.renderFinished));
    VK_CHECK(vkCreateFence    (g_vk.device, &fnCI,  nullptr, &g_vk.inFlight));

    /* 16. Pre-record command buffers */
    for (uint32_t i = 0; i < g_vk.imageCount; i++) {
        VkCommandBufferBeginInfo bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        vkBeginCommandBuffer(g_vk.cmdBuffers[i], &bi);

        VkClearValue clearVal = {};
        clearVal.color = {{0.1f, 0.1f, 0.1f, 1.0f}};  /* dark-grey background */

        VkRenderPassBeginInfo rpBI = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        rpBI.renderPass        = g_vk.renderPass;
        rpBI.framebuffer       = g_vk.framebuffers[i];
        rpBI.renderArea.extent = g_vk.swapExtent;
        rpBI.clearValueCount   = 1;
        rpBI.pClearValues      = &clearVal;

        vkCmdBeginRenderPass(g_vk.cmdBuffers[i], &rpBI, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(g_vk.cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, g_vk.pipeline);
        vkCmdDraw(g_vk.cmdBuffers[i], 3, 1, 0, 0);   /* 3 vertices = triangle */
        vkCmdEndRenderPass(g_vk.cmdBuffers[i]);
        vkEndCommandBuffer(g_vk.cmdBuffers[i]);
    }

    g_vk.ready = true;
    LOGI("=== Vulkan ready — triangle should be visible ===");
}

/* ───────────── Render frame ──────────────────────────────────────────────── */

static void renderFrame() {
    if (!g_vk.ready) return;

    vkWaitForFences(g_vk.device, 1, &g_vk.inFlight, VK_TRUE, UINT64_MAX);
    vkResetFences  (g_vk.device, 1, &g_vk.inFlight);

    uint32_t imgIdx;
    VkResult r = vkAcquireNextImageKHR(g_vk.device, g_vk.swapchain, UINT64_MAX,
                                       g_vk.imageAvailable, VK_NULL_HANDLE, &imgIdx);
    if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR) {
        LOGI("Swapchain out of date, skipping frame");
        return;
    }

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.waitSemaphoreCount   = 1;
    si.pWaitSemaphores      = &g_vk.imageAvailable;
    si.pWaitDstStageMask    = &waitStage;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &g_vk.cmdBuffers[imgIdx];
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores    = &g_vk.renderFinished;
    vkQueueSubmit(g_vk.graphicsQueue, 1, &si, g_vk.inFlight);

    VkPresentInfoKHR pi = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores    = &g_vk.renderFinished;
    pi.swapchainCount     = 1;
    pi.pSwapchains        = &g_vk.swapchain;
    pi.pImageIndices      = &imgIdx;
    vkQueuePresentKHR(g_vk.graphicsQueue, &pi);
}

/* ───────────── Android glue ──────────────────────────────────────────────── */

static void onAppCmd(struct android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (app->window) {
                LOGI("Window ready, initialising Vulkan...");
                vulkanInit(app);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            g_vk.ready = false;
            LOGI("Window terminated");
            break;
        default: break;
    }
}

void android_main(struct android_app* app) {
    app->onAppCmd = onAppCmd;

    while (true) {
        int events;
        struct android_poll_source* source;
        while (ALooper_pollAll(g_vk.ready ? 0 : -1, nullptr, &events,
                               (void**)&source) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) return;
        }
        renderFrame();
    }
}
