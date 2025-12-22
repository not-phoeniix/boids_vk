#include "graphics_manager.h"
#include <GLFW/glfw3.h>
#include <iostream>

// https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/01_Instance.html

constexpr bool enable_validation_layers = true;
const std::vector<const char*> valication_layers = {
    "VK_LAYER_KHRONOS_validation"
};

#pragma region // Helper functions!

static bool is_device_suitable(vk::raii::PhysicalDevice device) {
    auto device_properties = device.getProperties();
    auto device_features = device.getFeatures();
    auto queue_families = device.getQueueFamilyProperties();
    
    // for (const auto& device : device)
    
    // if (device_properties)
}

#pragma endregion

#pragma region // Class definitions <3

GraphicsManager::GraphicsManager() {
    CreateInstance();
}

GraphicsManager::~GraphicsManager() {
}

void GraphicsManager::CreateInstance() {
    constexpr vk::ApplicationInfo app_info {
        .pApplicationName = "Trangle 3 <3",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine :O",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion14
    };

    // ~~~ vulkan validation layers ~~~

    std::vector<const char*> required_layers;
    if (enable_validation_layers) {
        required_layers.assign(valication_layers.begin(), valication_layers.end());
    }

    auto layer_properties = context.enumerateInstanceLayerProperties();
    if (std::ranges::any_of(required_layers, [&layer_properties](auto const& required_layer) {
            return std::ranges::none_of(layer_properties, [&required_layer](auto const& layer_property) {
                return strcmp(layer_property.layerName, required_layer) == 0;
            });
        })) {
        throw std::runtime_error("One or more required validation layers are not supported!");
    }

    // ~~~ GLFW instance extensions ~~~

    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    auto extension_properties = context.enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < glfw_extension_count; i++) {
        if (
            std::ranges::none_of(
                extension_properties,
                [glfw_extension = glfw_extensions[i]](auto const& extension_property) {
                    return strcmp(extension_property.extensionName, glfw_extension) == 0;
                }
            )
        ) {
            throw std::runtime_error("Required GLFW extension not supported: " + std::string(glfw_extensions[i]));
        }
    }

    // ~~~ create instance itself ~~~

    vk::InstanceCreateInfo create_info {
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(required_layers.size()),
        .ppEnabledLayerNames = required_layers.data(),
        .enabledExtensionCount = glfw_extension_count,
        .ppEnabledExtensionNames = glfw_extensions
    };

    instance = vk::raii::Instance(context, create_info);
}

void GraphicsManager::PickPhysicalDevice() {
    auto devices = instance.enumeratePhysicalDevices();

    if (devices.empty()) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    for (const auto& device : devices) {
        auto device_props = device.getProperties();
        auto device_features = device.getFeatures();

        physical_device = device;
        break;
    }
}

#pragma endregion
