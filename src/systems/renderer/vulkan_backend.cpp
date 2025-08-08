#include "systems/renderer/vulkan_backend.h"
#include "systems/renderer/renderer.h"

#include "basic.h"
#include "systems/logger.h"

#include <error.h>

namespace memviz {

using RendererCreateInfo = Renderer::CrateInfo;

namespace {

constexpr addr_size VERSION_BUFFER_SIZE = 255;

void getVulkanVersion(char out[VERSION_BUFFER_SIZE]);
void logVulkanVersion();

} // namesspace

Error vulkanInit(RendererCreateInfo&&) {
    logVulkanVersion();

    return Error::OK;
}

Error (*Renderer::init)(Renderer::CrateInfo&&) = vulkanInit;

namespace {

void logVulkanVersion() {
    char buff[VERSION_BUFFER_SIZE];
    getVulkanVersion(buff);
    logInfoTagged(RENDERER_TAG, "Selected Renderer: {}", buff);
}

void getVulkanVersion(char out[VERSION_BUFFER_SIZE]) {
    u32 version = 0;
    VK_MUST(vkEnumerateInstanceVersion(&version));

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

} // memviz
