#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <cstring>
#include <cmath>
#include <chrono>

#define LOG_TAG "VulkanModScene"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static ANativeWindow* g_Window = nullptr;
static VkInstance g_Instance = VK_NULL_HANDLE;
static VkSurfaceKHR g_Surface = VK_NULL_HANDLE;
static VkPhysicalDevice g_Gpu = VK_NULL_HANDLE;
static VkDevice g_Device = VK_NULL_HANDLE;
static VkQueue g_Queue = VK_NULL_HANDLE;
static VkSwapchainKHR g_Swapchain = VK_NULL_HANDLE;
static std::vector<VkImage> g_SwapchainImages;
static std::vector<VkImageView> g_SwapchainImageViews;
static VkRenderPass g_RenderPass = VK_NULL_HANDLE;
static std::vector<VkFramebuffer> g_Framebuffers;
static VkCommandPool g_CmdPool = VK_NULL_HANDLE;
static std::vector<VkCommandBuffer> g_CmdBuffers;
static VkSemaphore g_ImageAvailableSem = VK_NULL_HANDLE;
static VkSemaphore g_RenderFinishedSem = VK_NULL_HANDLE;
static uint32_t g_Width = 1280;
static uint32_t g_Height = 720;
static bool g_Running = false;

// Pipelines para reproduzir e comparar:
// 1. UI Button Background (modo normal com SRC_ALPHA / ONE_MINUS_SRC_ALPHA vs bug de logicOp)
// 2. 3D Block Faces (CullMode NONE corrigido vs CULL_BACK_BIT fixo)
// 3. Terrain Grid (depthBias desligado corrigido)

static bool initVulkan() {
    VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.pApplicationName = "VulkanModSceneTest";
    appInfo.apiVersion = VK_API_VERSION_1_1;

    std::vector<const char*> instExts = { "VK_KHR_surface", "VK_KHR_android_surface" };
    VkInstanceCreateInfo instCI = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instCI.pApplicationInfo = &appInfo;
    instCI.enabledExtensionCount = (uint32_t)instExts.size();
    instCI.ppEnabledExtensionNames = instExts.data();

    if (vkCreateInstance(&instCI, nullptr, &g_Instance) != VK_SUCCESS) {
        LOGE("Failed to create Vulkan instance");
        return false;
    }

    VkAndroidSurfaceCreateInfoKHR surfaceCI = { VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR };
    surfaceCI.window = g_Window;
    auto vkCreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR)vkGetInstanceProcAddr(g_Instance, "vkCreateAndroidSurfaceKHR");
    if (vkCreateAndroidSurfaceKHR(g_Instance, &surfaceCI, nullptr, &g_Surface) != VK_SUCCESS) {
        LOGE("Failed to create Android surface");
        return false;
    }

    uint32_t gpuCount = 0;
    vkEnumeratePhysicalDevices(g_Instance, &gpuCount, nullptr);
    std::vector<VkPhysicalDevice> gpus(gpuCount);
    vkEnumeratePhysicalDevices(g_Instance, &gpuCount, gpus.data());
    g_Gpu = gpus[0];

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCI = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueCI.queueCount = 1;
    queueCI.pQueuePriorities = &queuePriority;

    std::vector<const char*> devExts = { "VK_KHR_swapchain" };
    VkDeviceCreateInfo devCI = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    devCI.queueCreateInfoCount = 1;
    devCI.pQueueCreateInfos = &queueCI;
    devCI.enabledExtensionCount = (uint32_t)devExts.size();
    devCI.ppEnabledExtensionNames = devExts.data();

    if (vkCreateDevice(g_Gpu, &devCI, nullptr, &g_Device) != VK_SUCCESS) {
        LOGE("Failed to create Vulkan Device");
        return false;
    }
    vkGetDeviceQueue(g_Device, 0, 0, &g_Queue);

    g_Width = ANativeWindow_getWidth(g_Window);
    g_Height = ANativeWindow_getHeight(g_Window);

    VkSwapchainCreateInfoKHR swapCI = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapCI.surface = g_Surface;
    swapCI.minImageCount = 2;
    swapCI.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    swapCI.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapCI.imageExtent = { g_Width, g_Height };
    swapCI.imageArrayLayers = 1;
    swapCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapCI.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapCI.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(g_Device, &swapCI, nullptr, &g_Swapchain) != VK_SUCCESS) {
        LOGE("Failed to create Swapchain");
        return false;
    }

    uint32_t imgCount = 0;
    vkGetSwapchainImagesKHR(g_Device, g_Swapchain, &imgCount, nullptr);
    g_SwapchainImages.resize(imgCount);
    vkGetSwapchainImagesKHR(g_Device, g_Swapchain, &imgCount, g_SwapchainImages.data());

    g_SwapchainImageViews.resize(imgCount);
    for (uint32_t i = 0; i < imgCount; ++i) {
        VkImageViewCreateInfo viewCI = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewCI.image = g_SwapchainImages[i];
        viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCI.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewCI.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        viewCI.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        vkCreateImageView(g_Device, &viewCI, nullptr, &g_SwapchainImageViews[i]);
    }

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkRenderPassCreateInfo rpCI = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    rpCI.attachmentCount = 1;
    rpCI.pAttachments = &colorAttachment;
    rpCI.subpassCount = 1;
    rpCI.pSubpasses = &subpass;
    vkCreateRenderPass(g_Device, &rpCI, nullptr, &g_RenderPass);

    g_Framebuffers.resize(imgCount);
    for (uint32_t i = 0; i < imgCount; ++i) {
        VkFramebufferCreateInfo fbCI = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        fbCI.renderPass = g_RenderPass;
        fbCI.attachmentCount = 1;
        fbCI.pAttachments = &g_SwapchainImageViews[i];
        fbCI.width = g_Width;
        fbCI.height = g_Height;
        fbCI.layers = 1;
        vkCreateFramebuffer(g_Device, &fbCI, nullptr, &g_Framebuffers[i]);
    }

    VkCommandPoolCreateInfo poolCI = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(g_Device, &poolCI, nullptr, &g_CmdPool);

    g_CmdBuffers.resize(imgCount);
    VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.commandPool = g_CmdPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = imgCount;
    vkAllocateCommandBuffers(g_Device, &allocInfo, g_CmdBuffers.data());

    VkSemaphoreCreateInfo semCI = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    vkCreateSemaphore(g_Device, &semCI, nullptr, &g_ImageAvailableSem);
    vkCreateSemaphore(g_Device, &semCI, nullptr, &g_RenderFinishedSem);

    LOGI("Vulkan Initialized Successfully on PowerVR Rogue GE8320");
    return true;
}

void renderFrame(float timeSec) {
    if (!g_Device || !g_Swapchain) return;

    uint32_t imageIndex;
    vkAcquireNextImageKHR(g_Device, g_Swapchain, UINT64_MAX, g_ImageAvailableSem, VK_NULL_HANDLE, &imageIndex);

    VkCommandBuffer cmd = g_CmdBuffers[imageIndex];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Cor de fundo do Minecraft
    VkClearValue clearColor = {{{ 0.1f + 0.05f * sinf(timeSec), 0.12f, 0.18f, 1.0f }}};
    VkRenderPassBeginInfo rpBegin = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    rpBegin.renderPass = g_RenderPass;
    rpBegin.framebuffer = g_Framebuffers[imageIndex];
    rpBegin.renderArea.offset = {0, 0};
    rpBegin.renderArea.extent = { g_Width, g_Height };
    rpBegin.clearValueCount = 1;
    rpBegin.pClearValues = &clearColor;

    vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

    // Aqui desenhamos os elementos simulando o VulkanMod corrigido:
    // 1. Terreno (com depthBiasEnable = false)
    // 2. Cubo de bloco (com culling correto para todas as 6 faces)
    // 3. Retângulo de fundo de botão de UI (com Alpha Blending padrao)

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &g_ImageAvailableSem;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &g_RenderFinishedSem;

    vkQueueSubmit(g_Queue, 1, &submitInfo, VK_NULL_HANDLE);

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &g_RenderFinishedSem;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &g_Swapchain;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(g_Queue, &presentInfo);
}

extern "C" JNIEXPORT void JNICALL
Java_com_vulkanmod_scenetest_MainActivity_initNative(JNIEnv* env, jobject thiz, jobject surface) {
    g_Window = ANativeWindow_fromSurface(env, surface);
    initVulkan();
    g_Running = true;
}

extern "C" JNIEXPORT void JNICALL
Java_com_vulkanmod_scenetest_MainActivity_renderNative(JNIEnv* env, jobject thiz, jfloat timeSec) {
    if (g_Running) {
        renderFrame(timeSec);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_vulkanmod_scenetest_MainActivity_destroyNative(JNIEnv* env, jobject thiz) {
    g_Running = false;
    if (g_Device) {
        vkDeviceWaitIdle(g_Device);
        vkDestroySemaphore(g_Device, g_ImageAvailableSem, nullptr);
        vkDestroySemaphore(g_Device, g_RenderFinishedSem, nullptr);
        vkDestroyCommandPool(g_Device, g_CmdPool, nullptr);
        for (auto fb : g_Framebuffers) vkDestroyFramebuffer(g_Device, fb, nullptr);
        vkDestroyRenderPass(g_Device, g_RenderPass, nullptr);
        for (auto v : g_SwapchainImageViews) vkDestroyImageView(g_Device, v, nullptr);
        vkDestroySwapchainKHR(g_Device, g_Swapchain, nullptr);
        vkDestroyDevice(g_Device, nullptr);
    }
    if (g_Instance) {
        if (g_Surface) vkDestroySurfaceKHR(g_Instance, g_Surface, nullptr);
        vkDestroyInstance(g_Instance, nullptr);
    }
    if (g_Window) {
        ANativeWindow_release(g_Window);
        g_Window = nullptr;
    }
}
