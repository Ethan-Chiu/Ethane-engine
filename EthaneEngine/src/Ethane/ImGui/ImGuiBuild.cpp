#include "ethpch.h"

// Dear ImGui platform / renderer backends, compiled into the engine (the ImGui project builds the core).
// Vulkan first: the GLFW backend declares a placeholder VkResult unless vulkan.h is already included.
#include "backends/imgui_impl_vulkan.cpp"
#include "backends/imgui_impl_glfw.cpp"
