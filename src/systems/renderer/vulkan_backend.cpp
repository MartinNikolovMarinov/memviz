#include "systems/renderer/vulkan_backend.h"
#include "systems/renderer/renderer.h"

#include "basic.h"
#include "platform.h"
#include "error.h"

#include "systems/logger.h"

PRAGMA_WARNING_PUSH

PRAGMA_WARNING_SUPPRESS_ALL

#include <vulkan/vulkan_core.h>
#include <vulkan/vk_layer.h>

PRAGMA_WARNING_POP

PRAGMA_WARNING_PUSH

// For this file I do not care about old style casting:
DISABLE_GCC_AND_CLANG_WARNING(-Wold-style-cast)

namespace memviz {

using RendererCreateInfo = Renderer::CrateInfo;

template <typename T>
using TempStaticArr = core::ArrStatic<T, 254>;

using ExtPropsList = core::ArrStatic<VkExtensionProperties, 254>;
using LayerPropsList = core::ArrStatic<VkLayerProperties, 254>;

namespace {

// ------------------------------------------ BEGIN CONSTNATS ----------------------------------------------------------

constexpr const char* INST_EXTS[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(OS_MAC) && OS_MAC == 1
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#endif
#if defined(MEMVIZ_DEBUG) && MEMVIZ_DEBUG == 1
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
};
constexpr addr_size INST_EXTS_COUNT = CORE_C_ARRLEN(INST_EXTS);

#if defined(MEMVIZ_DEBUG) && MEMVIZ_DEBUG == 1
#define MEMVIZ_VALIDATION_LAYERS_ENABLED

constexpr const char* LAYERS[] = {
    "VK_LAYER_KHRONOS_validation",
    "VK_LAYER_KHRONOS_profiles"
};
constexpr addr_size LAYERS_COUNT = CORE_C_ARRLEN(LAYERS);

#endif

// ------------------------------------------ END CONSTNATS ------------------------------------------------------------

// ------------------------------------------ BEGIN RENDERER STATE -----------------------------------------------------

// ExtPropsList g_allSupportedInstExts;
LayerPropsList g_allSupportedInstLayers;

VkInstance g_instance = VK_NULL_HANDLE;

// ------------------------------------------ END RENDERER STATE -------------------------------------------------------

// ------------------------------------------ BEGIN STATIC FUNCTIONS ---------------------------------------------------

void createInstance(const RendererCreateInfo& rendererInfo);

// FIXME: Start Implementing these:

// ExtPropsList* getAllSupportedInstExtensions(bool useCache = true);
// void          logInstExtPropsList(const ExtPropsList& list);
// bool          checkSupportForInstExtension(const char* extensionName);

LayerPropsList* getAllSupportedInstLayers(bool invalidateCache = false);
void            logInstLayersList(const LayerPropsList& list);
bool            checkSupportForInstLayer(const char* name);

// GPUDeviceList* getAllSupportedPhysicalDevices(VkInstance instance, bool useCache = true);
// void           logPhysicalDevicesList(const GPUDeviceList& list);

[[nodiscard]] VkDebugUtilsMessengerEXT vulkanCreateDebugMessenger(VkInstance instance);
[[nodiscard]] VkDebugUtilsMessengerCreateInfoEXT defaultDebugMessengerInfo();
[[nodiscard]] VkResult wrap_vkCreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger
);
void wrap_vkDestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator
);

constexpr addr_size VERSION_BUFFER_SIZE = 255;
void getVulkanVersion(char out[VERSION_BUFFER_SIZE]);
void logVulkanVersion();

// ------------------------------------------ END STATIC FUNCTIONS -----------------------------------------------------

} // namesspace

Error vulkanInit(RendererCreateInfo&& rendererInfo) {
    logVulkanVersion();
    logInstLayersList(*getAllSupportedInstLayers());

    createInstance(rendererInfo);

    return Error::OK;
}

void vulkanShutdown() {
    logInfoTagged(RENDERER_TAG, "Shutting down Vulkan renderer.");

    if (g_instance != VK_NULL_HANDLE) {
        logInfoTagged(RENDERER_TAG, "Destroying Vulkan instance");
        vkDestroyInstance(g_instance, nullptr);
        g_instance = VK_NULL_HANDLE;
    }
}

namespace {

void createInstance(const RendererCreateInfo& rendererInfo) {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = rendererInfo.appName;
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1); // TODO: set application version
    appInfo.pEngineName = rendererInfo.appName;
    appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    // Retrieve required extensions by the platform layer
    i32 pltExtCount = 0;
    Platform::requiredVulkanExtsCount(pltExtCount);
    const addr_size requiredExtCount = addr_size(pltExtCount) + INST_EXTS_COUNT;
    TempStaticArr<const char*> extensions (requiredExtCount, nullptr);
    Platform::requiredVulkanExts(extensions.data());

    // Add the rest of the extensions
    logInfoTagged(RENDERER_TAG, "Enable extensions:");
    for (addr_size i = 0; i < INST_EXTS_COUNT; i++) {
        const char* ex = INST_EXTS[i];
        extensions[addr_size(pltExtCount) + i] = ex;
        logInfoTagged(RENDERER_TAG, "\t{}", ex);
    }

    VkFlags instanceCreateInfoFlags = 0;
#if defined(OS_MAC) && OS_MAC == 1
        // Enable portability extension for MacOS.
        instanceCreateInfoFlags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.flags = instanceCreateInfoFlags;
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = u32(extensions.len());
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

#ifdef MEMVIZ_VALIDATION_LAYERS_ENABLED
    logInfoTagged(RENDERER_TAG, "Enabled layers:");
    for (addr_size i = 0; i < LAYERS_COUNT; i++) {
        const char* layer = LAYERS[i];
        if (checkSupportForInstLayer(layer)) {
            logInfoTagged(RENDERER_TAG, "\t{}", layer);
        }
        else {
            logErrTagged(RENDERER_TAG, "{} layer is not supported", layer);
            Panic(false, "Missing Vulkan Instance Layer"); // this should not happen
        }
    }

    instanceCreateInfo.enabledLayerCount = LAYERS_COUNT;
    instanceCreateInfo.ppEnabledLayerNames = LAYERS;

    // TODO2: Should this be enabled in release builds too ?
    // The following code is required to enable the debug utils extension during instance creation:
    static VkDebugUtilsMessengerCreateInfoEXT debugMessageInfo = defaultDebugMessengerInfo();
    instanceCreateInfo.pNext = &debugMessageInfo;
#endif

    VK_MUST(
        vkCreateInstance(&instanceCreateInfo, nullptr, &g_instance),
        "Failed to create VkInstance"
    );
}

LayerPropsList* getAllSupportedInstLayers(bool invalidateCache) {
    if (!invalidateCache && !g_allSupportedInstLayers.empty()) {
        return &g_allSupportedInstLayers;
    }

    u32 layerCount;
    VK_MUST(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

    auto layList = LayerPropsList(layerCount, VkLayerProperties{});
    VK_MUST(vkEnumerateInstanceLayerProperties(&layerCount, layList.data()));

    g_allSupportedInstLayers = std::move(layList);
    return &g_allSupportedInstLayers;
}

void logInstLayersList(const LayerPropsList& list) {
    logInfoTagged(RENDERER_TAG, "Layers ({})", list.len());
    for (addr_size i = 0; i < list.len(); i++) {
        logInfoTagged(RENDERER_TAG, "\tname: {}, description: {}, spec version: {}, impl version: {}",
                      list[i].layerName, list[i].description, list[i].specVersion, list[i].implementationVersion);
    }
}

bool checkSupportForInstLayer(const char* name) {
    const LayerPropsList& layers = *getAllSupportedInstLayers();
    for (addr_size i = 0; i < layers.len(); i++) {
        VkLayerProperties p = layers[i];
        if (core::memcmp(p.layerName, name, core::cstrLen(name)) == 0) {
            return true;
        }
    }

    return false;
}

[[nodiscard]] VkDebugUtilsMessengerEXT vulkanCreateDebugMessenger(VkInstance instance) {
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = defaultDebugMessengerInfo();
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkResult vres = wrap_vkCreateDebugUtilsMessengerEXT(instance,
                                                        &debugMessengerCreateInfo,
                                                        nullptr,
                                                        &debugMessenger);
    VK_MUST(vres, "Failed to Create Vulkan Debug Messenger");
    return debugMessenger;
}

[[nodiscard]] VkDebugUtilsMessengerCreateInfoEXT defaultDebugMessengerInfo() {
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};

    debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                                               // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

    debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    debugMessengerCreateInfo.pfnUserCallback = [](
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        [[maybe_unused]] void* pUserData
    ) -> VkBool32 {
        static constexpr addr_size DEBUG_MESSAGE_BUFFER_SIZE = 1024;
        char buffer[DEBUG_MESSAGE_BUFFER_SIZE];
        char* ptr = buffer;

        // Write message type to the buffer
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
            ptr += core::memcopy(ptr, "[GENERAL] ", core::cstrLen("[GENERAL] "));
        }
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
            ptr += core::memcopy(ptr, "[VALIDATION] ", core::cstrLen("[VALIDATION] "));
        }
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
            ptr += core::memcopy(ptr, "[PERFORMANCE] ", core::cstrLen("[PERFORMANCE] "));
        }

        // Write callback message to the buffer
        ptr += core::memcopy(ptr, pCallbackData->pMessage, core::cstrLen(pCallbackData->pMessage));

        // Null-terminate the buffer
        *ptr = '\0';

        // Log the message with the appropriate logger
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            logErrTagged(RENDERER_VALIDATION_TAG, buffer);
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            logWarnTagged(RENDERER_VALIDATION_TAG, buffer);
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            logInfoTagged(RENDERER_VALIDATION_TAG, buffer);
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
            logDebugTagged(RENDERER_VALIDATION_TAG, buffer);
        }

        return VK_FALSE;
    };

    return debugMessengerCreateInfo;
}

[[nodiscard]] VkResult wrap_vkCreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger
) {
    static PFN_vkCreateDebugUtilsMessengerEXT func = nullptr;

    if (func == nullptr) {
        func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
        if (func == nullptr) {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
}

void wrap_vkDestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator
) {
    static PFN_vkDestroyDebugUtilsMessengerEXT func = nullptr;

    if (func == nullptr) {
        func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (func == nullptr) {
            return;
        }
    }

    func(instance, debugMessenger, pAllocator);
}

void logVulkanVersion() {
    char buff[VERSION_BUFFER_SIZE];
    getVulkanVersion(buff);
    logInfoTagged(RENDERER_TAG, "Selected Renderer: {}", buff);
}

void getVulkanVersion(char out[VERSION_BUFFER_SIZE]) {
    u32 version = 0;
    VK_MUST(vkEnumerateInstanceVersion(&version), "Failed to get Vulkan Version");

    u32 n;

    out += core::memcopy(out, "Vulkan v", core::cstrLen("Vulkan v"));
    n = core::Unpack(core::intToCstr(VK_VERSION_MAJOR(version), out, VERSION_BUFFER_SIZE));
    out[n] = '.';
    out += n + 1;

    n = core::Unpack(core::intToCstr(VK_VERSION_MINOR(version), out, VERSION_BUFFER_SIZE));
    out[n] = '.';
    out += n + 1;

    n = core::Unpack(core::intToCstr(VK_VERSION_PATCH(version), out, VERSION_BUFFER_SIZE));
    out[n] = '\0';
    out += n;
}

} // namespace

Error (*Renderer::init)(Renderer::CrateInfo&&) = vulkanInit;
void (*Renderer::shutdown)(void) = vulkanShutdown;

} // memviz

PRAGMA_WARNING_POP
