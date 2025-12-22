#include "graphics_manager.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstring>
#include <string>
#include <optional>

// TODO: this: https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Window_surface

constexpr bool enable_validation_layers = true;
const std::vector<const char*> validation_layers = {
    "VK_LAYER_KHRONOS_validation"
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;

    bool is_complete() {
        return graphics.has_value();
    }
};

#pragma region // Helper functions!

QueueFamilyIndices find_queue_families(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    // find the indices of the returned families and store !!!
    for (uint32_t i = 0; i < queue_family_count; i++) {
        if ((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            indices.graphics = i;
        }

        if (indices.is_complete()) {
            break;
        }
    }

    return indices;
}

static bool is_device_suitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = find_queue_families(device);

    return indices.is_complete();
}

#pragma endregion

#pragma region // Class definitions <3

GraphicsManager::GraphicsManager() {
    CreateInstance();
    PickPhysicalDevice();
    CreateLogicalDevice();
}

GraphicsManager::~GraphicsManager() {
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void GraphicsManager::CreateInstance() {
    VkApplicationInfo app_info {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Triangle 3 <3",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine :O",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_4
    };

    // ~~~ vulkan validation layers ~~~

    std::vector<const char*> layers_to_use;
    if (enable_validation_layers) {
        uint32_t layer_count = 0;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        // make sure that each validation layer we wannna use is
        //   actually in the supported layer properties list
        for (const char* layer_name : validation_layers) {
            bool found = false;

            for (const auto& layer_properties : available_layers) {
                if (strcmp(layer_name, layer_properties.layerName) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                throw std::runtime_error("Validation layer requested but not available: " + std::string(layer_name));
            }
        }

        layers_to_use.assign(validation_layers.begin(), validation_layers.end());
    }

    // ~~~ GLFW instance extensions ~~~

    // get GLFW extension
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    // make sure extensions are supported
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

    for (uint32_t i = 0; i < glfw_extension_count; i++) {
        bool found = false;
        for (const auto& extension : extensions) {
            if (strcmp(glfw_extensions[i], extension.extensionName) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            throw std::runtime_error("Required GLFW extension not supported: " + std::string(glfw_extensions[i]));
        }
    }

    // ~~~ create instance itself ~~~

    VkInstanceCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(layers_to_use.size()),
        .ppEnabledLayerNames = layers_to_use.data(),
        .enabledExtensionCount = glfw_extension_count,
        .ppEnabledExtensionNames = glfw_extensions
    };

    if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create instance!");
    }
}

void GraphicsManager::PickPhysicalDevice() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

    if (device_count == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

    for (const auto& device : devices) {
        if (is_device_suitable(device)) {
            physical_device = device;
            break;
        }
    }

    if (physical_device == nullptr) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

void GraphicsManager::CreateLogicalDevice() {
    QueueFamilyIndices indices = find_queue_families(physical_device);

    // priorities are used to influence scheduling order, inchresting
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = indices.graphics.value(),
        .queueCount = 1,
        .pQueuePriorities = &queue_priority
    };

    // we'll come back to this when we wanna add more fun features to the program
    VkPhysicalDeviceFeatures device_features {};

    VkDeviceCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledLayerCount = 0,
        .enabledExtensionCount = 0,
        .pEnabledFeatures = &device_features,
    };

    // we don't rlly need this with newer versions of vulkan since
    //   instance validation layers kinda replaced everything
    //   else but we keep it just to be compatible for old versions
    if (enable_validation_layers) {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
    }

    if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device!");
    }

    // aaaaand store the queues from the device we just created :D
    vkGetDeviceQueue(device, indices.graphics.value(), 0, &graphics_queue);
}

#pragma endregion
