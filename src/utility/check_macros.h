#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <iostream>
#include <cstdlib>

// Checks a VkResult, printing if there is an error and returning false
//   (for use in functions that return bool for success)
#define VK_CHECK(x)                                           \
    do {                                                      \
        VkResult err = x;                                     \
        if (err != VK_SUCCESS) {                              \
            std::cerr << "VK ERROR @ ["                       \
                      << __FILE__ << ":" << __LINE__ << "]: " \
                      << string_VkResult(err)                 \
                      << std::endl;                           \
            return false;                                     \
        }                                                     \
    } while (0)

// Checks a vk-bootstrap result, running a with the result if a value isn't present
//   (for use with vkb results in functions that return bool for success)
#define VKB_CHECK(x)                                          \
    do {                                                      \
        if (!x.has_value()) {                                 \
            std::cerr << "VKB ERROR @ ["                      \
                      << __FILE__ << ":" << __LINE__ << "]: " \
                      << x.error().message()                  \
                      << std::endl;                           \
            return false;                                     \
        }                                                     \
    } while (0)

// Checks a VkResult, printing if there is an error and aborts
//   (for use in situations where you can't return a boolean, like a ctor)
#define VK_CHECK_ABORT(x)                                     \
    do {                                                      \
        VkResult err = x;                                     \
        if (err != VK_SUCCESS) {                              \
            std::cerr << "VK ERROR @ ["                       \
                      << __FILE__ << ":" << __LINE__ << "]: " \
                      << string_VkResult(err)                 \
                      << std::endl;                           \
            std::abort();                                     \
        }                                                     \
    } while (0)

// Checks a boolean value and returns false if it is false
//   (for use when executing bool-returning init functions)
#define BOOL_CHECK(x)                                   \
    if (x == false) {                                   \
        std::cerr << "BOOL ERROR @ ["           \
                  << __FILE__ << ":" << __LINE__ << "]" \
                  << std::endl;                         \
        return false;                                   \
    }
