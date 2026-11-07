#pragma once

// Must be defined before vulkan.hpp is seen (and when building the vulkan_hpp module).
#ifndef VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#   define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#endif

#if !defined(USE_CPP20_MODULES)
#   include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
